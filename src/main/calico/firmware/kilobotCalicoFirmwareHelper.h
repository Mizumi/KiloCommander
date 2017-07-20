#ifndef KILOBOT_CALICO_FIRMWARE_HELPER_H
#define KILOBOT_CALICO_FIRMWARE_HELPER_H

#include <stdio.h>
#include <stdlib.h>

#include "kilolib.h"
#include "vs.h"

#include "kilobotCalicoDefinitions.h"

// Assume user provides setup and loop functions for Kilobot.
extern void setup();
extern void loop();

/**
 * This method is invoked whenever a data message (MSD_SEND_BROADCAST) is
 * received on this unit.
 *
 * @param msg Message received on this unit. The first byte is the message type (
 *            typically MSG_SEND_BROADCAST), with all following bytes containing
 *            the message data.
 */
extern void onCalicoMessageReceived(uint8_t* msg);

/**
 * Returns the next available and queued message to transmit.
 *
 * @return The next available message to transmit. If
 *         no messages are available, NULL will be returned.
 */
message_t *getNextCalicoMessage();

/**
 * Progresses the message queue forward once. This method should be invoked
 * after the message sent via {@link #vskGetNextMessage()} is successfully
 * broadcast.
 *
 * @return 0 if the queue was progressed, and -1 if the queue is empty.
 */
int progressNextCalicoMessage();

/**
 * Decodes a message into the format of a VsBroadcast and attempts to process
 * it into the local virtual stigmergy table.
 *
 * @param msg Message to decode.
 */
void decodeAndProcessCalicoMessage(message_t *msg);

/**
 * Adds a new message to the message transmission queue. If the queue is full,
 * the message will not be added.
 *
 * @param message Message to queue.
 *
 * @return 0 if the message was queued, and -1 if the queue is currently full.
 */
int queueCalicoBroadcast(message_t message);

/**
 * Adds a new VsBroadcast to the message transmission queue. If the queue
 * is full, the message will not be added.
 *
 * @param broadcast Broadcast to queue.
 *
 * @return 0 if the message was queued, and -1 if the queue is currently full.
 */
int queueVsBroadcast(VsBroadcast broadcast);

/**
 * Performs default kilobot initialization routines using the Calico firmware helpers.
 *
 * @return Exit status.
 */
int defaultCalicoKilobotMainSetup();

#endif
