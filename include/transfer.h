/**
 * @file transfer.h
 * @brief Header file for file transfer functions.
 *
 * This file defines the structures and function prototypes for sending and
 * receiving files.
 */

#ifndef TRANSFER_H
#define TRANSFER_H

#include <sys/types.h>
#include <sys/stat.h>
#include "colors.h"

/**
 * @struct FileMetaData
 * @brief A structure to hold metadata about a file.
 *
 * This structure is used to send information about a file, such as its name,
 * size, and permissions, before the file content is transferred.
 */
typedef struct {
    char filename[256]; /**< The name of the file. */
    off_t size;         /**< The size of the file in bytes. */
    mode_t mode;        /**< The file permissions. */
} FileMetaData;

/**
 * @brief Sends a file to a connected receiver.
 *
 * @param filepath The path to the file to be sent.
 * @return 0 on success, -1 on error.
 */
int transferSend(const char *filepath);

/**
 * @brief Receives a file from a sender.
 *
 * @param server_addr The IP address of the sender.
 * @return 0 on success, -1 on error.
 */
int transferReceive(const char *server_addr);

#endif // TRANSFER_H