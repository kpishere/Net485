//
//  POSIXSerialPort.m
//  POSIXSerialPort
//
//  Created by kpishere on 2020-07-17.
//  Copyright © 2020  GPL-2.0 License
//
#import <Foundation/Foundation.h>
#import "POSIXSerialPort.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/syslimits.h>
#include <dispatch/dispatch.h>

static __strong NSMutableArray *allSerialPorts;

@interface POSIXSerialPort ()
{
    struct termios originalPortAttributes;
}
@property (copy, readwrite) NSString *path;
@property dispatch_fd_t fileDescriptor;
@property size_t readChunk;
@property float readBlockMs;
@property dispatch_data_t _data;
@property int baudRate;
@property (nonatomic) BOOL allowsNonStandardBaudRates;
@property (nonatomic) NSUInteger numberOfStopBits;
@property (nonatomic) NSUInteger numberOfDataBits;
@property (nonatomic) BOOL shouldEchoReceivedData;
@property (nonatomic) POSIXSerialPortParity parity;
@property (nonatomic) BOOL usesRTSCTSFlowControl;
@property (nonatomic) BOOL usesDTRDSRFlowControl;
@property (nonatomic) BOOL usesDCDOutputFlowControl;
@property (nonatomic) BOOL RTS;
@property (nonatomic) BOOL DTR;

#if OS_OBJECT_USE_OBJC
@property (nonatomic, strong) dispatch_queue_t _posixSerialPortQueue;
@property (nonatomic, strong) dispatch_source_t _deviceSource;
#else
@property (nonatomic) dispatch_queue_t _posixSerialPortQueue;
@property (nonatomic) dispatch_source_t _deviceSource;
#endif

@end

@implementation POSIXSerialPort

+ (void)initialize
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        allSerialPorts = [[NSMutableArray alloc] init];
    });
}

+ (void)addSerialPort:(POSIXSerialPort *)port;
{
    [allSerialPorts addObject:[NSValue valueWithNonretainedObject:port]];
}

+ (void)removeSerialPort:(POSIXSerialPort *)port;
{
    NSValue *valueToRemove = nil;
    for (NSValue *value in allSerialPorts)
    {
        if ([value nonretainedObjectValue] == port)
        {
            valueToRemove = value;
            break;
        }
    }
    if (valueToRemove) [allSerialPorts removeObject:valueToRemove];
}

+ (POSIXSerialPort *)existingPortWithPath:(NSString *)path;
{
    POSIXSerialPort *existingPort = nil;
    for (NSValue *value in allSerialPorts)
    {
        POSIXSerialPort *port = [value nonretainedObjectValue];
        if ([port.path isEqualToString:path])
        {
            existingPort = port;
            break;
        }
    }
    
    return existingPort;
}

+ (POSIXSerialPort *)serialPortWithPath:(NSString *)devicePath baudRate:(int)br
{
    return [[self class] serialPortWithPath:devicePath baudRate:br readChunk:DEFAULT_READ_CHUNK readBlockMs:DEFAULT_READ_BLOCKTIME_MS];
}

+ (POSIXSerialPort *)serialPortWithPath:(NSString *)devicePath baudRate:(int)br readChunk:(size_t)rc readBlockMs:(float)rbms
{
    POSIXSerialPort *o = [self alloc];
    o.baudRate = br;
    o.readChunk = rc;
    o.readBlockMs = rbms;
    return [o initWithPath:devicePath];
}

+ (int)deviceFromBSDPath:(NSString *)bsdPath;
{
    dispatch_fd_t fd  = open([bsdPath cStringUsingEncoding:NSASCIIStringEncoding], O_RDWR | O_NOCTTY | O_EXLOCK | O_NONBLOCK);
    if (fd == -1)
    {
        NSLog(@"Error opening port (%s):%i", [bsdPath UTF8String], errno);
    }
    return fd;
}

- (instancetype)init
{
    NSAssert(0, @"POSIXSerialPort must be init'd using -initWithPath:");
    self = [self initWithPath:@""]; // To keep compiler happy.
    return self;
}

- (void)dealloc
{
    [[self class] removeSerialPort:self];
}

