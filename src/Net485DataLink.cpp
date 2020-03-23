/*
 *  Net485DataLink.cpp
 *  Net485
 *
 */
#define DEBUG

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
#ifdef DEBUG
    Serial.print("DataLink.init(): {macAddr:"); this->macAddr.display();
    Serial.print(" nodeType:"); Serial.print((unsigned char)this->nodeType,HEX);
    Serial.println("}");
#endif
}
Net485DataLink::~Net485DataLink() {
}
// Calculate checksum and set on packet before sending
void Net485DataLink::send(Net485Packet *packet) {
    unsigned char iSum1 = 0xAA; // New Fletcher Seed.
    unsigned char iSum2 = 0;
    
    // copy header bytes to send to structure location
    packet->dataSize = packet->header()[HeaderStructureE::PacketLength];

    // Accumulate data to send
    ACCUMULATE_FLETCHER(iSum1,iSum2,packet->header()[HeaderStructureE::PacketLength] + MTU_HEADER,packet->buffer)
    // Set checksum bytes to send
    packet->checksum()[0] =0xff - (((iSum1 + iSum2))% 0xff);
    packet->checksum()[1] =0xff - (((iSum1 + packet->checksum()[0]))%0xff);
    
#ifdef DEBUG
    {
        char messageBuffer[512]; // Temp for testing only
        
        sprintf(messageBuffer,"send {header: 0x");
        for(int i=0; i<MTU_HEADER ; i++)
            sprintf( &(messageBuffer[strlen("send {header: 0x")+i*3]), "%0X  ",packet->header()[i]);
        sprintf(&(messageBuffer[strlen("send {header: 0x")+3*MTU_HEADER]),", data: 0x");
        for(int i=0; i<packet->dataSize ; i++)
            sprintf( &(messageBuffer[strlen("send {header: 0x"", data: 0x")
                                     +3*(MTU_HEADER+i)]), "%0X  ",packet->data()[i]);
        sprintf(&(messageBuffer[strlen("send {header: 0x"", data: 0x")
                                +3*(MTU_HEADER+packet->dataSize)])
                ,", chksum: 0x%0X%0X, now:%lu}"
                , packet->checksum()[0], packet->checksum()[1], millis());
        Serial.println(messageBuffer);
    }
#endif
    Net485Physical_HardwareSerial::send(packet);
}
// Get pointer to next packet in ring buffer
//
// Return pointer to next packet
Net485Packet *Net485DataLink::getNextPacket() {
    Net485Packet *retPkt = Net485Physical_HardwareSerial::getNextPacket();

    // Copy bytes received into structure location
    retPkt->dataSize = retPkt->header()[HeaderStructureE::PacketLength];
#ifdef DEBUG
    {
        char messageBuffer[512]; // Temp for testing only
        
        sprintf(messageBuffer,"get {header: 0x");
        for(int i=0; i<MTU_HEADER ; i++)
            sprintf( &(messageBuffer[strlen("get {header: 0x")+i*3]), "%0X  "
                    ,retPkt->header()[i]);
        sprintf(&(messageBuffer[strlen("get {header: 0x")+3*MTU_HEADER]),", data: 0x");
        for(int i=0; i<retPkt->dataSize ; i++)
            sprintf( &(messageBuffer[strlen("get {header: 0x"", data: 0x")
                                     +3*(MTU_HEADER+i)]), "%0X  ",retPkt->data()[i]);
        sprintf(&(messageBuffer[strlen("get {header: 0x"", data: 0x")
                                +3*(MTU_HEADER+retPkt->dataSize)])
                ,", chksum: 0x%0X%0X, now: %lu}"
                , retPkt->checksum()[0], retPkt->checksum()[1], millis());
        Serial.println(messageBuffer);
    }
#endif
   return retPkt;
}
// Base class checks for packet, we apply CRC check and ignore if not valid
//
// Returns: true for CRC valid packet, false if no packet or not valid
bool Net485DataLink::hasPacket() {
    bool retVal = Net485Physical_HardwareSerial::hasPacket();
    if(retVal) {
#ifdef DEBUG
    Serial.print("Net485DataLink::hasPacket(): yes isValid:");
#endif
        Net485Packet *pkt = Net485Physical_HardwareSerial::getNextPacket(true);
        retVal = this->isChecksumValid(pkt); // If not CRC valid, will return false and ignore packat
#ifdef DEBUG
    Serial.println(retVal);
#endif
        if(!retVal) Net485Physical_HardwareSerial::getNextPacket(); // Read bad packet and ignore
    }
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


