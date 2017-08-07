/*
 * Virtual Stigmergy for swarm communications.
 */
#ifndef ARGOS3_KILOBOT_VS_H
#define ARGOS3_KILOBOT_VS_H

#include <stdint.h>

// Math macros for simplicity.
#ifndef max
    #define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
    #define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef square
    #define square(x) ((x)*(x))
#endif

// TODO: Modify to support dual-table stigmergy (see markdown document in repository root).

// Indicator byte for VS broadcasts validating they are VS broadcast messages.
#define IS_VS_BROADCAST_MESSAGE 42

// Defines for VS operations.
#define NON_VS_MSG (unsigned char) 0
#define VS_MSG (unsigned char) 1
#define VS_GET (unsigned char) 0
#define VS_PUT (unsigned char) 1
#define VS_MIN_SIZE 8
#define VS_MAX_SIZE 64

// Defines for spatial and temporal timeouts.
#define VS_MAX_TUPLE_DISTANCE 100
#define VS_MAX_TUPLE_AGE 100

// Defines for truth.
typedef int bool;
#define true 1
#define false 0

/*
 * Basic struct for holding tuple data in the virtual stigmergy table.
 */
typedef struct VsTuples {
    // Tuple user data.
    uint8_t key; // Key of the tuple that this broadcast affects.
    uint8_t posX; // X-position of this tuple.
    uint8_t posY; // Y-position of this tuple.
    uint8_t id; // ID of the agent that created this tuple.
    uint16_t value; // Value of this tuple.
    uint16_t timestamp; // Lamport clock timestamp of this tuple.

    // Tuple metadata.
    uint64_t lastAccessed; // Last timestamp when this tuple was accessed.
} VsTuple;

/*
 * Basic struct for condensing virtual stigmergy broadcast information.
 */
typedef struct VsBroadcasts {
    int action; // Virtual stigmergy action that this broadcast represents.
    VsTuple tuple; // Tuple that this broadcast affects.
} VsBroadcast;

/*
 * Initializes the virtual stigmergy table. This method must be invoked before
 * any other methods are invoked.
 *
 * @param localId ID of the agent this virtual stigmergy is running on.
 *
 * @return true (1) if initialization succeeds and false (0) otherwise.
 */
extern bool vsInit(uint8_t localId);

/*
 * Retrieves the local ID of this virtual stigmergy agent. When trying to
 * place this agent's ID into a virtual stigmergy value, this method should
 * be used.
 *
 * @return The local ID of the agent this virtual stigmergy is running on.
 */
extern uint8_t getVsLocalId();

/*
 * @return The current X location of the agent this virtual stigmergy is running on.
 */
extern uint8_t getVsLocationX();

/*
 * @return The current Y location of the agent this virtual stigmergy is running on.
 */
extern uint8_t getVsLocationY();

/*
 * Sets the X/Y coordinates of the agent this virtual stigmergy is running on.
 *
 * Whenever the agent is aware of its position changing, this method should be
 * invoked so that the virtual stigmergy tables can provide the correct spatial
 * telemetry when performing operations.
 *
 * @param x X-coordinate of the agent.
 * @param y Y-coordinate of the agent.
 */
extern void setVsLocation(uint8_t x, uint8_t y);

/**
 * Calculates the approximate distance from this agent to the specified tuple.
 *
 * @param tuple Tuple to calculate the distance to.
 *
 * @return Approximate distance to the given tuple.
 */
extern float distanceToTuple(VsTuple tuple);

/**
 * Calculates the approximate distance from this agent to the specified tuple.
 *
 * TODO: Should this get unknown tuples?
 *
 * @param key Key of the tuple to calculate the distance to.
 *
 * @return Approximate distance to the given tuple. If this unit does not
 *         possess a copy of the tuple, -1 will be returned.
 */
extern float distanceToTupleWithKey(uint8_t key);

/*
 * Invoked whenever there's a conflict in two values (identical timestamps
 * written by different agents).
 *
 * TODO: For now, robots with bigger IDs win always! Ha-ha!
 * TODO: Actual implementation should be extern'd and user-defined.
 *
 * @param key Key of the conflicted value.
 * @param localValue Local value (the value in this virtual stigmergy table).
 * @param remoteValue Remote value (the value received from a foreign table).
 *
 * @return The winning value.
 */
extern VsTuple vsOnConflict (uint8_t key, VsTuple localValue, VsTuple remoteValue);

/*
 * Invoked whenever this agent loses a conflict to a remote agent.
 *
 * TODO: Actual implementation should be extern'd and user-defined.
 *
 * @param key Key of the conflicted value.
 * @param winningValue Value that won the conflict.
 */
