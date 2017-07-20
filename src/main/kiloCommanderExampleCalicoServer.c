#include "kilobotCalicoDriver.h"

// Helper for finding max values.
int max(int one, int two) {
    if (one > two) {
        return one;
    } else {
        return two;
    }
}

int main(int argc, char* argv[]) {
    // Start the firmware driver.
    initializeCalicoDriver(OHC_DEFAULT_ADDRESS_MACOS);

    while (1) {
        int i;
        for (i = 0; i < 90; i++) {
            // Print debugging info.
            fprintf(stderr, "On ID:%d\n", i);

            // Send message to set kilobot color and then wait.
            setColor(i, i ,RGB(max((rand() % 3), 1), max((rand() % 3), 1), max((rand() % 3), 1)));
            usleep(25000);

            // Send message to reset kilobot color and then wait.
            setColor(i, i ,RGB(0, 0, 0));
            usleep(25000);
        }
    }
}
