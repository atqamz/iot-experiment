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
    
    Serial.println("Light sensor initialized on GPIO 4");
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
    
    // MDL-07 behavior:
    // - Dark environment: Low ADC value (close to 0)
    // - Bright environment: High ADC value (close to 4095)
    
    // Invert for dimmer control:
    // - Dark (0) -> 100% brightness
    // - Light (4095) -> 0% brightness
    
    int brightness = map(lightLevel, 0, 4095, 100, 0);
    
    // Constrain to valid range
    return constrain(brightness, 0, 100);
}
