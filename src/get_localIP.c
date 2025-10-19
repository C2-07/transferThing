/**
 * @file get_localIP.c
 * @brief Function to get the local IP address of the machine.
 *
 * This file implements a function to retrieve the local IP address by creating
 * a UDP socket and connecting to a public DNS server.
 */

#include "get_localIP.h"
#include <stdio.h>          // For printf(), perror()
#include <string.h>         // For memset(), strncpy()
#include <arpa/inet.h>      // For inet_pton(), inet_ntop(), struct sockaddr_in
#include <sys/socket.h>     // For socket(), connect(), getsockname()
#include <unistd.h>         // For close()

/**
 * @brief Gets the local IP address of the machine.
 *
 * This function creates a UDP socket and "connects" it to a public DNS server
 * (Google's 8.8.8.8). This forces the kernel to choose a local interface and
 * IP address, which can then be retrieved with getsockname().
 *
 * @param localIP A buffer to store the local IP address.
 * @return 0 on success, -1 on error.
 */
int get_localIP(char *localIP) {
    // Create a UDP socket.
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    // Define a remote endpoint (Google's DNS server). We don't actually send
    // any data, but this allows the kernel to determine the best local interface.
    struct sockaddr_in remote, local;
    socklen_t len = sizeof(local);
    memset(&remote, 0, sizeof(remote));

    remote.sin_family = AF_INET;
    remote.sin_port = htons(80); // Arbitrary port
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

    // "Connect" the UDP socket to the remote address. This doesn't send any
    // packets, but it associates the socket with a local IP address.
    if (connect(sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    // Get the local address that the kernel chose for this socket.
    if (getsockname(sock, (struct sockaddr*)&local, &len) < 0) {
        perror("getsockname");
        close(sock);
        return -1;
    }

    // Convert the binary IP address to a human-readable string.
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local.sin_addr, ip, sizeof(ip));
    if (localIP != NULL) {
      strncpy(localIP, ip,sizeof(ip));
    }
    // Close the socket.
    close(sock);
    return 0;
}
