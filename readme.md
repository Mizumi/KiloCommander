# Kilo Commander
Kilo Commander is a C API for interacting with the Overhead Control Module for a swarm of Kilobots. Currently, it only supports UNIX systems.

## Payload Structure
### Command Payload Structure (0)
[0]: ID-FROM
[1]: ID-TO
[2]: L-Motor
[3]: R-Motor
[4]: R-LED
[5]: G-LED
[6]: B-LED
[8]: TYPE

### GPS Payload Structure (1)
[0]: X
[1]: Y
[8]: TYPE