- (instancetype)initWithPath:(NSString *)devicePath
{
    POSIXSerialPort *existingPort = [[self class] existingPortWithPath:devicePath];
    
    if (existingPort != nil)
    {
        self = nil;
        return existingPort;
    }

    int device = [[self class] deviceFromBSDPath:devicePath];
    if (device < 0) return nil;
    
    self.path = devicePath;
    self.fileDescriptor = device;

    self = [super init];
    if (self != nil)
    {
        self.numberOfDataBits = 8;
        self.parity = POSIXSerialPortParityNone;
        self.shouldEchoReceivedData = NO;
        self.usesRTSCTSFlowControl = NO;
        self.usesDTRDSRFlowControl = NO;
        self.usesDCDOutputFlowControl = NO;
        self.RTS = NO;
        self.DTR = NO;
        self.allowsNonStandardBaudRates = NO;
    }
    self._posixSerialPortQueue = nil;

    [[self class] addSerialPort:self];
    return self;
}

- (NSString *)description
{
    return self.path;
}

- (NSUInteger)hash
{
    return [self.path hash];
}

- (BOOL)isEqual:(id)object
{
    if (![object isKindOfClass:[self class]]) return NO;
    if (![object respondsToSelector:@selector(path)]) return NO;
    
    return [self.path isEqual:[object path]];
}

#pragma mark - Public Methods

