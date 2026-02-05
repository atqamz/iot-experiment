// Reference XY-MD02: https://www.idbsmart.cz/wp-content/uploads/2024/05/xy-md02-manual.pdf

#include <HardwareSerial.h>
#include "modbus.h"
#include "pzem.h"
#include "dimmer.h"
#include "wifi_firebase.h"
#include "light_sensor.h"

HardwareSerial SensorSerial(2); // UART2 for Modbus

#define RXD2 18
#define TXD2 17

// Dimmer brightness (controlled by light sensor)
int brightness = 0;
int lightLevel = 0;

// Firebase upload interval (5 seconds)
const unsigned long FIREBASE_INTERVAL = 5000;
unsigned long lastFirebaseUpload = 0;

// Firestore logging interval (5 minutes)
const unsigned long FIRESTORE_INTERVAL = 300000;  // 5 * 60 * 1000
unsigned long lastFirestoreUpload = 0;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== ESP32-S3 IoT System ===");
  
  SensorSerial.begin(9600, SERIAL_8N1, RXD2, TXD2); // RXD2, TXD2 for Modbus
  pinMode(RS485_DIR, OUTPUT); // From modbus.h
  digitalWrite(RS485_DIR, LOW);

  // Initialize WiFi (also initializes NTP for timestamps)
  initWiFi();

  initializePZEM();       // Initialize PZEM sensor
  initializeDimmers();    // Initialize the dimmers
  initLightSensor();      // Initialize light sensor
  
  Serial.println("System Ready!");
}

void loop()
{
  // Read Modbus sensor
  float temperature = readModBus(0x0001); // Temperature ( check ref manual )
  float humidity = readModBus(0x0002);    // Humidity ( check ref manual )

  Serial.print("{\"temperature\":");
  Serial.print(temperature);
  Serial.print(",\"humidity\":");
  Serial.print(humidity);
  Serial.println("}");

  // Read PZEM sensor
  PzemData pzemData = readPZEM();
  if (pzemData.connected) {
    Serial.print("{\"voltage\":");
    Serial.print(pzemData.voltage);
    Serial.print(",\"current\":");
    Serial.print(pzemData.current);
    Serial.print(",\"power\":");
    Serial.print(pzemData.power);
    Serial.print(",\"energy\":");
    Serial.print(pzemData.energy);
    Serial.print(",\"frequency\":");
    Serial.print(pzemData.frequency);
    Serial.print(",\"pf\":");
    Serial.print(pzemData.pf);
    Serial.println("}");
  } else {
    Serial.println("{\"pzem_status\":\"disconnected\"}");
  }

  // --- Automatic Light-Based Dimmer Control ---
  // Read light sensor and calculate brightness
  lightLevel = readLightLevel();
  brightness = calculateBrightness();
  
  // Apply brightness to both dimmers
  setDimmerBrightness(1, brightness);
  setDimmerBrightness(2, brightness);
  
  // Log light sensor data
  Serial.print("{\"light_level\":");
  Serial.print(lightLevel);
  Serial.print(",\"auto_brightness\":");
  Serial.print(brightness);
  Serial.println("}");

  // --- Firebase Realtime DB Upload (every 5 seconds) ---
  unsigned long currentMillis = millis();
  if (currentMillis - lastFirebaseUpload >= FIREBASE_INTERVAL) {
    lastFirebaseUpload = currentMillis;
    
    // Send all sensor data to Firebase Realtime DB (live state)
    if (sendDataToFirebase(temperature, humidity, pzemData, brightness, lightLevel)) {
      Serial.println("{\"firebase\":\"upload_success\"}");
    } else {
      Serial.println("{\"firebase\":\"upload_failed\"}");
    }
  }

  // --- Firestore Logging (every 5 minutes) ---
  if (currentMillis - lastFirestoreUpload >= FIRESTORE_INTERVAL) {
    lastFirestoreUpload = currentMillis;
    
    // Send data to Firestore for historical logging
    if (sendDataToFirestore(temperature, humidity, pzemData, brightness, lightLevel)) {
      Serial.println("{\"firestore\":\"log_success\"}");
    } else {
      Serial.println("{\"firestore\":\"log_failed\"}");
    }
  }

  delay(100); // Short delay for responsive dimmer control
}
