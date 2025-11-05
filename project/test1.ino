// Reference XY-MD02: https://www.idbsmart.cz/wp-content/uploads/2024/05/xy-md02-manual.pdf

#include <HardwareSerial.h>
#include "modbus.h"
#include "pzem.h"
#include "dimmer.h"

HardwareSerial SensorSerial(2); // UART2 for Modbus

#define RXD2 18
#define TXD2 17

// Dimmer control variables
int brightness1 = 0;
int brightness2 = 0;
int fadeAmount = 5; // How much to fade each step

void setup()
{
  Serial.begin(115200);
  SensorSerial.begin(9600, SERIAL_8N1, RXD2, TXD2); // RXD2, TXD2 for Modbus
  pinMode(RS485_DIR, OUTPUT); // From modbus.h
  digitalWrite(RS485_DIR, LOW);

  initializePZEM(); // New function call
  initializeDimmers(); // Initialize the dimmers
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

  // --- Manual Dimmer Control Demo ---
  setDimmerBrightness(1, brightness1);
  setDimmerBrightness(2, brightness2);
  Serial.print("{\"dimmer_brightness\":");
  Serial.print(brightness1);
  Serial.println("}");

  // Change the brightness for next time through the loop
  brightness1 = brightness1 + fadeAmount;
  brightness2 = brightness2 + fadeAmount; // Synced for demo

  // Reverse the direction of the fade at the ends
  if (brightness1 <= 0 || brightness1 >= 100) {
    fadeAmount = -fadeAmount;
  }

  delay(100); // Shorten delay for smoother fading;
}