- (void)open;
{
    int result = 0;
    dispatch_queue_t mainQueue = dispatch_get_main_queue();

    if ( self._posixSerialPortQueue != nil) return;

    // allow cycles of open/close on device
    if(self.fileDescriptor < 0) {
        self.fileDescriptor = [[self class] deviceFromBSDPath:self.path];
        if(self.fileDescriptor < 0) return;
    }
    
    // Flush away any bytes previously read or written.
    result = tcflush(self.fileDescriptor, TCIOFLUSH);
    if (result)
    {
        [self notifyDelegateOfPosixError]; // Warning only
    }
    /*
    result = grantpt(self.fileDescriptor);
    if(result) {
        [self notifyDelegateOfPosixError];
    }
    result = unlockpt(self.fileDescriptor);
    if(result) {
        [self notifyDelegateOfPosixError];
    }
    */
    // Port opened successfully, set options
    tcgetattr(self.fileDescriptor, &originalPortAttributes); // Get original options so they can be reset later
    
    struct termios options;
    if (tcgetattr(self.fileDescriptor, &options))
    {
        [self notifyDelegateOfPosixError];
        return;
    }
    
    // Turn off any options that might interfere with our ability to send and
    // receive raw binary bytes. VMIN & VTIME settubgs are controlled further down by dispatch_io_...
    cfmakeraw(&options);
    options.c_cc[VMIN] = 0; // Wait for at least N character(s) before returning
    options.c_cc[VTIME] = 0; // Wait N * 100 milliseconds between bytes before returning from read
    
    // Set 8 data bits
    options.c_cflag &= ~CSIZE;
    switch (self.numberOfDataBits) {
        case 5:
            options.c_cflag |= CS5;
            break;
        case 6:
            options.c_cflag |= CS5;
            break;
        case 7:
            options.c_cflag |= CS7;
            break;
        case 8:
            options.c_cflag |= CS8;
            break;
        default:
            break;
    }
    
    // Set parity
    switch (self.parity) {
        case POSIXSerialPortParityNone:
            options.c_cflag &= ~PARENB;
            break;
        case POSIXSerialPortParityEven:
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            break;
        case POSIXSerialPortParityOdd:
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            break;
        default:
            break;
    }
    
    options.c_cflag = [self numberOfStopBits] > 1 ? options.c_cflag | CSTOPB : options.c_cflag & ~CSTOPB; // number of stop bits
    options.c_lflag = [self shouldEchoReceivedData] ? options.c_lflag | ECHO : options.c_lflag & ~ECHO; // echo
    options.c_cflag = [self usesRTSCTSFlowControl] ? options.c_cflag | CRTSCTS : options.c_cflag & ~CRTSCTS; // RTS/CTS Flow Control
    options.c_cflag = [self usesDTRDSRFlowControl] ? options.c_cflag | (CDTR_IFLOW | CDSR_OFLOW) : options.c_cflag & ~(CDTR_IFLOW | CDSR_OFLOW); // DTR/DSR Flow Control
    options.c_cflag = [self usesDCDOutputFlowControl] ? options.c_cflag | CCAR_OFLOW : options.c_cflag & ~CCAR_OFLOW; // DCD Flow Control
    
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_cflag |= HUPCL; // Turn on hangup on close
    options.c_cflag |= CREAD; // Enable receiver

    // Set baud rate
    switch (self.baudRate)
    {
        case 4800:   cfsetospeed(&options, B4800);   break;
        case 9600:   cfsetospeed(&options, B9600);   break;
        case 19200:   cfsetospeed(&options, B19200);   break;
        case 38400:   cfsetospeed(&options, B38400);   break;
        case 115200: cfsetospeed(&options, B115200); break;
        default:
            cfsetspeed(&options, self.baudRate);
            break;
    }
    cfsetispeed(&options, cfgetospeed(&options));
    
    if (tcsetattr(self.fileDescriptor, TCSANOW, &options)) {
        if (self.allowsNonStandardBaudRates) {
            // Not supported, needs IOKit <serial/ioss.h>
            // Try to set baud rate via ioctl if normal port settings fail
            //int new_baud = self.baudRate;
            //result = ioctl(self.fileDescriptor, IOSSIOSPEED, &new_baud, 1);
        }
        // Notify delegate of port error stored in errno
        [self notifyDelegateOfPosixError];
    }
    
    // Set up read/write dispatch queue
    self._posixSerialPortQueue = dispatch_queue_create([[self description] UTF8String], DISPATCH_QUEUE_SERIAL);
    self._deviceSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, self.fileDescriptor, 0, self._posixSerialPortQueue);
    dispatch_source_set_cancel_handler(self._deviceSource, ^(void){
        // The next tcsetattr() call can fail if the port is waiting to send data. This is likely to happen
        // e.g. if flow control is on and the CTS line is low. So, turn off flow control before proceeding
        struct termios options;
        tcgetattr(self.fileDescriptor, &options);
        options.c_cflag &= ~CRTSCTS; // RTS/CTS Flow Control
        options.c_cflag &= ~(CDTR_IFLOW | CDSR_OFLOW); // DTR/DSR Flow Control
        options.c_cflag &= ~CCAR_OFLOW; // DCD Flow Control
        tcsetattr(self.fileDescriptor, TCSANOW, &options);
        
        // Set port back the way it was before we used it
        tcsetattr(self.fileDescriptor, TCSADRAIN, &self->originalPortAttributes);

        if (close(self.fileDescriptor))
        {
            NSLog(@"Error closing serial port with file descriptor %i:%i", self.fileDescriptor, errno);
            [self notifyDelegateOfPosixError];
        }
        self.fileDescriptor = -1;
        if ([self.delegate respondsToSelector:@selector(serialPortWasClosed:)])
        {
            [(id)self.delegate performSelectorOnMainThread:@selector(serialPortWasClosed:) withObject:self waitUntilDone:YES];
        }
    });
    dispatch_source_set_event_handler(self._deviceSource, ^{
    
        dispatch_io_t _channel = dispatch_io_create(DISPATCH_IO_STREAM, self.fileDescriptor, self._posixSerialPortQueue, ^(int error) {
            if(error) {
                [self notifyDelegateOfPosixError];
            }
        });

        dispatch_io_set_high_water(_channel, self.readChunk);
        dispatch_io_set_low_water(_channel, 1);
        dispatch_io_set_interval(_channel, NSEC_PER_MSEC * self.readBlockMs, DISPATCH_IO_STRICT_INTERVAL);
    
        // Aggregate buffers received
        dispatch_io_read(_channel, 0, self.readChunk, self._posixSerialPortQueue, ^(bool done, dispatch_data_t data, int error){
            if(error) {
                NSLog(@"An error occured in read for device: %d", error);
            }
            if (data && data != dispatch_data_empty) {
                if (self._data == NULL) {
                    self._data = data;
                } else {
                    self._data = dispatch_data_create_concat(self._data, data);
                }
                [self processDataReceived];
            }
            if (done && error) {
                dispatch_io_close(_channel, 0);
            }
        });
    });
    dispatch_resume(self._deviceSource);
    
    dispatch_async(mainQueue, ^{
        if ([self.delegate respondsToSelector:@selector(serialPortWasOpened:)])
        {
            [self.delegate serialPortWasOpened:self];
        }
    });
}
- (void)close;
{
    if (self.isOpen) {
        dispatch_source_cancel(self._deviceSource);
        self._posixSerialPortQueue = nil;
    }
}

