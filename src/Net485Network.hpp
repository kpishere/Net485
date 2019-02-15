/*
 *  Net485Network.hpp
 *  Net485
 *
 */
#ifndef Net485Network_hpp
#define Net485Network_hpp

#include "Net485DataLink.hpp"
#include "Net485API.hpp"

/*
enum Address485Purpose {
	Broadcast, 
	Subnet_2_Subordinate,
	Reserved,
	Subnet_3_Subordinate,
	Restricted,
	Arbitration,
	Coordinator,
	COUNT
};
enum SubNet485Purpose {
	Broadcast, 
	Reserved,
	COUNT
};
enum Send485Method {
    NonRouted = 0x00, PriorityRouted = 0x01, NodeTypeRouted 0x02, SocketRouted 0x03
}
*/


class Net485Network {
private:
    Net485DataLink *net485dl;
    uint64_t sessionId;
    bool coordinatorCapable;
public:
    Net485Network(Net485DataLink *_net, bool _coordinatorCapable = true);
    ~Net485Network();
    
    inline Net485Packet *setR2R(Net485Packet *_pkt) {
        _pkt->header()[HeaderStructureE::HeaderSrcNodeType] = net485dl->getNodeType();
        _pkt->header()[HeaderStructureE::PacketMsgType] = MSGTYP_R2R;
        _pkt->header()[HeaderStructureE::PacketNumber] = PKTNUMBER(true,false);
        _pkt->header()[HeaderStructureE::PacketLength] = 17;
        _pkt->data()[0] = 0x00;
        memcpy((void *)&(_pkt->data()[1]), net485dl->getMacAddr().mac, Net485MacAddressE::SIZE);
        memcpy((void *)&(_pkt->data()[1+Net485MacAddressE::SIZE])
               , &sessionId, sizeof(uint64_t));
        return _pkt;
    }
};

#endif
