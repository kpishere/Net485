/*
 *  Net485Network.cpp
 *  Net485
 */

#include "Net485Network.hpp"

#define DEBUG

#ifdef DEBUG
uint8_t lastState = 0xff;
#endif

#define ANET_SLOTLO 6000
#define ANET_SLOTHI 30000
const uint8_t nodeTypeArbFilterList[] = {MSGTYP_ANUCVER,MSGTYP_NDSCVRY, NULL};


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

void Net485Network::loopClient(unsigned long _thisTime) {
    Net485Packet *recvPtr, sendPkt;
    bool havePkt;
    Net485Node *discNode;
    char messageBuffer[160]; // Temp for testing only
    // TODO
    // Listen and write to debug serial for now
    if (net485dl->hasPacket())
    {
        recvPtr = net485dl->getNextPacket();
        this->lasttimeOfMessage = _thisTime;
        discNode = this->getNodeDiscResp(recvPtr);
        switch(recvPtr->header()[HeaderStructureE::PacketMsgType]) {
            case MSGTYP_ANUCVER: break; // Ignore these requests, we are client unless there is timeout
            case MSGTYP_NDSCVRY:
                // A Node discovery request
                this->sessionId = this->net485dl->newSessionId();
                this->slotTime = this->net485dl->newSlotDelayMicroSecs(ANET_SLOTLO,ANET_SLOTHI);
#ifdef DEBUG
                {
                    Serial.print("{sessionId: ");
                    Serial.print((long unsigned int)this->sessionId);
                    Serial.print(", msgType: ");
                    Serial.print((uint8_t)recvPtr->header()[(int)HeaderStructureE::PacketMsgType] );
                    Serial.println("}");
                }
#endif

                havePkt = false;
                while (MILLISECDIFF(_thisTime,this->lasttimeOfMessage) < this->slotTime && !havePkt) {
                    havePkt = net485dl->hasPacket();
                }
                if(!havePkt) {
                    if(discNode->nodeType == NTC_ANY || discNode->nodeType == net485dl->getNodeType())
                    {
                        this->net485dl->send(this->setNodeDiscResp(&sendPkt));
                        this->lasttimeOfMessage = _thisTime;
                        havePkt = false;
                        while (MILLISECDIFF(_thisTime,this->lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
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
                break;
            default:
                if(discNode == NULL) {
                    Serial.print("{havemsg");
                    // Request
                    if(net485dl->isChecksumValid(recvPtr)) {
                        Serial.println(", valid: true}");
                        
                        // Send ACK
                        this->net485dl->send(this->setACK(&pktToSend));
                        
                        sprintf(messageBuffer,", header: 0x");
                        for(int i=0; i<MTU_HEADER ; i++) sprintf( &(messageBuffer[11+i*2]), "%0X ",recvPtr->header()[i]);
                        sprintf(&(messageBuffer[11+2*MTU_HEADER]),", data: %0X .. (%d), chksum: 0x%0X%0X }", recvPtr->data()[0], recvPtr->dataSize
                                ,recvPtr->checksum()[0], recvPtr->checksum()[1]);
                        Serial.println(messageBuffer);
                    } else {
                        Serial.println(", valid: false}");
                    }
                }
        }
    }
}
void Net485Network::loopServer(unsigned long _thisTime) {
    Net485Packet *pkt, sendPkt;
    Net485DataVersion netVer;
    bool havePkt = false;
    if(lastNodeListPoll == 0 ||  MILLISECDIFF(_thisTime,this->lastNodeListPoll) > NODELIST_REPOLLTIME)
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
            this->becomeClient(pkt, _thisTime);
            this->lasttimeOfMessage = _thisTime;
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
    this->lastNodeListPoll = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < SLOT_HIGH && !havePkt) {
        havePkt = net485dl->hasPacket();
        if(havePkt) {
            pkt = net485dl->getNextPacket();
            this->lasttimeOfMessage = millis();
            if(this->getNodeDiscResp(pkt, true) == NULL) {
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
            sprintf(messageBuffer,"{state: %d, now:%lu, deltaLasttimeOfMessage:%lu}",this->state, thisTime, MILLISECDIFF(thisTime,lasttimeOfMessage));
            Serial.println(messageBuffer);
            lastState = this->state;
        }
    }
#endif
    // Network silence time - device warm starts if this silence is exceeded
    if( MILLISECDIFF(thisTime,lasttimeOfMessage) > PROLONGED_SILENCE ) {
        this->warmStart(thisTime);
    }
    switch(this->state) {
        case Net485State::ANClient:
            this->loopClient(thisTime);
            break;
        case Net485State::ANServer:
            this->loopServer(thisTime);
            break;
        default:
            this->warmStart(thisTime);
            break;
    }
}
/// Returns true if CAVA message
bool Net485Network::becomeClient(Net485Packet *pkt, unsigned long _thisTime) {
    Net485DataVersion netVer;
    bool isCAVA = false;
#ifdef DEBUG
    char messageBuffer[160]; // Temp for testing only
#endif

    if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ANUCVER) {
        isCAVA = true;
        // 1.1 CAVA Seen
        netVer.init(pkt);
        
#ifdef DEBUG
        sprintf(messageBuffer,"{becomClient: %hu leftVer: %hu leftRev:%hu rightVer: %hu rightRev: %hu}",netVer.comp(this->ver,netVer), this->ver.Version, this->ver.Revision, netVer.Version, netVer.Revision);
        Serial.println(messageBuffer);
#endif

        if( netVer.comp(this->ver,netVer)>0 ) {
            // Is Net CAVA < own CAVA 1.2 Yes
            this->state = Net485State::ANServerBecomingB;
            this->slotTime = 0; // Set for recalc of slot time
        } else {
            // Become client 1.1 No
            if(this->sub != NULL) {
                net485dl->setPacketFilter(NULL,NULL,NULL,NULL);
                this->state = Net485State::ANClient;
#ifdef DEBUG
                Serial.println("{client, gone quiet}");
#endif
            } else {
                // otherwise, go quiet - left as client becoming until reset
#ifdef DEBUG
                Serial.println("{client without subordinate, gone quiet}");
#endif
            }
        }
        this->lasttimeOfMessage = _thisTime;
    }
    return isCAVA;
}
void Net485Network::warmStart(unsigned long _thisTime) {
    Net485Packet *pkt, sendPkt;
    Net485DataVersion netVer;
    bool havePkt = false;
    
    // Pick new random slot time and wait further for specific msg types
    if(!this->slotTime) {
        this->slotTime = net485dl->newSlotDelayMicroSecs(ANET_SLOTLO,ANET_SLOTHI);
        switch (this->state) {
            case Net485State::ANServerBecomingA:
            case Net485State::ANServerBecomingB:
                net485dl->setPacketFilter(NULL,NULL,NULL,NULL);
                break;
            default:
                net485dl->setPacketFilter(NULL,NULL,NULL,nodeTypeArbFilterList);
                break;
        }
    }
    if( this->ver.isFFD ) {
        switch(this->state) {
            case Net485State::ANServerBecomingA:
                if(net485dl->hasPacket()) {
                    pkt = net485dl->getNextPacket();
                    if(this->becomeClient(pkt, _thisTime)) {
                        // 1.2.1 become's client
                    } else if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_NDSCVRY) {
                        // Restart warmStart process 1.2.2 Yes
                        this->state = Net485State::None;
                        this->slotTime = 0; // Set for recalc of slot time
                        this->lasttimeOfMessage = _thisTime;
                    }
                } else if(MILLISECDIFF(_thisTime,lasttimeOfMessage) > this->slotTime) {
                    // Timeout 1.2.3 Yes
                    this->net485dl->send(this->setCAVA(&pktToSend));
                    this->lasttimeOfMessage = millis();
                    this->state = Net485State::ANServerWaiting;
                }
                break;
            case Net485State::ANServerBecomingB:
                if(net485dl->hasPacket()) {
                    pkt = net485dl->getNextPacket();
                    if(this->becomeClient(pkt, _thisTime)) {
                        // 1.1.2.1 become's client
                    } else {
                        // Restart warmStart process 1.1.2.2 Yers
                        this->state = Net485State::None;
                        this->slotTime = 0; // Set for recalc of slot time
                        this->lasttimeOfMessage = _thisTime;
                    }
                } else if(MILLISECDIFF(_thisTime,lasttimeOfMessage) > this->slotTime) {
                    // Timeout 1.1.2.3 Yes
                    this->net485dl->send(this->setCAVA(&pktToSend));
                    this->lasttimeOfMessage = millis();
                    this->state = Net485State::ANServerWaiting;
                }
                break;
            case Net485State::ANServerWaiting:
                if(net485dl->hasPacket()) {
                    pkt = net485dl->getNextPacket();
                    if(this->becomeClient(pkt, _thisTime)) {
                        // 1.2.1 become's client
                    } else {
                        // Restart warmStart process 1.1.2.2 Yers
                        this->state = Net485State::ANServerBecomingB;
                        this->slotTime = 0; // Set for recalc of slot time
                        this->lasttimeOfMessage = _thisTime;
                    }
                } else if( MILLISECDIFF(_thisTime,this->lasttimeOfMessage) > RESPONSE_TIMEOUT ) {
                    // 1.1.2.3.2 Yes
                    this->state = Net485State::ANServer;
                }
                break;
            default:
                if(net485dl->hasPacket()) {
                    pkt = net485dl->getNextPacket();
                    if(this->becomeClient(pkt, _thisTime)) {
                        // 1.1 become's client
                    } else if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_NDSCVRY) {
                        // 1.2 It is Node Discovery (see filter above for default state)
                        this->state = Net485State::ANServerBecomingA;
                        this->slotTime = 0; // Set for recalc of slot time
                        this->lasttimeOfMessage = _thisTime;
                    }
                } else if(MILLISECDIFF(_thisTime,lasttimeOfMessage) > this->slotTime) {
                    // Timeout 2. Yes
                    this->state = Net485State::ANServerBecomingB;
                    this->slotTime = 0; // Set for recalc of slot time
                    this->lasttimeOfMessage = _thisTime;
                }
                break;
        }
    } else {
        // Is client
        if(this->sub != NULL) {
            net485dl->setPacketFilter(NULL,NULL,NULL,NULL);
            this->state = Net485State::ANClient;
#ifdef DEBUG
            Serial.println("{client, gone quiet}");
#endif
        } else {
            // otherwise, go quiet - left as client becoming until reset
#ifdef DEBUG
            Serial.println("{client without subordinate, gone quiet}");
#endif
        }
    }
}
