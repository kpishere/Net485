

#ifndef Net485Subord_hpp
#define Net485Subord_hpp

#include <stdlib.h>
#include <Arduino.h>
#include "Net485API.hpp"

class Net485Subord {
private:
public:
    Net485Subord();
    ~Net485Subord();
    
    virtual void send(Net485Packet *packet) {};
    virtual bool hasPacket(unsigned long *millisLastRead = NULL) { return false;};
    virtual Net485Packet *getNextPacket(bool peek = false) { return NULL; };

};

#endif

