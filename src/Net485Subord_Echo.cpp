#include "Net485Subord_Echo.hpp"

Net485Subord_Echo::Net485Subord_Echo() {
    this->workQueue = new Net485WorkQueue(NULL);
    this->popNext = false;
}

Net485Subord_Echo::~Net485Subord_Echo() {
    delete this->workQueue;
}
void Net485Subord_Echo::doPop() {
    if(this->popNext) {
        this->workQueue->pop();
        this->popNext = false;
    }
}

