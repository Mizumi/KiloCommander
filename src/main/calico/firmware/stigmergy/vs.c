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
// TODO: Make size configurable, blah, blah.
VsTuple vsMap[VS_SIZE];

// Current number of elements in the Virtual Stigmergy table (total).
int vsMapSize = 0;

// Write heads for watched and passive values.
int vsMapWatchedHead = 0;
int vsMapPassiveHead = VS_SIZE - 1;

// "Clock" for tracking access rate of elements.
uint64_t vsMapAccessClock = 0;

/**
 * TODO:
 *
 * @param idx
 *
 *  @return [description]
 */
bool _isWatched(int idx) {
    return (idx >= 0) && (idx < vsMapWatchedHead);
}

/**
 * TODO:
 *
 * @method _isPassive
 * @param idx
 * @return [description]
 */
bool _isPassive(int idx) {
    return (idx < VS_SIZE) && (idx > vsMapPassiveHead);
}

/**
 * TODO:
 *
 * @param key
 *
 * @return [description]
 */
int _getIndexOfTupleWithKey(uint8_t key) {
    int i;

    // Look for the key among our watched tuples.
    for (i = 0; i < vsMapWatchedHead; i++) {
        if (vsMap[i].key == key) {
            return i;
        }
    }

    // Look for the key among our passive tuples.
    for (i = vsMapPassiveHead + 1; i < VS_SIZE; i++) {
        if (vsMap[i].key == key) {
            return i;
        }
    }

    // Key not found.
    return -1;
}

/**
 * TODO:
 *
 * @return [description]
 */
uint8_t _keyOfFurthestPassiveTuple() {
    // If there are no passive tuples, exit.
    if (vsMapPassiveHead + 1 >= VS_SIZE) {
        return -1;
    }

    // Furthest tuple index.
    int idx = vsMapPassiveHead + 1;

    // Furthest tuple distance so far. Initialize to the first element.
    float furthestDistance = distanceTo(vsMap[idx]);

    // Iterate over all tuples, finding the furthest one.
    int i;
    for (i = vsMapPassiveHead + 2; i < VS_SIZE; i++) {
        if (distanceTo(vsMap[i]) > furthestDistance) {
            idx = i;
            furthestDistance = distanceTo(vsMap[i]);
        }
    }

    // Return index.
    return idx;
}

/**
 * TODO:
 *
 * @return [description]
 */
uint8_t _keyOfOldestWatchedTuple() {
    // If there are no watched tuples, exit.
    if (vsMapWatchedHead <= 0) {
        return -1;
    }

    // Oldest tuple index.
    int idx = 0;

    // Oldest tuple age so far.
    uint64_t oldestTuple = vsMap[idx].accessed;

    // Iterate over all tuples, finding the oldest one.
    int i;
    for (i = 1; i < vsMapWatchedHead; i++) {
        if (vsMap[i].accessed < oldestTuple) {
            idx = i;
            oldestTuple = vsMap[i].accessed;
        }
    }

    return idx;
}

/**
 *
 * TODO:
 *
 * @param key
 * @param tuple
 */
void _addPassiveTuple(uint8_t key, VsTuple tuple, bool overwriteWatchedTuples) {
    // Perform insertion only if there's space.
    if (vsMapPassiveHead >= max(vsMapWatchedHead, VS_SIZE_MIN_WATCHED)) {
        // Insert the tuple into the passive section of our table.
        vsMap[vsMapPassiveHead] = tuple;

        // Progress head.
        vsMapPassiveHead = min(vsMapPassiveHead - 1, VS_SIZE - 1);
    } else {

        // Can we overwrite any watched tuples?
        if (overwriteWatchedTuples && vsMapPassiveHead >= VS_SIZE_MIN_WATCHED) {
            // Determine the oldest watched tuple.
            uint8_t key = _keyOfOldestWatchedTuple();

            // Overwrite that tuple.
            int keyIdx = _getIndexOfTupleWithKey(key);
            vsMap[keyIdx] = vsMap[vsMapPassiveHead];

            // Insert this tuple.
            vsMap[vsMapPassiveHead] = tuple;

            // Progress heads.
            vsMapWatchedHead = vsMapPassiveHead;
            vsMapPassiveHead -= 1;
        } else {
            // Determine the furthest passive tuple.
            uint8_t key = _keyOfFurthestPassiveTuple();

            // Overwrite the furthest passive tuple.
            int keyIdx = _getIndexOfTupleWithKey(key);
            vsMap[keyIdx] = tuple;
        }
    }
}

/**
 * Helper method to process and mark a tuple as watched.
 *
 * @param key Key of the tuple to watch.
 * @param tuple Tuple to watch.
 */
