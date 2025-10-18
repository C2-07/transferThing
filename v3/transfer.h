#ifndef TRANSFER_H
#define TRANSFER_H

#include <sys/types.h>
#include <sys/stat.h>
#include "colors.h"

typedef struct {
    char filename[256]; // only filename, not path
    off_t size;
    mode_t mode;
} FileMetaData;

int share(const char *filepath);
int receive(const char *server_addr);

#endif // TRANSFER_H
