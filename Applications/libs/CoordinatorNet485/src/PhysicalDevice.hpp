//
//  PhysicalDevice.hpp
//  CoordinatorNet485
//
//  Created by Kevin Peck on 2020-07-06.
//  Copyright © 2020 Kevin Peck. All rights reserved.
//
#ifndef PhysicalDevice_hpp
#define PhysicalDevice_hpp

#if defined(__AVR__)
    #include <Arduino.h>
    #include <HardwareSerial.h>
    #define OUTPUT_DRIVE_ENABLE_PIN 2
#elif defined(ESP8266)
    #include <Arduino.h>
    #include <HardwareSerial.h>
    #define OUTPUT_DRIVE_ENABLE_PIN D6
#elif __APPLE__
    #define OUTPUT_DRIVE_ENABLE_PIN 0
    #include <string.h>
    #include <stdlib.h>
    #include <stdio.h>

    #include "HardwareSerial.hpp"

    #define min(x,y) ( (x)>(y)?(y):(x) )

    #define randomSeed(x) (srandom((unsigned int)x))
    #define analogRead(x) 0
    #define random(x,y) random()

    // This behaviour not desired in this environment
    #define cli()
    #define sei()
    #define digitalWrite(x,y)
    #define pinMode(x,y)
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
    #error "Unknown target"
#endif

#endif /* PhysicalDevice_hpp */
