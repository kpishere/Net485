# Macos (Darwin/Mach) POSIXSerialPort

This Serial port framwork is Objective-C and Swift friendly (tested on V4.2) in the same way that ORSSerialPort is commonly used (see https://github.com/armadsen/ORSSerialPort).  Of note is this other project, never used it but it also follows the same vein of serial device, I include it becasue it does not come up in searches much but looks and may even be the same -- it is 100% swift implementation -- https://github.com/MichMich/nRF8001-Swift

Some key differences are :  

1) This library works equally well with CU and TTY devices for both virtual or non-virtual devices
2) It does not depend upon IOKit (which was causing #1 issue above for ORSSerialPort)
3) There is no pre-conceived notion of what defines a packet (ORSSerialPort is limited in this way in that it requires a delimiter to define a packet)

As it currently is, it is an extreamly simple library, or rather, it is very small.

Review the Tests to get a sense of how the library works.  You essentially, define the port, define a delegate that receives callbacks for port open, close, and new data messages.  Of note in this pattern is the callback 'serialPort:nextDataSegmentValidIn:' -- this is called whenever data is received, each time it is called, the delegate reviews the accumulated buffer and if it finds a valid packet by some defition, the return structure returns {offset, length} values that will indicate what portion of data makes a valid packet.  Where {0.0} is returned, the data will be retained and accumulated with the next receipt of data.  Only when a valid packet exists, is 'serialPort:didReceiveData:' called.

## Unit Test Requirements

The unit tests use socat to create virtual serial ports.  Homebrew is a good source for this tool in Macos.  Also, you'll have to change access to your /dev folder with 

sudo chmod o+rw /dev

to allow socat to create devices and XCode tests to read/write to them.
