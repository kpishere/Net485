//
//  POSIXSerialPortTests.m
//  POSIXSerialPortTests
//
//  Created by Kevin Peck on 2020-07-17.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "../POSIXSerial.h"

#define PORT_PATH @"/dev/ttys001"
#define BAUD_RATE 9600

@class POSIXSerialPortDelegate_TEST;

@interface POSIXSerialPortTests : XCTestCase
@property POSIXSerialPort *port;
@property (nonatomic, strong) POSIXSerialPortDelegate_TEST *pDelegate;
@end

@interface POSIXSerialPortDelegate_TEST : NSObject  <NSObject,POSIXSerialPortDelegate>
- (void)serialPort:(POSIXSerialPort *)serialPort didReceiveData:(NSData *)data;
- (DataSegment)serialPort:(POSIXSerialPort *)serialPort nextDataSegmentValidIn:(NSData *)data;
- (void)serialPort:(POSIXSerialPort *)serialPort didEncounterError:(NSError *)error;
- (void)serialPortWasOpened:(POSIXSerialPort *)serialPort;
- (void)serialPortWasClosed:(POSIXSerialPort *)serialPort;

@property (nullable,weak) XCTestExpectation *portOpen;
@property (nullable,weak) XCTestExpectation *readWriteTestComplete;
@end

@implementation POSIXSerialPortDelegate_TEST
- (void)serialPort:(POSIXSerialPort *)serialPort didReceiveData:(NSData *)data;
{
    NSLog(@"serialPort didReceiveData (%s): %@", [[serialPort description] UTF8String]
        , data );
    [self.readWriteTestComplete fulfill];
}
- (DataSegment)serialPort:(POSIXSerialPort *)serialPort nextDataSegmentValidIn:(NSData *)data;
{
    DataSegment ds = {0, [data length]};
    NSLog(@"serialPort nextDataSegmentValidIn (%s): %zu, %zu", [[serialPort description] UTF8String]
    , ds.offset, ds.size );
    return ds;
}
- (void)serialPort:(POSIXSerialPort *)serialPort didEncounterError:(NSError *)error;
{
    NSLog(@"serialPort didEncounterError (%s): %s", [[serialPort description] UTF8String]
        , [[error description] UTF8String] );
}
- (void)serialPortWasOpened:(POSIXSerialPort *)serialPort;
{
    NSLog(@"serialPortWasOpened (%s)", [[serialPort description] UTF8String] );
    [self.portOpen fulfill];
}
- (void)serialPortWasClosed:(POSIXSerialPort *)serialPort;
{
    NSLog(@"serialPortWasClosed (%s)", [[serialPort description] UTF8String] );
}
@end

@implementation POSIXSerialPortTests

- (void)setUp {
/*
    ## PRE_ACTION
    # You'll have to do this to allow test to run :
    # sudo chmod o+rw /dev
    #
    socat pty,raw,nonblock,ispeed=115200,echo=0,link=/dev/ttys000 EXEC:/bin/cat &
    echo $! > /tmp/socat_000.pid
*/
    self.port = [POSIXSerialPort serialPortWithPath:PORT_PATH baudRate:BAUD_RATE];
    self.pDelegate = [POSIXSerialPortDelegate_TEST alloc];
    self.port.delegate = self.pDelegate;
    self.pDelegate.portOpen = [self expectationWithDescription:@"Port open"];
    self.pDelegate.readWriteTestComplete = [self expectationWithDescription:@"Read complete"];
}

- (void)tearDown {
/*
    ## POST_ACTION
    #
    PID=$(</tmp/socat_000.pid)
    kill -9 $PID
    rm /dev/ttys00* /dev/ttys01* /dev/ttys02* /dev/ttys03* /dev/ttys04*
*/
}

// Test allocation/deallocation of port/channel opening
- (void)test_openClose {
    NSAssert( [self.port isOpen] == NO, @"IsOpen falsely returns YES");
    [self.port open];
    NSAssert( [self.port isOpen] == YES, @"IsOpen falsely returns NO");
    [self.port close];
    NSAssert( [self.port isOpen] == NO, @"IsOpen falsely returns YES after Open/Close");
    [self.port open];
    NSAssert( [self.port isOpen] == YES, @"IsOpen falsely returns NO after Open/Close/Open");
    [self.port close];
    NSAssert( [self.port isOpen] == NO, @"IsOpen falsely returns YES after Open/Close/Open/Close");
}

- (void)test_writeRead {
    NSData *sendData = [@"FEED BEEF" dataUsingEncoding:NSUTF8StringEncoding];
    
    [self.port open];
    [self waitForExpectations:@[self.pDelegate.portOpen] timeout:5];

    [self.port sendData:sendData];
    [self waitForExpectations:@[self.pDelegate.readWriteTestComplete] timeout:10];

    [self.port close];
}

@end
