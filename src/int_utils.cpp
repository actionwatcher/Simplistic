#include "int_utils.h"
#include <Arduino.h>

/** private helpers */
uint16_t calculatePrescaler(uint32_t milliseconds) {
    uint32_t cycles = F_CPU / 1000 * milliseconds;
    uint16_t prescaler = 1024; // gives 0.064 ms resolution
    if (cycles <= 65536) {
        prescaler = 1;
    } else if (cycles <= 65536 * 8) {
        prescaler = 8;
    } else if (cycles <= 65536 * 64) {
        prescaler = 64;
    } else if (cycles <= 65536 * 256) {
        prescaler = 256; // 0.016 ms resolution
    }
    return prescaler;
}

uint16_t calculateComparatorValue(uint32_t milliseconds, uint16_t prescaler) {
    uint32_t cycles = F_CPU / 1000 * milliseconds;
    uint16_t comparatorValue = (cycles / prescaler) + 0.5;
    return comparatorValue;
}

uint8_t calculatePrescalerRegister(uint16_t prescaler) {
    const uint16_t scalers[] = {1, 8, 64, 256, 1024};
    const uint8_t registerValues[] = {1<<CS10, 1<<CS11, (1<<CS10) | (1<<CS11), 1<<CS12, (1<<CS10) | (1<<CS12)};
    for(auto i = 0; i < 5; i++) {
        if(scalers[i] == prescaler) {
            return registerValues[i];
        }
    }
    return 0;
} 

/** pulic interface */
void setupTimer1ForDuration(uint32_t milliseconds) {
    auto prescaler = calculatePrescaler(milliseconds);
    auto comparatorValue = calculateComparatorValue(milliseconds, prescaler);
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1  = 0; // initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = comparatorValue;
    // turn on CTC mode
    //TCCR1B |= (1 << WGM12);
    // Set CS12 and CS10 bits for 1024 prescaler
    TCCR1B |= calculatePrescalerRegister(prescaler);
}

void setupTimer1ForDurations(uint32_t signal_milliseconds, uint32_t pause_milliseconds, uint32_t debounce_ms) {
    auto prescaler = calculatePrescaler(signal_milliseconds + pause_milliseconds);
    auto signalComparatorValue = calculateComparatorValue(signal_milliseconds, prescaler);
    auto pauseComparatorValue = calculateComparatorValue(pause_milliseconds + signal_milliseconds, prescaler);
    auto debounceComparatorValue = min(calculateComparatorValue(debounce_ms, prescaler), pauseComparatorValue);
    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1  = 65536 - pauseComparatorValue; // initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = signalComparatorValue + 65536 - pauseComparatorValue;
    OCR1B = debounceComparatorValue - 65536 - pauseComparatorValue;
    TCCR1B |= calculatePrescalerRegister(prescaler);
}

void ctxForTiming(TimerCtx& ctx, uint32_t signal_milliseconds, uint32_t pause_milliseconds, uint32_t debounce_ms) {
    auto prescaler = calculatePrescaler(signal_milliseconds + pause_milliseconds);
    auto signalComparatorValue = calculateComparatorValue(signal_milliseconds, prescaler);
    auto pauseComparatorValue = calculateComparatorValue(pause_milliseconds + signal_milliseconds, prescaler);
    auto debounceComparatorValue = min(calculateComparatorValue(debounce_ms, prescaler), pauseComparatorValue);
    ctx.offset = 65536 - pauseComparatorValue; // initialize counter value to 0
    // set compare match register for 1hz increments
    ctx.signal_comparator = signalComparatorValue + ctx.offset;
    ctx.debounce_comparator = debounceComparatorValue + ctx.offset;
    ctx.prescaler = calculatePrescalerRegister(prescaler);
}

void setupTimer1(const TimerCtx& ctx) {
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = ctx.offset; // initialize counter value to 0
    OCR1A = ctx.signal_comparator;
    OCR1B = ctx.debounce_comparator;
    TCCR1B |= ctx.prescaler;
}
void enableTimer1(bool enable) {
    if (enable) {
        TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B) | (1 << TOIE1);
    } else {
        TIMSK1 &= ~(1 << OCIE1A | (1 << OCIE1B) | (1 << TOIE1));
    }
}

void printCtx(const char * const prefix, const TimerCtx& ctx) {
    Serial.print(prefix);
    Serial.print(" Prescaler: "); Serial.print(ctx.prescaler);
    Serial.print(", Offset: "); Serial.print(ctx.offset);
    Serial.print(", Signal: "); Serial.print(ctx.signal_comparator);
    Serial.print(", Debounce: "); Serial.println(ctx.debounce_comparator);
}