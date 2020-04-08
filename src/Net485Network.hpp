/*
 *  Net485Network.hpp
 *  Net485
 *
 */
#ifndef Net485Network_hpp
#define Net485Network_hpp

#define DEBUG

#include "Net485DataLink.hpp"
#include "Net485API.hpp"
#include "Net485Subord.hpp"

#define NODELIST_REPOLLTIME 110000 /* milli-sconds  ?correct? */
#define RESPONSE_TIMEOUT 3000 /* milli-sconds */
#define PROLONGED_SILENCE 120000 /* milli-sconds */
#define MILLISECDIFF(future,past) ((future < past)? (0xffffffffUL-past)+future : future - past )

typedef enum Net485NodeStatE {
    OffLine,
    Unverified,
    Verified,
    Online
} Net485NodeStat;

typedef struct Net485NodeS {
    Net485MacAddress macAddr;
    uint64_t sessionId;
    uint8_t nodeType;
    Net485NodeStat nodeStatus;
    uint8_t version;
    unsigned long lastExchange;
    void init(Net485Packet *pkt) {
        version = PKTVER(pkt);
        if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_GNODEID)) {
            nodeType = pkt->data()[0];
            memcpy(&macAddr, &(pkt->data()[1]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+1]), sizeof(uint64_t));
            nodeStatus = Net485NodeStatE::Unverified;
        }
        // Init Node values from Node Discovery request
        if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_NDSCVRY) {
            nodeType = pkt->data()[0];
            // Initialize other values to zero
            macAddr.clear();
            sessionId = 0;
            version = 0;
            lastExchange = 0;
            nodeStatus = Net485NodeStatE::Unverified;
        }
        if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_NDSCVRY)) {
            nodeType = pkt->data()[0];
            memcpy(&macAddr, &(pkt->data()[2]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+2]), sizeof(uint64_t));
            nodeStatus = Net485NodeStatE::Unverified;
        }
        if(pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_SADDR) {
            nodeType = NTC_ANY;
            version = pkt->data()[1];
            memcpy(&macAddr, &(pkt->data()[2]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+2]), sizeof(uint64_t));
            nodeStatus = (pkt->data()[sizeof(uint64_t)+Net485MacAddressE::SIZE+2] == 0x01
                          ? Net485NodeStatE::Verified
                          : Net485NodeStatE::Unverified);
        }
#ifdef DEBUG
        Serial.print("Net485Node.init(): "); display();
#endif
        lastExchange = millis();
    };
    bool isSameAs(struct Net485NodeS *_node) {
        return _node->macAddr.isSameAs(&macAddr) && _node->sessionId == sessionId
        && (_node->nodeType == nodeType
            || _node->nodeType == NTC_ANY
            || nodeType == NTC_ANY) && (_node->version == version || _node->version == 0 || version == 0 );
    };
    uint8_t nextNodeLocation(uint8_t netNodeList[MTU_DATA]) {
        int i;
        // Get next available position
        i = ((nodeType == NTC_THERM || nodeType == NTC_ZCTRL)
             ? NODEADDR_PRIMY
             : (version == NETV1 ? 1+NODEADDR_V1LO : NODEADDR_V2LO ));
        while(netNodeList[i]!=0 && i<= (version == NETV1? NODEADDR_V1HI : NODEADDR_V2HI ) ) {
            i = (version == NETV2
                 && (nodeType == NTC_THERM || nodeType == NTC_ZCTRL)
                 && i==1+NODEADDR_V1LO
                 ? NODEADDR_V2LO : i+1 );
        }
        // Out of nodes, return zero
        if(i > (version == NETV1? NODEADDR_V1HI : NODEADDR_V2HI )) return 0;
        // TODO: Check if condensed list is full (if there are ANY V1 devices)
        return i;
    };
    bool isNodeIdValid(uint8_t _nodeId) {
        if((nodeType == NTC_THERM || nodeType == NTC_ZCTRL) && _nodeId == 1 )
        {
#ifdef DEBUG
            Serial.println("nodeId is valid - nodeType == NTC_THERM || nodeType == NTC_ZCTRL");
#endif
            return true;
        }
        if(version == NETV1 && _nodeId > NODEADDR_V1LO && _nodeId <= NODEADDR_V1HI) return true;
        {
#ifdef DEBUG
            Serial.println("nodeId is valid - version == NETV1 && _nodeId > NODEADDR_V1LO && _nodeId <= NODEADDR_V1HI");
#endif
            return true;
        }
        if(version == NETV2 && _nodeId >= NODEADDR_V2LO && _nodeId <= NODEADDR_V2HI) return true;
        {
#ifdef DEBUG
            Serial.println("nodeId is valid - NETV2 && _nodeId >= NODEADDR_V2LO && _nodeId <= NODEADDR_V2HI");
#endif
            return true;
        }
#ifdef DEBUG
            Serial.println("nodeId is NOT valid ");
#endif
        return false;
    };
    void display() {
#ifdef DEBUG
        Serial.print("{Node sessionid:");
        Serial.print((unsigned long)sessionId >> 32,HEX);
        Serial.print(" ");
        Serial.print((unsigned long)sessionId & 0xffffffff,HEX);
        Serial.print(" nodeType:");
        Serial.print(nodeType,HEX);
        Serial.print(" nodeStatus:");
        Serial.print(nodeStatus,HEX);
        Serial.print(" version:");
        Serial.print(version,HEX);
        Serial.print(" lastExchange:");
        Serial.print(lastExchange);
        Serial.print(" macAddr:");
        macAddr.display();
        Serial.println("}");
#endif
    }
} Net485Node;