void _addWatchedTuple(uint8_t key, VsTuple tuple) {
    // Perform an insertion only if there's space.
    if (vsMapWatchedHead <= min(vsMapPassiveHead, VS_SIZE - VS_SIZE_MIN_PASSIVE - 1)) {
        // Insert the tuple into the watched section of our table.
        vsMap[vsMapWatchedHead] = tuple;

        // Progress head.
        vsMapWatchedHead += 1;
    } else {

        // Can we overwrite any passive tuples?
        if (vsMapPassiveHead < VS_SIZE - VS_SIZE_MIN_PASSIVE - 1) {
            // Determine the furthest away passive tuple.
            uint8_t k = _keyOfFurthestPassiveTuple();

            // Swap the furthest passive tuple with the edge of our
            // watched tuples.
            int keyIdx = _getIndexOfTupleWithKey(k);
            vsMap[keyIdx] = vsMap[vsMapWatchedHead];

            // Write the watched tuple in.
            vsMap[vsMapWatchedHead] = tuple;

            // Progress heads.
            vsMapPassiveHead = max(vsMapWatchedHead, vsMapPassiveHead);
            vsMapWatchedHead += 1;
        } else {
            // Determine the oldest watched tuple.
            uint8_t k = _keyOfOldestWatchedTuple();

            // Save the oldest tuple.
            int keyIdx = _getIndexOfTupleWithKey(k);
            VsTuple temp = vsMap[keyIdx];

            // Overwrite it with the new tuple.
            vsMap[keyIdx] = tuple;

            // Try to put the tuple back as a passive tuple
            // without overwriting the recent tuple.
            _addPassiveTuple(key, temp, true);
        }
    }
}

void _removePassiveTuple(int idx) {
    // Reverse the writing head for passive tuples.
    vsMapPassiveHead += 1;

    // Overwrite the data in this slot to replace with the last tuple.
    vsMap[idx] = vsMap[vsMapPassiveHead];
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

    // Set this tuple's access time.
    tuple.accessed = vsMapAccessClock;

    // Find the index for this tuple.
    int idx = _getIndexOfTupleWithKey(key);

    // See if we have the tuple in our table.
    if (idx < 0) {
        // No tuple exists for this key. Insert it.

        // If this is an agent-triggered operation, insert as watched.
        if (triggeredByAgent) {
            _addWatchedTuple(key, tuple);

        // Otherwise insert as passive, but do not override watched tuples.
        } else {
            _addPassiveTuple(key, tuple, false);
        }

    // If we do, was this operation triggered by the agent?
    } else if (triggeredByAgent) {
        // Was this tuple stored in the passive table?
        if (_isPassive(idx)) {
            // Promote this tuple.

            // Remove this tuple from the passive list.
            _removePassiveTuple(idx);

            // Add it to the reecnt list.
            _addWatchedTuple(key, tuple);

        // This tuple is already watched. Update it.
        } else {
            // Perform update only.
            vsMap[idx] = tuple;
            // fprintf(stderr, "IT@ Unit %u tuple with ID %u and IDX %u is stored actively.\n", getVsLocalId(), key, idx);
        }

    // This operation was not triggered by the agent; update tuple only, but
    // do not change the tables it is in.
    } else {
        vsMap[idx] = tuple;
    }

    // Progress access clock.
    vsMapAccessClock += 1;
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

    // Create an empty tuple.
    VsTuple tuple;

    // Find the index for this tuple.
    int idx = _getIndexOfTupleWithKey(key);

    // Is the tuple not in our table?
    if (idx < 0) {
        // Create a "Default" empty tuple if the given tuple
        // key doesn't exist in our table.
        tuple.value = 0;
        tuple.posX = vsX;
        tuple.posY = vsY;
        tuple.timestamp = 0;
        tuple.id = vsLocalId;
        tuple.accessed = vsMapAccessClock;

        // Was this operation triggered by the agent?
        if (triggeredByAgent) {
            // Add the tuple as a watched tuple.
            _addWatchedTuple(key, tuple);
        }

        // TODO: Needed?
//        else {
//            // Add the tuple as a passive tuple.
//            _addPassiveTuple(key, tuple, false);
//        }
    } else {
        // Retrieve tuple information.
        tuple = vsMap[idx];

        // Update access time.
        tuple.accessed = vsMapAccessClock;
        vsMap[idx] = tuple;

        // If this operation was triggered by the agent,
        // was the tuple stored passively?
        if (triggeredByAgent && _isPassive(idx)) {
            // Promote the tuple to a watched tuple.

            // Remove it from the passive list.
            _removePassiveTuple(idx);

            // Add it as watched tuple.
            _addWatchedTuple(key, tuple);
        } else {
            // fprintf(stderr, "RT@ Unit %u tuple with ID %u and IDX %u is stored actively.\n", getVsLocalId(), key, idx);
        }
    }

    // Progress operation counter.
    vsMapAccessClock += 1;

    // Return the tuple.
    return tuple;
}

bool vsInit(uint8_t localId) {
    if (!vsInitialized) {
        vsLocalId = localId;

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

float distanceTo(VsTuple tuple) {
    return sqrt(pow(vsX - vsY, 2) + pow(tuple.posX - tuple.posY, 2));
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
        b->tuple.accessed = 0;
        return true;
    } else {
        return false;
    }
}

int has(uint8_t key) {
   return (_getIndexOfTupleWithKey(key) > -1);
}

int vsSize() {
    return vsMapSize;
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
