

#ifndef Net485Subord_UART_hpp
#define Net485Subord_UART_hpp

#include "Net485Subord.hpp"

#define WAITTIME_ONREAD 100 /* ms */

class Net485Subord_UART : public Net485Subord {
private:
    HardwareSerial *hwSerial;
    Net485Packet incomingPkt;
    bool incomingPktWaiting;
    
    bool readPkt(int _waitMs = WAITTIME_ONREAD);
public:
    Net485Subord_UART(HardwareSerial *_hwSerial);
    ~Net485Subord_UART();
    
    void send(Net485Packet *packet);
    bool hasPacket(unsigned long *millisLastRead = NULL);
    Net485Packet *getNextPacket(bool peek = false);
};

#endif

