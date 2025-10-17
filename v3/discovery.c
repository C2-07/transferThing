#include "discovery.h"  // For AdvertiseUDP, DiscoverUDP
#include <arpa/inet.h>  // For inet_ntoa, inet_pton, htons, ntohs
#include <netinet/in.h> // For sockaddr_in
#include <stdio.h>      // For printf, perror
#include <stdlib.h>     // For exit
#include <string.h>     // For memset, strcmp, strlen
#include <sys/socket.h> // For socket, bind, sendto, recvfrom
#include <sys/types.h>  // For socket types
#include <unistd.h>     // For close()

#define SERVER_PORT 8989
#define BROADCAST_ADDR "255.255.255.255" // Address used for LAN-wide broadcast

/*
 * AdvertiseUDP()
 * ----------------
 * This function acts as a UDP server or "advertiser".
 * It listens for discovery messages from other devices on the LAN.
 * When it receives a valid discovery request ("DISCOVERY_P2P"), it replies
 * with its own IP address.
 */
int AdvertiseUDP() {
  int sockfd;
  struct sockaddr_in addr, client;
  char buffer[512];
  socklen_t len = sizeof(client);

  // 1. Create a UDP socket (SOCK_DGRAM = UDP)
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // 2. Prepare the local address for binding
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;          // IPv4 address family
  addr.sin_port = htons(SERVER_PORT); // Convert to network byte order
  addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces

  // 3. Bind the socket to the local address and port
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("Waiting for discovery messages on UDP port %d...\n", SERVER_PORT);

  // 4. Wait for incoming messages in an infinite loop
  while (1) {
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr *)&client, &len);
    if (n < 0)
      continue;

    buffer[n] = '\0'; // Null-terminate received string

    // 5. Check if message matches the discovery keyword
    if (strcmp(buffer, "DISCOVERY_P2P") == 0) {
      char reply[128];

      // Prepare a reply message with advertiser's IP address
      sprintf(reply, "P2P_DEVICE:%s", inet_ntoa(client.sin_addr));

      // Send the reply back to the requester
      sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr *)&client, len);

      printf("Replied to discovery request from %s\n",
             inet_ntoa(client.sin_addr));
      // Client found proceeding to transfering stage
      break;
    }
  }

  // 6. Clean up
  close(sockfd);
  return 0;
}

/*
 * DiscoverUDP()
 * ----------------
 * This function acts as a UDP client or "discoverer".
 * It sends a broadcast packet to the network to find devices that
 * are running AdvertiseUDP().
 * Devices that receive the broadcast will reply with their IP address.
 */
int DiscoverUDP(char *address) {
  int sockfd;
  struct sockaddr_in broadcastAddr, from;
  socklen_t len = sizeof(from);
  char buffer[512];
  const char *msg = "DISCOVERY_P2P";

  // 1. Create a UDP socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // 2. Enable permission to send broadcast messages on this socket
  int broadcastEnable = 1;
  // SOL -> Set Option Level, SO -> Set Option
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
                 sizeof(broadcastEnable)) < 0) {
    perror("setsockopt (SO_BROADCAST)");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // 3. Prepare the broadcast address structure
  memset(&broadcastAddr, 0, sizeof(broadcastAddr));
  broadcastAddr.sin_family = AF_INET;          // IPv4
  broadcastAddr.sin_port = htons(SERVER_PORT); // Same port as server
  inet_pton(AF_INET, BROADCAST_ADDR,
            &broadcastAddr.sin_addr); // Convert text IP to binary

  // 4. Send the discovery message to the broadcast address
  sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broadcastAddr,
         sizeof(broadcastAddr));
  printf("Sent discovery broadcast.\n");

  // 5. Wait for replies from any listening devices
  while (1) {
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr *)&from, &len);
    if (n > 0) {
      buffer[n] = '\0';
      address = inet_ntoa(from.sin_addr);
      printf("Found device: %s", inet_ntoa(from.sin_addr));
      // Server found proceeding to transfering stage.
      break;
    }
  }

  // 6. Close socket (unreachable in this loop)
  close(sockfd);
  return 0;
}

/*
 * main()
 * -------
 * Entry point. Choose whether to advertise or discover.
 * For now, it runs AdvertiseUDP() by default.
 */
