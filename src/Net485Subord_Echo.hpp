

#ifndef Net485Subord_Echo_hpp
#define Net485Subord_Echo_hpp

#define DEBUG

#include "Net485Subord.hpp"
#include "Net485WorkQueue.hpp"

class Net485Subord_Echo : public Net485Subord {
private:
    Net485WorkQueue *workQueue;
    bool popNext;
    void doPop();
public:
    Net485Subord_Echo();
    ~Net485Subord_Echo();
    
    void send(Net485Packet *packet) {
        Net485Packet sendPkt;
        uint8_t fromNodeId = packet->header()[HeaderStructureE::HeaderDestAddr];
        uint8_t toNodeId = packet->header()[HeaderStructureE::HeaderSrcAddr];
        uint8_t msgType = packet->header()[HeaderStructureE::PacketMsgType];
#ifdef DEBUG
    Serial.print(" virtualNode:"); Serial.print(fromNodeId);
    Serial.print(" msg:"); Serial.println(msgType);
#endif
        doPop();
        
        memcpy(&sendPkt,packet,sizeof(Net485Packet));
        sendPkt.header()[HeaderStructureE::HeaderDestAddr] = toNodeId;
        sendPkt.header()[HeaderStructureE::HeaderSrcAddr] = fromNodeId;
        
        switch(msgType) {
        case MSGTYP_R2R:
            sendPkt.data()[0] == R2R_ACK_CODE;
            break;
        //case MSGTYP_SNETLIST:
        default:    // Generic expect MSGRESP() and only check that is returned
            sendPkt.header()[HeaderStructureE::PacketMsgType] = MSGRESP(msgType);
            break;
        }
        this->workQueue->pushWork(sendPkt);
    };
    bool hasPacket(unsigned long *millisLastRead = NULL) {
        doPop();
        return this->workQueue->hasWork();
    };
    Net485Packet *getNextPacket(bool peek = false) {
        return this->workQueue->top();
    };

};

#endif