enum Net485State {
    None,
    ANClient, /* 1 */
    ANServerBecomingA,
    ANServerBecomingB,
    ANServerWaiting,
    ANServer, /* 5 */
    ANServerPollNodes,
    ANServerDiscover
};

class Net485Network {
private:
    Net485DataLink *net485dl;

    // Network status
    unsigned long lasttimeOfMessage, lastNodeListPoll;
    unsigned int slotTime, becomingTime;
    Net485State state;

    Net485DataVersion ver;
    uint64_t sessionId;
    uint8_t nodeId, subNet;
    
    uint8_t netNodeList[MTU_DATA];
    uint8_t netNodeListHighest;
    Net485Node *nodes[MTU_DATA];
    Net485Packet pktToSend;
    Net485Subord *sub;

    // For RFD Subordinate's coordinator info
    uint64_t coordinatorMACaddr, coordinatorSessionId;

    // For FFD - Coordinator
    Net485Node *netNodes;

    void warmStart(unsigned long _thisTime);
    void loopClient(unsigned long _thisTime);
    void loopServer(unsigned long _thisTime);
    bool becomeClient(Net485Packet *pkt, unsigned long _thisTime);

    uint8_t reqRespNodeId(uint8_t _node, uint8_t _subnet, bool _validateOnly = false);
    uint8_t reqRespNodeDiscover(uint8_t _nodeIdFilter = 0x00);
    bool reqRespSetAddress(uint8_t _node, uint8_t _subnet);

    uint8_t nodeExists(Net485Node *_node, bool firstNodeTypeSearch = false);
    uint8_t addNode(Net485Node *_node, uint8_t _nodeId = 0);
public:
    Net485Network(Net485DataLink *_net, Net485Subord *_sub = NULL, bool _coordinatorCapable = true, uint16_t _coordVer = 0, uint16_t _coordRev = 0);
    ~Net485Network();
    
    void loop();
    
