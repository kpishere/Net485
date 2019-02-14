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

#define NODEADDR_BCAST 0x00
#define NODEADDR_COORD 0xFF

#define SUBNET_BCAST 0x00
#define SUBNET_MAINT 0x01
#define SUBNET_V1SPEC 0x02
#define SUBNET_V2SPEC 0x03

#define NTC_THERM       0x01 /*Thermostat*/
#define NTC_FURNGAS     0x02 /*Gas Furnace*/
#define NTC_AHNDLR      0x03 /*Air Handler*/
#define NTC_ACOND       0x04 /*Air Conditioner*/
#define NTC_HPUMP       0x05 /*Heat Pump*/
#define NTC_FURNELEC    0x06 /*Electric Furnace*/
#define NTC_PKGGAS      0x07 /*Package System - Gas*/
#define NTC_PKGELEC     0x08 /*Package System - Electric*/
#define NTC_XOVER       0x09 /*Crossover (aka OBBI)*/
#define NTC_COMPSEC     0x0A /*Secondary Compressor*/
#define NTC_AIREXCH     0x0B /*Air Exchanger*/
#define NTC_UNICTL      0x0C /*Unitary Control*/
#define NTC_DEHUM       0x0D /*Dehumidifier*/
#define NTC_ACLEANE     0x0E /*Electronic Air Cleaner*/
#define NTC_ERV         0x0F /*ERV*/
#define NTC_HUMEV       0x10 /*Humidifier (Evaporative)*/
#define NTC_HUMSTM      0x11 /*Humidifier (Steam)*/
#define NTC_HRV         0x12 /*HRV*/
#define NTC_IAQANZR     0x13 /*IAQ Analyzer*/
#define NTC_MEDACLN     0x14 /*Media Air Cleaner*/
#define NTC_ZCTRL       0x15 /*Zone Control*/
#define NTC_ZUI         0x16 /*Zone User Interface*/
#define NTC_BOILER      0x17 /*Boiler*/
#define NTC_WHGAS       0x18  /*Water Heater – Gas*/
#define NTC_WHELEC      0x19 /*Water Heater – Electric*/
#define NTC_WHCMTL      0x1A /*Water Heater - Commercial*/
#define NTC_POOLH       0x1B /*Pool Heater*/
#define NTC_CFAN        0x1C /*Ceiling Fan*/
#define NTC_GW          0x1D /*Gateway*/
#define NTC_DIAG        0x1E /*Diagnostic Device*/
#define NTC_LTCTRL      0x1F /*Lighting Control*/
#define NTC_SCURT       0x20 /*Security System*/
#define NTC_UVLT        0x21 /*UV Light*/
#define NTC_WDSV        0x22 /*Weather Data Device*/
#define NTC_WHSEFAN     0x23 /*Whole House Fan*/
#define NTC_SLRINV      0x24 /*Solar Inverter*/
#define NTC_ZDAMP       0x25 /*Zone Damper*/
#define NTC_ZTC         0x26 /*Zone Temperature Control (ZTC)*/
#define NTC_TSENSE      0x27 /*Temperature Sensor*/
#define NTC_OCC         0x28 /*Occupancy Sensor*/
#define NTC_NETCTRL     0xA6 /*Network Coordinator*/

enum HeaderStructureE {
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
};

#define PARMBYTEHI(x) ((x)>>8)
#define PARMBYTELO(x) ((x)&0xff)

enum Net485MacAddressE {
    Reserved = 0x00, MfgId = 0x01, DeviceId = 0x03, SIZE = 8
};

typedef struct Net485MacAddressS {
    uint8_t mac[Net485MacAddressE::SIZE];
    uint16_t manufacturerId(uint16_t mfgId=0) {
        if(mfgId) {
            memcpy(&mac[Net485MacAddressE::MfgId], &mfgId, sizeof(uint16_t));
        } else {
            memcpy(&mfgId, &mac[Net485MacAddressE::MfgId], sizeof(uint16_t));
        }
        return mfgId;
    };
    uint64_t deviceId(uint64_t devId) { /* only lower N bytes copied! */
        if(devId) {
            memcpy(&mac[Net485MacAddressE::DeviceId], (((char *)&devId)+Net485MacAddressE::DeviceId), Net485MacAddressE::SIZE - Net485MacAddressE::DeviceId);
        } else {
            devId = 0;
            memcpy((((char *)&devId)+Net485MacAddressE::DeviceId), &mac[Net485MacAddressE::DeviceId], Net485MacAddressE::SIZE - Net485MacAddressE::DeviceId);
        }
        return devId;
    }
} Net485MacAddress;

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
    
    
    
    // Calculate checksum and set on packet before sending
    void send(Net485Packet *packet);
    
    bool isChecksumValid(Net485Packet *packet);
};

#endif Net485DataLink_hpp

