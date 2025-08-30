#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

char* get_local_ip(char *buffer, size_t buffer_size) {
    char hostname[256];
    struct addrinfo hints, *res, *p;

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        snprintf(buffer, buffer_size, "127.0.0.1");
        return buffer;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        snprintf(buffer, buffer_size, "127.0.0.1");
        return buffer;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
        inet_ntop(AF_INET, &addr->sin_addr, buffer, buffer_size);
    }

    freeaddrinfo(res);
    return buffer;
}

int main() {
    char ip[INET_ADDRSTRLEN];
    printf("%s\n", get_local_ip(ip, sizeof(ip)));
    return 0;
}
