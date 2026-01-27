#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <Arduino.h>

// MDL-07 Light Sensor Pin (Analog Output)
#define LIGHT_SENSOR_PIN 5  // GPIO 5 (ADC1_CH4) - Changed from GPIO 4 to avoid conflict with RS485_DIR

// Initialize the light sensor
void initLightSensor();

// Read raw light level from sensor
// Returns: 0 (dark) to 4095 (bright light)
int readLightLevel();

// Calculate dimmer brightness based on light level
// Returns: 0 (light/off) to 100 (dark/full brightness)
// Inverted: darker environment = brighter dimmer
int calculateBrightness();

#endif
