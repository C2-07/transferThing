#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/uio.h> // macOS: sendfile(fd, sockfd, offset, len, etc)
#elif defined(__linux__)
#include <sys/sendfile.h> // Linux: sendfile(sockfd, fd, offset, count)
#endif

#define PORT "6969"
#define BUFFER_SIZE (1024 * 512) // 512 KB buffer size
#define LOOPBACK "127.0.0.1"

// Style Constants
#define COLOR_BLUE "\033[94m"
#define COLOR_GREEN "\033[92m"
#define COLOR_YELLOW "\033[93m"
#define COLOR_RED "\033[91m"
#define COLOR_BOLD "\033[1m"
#define COLOR_END "\033[0m"

// Struct for target host info
typedef struct {
  char *hostname;
  char *port;
} Host;

// Struct for sending file metadata before the actual data
typedef struct {
  char filename[256];
  off_t size;
  mode_t mode;
} FileMetadata;

// Function declarations
void setupAddrInfo(struct addrinfo **result, const Host *host);
void createSocket(int *sockfd, const struct addrinfo *info);
void startClientConnection(int sockfd, const struct addrinfo *info);
void startServerConnection(int sockfd, const struct addrinfo *info);
void sendFile(const char *filepath, int sockfd);
void receiveFile(int sockfd);
char *getLocalIp(char *buffer, size_t buffer_size);
void showUsageAndExit();

// Main logic: determines mode (server/client), sets up networking, then
// sends/receives
int main(int argc, char *argv[]) {
  if (argc < 2)
    showUsageAndExit();

  bool isServer = true;
  Host host = {NULL, PORT};
  struct addrinfo *addrinfoList = NULL;
  int sockfd;

  if (strcmp(argv[1], "recv") == 0) {
    isServer = false;
    host.hostname = argv[2]; // Set remote host IP
  }

  setupAddrInfo(&addrinfoList, &host);
  createSocket(&sockfd, addrinfoList);

  if (isServer) {
    // Start server: bind, listen, accept
    startServerConnection(sockfd, addrinfoList);
    int clientFd =
        accept(sockfd, addrinfoList->ai_addr, &addrinfoList->ai_addrlen);
    if (clientFd < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    sendFile(argv[1], clientFd);
    close(clientFd);
  } else {
    // Start client: connect and receive
    startClientConnection(sockfd, addrinfoList);
    receiveFile(sockfd);
  }

  freeaddrinfo(addrinfoList);
  close(sockfd);
  return EXIT_SUCCESS;
}

// Setup address info using getaddrinfo (supports localhost/IP/any)
void setupAddrInfo(struct addrinfo **result, const Host *host) {
  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = (host->hostname == NULL) ? AI_PASSIVE : 0;

  int status = getaddrinfo(host->hostname, host->port, &hints, result);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }
}

// Create TCP socket using resolved address info
void createSocket(int *sockfd, const struct addrinfo *info) {
  *sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (*sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
}

// Server binds, prints IP, and listens
void startServerConnection(int sockfd, const struct addrinfo *info) {
  if (bind(sockfd, info->ai_addr, info->ai_addrlen) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  char ip[INET_ADDRSTRLEN];
  printf("Server is listening on %s%s:%s%s\n", COLOR_YELLOW,
         getLocalIp(ip, sizeof(ip)), PORT, COLOR_END);

  if (listen(sockfd, 8) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

// Client connects to server
void startClientConnection(int sockfd, const struct addrinfo *info) {
  if (connect(sockfd, info->ai_addr, info->ai_addrlen) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
  printf("Connected to server.\n");
}

// Send file metadata and file content using platform-specific methods
void sendFile(const char *filepath, int sockfd) {
  int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    perror("open (sendFile)");
    return;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    perror("fstat");
    close(fd);
    return;
  }

  FileMetadata metadata = {0};
  strncpy(metadata.filename, filepath, sizeof(metadata.filename) - 1);
  metadata.size = st.st_size;
  metadata.mode = st.st_mode;

  // Send metadata before file content
  send(sockfd, &metadata, sizeof(metadata), 0);

#if defined(__APPLE__)
  // macOS sendfile: offset is by value, not pointer
  off_t offset = 0;
  if (sendfile(fd, sockfd, offset, &metadata.size, NULL, 0) < 0) {
    perror("sendfile (macOS)");
  }
#elif defined(__linux__)
  // Linux sendfile: offset is a pointer
  off_t offset = 0;
  while (offset < st.st_size) {
    ssize_t sent = sendfile(sockfd, fd, &offset, st.st_size - offset);
    if (sent <= 0) {
      perror("sendfile (Linux)");
      break;
    }
  }
#else
  // Fallback for platforms like Windows, Android, etc.
  char buffer[BUFFER_SIZE];
  ssize_t readBytes;
  while ((readBytes = read(fd, buffer, sizeof(buffer))) > 0) {
    ssize_t totalSent = 0;
    while (totalSent < readBytes) {
      ssize_t sent = send(sockfd, buffer + totalSent, readBytes - totalSent, 0);
      if (sent < 0) {
        perror("send (fallback)");
        break;
      }
      totalSent += sent;
    }
  }
#endif

  printf("[%s✓%s] %s sent successfully\n", COLOR_GREEN, COLOR_END,
         metadata.filename);
  close(fd);
}

// Receive file metadata and save incoming data to disk
void receiveFile(int sockfd) {
  FileMetadata metadata = {0};

  ssize_t receivedMeta = recv(sockfd, &metadata, sizeof(metadata), 0);
  if (receivedMeta != sizeof(metadata)) {
    perror("recv (metadata)");
    return;
  }

  printf("%s Receiving file: %s%s\n", COLOR_BLUE, metadata.filename, COLOR_END);

  int fd = open(metadata.filename, O_CREAT | O_WRONLY | O_TRUNC, metadata.mode);
  if (fd < 0) {
    perror("open (receiveFile)");
    return;
  }

  off_t received = 0;
  char buffer[BUFFER_SIZE];

  while (received < metadata.size) {
    ssize_t bytes = recv(sockfd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
      break;
    write(fd, buffer, bytes);
    received += bytes;
  }

  printf("[%s✓%s] Received: %s (%lld bytes)\n", COLOR_GREEN, COLOR_END,
         metadata.filename, (long long)metadata.size);
  close(fd);
}

// Print usage help and exit
void showUsageAndExit() {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr,
          "  ./transferThing <filename>        # Send file as server\n");
  fprintf(stderr,
          "  ./transferThing recv <host>       # Receive file as client\n");
  exit(EXIT_FAILURE);
}

// Get local IP address using gethostname + getaddrinfo
char *getLocalIp(char *buffer, size_t buffer_size) {
  char hostname[256];
  struct addrinfo hints, *res, *p;

  if (gethostname(hostname, sizeof(hostname)) != 0) {
    perror("gethostname");
    snprintf(buffer, buffer_size, LOOPBACK);
    return buffer;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
    snprintf(buffer, buffer_size, LOOPBACK);
    return buffer;
  }

  for (p = res; p != NULL; p = p->ai_next) {
    struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, buffer, buffer_size);
    if (strcmp(buffer, LOOPBACK) == 0)
      continue;
    break; // Only take the first match
  }

  freeaddrinfo(res);
  return buffer;
}
