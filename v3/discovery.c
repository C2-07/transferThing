#include "discovery.h"  // For AdvertiseUDP, DiscoverUDP
#include <arpa/inet.h>  // For inet_ntoa, inet_ntop, inet_pton, htons, ntohs
#include <netinet/in.h> // For sockaddr_in
#include <stdio.h>      // For printf, perror
#include <stdlib.h>     // For exit
#include <string.h>     // For memset, strcmp, strlen
#include <sys/socket.h> // For socket, bind, sendto, recvfrom, setsockopt
#include <sys/types.h>  // For socket types
#include <unistd.h>     // For close()

#define SERVER_PORT 8989
#define BROADCAST_ADDR "255.255.255.255" // LAN-wide broadcast address

/*
 * AdvertiseUDP()
 * ----------------
 * Acts as a UDP server (the advertiser).
 * Waits for discovery packets ("DISCOVERY_P2P") from clients on LAN.
 * When found, replies with its *own* IP address.
 */
int AdvertiseUDP() {
  int sockfd;
  struct sockaddr_in addr, client;
  char buffer[512];
  socklen_t len = sizeof(client);

  // 1. Create UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Allow immediate rebinding after program restart
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // 2. Bind socket to all interfaces on SERVER_PORT
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("%s[ADVERTISE]%s Listening for discovery messages on UDP port %d...\n",
         COLOR_RED, COLOR_END, SERVER_PORT);

  // 3. Wait for discovery messages
  while (1) {
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr *)&client, &len);
    if (n < 0)
      continue;

    buffer[n] = '\0'; // Null-terminate message

    // 4. If valid discovery keyword received
    if (strcmp(buffer, "DISCOVERY_P2P") == 0) {
      char reply[128];
      char local_ip[INET_ADDRSTRLEN];

      // Get our own IP bound to this socket
      struct sockaddr_in temp;
      socklen_t temp_len = sizeof(temp);
      if (getsockname(sockfd, (struct sockaddr *)&temp, &temp_len) == 0) {
        inet_ntop(AF_INET, &temp.sin_addr, local_ip, sizeof(local_ip));
      } else {
        strcpy(local_ip, "UNKNOWN");
      }

      // Prepare reply message
      snprintf(reply, sizeof(reply), "P2P_DEVICE:%s", local_ip);

      // Send reply to client
      if (sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr *)&client,
                 len) < 0) {
        perror("sendto");
      } else {
        printf("%s[ADVERTISE]%s Replied to discovery request from %s with %s\n",
               COLOR_RED, COLOR_END, inet_ntoa(client.sin_addr), local_ip);
      }

      // Optional: break after one response
      break;
    }
  }

  close(sockfd);
  return 0;
}

/*
 * DiscoverUDP()
 * ----------------
 * Acts as a UDP client (the discoverer).
 * Broadcasts "DISCOVERY_P2P" to the LAN and waits for device replies.
 * On success, stores found device IP into `address`.
 */
int DiscoverUDP(char *address) {
  int sockfd;
  struct sockaddr_in broadcastAddr, from;
  socklen_t len = sizeof(from);
  char buffer[512];
  const char *msg = "DISCOVERY_P2P";

  // 1. Create UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // 2. Enable broadcast option
  int broadcastEnable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
                 sizeof(broadcastEnable)) < 0) {
    perror("setsockopt (SO_BROADCAST)");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // 3. Set 2-second timeout for receiving reply
  struct timeval timeout = {100, 0};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  // 4. Setup broadcast address
  memset(&broadcastAddr, 0, sizeof(broadcastAddr));
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, BROADCAST_ADDR, &broadcastAddr.sin_addr);

  printf("%s[DISCOVERY]%s Broadcasting on %s:%d\n", COLOR_BLUE, COLOR_END,
         BROADCAST_ADDR, SERVER_PORT);

  // 5. Send discovery packet
  if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broadcastAddr,
             sizeof(broadcastAddr)) < 0) {
    perror("sendto");
    close(sockfd);
    return -1;
  }

  printf("%s[DISCOVERY]%s Sent broadcast, waiting for replies...\n", COLOR_BLUE,
         COLOR_END);

  // 6. Wait for a reply
  int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                   (struct sockaddr *)&from, &len);

  if (n > 0) {
    buffer[n] = '\0';
    strncpy(address, inet_ntoa(from.sin_addr), 63);
    address[63] = '\0';
    printf("%s[DISCOVERY]%s Found device: %s | Message: %s\n", COLOR_BLUE,
           COLOR_END, address, buffer);
  } else {
    printf("%s[DISCOVERY]%s No devices found within timeout.\n", COLOR_BLUE,
           COLOR_END);
  }

  close(sockfd);
  return 0;
}
