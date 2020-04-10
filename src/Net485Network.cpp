/*
 *  Net485Network.cpp
 *  Net485
 */

#include "Net485Network.hpp"
#include <assert.h>

#ifdef DEBUG
uint8_t lastState = 0xff;
#endif

#define ANET_SLOTLO  6000
#define ANET_SLOTHI 30000
const uint8_t msgTypeVerDiscFilterList[] = {MSGTYP_ANUCVER,MSGTYP_NDSCVRY, NULL};
const uint8_t msgTypeAssignNodeFilterList[] = {MSGTYP_SADDR, NULL};


Net485Network::Net485Network(Net485DataLink *_net, Net485Subord *_sub, bool _coordinatorCapable
                             , uint16_t _coordVer = 0, uint16_t _coordRev = 0) {
    assert(_net != NULL && _net->getNodeType() != NTC_ANY); // Node type must be defined
    this->workQueue = new Net485WorkQueue(this);
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
    this->nodeId = 0;
    this->subNet = 0;
    for(int i=0; i<MTU_DATA; i++) {
        this->netNodeList[i] = 0x00;
        this->nodes[i] = NULL;
    }
    this->netNodeListHighest = 0;
    if(this->sub != NULL && this->ver.isFFD) {
        this->netNodeList[0] = this->net485dl->getNodeType();
        this->netNodeListHighest++;
    }
}
Net485Network::~Net485Network()
{
    delete this->workQueue;
}
// Search from Primary Node Address location up to Note Address Request broker
// for matching node in node list.
// firstNodeTypeSearch: true - match first nodeType, false - match node type, sessionId, and MAC
// Returns: index in node list if found, zero otherwise
uint8_t Net485Network::nodeExists(Net485Node *_node, bool firstNodeTypeSearch) {
    for(int i = NODEADDR_PRIMY; i< MTU_DATA; i++) {
        if(this->netNodeList[i] > 0) { // A node location is used
            if(firstNodeTypeSearch && _node->nodeType == this->netNodeList[i] ) {
                return i;
            } else {
                if(_node->isSameAs(this->nodes[i])) // If node is same, return location
                    return i;
            }
        }
    }
    return 0;
}

