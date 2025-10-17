#include <sys/stat.h>

#if defined(__APPLE__)
#include <sys/uio.h> // macOS: sendfile(fd, sockfd, offset, len, etc)
#elif defined(__linux__)
#include <sys/sendfile.h> // Linux: sendfile(sockfd, fd, offset, count)
#endif

#define PORT "8989"
#define BUFFER_SIZE 1024 * 512 // 512 KB buffer Size
#define LOOPBACK "127.0.0.1"
//
// Style Constants
#define COLOR_BLUE "\033[94m"
#define COLOR_GREEN "\033[92m"
#define COLOR_YELLOW "\033[93m"
#define COLOR_RED "\033[91m"
#define COLOR_BOLD "\033[1m"
#define COLOR_END "\033[0m"

typedef struct {
  char filename[256];
  off_t size;
  mode_t mode;
} FileMetaData;


int share(const char *fp);
int recieve(char *server_addr);
