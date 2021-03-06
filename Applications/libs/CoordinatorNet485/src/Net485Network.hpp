/*
 *  Net485Network.hpp
 *  Net485
 *
 */
#ifndef Net485Network_hpp
#define Net485Network_hpp

#include "Net485DataLink.hpp"
#include "Net485API.hpp"
#include "Net485Subord.hpp"
#include "Net485WorkQueue.hpp"

#if defined(__AVR__)
#elif defined(ESP8266)
#elif __APPLE__
    #include <unistd.h>
    // Slow down blocking loops in multi-process environments
    #define MICROSECONDS_IN_SECOND 1e6
    #define LOOP_RATE_PER_SECOND 500
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
    #error "Unknown target"
#endif

#define NODELIST_REPOLLTIME 110000 /* milli-sconds  ?correct? */
#define RESPONSE_TIMEOUT 3000 /* milli-sconds */
#define PROLONGED_SILENCE 120000 /* milli-sconds */
#define MILLISECDIFF(future,past) ((future < past)? (0xffffffffUL-past)+future : future - past )
#define MSG_RESEND_ATTEMPTS 3
#define R2R_LOOPS_PERDATACYCLE 5
#define ANET_SLOTLO  6000 /* Slot delay for node discovery 6s to 30s */
#define ANET_SLOTHI 30000

#define PARM1_CMND_HEAT 0x64
#define PARM1_CMND_COOL 0x65
#define PARM1_CMND_FAN 0x66
#define PARM1_CMND_EMERG 0x67
#define PARM1_CMND_DEFRST 0x68
#define PARM1_CMND_AUXHEAT 0x69

enum Net485Diagnostic : uint8_t {
    UNDEFINED = 0,
    ERROR,
    WARNING,
    INFO_LOW,
    INFO_HIGH
};

typedef enum Net485NodeStatE {
    OffLine,
    Unverified,
    Verified,
    Online
} Net485NodeStat;

typedef struct Net485RoutingS {
    uint8_t SendMethod;
    uint8_t SendParam;
    uint8_t SendParam1;
    uint8_t SourceNode; // If not set, RFD/FFD sets
} Net485Routing;