// Add new node to the node list in the next available location
// _node: Node to add to list
// _nodeId: Optional, specifiy node location
// Returns: Node index of new node or zero of out of locations
// TODO: Is there memory leak with updater?  Should free first?
uint8_t Net485Network::addNode(Net485Node *_node, uint8_t _nodeId) {
    uint8_t nodeIndex = (_nodeId > 0 ? _nodeId : _node->nextNodeLocation(this->netNodeList));

    if(nodeIndex > 0) {
        if(this->nodes[nodeIndex] == NULL) {
            this->nodes[nodeIndex] = malloc(sizeof(Net485Node));
        }
        this->netNodeList[nodeIndex] = _node->nodeType;
        memcpy(this->nodes[nodeIndex],_node,sizeof(Net485Node));
        this->netNodeListHighest = (nodeIndex+1 > this->netNodeListHighest ? nodeIndex+1 : this->netNodeListHighest);
    }
    return nodeIndex;
}
void Net485Network::loopClient(unsigned long _thisTime) {
    Net485Packet *recvPtr, sendPkt;
    bool havePkt;
    
    if (net485dl->hasPacket())
    {
        recvPtr = net485dl->getNextPacket();
        if(this->nodeId > 0) { // Enrolled in network behaviour
            if( recvPtr->header()[HeaderStructureE::HeaderDestAddr] == this->nodeId
                && recvPtr->header()[HeaderStructureE::HeaderSubnet] == this->subNet )
            { // Addressed to this node messages
                switch(recvPtr->header()[HeaderStructureE::PacketMsgType]) {
                    default:
                        this->net485dl->send(this->setACK(&pktToSend));
#ifdef DEBUG
                        {
                            char messageBuffer[160]; // Temp for testing only
                            sprintf(messageBuffer,"{header: 0x");
                            for(int i=0; i<MTU_HEADER ; i++) sprintf( &(messageBuffer[11+i*2]), "%0X ",recvPtr->header()[i]);
                            sprintf(&(messageBuffer[11+2*MTU_HEADER]),", data: %0X .. (%d), chksum: 0x%0X%0X }", recvPtr->data()[0], recvPtr->dataSize
                                    ,recvPtr->checksum()[0], recvPtr->checksum()[1]);
                            Serial.println(messageBuffer);
                        }
#endif
                }
                this->lasttimeOfMessage = millis();
            }
            if( recvPtr->header()[HeaderStructureE::HeaderDestAddr] == NODEADDR_BCAST
                && (recvPtr->header()[HeaderStructureE::HeaderSubnet] == this->subNet
                || recvPtr->header()[HeaderStructureE::HeaderSubnet] == SUBNET_BCAST ) )
            { // Broadcast messages
                switch(recvPtr->header()[HeaderStructureE::PacketMsgType]) {
                    case MSGTYP_SADDR: // Coordinator sets device address and subnet
                        if(this->getNodeAddress(recvPtr)) { // If message is for this device (same sessionId and mac)
                            this->net485dl->send(this->setRespNodeAddress(&sendPkt));
                        }
                        break;
                    case MSGTYP_ADDRCNFM: // Verify position in node list, refresh timeout
                        if(!this->isNodeListValid(recvPtr)) {
                            this->state = Net485State::ANClient;
                            this->nodeId = 0;
                            this->subNet = 0;
                        }
                        this->lasttimeOfMessage = millis();
                        break;
                    default:
                    // These are ignored
                        break;
                }
            }
        } else { // Not in network
            // We only listen for a Node discovery request
            if( recvPtr->header()[HeaderStructureE::PacketMsgType] == MSGTYP_NDSCVRY ) {
                this->slotTime = this->net485dl->newSlotDelay(ANET_SLOTLO,ANET_SLOTHI);
#ifdef DEBUG
                Serial.print("delay ... "); Serial.print(this->slotTime);
#endif
                // Slot delay before responding to broadcast message
                havePkt = false;
                this->lasttimeOfMessage = millis();
                while (MILLISECDIFF(millis(),this->lasttimeOfMessage) < this->slotTime && !havePkt) {
                    havePkt = net485dl->hasPacket();
                }
#ifdef DEBUG
                Serial.print(" havePkt:"); Serial.println(havePkt,HEX);
#endif
                if(!havePkt) {
                    uint8_t nodeTypeFilter = getNodeDiscNodeTypeFilter(recvPtr);
#ifdef DEBUG
                    Serial.print("clientNodeType:"); Serial.print(net485dl->getNodeType(),HEX); Serial.print(" discoverNodeType:"); Serial.println(nodeTypeFilter,HEX);
#endif
                    // Nobody else responded, continue
                    if(nodeTypeFilter == NTC_ANY || nodeTypeFilter == net485dl->getNodeType())
                    {
                        // Message is for any node or this node's nodeType
                        this->net485dl->send(this->setNodeDiscResp(&sendPkt));
                        this->lasttimeOfMessage = millis();
                        havePkt = false;
                        // Wait for address assignment of this node
#ifdef DEBUG
                    Serial.print("wait ... ");
#endif
                        this->lasttimeOfMessage = millis();
                        net485dl->setPacketFilter(NULL,NULL,NULL,msgTypeAssignNodeFilterList);
                        while (MILLISECDIFF(millis(),this->lasttimeOfMessage)
                            < RESPONSE_TIMEOUT * 1.2 /* This factor is a divergance from specification which is 1.0 */
                                && !havePkt) {
                            havePkt = net485dl->hasPacket(&(this->lasttimeOfMessage));
                            if(havePkt) {
                                recvPtr = net485dl->getNextPacket();
#ifdef DEBUG
                                Serial.print(" received msgType:");
                                Serial.println(recvPtr->header()[HeaderStructureE::PacketMsgType]);
#endif
                                if(this->getNodeAddress(recvPtr)) {
                                    this->net485dl->send(this->setRespNodeAddress(&sendPkt));
                                }
                            }
                        }
                        net485dl->setPacketFilter(NULL,NULL,NULL,NULL);
#ifdef DEBUG
                        if(!havePkt) {
                            Serial.println(" no assignment from coordinator!");
                        }
#endif
                    }
                } /* !havePkt */
            } /* MSGTYP_NDSCVRY */
        } /* Not in network */
    } /* If have packet */
}
void Net485Network::loopServer(unsigned long _thisTime) {
    Net485Packet *pkt, sendPkt;
    Net485DataVersion netVer;
    bool found = false;

    if(lastNodeListPoll == 0 ||  MILLISECDIFF(_thisTime,this->lastNodeListPoll) > NODELIST_REPOLLTIME)
    {
        //
        // Poll for new nodes
        //
        uint8_t nodeId = Net485Network::reqRespNodeDiscover();
#ifdef DEBUG
                Serial.print("nodeDisc: "); Serial.print(nodeId); Serial.print(" ");
#endif
        uint8_t nextNodeId = 0;
        // Check for pre-existing node and/or find next free node
        // Loop exits with:
        //          - zero if no node found
        //          - non-zero is next free node
        while( !found
            && nodeId > 0
            && nodeId < NODEADDR_V2HI /* out of valid node Ids */)
        {
            if(nodeId==NODEADDR_PRIMY) {
                // For thermostat node type - send msg to check availability of nodeid
                nextNodeId = Net485Network::reqRespNodeId(nodeId, SUBNET_V1SPEC, true);
#ifdef DEBUG
                Serial.print(" isV1SPECThermostat: "); Serial.print(nextNodeId); Serial.print(" ");
#endif
            } else {
                // Any other device from thermostat - send msg to check availability of nodeid
                nextNodeId = Net485Network::reqRespNodeId(nodeId, SUBNET_BCAST, true);
#ifdef DEBUG
                Serial.print(" isAnyDevice: "); Serial.print(nextNodeId); Serial.print(" ");
#endif
            }
            found = (nodeId == nextNodeId)  /* Node IDs match if exact match found, set exit condition */
                || (nextNodeId == 0 && nodeId > 0); /* No prior existing node */
            if(nodeId < nextNodeId) nodeId = nextNodeId;
            if(nextNodeId == 0) {
                // There is no pre-existing node at this location, so make temp node list addition perminant
                this->netNodeList[nodeId] = this->nodes[nodeId]->nodeType;
                this->nodes[nodeId]->nodeStatus = Net485NodeStatE::Verified;
                
            }
        }
        if(nodeId > 0) { // Node ID validated with device, Assign node ID location
            nodeId = Net485Network::reqRespSetAddress(nodeId, NODEADDR_BCAST);
        }
        //
        // Address Confirmation broadcast
        //
        this->net485dl->send(this->setAddressConfirm(&sendPkt));
        //
        this->lastNodeListPoll = _thisTime;
    } else {
        // If out-of process message is ever received by server, it should attempt to yield and become client
        if(net485dl->hasPacket()) {
            pkt = net485dl->getNextPacket();
            this->becomeClient(pkt, _thisTime);
            this->lasttimeOfMessage = _thisTime;
        }
    }
    // TODO: The server related stuff should go here
}
// Set the device's nodeId.  If no valid response, put node offline
//
// Returns: true if value set, false otherwise
bool Net485Network::reqRespSetAddress(uint8_t _node, uint8_t _subnet) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false;
    uint8_t setNode = _node;
    uint8_t setSubnet = _subnet;
    
    // Unassign this device if not verified by this point
