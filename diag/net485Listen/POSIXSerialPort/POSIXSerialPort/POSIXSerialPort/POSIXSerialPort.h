//
//  POSIXSerialPort.h
//  POSIXSerialPort
//
//  Created by kpishere on 2020-07-17.
//  Copyright © 2020  GPL-2.0 License
//
#import <Foundation/Foundation.h>

@protocol POSIXSerialPortDelegate;

@class POSIXSerialRequest;
@class POSIXSerialPacketDescriptor;

//#define LOG_SERIAL_PORT_ERRORS
typedef NS_ENUM(NSUInteger, POSIXSerialPortParity) {
    POSIXSerialPortParityNone = 0,
    POSIXSerialPortParityOdd,
    POSIXSerialPortParityEven
};

typedef struct DataSegmentS {
    size_t offset;
    size_t size;
} DataSegment;

#define DEFAULT_READ_CHUNK 1024
#define DEFAULT_READ_BLOCKTIME_MS 10

@interface POSIXSerialPort : NSObject
+ (nullable POSIXSerialPort *)serialPortWithPath:(NSString *)devicePath baudRate:(int)br;
+ (nullable POSIXSerialPort *)serialPortWithPath:(NSString *)devicePath baudRate:(int)br readChunk:(size_t)rc readBlockMs:(float)rbms;
- (nullable instancetype)initWithPath:(NSString *)devicePath;
- (void)open;
- (void)close;
- (void)sendData:(NSData *)data;
@property (nonatomic, weak, nullable) id<POSIXSerialPortDelegate> delegate;
@property (readonly, getter = isOpen) BOOL open;
@property (copy, readonly) NSString *path;
@end

@protocol POSIXSerialPortDelegate <NSObject>
@required
- (void)serialPort:(POSIXSerialPort *)serialPort didReceiveData:(NSData *)data;
@optional
- (DataSegment)serialPort:(POSIXSerialPort *)serialPort nextDataSegmentValidIn:(NSData *)data;
- (void)serialPort:(POSIXSerialPort *)serialPort didEncounterError:(NSError *)error;
- (void)serialPortWasOpened:(POSIXSerialPort *)serialPort;
- (void)serialPortWasClosed:(POSIXSerialPort *)serialPort;
@end
