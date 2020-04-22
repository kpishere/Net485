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

#define ROUTEP_DEVICES 6
#define ROUTEP_COMMANDS 6
const uint8_t routingPriority[][ROUTEP_DEVICES] = {
    {PARM1_CMND_HEAT,   NTC_ZCTRL,  NTC_HPUMP,  NTC_FURNGAS,    NTC_XOVER,  NTC_AHNDLR},
    {PARM1_CMND_COOL,   NTC_ZCTRL,  NTC_HPUMP,  NTC_ACOND,      NTC_XOVER,  NTC_FURNGAS},
    {PARM1_CMND_FAN,    NTC_ZCTRL,  NTC_AHNDLR, NTC_FURNGAS,    NTC_XOVER,  NTC_XOVER},
    {PARM1_CMND_EMERG,  NTC_ZCTRL,  NTC_AHNDLR, NTC_FURNGAS,    NTC_XOVER,  NTC_XOVER},
    {PARM1_CMND_DEFRST, NTC_ZCTRL,  NTC_AHNDLR, NTC_FURNGAS,    NTC_XOVER,  NTC_XOVER},
    {PARM1_CMND_AUXHEAT,NTC_ZCTRL,  NTC_AHNDLR, NTC_FURNGAS,    NTC_XOVER,  NTC_XOVER}
};

