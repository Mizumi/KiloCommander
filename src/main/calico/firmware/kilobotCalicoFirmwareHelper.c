#include "kilobotCalicoFirmwareHelper.h"

// Ring buffer for storing messages to broadcast.
#define CALICO_BUFFER_MAX_SIZE 8
int16_t head = 0;
int16_t tail = 0;
message_t buffer[CALICO_BUFFER_MAX_SIZE];

// Get next message on request.
message_t *message_tx() {
    return getNextCalicoMessage();
}

// Progress message head on success.
void message_tx_success() {
    progressNextCalicoMessage();
}

// Process incoming messages.
void message_rx(message_t *msg, distance_measurement_t *dist) {
    decodeAndProcessCalicoMessage(msg);
}

// Handle Virtual Stigmergy broadcast requests.
void onBroadcastTransmit(VsBroadcast broadcast) {
    queueVsBroadcast(broadcast);
}

message_t *getNextCalicoMessage() {
    if (head - tail > 0) {
        return &buffer[tail % CALICO_BUFFER_MAX_SIZE];
    } else {
        return NULL;
    }
}

int progressNextCalicoMessage() {
    // Only progress if we're behind. It makes no sense to progress beyond
    // the writing head...
    if (tail < head) {
        tail += 1;

        // Attempt to reset head/tail to zero on occasion in order to avoid overflow.
        if (head == tail && head % CALICO_BUFFER_MAX_SIZE == 0) {
            head = 0;
            tail = 0;
        }

        return 0;
    } else {
        return -1;
    }
}

void decodeAndProcessCalicoMessage(message_t *msg) {

    // Is it a VS Broadcast (most common case.)
    if (msg->data[0] == MSG_SEND_VS_BROADCAST) {
        // Prepare struct to decode into.
        VsBroadcast b;
        b.action = 0; // Meaningless assignment in order to force init struct.

        // Shift broadcast array to front.
        // TODO: Confirm no off-by-one errors here.
        // TODO: Can be removed once VS is fixed to only use 7 elements.
        int i;
        for (i = 0; i < 8; i++) {
            msg->data[i] = msg->data[i + 1];
        }

        // Attempt to decode; if it's not a VsBroadcast, this will fail.
        if (decodeVsBroadcast(msg->data, &b)) {
            // Process broadcast.
            onBroadcastReceived(b);
        } else {
            fprintf(stderr, "Could not decode VS broadcast because it isn't a VS broadcast.");
        }

    // Is it a regular broadcast?
    } else if (msg->data[0] == MSG_SEND_BROADCAST) {
        // Hand off processing to the user.
        onCalicoMessageReceived(msg->data);

    // Is it a regular message targeted towards this unit?
    } else if (msg->data[0] == MSG_SEND_MSG &&
               msg->data[1] <= kilo_uid &&
               msg->data[2] >= kilo_uid) {
       // Hand off processing to the user.
       onCalicoMessageReceived(msg->data);

   // Is it a position assignment message targeted towards this unit?
   } else if (msg->data[0] == MSG_SET_POS &&
              msg->data[1] == kilo_uid) {
      // Set our local VS position.
      setVsLocation(msg->data[2], msg->data[3]);
      setVsRotation(msg->data[4]);

   // Is it a color assignment message targeted towards this unit?
   } else if (msg->data[0] == MSG_SET_COLOR &&
              msg->data[1] <= kilo_uid &&
              msg->data[2] >= kilo_uid) {
      // Set our color
      set_color(msg->data[3]);

   // Is it a motor assignment message targeted towards this unit?
   } else if (msg->data[0] == MSG_SET_MOTORS &&
              msg->data[1] <= kilo_uid &&
              msg->data[2] >= kilo_uid) {
      // Set our motors.
      set_motors(msg->data[3], msg->data[4]);

   // Unknown message; oh my!
   } else {
       fprintf(stderr, "Unknown Message RX / Kilo Type: %u / Calico Type: %u\n", msg->type, msg->data[0]);
       fprintf(stderr, "Data: [");
       int i = 0;
       for (i = 0; i < 9; i++) {
           fprintf(stderr, "%d: %u, ", i, msg->data[i]);
       }
       fprintf(stderr, "]\n");
   }
}

/**
 * Helper method for queuing the broadcast messages.
 *
 *
 * @param payload Payload to broadcast.
 * @param type Message type to broadcast.
 *
 * @return 0 on success, -1 on failure.
 */
int _queueCalicoBroadcastHelper(uint8_t* payload, uint8_t type) {

    // Only transmit if there is space to do so.
    if ((head - tail) < CALICO_BUFFER_MAX_SIZE) {

        // Initialize message structure.
        message_t message;
        message.type = NORMAL;

        // Encode message metadata.
        message.data[0] = type;

        // Encode message payload.
        int i;
        for (i = 0; i < MSG_MAX_SIZE - 1; i++) {
            message.data[i + 1] = payload[i];
        }

        // Calculate checksum.
        message.crc = message_crc(&message);

        // Add the message to the ringbuffer and progress the head.
        buffer[head % CALICO_BUFFER_MAX_SIZE] = message;
        head += 1;

        return 0;
    } else {
        return -1;
    }
}

int queueCalicoBroadcast(uint8_t* payload) {
    return _queueCalicoBroadcastHelper(payload, MSG_SEND_BROADCAST);
}

int queueVsBroadcast(VsBroadcast broadcast) {
    // Encode message for broadcast.
    uint8_t payload[MSG_MAX_SIZE];
    encodeVsBroadcast(broadcast, payload);

    // Queue it for broadcast.
    return _queueCalicoBroadcastHelper(payload, MSG_SEND_VS_BROADCAST);
}

int defaultCalicoKilobotMainSetup() {
    kilo_init();
    kilo_message_tx = message_tx;
    kilo_message_rx = message_rx;
    kilo_message_tx_success = message_tx_success;
    kilo_start(setup, loop);

    return 0;
}
