#include "kiloCommander.h"

// 0 if the commander driver has been started, and 1 otherwise.
int started = 0;

// 0 or greater if the commander driver is connected, and negative otherwise.
int context = -1;

// 0 if the commander driver is currently sending, and 1 otherwise.
int sending = 0;

// Default "empty" data packet for sending commands.
uint8_t emptyDataPacket[9] = {0};

/**
 * Open the Overhead Controller on some device.
 *
 * TODO: Add validation if controller is already open.
 *
 * @return [description]
 */
void openOhc(const char* name) {

    // Attempt to open the port.
    int fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);

    // Print an error on failure.
    if (fd == -1) {
        fprintf(stderr, "Unable to open overhead controller @ %s\n", name);

    // Configure the port.
    } else {
        // Get port options.
        struct termios options;
        tcgetattr(fd, &options);

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
        tcsetattr(fd, TCSANOW, &options);
    }

    // Started.
    started = 1;
    context = fd;
}

/**
 * TODO:
 *
 * @method sendDataMessage
 * @param payload
 * @param type
 */
int sendMessage(uint8_t *payload, uint8_t type_int, int withPayload) {
    if (!started) {
        openOhc(OHC_DEFAULT_ADDRESS);
    }

    unsigned char type = (unsigned char) type_int;

    // Exit if bad context.
    if (context < 0) {
        fprintf(stderr, "Cannot send data messages if the serial port is not connected (context = %d).\n", context);
        return -1;
    }

    // Prepare packet.
    char packet[PACKET_SIZE + 1] = {0};

    if (type == COMMAND_STOP) {
        sending = 0;
        packet[0] = PACKET_HEADER;
        packet[1] = PACKET_STOP;
        packet[PACKET_SIZE-1]=PACKET_HEADER^PACKET_STOP;
    } else {
        if (sending) {
            sendMessage(emptyDataPacket, COMMAND_STOP, 0);
        }

        sending = 1;

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
    int n = write(context, packet, PACKET_SIZE);
    tcdrain(context);

    // Return status.
    return n;
}

void kbReset() {
    sendMessage(emptyDataPacket, RESET, 0);
}

void kbRun() {
    sendMessage(emptyDataPacket, RUN, 0);
}

void kbSendMessage(uint8_t *payload) {
    sendMessage(payload, NORMAL, 1);
}
