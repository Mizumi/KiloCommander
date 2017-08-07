#include "vs.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Has VS been initialized?
bool vsInitialized = false;

// Local ID of this system.
uint8_t vsLocalId;

// Location of this system in space.
uint8_t vsX = 0;
uint8_t vsY = 0;

// Virtual Stigmergy table.
VsTuple vsMap[VS_SIZE];

// Virtual Stigmergy table index, used to map from keys to an actual index in the table.
int vsMapIndexActiveHead = 0;
int vsMapIndexPassiveHead = VS_SIZE - 1;

// Current age of tuples in the Virtual Stigmergy table (lamport clock based on access).
uint64_t vsMapLamportClock = 0;

/**
 * Helper method to create an empty VsTuple.
 */
VsTuple _createEmptyVsTuple() {
    VsTuple tuple;
    tuple.id = 0; // TODO: Initialize to vsLocalId?
    tuple.timestamp = 0;
    tuple.key = 0;
    tuple.value = 0;
    tuple.posX = 0; // TODO: Initialize to vsX?
    tuple.posY = 0; // TODO: Initialize to vsY?
    tuple.lastAccessed = 0; // TODO: Initialize to vsMapLamportClock?

    return tuple;
}

/**
 * Returns the index of the tuple with the associated key, or -1 if no such index is found.
 */
int _getIndexOfTupleWithKey(uint8_t key) {
    // Tuple index holder.
    int i;

    // Look for the key among our watched tuples.
    for (i = 0; i < vsMapIndexActiveHead; i++) {
        if (vsMap[i].timestamp > 0 && vsMap[i].key == key) {
            return i;
        }
    }

    // Look for the key among our passive tuples.
    for (i = vsMapIndexPassiveHead + 1; i < VS_SIZE; i++) {
        if (vsMap[i].timestamp > 0 && vsMap[i].key == key) {
            return i;
        }
    }

    // Key not found.
    return -1;
}

/**
 * Helper method to determine if a tuple is in the active space.
 */
bool _isActiveTuple(uint8_t key) {
    int idx = _getIndexOfTupleWithKey(key);
    return (idx >= 0) && (idx < vsMapIndexActiveHead);
}

/**
 * Helper method to determine if a tuple is in the passive space.
 */
bool _isPassiveTuple(uint8_t key) {
    int idx = _getIndexOfTupleWithKey(key);
    return (idx < VS_SIZE) && (idx > vsMapIndexPassiveHead);
}

/**
 * Returns the index of the oldest active tuple, or -1 if there are none.
 */
int _getIndexOfOldestActiveTuple() {
    // If there are no active tuples, exit.
    if (vsMapIndexActiveHead <= 0) {
        return -1;
    }

    // Tuple index holder.
    int idx = -1;

    // Oldest seen age so far. Initialize to the maximum value of a uint64.
    uint64_t oldestTupleAge = UINT64_MAX;

    // Iterate over all active tuples, finding the oldest one.
    int i;
    for (i = 0; i < vsMapIndexActiveHead; i++) {
        if (vsMap[i].timestamp > 0 && vsMap[i].lastAccessed < oldestTupleAge) {
            idx = i;
            oldestTupleAge = vsMap[i].lastAccessed;
        }
    }

    // Return the index. If, for some reason, none were found, return -1.
    return idx;
}

/**
 * Returns the index of the furthest-away passive tuple, or -1 if there are none.
 */
int _getIndexOfFurthestPassiveTuple() {
    // If there are no passive tuples, exit.
    if (vsMapIndexPassiveHead + 1 >= VS_SIZE) {
        return -1;
    }

    // Tuple index holder.
    int idx = -1;

    // Furthest seen distance so far. Initialize to a negative value.
    float furthestDistance = -1;

    // Iterate over all tuples, finding the furthest one.
    int i;
    for (i = vsMapIndexPassiveHead + 1; i < VS_SIZE; i++) {
        float distance = distanceToTuple(vsMap[i]);
        if (vsMap[i].timestamp > 0 && distance > furthestDistance) {
            idx = i;
            furthestDistance = distance;
        }
    }

    // Return the index. If, for some reason, none were found, return -1.
    return idx;
}

