//
//  main.h
//  Applications
//
//  Created by Kevin Peck on 2020-07-25.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//
//  net485Coodinator - A utillity command line tool to configure the
//      CoordinatorNet485.a library for execution on two serial ports.
//

#import <Foundation/Foundation.h>

#include "HardwareSerial.hpp"
#include "Net485DataLink.hpp"
#include "Net485Network.hpp"
#include "Net485Subord_APP.hpp"

const int  baudRate = 9600;
const uint8_t ver = 1, rev = 4;
const int secondsToRun = 60 * 60 * 24; // a day

#define ARGV_NET485 1
#define ARGV_APP 2
#define ARGV_Net485API_NODETYPE 3

NS_ASSUME_NONNULL_BEGIN

int main (int argc, const char * _Nonnull argv[]) {
    CFAbsoluteTime timeInSeconds = CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime timeNext = CFAbsoluteTimeGetCurrent();

    @autoreleasepool {
        if(argc >= 3) {
            NSLog(@"Net %s app %s nodeType %s", argv[ARGV_NET485], argv[ARGV_APP], argv[ARGV_Net485API_NODETYPE]);
            
            HardwareSerial *net = new HardwareSerial(argv[ARGV_NET485], baudRate);
            HardwareSerial *app = new HardwareSerial(argv[ARGV_APP], baudRate);
            uint8_t nodeType = atoi( argv[ARGV_Net485API_NODETYPE] );

            // Cnxn to Application
            Net485Subord *devApp = new Net485Subord_APP(app);
            
            // Cnxn to network with identity of application type
            Net485DataLink *device485 = new Net485DataLink(net, nodeType, 0, 0, baudRate );
            
            // Get and log the devices MAC address
            Net485MacAddress m = device485->getMacAddr();
            NSLog(@"Mac %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x" ,m.mac[0],m.mac[1],m.mac[2],m.mac[3],m.mac[4],m.mac[5],m.mac[6],m.mac[7]);

            Net485Network *net485 = new Net485Network(device485, devApp, true, ver, rev, Net485Diagnostic::INFO_LOW);

            while( (timeNext - timeInSeconds) < secondsToRun ) {
                usleep( MICROSECONDS_IN_SECOND / LOOP_RATE_PER_SECOND);
                
                net485->loop();
                timeNext = CFAbsoluteTimeGetCurrent();
            }
        } else { // Argc < 2
            NSLog(@"Usage of `%s <network serial port> <serial port to application> <node type code>`", argv[0]);
        }
    }
   return 0;
}

NS_ASSUME_NONNULL_END
