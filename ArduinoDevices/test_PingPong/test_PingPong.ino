/*
 *  A test scrpt that runs on either AVR (Mega 2650) or ESP8266
 *
 * ESP8266: Serial 0 is used for code upload and RS485 communication.  Homie provides WiFi debug and control access via MQTT
 * 
 * AVR : Serial 0 is used for code upload, debugging, and control.  Serial 1 is used for RS485 communication
 *
 */
#if defined(__AVR__)
#include <Arduino.h>
#elif defined(ESP8266)
#include <Homie.h>
#endif

#include "src/Net485Physical_HardwareSerial.hpp"

#define FW_NAME "net485gw"
#define FW_VERSION __DATE__

#define PKT_STACKSIZE 5
Net485Physical_HardwareSerial *device485;
unsigned long lastUpdate = 0;
uint8_t msglen, loops;
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
HomieNode controlNode0("net485test", "net485");

void setupHandler() {
    controlNode0.setProperty("msglen").send("0");      
    controlNode0.setProperty("loops").send("0");
    controlNode0.setProperty("master").send("0");
    controlNode0.setProperty("packet").send("");
}

void loopHandler() {
  unsigned long thisUpdate = millis();

  if (device485->hasPacket()) {
    /*
    sprintf(messageBuffer,"{havemsg}");
    controlNode0.setProperty("packet").send(messageBuffer);

    // Log msg over wifi
    recvPtr = device485->getNextPacket();  
*/
    if(!master) { // slave sends echo back
      memcpy(&sendPacket,recvPtr,sizeof(Net485Packet));
      device485->send(&sendPacket);
    }
  /*    
    sprintf(messageBuffer,"{header: 0x");
    for(int i=0; i<MTU_HEADER ; i++) sprintf( &(messageBuffer[11+i*2]), "%0X ",recvPtr->header()[i]);
    sprintf(&(messageBuffer[11+2*MTU_HEADER]),", data: %0X .. (%d), chksum: 0x%0X%0X }", recvPtr->data()[0], recvPtr->dataSize
      ,recvPtr->checksum()[0], recvPtr->checksum()[1]);
    controlNode0.setProperty("packet").send(messageBuffer);    
    */
    lastUpdate = thisUpdate;
  }

  if(master && loops > 0 && device485->readyToSend()) {
    // Log send over wifi
    sprintf(messageBuffer,"{master: %d, loops: %d, msglen: %d}", master,loops, msglen);
    controlNode0.setProperty("packet").send(messageBuffer);
    setMessageValues(&sendPacket, msglen);
    // send over device
    device485->send(&sendPacket);
    loops--;
  }    
}
bool propCtrlHandler_msglen(const HomieRange& range, const String& value) {
  msglen = value.toInt();
  controlNode0.setProperty("msglen").send(value);      
  return true;
}
bool propCtrlHandler_loops(const HomieRange& range, const String& value) {
  loops = value.toInt();
  controlNode0.setProperty("loops").send(value);
  return true;
}
bool propCtrlHandler_master(const HomieRange& range, const String& value) {
  master = (value.toInt() > 0);
  controlNode0.setProperty("master").send(value);
  return true;
}
#endif

void setMessageValues(Net485Packet *_pkt, uint8_t _msglen) {
  _pkt->dataSize = _msglen;
  // dummy values for header
  for(int i=0; i<MTU_HEADER; i++) _pkt->header()[i] = i;
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
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  Homie.disableLogging();   

  controlNode0.advertise("msglen").settable(propCtrlHandler_msglen);
  controlNode0.advertise("loops").settable(propCtrlHandler_loops); 
  controlNode0.advertise("master").settable(propCtrlHandler_master);
  controlNode0.advertise("packet");iuhˆˆhuj 
  Homie.setup();
#endif  
  
  Serial.println("Hardware start now");
  device485 = new Net485Physical_HardwareSerial(PKT_STACKSIZE, &Serial2);
  Serial.println("Hardware stop now");
}

void loop() {  
#if defined(__AVR__)
  unsigned long thisUpdate = millis();

  loopCommandSerial();

  if (device485->hasPacket()) {
    Serial.println("{havemsg}");

    // Log msg over wifi
    recvPtr = device485->getNextPacket();  

    if(!master) { // slave sends echo back
      memcpy(&sendPacket,recvPtr,sizeof(Net485Packet));
      device485->send(&sendPacket);
    }

    sprintf(messageBuffer,"{header: 0x");
    for(int i=0; i<MTU_HEADER ; i++) sprintf( &(messageBuffer[11+i*2]), "%0X ",recvPtr->header()[i]);
    sprintf(&(messageBuffer[11+2*MTU_HEADER]),", data: %0X .. (%d), chksum: 0x%0X%0X }", recvPtr->data()[0], recvPtr->dataSize
      ,recvPtr->checksum()[0], recvPtr->checksum()[1]);
    Serial.println(messageBuffer);    
  }

  if(master && loops > 0 && device485->readyToSend() 
  && thisUpdate - lastUpdate > nextPktDelay ) {
    // Log send 
    sprintf(messageBuffer,"{master: %d, loops: %d, msglen: %d}", master,loops, msglen);
    Serial.println(messageBuffer);
    setMessageValues(&sendPacket, msglen);
    // send over device
    device485->send(&sendPacket);
    loops--;
    lastUpdate = thisUpdate;    
  }    
#elif defined(ESP8266)
  Homie.loop();
#endif
}
