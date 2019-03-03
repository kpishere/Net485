/*
 *  Net485Network.cpp
 *  Net485
 */

#include "Net485Network.hpp"

#define DEBUG
uint8_t lastState = 0xff;

Net485Network::Net485Network(Net485DataLink *_net, Net485Subord *_sub, bool _coordinatorCapable
                             , uint16_t _coordVer = 0, uint16_t _coordRev = 0) {
    this->net485dl = _net;
    this->sessionId = this->net485dl->newSessionId();
    this->ver.Version = _coordVer;
    this->ver.Revision = _coordRev;
    this->ver.isFFD = _coordinatorCapable;
    this->lasttimeOfMessage = 0;
    this->lastNodeListPoll = 0;
    this->slotTime = 0;
    this->state = Net485State::None;
    this->sub = _sub;
    for(int i=0; i<MTU_DATA; i++) {
        this->netNodeList[i] = 0x00;
        this->nodes[i] = NULL;
    }
    this->netNodeListCount = 0;
}

uint8_t Net485Network::nodeExists(Net485Node *_node) {
    for(int i = NODEADDR_PRIMY; i< NODEADDR_NARB; i++) {
        if(this->netNodeList[i] > 0) {
            if(_node->isSameAs(this->nodes[i])) return i;
        }
    }
    return 0;
}


#define ANET_SLOTLO 6000
#define ANET_SLOTHI 30000
const uint8_t nodeTypeArbFilterList[] = {MSGTYP_ANUCVER,MSGTYP_NDSCVRY, NULL};