/**
 * Helper method to remove tuples from the stigmergy table.
 *
 * @param key Key of the tuple to remove.
 */
void _removeTuple(uint8_t key) {
    // Only remove tuples that need to be removed.
    if (_getIndexOfTupleWithKey(key) >= 0) {

        // Shuffle all tuples that come later backwards (active tuples).
        if (_isActiveTuple(key)) {
            int i;
            for (i = _getIndexOfTupleWithKey(key); i < vsMapIndexActiveHead; i++) {
                if (i + 1 < vsMapIndexActiveHead) {
                    // Shuffle tuple backwards.
                    vsMap[i] = vsMap[i + 1];

                // Fail-fast if the next index is out of bounds.
                } else {
                    break;
                }
            }

            // Move the head backwards once.
            vsMapIndexActiveHead = vsMapIndexActiveHead - 1;

        // Shuffle all tuples that come later forwards (passive tuples).
        } else {
            int i;
            for (i = _getIndexOfTupleWithKey(key); i > vsMapIndexPassiveHead; i--) {
                if (i - 1 > vsMapIndexPassiveHead) {
                    // Shuffle tuple forwards.
                    vsMap[i] = vsMap[i - 1];

                // Fail-fast if the next index is out of bounds.
                } else {
                    break;
                }
            }

            // Move the head backwards once.
            vsMapIndexPassiveHead = vsMapIndexPassiveHead + 1;
        }

        // Clear index.
        vsMap[_getIndexOfTupleWithKey(key)] = _createEmptyVsTuple();
    }
}

/**
 * Helper method to prune any tuples that are too far away or too old.
 */
void _pruneTuples() {
    int i;
    for (i = 0; i < VS_SIZE; i++) {
        if (_getIndexOfTupleWithKey(i) > -1 &&
            ((vsMapLamportClock - vsMap[_getIndexOfTupleWithKey(i)].lastAccessed > VS_MAX_TUPLE_AGE) ||
             (distanceToTupleWithKey(i) > VS_MAX_TUPLE_DISTANCE))) {
            _removeTuple(i);
        }
    }
}

/**
 * Helper method to insert tuples into this stigmergy table.
 *
 * @param key Key of the tuple to insert.
 * @param tuple Tuple to insert.
 * @param triggeredByAgent True if this operation is being explicitly triggered
 *                         by the agent and false if this operation is happening
 *                         passively due to a broadcast or other non-agent
 *                         action.
 */