    inline Net485Packet *setR2R(Net485Packet *_pkt) {
#define R2R_CODE 0x00
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_R2R;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 17;
        _pkt->data()[0] = R2R_CODE;
        memcpy((void *)&(_pkt->data()[1]), net485dl->getMacAddr().mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[1+Net485MacAddressE::SIZE])
               , &sessionId, sizeof(uint64_t));
        return _pkt;
    }
    inline Net485Packet *setACK(Net485Packet *_pkt, uint8_t _destNodeId = NODEADDR_COORD, uint8_t _subNet = SUBNET_V2SPEC ) {
#define ACK_CODE 0x06
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _destNodeId;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = this->nodeId;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = _subNet;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_R2R;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 17;
        _pkt->data()[0] = ACK_CODE;
        memcpy((void *)&(_pkt->data()[1]), net485dl->getMacAddr().mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[1+Net485MacAddressE::SIZE])
               , &sessionId, sizeof(uint64_t));
        return _pkt;
    }
    inline Net485Packet *setCAVA(Net485Packet *_pkt) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_NARB;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_ANUCVER;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 5;
        return ver.write(_pkt);
    }
    
    inline Net485Packet *setNetState(Net485Packet *_pkt) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_V2SPEC;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_NETSTATE;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 0;
        return _pkt;
    }
    inline uint8_t getNetStateResp(Net485Packet *_pkt) {
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_NETSTATE) ) {
            memcpy(netNodeList,_pkt->data()
                   , _pkt->header()[HeaderStructureE::PacketLength]);
            return _pkt->header()[HeaderStructureE::PacketLength];
        } else return 0;
    }
    
    inline Net485Packet *getNodeId(Net485Packet *_pkt, uint8_t _destAddr, uint8_t _subNet = SUBNET_BCAST) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _destAddr;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = _subNet;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_GNODEID;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 0;
        return _pkt;
    }
    // Add node to node list at specified node Id OR test and compare node at node list
    // location with node provided.  Specified node will have status either OffLine or Verified.
    // _pkt: packet with node to add or compare to what is in node list
    // _validateOnly: flag for adding or validating
    //
    // Returns: zero if packet is not MSGRESP(MSGTYP_GNODEID), nodeId if verified packet OR next nodeId if
    //   verification fails
    inline uint8_t addNodeId(Net485Packet *_pkt, uint8_t _node, bool _validateOnly = false) {
        Net485Node tmpNode;
        uint8_t nodeId = 0;
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_GNODEID) ) {
            tmpNode.init(_pkt);
#ifdef DEBUG
    Serial.print(" addNodeId: validate:"); Serial.print(_validateOnly); Serial.print(" ");
    Serial.print(" isValid:"); Serial.print(tmpNode.isNodeIdValid(_node)); Serial.print(" ");
    Serial.print(" isSame:"); Serial.print(this->nodes[_node]->isSameAs(&tmpNode)); Serial.print(" ");
    Serial.println("  ");
    this->nodes[_node]->display();
    Serial.println("  ");
    tmpNode.display();
    Serial.println("  ");
#endif
            if(_validateOnly) {
                // Compares with what is in node list, if pass then change status
                if(tmpNode.isNodeIdValid(_node)) {
                    if(this->nodes[_node]->isSameAs(&tmpNode)) {
                        this->nodes[nodeId]->nodeStatus = Net485NodeStatE::Verified;
                        nodeId = _node;
                        this->netNodeList[nodeId] = tmpNode.nodeType;
                    } else {
                        nodeId = tmpNode.nextNodeLocation(this->netNodeList);
                    }
                } else {
                    this->nodes[_node]->nodeStatus = Net485NodeStatE::OffLine;
                    this->nodes[_node]->lastExchange = millis();
                    nodeId = tmpNode.nextNodeLocation(this->netNodeList);
                }
            } else {
                nodeId = this->addNode(&tmpNode, _node);
            }
        }
        return nodeId;
    }
    inline Net485Packet *setNodeAddress(Net485Packet *_pkt, uint8_t _newAddr, uint8_t _newSubNet, Net485Node *_node) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_SADDR;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 19;
        _pkt->data()[0] = _newAddr;
        _pkt->data()[1] = _newSubNet;
        memcpy((void *)&(_pkt->data()[2]), _node->macAddr.mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[2+Net485MacAddressE::SIZE])
               , &(_node->sessionId), sizeof(uint64_t));
        _pkt->data()[2+Net485MacAddressE::SIZE+sizeof(uint64_t)] = 0x01;
        return _pkt;
    }
    // Set the node ID from the message if MAC, Session, and Node status is verified
    //
    // Return true if set, false otherwise
    inline bool getNodeAddress(Net485Packet *_pkt) {
        Net485Node tmpNode;
        bool result = false;
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_SADDR ) {
            tmpNode.init(_pkt);
            tmpNode.nodeType = this->net485dl->getNodeType();
            result = net485dl->getMacAddr().isSameAs(&(tmpNode.macAddr))
                && tmpNode.sessionId == this->sessionId
                && _pkt->data()[2+Net485MacAddressE::SIZE+sizeof(uint64_t)] == 0x01;
            if(result) {
                this->nodeId = _pkt->data()[0];
                this->subNet = _pkt->data()[1];
                if(this->nodeId > 0) this->addNode(&tmpNode, this->nodeId);
                // Don't set subnet as we only code for V2
            }
        }
#ifdef DEBUG
        Serial.print("NodeAddress set: ");
        Serial.print(result);
        Serial.print(" thisDevice:");
        net485dl->getMacAddr().display();
        Serial.print(" nodeId: ");
        Serial.println(this->nodeId);
