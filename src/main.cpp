#include <Arduino.h>


// Core library for code-sense - IDE-based
#if defined(ENERGIA) // LaunchPad specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ESP8266) // ESP8266 specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.8 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif // end IDE

#include "Pinout.h"

#include "fifobuffer.h"
#include "int_utils.h"
#include "local_types.h"

#define NOT_PRESSED HIGH
#define PRESSED LOW
#define STRAIGHT 0

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
DAT = 1,
PAU = 2;

const unsigned short
    max_speed_wps = 35, // 34 ms
    min_speed_wps = 15; // 80 ms
const unsigned kMSBeforSwitchingMode = 5000; // five seconds continous press before switching between paddles and key
unsigned
speed_wps = min_speed_wps,
UNIT = 1200/speed_wps;

static unsigned short paddlePins[] = {kKeyerLeftPaddle, kKeyerRightPaddle};
//static unsigned durations[] = {UNIT, 3*UNIT, UNIT};
static TimerCtx ctxs[2];
bool LED_STATE = false;
wkstate_t gSendState = wkstate_t::kDone;
FIFOBuffer ditsdahs(2);
bool sendBuf[] = {false, false}; // dit, dah

void read_speed() {
    if (gSendState != kDone)
        return;
    static int speed_val = 0;
    auto val = analogRead(kKeyerSpeed);
    if (abs(val - speed_val) <= 10)
        return;
    UNIT = 1200.0/(min_speed_wps + (max_speed_wps - min_speed_wps)*((float)val/1023.0));
    speed_val = val;
    ctxForTiming(ctxs[DIT], UNIT, UNIT, 1.5*UNIT);
    ctxForTiming(ctxs[DAT], 3*UNIT, UNIT, 3.5*UNIT);
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

void process_dd_buffer() {
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
        setupTimer1(ctxs[sel]);
        keyDown();
        enableTimer1(true);
    }
    sel = (sel + 1) % 2;
}

void read_paddles1() { // FIX it read whole port instead of digitalRead
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

// second timer - pause off
ISR(TIMER1_COMPB_vect){
   gSendState = kWaiting;      //Allow paddle polling
}

// second timer - pause off
ISR(TIMER1_OVF_vect){
  enableTimer1(false);
  gSendState = kDone;
}

void InitKeyer() {
    pinMode(paddlePins[DIT], INPUT_PULLUP);
    pinMode(paddlePins[DAT], INPUT_PULLUP);
    pinMode(kLEDPin, OUTPUT);
    pinMode(kKeyerAudio, OUTPUT);
    pinMode(kKeyerSpeed, INPUT);
    addTask(&tasks, read_paddles1);
    addTask(&tasks, process_dd_buffer);
    addTask(&tasks, read_speed);
}

void ProcessKeyer() {
    static task_t *current_task = tasks;
    current_task->run();
    current_task = current_task->next;
}


void setup()
{
    //Serial.begin(9600);
    InitKeyer();
}

void loop()
{
    ProcessKeyer();
}
