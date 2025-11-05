#include "dimmer.h"
#include <RBDdimmer.h>

// User-defined pins
const int ZERO_CROSS_PIN = 14;
const int DIMMER_1_PIN = 13;
const int DIMMER_2_PIN = 12;

// Create dimmer objects
dimmerLamp dimmer1(DIMMER_1_PIN, ZERO_CROSS_PIN);
dimmerLamp dimmer2(DIMMER_2_PIN, ZERO_CROSS_PIN);

void initializeDimmers() {
    // Initialize the dimmers
    dimmer1.begin(NORMAL_MODE, ON);
    dimmer2.begin(NORMAL_MODE, ON);
}

void setDimmerBrightness(int channel, int brightness) {
    // Clamp brightness value between 0 and 100
    if (brightness < 0) {
        brightness = 0;
    }
    if (brightness > 100) {
        brightness = 100;
    }

    if (channel == 1) {
        dimmer1.setPower(brightness);
    } else if (channel == 2) {
        dimmer2.setPower(brightness);
    }
}
