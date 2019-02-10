/*
 *  Net485DataLink.h
 *  Net485
 *
 *  Created by Kevin D Peck on 17-09-24.
 *  Copyright 2017 Shea. All rights reserved.
 *
 */
#include "Net485Physical_HardwareSerial.hpp"

typedef enum HeaderStructureE {
    HeaderDestAddr = 0x00, HeaderSrcAddr = 0x01,    HeaderSubnet = 0x02,
    HeaderSndMethd = 0x03, HeaderSndParam = 0x04,   HeaderSrcNodeType = 0x06,
    PacketMsgType = 0x07,  PacketNumber = 0x08,     PacketLength = 0x09
} HeaderStructure;
#define PACKET_SIZE(p) ((p)->header()[)

#define MAC_RESERVED 1
#define MAC_MFGID_BYTES 2
#define MAC_ADDR_DEVICEID_BYTES 5
#define MAC_SIZE (MAC_RESERVED+MAC_MFGID_BYTES+MAC_ADDR_DEVICEID_BYTES)
typerdef struct Net485MacAddressS {
	uint8_t mac[MAC_SIZE];
    uint8_t *manufacturerId() { return &mac[MAC_RESERVED];};
    uint8_t *devideId() {return &mac[MAC_RESERVED+MAC_MFGID_BYTES]};
} Net485MacAddress;

class Net485DataLink extends Net485Physical_HardwareSerial {
private:
public:
    Net485DataLink();
    ~Net485DataLink();
    
    // for changing default
    void setBaudRate(int baudRate);
    
    // Calculate checksum and set on packet before sending
    void send(Net485Packet *packet, bool wait = false);
    
    bool isChecksumValid(Net485Packet *packet);
}



