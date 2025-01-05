//
//  Pinout.h
//  TrcvControlCenter
//
//  Created by Arkady Ten on 7/5/17.
//  Copyright © 2017 km6jma. All rights reserved.
//

#ifndef Pinout_h
#define Pinout_h
#include <Arduino.h>

typedef enum {
    kEncoderPinA = 3,
    kEncoderPinB = 2,
    kEncoderButtonPin   = 4,
    kRSTPin         =  5,   // RST - Reset, pin 4 on LCD.
    kSCEPin         =  6,   // SCE - Chip select, pin 3 on LCD.
    kDCPin          =  7,   // DC - Data/Command, pin 5 on LCD.
    kSDINPin        =  11,   // DN(MOSI) - Serial data, pin 6 on LCD.
    kSCLKPin        =  13,   // SCLK - Serial clock, pin 7 on LCD not changable as of right now
    kBacklightPin   = 10,   // LED - Backlight LED, pin 8 on LCD.
    kKeyerAudio     =  7,
    kKeyerLeftPaddle = 2,
    kKeyerRightPaddle = 3,
    kLEDPin         = 13,   // A3
    kKeyerSpeed     =  A5
    /* PIN_WIRE_SDA        (18) analog pin 4 */
    /* PIN_WIRE_SCL        (19) analog pin 5 on duemilanove */
} pinout_t;

enum {
    kVFOChannel = 0,
    kCWChannel  = 1
};

#endif /* Pinout_h */
