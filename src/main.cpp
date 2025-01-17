#include <Arduino.h>
#include "Pinout.h"

#include "int_utils.h"
#include "local_types.h"

static const unsigned long kKeyerTone = 600; //Hz

typedef void (*processing_unit_t)(void);
typedef struct task_t {
    task_t *next;
    processing_unit_t run;
} task_t;

void noop_task() {
    return;
}

task_t default_task = { nullptr, noop_task};
task_t *tasks = &default_task;

void addTask(task_t** tasks, processing_unit_t task) {
    task_t *n = new task_t{*tasks, task};
    *tasks = n;
    default_task.next = *tasks;
}

unsigned short
    DIT = 0,
    DAH = 1,
    PAU = 2;

const unsigned short
    max_speed_wps = 35, // 34 ms
    min_speed_wps = 15; // 80 ms

unsigned
    speed_wps = min_speed_wps,
    UNIT = 1200/speed_wps;

static unsigned short paddlePins[] = {kKeyerLeftPaddle, kKeyerRightPaddle};
TimerCtx ctxs[2];
bool LED_STATE = false;
wkstate_t gSendState = wkstate_t::kDone;
bool sendBuf[] = {false, false}; // dit, dah buffer

void readSpeed() {
    if (gSendState != kDone)
        return;
    static int current_val = 0;
    auto val = analogRead(kKeyerSpeed);
    if (abs(val - current_val) <= 10)
        return;
    UNIT = 1200.0/(min_speed_wps + (max_speed_wps - min_speed_wps)*((float)val/1023.0));
    current_val = val;
    ctxForTiming(ctxs[DIT], UNIT, UNIT, 1.5*UNIT);
    ctxForTiming(ctxs[DAH], 3*UNIT, UNIT, 3.5*UNIT);
}


inline void keyDown() {
    LED_STATE = true;
    tone(kKeyerAudio, kKeyerTone);
    digitalWrite(LED_BUILTIN, LED_STATE);
}

inline void keyUp() {
    LED_STATE = false;
    noTone(kKeyerAudio);
    digitalWrite(LED_BUILTIN, LED_STATE);
}

void send() {
    static unsigned sel = 0;
    if (gSendState != kDone) {
        return;
    }
    if (sendBuf[sel]) { // send next dit or dah
        sendBuf[sel] = false; // sent
        if (sel == DIT)
            gSendState = kSendingDIT;
        else
            gSendState = kSendingDAH;
        keyDown();
        startTimer1(ctxs[sel]);
    }
    sel = (sel + 1) % 2;
}

void readPaddles() { // FIX it read whole port instead of digitalRead
    static unsigned sel = 0;
    auto ready = sel == DIT ? (gSendState != kSendingDIT) : (gSendState != kSendingDAH);
    if (ready && !sendBuf[sel]) {
        auto val = digitalRead(paddlePins[sel]);
        if (val == LOW)
            sendBuf[sel] = true;
    }
    sel = (sel+1) % 2;
}

// First timer done - tone off
ISR(TIMER1_COMPA_vect){
  keyUp();
}

// second timer - deboucne off
ISR(TIMER1_COMPB_vect){
   gSendState = kWaiting;      //Allow paddle polling
}

// overflow can fire next timer
ISR(TIMER1_OVF_vect){
  stopTimer1();
  gSendState = kDone;
}

void initKeyer() {
    pinMode(paddlePins[DIT], INPUT_PULLUP);
    pinMode(paddlePins[DAH], INPUT_PULLUP);
    pinMode(kLEDPin, OUTPUT);
    pinMode(kKeyerAudio, OUTPUT);
    pinMode(kKeyerSpeed, INPUT);
    addTask(&tasks, readPaddles);
    addTask(&tasks, send);
    addTask(&tasks, readSpeed);
}

void processTasks() {
    static task_t *current_task = tasks;
    current_task->run();
    current_task = current_task->next;
}


void setup() {
    initKeyer();
}

void loop() {
    processTasks();
}
