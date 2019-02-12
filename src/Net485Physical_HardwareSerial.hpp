/*
 *  Net485Physical_HardwareSerial.h
 *
 */
#ifndef Net485Physical_HardwareSerial_hpp
#define Net485Physical_HardwareSerial_hpp

#include <stdlib.h>
#include <Arduino.h>

#if defined(__AVR__)
#define OUTPUT_DRIVE_ENABLE_PIN 2
#elif defined(ESP8266)
#define OUTPUT_DRIVE_ENABLE_PIN D6
#endif


#define BITS_START 1
#define BITS_STOP 1
#define BITS_DATA 8
#define DATARATE_BPS_DEF 9600UL
#define RINGBUFSIZE_DEF 5
#define BYTE_FRAME_TIME(bps) ( (1000UL * 1000UL / (bps)) * (BITS_DATA+BITS_START+BITS_STOP)) // useconds
#define INTER_PACKET_DELAY 100000UL // useconds before each packet
#define PRE_DRIVE_HOLD_TIME 300UL // useconds (200-500 allowable)
#define POST_DRIVE_HOLD_TIME 300UL // useconds (200-500 allowable)
#define MTU_HEADER 10UL
#define MTU_DATA 240UL
#define MTU_CHECKSUM 2UL
#define MTU_MAX (MTU_HEADER + MTU_DATA + MTU_CHECKSUM)
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

typedef struct Net485PacketS {
    uint8_t buffer[MTU_MAX];
    uint8_t dataSize;
    uint8_t* header() {
        return &buffer[0];
    }
    uint8_t* data() {
        return &buffer[MTU_HEADER];
    }
    uint8_t* checksum() {
        return &buffer[MTU_HEADER + dataSize];
    }
} Net485Packet;

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
    
    static void initTimer();
    static void setTimer(uint32_t usec, DriveState newState);
    static void clearTimer();
public:
    ////
    // These properties are public but are only used by clobal functions in module
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
    
    bool hasPacket();
    Net485Packet *getNextPacket();
    
    void setPacketFilter(); // TODO: to be defined
};

#endif /*Net485Physical_HardwareSerial_hpp*/
