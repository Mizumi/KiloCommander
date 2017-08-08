#include "kilobotCalicoDriver.h"

// File descriptor for the overhead controller.
int ohcFd = -1;

void initializeCalicoDriver(const char* overheadControllerAddress) {

    // Open the overhead controller.
    ohcFd = openOhc(overheadControllerAddress);

    // Print an error if the driver could not be opened.
    if (ohcFd < 0) {
        fprintf(stderr, "Opening the overhead controller failed with code %d.\n", ohcFd);
    } else {
        fprintf(stderr, "Successfully opened the overhead controller with file descriptor %d.\n", ohcFd);
    }
}

void setMotors(uint8_t idMin, uint8_t idMax, uint8_t left, uint8_t right) {
    // Prepare a message for sending.
    uint8_t message[MSG_MAX_SIZE] = {0};

    // Insert message metadata.
    message[0] = MSG_SET_MOTORS;
    message[1] = idMin;
    message[2] = idMax;

    // Insert message payload.
    message[3] = left;
    message[4] = right;

    // Send the message.
    kbSendMessage(ohcFd, message);
}

void setColor(uint8_t idMin, uint8_t idMax, uint8_t color) {
    // Prepare a message for sending.
    uint8_t message[MSG_MAX_SIZE] = {0};

    // Insert message metadata.
    message[0] = MSG_SET_COLOR;
    message[1] = idMin;
    message[2] = idMax;

    // Insert message payload.
    message[3] = color;

    // Send the message.
    kbSendMessage(ohcFd, message);
}

void setPos(uint8_t id, uint8_t posX, uint8_t posY) {
    // Prepare a message for sending.
    uint8_t message[MSG_MAX_SIZE] = {0};

    // Insert message metadata.
    message[0] = MSG_SET_POS;
    message[1] = id;

    // Insert message payload.
    message[2] = posX;
    message[3] = posY;

    // Send the message.
    kbSendMessage(ohcFd, message);
}

void sendMessage(uint8_t idMin, uint8_t idMax, uint8_t* payload) {
    // Prepare a message for sending.
    uint8_t message[MSG_MAX_SIZE] = {0};

    // Insert message metadata.
    message[0] = MSG_SEND_MSG;
    message[1] = idMin;
    message[2] = idMax;

    // Insert message payload.
    int i;
    for (i = 0; i < 6; i++) {
        message[3 + i] = payload[i];
    }

    // Send the message.
    kbSendMessage(ohcFd, message);
}

void sendBroadcastMessage(uint8_t* payload) {
    // Prepare a message for sending.
    uint8_t message[MSG_MAX_SIZE] = {0};

    // Insert message metadata.
    message[0] = MSG_SEND_BROADCAST;

    // Insert message payload.
    int i;
    for (i = 0; i < 8; i++) {
        message[1 + i] = payload[i];
    }

    // Send the message.
    kbSendMessage(ohcFd, message);
}
