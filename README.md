# Net485
 A popular network protocol for RS485 / EIA-485 transmission networks in residential HVAC following OSI Layer conventions

This project is still very much a work in progress.  Each layer of the protocol, as it is added, will get it's own test script(s) and already Layer 1 is still very much usable for a variety of protocols.  What each layer provides is briefly listed below.

Layer 1: 

- Packet structure of 10-byte header, 0-240 byte data body, and 2-byte checksum number
- Ring buffer of N packets on receipt
- Management of Drive/Receive pin for a minimum inter-packet delay time, a pre-drive, and a post-drive time

Layer 2:

- Declaration of header structure
- Checksum algorithm for header+data

Layers 3-5:

- The bulk of the work is here.  Version announcemnt, network coordination of who is master (coordinator), and automatic node assignment are working.  
- Data packets infrastructure in place along with 4 types of packet routing. 
- Token Offer Broadcast and correct handling of devices on coordinator side in case of device timeouts.
- Debugging messages from the network coordinator are now piped through a diagnostic message type of 15 characters (may use a "manufacturer's" specific message type here too as 15 characters is very tiny window for debugging info).
- Devices have been tested on physical hardware and now have been ported to a Unix type environment (Macos primarily)

Milestone 1 : 

A big change in project structure now.  Now, with the coordinator services running (test_network_furnace and test_network_thermostat), their 2nd serial lines are expected to be communicating with the respective device's application process.  The communication is the exact same packet structure as on the RS485 wire but is only the data packets relevant to the application (and some header values are switched as defined in CT documentation).

Presently, one can use the net485Listener tool to see packets to the device.  I'll be making respective utilities to emulate some devices.  This may be close to being useful for those that wish to make HVAC devices communicate now.

NOTE: Network shared data feature still is not implemented and remains to be done - this is to come soon once the devices can be virtualized and debuging/testing not so painfull.

# Net485Listen

Do check out the sub-project under 'Applications' folder - a very handy snooper tool for existing networks.  You got one of the following brands?  They all use the same protocol (albeit with their own extensions).  If any of you all are interested, I'm down for logging network activity and reverse-engineering and documenting all of these extensions and ultimately adding support to this library.  You can help out by snooping your own network and submitting samples of what is logged.  Data submitted is avilable under `diag\logs` folder.

- Carrier: Infinity (or Greenspeed)
- Bryant: Evolution
- Amana, Goodman and Daikin: ComfortNet
- Trane and American Standard: ComfortLink II
- Rheem and Ruud: Comfort Control System
- Lennox: iComfort
- Maytag, Tappan, Westinghouse and others: iQ Drive
- Heil, Comfortmaker, Keep Rite and others: Observer
- Armstrong Air: Comfort Sync
- York: Affinity
- Luxaire: Acclimate
- Coleman: Echelon

# Focus shifted

With the low level Coordinator functioning, there is a need for simulated networks in order to test device functionality.  A framwork dependancy has shifted from ORSSerialPort to [POSIXSerialPort](https://github.com/kpishere/POSIXSerialPort) as the latter supports physical and virtual devices equally well.  With everything virtuallized, development/test/debug cycles should be much quicker and progress is expected to accelerate.  As the workflow gets established, detail will be provided in how to construct it.  Initial testing with the use of [socat](https://medium.com/@copyconstruct/socat-29453e9fc8a6) to create virtual serial ports that connect to UDP endpoints has gone well and it works with [Node-Red](https://nodered.org) quite nicely.