#endif
        return result;
    }
    inline Net485Packet *setRespNodeAddress(Net485Packet *_pkt) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = this->nodeId;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = this->subNet;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGRESP(MSGTYP_SADDR);
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 19;
        _pkt->data()[0] = this->nodeId;
        _pkt->data()[1] = this->subNet;
        memcpy((void *)&(_pkt->data()[2]), (this->nodes[this->nodeId])->macAddr.mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[2+Net485MacAddressE::SIZE])
               , &((this->nodes[this->nodeId])->sessionId), sizeof(uint64_t));
        _pkt->data()[2+Net485MacAddressE::SIZE+sizeof(uint64_t)] = 0x01;
        return _pkt;
    }
    inline Net485Packet *getRespNodeAddress(Net485Packet *_pkt) {
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_NDSCVRY) ) {
            return _pkt;
        } else return NULL;
    }

    inline Net485Packet *setNodeDisc(Net485Packet *_pkt, uint8_t _nodeIdFilter = 0x00) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_NDSCVRY;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 1;
        _pkt->data()[0] = _nodeIdFilter;
        return _pkt;
    }
    //  Create a new response.  Side effect is creation of new sessionId.
    //
    //
    inline Net485Packet *setNodeDiscResp(Net485Packet *_pkt, uint8_t _nodeId = NODEADDR_BCAST) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = _nodeId;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_V2SPEC;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGRESP(MSGTYP_NDSCVRY);
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 18;
        _pkt->data()[0] = net485dl->getNodeType();
        _pkt->data()[1] = 0x00;

        this->sessionId = this->net485dl->newSessionId();

        memcpy((void *)&(_pkt->data()[2]), net485dl->getMacAddr().mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[2+Net485MacAddressE::SIZE])
               , &(this->sessionId), sizeof(uint64_t));
        return _pkt;
    }
    // Evaluate Node Discovery Message response and add to list.
    // _pkt : The package to search
    // Returns: index to node in node list if found or created,
    //  zero if not node discovery message or node list is full
    inline uint8_t getNodeDiscResp(Net485Packet *_pkt) {
        Net485Node tmpNode;
        uint8_t nodeIndex = 0;
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_NDSCVRY)) {
            tmpNode.init(_pkt);
            if(tmpNode.nodeType == NTC_ANY) return 0;
            nodeIndex = nodeExists(&tmpNode);
            if(nodeIndex == 0) nodeIndex = this->addNode(&tmpNode);
#ifdef DEBUG
            Serial.print("getNodeDiscResp: nodeIndex:"); Serial.print(nodeIndex,HEX); Serial.print(" ");  tmpNode.display();
#endif
        }
        return nodeIndex;
    }
    // Evaluate Node Discovery Message.
    // _pkt : The package to search
    // Returns: Node Type filter value
    inline uint8_t getNodeDiscNodeTypeFilter(Net485Packet *_pkt) {
        Net485Node n;
        n.init(_pkt);
        return n.nodeType;
    }
    // Prepare an Address confirmation packet for sending
    //
    // Returns pointer provided as argument
    inline Net485Packet *setAddressConfirm(Net485Packet *_pkt) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_BCAST;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SUBNET_V2SPEC;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_ADDRCNFM;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = this->netNodeListHighest;
        memcpy((void *)(_pkt->data()), this->netNodeList, _pkt->header()[HeaderStructureE::PacketLength] );
        return _pkt;
    }
    // Compare local node list with broadcast node list
    //
    // returns True if same, false otherwise
    inline bool isNodeListValid(Net485Packet *_pkt) {
        bool retVal = true;
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ADDRCNFM) {
            int nodeCount = _pkt->header()[HeaderStructureE::PacketLength];
#ifdef DEBUG
    Serial.print("isNodeListValid(MSGTYP_ADDRCNFM): count:"); Serial.print(nodeCount);
    Serial.print(" this->nodeId:"); Serial.print(this->nodeId, HEX);
    Serial.print(" this->netNodeListHighest:"); Serial.print(this->netNodeListHighest);
#endif
            if(this->nodeId == 0) return false;
            if(this->netNodeListHighest != nodeCount) return false;
            for(int i=NODEADDR_PRIMY; i<nodeCount && retVal; i++) {
                // Don't compare the local subordinate node, they will be different (i=0)
                retVal = retVal & ( this->netNodeList[i] == _pkt->data()[i] );
#ifdef DEBUG
    Serial.print(" index:"); Serial.print(i);
    Serial.print(" "); Serial.print(this->netNodeList[i], HEX);
    Serial.print(" =?= "); Serial.print(_pkt->data()[i], HEX);
#endif
            }
#ifdef DEBUG
    Serial.print(" retVal:"); Serial.println(retVal);
#endif
            return retVal;
        }
        return false;
    }
};

#endif
