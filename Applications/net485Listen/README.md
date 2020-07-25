# net485Listen

This command line utility simply listens and dumps any packets it can interpret on the serial line coming in.  It is intended to be connected to an RS485 network that is often used in HVAC on systems commonly referred to as 'communicating'. Each manufacturer these days calls their system something different but most are using the same fundamental network.

This utility will most likely dump the data from all of these networks but it won't necessarily interpret all of the bits.  It is a great tool down that path though!

Check out the WIKI for this repository, there is a reference rig there with some keywords for finding the parts needed to hook into your HVAC system and start monitoring it.

If Swift isn't your thing (this is written in Swift v4.2), there is a binary executable in the bin folder of this repository (it is signed).  You could try using that (it is compiled on HighSierra MacOS). [Sorry, I tried doing in Python .. just not my thing. The significance of indents is a huge blocker for me and feels like going backwards to FORTRAN!]

If you do get into this, you'll notice the 'Dataflow message types' vs. the 'Application message types'.  The difference is that the coordinator (or master) and the slave(s) exchange 'Dataflow message types' just to manage who is allowed to talk on this half-duplex network. These types of messages also support other features of the network (multiple routing methods for messages, self healing, shared network data, for instance.).  The actual purposeful messages that are shared between devices are the 'Application messages types'.  So, you'd often want to ignore the Dataflow messages -- unless you are debugging or wanting to understand how the network works!

## Working DEV notes 

The project is moving away from physical hardware devices for a while as development gets more complex with the need for simulated device networks.  In that vein, it was discovered that a new serial port framework was needed [POSIXSerialPort`](https://github.com/kpishere/POSIXSerialPort) as the old one does not connect to TTY virtual devices!  

With the use of `socat` we can easily create virtual TTY(serial) devices, map them to UDP ports and connect those to Node-Red where the network is visually wired and scenarios can be simulated etc.  It should pick up the pace of the project. The early days of working with the hardware were slow but still usefull for a solid and well understood foundation.

```
# Set up a psudo tty port that connects to two uni-directional UDP ports for interfacing with Node-Red
sudo socat -x -d -d -d -d UDP-RECV:2012\!\!UDP-SEND:localhost:2010 pty,raw,ispeed=115200,link=/dev/furnace
```

