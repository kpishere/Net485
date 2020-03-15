# net485Listen

This command line utility simply listens and dumps any packets it can interpret on the serial line coming in.  It is intended to be connected to an RS485 network that is often used in HVAC on systems commonly referred to as 'communicating'. Each manufacturer these days calls their system something different but most are using the same fundamental network.

This utility will most likely dump the data from all of these networks but it won't necessarily interpret all of the bits.  It is a great tool down that path though!

Check out the WIKI for this repository, there is a reference rig there with some keywords for finding the parts needed to hook into your HVAC system and start monitoring it.

If Swift isn't your thing (this is written in Swift v4.2), there is a binary executable in the bin folder of this repository (it is signed).  You could try using that (it is compiled on HighSierra MacOS). [Sorry, I tried doing in Python .. just not my thing. The significance of indents is a huge blocker for me and feels like going backwards to FORTRAN!]
