#include "transfer.h"
#include "discovery.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "colors.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [recv|send <file>]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "recv") == 0) {
        printf("%s[DISCOVERY]%s Searching for sender...\n", COLOR_BLUE, COLOR_END);
        char address[64];
        if (DiscoverUDP(address) != 0) {
          perror("DiscoverUDP");
          exit(EXIT_FAILURE);
        }
        receive(address);
    } else {
        printf("%s[ADVERTISE]%s Waiting for receiver discovery...\n", COLOR_RED, COLOR_END);
        if (AdvertiseUDP()) {
          perror("AdvertiseUDP");
          exit(EXIT_FAILURE);
        }
        share(argv[1]);
    }

    return 0;
}
