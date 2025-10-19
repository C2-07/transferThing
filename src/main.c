/**
 * @file main.c
 * @brief Entry point for the file transfer application.
 *
 * This file contains the main function that parses command-line arguments
 * and initiates either the sending or receiving process.
 */

#include "colors.h"    // For color codes
#include "discovery.h" // For discoveryListen() and discoveryAdvertise()
#include "transfer.h"  // For transferSend() and transferReceive()
#include <stdio.h>     // For printf(), perror()
#include <stdlib.h>    // For exit(), EXIT_FAILURE
#include <string.h>    // For strcmp()

/**
 * @brief Main function to handle command-line arguments.
 *
 * Depending on the arguments, this function will either start the file
 * receiving process or the file sending process.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char *argv[]) {
  // Ensure that at least one command (send or recv) is provided.
  if (argc < 2) {
    printf("Usage: %s [recv| <filename to be sent>]\n", argv[0]);
    return 1;
  }

  // If the command is "recv", start the discovery and receiving process.
  if (strcmp(argv[1], "recv") == 0) {
    char address[64] = {0}; // For garbage value
    // User provided IP address for SERVER
    if (argc == 3) {
      strncpy(address, argv[2], 63);
    }
    // Listen for a sender's broadcast.
    if (discoveryListen(address) != 0) {
      printf("%s[DISCOVERY]%s Searching for sender...\n", COLOR_BLUE,
             COLOR_END);
      perror("DiscoverUDP");
      exit(EXIT_FAILURE);
    }
    // Once a sender is found, start the file transfer.
    transferReceive(address);
  } else {
    // If the command is "send", start the advertising and sending process.
    printf("%s[ADVERTISE]%s Waiting for receiver discovery...\n", COLOR_RED,
           COLOR_END);
    // Advertise the sender's presence.
    if (discoveryAdvertise()) {
      perror("AdvertiseUDP");
      exit(EXIT_FAILURE);
    }
    // Start sending the specified file.
    transferSend(argv[1]);
  }

  return 0;
}
