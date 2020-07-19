#include "Net485Subord_APP.hpp"
#include "Net485DataLink.hpp"

Net485Subord_APP::Net485Subord_APP(HardwareSerial *_hwSerial) {
    this->hwSerial = _hwSerial;
    this->incomingPktWaiting = false;
}
Net485Subord_APP::~Net485Subord_APP() {
}
bool Net485Subord_APP::readPkt(int _waitms) {
    bool pktReady = false;
    size_t bytes = hwSerial->available();

    if(bytes >= MTU_HEADER + MTU_CHECKSUM) {
        bytes = hwSerial->readBytes(incomingPkt.buffer, bytes);
        pktReady = (bytes >= MTU_HEADER + MTU_CHECKSUM);
        // check header if packet complete
        pktReady &= ( incomingPkt.header()[HeaderStructureE::PacketLength] == (bytes - (MTU_HEADER + MTU_CHECKSUM)) );
        // check CRC
        pktReady &= Net485DataLink::isChecksumValid(&incomingPkt);

        incomingPkt.dataSize += bytes - MTU_HEADER + MTU_CHECKSUM; // Set dataSize to payload byte count
    }

    return pktReady;
}
void Net485Subord_APP::send(Net485Packet *packet) {
    Net485DataLink::calculateChecksum(packet);
    this->hwSerial->write(packet->buffer,MTU_HEADER + packet->dataSize + MTU_CHECKSUM);
};
bool Net485Subord_APP::hasPacket(unsigned long *millisLastRead) {
    if(!incomingPktWaiting) {
        incomingPktWaiting = readPkt();
    }
    return incomingPktWaiting;
};
Net485Packet *Net485Subord_APP::getNextPacket(bool peek) {
    return &incomingPkt;
};
