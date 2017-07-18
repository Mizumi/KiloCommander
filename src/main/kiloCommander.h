#ifndef KILO_COMMANDER_H
#define KILO_COMMANDER_H

#include <stdlib.h>  /* Something else. */
#include <stdint.h>  /* Unsigned integer types */
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#define OHC_DEFAULT_ADDRESS_MACOS "/dev/tty.usbserial-A904R919"

/**
 * Opens an overhead controller on the specified serial interface.
 *
 * @param name Name of the serial interface/file which the overhead controller is connected to.
 *
 * @return The file descriptor for the opened overhead controller device.
 */
int openOhc(const char* name);

/**
 * Transmits a message to the kilobot swarm from the overhead controller.
 *
 * @param fd File descriptor for the overhead controller to send this message on.
 * @param payload 9-Byte payload to transmit.
 *
 * @return -1 on failure.
 */
int kbSendMessage(int fd, uint8_t *payload);

/**
 * Transmits the RUN command to the kilobot swarm from the overhead controller.
 *
 * @param fd File descriptor for the overhead controller to send this message on.
 *
 * @return -1 on failure.
 */
int kbRun(int fd);

/**
 * Transmits the RESET command to the kilobot swarm from the overhead controller.
 *
 * @param fd File descriptor for the overhead controller to send this message on.
 *
 * @return -1 on failure.
 */
int kbReset(int fd);

#endif
