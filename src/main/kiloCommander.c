#include "kiloCommander.h"

// Magic definitions.
#define PAGE_SIZE 128
#define PACKET_HEADER 0x55
#define PACKET_SIZE PAGE_SIZE + 4
#define COMMAND_STOP 250
#define OHC_BAUD 38400

// Command packet types.
enum {
    PACKET_STOP,
    PACKET_LEDTOGGLE,
    PACKET_FORWARDMSG,
    PACKET_FORWARDRAWMSG,
    PACKET_BOOTPAGE
};

// Data packet types.
typedef enum {
    NORMAL = 0,
    GPS,
    SPECIAL = 0x80,
    BOOT = 0x80,
    BOOTPGM_PAGE,
    BOOTPGM_SIZE,
    RESET,
    SLEEP,
    WAKEUP,
    CHARGE,
    VOLTAGE,
    RUN,
    READUID,
    CALIB
} message_type_t;

// Default "empty" data packet for sending commands.
uint8_t emptyDataPacket[9] = {0};

typedef struct KiloCommanderState {
    // 0 or greater if the commander driver is connected, and negative otherwise.
    int fd;

    // 0 if the commander driver is currently idle (not sending), and 1 otherwise.
    int sending;
} KiloCommanderState;

// Fixed size support for up to eight overhead controllers.
KiloCommanderState state[8];
int stateSize = 0;

/**
 * TODO:
 *
 * @param fd
 *
 * @return [description]
 */
KiloCommanderState* _getState(int fd) {
    int i;
    for (i = 0; i < stateSize; i++) {
        if (state[i].fd == fd) {
            return &state[i];
        }
    }

    return NULL;
}

/**
 * TODO:
 *
 * @param fd
 *
 * @return [description]
 */
int _hasState(int fd) {
    int i;

    for (i = 0; i < stateSize; i++) {
        if (state[i].fd == fd) {
            return 1;
        }
    }

    return 0;
}

/**
 * TODO:
 *
 * @param state
 *
 * @return [description]
 */
int _insertState(KiloCommanderState KiloCommanderState) {
    if (stateSize < 8) {
        state[stateSize] = KiloCommanderState;
        stateSize++;
        return 1;
    }

    return 0;
}

int openOhc(const char* name) {

    // Attempt to open port.
    KiloCommanderState state;
    state.fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
    state.sending = 0;

    // Print an error on failure.
    if (state.fd == -1) {
        fprintf(stderr, "Unable to open overhead controller @ %s\n", name);

    // Configure the port.
    } else {
        // Get port options.
        struct termios options;
        tcgetattr(state.fd, &options);

        // Set I/O baud.
        cfsetispeed(&options, OHC_BAUD);
        cfsetospeed(&options, OHC_BAUD);

        // Something about receivers and local modes.
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        options.c_cflag &= ~CRTSCTS;
        options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
        options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
        options.c_oflag &= ~OPOST; // make raw

        // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
        options.c_cc[VMIN]  = 0;
        options.c_cc[VTIME] = 0;

        // Apply the options.
        tcsetattr(state.fd, TCSANOW, &options);

        // Register state.
        _insertState(state);
    }

    return state.fd;
}

int sendMessage(int fd, uint8_t *payload, uint8_t type_int, int withPayload) {
    // Exit if bad fd.
    if (!_hasState(fd)) {
        fprintf(stderr, "Cannot send data messages if the serial port is not connected (FD = %d).\n", fd);
        return -1;
    }

    // Get state.
    KiloCommanderState* state = _getState(fd);

    // Cast type to a char for transmission.
    unsigned char type = (unsigned char) type_int;

    // Prepare packet.
    char packet[PACKET_SIZE + 1] = {0};

    if (type == COMMAND_STOP) {
        state->sending = 0;

        packet[0] = PACKET_HEADER;
        packet[1] = PACKET_STOP;
        packet[PACKET_SIZE-1]=PACKET_HEADER^PACKET_STOP;
    } else {
        if (state->sending) {
            sendMessage(fd, emptyDataPacket, COMMAND_STOP, 0);
        }

        state->sending = 1;

        uint8_t checksum = PACKET_HEADER^PACKET_FORWARDMSG^type;

        // Configure as a data packet.
        packet[0] = PACKET_HEADER;
        packet[1] = PACKET_FORWARDMSG;

        // Insert payload information.
        if (withPayload) {
            for (int i = 0; i < 9; i++) {
                packet[2+i] = payload[i];
                checksum ^= payload[i];
            }
        }

        // Finalize type and checksum.
        packet[11] = type;
        packet[PACKET_SIZE-1] = checksum;
    }

    // Terminate packet.
    packet[PACKET_SIZE] = '\0';

    // Publish data packet.
    int n = write(state->fd, packet, PACKET_SIZE);
    tcdrain(state->fd);

    // Return status.
    return n;
}

int kbSendMessage(int fd, uint8_t *payload) {
    return sendMessage(fd, payload, NORMAL, 1);
}

int kbReset(int fd) {
    return sendMessage(fd, emptyDataPacket, RESET, 0);
}

int kbRun(int fd) {
    return sendMessage(fd, emptyDataPacket, RUN, 0);
}
