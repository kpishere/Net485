/*
 *  Net485DataLink.h
 *  Net485
 *
 *  Created by Kevin D Peck on 17-09-24.
 *  Copyright 2017 Shea. All rights reserved.
 *
 */
#ifndef Net485DataLink_hpp
#define Net485DataLink_hpp

#include "Net485Physical_HardwareSerial.hpp"

typedef enum HeaderStructureE {
    HeaderDestAddr = 0x00,
    HeaderSrcAddr = 0x01,
    HeaderSubnet = 0x02,
    HeaderSndMethd = 0x03,
    HeaderSndParam = 0x04,
    HeaderSndParam1 = 0x05,
    HeaderSrcNodeType = 0x06,
    PacketMsgType = 0x07,
    PacketNumber = 0x08,
    PacketLength = 0x09
} HeaderStructure;

#define PARMBYTEHI(x) ((x)>>8)
#define PARMBYTELO(x) ((x)&0xff)

#define MAC_RESERVED 1
#define MAC_MFGID_BYTES 2
#define MAC_ADDR_DEVICEID_BYTES 5
#define MAC_SIZE (MAC_RESERVED+MAC_MFGID_BYTES+MAC_ADDR_DEVICEID_BYTES)
typedef struct Net485MacAddressS {
	uint8_t mac[MAC_SIZE];
    uint8_t *manufacturerId() { return &mac[MAC_RESERVED];};
    uint8_t *devideId() {return &mac[MAC_RESERVED+MAC_MFGID_BYTES];};
} Net485MacAddress;

class Net485DataLink : public Net485Physical_HardwareSerial {
private:
public:
    Net485DataLink(HardwareSerial *_hwSerial
                   , uint8_t _ringbufSize = RINGBUFSIZE_DEF
                   , int _baudRate = DATARATE_BPS_DEF
                   , int _drivePin = OUTPUT_DRIVE_ENABLE_PIN);
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
    
    // Calculate checksum and set on packet before sending
    void send(Net485Packet *packet);
    
    bool isChecksumValid(Net485Packet *packet);
};

#endif Net485DataLink_hpp

