#include "transfer.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/uio.h>
#elif defined(__linux__)
#include <sys/sendfile.h>
#endif

#define PORT "8989"
#define BUFFER_SIZE (1024 * 512) // 512KB

// ------------------------- SHARE FUNCTION -------------------------
int share(const char *filepath) {
    if (!filepath) {
        fprintf(stderr, "No file path provided.\n");
        return -1;
    }

    printf("%s[INFO]%s Preparing to share file: %s\n", COLOR_BLUE, COLOR_END, filepath);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    // Extract filename from full path
    const char *filename = strrchr(filepath, '/');
    filename = filename ? filename + 1 : filepath;

    FileMetaData metadata = {0};
    strncpy(metadata.filename, filename, sizeof(metadata.filename) - 1);
    metadata.size = st.st_size;
    metadata.mode = st.st_mode;

    // TCP setup
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        close(fd);
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
        perror("bind");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    if (listen(sockfd, 1) != 0) {
        perror("listen");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    printf("%s[WAIT]%s Waiting for receiver to connect on port %s...\n", COLOR_YELLOW, COLOR_END, PORT);

    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);
    int clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (clientfd < 0) {
        perror("accept");
        freeaddrinfo(res);
        close(fd);
        close(sockfd);
        return -1;
    }

    // Send metadata first
    if (send(clientfd, &metadata, sizeof(metadata), 0) != sizeof(metadata)) {
        perror("send metadata");
        close(fd);
        close(clientfd);
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    printf("%s[SENDING]%s File: %s (%lld bytes)\n", COLOR_BLUE, COLOR_END,
           metadata.filename, (long long)metadata.size);

#if defined(__APPLE__)
    off_t offset = 0;
    if (sendfile(fd, clientfd, offset, &metadata.size, NULL, 0) < 0)
        perror("sendfile (macOS)");
#elif defined(__linux__)
    off_t offset = 0;
    while (offset < st.st_size) {
        ssize_t sent = sendfile(clientfd, fd, &offset, st.st_size - offset);
        if (sent <= 0) {
            perror("sendfile (Linux)");
            break;
        }
    }
#else
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        ssize_t totalSent = 0;
        while (totalSent < bytes) {
            ssize_t sent = send(clientfd, buffer + totalSent, bytes - totalSent, 0);
            if (sent < 0) {
                perror("send (fallback)");
                break;
            }
            totalSent += sent;
        }
    }
#endif

    printf("%s[DONE]%s Sent file successfully!\n", COLOR_GREEN, COLOR_END);

    close(fd);
    close(clientfd);
    close(sockfd);
    freeaddrinfo(res);
    return 0;
}

// ------------------------- RECEIVE FUNCTION -------------------------
int receive(const char *server_addr) {
    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_addr, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return -1;
    }

    printf("%s[INFO]%s Connecting to %s:%s ...\n", COLOR_BLUE, COLOR_END, server_addr, PORT);

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
        perror("connect");
        freeaddrinfo(res);
        close(sockfd);
        return -1;
    }

    FileMetaData metadata = {0};
    if (recv(sockfd, &metadata, sizeof(metadata), 0) != sizeof(metadata)) {
        perror("recv metadata");
        freeaddrinfo(res);
        close(sockfd);
        return -1;
    }

    printf("%s[INFO]%s Receiving: %s (%lld bytes)\n", COLOR_BLUE, COLOR_END,
           metadata.filename, (long long)metadata.size);

    int fd = open(metadata.filename, O_CREAT | O_WRONLY | O_TRUNC, metadata.mode);
    if (fd < 0) {
        perror("open");
        freeaddrinfo(res);
        close(sockfd);
        return -1;
    }

    off_t received = 0;
    char buffer[BUFFER_SIZE];
    while (received < metadata.size) {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        write(fd, buffer, bytes);
        received += bytes;
    }

    printf("%s[DONE]%s Received %s (%lld bytes)\n", COLOR_GREEN, COLOR_END,
           metadata.filename, (long long)metadata.size);

    close(fd);
    close(sockfd);
    freeaddrinfo(res);
    return 0;
}
