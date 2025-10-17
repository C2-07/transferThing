#include "transfer.h"
#include "discovery.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage:\n"
                "  %s send <file>\n"
                "  %s recv\n",
                argv[0], argv[0]);
        return 1;
    }

    // Receiver mode (auto-discover sender)
    if (strcmp(argv[1], "recv") == 0) {
        char address[64] = {0};
        printf("%s[DISCOVERY]%s Searching for sender...\n", COLOR_BLUE, COLOR_END);
        DiscoverUDP(address);

        if (strlen(address) == 0) {
            fprintf(stderr, "%s[ERROR]%s No sender found.\n", COLOR_RED, COLOR_END);
            return 1;
        }

        printf("%s[FOUND]%s Sender at %s\n", COLOR_GREEN, COLOR_END, address);
        receive(address);
    }

    // Sender mode (advertise and share)
    else if (strcmp(argv[1], "send") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s send <file>\n", argv[0]);
            return 1;
        }

        printf("%s[ADVERTISE]%s Waiting for receiver discovery...\n", COLOR_YELLOW, COLOR_END);
        AdvertiseUDP();
        share(argv[2]);
    }

    else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
