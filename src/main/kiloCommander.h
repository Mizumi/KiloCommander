#include <stdlib.h>  /* Something else. */
#include <stdint.h>  /* Unsigned integer types */
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

// Magic definitions.
#define PAGE_SIZE 128
#define PACKET_HEADER 0x55
#define PACKET_SIZE PAGE_SIZE+4
#define COMMAND_STOP 250

// Overhead controller communication definitions.
#define OHC_DEFAULT_ADDRESS "/dev/tty.usbserial-A904R919"
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

void openOhc(const char* name);
void kbRun();
void kbReset();
void kbSendMessage(uint8_t *payload);