#pragma mark Port Read/Write
- (void)processDataReceived;
{
    if(self._data && self._data != dispatch_data_empty) {
        // Create contingent region of memory
        dispatch_data_t refRegion = dispatch_data_create_map(self._data,nil,nil);        
        dispatch_data_apply(refRegion, ^bool(dispatch_data_t region, size_t offset, const void *buffer, size_t size) {
            
            NSData *bufferRef = [NSData dataWithBytesNoCopy:(void *)(buffer+offset) length:size freeWhenDone:NO];
            
            dispatch_async(dispatch_get_main_queue(), ^(void){
            
                DataSegment ds = {offset,size};
            
                // Allow delegate to adjust for a valid data segment
                if ([self.delegate respondsToSelector:@selector(serialPort:nextDataSegmentValidIn:)])
                {
                    ds = [self.delegate serialPort:self nextDataSegmentValidIn:bufferRef];
                }
            
                // Call delegate to consume data with reference to valid segment
                // -- delegate must copy data if it is to be retained
                NSData *packetRef = [NSData dataWithBytesNoCopy:(void *)(buffer+ds.offset) length:ds.size freeWhenDone:NO];
            
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    if ([self.delegate respondsToSelector:@selector(serialPort:didReceiveData:)]
                        && ds.size > 0 )
                    {
                        [self.delegate serialPort:self didReceiveData:packetRef];
                    }
                    
                    // Update data reference, removing consumed data
                    dispatch_async(dispatch_get_main_queue(), ^(void){
                        if(ds.offset != offset || ds.size != size) {
                            self._data = dispatch_data_create_map(dispatch_data_create_subrange(region, offset+ds.size+(ds.offset-offset)
                                , size - (ds.size + (ds.offset-offset))),nil,nil);
                        }
                    });
                });
            });
            return true;
        });
    }
}
- (void)sendData:(NSData *)data;
{
    if (!self.isOpen || [data length] == 0 || self._posixSerialPortQueue == nil ) return;
 
    dispatch_data_t dataToSend = dispatch_data_create([data bytes], [data length],
         self._posixSerialPortQueue, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    
    dispatch_io_t _channel = dispatch_io_create(DISPATCH_IO_STREAM, self.fileDescriptor, self._posixSerialPortQueue, ^(int error) {
        if(error) {
            [self notifyDelegateOfPosixError];
        }
    });
    
    dispatch_io_write(_channel,0,dataToSend,self._posixSerialPortQueue, ^(bool wdone, dispatch_data_t wdata, int werr){
        if(werr) {
            [self notifyDelegateOfPosixError];
        }
        if (wdone && werr) {
            dispatch_io_close(_channel, 0);
        }
    });
}

#pragma mark Helper Methods

- (void)notifyDelegateOfPosixError
{
    [self notifyDelegateOfPosixErrorWaitingUntilDone:NO];
}

- (void)notifyDelegateOfPosixErrorWaitingUntilDone:(BOOL)shouldWait;
{
    if (![self.delegate respondsToSelector:@selector(serialPort:didEncounterError:)]) return;
    
    NSDictionary *errDict = @{NSLocalizedDescriptionKey: @(strerror(errno)),
                              NSFilePathErrorKey: self.path};
    NSError *error = [NSError errorWithDomain:NSPOSIXErrorDomain
                                         code:errno
                                     userInfo:errDict];
    
    void (^notifyBlock)(void) = ^{
        [self.delegate serialPort:self didEncounterError:error];
    };
    
    if ([NSThread isMainThread]) {
        notifyBlock();
    } else if (shouldWait) {
        dispatch_sync(dispatch_get_main_queue(), notifyBlock);
    } else {
        dispatch_async(dispatch_get_main_queue(), notifyBlock);
    }
}

#pragma mark Properties

- (BOOL)isOpen { return self._posixSerialPortQueue != nil; }

@end
