#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#if defined(__APPLE__)
#include <sys/uio.h>  // for sendfile on macOS
#elif defined(__linux__)
#include <sys/sendfile.h>  // for Linux sendfile
#endif


#define HOSTNAME "localhost"
#define PORT "6969"
#define BUFFER_SIZE (1024 * 512)

typedef struct {
  char *hostname;
  char *port;
} Host;

typedef struct {
  char filename[256];
  off_t size;
  mode_t mode;
} FileMetadata;

void setupAddrInfo(struct addrinfo **result, const Host *host);
void createSocket(int *sockfd, const struct addrinfo *info);
void startClientConnection(int sockfd, const struct addrinfo *info);
void startServerConnection(int sockfd, const struct addrinfo *info);
void sendFile(const char *filepath, int sockfd);
void receiveFile(int sockfd);
void showUsageAndExit();

int main(int argc, char *argv[]) {
  if (argc < 2) {
    showUsageAndExit();
  }

  bool isServer = true;
  Host host = {NULL, PORT};
  struct addrinfo *addrinfoList = NULL;
  int sockfd;

  if (strcmp(argv[1], "recv") == 0) {
    isServer = false;
    host.hostname = argv[2];
  }

  setupAddrInfo(&addrinfoList, &host);
  createSocket(&sockfd, addrinfoList);

  if (isServer) {
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
    startClientConnection(sockfd, addrinfoList);
    receiveFile(sockfd);
  }

  freeaddrinfo(addrinfoList);
  close(sockfd);
  return EXIT_SUCCESS;
}

void setupAddrInfo(struct addrinfo **result, const Host *host) {
  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (host->hostname == NULL) {
    hints.ai_flags = AI_PASSIVE;
  }

  int status = getaddrinfo(host->hostname, host->port, &hints, result);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }
}

void createSocket(int *sockfd, const struct addrinfo *info) {
  *sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (*sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
}

void startServerConnection(int sockfd, const struct addrinfo *info) {
  if (bind(sockfd, info->ai_addr, info->ai_addrlen) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(sockfd, 8) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

void startClientConnection(int sockfd, const struct addrinfo *info) {
  if (connect(sockfd, info->ai_addr, info->ai_addrlen) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
}

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

  send(sockfd, &metadata, sizeof(metadata), 0);

#if defined(__APPLE__)
  off_t offset = 0;
  if (sendfile(fd, sockfd, offset, &metadata.size, NULL, 0) < 0) {
    perror("sendfile (macOS)");
  }
#elif defined(__linux__)
  off_t offset = 0;
  ssize_t sent;
  while (offset < st.st_size) {
    sent = sendfile(sockfd, fd, &offset, st.st_size - offset);
    if (sent <= 0) {
      perror("sendfile (Linux)");
      break;
    }
  }
#else
  // Fallback for platforms like Android/Windows with no sendfile
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

  close(fd);
}


void receiveFile(int sockfd) {
  FileMetadata metadata = {0};

  ssize_t receivedMeta = recv(sockfd, &metadata, sizeof(metadata), 0);
  if (receivedMeta != sizeof(metadata)) {
    perror("recv (metadata)");
    return;
  }

  printf("Receiving file: %s\n", metadata.filename);

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

  printf("[+] Received: %s (%lld bytes)\n", metadata.filename,
         (long long)metadata.size);
  close(fd);
}

void showUsageAndExit() {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr,
          "  ./transferThing <filename>        # Send file as server\n");
  fprintf(stderr,
          "  ./transferThing recv <host>       # Receive file as client\n");
  exit(EXIT_FAILURE);
}
