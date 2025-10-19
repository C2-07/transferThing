/**
 * @file discovery.c
 * @brief Functions for device discovery using UDP broadcast.
 *
 * This file implements a simple discovery mechanism where a sender advertises
 * its presence on the network, and a receiver can discover the sender's IP
 * address.
 */

#include "discovery.h"
#include <arpa/inet.h> // For inet_ntoa(), inet_ntop(), inet_pton(), htons(), ntohs()
#include <netinet/in.h> // For struct sockaddr_in
#include <stdio.h>      // For printf(), perror(), snprintf()
#include <stdlib.h>     // For exit(), EXIT_FAILURE
#include <string.h>     // For memset(), strcmp(), strlen(), strcpy()
#include <sys/socket.h> // For socket(), bind(), sendto(), recvfrom(), setsockopt(), getsockname()
#include <sys/types.h> // For socket types
#include <unistd.h>    // For close()

#define SERVER_PORT 8989
#define BROADCAST_ADDR "255.255.255.255" // LAN-wide broadcast address

/**
 * @brief Advertises the sender's presence on the network.
 *
 * This function acts as a UDP server, listening for discovery packets from
 * receivers. When a discovery packet is received, it replies with its own IP
 * address.
 *
 * @return 0 on success, -1 on error.
 */
int discoveryAdvertise() {
  int sockfd;
  struct sockaddr_in addr, client;
  char buffer[512];
  socklen_t len = sizeof(client);

  // Create a UDP socket.
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Allow the socket to be reused immediately after the program exits.
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Bind the socket to all available network interfaces on the specified port.
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  char localIP[64];
  if (get_localIP(localIP) != 0) {
    perror("get_localIP");
  }
  printf("%s[ADVERTISE]%s Listening for discovery messages on IP %s%s%s:%d...\n",
         COLOR_RED, COLOR_END, COLOR_MAGENTA, localIP, COLOR_END, SERVER_PORT);

  // Wait for discovery messages from receivers.
  while (1) {
    int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr *)&client, &len);
    if (n < 0)
      continue;

    buffer[n] = '\0'; // Null-terminate the received message.

    // If a valid discovery message is received, reply with the sender's IP
    // address.
    if (strcmp(buffer, "DISCOVERY_P2P") == 0) {
      char reply[128];
      char local_ip[INET_ADDRSTRLEN];

      // Get the sender's own IP address.
      struct sockaddr_in temp;
      socklen_t temp_len = sizeof(temp);
      if (getsockname(sockfd, (struct sockaddr *)&temp, &temp_len) == 0) {
        inet_ntop(AF_INET, &temp.sin_addr, local_ip, sizeof(local_ip));
      } else {
        strcpy(local_ip, "UNKNOWN");
      }

      // Prepare the reply message.
      snprintf(reply, sizeof(reply), "P2P_DEVICE:%s", local_ip);

      // Send the reply to the client.
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

/**
 * @brief Listens for a sender's advertisement on the network.
 *
 * This function acts as a UDP client, broadcasting a discovery packet to the
 * LAN and waiting for a reply from a sender. When a reply is received, it
 * stores the sender's IP address.
 *
 * @param address A buffer to store the sender's IP address.
 * @return 0 on success, -1 on error.
 */
int discoveryListen(char address[]) {
  int sockfd;
  struct sockaddr_in broadcastAddr, from;
  socklen_t len = sizeof(from);
  char buffer[512];
  const char *msg = "DISCOVERY_P2P";

  // Create a UDP socket.
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Enable the broadcast option on the socket.
  int broadcastEnable = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable,
                 sizeof(broadcastEnable)) < 0) {
    perror("setsockopt (SO_BROADCAST)");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // Set a timeout for receiving a reply.
  struct timeval timeout = {10, 0};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  // Set up the broadcast address.
  memset(&broadcastAddr, 0, sizeof(broadcastAddr));
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, strlen(address) == 0 ? BROADCAST_ADDR : address,
            &broadcastAddr.sin_addr);

  printf("%s[DISCOVERY]%s Broadcasting on %s:%d\n", COLOR_BLUE, COLOR_END,
         strlen(address) == 0 ? BROADCAST_ADDR : address, SERVER_PORT);

  // Send the discovery packet.
  if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&broadcastAddr,
             sizeof(broadcastAddr)) < 0) {
    perror("sendto");
    close(sockfd);
    return -1;
  }

  printf("%s[DISCOVERY]%s Sent broadcast, waiting for replies...\n", COLOR_BLUE,
         COLOR_END);

  // Wait for a reply from a sender.
  int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                   (struct sockaddr *)&from, &len);

  if (n > 0) {
    buffer[n] = '\0';
    // Store the sender's IP address.
    strncpy(address, inet_ntoa(from.sin_addr), 63);
    address[63] = '\0';
    printf("%s[DISCOVERY]%s Found device: %s | Message: %s\n", COLOR_BLUE,
           COLOR_END, address, buffer);
  } else {
    printf("%s[DISCOVERY]%s No devices found within timeout.\n", COLOR_BLUE,
           COLOR_END);
    return -1;
  }

  close(sockfd);
  return 0;
}