void _insertTuple(uint8_t key, VsTuple tuple, bool triggeredByAgent) {

    // Update this tuple's access time.
    tuple.lastAccessed = vsMapLamportClock;

    // Find the index for this tuple (it may not exist).
    int idx = _getIndexOfTupleWithKey(key);

    // If the tuple is not in our table, insert it immediately.
    if (idx < 0) {

        // Insert as an active tuple.
        if (triggeredByAgent) {

            // We have room!
            // TODO: Validate this logic.
            if (vsMapIndexActiveHead <= min(vsMapIndexPassiveHead, VS_SIZE - VS_SIZE_MIN_PASSIVE - 1)) {

                // Insert the tuple.
                vsMap[vsMapIndexActiveHead] = tuple;

                // Progress head.
                vsMapIndexActiveHead++;

            // We're out of room, and the only solution is to overwrite.
            // Demote the oldest active tuple to a passive tuple.
            } else {

                // Find the oldest active tuple.
                int idx = _getIndexOfOldestActiveTuple();

                // Grab a copy of it.
                VsTuple oldestTuple = vsMap[idx];

                // Remove it from the map.
                _removeTuple(oldestTuple.key);

                // Insert the new tuple.
                // This is ok because the active head will have been moved backwards
                // by _removeTuple(...).
                vsMap[vsMapIndexActiveHead] = tuple;

                // Progress the active head.
                // This is ok because the active head will have been moved backwards
                // by _removeTuple(...).
                vsMapIndexActiveHead++;

                // Re-insert the old tuple as a passive tuple.
                _insertTuple(oldestTuple.key, oldestTuple, false);
            }

        // Insert as a passive tuple.
        } else {

            // We have room!
            // TODO: Validate this logic.
            if (vsMapIndexPassiveHead >= max(vsMapIndexActiveHead, VS_SIZE_MIN_ACTIVE)) {

                    // Insert the tuple.
                    vsMap[vsMapIndexPassiveHead] = tuple;

                    // Progress head.
                    vsMapIndexPassiveHead--;

            // We're out of room, and the only solution is to overwrite.
            // Replace the furthest passive tuple.
            } else {

                // Find the furthest passive tuple.
                int idx = _getIndexOfFurthestPassiveTuple();

                // Grab a copy of it.
                VsTuple furthestTuple = vsMap[idx];

                // Only proceed if this tuple is closer than that tuple;
                // there is no point in inserting an even further away tuple.
                if (distanceToTuple(tuple) < distanceToTuple(furthestTuple)) {

                    // Remove it from the map.
                    _removeTuple(furthestTuple.key);

                    // Insert the new tuple.
                    // This is ok because the passive head will have been moved backwards
                    // by _removeTuple(...).
                    vsMap[vsMapIndexPassiveHead] = tuple;

                    // Progress the passive head.
                    // This is ok because the passive head will have been moved backwards
                    // by _removeTuple(...).
                    vsMapIndexPassiveHead--;
                }
            }
        }

        // Progress access clock.
        vsMapLamportClock++;

    // If the tuple is in our table but has a mismatch in its status,
    // promote it to active state.
    } else if (triggeredByAgent && _isPassiveTuple(idx)) {

        // Remove the tuple.
        _removeTuple(key);

        // Re-insert the tuple as an active tuple.
        _insertTuple(key, tuple, triggeredByAgent);

        // We do not progress the access clock because the recursive call above will handle that for us.
        return;

    // Otherwise, perform a direct value update.
    } else {
        // Insert tuple.
        vsMap[idx] = tuple;

        // Progress access clock.
        vsMapLamportClock++;
    }
}

/**
 * Helper method to retrieve tuples from this stigmergy table.
 *
 * @param key Key of the tuple to retrieve.
 * @param triggeredByAgent True if this operation is being explicitly triggered
 *                         by the agent and false if this operation is happening
 *                         passively due to a broadcast or other non-agent
 *                         action.
 *
 * @return The retrieved tuple.
 */
VsTuple _retrieveTuple(uint8_t key, bool triggeredByAgent) {

    // Identify the tuple index.
    int idx = _getIndexOfTupleWithKey(key);

    // If we don't have the tuple, return a blank one.
    if (idx < 0) {

        // Print a warning. TODO: Needed? This gets triggered a lot because of
        //                        the way broadcasts are handled.
        // fprintf(stderr, "Access out of bounds in _retrieveTuple.\n");

        // Create an empty tuple.
        VsTuple tuple = _createEmptyVsTuple();

        // TODO: Verify whether or not we need to insert the blank tuple
        //       as a watched tuple if this was agent-triggered. We might
        //       not need to.
        // _insertTuple(key, tuple, triggeredByAgent);

        // Return a blank tuple.
        return tuple;

    // Otherwise, return the one we have.
    } else {
        // Update last access time.
        vsMap[idx].lastAccessed = vsMapLamportClock;

        // Fetch the tuple.
        VsTuple tuple = vsMap[idx];

        // If the tuple is in our table but has a mismatch in its status,
        // promote it to active state.
        if (triggeredByAgent && _isPassiveTuple(idx)) {

            // Remove the tuple.
            _removeTuple(key);

            // Re-insert the tuple as an active tuple.
            _insertTuple(key, tuple, triggeredByAgent);

            // We do not progress the access clock because the recursive call above will handle that for us.

        // Otherwise, progress the clock and return the tuple directly.
        } else {
            // Progress lamport clock.
            vsMapLamportClock++;
        }

        // Return the tuple.
        return tuple;
    }
}

