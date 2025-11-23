#include <Arduino.h> // <--- CRITICAL FIX: Must be at the top!
#include "dimmer.h"

// --- PIN DEFINITIONS ---
// Make sure these match your physical wiring!
const int ZERO_CROSS_PIN = 14;
const int DIMMER_1_PIN = 13;
const int DIMMER_2_PIN = 12;

// --- HARDWARE TIMER HANDLES ---
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;

// --- STATE VARIABLES ---
volatile int delayTime1 = 0;
volatile int delayTime2 = 0;
volatile bool ch1_active = false;
volatile bool ch2_active = false;

// Noise Filter variable
volatile unsigned long lastZCTime = 0;

// Pulse width for the TRIAC trigger (10 microseconds)
const int TRIAC_PULSE_MICROS = 10;

// --- INTERRUPT SERVICE ROUTINES (ISRs) ---

// Timer 1 ISR: Turn on Channel 1
void IRAM_ATTR onTimer1() {
    if (ch1_active) {
        digitalWrite(DIMMER_1_PIN, HIGH);
        ets_delay_us(TRIAC_PULSE_MICROS);
        digitalWrite(DIMMER_1_PIN, LOW);
    }
}

// Timer 2 ISR: Turn on Channel 2
void IRAM_ATTR onTimer2() {
    if (ch2_active) {
        digitalWrite(DIMMER_2_PIN, HIGH);
        ets_delay_us(TRIAC_PULSE_MICROS);
        digitalWrite(DIMMER_2_PIN, LOW);
    }
}

// Zero-Cross ISR
void IRAM_ATTR onZeroCross() {
    unsigned long now = micros();

    // Noise Filter: Ignore noise faster than 8.5ms (8500us)
    if (now - lastZCTime > 8500) {
        lastZCTime = now;

        // Restart Timers
        if (ch1_active) {
            // New API: timerWrite sets counter value
            timerWrite(timer1, 0);
            // New API: timerAlarm sets target, autureload=false, enable=0
            // For one-shot in v3.0, we usually re-arm or just use setAlarm
            timerAlarm(timer1, delayTime1, false, 0); 
        }
        
        if (ch2_active) {
            timerWrite(timer2, 0);
            timerAlarm(timer2, delayTime2, false, 0);
        }
    }
}

// --- PUBLIC FUNCTIONS ---

void initializeDimmers() {
    // 1. Setup Pins
    pinMode(DIMMER_1_PIN, OUTPUT);
    pinMode(DIMMER_2_PIN, OUTPUT);
    pinMode(ZERO_CROSS_PIN, INPUT_PULLUP);
    
    digitalWrite(DIMMER_1_PIN, LOW);
    digitalWrite(DIMMER_2_PIN, LOW);

    // 2. Setup Hardware Timers (ESP32 Core 3.0.x API)
    // 1MHz frequency means 1 tick = 1 microsecond
    timer1 = timerBegin(1000000); 
    timerAttachInterrupt(timer1, &onTimer1);

    timer2 = timerBegin(1000000);
    timerAttachInterrupt(timer2, &onTimer2);

    // 3. Setup Zero Cross Interrupt
    attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_PIN), onZeroCross, RISING);
}

void setDimmerBrightness(int channel, int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;

    // Map 1-100 brightness to ~8600us - 500us delay
    int delay = 0;
    bool active = true;

    if (brightness == 0) {
        active = false; 
    } else {
        // Map: High brightness = Short delay. Low brightness = Long delay.
        delay = map(brightness, 1, 100, 8600, 500); 
    }

    if (channel == 1) {
        ch1_active = active;
        delayTime1 = delay;
    } else if (channel == 2) {
        ch2_active = active;
        delayTime2 = delay;
    }
}