typedef struct Net485NodeS {
    Net485MacAddress macAddr;
    uint64_t sessionId;
    uint8_t nodeType;
    Net485NodeStat nodeStatus;
    uint8_t version;
    unsigned long lastExchange;
    void init(Net485Packet *pkt) {
        uint8_t msgType = pkt->header()[HeaderStructureE::PacketMsgType];
        version = PKTVER(pkt);
        if(msgType == MSGRESP(MSGTYP_GNODEID)) {
            nodeType = pkt->data()[0];
            memcpy(&macAddr, &(pkt->data()[1]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+1]), sizeof(uint64_t));
            nodeStatus = Net485NodeStatE::Unverified;
        }
        // Init Node values from Node Discovery request
        if(msgType == MSGTYP_NDSCVRY) {
            nodeType = pkt->data()[0];
            // Initialize other values to zero
            macAddr.clear();
            sessionId = 0;
            version = 0;
            lastExchange = 0;
            nodeStatus = Net485NodeStatE::Unverified;
        }
        if(msgType == MSGRESP(MSGTYP_NDSCVRY)) {
            nodeType = pkt->data()[0];
            memcpy(&macAddr, &(pkt->data()[2]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+2]), sizeof(uint64_t));
            nodeStatus = Net485NodeStatE::Unverified;
        }
        if(msgType == MSGRESP(MSGTYP_TOKEN)) {
            nodeType = pkt->data()[0]; /* this is node ID !*/
            version = (pkt->data()[1] == SUBNET_V2SPEC ? NETV2 : NETV1);
            memcpy(&macAddr, (void *)&(pkt->data()[2]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, (void *)&(pkt->data()[2+Net485MacAddressE::SIZE]), sizeof(uint64_t));
        }
        if(msgType == MSGTYP_R2R ) {
            memcpy(&macAddr, &(pkt->data()[1]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+1]), sizeof(uint64_t));
            nodeStatus = Net485NodeStatE::Unverified;
        }
        if(msgType == MSGTYP_SADDR) {
            nodeType = NTC_ANY;
            version = pkt->data()[1];
            memcpy(&macAddr, &(pkt->data()[2]), Net485MacAddressE::SIZE);
            memcpy(&sessionId, &(pkt->data()[Net485MacAddressE::SIZE+2]), sizeof(uint64_t));
            nodeStatus = (pkt->data()[sizeof(uint64_t)+Net485MacAddressE::SIZE+2] == 0x01
                          ? Net485NodeStatE::Verified
                          : Net485NodeStatE::Unverified);
        }
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
            return true;
        }
        if(version == NETV1 && _nodeId > NODEADDR_V1LO && _nodeId <= NODEADDR_V1HI) return true;
        {
            return true;
        }
        if(version == NETV2 && _nodeId >= NODEADDR_V2LO && _nodeId <= NODEADDR_V2HI) return true;
        {
            return true;
        }
        return false;
    };
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
    Net485Diagnostic diagnostic;
    
    uint8_t netNodeList[MTU_DATA];
    uint8_t netNodeListHighest; /* An index value */
    Net485Node *nodes[MTU_DATA];
    Net485Packet pktToSend;
    Net485Subord *sub;

    Net485WorkQueue *workQueue;

    // For RFD Subordinate's coordinator info
    uint64_t coordinatorMACaddr, coordinatorSessionId;

    // For FFD - Coordinator
    Net485Node *netNodes;

    void warmStart(unsigned long _thisTime);
    void loopClient(unsigned long _thisTime);
    void loopServer(unsigned long _thisTime);
    bool becomeClient(Net485Packet *pkt, unsigned long _thisTime);

    uint8_t reqRespNodeId(uint8_t _node, uint8_t _subnet);
    uint8_t reqRespTOB(uint8_t _nodeTypeFilter = NTC_ANY, uint8_t _destNodeId = NODEADDR_BCAST, uint8_t _subNet = SUBNET_V2SPEC);
    uint8_t reqRespNodeDiscover(uint8_t _nodeIdFilter = 0x00);
    bool reqRespSetAddress(uint8_t _node);

    uint8_t nodeExists(Net485Node *_node, bool firstNodeTypeSearch = false);
    bool subnetNodeExists(uint8_t _subNet);
    uint8_t rollNextNet2node();
    uint8_t addNode(Net485Node *_node, uint8_t _nodeId = 0);
    void delNode(uint8_t _nodeId);

    void sendVirtDevMacMsg();

    
    // Utillity function to copy node list to packet's data
    // Expects packet header to be populated before calling this method.
    //
    inline void copyNodeListToPacket(Net485Packet *_pkt) {
        // Clear values in packet
        for(int i=0; i<MTU_DATA; i++) _pkt->data()[i] = 0x00;
        // Copy values to packet
        if( _pkt->header()[HeaderStructureE::HeaderSubnet] == SUBNET_V1SPEC ) {
            // Is Version 1 - condensed list
            int k = 0, v;
            for(int i=0; i<=NODEADDR_V1HI; i++ ) _pkt->data()[i] = 0x00;
            for(int i=0; i<=this->netNodeListHighest; i++) {
                bool dobreak = false;
                v = this->netNodeList[i];
                if(v == 0) continue; // don't add empty nodes
                for(int j=0; j<k && dobreak == false; j++) dobreak |= (v == _pkt->data()[j]);
                if(dobreak) continue; // add only unique node types
                _pkt->data()[k] = v;
                k++;
            }
            _pkt->header()[HeaderStructureE::PacketLength] = k;
        } else { // Is Version 2
            _pkt->header()[HeaderStructureE::PacketLength] = this->netNodeListHighest + 1;
            memcpy((void *)(_pkt->data()), this->netNodeList, _pkt->header()[HeaderStructureE::PacketLength] );
        }
    }
    // Utillity function to copy node list from packet to this object
    //
    inline void copyPacketToNodeList(Net485Packet *_pkt) {
        if( _pkt->header()[HeaderStructureE::HeaderSubnet] == SUBNET_V1SPEC ) {
            // Not expected to be implemented.
        } else {
            for(int i=0; i<MTU_DATA; i++) this->netNodeList[i] = 0x00;

            this->netNodeListHighest = _pkt->header()[HeaderStructureE::PacketLength] - 1;
            memcpy(this->netNodeList, (void *)(_pkt->data()),_pkt->header()[HeaderStructureE::PacketLength]);
        }
    }

    bool sendMsgGetResponseInPlace(Net485Packet *_pkt);
    uint8_t getNodeIndexAmongTypes(uint8_t _nodeId);
    uint8_t getNodeIdOfType(uint8_t _nodeType, uint8_t _nodeIndex = 0);
    bool setPriorityNode(Net485Packet *_pkt);
    void assignNewNode(uint8_t foundNodeId);
