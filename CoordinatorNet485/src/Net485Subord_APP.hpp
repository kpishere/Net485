

#ifndef Net485Subord_APP_hpp
#define Net485Subord_APP_hpp

#include "Net485Subord.hpp"

#define WAITTIME_ONREAD 100 /* ms */

class Net485Subord_APP : public Net485Subord {
private:
    HardwareSerial *hwSerial;
    Net485Packet incomingPkt;
    bool incomingPktWaiting;
    
    bool readPkt(int _waitMs = WAITTIME_ONREAD);
public:
    Net485Subord_APP(HardwareSerial *_hwSerial);
    ~Net485Subord_APP();
    
    void send(Net485Packet *packet);
    bool hasPacket(unsigned long *millisLastRead = NULL);
    Net485Packet *getNextPacket(bool peek = false);
};

#endif

