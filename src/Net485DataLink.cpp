/*
 *  Net485DataLink.cpp
 *  Net485
 *
 *  Created by Kevin D Peck on 17-09-24.
 *  Copyright 2017 Shea. All rights reserved.
 *
 */

#include "Net485DataLink.hpp"

#define ACCUMULATE_FLETCHER(s1,s2,len,ptr) \
for(short i = 0; i < (len); i++) { \
s1 = (s1 + (ptr)[i]) % 0xff; \
s2 = (s2 + s1) % 0xff; \
}

Net485DataLink::Net485DataLink() {
}
~Net485DataLink::Net485DataLink() {
}

// for changing default
void Net485DataLink::setBaudRate(int baudRate) {
}

// Calculate checksum and set on packet before sending
void Net485DataLink::send(Net485Packet *packet, bool wait = false) {
    unsigned char iSum1 = 0xAA; // New Fletcher Seed.
    unsigned char iSum2 = 0;

    // Accumulate data to send
    ACCUMULATE_FLETCHER(iSum1,iSum2,packet.header()[HeaderStructureE::PacketLength] + MTU_HEADER,packet.buffer)
    // Set checksum bytes to send
    packet.checksum()[0] =0xff - (((iSum1 + iSum2))% 0xff;
    packet.checksum()[1] =0xff - (((iSum1 + packet.checksum()[0]))%0xff;
}

// Fletcher Checksum calculation
bool Net485DataLink::isChecksumValid(Net485Packet *packet) {
    unsigned char iSum1 = 0xAA; // New Fletcher Seed.
    unsigned char iSum2 = 0;

    // Accumulate data sent
    ACCUMULATE_FLETCHER(iSum1,iSum2,packet.header()[HeaderStructureE::PacketLength] + MTU_HEADER,packet.buffer)
    // Accumulate checksum sent should result in zero
    ACCUMULATE_FLETCHER(iSum1,iSum2,MTU_CHECKSUM,packet.checksum())
    
    return ((iSum1 == 0) && (iSum2 == 0));
}