#ifdef DEBUG
        Serial.print("nodeId: "); Serial.print(_node);
        Serial.print(" subnet:"); Serial.print(_subnet);
        Serial.print(" status:"); Serial.print(this->nodes[_node]->nodeStatus);
        Serial.print(" verifiedStatus:"); Serial.print(Net485NodeStatE::Verified);
        Serial.println("");
#endif
    if(this->nodes[_node]->nodeStatus != Net485NodeStatE::Verified) {
        setNode = 0;
        setSubnet = 0;
    }
#ifdef DEBUG
        Serial.print("nodeAssign: {nodeId:"); Serial.print(setNode);
        Serial.print(" subnet:"); Serial.print(setSubnet);
        Serial.println("}");
#endif
    this->net485dl->send(this->setNodeAddress(&sendPkt,setNode,setSubnet,this->nodes[_node]));
    this->lasttimeOfMessage = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
        havePkt = net485dl->hasPacket(&(this->lasttimeOfMessage));
        if(havePkt) {
            pkt = net485dl->getNextPacket();
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
// Create and send a NodeID message, receive and conditionaly validate receipt.
//
// Return: NodeId if node Id responds, otherwise zero
uint8_t Net485Network::reqRespNodeId(uint8_t _node, uint8_t _subnet, bool _validateOnly) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false;
    uint8_t matchOrNextNodeId = 0;
    
    this->net485dl->send(this->getNodeId(&sendPkt,_node,_subnet));
    this->lasttimeOfMessage = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
        havePkt = net485dl->hasPacket(&(this->lasttimeOfMessage));
        if(havePkt) {
            pkt = net485dl->getNextPacket();
            matchOrNextNodeId = this->addNodeId(pkt,_node, false);
            this->lasttimeOfMessage = millis();
        }
    }
    return matchOrNextNodeId;
}
// Send broadcast for node response, if non response received, go to coordinator arbitration state
// otherwise, add to node list.
//
// Return nodeId if received response on node discovery request, zero otherwise
uint8_t Net485Network::reqRespNodeDiscover(uint8_t _nodeIdFilter) {
    Net485Packet *pkt, sendPkt;
    bool anyPkt = false;
    uint8_t nodeId = 0;
    
    this->net485dl->send(this->setNodeDisc(&sendPkt,_nodeIdFilter));
    this->lasttimeOfMessage = millis();
    this->lastNodeListPoll = millis();
    while(MILLISECDIFF(millis(),this->lasttimeOfMessage) < ANET_SLOTHI && !anyPkt) {
        anyPkt = net485dl->hasPacket();
    }
    if(anyPkt) {
        pkt = net485dl->getNextPacket();
        this->lasttimeOfMessage = millis();
        nodeId = this->getNodeDiscResp(pkt);
        if(!nodeId) {
            if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ANUCVER) {
                this->state = Net485State::ANClient;
                return false;
            }
        }
    }
    return nodeId;
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
            this->nodeId = 0;
            this->subNet = 0;
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
        this->slotTime = net485dl->newSlotDelay(ANET_SLOTLO,ANET_SLOTHI);
        switch (this->state) {
            case Net485State::ANServerBecomingA:
            case Net485State::ANServerBecomingB:
                net485dl->setPacketFilter(NULL,NULL,NULL,NULL);
                break;
            default:
                net485dl->setPacketFilter(NULL,NULL,NULL,msgTypeVerDiscFilterList);
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
        this->nodeId = 0;
        this->subNet = 0;
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
