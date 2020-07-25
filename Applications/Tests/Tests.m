//
//  Tests.m
//  Tests
//
//  Created by Kevin Peck on 2020-07-25.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//

#import <XCTest/XCTest.h>

@interface Tests : XCTestCase

@end

@implementation Tests

- (void)setUp {
/*
## PRE_ACTION
# You'll have to do this to allow test to run :
# sudo chmod o+rw /dev
#
socat pty,raw,nonblock,ispeed=9600,echo=0,wait-slave,link=/dev/ttys001 pty,raw,nonblock,ispeed=9600,echo=0,wait-slave,link=/dev/ttys010
echo $! > /tmp/socat_000.pid
echo "${PROJECT_DIR}/bin/net485Furnace"
*/

}

- (void)tearDown {
/*
PID=$(</tmp/socat_000.pid)
kill -9 $PID
rm /dev/ttys00* /dev/ttys01* /dev/ttys02* /dev/ttys03* /dev/ttys04*
*/}

- (void)testExample {
    // This is an example of a functional test case.
    // Use XCTAssert and related functions to verify your tests produce the correct results.
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