bool vsInit(uint8_t localId) {
    if (!vsInitialized) {
        vsLocalId = localId;

        // TODO: Do we need to initialize the dual tables in any way?

        // Table is now initialized.
        vsInitialized = true;

        return true;
    } else {
        return false;
    }
}

uint8_t getVsLocalId() {
    return vsLocalId;
}

uint8_t getVsLocationX() {
    return vsX;
}

uint8_t getVsLocationY() {
    return vsY;
}

void setVsLocation(uint8_t x, uint8_t y) {
    vsX = x;
    vsY = y;
}

float distanceToTuple(VsTuple tuple) {
    return sqrt(pow(vsX - vsY, 2) + pow(tuple.posX - tuple.posY, 2));
}

float distanceToTupleWithKey(uint8_t key) {
    if (has(key)) {
        return distanceToTuple(_retrieveTuple(key, false));
    } else {
        return -1;
    }
}

VsTuple vsOnConflict (uint8_t key, VsTuple localValue, VsTuple remoteValue) {
    // Higher IDs will always win.
    if (localValue.id > remoteValue.id) {
        return localValue;
    } else {
        return remoteValue;
    }
}

void vsOnConflictLost(uint8_t key, VsTuple winningValue) { }

void executeConflictManagement(uint8_t key, VsTuple localValue, VsTuple remoteValue) {
    // Resolve the conflict via our handler.
    _insertTuple(key, vsOnConflict(key, localValue, remoteValue), false);

    // If this robot lost the conflict, invoke the conflict lost callback.
    if (localValue.id != _retrieveTuple(key, false).id && localValue.id == vsLocalId) {
        vsOnConflictLost(key, remoteValue);
    }
}

void broadcastTuple(uint8_t key, int method) {
    // Setup broadcast struct.
    VsBroadcast broadcast;
    broadcast.action = method;

    // Broadcast a "default" tuple if the given key doesn't exist.
    VsTuple tuple = _retrieveTuple(key, false);
    if (tuple.timestamp > 0) {
        broadcast.tuple = tuple;
    } else {
        broadcast.tuple.key = key;
        broadcast.tuple.value = 0;
        broadcast.tuple.timestamp = 0;
        broadcast.tuple.id = vsLocalId;
    }

    onBroadcastTransmit(broadcast);
}

void onBroadcastReceived(VsBroadcast broadcast) {

    // Preload timestamps.
    VsTuple localTuple = _retrieveTuple(broadcast.tuple.key, false);
    uint16_t localTimestamp = localTuple.timestamp;
    uint16_t remoteTimestamp = broadcast.tuple.timestamp;

    // Handle PUT requests.
    if (broadcast.action == VS_PUT) {
        // If the broadcast tuple is newer than ours, update and retransmit.
        if (remoteTimestamp > localTimestamp) {
            _insertTuple(broadcast.tuple.key, broadcast.tuple, false);
            broadcastTuple(broadcast.tuple.key, VS_PUT);
        }

        // If it's the same timestamp and a different robot, perform conflict management.
        else if (localTimestamp == remoteTimestamp && localTuple.id != broadcast.tuple.id) {
            executeConflictManagement(broadcast.tuple.key, localTuple, broadcast.tuple);
        }
    }

    // Handle GET requests.
    else if (broadcast.action == VS_GET) {
        // If it's the same timestamp and a different robot, perform conflict management.
        if (localTimestamp == remoteTimestamp &&
            localTuple.id != broadcast.tuple.id &&
            remoteTimestamp != 0) {
            executeConflictManagement(broadcast.tuple.key, localTuple, broadcast.tuple);
        }

        // Update remote; they're out of date!
        else if (remoteTimestamp < localTimestamp) {
            broadcastTuple(broadcast.tuple.key, VS_PUT);
        }

        // Update local; we're out of date!
        else if (localTimestamp < remoteTimestamp) {
            _insertTuple(broadcast.tuple.key, broadcast.tuple, false);
            broadcastTuple(broadcast.tuple.key, VS_PUT);
        }
    }

    // Prune tuples.
    // _pruneTuples();
}

