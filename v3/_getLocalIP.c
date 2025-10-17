#include <stdio.h>          // printf()
#include <string.h>         // memset()
#include <arpa/inet.h>      // inet_pton(), inet_ntop(), sockaddr_in
#include <sys/socket.h>     // socket(), connect(), getsockname()
#include <unistd.h>         // close()

int main() {
    // 1. Create a UDP socket.
    // AF_INET = IPv4, SOCK_DGRAM = UDP.
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 2. Define a remote endpoint (8.8.8.8:80 — arbitrary, no data is sent).
    // We'll use this to let the kernel decide the best local interface to reach it.
    struct sockaddr_in remote, local;
    socklen_t len = sizeof(local);
    memset(&remote, 0, sizeof(remote));

    remote.sin_family = AF_INET;             // IPv4 address family
    remote.sin_port = htons(80);             // Destination port (arbitrary)
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);  // Convert "8.8.8.8" to binary

    // 3. "Connect" the UDP socket to that remote address.
    //
    // Note:
    //  - No packets are actually sent for UDP when calling connect().
    //  - It simply sets the default destination, so we can use send()/recv()
    //    without specifying an address each time.
    //  - More importantly, it triggers the kernel’s routing logic to determine
    //    which local IP/interface will be used to reach the destination.
    if (connect(sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    // 4. Now ask the kernel what local address it chose for this socket.
    // This works *only after* connect() (or bind()), since before that
    // the socket has no bound local address (would show 0.0.0.0:0).
    if (getsockname(sock, (struct sockaddr*)&local, &len) < 0) {
        perror("getsockname");
        close(sock);
        return 1;
    }

    // 5. Convert binary IP to human-readable form.
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local.sin_addr, ip, sizeof(ip));

    printf("My IP: %s\n", ip);

    // 6. Done — close socket.
    close(sock);
    return 0;
}
