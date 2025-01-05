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


#define NOT_PRESSED HIGH
#define PRESSED LOW
#define STRAIGHT 0

static const unsigned long kKeyerTone = 800; //Hz

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
max_speed_wps = 35,
min_speed_wps = 15;
const unsigned kMSBeforSwitchingMode = 5000; // five seconds continous press before switching between paddles and key
unsigned
speed_wps = min_speed_wps,
UNIT = 1200/speed_wps;

static bool currentPinState[] = {NOT_PRESSED, NOT_PRESSED};
static unsigned short paddlePins[] = {kKeyerLeftPaddle, kKeyerRightPaddle};
static long unsigned lastPinTransitionTime[] = {millis(), millis()};  // time of the last paddle or key transition
static unsigned durations[] = {UNIT, 3*UNIT, UNIT};
static uint8_t gKeyMode = 1; // 0 straight, 1 - paddle left, 2 - paddle right

void send_iambic() {
    static unsigned long start_time = 0;
    static unsigned short current_duration = 0;
    static unsigned short sel = 0;
    static bool inPause = false;
    static bool currentlySending = false;
    if (!currentlySending) {  //nothing being sent
        if (currentPinState[sel] == NOT_PRESSED) { //not pressed check another
            sel = (++sel) & 1;  // next paddle
            if (currentPinState[sel] == NOT_PRESSED) { // nothing happening
                sel = (++sel) & 1;  // next paddle
                return;
            }
        }
        start_time = millis();  // this will lock sender on send
        currentlySending = true;
        current_duration = durations[sel];
        digitalWrite(kLEDPin, HIGH);
        tone(kKeyerAudio, kKeyerTone);
        return;
    }
    
    if (millis() - start_time < current_duration) { // still sending
        return;
    }
    
    //sending is done
    if (inPause) {  // pause finished resume normal processing
        inPause = false;
        currentlySending = false;  // unlock sender
    } else { // stop sending dit or dat
        digitalWrite(kLEDPin, LOW);
        noTone(kKeyerAudio);
        currentPinState[sel] = NOT_PRESSED; // allow paddle signal
        sel = (++sel) & 1;  // rotate buffer
        // start sending pause
        start_time = millis();
        current_duration = durations[PAU];
        inPause = true;
    }
    return;
}

void send_tone() {
    static bool sending = false;
    if (currentPinState[DIT] == PRESSED || currentPinState[DAT] == PRESSED) {
        if (!sending) {
            tone(kKeyerAudio, kKeyerTone);
            digitalWrite(kLEDPin, HIGH);
            sending = true;
        }
    } else {
        if (sending) {
            noTone(kKeyerAudio);
            digitalWrite(kLEDPin, LOW);
            sending = false;
        }
    }
}

void send_signal() {
    if (gKeyMode == STRAIGHT) {
        send_tone();
    } else {
        send_iambic();
    }
}

void read_speed() {
    auto val = analogRead(kKeyerSpeed);
    UNIT = 1200.0/(min_speed_wps + (max_speed_wps - min_speed_wps)*((float)val/1023.0));
    durations[DIT] = UNIT;
    durations[DAT] = 3*UNIT;
    durations[PAU] = UNIT;
    Serial.println(val);
}

void update_keymode(const long unsigned& currentTime) {
    gKeyMode = (++gKeyMode)%3;
    currentPinState[DIT] = currentPinState[DAT] = NOT_PRESSED;
    lastPinTransitionTime[DIT] = lastPinTransitionTime[DAT] = currentTime;
    switch (gKeyMode) {
        case 1:
            DIT = 0;
            DAT = 1;
            break;
        case 2:
            DAT = 0;
            DIT = 1;
            break;
        default:
            ;
    }
}

void read_paddle(unsigned short sel) {
    static long unsigned privTime = {millis()};
    if (currentPinState[sel] == NOT_PRESSED) { // check only if signal processed
        auto currentTime = millis();
        currentPinState[sel] = digitalRead(paddlePins[sel]);
        if (currentPinState[sel] == NOT_PRESSED) { //reset switch timer
            lastPinTransitionTime[sel] = currentTime;
        } else if (currentTime - lastPinTransitionTime[sel] > kMSBeforSwitchingMode) { // need switching mode
            update_keymode(currentTime);
        }
    }
}

void read_key() {
    static long unsigned privTime = {millis()};
    auto currentTime = millis();
    if ((currentTime - lastPinTransitionTime[0] > kMSBeforSwitchingMode) && (currentPinState[DIT] == PRESSED || currentPinState[DAT] == PRESSED)) { // need switching mode
        update_keymode(currentTime);
        return;
    }
    auto newRead = (digitalRead(paddlePins[DIT]) && digitalRead(paddlePins[DAT])); //replace with port read
    if (currentPinState[DIT] != newRead) {
        if (currentTime - privTime > 10) {
            currentPinState[DIT] = currentPinState[DAT] = newRead;
            lastPinTransitionTime[DIT] = lastPinTransitionTime[DAT] = currentTime;
        }
        privTime = currentTime;
        return;
    }
}


void read_paddles() {
    if (gKeyMode == STRAIGHT) {
        read_key();
    } else {
        read_paddle(DIT);
        read_paddle(DAT);
    }
}


void InitKeyer() {
    pinMode(paddlePins[DIT], INPUT_PULLUP);
    pinMode(paddlePins[DAT], INPUT_PULLUP);
    pinMode(kLEDPin, OUTPUT);
    pinMode(kKeyerAudio, OUTPUT);
    pinMode(kKeyerSpeed, INPUT);
    addTask(&tasks, read_paddles);
    addTask(&tasks, send_signal);
    addTask(&tasks, read_speed);
}

void ProcessKeyer() {
    static task_t *current_task = tasks;
    current_task->run();
    current_task = current_task->next;
}


void setup()
{
  Serial.begin(9600);
    // StartLogging(9600);
    
    // lcdBegin(); // This will setup our pins, and initialize the LCD
    // setContrast(60); // Good values range from 40-60
    // //delay(2000);
    // InitDisplay(gDisplayLayout);
    // clearDisplay(WHITE);
    // updateDisplay();
    // InitRotaryEncoder(updateFrequency);
    // InitVFO();
    InitKeyer();
    // EnableDSSChannel(kVFOChannel, true);
    // SetFrequency(kCWChannel, kBFOFrequency + 800);
    // Logln("InitVFO done");
}

void printTime();
// Loop turns the display into a local serial monitor echo.
// Type to the Arduino from the serial monitor, and it'll echo
// what you type on the display. Type ~ to clear the display.
void loop()
{
    // printTime();
    // auto rButtonState = ReadRotaryEncoderButton();
    // static auto prevrButtonState = rButtonState;
    // if (prevrButtonState != rButtonState) {
    //     if (prevrButtonState) { // button released
    //         if (gButtonChangeBand) {
    //             freqIndx = ++freqIndx%kFreqCount;
    //             freq = band[freqIndx]*1.e6;
    //             kShift = freq > kBFOFrequency ? -kBFOFrequency : kBFOFrequency;
    //             int zero = 0;
    //             updateFrequency((void*)&zero);
    //         }
    //     } else { // step change
    //         ChangeStep();
    //     }
        
    //     prevrButtonState = rButtonState;
    // }
    // printFrequency();
    // updateDisplay();
    // SetFrequency(kVFOChannel, freq + kShift);
    ProcessKeyer();
}
