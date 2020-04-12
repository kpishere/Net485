
#include "Net485WorkQueue.hpp"
#include "Net485Network.hpp"

Net485WorkQueue::Net485WorkQueue(Net485Network *net,int maxitems) {
    this->_net = net;
    this->_next = 0;
    this->_maxitems = maxitems;
    this->_data = new Net485Packet[maxitems + 1];
}
Net485WorkQueue::~Net485WorkQueue() {
    delete[] _data;
}
int Net485WorkQueue::pushWork(Net485Packet &pkt) {
    if(this->_next < this->_maxitems) { // Drops out when full
        this->_data[this->_next++]=pkt;
        // Check wrap around
        if (this->_next > this->_maxitems) this->_next -= (this->_maxitems + 1);
    }
#ifdef DEBUG
    Serial.print("pushWork() ");
    Serial.print(this->_next);
    Serial.print(" ");
    this->top()->display();
#endif
    return this->_next;
}
Net485Packet* Net485WorkQueue::top() {
    if(this->_next <= 0) return NULL; // Returns empty
    else return &(this->_data[this->_next-1]);
}
void Net485WorkQueue::pop() {
    if(this->_next > 0) {
        --this->_next;
    }
}
int Net485WorkQueue::doWork(int forNitems) {
    bool transComplete;
    
    while(forNitems > 0 && this->_next>0 ) {
        transComplete = false;
#ifdef DEBUG
        Serial.print("doWork() ");
        Serial.print(forNitems);
        Serial.print(" ");
        Serial.print(this->_next);
        Serial.print(",");
#endif
        // Execute as many steps as are required to complete routing
        if(this->_net != NULL) {
            while( !transComplete ) {
#ifdef DEBUG
                this->top()->display();
#endif
                transComplete = this->_net->routePacket( this->top() );
            }
        }
        this->pop();
        forNitems--;
    }
    return this->_next;
}
