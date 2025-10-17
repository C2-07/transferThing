#ifndef TRANSFER_H
#define TRANSFER_H

#include <sys/types.h>
#include <sys/stat.h>

// ANSI color codes
#define COLOR_BLUE  "\033[94m"
#define COLOR_GREEN "\033[92m"
#define COLOR_YELLOW "\033[93m"
#define COLOR_RED   "\033[91m"
#define COLOR_END   "\033[0m"

typedef struct {
    char filename[256]; // only filename, not path
    off_t size;
    mode_t mode;
} FileMetaData;

int share(const char *filepath);
int receive(const char *server_addr);

#endif // TRANSFER_H
