/*
 *  A test scrpt that runs on either AVR (Mega 2650)
 * 
 * AVR : 
 *  Serial 0 is used for code upload and communication with virtual device  
 *  Serial 2 is used for RS485 communication
 *
 */
#if defined(__AVR__)
#include <Arduino.h>
#elif defined(ESP8266)
#endif

#include "src/Net485Network.hpp"
#include "src/Net485Subord_UART.hpp"

#define FW_NAME "net485gw"
#define FW_VERSION __DATE__

#define NETVER 1
#define NETREV 4

uint8_t ver = NETVER, rev = NETREV;
Net485DataLink *device485;
Net485Network *net485;
Net485Subord *devApp;

void setup() {  
#if defined(__AVR__)
  Serial.begin(DATARATE_BPS_DEF);
  Serial2.begin(DATARATE_BPS_DEF);
#elif defined(ESP8266)
#endif  
  
  device485 = new Net485DataLink(&Serial2, NTC_THERM );
  devApp = new Net485Subord_UART(&Serial);
  net485 = new Net485Network(device485, devApp, true, ver, rev, Net485Diagnostic::INFO_LOW);
}

void loop() {  
#if defined(__AVR__)
  net485->loop();
#elif defined(ESP8266)
#endif
}