void Net485Network::loopClient() {
    Net485Packet *recvPtr, sendPkt;
    bool havePkt;
    Net485Node *discNode;
    char messageBuffer[160]; // Temp for testing only
    // TODO
    // Listen and write to debug serial for now
    if (net485dl->hasPacket()) {
        recvPtr = net485dl->getNextPacket();
        this->lasttimeOfMessage = millis();
        discNode = this->getNodeDiscResp(recvPtr);
        if(discNode == NULL) {
            Serial.print("{havemsg");
            // Request
            if(net485dl->isChecksumValid(recvPtr)) {
                Serial.println(", valid: true");
            } else {
                Serial.println(", valid: false");
            }
            // Send ACK
            this->net485dl->send(this->setACK(&pktToSend));

            sprintf(messageBuffer,", header: 0x");
            for(int i=0; i<MTU_HEADER ; i++) sprintf( &(messageBuffer[11+i*2]), "%0X ",recvPtr->header()[i]);
            sprintf(&(messageBuffer[11+2*MTU_HEADER]),", data: %0X .. (%d), chksum: 0x%0X%0X }", recvPtr->data()[0], recvPtr->dataSize
                    ,recvPtr->checksum()[0], recvPtr->checksum()[1]);
            Serial.println(messageBuffer);
        } else {
            // A Node discovery request
            this->sessionId = this->net485dl->newSessionId();
            this->slotTime = this->net485dl->newSlotDelayMicroSecs(ANET_SLOTLO,ANET_SLOTHI);
            havePkt = false;
            while (MILLISECDIFF(millis(),this->lasttimeOfMessage) < this->slotTime && !havePkt) {
                havePkt = net485dl->hasPacket();
            }
            if(!havePkt) {
                if(discNode->nodeType == NTC_ANY || discNode->nodeType == net485dl->getNodeType()) {
                    this->net485dl->send(this->setNodeDiscResp(&sendPkt));
                    this->lasttimeOfMessage = millis();
                    havePkt = false;
                    while (MILLISECDIFF(millis(),this->lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
                        havePkt = net485dl->hasPacket();
                        if(havePkt) {
                            recvPtr = net485dl->getNextPacket();
                            if(this->getNodeAddress(recvPtr)) {
                                this->net485dl->send(this->setACK(&sendPkt));
                            }
                        }
                    }                    
                }
            }
        }
    }
}
void Net485Network::loopServer() {
    Net485Packet *pkt, sendPkt;
    Net485DataVersion netVer;
    bool havePkt = false;
    if(lastNodeListPoll == 0 ||  MILLISECDIFF(this->lasttimeOfMessage,this->lastNodeListPoll) > NODELIST_REPOLLTIME)
    {
        if(Net485Network::reqRespNodeDiscover()) {
            // Loop over Nodes
            for(int i =NODEADDR_PRIMY; i<MTU_DATA; i++) {
                if(this->netNodeList[i]>0) {
                    havePkt = false;
                    if(i==NODEADDR_PRIMY) {
                        havePkt = Net485Network::reqRespNodeId(this->netNodeList[i], SUBNET_V1SPEC, true);
                    }
                    if(!havePkt) {
                        havePkt = Net485Network::reqRespNodeId(this->netNodeList[i], SUBNET_BCAST, true);
                    }
                    if(!havePkt) {
                        havePkt = Net485Network::reqRespSetAddress(i, NODEADDR_BCAST);
                    }
                    if(havePkt) {
                        this->nodes[i]->nodeStatus = Net485NodeStatE::Verified;
                    }
                }
            }
        }
    } else {
        if(net485dl->hasPacket()) {
            pkt = net485dl->getNextPacket();
            if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ANUCVER) {
                netVer.init(pkt);
                this->state = (netVer.comp(this->ver,netVer)>0
                               ? Net485State::ANServerBecoming
                               : Net485State::ANClientBecoming);
            } else {
                // TODO: Process packet as server
                Serial.print("{serverToFlowData}");
            }
            this->lasttimeOfMessage = millis();
        }
    }
}
bool Net485Network::reqRespSetAddress(uint8_t _node, uint8_t _subnet) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false;
    
    this->net485dl->send(this->setNodeAddress(&sendPkt,_node,_subnet));
    this->lasttimeOfMessage = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
        havePkt = net485dl->hasPacket();
        if(havePkt) {
            pkt = net485dl->getNextPacket();
            this->getRespNodeAddress(pkt);
            this->lasttimeOfMessage = millis();
            this->nodes[_node]->lastExchange = this->lasttimeOfMessage;
            
            if(this->getRespNodeAddress(pkt) == NULL) {
                havePkt = false;
                this->nodes[_node]->nodeStatus = Net485NodeStatE::OffLine;
            }
        }
    }
    return havePkt;
}
bool Net485Network::reqRespNodeId(uint8_t _node, uint8_t _subnet, bool _validateOnly) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false;

    this->net485dl->send(this->setNodeId(&sendPkt,_node,_subnet));
    this->lasttimeOfMessage = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
        havePkt = net485dl->hasPacket();
        if(havePkt) {
            pkt = net485dl->getNextPacket();
            this->getNodeId(pkt,_node, _validateOnly);
            this->lasttimeOfMessage = millis();
            if(this->nodes[_node]->nodeStatus == Net485NodeStatE::OffLine)  havePkt = false;
        }
    }
    return havePkt;
}
bool Net485Network::reqRespNodeDiscover(uint8_t _nodeIdFilter) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false, anyPkt = false;
    
    this->net485dl->send(this->setNodeDisc(&sendPkt,_nodeIdFilter));
    this->lasttimeOfMessage = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < SLOT_HIGH && !havePkt) {
        havePkt = net485dl->hasPacket();
        if(havePkt) {
            pkt = net485dl->getNextPacket();
            this->lasttimeOfMessage = millis();
            if(this->getNodeDiscResp(pkt) == NULL) {
                if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ANUCVER) {
                    this->state = Net485State::ANServerWaiting;
                    return false;
                }
            }
            anyPkt = true;
        }
    }
    return anyPkt;
}
void Net485Network::loop() {
    unsigned long thisTime = millis();
#ifdef DEBUG
    {
        char messageBuffer[100]; // Temp for testing only
        if(lastState != this->state) {
            sprintf(messageBuffer,"{state: %d, now:%lu}",this->state, thisTime);
            Serial.println(messageBuffer);
            lastState = this->state;
        }
    }
#endif
    // Network silence time
    if( MILLISECDIFF(thisTime,lasttimeOfMessage) > PROLONGED_SILENCE ) {
        this->warmStart(thisTime);
    }
    switch(this->state) {
        case Net485State::ANClient:
            this->loopClient();
            break;
        case Net485State::ANServer:
            this->loopServer();
            break;
        default:
            this->warmStart(thisTime);
            break;
    }
}

