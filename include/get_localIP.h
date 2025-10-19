/**
 * @file get_localIP.h
 * @brief Header file for the get_localIP function.
 *
 * This file contains the function prototype for getting the local IP address.
 */

#ifndef GET_LOCAL_IP_H
#define GET_LOCAL_IP_H

/**
 * @brief Gets the local IP address of the machine.
 *
 * @param localIP A buffer to store the local IP address.
 * @return 0 on success, -1 on error.
 */
int get_localIP(char *localIP);

#endif // GET_LOCAL_IP_H