public:
    Net485Network(Net485DataLink *_net, Net485Subord *_sub = NULL, bool _coordinatorCapable = true, uint16_t _coordVer = 0, uint16_t _coordRev = 0, Net485Diagnostic _diagnostic = UNDEFINED);
    ~Net485Network();
        
    void loop();
    
    // Send packet to appropriate device.  Packet is routed as per header values to appropriate network device.
    // If further exchange is needed in processing, the next packet to send is over written in place from initial values
    // and false value is returned.
    //
    // Return: true - packet exchange complete, false - new packet to send set in place
    inline bool routePacket(Net485Packet *_pkt) {
        bool isExchangeComplete = true;
        bool gotResponse = false;
        uint8_t sentNodeId = _pkt->header()[HeaderStructureE::HeaderDestAddr];
        
        if(_pkt != NULL) {
            uint8_t msgType = _pkt->header()[HeaderStructureE::PacketMsgType];
            switch(msgType) {
            case MSGTYP_R2R:
                gotResponse = this->sendMsgGetResponseInPlace(_pkt);
                if(gotResponse) {
                    isExchangeComplete = (_pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_R2R
                        && _pkt->data()[0] == R2R_ACK_CODE);
                    if(isExchangeComplete) {
                        this->isR2RACKValid(_pkt, sentNodeId);
                    }
                }
                break;
            //case MSGTYP_SNETLIST:
            default:    // Generic expect MSGRESP() and only check that is returned
                gotResponse = this->sendMsgGetResponseInPlace(_pkt);
                if(gotResponse) {
                    isExchangeComplete = (MSGRESP(msgType) == _pkt->header()[HeaderStructureE::PacketMsgType]);
                }
                break;
            }
        }
        if(!gotResponse) {
            isExchangeComplete = true; // Regardless of stage in process, this work is done
            // Only remove NET nodes from list
            if(_pkt->header()[HeaderStructureE::HeaderDestAddr] > 0) {
                this->nodes[_pkt->header()[HeaderStructureE::HeaderDestAddr]]->nodeStatus = Net485NodeStatE::OffLine;
            }
        }
        return isExchangeComplete;
    }
    
    // Push node list to each node on network for NETV1.
    //
    // Note: Node Lists are sent directly and individually to each Subnet 2 node, directly and individually to each newly added subnet 3 node, and may be broadcast to all other Subnet 3 nodes. A broadcast node list to subnet 3 is required to ensure a node that considers itself as being addressed, will receive an indication the coordinator no longer considers that node part of the network.
    //
    //  Current:  Broadcast to each node in list (this does Subnet 2 then Subnet 3), then send
    //      broadcast to Subnet 3
    //
    inline void issueToNetwork(uint8_t msgType, bool doWorkImmediately = false, bool vnet2Broadcast = true) {
        Net485Packet sendPkt, *ptr;
        int workItems = 0;
        // Issue node list to each node on network
        for(int i=0; i<=this->netNodeListHighest; i++) {
            if(this->netNodeList[i] == 0x00) continue;
            if(this->nodes[i]->version == NETV2) continue;
            switch(msgType) {
                case MSGTYP_SNETLIST:
                    this->setNetList(&sendPkt, i);
                    break;
                case MSGTYP_R2R:
                    this->setR2R(&sendPkt, i);
                    break;
                default:
                    continue;
            }
            this->workQueue->pushWork(sendPkt); // NB: Zero gets translated to NODEADDR_VRTSUB by this method
            workItems++;
        }
        if(doWorkImmediately) this->workQueue->doWork(workItems);
        if(vnet2Broadcast) {
            // Broadcast on Subnet 3
            switch(msgType) {
                case MSGTYP_SNETLIST:
                    ptr = this->setNetList(&sendPkt, NODEADDR_BCAST);
                    break;
                case MSGTYP_R2R:
                    ptr = this->setR2R(&sendPkt, NODEADDR_BCAST);
                    break;
                default:
                    return;
            }
            this->net485dl->send( ptr );
        }
    }
    inline void removeOfflineDevices() {
        bool removedADevice = false;
        for(int i=1; i<=this->netNodeListHighest; i++) {
            if(this->netNodeList[i] == 0x00) continue;
            if(this->nodes[i]->nodeStatus == Net485NodeStatE::OffLine) {
                this->delNode(i);
                removedADevice = removedADevice || true;
            }
        }
        if(removedADevice) this->issueToNetwork(MSGTYP_SNETLIST,false);
    }
    inline Net485Packet *setR2R(Net485Packet *_pkt, uint8_t _destNodeId) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _destNodeId;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = (this->nodes[_destNodeId]->version==NETV1 ? SUBNET_V1SPEC:SUBNET_V2SPEC );
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
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
        _pkt->data()[0] = R2R_ACK_CODE;
        memcpy((void *)&(_pkt->data()[1]), net485dl->getMacAddr().mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[1+Net485MacAddressE::SIZE])
               , &sessionId, sizeof(uint64_t));
        return _pkt;
    }
    // Validate R2R Ack
    // _pkt: packet with node to add or compare to what is in node list
    //
    // Returns: zero if packet is not MSGRESP(MSGTYP_GNODEID), nodeId if verified packet OR next nodeId if
    //   verification fails
    inline bool isR2RACKValid(Net485Packet *_pkt, uint8_t _node) {
        Net485Node tmpNode;
        bool isValid = false;
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_R2R
            && _pkt->data()[0] == R2R_ACK_CODE) {
            tmpNode.init(_pkt);
            isValid = this->nodes[_node]->isSameAs(&tmpNode);
        }
        return isValid;
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
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGRESP(MSGTYP_SADDR) ) {
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
            if(nodeIndex == 0) {
                nodeIndex = this->addNode(&tmpNode);
            }
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
    // Prepare an Address confirmation packet for sending - Ver.2 spec only
    //
    // Returns pointer provided as argument
    inline Net485Packet *setAddressConfirm(Net485Packet *_pkt, uint8_t _dest = NODEADDR_BCAST, uint8_t _subNet = SUBNET_V2SPEC) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _dest;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = _subNet;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_ADDRCNFM;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        this->copyNodeListToPacket(_pkt);
        return _pkt;
    }
    // Compare local node list with broadcast node list
    //
    // returns True if same, false otherwise
    inline bool isNodeListValid(Net485Packet *_pkt) {
        bool retVal = true;
        if(_pkt->header()[HeaderStructureE::PacketMsgType] == MSGTYP_ADDRCNFM) {
            int nodeCount = _pkt->header()[HeaderStructureE::PacketLength];
            if(this->nodeId == 0) return false;
            if(this->netNodeListHighest+1 != nodeCount) return false;
            for(int i=0; i<nodeCount && retVal; i++) {
                retVal = retVal & ( this->netNodeList[i] == _pkt->data()[i] );
            }
            return retVal;
        }
        return false;
    }
    // Prepare an Address confirmation packet for sending
    //
    // Returns pointer provided as argument
    inline Net485Packet *setNetList(Net485Packet *_pkt, uint8_t _nodeId) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _nodeId;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        if( _nodeId == NODEADDR_BCAST ) {
            _pkt->header()[HeaderStructureE::HeaderSubnet] = SUBNET_V2SPEC;
        } else {
            _pkt->header()[HeaderStructureE::HeaderSubnet] = (this->nodes[_nodeId]->version==NETV1 ? SUBNET_V1SPEC:SUBNET_V2SPEC );
        }
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_SNETLIST;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(false,false);
        this->copyNodeListToPacket(_pkt);
        return _pkt;
    }
    // Respond to node list set command
    //
    // Returns pointer provided as argument
    inline Net485Packet *setNetListResp(Net485Packet *_pkt) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = NODEADDR_COORD;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = this->nodeId;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = (this->nodes[this->nodeId]->version==NETV1 ? SUBNET_V1SPEC:SUBNET_V2SPEC );
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = this->net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGRESP(MSGTYP_SNETLIST);
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(false,false);
        this->copyNodeListToPacket(_pkt);
        return _pkt;
    }
    inline Net485Packet *setTOB(Net485Packet *_pkt, uint8_t _nodeTypeFilter = NTC_ANY, uint8_t _destNodeId = NODEADDR_BCAST, uint8_t _subNet = SUBNET_V2SPEC) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _destNodeId;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = NODEADDR_COORD;
        if(_destNodeId == NODEADDR_BCAST) {
            _pkt->header()[HeaderStructureE::HeaderSubnet] = _subNet;
        } else {
            _pkt->header()[HeaderStructureE::HeaderSubnet] = (this->nodes[_destNodeId]->version==NETV1 ? SUBNET_V1SPEC:SUBNET_V2SPEC );
        }
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = NTC_NETCTRL;
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_TOKEN;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 1;
        _pkt->data()[0] = _nodeTypeFilter;
        return _pkt;
    }
    inline Net485Packet *setTOBACK(Net485Packet *_pkt, uint8_t _destNodeId = NODEADDR_COORD ) {
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = _destNodeId;
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = this->nodeId;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = this->subNet;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGRESP(MSGTYP_TOKEN);
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 18;
        _pkt->data()[0] = this->nodeId;
        _pkt->data()[1] = this->subNet;
        memcpy((void *)&(_pkt->data()[2]), net485dl->getMacAddr().mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[2+Net485MacAddressE::SIZE])
               , &sessionId, sizeof(uint64_t));
        return _pkt;
    }
    //
    // CIM Application message types
    //
    // MSGTYP_GCONF    0x01 /*Get Configuration*/
    // MSGTYP_GSTAT    0x02 /*Get Status*/
    // MSGTYP_CCMD     0x03 /*Control Command*/
    
    // MSGTYP_SDMSG    0x04 /*Set Display Message*/
    inline Net485Packet *setDisplayMessage(Net485Packet *_pkt, const char *_message = "",uint8_t _nodeTypeOrigin = 0x00, Net485Routing _route = {SNDMTHD_NOROUTE, 0x00, 0x00, 0x00}) {
#define MSGTYP_SDMSG_MAX_STRLEN 30
        _pkt->header()[HeaderStructureE::HeaderDestAddr] = 0x00; /* Set by Net485Routing */
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = this->nodeId;
        _pkt->header()[HeaderStructureE::HeaderSubnet] = this->subNet;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = _route.SendMethod;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = _route.SendParam;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = _route.SendParam1;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = (_route.SourceNode > 0x00 ? _route.SourceNode : net485dl->getNodeType());
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_SDMSG;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(false,false);
        _pkt->data()[0] = (_nodeTypeOrigin > 0x00 ? _nodeTypeOrigin : net485dl->getNodeType());
        _pkt->data()[1] = min( strlen(_message), MSGTYP_SDMSG_MAX_STRLEN );
        memcpy((void *)&(_pkt->data()[2]), _message, _pkt->data()[1] );
        _pkt->header()[HeaderStructureE::PacketLength] = _pkt->data()[1] + 2;
        return _pkt;
    }
    inline Net485Packet *setDisplayMessageACK(Net485Packet *_pkt) {
#define MAX_STRLEN 30
        _pkt->header()[HeaderStructureE::HeaderSrcAddr] = this->nodeId;
        _pkt->header()[HeaderStructureE::HeaderSndMethd] = SNDMTHD_NOROUTE;
        _pkt->header()[HeaderStructureE::HeaderSndParam] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSndParam1] = 0x00;
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGRESP(MSGTYP_SDMSG);
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(false,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 2;
        _pkt->data()[0] = 0xAC;
        _pkt->data()[1] = 0x06;
        return _pkt;
    }
};

#endif
