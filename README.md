# Net485
 A popular network protocol for RS485 / EIA-485 transmission networks in residential HVAC following OSI Layer conventions

This project is still very much a work in progress.  Each layer of the protocol, as it is added, will get it's own test script(s) and already Layer 1 is still very much usable for a variety of protocols.  What each layer provides is briefly listed below.

Layer 1: 

- Packet structure of 10-byte header, 0-240 byte data body, and 2-byte checksum number
- Ring buffer of N packets on receipt
- Management of Drive/Receive pin for a minimum inter-packet delay time, a pre-drive, and a post-drive time
