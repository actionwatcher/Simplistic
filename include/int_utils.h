#include <Arduino.h>

typedef struct{
    uint8_t
        prescaler;
    uint16_t 
        signal_comparator{0},
        offset{0},
        debounce_comparator{0};
} TimerCtx;

void stopTimer1();
void startTimer1(const TimerCtx& ctx);
void ctxForTiming(TimerCtx& ctx, uint32_t signal_milliseconds, uint32_t pause_milliseconds, uint32_t debounce_ms);
