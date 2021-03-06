/*
 *  Net485Physical_HardwareSerial.cpp
 *
 */

#include "Net485Physical_HardwareSerial.hpp"

#if defined(__AVR__)
    // TICKS PER us for MEGA 2560
    #define IR_SEND_ADJ 0.0625 //0.1300
#elif defined(ESP8266)
    // Ticks per us for ESP8266
    #define IR_SEND_ADJ 5.008
#elif __APPLE__
    #include <dispatch/dispatch.h>

    static dispatch_queue_t timeEvents;
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
    #error "Unknown compiler"
#endif


#define SERIALBUFFERSIZE 256

#define RINGBUFLOC(loc) (&ringbuf[((((uint32_t)loc) % ringbufSize) * sizeof(Net485Packet))])

/*
 * NOTE: Only one instance of this class supported on these devices !!
 */
#if defined(__AVR__)
ISR(TIMER1_COMPA_vect){
    if(Net485Physical_HardwareSerial::lastInstance)
        Net485Physical_HardwareSerial::lastInstance->packetSequencer();
}
#elif defined(ESP8266)
void ISRHandlerTimer() {
    if(Net485Physical_HardwareSerial::lastInstance)
        Net485Physical_HardwareSerial::lastInstance->packetSequencer();
}
#endif

#if defined(__AVR__) || defined(ESP8266)
#endif /* defined(__AVR__) || defined(ESP8266) */

Net485Physical_HardwareSerial *Net485Physical_HardwareSerial::lastInstance;

Net485Packet *Net485Physical_HardwareSerial::ringbuf;
uint8_t Net485Physical_HardwareSerial::ringbufSize;
uint8_t Net485Physical_HardwareSerial::ringbufPktCount;
uint8_t Net485Physical_HardwareSerial::ringbufPktCurrent;
Net485Packet *Net485Physical_HardwareSerial::packetToSend;
DriveState Net485Physical_HardwareSerial::driveState;
int Net485Physical_HardwareSerial::baudRate;
int Net485Physical_HardwareSerial::drivePin;
HardwareSerial *Net485Physical_HardwareSerial::hwSerial;
unsigned long Net485Physical_HardwareSerial::loopCount;
uint8_t *Net485Physical_HardwareSerial::msgTypeFilterList;

void Net485Physical_HardwareSerial::initTimer() {
#if defined(__AVR__)
    // Set up timer counter
    TCCR1A = 0;// set entire TCCR1A register to 0
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0
    // Set next timer value
    OCR1A = 0xFFFE;
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12 bit for 256 prescaler (0.000,016s or 4 micro-second)
    TCCR1B |= (1 << CS12);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
#elif defined(ESP8266)
    timer1_isr_init();
    //Initialize Ticker every 5 ticks/us - 1677721.4 us max
    timer1_attachInterrupt(ISRHandlerTimer);
    // Set first comparitor value to trigger in short time & enable interrupt
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
#elif __APPLE__
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        timeEvents = dispatch_queue_create("Net485Physical_HardwareSerial", DISPATCH_QUEUE_CONCURRENT);
    });
#endif
}
void Net485Physical_HardwareSerial::setTimer(uint32_t usec, DriveState newState) {
    cli();//stop interrupts
    driveState = newState;
#if defined(__AVR__)
    // Set next timer value
    OCR1A =  (uint16_t)((uint16_t)(usec * IR_SEND_ADJ) + 0.5);
#elif defined(ESP8266)
    // Set next timer value
    timer1_write((uint16_t)((uint16_t)(usec * IR_SEND_ADJ) + 0.5));
#elif __APPLE__
    dispatch_after( dispatch_time(DISPATCH_TIME_NOW, usec * NSEC_PER_USEC), timeEvents, ^(void){
        this->packetSequencer();
    });
#endif
    sei();//allow interrupts
}
void Net485Physical_HardwareSerial::clearTimer() {
    // disable timer compare interrupt
#if defined(__AVR__)
    TIMSK1 &= ~(1 << OCIE1A);
#elif defined(ESP8266)
    timer1_disable();
#endif
}
bool valueInVector(uint8_t val, uint8_t *ptrNullTerm) {
    bool found = false;
    for( int i = 0; ptrNullTerm[i] != 0x00 && !found; i++) {
        found = found || ptrNullTerm[i] == val;
    }
    return found;
}

