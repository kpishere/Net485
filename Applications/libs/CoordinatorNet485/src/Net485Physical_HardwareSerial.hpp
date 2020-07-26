/*
 *  Net485Physical_HardwareSerial.h
 *
 */
#ifndef Net485Physical_HardwareSerial_hpp
#define Net485Physical_HardwareSerial_hpp

#include <stdlib.h>
#include "PhysicalDevice.hpp"

#include "Net485API.hpp"

#define BITS_START 1
#define BITS_STOP 1
#define BITS_DATA 8
#define DATARATE_BPS_DEF 9600UL
#define RINGBUFSIZE_DEF 5
#define BYTE_FRAME_TIME(bps) ( (1000UL * 1000UL / (bps)) * (BITS_DATA+BITS_START+BITS_STOP)) // useconds
#define INTER_PACKET_DELAY 100000UL // useconds before each packet
#define PRE_DRIVE_HOLD_TIME 300UL // useconds (200-500 allowable)
#define POST_DRIVE_HOLD_TIME 300UL // useconds (200-500 allowable)
#define MTU_PACKET_TIME(bps,bytes) ( BYTE_FRAME_TIME(bps) * ((bytes)+MTU_HEADER+MTU_CHECKSUM)) // useconds
#define DELIMIT_MEASURE_PACKET 3500UL //useconds inter-character delay longer than this time, is end of packet

typedef enum DriveStateE {
    InterPktDelay,
    PreDrive,
    Sending,
    PostDrive,
    COUNT_STATES,
    ReadingPacket,
    PacketReady,
    Ready
} DriveState;

class Net485Physical_HardwareSerial {
private:
    static Net485Packet *ringbuf;
    static uint8_t ringbufSize;
    static uint8_t ringbufPktCount;
    static uint8_t ringbufPktCurrent;
    static Net485Packet *packetToSend;
    static DriveState driveState;
    static int baudRate, drivePin;
    static HardwareSerial *hwSerial;
    static unsigned long loopCount;
    
    static void initTimer();
    static void clearTimer();
    void setTimer(uint32_t usec, DriveState newState);

    static uint8_t *msgTypeFilterList;
public:
    ////
    // These properties are public but are only used by global functions in module
    ////
    static Net485Physical_HardwareSerial *lastInstance;
    void packetSequencer();
    void readData();

    ////
    // Interface members
    ////
    Net485Physical_HardwareSerial(HardwareSerial *_hwSerial
                                  , uint8_t _ringbufSize = RINGBUFSIZE_DEF
                                  , int _baudRate = DATARATE_BPS_DEF
                                  , int _drivePin = OUTPUT_DRIVE_ENABLE_PIN);
    ~Net485Physical_HardwareSerial();
    
    bool readyToSend();
    void send(Net485Packet *packet);
    
    virtual bool hasPacket(unsigned long *millisLastRead = NULL);
    Net485Packet *getNextPacket(bool peek = false);
    
    inline unsigned long getLoopCount() { return loopCount;}
    
    // A null-terminated list of values to filter by
    void setPacketFilter(uint8_t *destAddr,
                         uint8_t *subNet,
                         uint8_t *srcNodeTyp,
                         const uint8_t *pktMsgTyp);
};

#endif /*Net485Physical_HardwareSerial_hpp*/
