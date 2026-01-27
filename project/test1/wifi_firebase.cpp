#include "wifi_firebase.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Track connection states
bool wifiConnected = false;
bool timeInitialized = false;

void initWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println();
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
        
        // Initialize NTP after WiFi connection
        initNTP();
    } else {
        wifiConnected = false;
        Serial.println();
        Serial.println("WiFi connection failed!");
    }
}

void initNTP() {
    Serial.print("Syncing time with NTP");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER, "time.nist.gov");
    
    // Wait for time to be set
    time_t now;
    int attempts = 0;
    while ((now = time(nullptr)) < 1000000000 && attempts < 30) {
        Serial.print(".");
        delay(500);
        attempts++;
    }
    
    timeInitialized = (now > 1000000000);
    
    if (timeInitialized) {
        Serial.println();
        Serial.print("Time synchronized: ");
        Serial.println(getTimestamp());
    } else {
        Serial.println();
        Serial.println("NTP sync failed!");
    }
}

String getTimestamp() {
    if (!timeInitialized) {
        return "1970-01-01 00:00:00";
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    char buffer[25];
    // Format: YYYY-MM-DD HH:MM:SS
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
}

bool isWiFiConnected() {
    wifiConnected = (WiFi.status() == WL_CONNECTED);
    return wifiConnected;
}

bool sendDataToFirebase(float temperature, float humidity, PzemData pzemData, int brightness, int lightLevel) {
    if (!isWiFiConnected()) {
        Serial.println("WiFi not connected, skipping Firebase upload");
        return false;
    }
    
    HTTPClient https;
    
    // Firebase RTDB REST API URL
    String url = "https://" + String(FIREBASE_REALTIME_HOST) + "/device/sensorData.json";
    
    https.begin(url);
    https.addHeader("Content-Type", "application/json");
    
    // Build JSON payload using ArduinoJson
    JsonDocument doc;
    
    // Timestamps (root level)
    doc["timestamp"] = getTimestamp();
    doc["unix_time"] = (unsigned long)time(nullptr);
    
    // Environment group (XY-MD02 Modbus sensor)
    JsonObject environment = doc["environment"].to<JsonObject>();
    environment["temperature"] = round(temperature * 10) / 10.0;
    environment["humidity"] = round(humidity * 10) / 10.0;
    environment["connected"] = (temperature != -1);  // -1 means no response
    
    // Power group (PZEM-004T sensor)
    JsonObject power = doc["power"].to<JsonObject>();
    power["voltage"] = round(pzemData.voltage * 10) / 10.0;
    power["current"] = round(pzemData.current * 100) / 100.0;
    power["power"] = round(pzemData.power * 10) / 10.0;
    power["energy"] = round(pzemData.energy * 1000) / 1000.0;
    power["frequency"] = round(pzemData.frequency * 10) / 10.0;
    power["pf"] = round(pzemData.pf * 100) / 100.0;
    power["connected"] = pzemData.connected;
    
    // Lighting group (Light sensor + Dimmers)
    JsonObject lighting = doc["lighting"].to<JsonObject>();
    lighting["light_level"] = lightLevel;           // Raw ADC value (0-4095)
    lighting["auto_brightness"] = brightness;       // Calculated brightness (0-100%)
    lighting["dimmer1"] = brightness;
    lighting["dimmer2"] = brightness;
    
    // System group (Device info)
    JsonObject system = doc["system"].to<JsonObject>();
    system["device"] = "ESP32S3_IoT";
    system["free_heap"] = ESP.getFreeHeap();
    system["wifi_rssi"] = WiFi.RSSI();              // Signal strength in dBm
    
    // Serialize JSON to string
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send PUT request (overwrites data at path)
    int httpCode = https.PUT(jsonString);
    
    https.end();
    
    if (httpCode == 200) {
        Serial.print("Firebase update OK at ");
        Serial.println(getTimestamp());
        return true;
    } else {
        Serial.print("Firebase update failed, HTTP code: ");
        Serial.println(httpCode);
        return false;
    }
}
