/*
 * Default Calico firmware that provides a "dumb" firmware interface to be controlled
 * purely by an overhead controller driven via the Kilobot Calico Driver.
 *
 * @author Brandon Sanders [brandon@alicorn.io]
 */
#include "kilolib.h"
#include "kilobotCalicoFirmwareHelper.h"

void onCalicoMessageReceived(uint8_t* msg) {

}

void setup() {

}

void loop() {

}

int main() {
    return defaultCalicoKilobotMainSetup();
}
