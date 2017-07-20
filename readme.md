# Kilo Commander
Kilo Commander is a C API for interacting with the Overhead Control Module for a swarm of Kilobots. Currently, it only supports UNIX systems.

## Firmware
Kilo Commander comes with  firmware that allows native C code on a desktop to communicate with C code on a resource-constrained embedded device. The firmware has two components, the ***driver*** and the ***firmware***.

### Driver
The driver APIs are executed on the server (desktop) that is controlling the embedded devices running the firmware. The header file can be found in `src/main/calico/driver/kilobotCalicoDriver.h` and relies on the presence of its corresponding source file, the `src/main/calico/kilobotCalicoDefinitions.h` file and the Kilo Commander source files. An example usage of this API is found in the root of this project as `kiloCommanderExampleCalicoServer.c`.

### Firmware
The firmware APIs are executed on the kilobots directly. The header file can be found in `src/main/calico/firmware/kilobotCalicoFirmwareHelper.h`, and relies on its corresponding source file, the Virtual Stigmergy files from the SpaceTimeVStig project, and the `src/main/calico/kilobotCalicoDefinitions.h` file. We've provided a blank template file in `src/main/calico/firmware/kilobotCalicoFirmwareDefault.c` for use with the firmware helper files which you can directly compile and run on a kilobot to test that your driver and firmware are working correctly together.
