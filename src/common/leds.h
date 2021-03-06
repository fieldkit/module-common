#ifndef LEDS_H_INCLUDED
#define LEDS_H_INCLUDED

#include <Adafruit_NeoPixel.h>

namespace fk {

class Leds {
private:
    Adafruit_NeoPixel pixel_{ 1, A3, NEO_GRB + NEO_KHZ400 };
    uint32_t user_activity_{ 0 };
    bool gps_fix_{ false };

public:
    Leds();

public:
    void setup();
    bool task();

public:
    void notifyInitialized();
    void notifyAlive();
    void notifyBattery(float percentage);
    void notifyNoModules();
    void notifyReadingsBegin();
    void notifyReadingsDone();
    void notifyFatal();
    void notifyCaution();
    void notifyWarning();
    void notifyHappy();
    void notifyButtonPressed();
    void notifyButtonLong();
    void notifyButtonShort();
    void notifyButtonReleased();
    void notifyTopPassed();
    void notifyWifiOn();
    void notifyWifiOff();
    void off();
    void gpsFix(bool gps_fix) {
        gps_fix_ = gps_fix;
    }

private:
    bool disabled();

};

}

#endif
