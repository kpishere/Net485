//
//  Test_HardwareSerial.m
//  Test_HardwareSerial
//
//  Created by Kevin Peck on 2020-07-15.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "HardwareSerial.hpp"
#include "Net485DataLink.hpp"
#include "Net485Network.hpp"
#include "Net485Subord_APP.hpp"

const char *echoDevice = "/dev/ttys000";
const int  baudRate = 9600;

// Linked
const char *net485Device1 = "/dev/ttys010";
const char *net485Device2 = "/dev/ttys020";
// Linked
const char *appDevice1 = "/dev/ttys030";
const char *appDevice2 = "/dev/ttys040";

const uint8_t ver = 1, rev = 4;
const int secondsToRun = 60;

@interface Test_HardwareSerial : XCTestCase
@end

@implementation Test_HardwareSerial

/*  // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
 */

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)testHardwareSerial_ReadWriteEcho {
    HardwareSerial *hw = new HardwareSerial(echoDevice, baudRate);
    uint8_t bufferSent[] = {0xBE, 0xEF, 0xFA, 0xCE};
    uint8_t bufferRead[] = {0x00, 0x00, 0x00, 0x00};
    int bytesRead = 0;
    size_t inReadBuf;
    
    hw->write((uint8_t *)&bufferSent, sizeof(bufferSent));
    
    while( bytesRead < sizeof(bufferSent)) {
        inReadBuf = hw->available();
        if(inReadBuf>0) {
            inReadBuf = hw->readBytes((uint8_t *)(&bufferRead + bytesRead), inReadBuf);
            bytesRead += inReadBuf;
        }
    }
    
    XCTAssertEqual( sizeof(bufferSent), bytesRead, "Sent/Read bytes do not equal");
    for(int i=0; i< sizeof(bufferSent); i++) {
        XCTAssertEqual( bufferSent[i], bufferRead[i], "Sent/Read bytes do not match");
    }
}

- (void)testNet485DataLink_Start {
    CFAbsoluteTime timeInSeconds = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime timeNext = CFAbsoluteTimeGetCurrent();
    HardwareSerial *hw = new HardwareSerial(net485Device1, baudRate);
    HardwareSerial *app = new HardwareSerial(appDevice1, baudRate);
    Net485Subord *devApp;
    Net485DataLink *device485;
    Net485Network *net485;

    device485 = new Net485DataLink(hw, NTC_FURNGAS, 0, 0, baudRate );
    Net485MacAddress m = device485->getMacAddr();
    NSLog(@"Mac %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x" ,m.mac[0],m.mac[1],m.mac[2],m.mac[3],m.mac[4],m.mac[5],m.mac[6],m.mac[7]);
    devApp = new Net485Subord_APP(app);
    net485 = new Net485Network(device485, devApp, true, ver, rev, Net485Diagnostic::INFO_LOW);
    
    while( (timeNext - timeInSeconds) < secondsToRun ) {
        net485->loop();
        timeNext = CFAbsoluteTimeGetCurrent();
    }
}

@end
