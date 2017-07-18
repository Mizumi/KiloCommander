#include "kiloCommander.h"

int max(int one, int two) {
    if (one > two) {
        return one;
    } else {
        return two;
    }
}

int main(int argc, char* argv[]) {
    int fd = openOhc(OHC_DEFAULT_ADDRESS);

    while (1) {
        // int i = atoi(argv[1]);
        int i;
        uint8_t payload[9];

        for (i = 0; i < 20; i++) {
            fprintf(stderr, "On ID:%d\n", i);
            payload[8] = i;
            payload[0] = max((rand() % 3), 1);
            payload[1] = max((rand() % 3), 1);
            payload[2] = max((rand() % 3), 1);

            kbSendMessage(fd, payload);
            usleep(25000);

            payload[8] = i;
            payload[0] = 0;
            payload[1] = 0;
            payload[2] = 0;

            kbSendMessage(fd, payload);
            usleep(25000);
        }
    }
}
