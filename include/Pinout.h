//
//  Pinout.h
//  TrcvControlCenter
//
//  Created by Arkady Ten on 7/5/17.
//  Copyright © 2025 Arkady Ten / NU6N. All rights reserved.
//

#ifndef Pinout_h
#define Pinout_h
#include <Arduino.h>

typedef enum {
    kKeyerAudio     =  7,
    kKeyerLeftPaddle = 2,
    kKeyerRightPaddle = 3,
    kLEDPin         = 13,   // builtin
    kKeyerSpeed     =  A5
} pinout_t;

#endif /* Pinout_h */
