/*
 *  Net485DataLink.cpp
 *  Net485
 *
 */
#include "Net485DataLink.hpp"

#define ACCUMULATE_FLETCHER(s1,s2,len,ptr) \
for(short i = 0; i < (len); i++) { \
s1 = (s1 + (ptr)[i]) % 0xff; \
s2 = (s2 + s1) % 0xff; \
}

Net485DataLink::Net485DataLink(HardwareSerial *_hwSerial
                   , uint8_t _srcNodeTyp
                   , uint16_t _mfgId
                   , uint64_t _deviceId
                   , int _baudRate
                   , int _drivePin
                   , uint8_t _ringbufSize )
: Net485Physical_HardwareSerial(_hwSerial, _ringbufSize, _baudRate, _drivePin) {
    unsigned long seed = this->getLoopCount();
    
    this->nodeType = _srcNodeTyp;
    if(_srcNodeTyp) seed += _srcNodeTyp;
    if(_mfgId) seed += (((unsigned long)_mfgId) << 16);
    // Set reserved byte to a non-zero number to indicate MAC is random generated
    this->macAddr.mac[Net485MacAddressE::Reserved] = 0x00;
    if(_deviceId == 0 || _mfgId == 0) {
        this->macAddr.setRandom(seed);
    }
}
Net485DataLink::~Net485DataLink() {
}
// Calculate checksum and set on packet before sending
void Net485DataLink::send(Net485Packet *packet) {
    calculateChecksum(packet);
    Net485Physical_HardwareSerial::send(packet);
}
// Get pointer to next packet in ring buffer
//
// Return pointer to next packet
Net485Packet *Net485DataLink::getNextPacket() {
    Net485Packet *retPkt = Net485Physical_HardwareSerial::getNextPacket();

    // Copy bytes received into structure location
    retPkt->dataSize = retPkt->header()[HeaderStructureE::PacketLength];
    
   return retPkt;
}
// Fletcher Checksum calculation
bool Net485DataLink::isChecksumValid(Net485Packet *packet) {
    unsigned char iSum1 = 0xAA; // New Fletcher Seed.
    unsigned char iSum2 = 0;

    // Accumulate data sent
    ACCUMULATE_FLETCHER(iSum1,iSum2,packet->header()[HeaderStructureE::PacketLength] + MTU_HEADER,packet->buffer)
    // Accumulate checksum sent should result in zero
    ACCUMULATE_FLETCHER(iSum1,iSum2,MTU_CHECKSUM,packet->checksum())
    
    return ((iSum1 == 0) && (iSum2 == 0) && packet->header()[HeaderStructureE::PacketLength] == packet->dataSize);
}
void Net485DataLink::calculateChecksum(Net485Packet *packet) {
    unsigned char iSum1 = 0xAA; // New Fletcher Seed.
    unsigned char iSum2 = 0;
    
    // copy header bytes to send to structure location
    packet->dataSize = packet->header()[HeaderStructureE::PacketLength];

    // Accumulate data to send
    ACCUMULATE_FLETCHER(iSum1,iSum2,packet->header()[HeaderStructureE::PacketLength] + MTU_HEADER,packet->buffer)
    // Set checksum bytes to send
    packet->checksum()[0] =0xff - (((iSum1 + iSum2))% 0xff);
    packet->checksum()[1] =0xff - (((iSum1 + packet->checksum()[0]))%0xff);
}