void Net485Physical_HardwareSerial::packetSequencer() {
    switch(driveState) {
        // Writing packet sequence
        case DriveStateE::InterPktDelay:
            initTimer();
            setTimer(INTER_PACKET_DELAY, DriveStateE::PreDrive);
            break;
        case DriveStateE::PreDrive:
            // Set RS485 for writing
            digitalWrite(drivePin,HIGH);
            setTimer(PRE_DRIVE_HOLD_TIME, DriveStateE::Sending);
            break;
        case DriveStateE::Sending:
            setTimer(MTU_PACKET_TIME(baudRate,packetToSend->dataSize), DriveStateE::PostDrive);
            hwSerial->write(packetToSend->buffer,MTU_HEADER + packetToSend->dataSize + MTU_CHECKSUM);
            break;
        case DriveStateE::PostDrive:
            // Hold RS485 for writing
            setTimer(POST_DRIVE_HOLD_TIME, DriveStateE::COUNT_STATES);
            break;
        case DriveStateE::COUNT_STATES:
            digitalWrite(drivePin,LOW);
            clearTimer();
            driveState = DriveStateE::Ready;
            break;
        // Reading Packet sequence
        case DriveStateE::ReadingPacket:
            initTimer();
            setTimer(DELIMIT_MEASURE_PACKET, DriveStateE::PacketReady);
            break;
        case DriveStateE::PacketReady:
        {
            // Get packet just written and update dataSize for data bytes only
            void *buffer = RINGBUFLOC(ringbufPktCurrent+1);

            // Packet ignored if less than minimum expected size
            if( ((Net485Packet *)buffer)->dataSize >= (MTU_HEADER + MTU_CHECKSUM)) {
                ((Net485Packet *)buffer)->dataSize -= (MTU_HEADER + MTU_CHECKSUM);
                // roll pointer forward
                ringbufPktCurrent += 1;
                // clear dataSize of next packet location
                buffer = RINGBUFLOC(ringbufPktCurrent+1);
                ((Net485Packet *)buffer)->dataSize = 0;
                // increment count of buffers ready for reading
                ringbufPktCount += 1;
            }
            clearTimer();
            driveState = DriveStateE::Ready;
            break;
        }
        case Ready: break;
    }
}
void Net485Physical_HardwareSerial::readData() {
    char *buffer;
    const int bytes = hwSerial->available();
    if(bytes > 0) {
        buffer = (char *)RINGBUFLOC(ringbufPktCurrent+1);

        hwSerial->readBytes((uint8_t *)(&buffer[((Net485Packet *)buffer)->dataSize]), bytes);

        ((Net485Packet *)buffer)->dataSize += bytes;
        driveState = DriveStateE::ReadingPacket;
        Net485Physical_HardwareSerial::lastInstance->packetSequencer();
    }
}
Net485Physical_HardwareSerial::Net485Physical_HardwareSerial(HardwareSerial *_hwSerial
                                                             , uint8_t _ringbufSize
                                                             , int _baudRate
                                                             , int _drivePin)
{
    ringbufSize = _ringbufSize;
    ringbuf = (Net485Packet *)malloc(sizeof(Net485Packet) * _ringbufSize);
    memset(ringbuf, 0x00, sizeof(Net485Packet) * _ringbufSize);
    ringbufPktCount = 0;
    ringbufPktCurrent = 0;
    loopCount = 0;
    hwSerial = _hwSerial;
    baudRate = _baudRate;
    drivePin = _drivePin;
    msgTypeFilterList = NULL;
    
    // Set drive enable pin low on RS485 device to set listen mode
    pinMode(drivePin,OUTPUT);
    digitalWrite(drivePin,LOW);
    
    hwSerial->setTimeout( DELIMIT_MEASURE_PACKET / 1000 );
#if defined(__AVR__)
#elif defined(ESP8266)
    hwSerial->setRxBufferSize(SERIALBUFFERSIZE);
    hwSerial->setDebugOutput(false);
#endif
    
    lastInstance = this;
    driveState = DriveStateE::Ready;
}
Net485Physical_HardwareSerial::~Net485Physical_HardwareSerial() {
    if(ringbuf) free((void *)ringbuf);
    lastInstance = 0;
}
bool Net485Physical_HardwareSerial::readyToSend() {
    return (driveState == DriveStateE::Ready);
}
void Net485Physical_HardwareSerial::send(Net485Packet *packet) {
    packetToSend = packet;
    driveState = DriveStateE::InterPktDelay;
    Net485Physical_HardwareSerial::lastInstance->packetSequencer();
}
// Check for any packets newly added in ring buffer.  Optionately update a pointer to
// current milisecond time -- usefull for resetting timeout after a packet is filtered out
// If packet received and filtered out, packet is read to effectively ignore it.
//
// Returns true if there is packet to read, false if no packets ready or packet is filtered
bool Net485Physical_HardwareSerial::hasPacket(unsigned long *millisLastRead) {
    loopCount++;
    if((ringbufPktCount > 0)) {
        if(millisLastRead != NULL) millisLastRead[0] = millis();
        Net485Packet *peekPkt = this->getNextPacket(true);
        if(
            (msgTypeFilterList == NULL || valueInVector(peekPkt->header()[HeaderStructureE::PacketMsgType], msgTypeFilterList) )
        ) {
            return true;
        } else {
            // Read this packet and advance buffer to ignore it
            this->getNextPacket();
        }
    } else {
        this->readData();
    }
    return false;
}
// Return pointer to next packet in ring buffer and optionally decrement unread packet count
// peek: false (default) - will adjust pointer for next packet after accessing this pointer,
//       true - allows read before consuming packet
//
// Returns: pointer to next packet
Net485Packet *Net485Physical_HardwareSerial::getNextPacket(bool peek) {
    Net485Packet *retPkt = (Net485Packet *)RINGBUFLOC(ringbufPktCurrent - ringbufPktCount + 1);
    if(!peek) ringbufPktCount -= 1;
    return retPkt;
}
// Update filter on packets to ignore
//
//
//
void Net485Physical_HardwareSerial::setPacketFilter(uint8_t *destAddr,
                                                    uint8_t *subNet,
                                                    uint8_t *srcNodeTyp,
                                                    const uint8_t *pktMsgTyp) {
    msgTypeFilterList = (uint8_t *)pktMsgTyp;
}
