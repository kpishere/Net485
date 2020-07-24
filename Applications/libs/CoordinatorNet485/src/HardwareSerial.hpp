//
//  HardwareSerial.h
//  CoordinatorNet485
//
//  Created by Kevin Peck on 2020-07-06.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//
//  An implementation of Arduino based HardwareSerial for POSIX OS
//

#ifndef HardwareSerial_hpp
#define HardwareSerial_hpp

class HardwareSerial
{
private:
    int fd;
    
    static int open_serial_port(const char * device, uint32_t baud_rate);
public:
    HardwareSerial(const char * device, uint32_t baud_rate);
    ~HardwareSerial();
    
    int available(void);
    void setTimeout(unsigned int mstimeout);
    
    // ::read(buffer, size): same as readBytes without timeout
    size_t readBytes(uint8_t* buffer, size_t size);
    size_t write(const uint8_t *buffer, size_t size);
};

// Misc. functions provided to emulate Arduino code environment
unsigned long millis();

#endif /* HardwareSerial_hpp */
