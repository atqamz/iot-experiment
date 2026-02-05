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

String getISOTimestamp() {
    if (!timeInitialized) {
        return "1970-01-01T00:00:00Z";
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = gmtime(&now);  // UTC time for Firestore
    
    char buffer[32];
    // ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
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

bool sendDataToFirestore(float temperature, float humidity, PzemData pzemData, int brightness, int lightLevel) {
    if (!isWiFiConnected()) {
        Serial.println("WiFi not connected, skipping Firestore upload");
        return false;
    }
    
    HTTPClient https;
    
    // Firestore REST API URL - creates new document in 'sensorLogs' collection
    String url = "https://firestore.googleapis.com/v1/projects/" + 
                 String(FIREBASE_PROJECT_ID) + 
                 "/databases/(default)/documents/sensorLogs";
    
    https.begin(url);
    https.addHeader("Content-Type", "application/json");
    
    // Build Firestore JSON format (requires special structure)
    JsonDocument doc;
    JsonObject fields = doc["fields"].to<JsonObject>();
    
    // Timestamp field
    JsonObject timestampField = fields["timestamp"].to<JsonObject>();
    timestampField["timestampValue"] = getISOTimestamp();
    
    // Local timestamp (human readable)
    JsonObject localTimeField = fields["local_time"].to<JsonObject>();
    localTimeField["stringValue"] = getTimestamp();
    
    // Environment group
    JsonObject envField = fields["environment"].to<JsonObject>();
    JsonObject envMap = envField["mapValue"].to<JsonObject>();
    JsonObject envFields = envMap["fields"].to<JsonObject>();
    
    JsonObject tempField = envFields["temperature"].to<JsonObject>();
    tempField["doubleValue"] = round(temperature * 10) / 10.0;
    
    JsonObject humField = envFields["humidity"].to<JsonObject>();
    humField["doubleValue"] = round(humidity * 10) / 10.0;
    
    JsonObject envConnField = envFields["connected"].to<JsonObject>();
    envConnField["booleanValue"] = (temperature != -1);
    
    // Power group
    JsonObject powerField = fields["power"].to<JsonObject>();
    JsonObject powerMap = powerField["mapValue"].to<JsonObject>();
    JsonObject powerFields = powerMap["fields"].to<JsonObject>();
    
    JsonObject voltageField = powerFields["voltage"].to<JsonObject>();
    voltageField["doubleValue"] = round(pzemData.voltage * 10) / 10.0;
    
    JsonObject currentField = powerFields["current"].to<JsonObject>();
    currentField["doubleValue"] = round(pzemData.current * 100) / 100.0;
    
    JsonObject pwrField = powerFields["power"].to<JsonObject>();
    pwrField["doubleValue"] = round(pzemData.power * 10) / 10.0;
    
    JsonObject energyField = powerFields["energy"].to<JsonObject>();
    energyField["doubleValue"] = round(pzemData.energy * 1000) / 1000.0;
    
    JsonObject freqField = powerFields["frequency"].to<JsonObject>();
    freqField["doubleValue"] = round(pzemData.frequency * 10) / 10.0;
    
    JsonObject pfField = powerFields["pf"].to<JsonObject>();
    pfField["doubleValue"] = round(pzemData.pf * 100) / 100.0;
    
    JsonObject pzemConnField = powerFields["connected"].to<JsonObject>();
    pzemConnField["booleanValue"] = pzemData.connected;
    
    // Lighting group
    JsonObject lightField = fields["lighting"].to<JsonObject>();
    JsonObject lightMap = lightField["mapValue"].to<JsonObject>();
    JsonObject lightFields = lightMap["fields"].to<JsonObject>();
    
    JsonObject lightLevelField = lightFields["light_level"].to<JsonObject>();
    lightLevelField["integerValue"] = String(lightLevel);
    
    JsonObject brightnessField = lightFields["auto_brightness"].to<JsonObject>();
    brightnessField["integerValue"] = String(brightness);
    
    JsonObject dimmer1Field = lightFields["dimmer1"].to<JsonObject>();
    dimmer1Field["integerValue"] = String(brightness);
    
    JsonObject dimmer2Field = lightFields["dimmer2"].to<JsonObject>();
    dimmer2Field["integerValue"] = String(brightness);
    
    // System group
    JsonObject sysField = fields["system"].to<JsonObject>();
    JsonObject sysMap = sysField["mapValue"].to<JsonObject>();
    JsonObject sysFields = sysMap["fields"].to<JsonObject>();
    
    JsonObject deviceField = sysFields["device"].to<JsonObject>();
    deviceField["stringValue"] = "ESP32S3_IoT";
    
    JsonObject heapField = sysFields["free_heap"].to<JsonObject>();
    heapField["integerValue"] = String(ESP.getFreeHeap());
    
    JsonObject rssiField = sysFields["wifi_rssi"].to<JsonObject>();
    rssiField["integerValue"] = String(WiFi.RSSI());
    
    // Serialize JSON to string
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Send POST request (creates new document with auto-generated ID)
    int httpCode = https.POST(jsonString);
    
    https.end();
    
    if (httpCode == 200) {
        Serial.print("Firestore log OK at ");
        Serial.println(getTimestamp());
        return true;
    } else {
        Serial.print("Firestore log failed, HTTP code: ");
        Serial.println(httpCode);
        return false;
    }
}
