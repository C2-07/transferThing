#include "transfer.h"
#include "discovery.h"
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {

  }
  if (strcmp(argv[1], "recv")) {
    char *address = NULL;
    DiscoverUDP(address);
    recieve(address);
  }
  else {
    AdvertiseUDP();
    share(argv[1]);
  }
}
