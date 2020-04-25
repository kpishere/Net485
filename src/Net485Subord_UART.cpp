#include "Net485Subord_UART.hpp"
#include "Net485DataLink.hpp"

Net485Subord_UART::Net485Subord_UART(HardwareSerial *_hwSerial) {
    this->hwSerial = _hwSerial;
    this->incomingPktWaiting = false;
}
Net485Subord_UART::~Net485Subord_UART() {
}
bool Net485Subord_UART::readPkt(int _waitms) {
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
void Net485Subord_UART::send(Net485Packet *packet) {
    this->hwSerial->write(packet->buffer,MTU_HEADER + packet->dataSize + MTU_CHECKSUM);
};
bool Net485Subord_UART::hasPacket(unsigned long *millisLastRead) {
    if(!incomingPktWaiting) {
        incomingPktWaiting = readPkt();
    }
    return incomingPktWaiting;
};
Net485Packet *Net485Subord_UART::getNextPacket(bool peek) {
    return &incomingPkt;
};
