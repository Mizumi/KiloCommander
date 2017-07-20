#ifndef KILOBOT_CALICO_DRIVER_H
#define KILOBOT_CALICO_DRIVER_H

#include <stdint.h>

#include "kilobotCalicoDefinitions.h"
#include "kiloCommander.h"

// Macro for determining RGB color values to send to kilobot units.
#define RGB(r,g,b) (r&3)|(((g&3)<<2))|((b&3)<<4)

/**
 * Initializes the Calico driver.
 *
 * Once the calico driver is initialized, you should be sure to enable the kilobot
 * swarm via a call to kbRun(). Once you're done controlling them you should call kbReset().
 * Both of these methods should be invoked multiple times over a period to be certain that
 * the swarm is running and/or reset, depending, due to data loss.
 *
 * TODO: Add support for multiple overhead controller addresses.
 *
 * @param overheadControllerAddress String address of the overhead controller on
 *                                  the system running this driver. On Macs,
 *                                  you can use the OHC_DEFAULT_ADDRESS_MACOS
 *                                  definition (usually).
 */
void initializeCalicoDriver(const char* overheadControllerAddress);

/**
 * Sets the motor values on a specific range of kilobots.
 *
 * @param idMin The lowest unit ID that will receive the message.
 * @param idMax The highest unit ID that will receive the message.
 * @param left The speed of the left motor.
 * @param right The speed of the right motor.
 */
void setMotors(uint8_t idMin, uint8_t idMax, uint8_t left, uint8_t right);

/**
 * Sets the LED color on a specific range of kilobots.
 *
 * @param idMin The lowest unit ID that will receive the message.
 * @param idMax The highest unit ID that will receive the message.
 * @param color An 8-bit unsigned integer denoting the color to use. Use the RGB(r, g, b)
 *              macro definition to determine this value.
 */
void setColor(uint8_t idMin, uint8_t idMax, uint8_t color);

/**
 * Sends a message to a specific kilobot containing X/Y coordinate data.
 *
 * @param posX 0 - 255 value denoting the kilobot's X-axis position.
 * @param posY 0 - 255 value denoting the kilobot's Y-axis position.
 */
void setPos(uint8_t id, uint8_t posX, uint8_t posY);

/**
 * Sends a 6 byte message to a specific range of kilobots.
 *
 * @param idMin The lowest unit ID that will receive the message.
 * @param idMax The highest unit ID that will receive the message.
 * @param payload A 6 element array of uint8_t.
 */
void sendMessage(uint8_t idMin, uint8_t idMax, uint8_t* payload);

/**
 * Sends a 8 byte message to the entire kilobot swarm.
 *
 * @param payload A 8 element array of uint8_t.
 */
void sendBroadcastMessage(uint8_t* payload);

#endif