void Net485Network::warmStart(unsigned long _thisTime) {
    Net485Packet *pkt, sendPkt;
    Net485DataVersion netVer;
    bool havePkt = false;

    // Pick new random slot time and wait further for specific msg types
    if(!this->slotTime) {
        this->slotTime = net485dl->newSlotDelayMicroSecs(ANET_SLOTLO,ANET_SLOTHI);
        switch (this->state) {
            case Net485State::ANServerBecoming:
                net485dl->setPacketFilter(NULL,NULL,NULL,NULL);
                break;
            default:
                net485dl->setPacketFilter(NULL,NULL,NULL,nodeTypeArbFilterList);
                break;
        }
    }
    if(MILLISECDIFF(_thisTime,lasttimeOfMessage) > this->slotTime)
    {
        if( this->ver.isFFD ) {
            switch(this->state) {
                case Net485State::ANClientBecoming:
                    if(this->sub != NULL) {
                        this->state = Net485State::ANClient;
                    } else {
                        // otherwise, go quiet - left as client becoming until reset
                        Serial.print("{clientGoneQuiet1}");
                        lasttimeOfMessage = millis();
                    }
                    break;
                case Net485State::ANServerBecoming:
                    if(net485dl->hasPacket()) {
                        pkt = net485dl->getNextPacket();
                        if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ANUCVER) {
                            netVer.init(pkt);
                            this->state = (netVer.comp(this->ver,netVer)>0
                                           ? Net485State::ANServerBecoming
                                           : Net485State::ANClientBecoming);
                        }
                        // If pkt of any type, re-try process
                        this->lasttimeOfMessage = _thisTime + PROLONGED_SILENCE;
                    } else {
                        this->net485dl->send(this->setCAVA(&pktToSend));
                        this->lasttimeOfMessage = millis();
                        this->state = Net485State::ANServerWaiting;
                    }
                    break;
                case Net485State::ANServerWaiting:
                    if(net485dl->hasPacket()) {
                        this->state = (MILLISECDIFF(_thisTime,becomingTime) > RESPONSE_TIMEOUT
                                       ? Net485State::ANServer
                                       : Net485State::None);
                        this->lasttimeOfMessage = _thisTime + PROLONGED_SILENCE;
                    }
                    break;
                default:
                    becomingTime = _thisTime;
                    if(net485dl->hasPacket()) {
                        pkt = net485dl->getNextPacket();
                        if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ANUCVER) {
                            netVer.init(pkt);
                            this->state = (netVer.comp(this->ver,netVer)>0
                                           ? Net485State::ANServerBecoming
                                           : Net485State::ANClientBecoming);
                        }
                        // If pkt = Node discovery re-try process
                        this->lasttimeOfMessage = _thisTime + PROLONGED_SILENCE;
                    } else {
                        this->state = Net485State::ANServerBecoming;
                    }
                    break;
            }
        } else {
            if(this->sub != NULL) {
                this->state = Net485State::ANClient;
            } else {
                // otherwise, go quiet - left as client becoming until reset
                Serial.println("{client without subordinate, gone quiet}");
                lasttimeOfMessage = millis();
            }
        }
        this->slotTime = 0; // Finished with slot time, clear it
    }
}
