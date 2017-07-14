#include <stdint.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#define PAGE_SIZE 128
#define PACKET_HEADER 0x55
#define PACKET_SIZE PAGE_SIZE+4
#define COMMAND_STOP 250

enum {
    PACKET_STOP,
    PACKET_LEDTOGGLE,
    PACKET_FORWARDMSG,
    PACKET_FORWARDRAWMSG,
    PACKET_BOOTPAGE
};

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

typedef struct {
    uint16_t id;
    int16_t x;
    int16_t y;
    int8_t theta;
    uint16_t unused;
} gpsmsg_t;

#define OHC_ADDRESS "/dev/tty.usbserial-A904R919"
#define OHC_BAUD 38400

int started = 0;
int context = -1;
int sending = 0;
uint8_t emptyDataPacket[9] = {0};

/**
 * Open the Overhead Controller on some device.
 *
 * @return [description]
 */
int openOhc() {

    // Attempt to open the port.
    int fd = open(OHC_ADDRESS, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);

    // Print an error on failure.
    if (fd == -1) {
        fprintf(stderr, "Unable to open overhead controller @ %s\n", OHC_ADDRESS);

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

    // Return final file descriptor.
    return fd;
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
        context = openOhc();
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

int main() {
    while (1) {
        kbRun();
        sleep(5);
        kbReset();
        sleep(5);
    }
}
