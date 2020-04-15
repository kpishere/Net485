/*
 *  A test scrpt that runs on either AVR (Mega 2650)
 * 
 * AVR : Serial 0 is used for code upload, debugging, and control.  
 *  Serial 2 is used for RS485 communication
 *
 */
#if defined(__AVR__)
#include <Arduino.h>
#elif defined(ESP8266)
#endif

#include "src/Net485Network.hpp"
#include "src/Net485Subord_Echo.hpp"

#define FW_NAME "net485gw"
#define FW_VERSION __DATE__

#define NETVER 1
#define NETREV 4

Net485DataLink *device485;
Net485Network *net485;
Net485Subord *devApp;
unsigned long lastUpdate = 0;
uint8_t msglen, loops, ver = NETVER, rev = NETREV;
uint16_t nextPktDelay = 250; // mseconds
bool master;
Net485Packet sendPacket;
Net485Packet *recvPtr;

char messageBuffer[160];

#if defined(__AVR__)
void loopCommandSerial() {
  int bytesReady = Serial.available();
  char nextChar;
  
  if(bytesReady > 0) {
    Serial.readBytes(&nextChar,1);

    switch(nextChar) {
      case 'm':
        master = true;
        Serial.println("{master: true}");
        break;
      case 's':
        master = false;
        Serial.println("{master: false}");
        break;
      default:
        if(nextChar >= '0' && nextChar <= '9') {
          loops = nextChar - '0';
          sprintf(messageBuffer,"{loops: %d}\n", loops);
          Serial.print(messageBuffer);
        }
        if(nextChar >= 'A' && nextChar <= 'Y') {
          msglen = 10 * (nextChar - 'A');
          sprintf(messageBuffer,"{msglen: %d}\n", msglen);
          Serial.print(messageBuffer);
        }
        break;
    }    
  }
}
#elif defined(ESP8266)
#endif

void setMessageValues(Net485Packet *_pkt, uint8_t _msglen) {
  _pkt->dataSize = _msglen;
  device485->setPacketHeader(_pkt,0x00, 0x00, 0x03, 0x01, 0x0100, 0x01, 0x01, 0x00, _msglen);
  // values for body
  for(int i=0; i<_msglen; i++) _pkt->data()[i] = _msglen;
  // dummy checksum
  _pkt->checksum()[0] = 0xA0;
  _pkt->checksum()[1] = 0x0A;
}

void setup() {  
#if defined(__AVR__)
  Serial.begin(115200);
  Serial2.begin(DATARATE_BPS_DEF);
  Serial.println(); Serial.println();
#elif defined(ESP8266)
#endif  
  
  Serial.println("Hardware start now");
  device485 = new Net485DataLink(&Serial2, NTC_FURNGAS );
  devApp = new Net485Subord_Echo();
  net485 = new Net485Network(device485, devApp, true, ver, rev);
  Serial.println("Hardware stop now");
}

void loop() {  
#if defined(__AVR__)
  unsigned long thisUpdate = millis();

  loopCommandSerial();

  net485->loop();

#elif defined(ESP8266)
#endif
}