extern void vsOnConflictLost(uint8_t key, VsTuple winningValue);

/*
 * Invoked whenever a broadcast is received from another agent.
 *
 * @param broadcast VsBroadcast tuple containing all broadcast data.
 */
extern void onBroadcastReceived(VsBroadcast broadcast);

/*
 * Encodes a VsBroadcast message as an array of 9 8-bit integers.
 *
 * @param broadcast VsBroadcast to encode.
 * @param values Size 9 array of integers to place the encoded data into.
 */
extern void encodeVsBroadcast(VsBroadcast broadcast, uint8_t *values);

/*
 * Decodes an array of 9 8-bit integers into a VsBroadcast message.
 *
 * Only arrays whose 9th value equals IS_VS_BROADCAST_MESSAGE will
 * be decodedd; all others will be ignored.
 *
 * @param broadcast Size 9 array of integers to decode.
 * @param b VsBroadcast to place the decoded data into.
 *
 * @return true if the broadcast was decoded, and false otherwise.
 */
extern bool decodeVsBroadcast(uint8_t *broadcast, VsBroadcast *b);

/*
 * Returns true if this virtual stigmergy has a mapping for the given
 * key, and false otherwise.
 *
 * @param key Key to check.
 *
 * @return true (1) if the key exists in this table and false (0) otherwise.
 */
extern int has(uint8_t key);

/*
 * @return the total number of tuples present in this virtual stigmergy.
 */
extern int vsSize();

/*
 * Inserts or updates a value in this virtual stigmergy table at
 * this agents current location.
 *
 * @param key Key of the value to put.
 * @param value Value to put.
 */
extern void put(uint8_t key, uint16_t value);

/*
 * Inserts or updates a value in this virtual stigmergy at
 * the given location.
 *
 * @param key Key of the value to put.
 * @param value Value to put.
 * @param posX X-coordinate to place the value at.
 * @param posY Y-coordinate to place the value at.
 */
extern void putAt(uint8_t key, uint16_t value, uint8_t posX, uint8_t posY);

/*
 * Retrieves the value of a key in this virtual stigmergy table.
 *
 * @param key Key to retrive the value of.
 *
 * @return The value of the given key. If the key does not exist, 0 is returned.
 */
extern uint16_t get(uint8_t key);

/*
 * Retrieves the tuple of a key in this virtual stigmergy table.
 *
 * @param key Key to to retrive the value of.
 *
 * @return The tuple of the given key. If the key does not exist, an empty tuple is returned.
 */
extern VsTuple getTuple(uint8_t key);

/*
 * Retrieves all of the tuples within a given location.
 *
 * @param posX X-coordinate to look for tuples at.
 * @param posY Y-coordiante to look for tuples at.
 * @param radius Radius around the X/Y coordinates to look for tuples at.
 * @param array Array of size {@link #VS_SIZE} which the found tuples will be placed in.
 *
 * @return The number of found tuples.
 */
extern int getTupleAt(uint8_t posX, uint8_t posY, uint8_t radius, VsTuple* array);

// External Configuration options /////////////////////////////////////////

// Configurable VS_SIZE. Defaults to 64 if not set before
// this header file is included. This size MAY NOT
// exceed the value defined by VS_MAX_SIZE and MAY NOT be less than 8.
#ifndef VS_SIZE
#define VS_SIZE 64
#endif

// Configurable VS_SIZE for the mininmum amount of space that must
// be reserved for active elements in the VS table.
#ifndef VS_SIZE_MIN_ACTIVE
#define VS_SIZE_MIN_ACTIVE VS_SIZE / VS_MIN_SIZE
#endif

// Configurable VS_SIZE for the minimum amount of space that must be
// reserved for passive elements in the VS table.
#ifndef VS_SIZE_MIN_PASSIVE
#define VS_SIZE_MIN_PASSIVE VS_SIZE / VS_MIN_SIZE
#endif

// Validate that VS map size does not exceed VS_MAX_SIZE.
// TODO: Enabling this error reporting causes simulations to not work correctly. But why?
// #if (VS_SIZE > VS_MAX_SIZE)
// #error VS_SIZE may not exceed VS_MAX_SIZE
// #endif

/*
 * Invoked whenever a broadcast must be transmitted. This method must
 * be defined by the agent's code.
 *
 * @param broadcast Broadcast to transmit.
 */
extern void onBroadcastTransmit(VsBroadcast broadcast);

// End External Configuration options /////////////////////////////////////

#endif
