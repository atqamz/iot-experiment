#ifndef PZEM_H
#define PZEM_H

// Define PZEM pins here
const int PZEM_RX_PIN = 1;
const int PZEM_TX_PIN = 2;

struct PzemData {
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float pf;
  bool connected;
};

void initializePZEM();
PzemData readPZEM();

#endif
