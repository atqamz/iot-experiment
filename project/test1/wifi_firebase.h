#ifndef WIFI_FIREBASE_H
#define WIFI_FIREBASE_H

#include <Arduino.h>
#include "pzem.h"

// WiFi credentials
#define WIFI_SSID "koswismacendanaputih_balkon"
#define WIFI_PASSWORD "AllahuAkbar"

// Firebase configuration
#define FIREBASE_PROJECT_ID "e-smarthome-62391"
#define FIREBASE_REALTIME_HOST "e-smarthome-62391-default-rtdb.asia-southeast1.firebasedatabase.app"

// NTP configuration for human-readable timestamp (GMT+7 Indonesia)
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 25200  // GMT+7 (7 * 3600)
#define DAYLIGHT_OFFSET_SEC 0

// Initialize WiFi connection
void initWiFi();

// Initialize NTP for time synchronization
void initNTP();

// Get human-readable timestamp string (format: "YYYY-MM-DD HH:MM:SS")
String getTimestamp();

// Get ISO 8601 timestamp for Firestore (format: "YYYY-MM-DDTHH:MM:SSZ")
String getISOTimestamp();

// Send sensor data to Firebase Realtime Database (live state, every 5 seconds)
// Returns true if successful, false otherwise
// brightness: current dimmer brightness (0-100)
// lightLevel: raw light sensor reading (0-4095)
bool sendDataToFirebase(float temperature, float humidity, PzemData pzemData, int brightness, int lightLevel);

// Send sensor data to Firestore (historical logging, every 5 minutes)
// Creates a new document in the 'sensorLogs' collection
bool sendDataToFirestore(float temperature, float humidity, PzemData pzemData, int brightness, int lightLevel);

// Check WiFi connection status
bool isWiFiConnected();

#endif
