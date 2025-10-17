#include "transfer.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT "8989"
#define BUFFER_SIZE 1024 * 512 // 512 KB buffer Size

/* Function : initMetaData
 * Param : *fp, *metadata
 *  fp -> file path
 *  metadata -> metadata of the file
 */
int share(const char *fp) {
  int fd = open(fp, O_RDONLY);
  if (fd < 0) {
    perror("open");
    return -1;
  }
  FileMetaData *metadata = {0};
  struct stat *st = NULL;
  fstat(fd, st);
  strncpy(metadata->filename, fp, sizeof(*fp));
  metadata->mode = st->st_mode;
  metadata->size = st->st_size;

  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // only used for server side sockets

  int err = getaddrinfo("", PORT, &hints, &res);
  if (err < 0) {
    // gai_strerror -> get address info string error
    fprintf(stderr, "%s", gai_strerror(err));
    return -1;
  }
  int sockfd;
  if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) <
      0) {
    perror("socket");
    return -1;
  }
  if (bind(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
    perror("bind");
    return -1;
  }
  if (listen(sockfd, 8) != 0) {
    perror("listen");
    return -1;
  }

  int clientfd;
  if ((clientfd = accept(sockfd, res->ai_addr, &res->ai_addrlen)) < 0) {
    perror("accept");
    return -1;
  }

  // Send metadata before file content
  if (send(clientfd, metadata, sizeof(*metadata), 0)) {
    perror("send");
    return -1;
  }

// macOS sendfile: offset is by value, not pointer
#if defined(__APPLE__)
  off_t offset = 0;
  if (sendfile(fd, sockfd, offset, &metadata->size, NULL, 0) < 0) {
    perror("sendfile (macOS)");
    return -1;
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
  // Windows : Yes this piece of shit needs to be covered too :(
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
  close(sockfd);
  return 0;
}

int recieve(char *server_addr) {
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int err = getaddrinfo(server_addr, PORT, &hints, &res);
  if (err < 0) {
    fprintf(stderr, "%s", gai_strerror(err));
    return -1;
  }

  int sockfd;
  if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) <
      0) {
    perror("socket");
    return -1;
  }
  if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
    perror("connect");
    return -1;
  }

  FileMetaData metadata = {0};
  ssize_t receivedMeta = recv(sockfd, &metadata, sizeof(metadata), 0);
  if (receivedMeta != sizeof(metadata)) {
    perror("recv (metadata)");
    return -1;
  }
  printf("%s Receiving file: %s%s\n", COLOR_BLUE, metadata.filename, COLOR_END);

  int fd = open(metadata.filename, O_CREAT | O_WRONLY | O_TRUNC, metadata.mode);
  if (fd < 0) {
    perror("open (recv file)");
    return -1;
  }

  off_t recieved = 0;
  char buffer[BUFFER_SIZE];
  while (recieved < metadata.size) {
    ssize_t bytes = recv(sockfd, &buffer, sizeof(buffer), 0);
    if (bytes <= 0) {
      break;
    }
    write(fd, buffer, bytes);
    recieved += bytes;
  }
  printf("[%s✓%s] Received: %s (%lld bytes)\n", COLOR_GREEN, COLOR_END,
         metadata.filename, (long long)metadata.size);

  close(fd);
  return 0;
}
