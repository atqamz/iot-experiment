#include "light_sensor.h"

// Smoothing variables to prevent flickering
const int NUM_SAMPLES = 10;
int samples[NUM_SAMPLES];
int sampleIndex = 0;
bool samplesReady = false;

void initLightSensor() {
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    
    // Initialize samples array
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = 0;
    }
    
    Serial.println("Light sensor initialized on GPIO 5");
}

int readLightLevel() {
    // Read new sample
    int rawValue = analogRead(LIGHT_SENSOR_PIN);
    
    // Add to rolling average for smoothing
    samples[sampleIndex] = rawValue;
    sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;
    
    if (sampleIndex == 0) {
        samplesReady = true;
    }
    
    // Calculate average
    long sum = 0;
    int count = samplesReady ? NUM_SAMPLES : sampleIndex;
    
    if (count == 0) {
        return rawValue;
    }
    
    for (int i = 0; i < count; i++) {
        sum += samples[i];
    }
    
    return sum / count;
}

int calculateBrightness() {
    int lightLevel = readLightLevel();
    
    // MDL-07 behavior (photoresistor module):
    // - Bright environment: Low ADC value (close to 0) - more light = lower resistance
    // - Dark environment: High ADC value (close to 4095) - less light = higher resistance
    
    // Dimmer control logic:
    // - Bright (low ADC ~0) -> Dimmer OFF (0%) - don't need artificial light
    // - Dark (high ADC ~4095) -> Dimmer ON (100%) - need artificial light
    
    int brightness = map(lightLevel, 0, 4095, 0, 100);
    
    // Constrain to valid range
    return constrain(brightness, 0, 100);
}
