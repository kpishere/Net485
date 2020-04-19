
#ifndef Net485WorkQueue_hpp
#define Net485WorkQueue_hpp

#define DEBUG

#include "Net485DataLink.hpp"
#include "Net485API.hpp"

class Net485Network;

#define DEFAULT_MAXITEMS 4

class Net485WorkQueue {
private:
    Net485Packet *_data;
    int _maxitems;

    Net485Network *_net;

public:
    int _next;

    Net485WorkQueue(Net485Network *net, int maxitems = DEFAULT_MAXITEMS);
    ~Net485WorkQueue();
    
    // Add work to stack by copying packet
    //
    // Returns # of items in stack after push
    int pushWork(Net485Packet &pkt);
    
    // Checks if any pending work to perform
    //
    // Returns true if work, false otherwise
    bool hasWork() { return ((this->_next) > 0); }
    
    // Performs work of next N packet(s) before returning.
    //
    // returns items in stack after work is performed
    int doWork(int forNitems = 1);

    Net485Packet* top();
    void pop();
};


#endif Net485WorkQueue_hpp
