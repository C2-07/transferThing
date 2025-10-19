/**
 * @file discovery.h
 * @brief Header file for device discovery functions.
 *
 * This file contains the function prototypes for advertising and discovering
 * devices on the network.
 */

#ifndef DISCOVERY_H
#define DISCOVERY_H

#include "colors.h"
#include "get_localIP.h"

/**
 * @brief Advertises the sender's presence on the network.
 *
 * @return 0 on success, -1 on error.
 */
int discoveryAdvertise();

/**
 * @brief Listens for a sender's advertisement on the network.
 *
 * @param address A buffer to store the sender's IP address.
 * @return 0 on success, -1 on error.
 */
int discoveryListen(char *address);

#endif // DISCOVERY_H