uint8_t nextSubnet3NodeId;

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
    nextSubnet3NodeId = 0;
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
bool Net485Network::subnetNodeExists(uint8_t _netVersion) {
    for(int i = NODEADDR_PRIMY; i<= this->netNodeListHighest; i++) {
        if(this->netNodeList[i] > 0) { // A node location is used
            if(this->nodes[i] != NULL && this->nodes[i]->version == _netVersion ) {
                return true;
            }
        }
    }
    return false;
}
uint8_t Net485Network::rollNextNet2node() {
    if(nextSubnet3NodeId > this->netNodeListHighest || nextSubnet3NodeId < NODEADDR_PRIMY)
        nextSubnet3NodeId = NODEADDR_PRIMY;
    for(int i = nextSubnet3NodeId ; i<= this->netNodeListHighest; i++) {
        if(this->netNodeList[i] > 0) { // A node location is used
            if(this->nodes[i] != NULL && this->nodes[i]->version == NETV2 ) {
                nextSubnet3NodeId++;
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
#ifdef DEBUG
    Serial.print(" addNode() nodeIndex a: "); Serial.print(nodeIndex, HEX); Serial.println("");
#endif
    if(nodeIndex > 0) {
        if(this->nodes[nodeIndex] == NULL) {
            this->nodes[nodeIndex] = malloc(sizeof(Net485Node));
        }
        this->netNodeList[nodeIndex] = _node->nodeType;
        memcpy(this->nodes[nodeIndex],_node,sizeof(Net485Node));
        this->netNodeListHighest = (nodeIndex > this->netNodeListHighest ? nodeIndex : this->netNodeListHighest);
#ifdef DEBUG
    Serial.print(" addNode() netNodeListHighest: "); Serial.print(this->netNodeListHighest, HEX); Serial.println("");
#endif
    }
#ifdef DEBUG
    Serial.print(" addNode() nodeIndex b: "); Serial.print(nodeIndex, HEX); Serial.println("");
#endif
    return nodeIndex;
}
uint8_t Net485Network::delNode(uint8_t _nodeId) {
#ifdef DEBUG
    Serial.print(" delNode() _nodeId: "); Serial.print(_nodeId, HEX);
    Serial.print(" netNodeListHighest: "); Serial.print(this->netNodeListHighest, HEX);
    Serial.print(" nodeType: "); Serial.print(this->netNodeList[_nodeId], HEX);
#endif
    if(this->netNodeList[_nodeId] > 0) {
        if(this->nodes[_nodeId] != NULL) {
            free(this->nodes[_nodeId]);
            this->nodes[_nodeId] = NULL;
        }
        this->netNodeList[_nodeId] = 0;
        // Reduce # if this is highest node in list
        if(_nodeId == this->netNodeListHighest - 1) {
            this->netNodeListHighest--;
        }
    }
#ifdef DEBUG
    Serial.print(" nodeType: "); Serial.print(this->netNodeList[_nodeId], HEX);
    Serial.print(" netNodeListHighest: "); Serial.print(this->netNodeListHighest, HEX);
    Serial.println("");
#endif
}
void Net485Network::loopClient(unsigned long _thisTime) {
    Net485Packet *recvPtr, sendPkt;
    bool havePkt;
    
    if (net485dl->hasPacket())
    {
        recvPtr = net485dl->getNextPacket();
#ifdef DEBUG
    Serial.print("myNodeId: "); Serial.print(this->nodeId, HEX);
    Serial.print(" targetNodeId: "); Serial.print(recvPtr->header()[HeaderStructureE::HeaderDestAddr], HEX);
    Serial.print(" mySubnet: "); Serial.print(this->subNet, HEX);
    Serial.print(" targetSubnet: "); Serial.print(recvPtr->header()[HeaderStructureE::HeaderSubnet], HEX);
    Serial.print(" msgType: "); Serial.print(recvPtr->header()[HeaderStructureE::PacketMsgType], HEX);
    Serial.println("");
#endif
        if(this->nodeId > 0) { // Enrolled in network behaviour
#ifdef DEBUG
    Serial.println(" IN NETWORK");
#endif
            if( recvPtr->header()[HeaderStructureE::HeaderDestAddr] == this->nodeId
                && recvPtr->header()[HeaderStructureE::HeaderSubnet] == this->subNet )
            { // Addressed to this node messages
#ifdef DEBUG
    Serial.println(" THIS NODE");
#endif
                switch(recvPtr->header()[HeaderStructureE::PacketMsgType]) {
                    case MSGTYP_SNETLIST:
                        this->copyPacketToNodeList(recvPtr);
                        this->net485dl->send(this->setNetListResp(&pktToSend));
                        break;
                    case MSGTYP_R2R:
#ifdef DEBUG
    Serial.println(" MSGTYP_R2R");
#endif
                        if(this->sub != NULL && this->sub->hasPacket(&(this->lasttimeOfMessage)) ) {
#ifdef DEBUG
    Serial.println(" VIRTUAL DEVICE CALL");
#endif
                            this->net485dl->send(this->sub->getNextPacket());
                        } else {
#ifdef DEBUG
    Serial.println(" VIRTUAL DEVICE R2R_ACK");
#endif
                            this->net485dl->send(this->setACK(&pktToSend));
                        }
                        break;
                    default:
                        if(!PKTISDATA(recvPtr) && this->sub != NULL) {
                            // Forward packet to virtual device if CT-CIM Application message types
                            this->sub->send(recvPtr);
                            this->net485dl->send(this->setACK(&pktToSend));
                        }
                        break;
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
                            this->lasttimeOfMessage = millis();
                        }
                        break;
                    case MSGTYP_ADDRCNFM: // Verify position in node list
                        if(!this->isNodeListValid(recvPtr)) {
                            this->state = Net485State::ANClient;
                            this->nodeId = 0;
                            this->subNet = 0;
                        }
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
                                Serial.println(recvPtr->header()[HeaderStructureE::PacketMsgType], HEX);
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
    bool found = false, newDeviceSet = false;
    int r2rPerDataCycle = R2R_LOOPS_PERDATACYCLE;
    int nodeId;

    if(lastNodeListPoll == 0 ||  MILLISECDIFF(_thisTime,this->lastNodeListPoll) > NODELIST_REPOLLTIME)
    {
        // Address Confirmation broadcast - NetV1
        this->issueR2RNetV1();
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
                nextNodeId = Net485Network::reqRespNodeId(nodeId, SUBNET_V1SPEC);
#ifdef DEBUG
                Serial.print(" isV1SPECThermostat: "); Serial.print(nextNodeId); Serial.print(" ");
#endif
            } else {
                // Any other device from thermostat - send msg to check availability of nodeid
                nextNodeId = Net485Network::reqRespNodeId(nodeId, SUBNET_BCAST);
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
            newDeviceSet = Net485Network::reqRespSetAddress(nodeId);
        }
        // Inform rest of network of new device
        if(newDeviceSet) {
            this->issueToNetwork(MSGTYP_SNETLIST,true);
        }
        //
        this->lastNodeListPoll = _thisTime;
    } else { // Dataflow cycle
        uint8_t foundNodeId;
        this->lasttimeOfMessage = _thisTime;

        // If out-of process message is ever received by server, ignore it
        if(net485dl->hasPacket()) {
            pkt = net485dl->getNextPacket();
        }
        this->removeOfflineDevices();
        
        // Priority subordinate active
        if( this->netNodeList[NODEADDR_PRIMY] == 0
            && this->net485dl->getNodeType() != NTC_THERM
            && this->net485dl->getNodeType() != NTC_ZCTRL ) {
#ifdef DEBUG
        Serial.print(" PRIMRY:"); Serial.print(this->netNodeList[NODEADDR_PRIMY]);
        Serial.print(" subNode:"); Serial.print(this->net485dl->getNodeType());
        Serial.println("");
#endif
            this->assignNewNode(NODEADDR_PRIMY);
        } else {
            // (1) Perform message transaction cycle with primary node
            this->setR2R(&sendPkt, NODEADDR_PRIMY);
            this->workQueue->pushWork(sendPkt);
            this->workQueue->doWork();
            // (2) Perform Node discovery
            foundNodeId = Net485Network::reqRespNodeDiscover();
            if(foundNodeId) {
                this->assignNewNode(foundNodeId);
            } else {
                // If Node on Subnet 3
                if(this->subnetNodeExists(NETV2)) {
                    // - (3) send Addr Conf. Broadcast -- clients check node list and
                    //   remove themselves if not on (they don't respond)
                    this->net485dl->send(this->setAddressConfirm(&sendPkt, NODEADDR_BCAST, SUBNET_V2SPEC));
                    do {
                        //      - (4) send Token Offer Broadcast
                        nodeId = this->reqRespTOB(NTC_ANY,NODEADDR_BCAST,SUBNET_V2SPEC);
                        if(nodeId) {
                        //      - (4.1.1) if resp, send R2R and process transaction
                            this->setR2R(&sendPkt, nodeId);
                            this->workQueue->pushWork(sendPkt);
                            this->workQueue->doWork();
                        }
                    } while(--r2rPerDataCycle > 0 && nodeId > 0); // - Loop upto 5 times
                }
                // (5) If Node on Subnet 2
                if(this->subnetNodeExists(NETV1)) {
                    // - For each node
                    //      - send R2R and process transaction
                    this->issueToNetwork(MSGTYP_R2R,true,false);
                }
                nodeId = this->rollNextNet2node();
                // (6) Send R2R to the next Subnet 3 node in a slow rolling list
                if(nodeId) {
                    this->setR2R(&sendPkt, nodeId);
                    this->workQueue->pushWork(sendPkt);
                    this->workQueue->doWork();
                }
                // (7) Send R2R to Virtual Internal Subordinate
                this->setR2R(&sendPkt, NODEADDR_VRTSUB);
                this->workQueue->pushWork(sendPkt);
                this->workQueue->doWork();
                // (8) Any other Coordinator Transactions
                this->workQueue->doWork();
            }
        }
    }
}
void Net485Network::assignNewNode(uint8_t foundNodeId) {
    foundNodeId = Net485Network::reqRespNodeId(foundNodeId, SUBNET_V1SPEC);
    if(foundNodeId == 0 ) {
        foundNodeId = Net485Network::reqRespNodeId(foundNodeId, SUBNET_V2SPEC);
    }
    if(foundNodeId)
    {
        this->netNodeList[foundNodeId] = this->nodes[foundNodeId]->nodeType;
        this->nodes[foundNodeId]->nodeStatus = Net485NodeStatE::Verified;
        if(Net485Network::reqRespSetAddress(foundNodeId)) {
            this->issueToNetwork(MSGTYP_SNETLIST,true);
        }
    }
}
// Set the device's nodeId.  If no valid response, put node offline
//
// Returns: true if value set, false otherwise
bool Net485Network::reqRespSetAddress(uint8_t _node) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false;
    uint8_t setNode = _node;
    uint8_t setSubnet = (this->nodes[_node]->version==NETV1 ? SUBNET_V1SPEC:SUBNET_V2SPEC );
    
    // Unassign this device if not verified by this point
#ifdef DEBUG
        Serial.print("nodeId: "); Serial.print(_node);
        Serial.print(" subnet:"); Serial.print(setSubnet);
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
#ifdef DEBUG
        Serial.println(" *node set offline* ");
#endif
                havePkt = false;
                this->delNode(_node);
            }
        }
    }
    return havePkt;
}
// Create and send a NodeID message, receive and conditionaly validate receipt.
//
// Return: NodeId if node Id responds, otherwise zero
uint8_t Net485Network::reqRespNodeId(uint8_t _node, uint8_t _subnet) {
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
// Create and send a Token Offer Broadcast (TOB) message, receive validate
//
// Return: NodeId if response and is valid, otherwise zero
uint8_t Net485Network::reqRespTOB(uint8_t _nodeTypeFilter, uint8_t _destNodeId, uint8_t _subNet) {
    Net485Packet *pkt, sendPkt;
    bool havePkt = false;
    Net485Node tobACK;
    uint8_t nodeId = 0;
    
    this->net485dl->send(this->setTOB(&sendPkt,_nodeTypeFilter,_destNodeId,_subNet));
    this->lasttimeOfMessage = millis();
    while(MILLISECDIFF(millis(),lasttimeOfMessage) < RESPONSE_TIMEOUT && !havePkt) {
        havePkt = net485dl->hasPacket();
        if(havePkt) {
            pkt = net485dl->getNextPacket();
            if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_TOKEN)) {
                tobACK.init(pkt);
                nodeId = tobACK.nodeType;
                tobACK.nodeType = NTC_ANY;
                if(this->nodes[nodeId] == NULL || !tobACK.isSameAs(this->nodes[nodeId])) {
                    nodeId = 0;
                }
            }
        }
    }
    return nodeId;
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
// Perform routing of message to next device
//
// Returns: true if received response, false otherwise (like for timeout) or not routed
bool Net485Network::sendMsgGetResponseInPlace(Net485Packet *_pkt) {
    Net485Packet *recvPtr, sendPkt;
    bool haveResponse = false;
    int retriesRemain = MSG_RESEND_ATTEMPTS;
    
    uint8_t sndMthd = _pkt->header()[HeaderStructureE::HeaderSndMethd];
    switch(sndMthd) {
    case SNDMTHD_PRIORITY:
        if(!this->setPriorityNode(_pkt)) {
#ifdef DEBUG
            Serial.println("SNDMTHD_PRIORITY not routed");
#endif
            return false; // This packet will not be routed
        }
        break;
    case SNDMTHD_NTYPE:
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = this->getNodeIdOfType(_pkt->header()[HeaderStructureE::HeaderSndParam], 0);
        if(_pkt->header()[HeaderStructureE::HeaderDestAddr] == NODEADDR_COORD) {
#ifdef DEBUG
            Serial.println("SNDMTHD_NTYPE not routed");
#endif
            return false; // This packet will not be routed
        }
        break;
    case SNDMTHD_BYSCKT: // This method supported by NETV2 only
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _pkt->header()[HeaderStructureE::HeaderSndParam];
        if(_pkt->header()[HeaderStructureE::HeaderDestAddr] == NODEADDR_COORD) {
#ifdef DEBUG
            Serial.println("SNDMTHD_BYSCKT not routed");
#endif
            return false; // This packet will not be routed
        }
        break;
    default: /* SNDMTHD_NOROUTE */
        break;
    }
    // update Send Parameter 2
    if(this->state == Net485State::ANServer) {
        _pkt->header()[HeaderStructureE::HeaderSndParam1]
        = this->getNodeIndexAmongTypes(_pkt->header()[HeaderStructureE::HeaderDestAddr]);
    } else { // Clients always clear this value
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0;
    }
    
    if( _pkt->header()[HeaderStructureE::HeaderDestAddr] == NODEADDR_VRTSUB
     || _pkt->header()[HeaderStructureE::HeaderDestAddr] == 0x00 ) {
        // This message is destined for the virtual node
        if(this->sub != NULL) {
#ifdef DEBUG
            Serial.println("VIRTUAL SEND");
#endif
            switch(_pkt->header()[HeaderStructureE::PacketMsgType]) {
                case MSGTYP_R2R:
                    if(this->sub != NULL && this->sub->hasPacket(&(this->lasttimeOfMessage))) {
                        recvPtr = this->sub->getNextPacket();
                        memcpy(_pkt,recvPtr, sizeof(Net485Packet));
                    } else {
                        uint8_t toNodeId = _pkt->header()[HeaderStructureE::HeaderSrcAddr];
                        recvPtr = this->setACK(_pkt, toNodeId);
                        recvPtr->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_VRTSUB;
                        memcpy(_pkt,recvPtr, sizeof(Net485Packet));
                    }
                    break;
                default:
                    if(!PKTISDATA(_pkt) && this->sub != NULL) {
                        // Forward packet to virtual device if CT-CIM Application message types
                        this->sub->send(_pkt);
                    }
                    break;
            }
            this->lasttimeOfMessage = millis();
        }
    } else {
        // Network routed messages
        if(this->nodes[_pkt->header()[HeaderStructureE::HeaderDestAddr]]->nodeStatus != Net485NodeStat::OffLine) {
            // Send packet, get response, retry up to N times
            do {
                retriesRemain--;
                this->net485dl->send(_pkt);
                this->lasttimeOfMessage = millis();
                while (MILLISECDIFF(millis(),this->lasttimeOfMessage) < RESPONSE_TIMEOUT && !haveResponse) {
                    haveResponse = net485dl->hasPacket(&(this->lasttimeOfMessage));
                    if(haveResponse) {
                        recvPtr = net485dl->getNextPacket();
                        memcpy(_pkt,recvPtr, sizeof(Net485Packet));
                    }
                }
            } while (retriesRemain > 0 && !haveResponse);
        }
    }
    return haveResponse;
}

uint8_t Net485Network::getNodeIndexAmongTypes(uint8_t _nodeId) {
    uint8_t nodeIndex = 0;
    uint8_t subjNodeType = this->nodes[_nodeId]->nodeType;
    for(int i=0; i<_nodeId; i++) nodeIndex += ((this->netNodeList[i]) == subjNodeType ? 1: 0);
    return nodeIndex;
}

// Find Nth node of a given type.
//
// Returns NodeId if found, NODEADDR_COORD value if NOT found
uint8_t Net485Network::getNodeIdOfType(uint8_t _nodeType, uint8_t _nodeIndex) {
    uint8_t nodeId = NODEADDR_COORD; // Indicates not found
    int finds = 0;
    for(int i=0; i<=this->netNodeListHighest && nodeId == NODEADDR_COORD; i++) {
        if(this->netNodeList[i] == _nodeType) {
            if(finds == _nodeIndex) {
                nodeId = i;
            } else {
                finds++;
            }
        }
    }
    return nodeId;
}

// Find the priority node for the given command.  NOTE: If returns false, destination node is invalid!
//
// Returns: true if found, false otherwise
bool Net485Network::setPriorityNode(Net485Packet *_pkt) {
    bool foundNode = false, foundDevice = false;
    uint8_t cmdCode = _pkt->header()[HeaderStructureE::HeaderSndParam];
    for(int i =0; i<ROUTEP_COMMANDS && !foundNode; i++) {
        foundNode |= (routingPriority[i][0] == cmdCode);
        if(foundNode) {
            for(int j=1; j < ROUTEP_DEVICES && !foundDevice; j++ ) {
                _pkt->header()[HeaderStructureE::HeaderDestAddr] = this->getNodeIdOfType(routingPriority[i][j], 0);
                foundDevice |= ( _pkt->header()[HeaderStructureE::HeaderDestAddr] != NODEADDR_COORD);
            }
        }
    }
    return foundNode && foundDevice;
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
        if(this->state != Net485State::ANClient) this->warmStart(thisTime);
        else { // Take device offline due to inactivity from coordinator
            this->nodeId = 0;
            this->subNet = 0;
        }
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
                    // Set the network Virtual node for coordinator
                    if(this->sub != NULL) {
                        this->netNodeList[0] = this->net485dl->getNodeType();
                        this->netNodeListHighest = 0;
                    }
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
