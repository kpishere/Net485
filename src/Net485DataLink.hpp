/*
 *  Net485DataLink.h
 *  Net485
 *
 */
#ifndef Net485DataLink_hpp
#define Net485DataLink_hpp

#include "Net485Physical_HardwareSerial.hpp"
#include "Net485API.hpp"


class Net485DataLink : public Net485Physical_HardwareSerial {
private:
    Net485MacAddress macAddr;
    uint8_t nodeType;
public:
    Net485DataLink(HardwareSerial *_hwSerial
                   , uint8_t _ringbufSize = RINGBUFSIZE_DEF
                   , int _baudRate = DATARATE_BPS_DEF
                   , int _drivePin = OUTPUT_DRIVE_ENABLE_PIN
                   , uint16_t _mfgId = 0
                   , uint64_t _deviceId = 0
                   , uint8_t _srcNodeTyp = 0
                   );
    ~Net485DataLink();
    
    inline Net485Packet *setPacketHeader(Net485Packet *packet,
                             uint8_t destAddr,
                             uint8_t srcAddr,
                             uint8_t subNet,
                             uint8_t sendMthd,
                             uint16_t sendParm,
                             uint8_t srcNodeTyp,
                             uint8_t pktMsgTyp,
                             uint8_t pktNum,
                             uint8_t pktLen
                                    ) {
        packet->header()[HeaderStructureE::HeaderDestAddr] = destAddr;
        packet->header()[HeaderStructureE::HeaderSrcAddr] = srcAddr;
        packet->header()[HeaderStructureE::HeaderSubnet] = subNet;
        packet->header()[HeaderStructureE::HeaderSndMethd] = sendMthd;
        packet->header()[HeaderStructureE::HeaderSndParam] = PARMBYTEHI(sendParm);
        packet->header()[HeaderStructureE::HeaderSndParam1] = PARMBYTELO(sendParm);
        packet->header()[HeaderStructureE::HeaderSrcNodeType] = srcNodeTyp;
        packet->header()[HeaderStructureE::PacketMsgType] = pktMsgTyp;
        packet->header()[HeaderStructureE::PacketNumber] = pktNum;
        packet->header()[HeaderStructureE::PacketLength] = pktLen;
        return packet;
    }
    
    inline uint64_t newSessionId() {
        return random(0x01,0xFFFFFFFFffffffff);
    }
#define SLOT_LOW 100
#define SLOT_HIGH 2500
    inline uint16_t newSlotDelayMicroSecs(unsigned int low = SLOT_LOW, unsigned int hi = SLOT_HIGH) {
        unsigned long seed = this->getLoopCount() + this->nodeType + this->macAddr.manufacturerId() + analogRead(0);
        randomSeed(seed);
        return random(SLOT_LOW,SLOT_HIGH) * 1000;
    }
    inline uint8_t getNodeType() { return this->nodeType;};
    inline Net485MacAddress getMacAddr() {return this->macAddr;};
    
    // Calculate checksum and set on packet before sending
    void send(Net485Packet *packet);
    Net485Packet *getNextPacket();

    bool isChecksumValid(Net485Packet *packet);
};

#endif Net485DataLink_hpp