void encodeVsBroadcast(VsBroadcast broadcast, uint8_t *values) {

    // Encode action and msg type information into the message.
    //
    // We forcibly limit (via %) the range of the keys to only 64
    // values. This allows us to encode the PUT or GET as the
    // eigth bit in our 8-bit zeroth value, giving us room to
    // pack location data and other valuable data in the messages.
    values[0] = (VS_MSG << 7) | (broadcast.action << 6) | (broadcast.tuple.key % 64);

    // Encode tuple data in the message.
    values[1] = (uint8_t) (broadcast.tuple.posX);
    values[2] = (uint8_t) (broadcast.tuple.posY);
    values[3] = (uint8_t) (broadcast.tuple.value & 0xff);
    values[4] = (uint8_t) (broadcast.tuple.value >> 8);
    values[5] = (uint8_t) (broadcast.tuple.timestamp & 0xff);
    values[6] = (uint8_t) (broadcast.tuple.timestamp >> 8);
    values[7] = (uint8_t) (broadcast.tuple.id);
    values[8] = 0;
}

bool decodeVsBroadcast(uint8_t *broadcast, VsBroadcast *b) {

    // We only decode VS broadcasts.
    if ((broadcast[0] >> 7) == VS_MSG) {
        // Decode action information from the message.
        b->action = ((broadcast[0] & 0b01000000) >> 6);
        b->tuple.key = (broadcast[0] & 0b00111111);

        // Decode tuple data from the message.
        b->tuple.posX = (uint8_t) broadcast[1];
        b->tuple.posY = (uint8_t) broadcast[2];
        b->tuple.value = (uint16_t) ((broadcast[4] << 8) | broadcast[3]);
        b->tuple.timestamp = (uint16_t) ((broadcast[6] << 8) | broadcast[5]);
        b->tuple.id = (uint8_t) (broadcast[7]);

        // Zero out metadata.
        b->tuple.lastAccessed = 0;
        return true;
    } else {
        return false;
    }
}

int has(uint8_t key) {
    return _getIndexOfTupleWithKey(key) >= 0;
}

int vsSize() {
    // TODO: Verify this logic.
    int numActive = vsMapIndexActiveHead;
    int numPassive = VS_SIZE - vsMapIndexPassiveHead - 1;
    return numActive + numPassive;
}

void put(uint8_t key, uint16_t value) {
    putAt(key, value, vsX, vsY);
}

void putAt(uint8_t key, uint16_t value, uint8_t posX, uint8_t posY) {

    // Initialize the tuple state.
    VsTuple tuple = _retrieveTuple(key, true);
    tuple.id = vsLocalId;
    tuple.timestamp += 1;
    tuple.key = key;
    tuple.value = value;
    tuple.posX = posX;
    tuple.posY = posY;

    // Commit the tuple locally.
    _insertTuple(key, tuple, true);

    // Broadcast change.
    broadcastTuple(key, VS_PUT);
}

uint16_t get(uint8_t key) {
    // Return the tuple's value.
    return getTuple(key).value;
}

VsTuple getTuple(uint8_t key) {

    // Find the tuple.
    VsTuple tuple = _retrieveTuple(key, true);

    // Broadcast request for data updates.
    broadcastTuple(key, VS_GET);

    // Return the tuple.
    return tuple;
}

int getTupleAt(uint8_t posX, uint8_t posY, uint8_t radius, VsTuple* array) {

    // Number of found tuples.
    int found = 0;

    // Iterate over the entire VS map looking for tuples.
    int i;
    for (i = 0; i < VS_SIZE; i++) {

        // Grab a reference to the tuple.
        VsTuple tuple = vsMap[i];

        // Does the tuple exist and lie within our circle?
        if (tuple.timestamp > 0 &&
            square(tuple.posX - posX) + square(tuple.posY - posY) <= square(radius)) {

            // Add it the list of found tuples.
            array[found] = tuple;

            // Increment the number of found tuples.
            found += 1;
        }
    }

    // Return the number of found tuples.
    return found;
}
