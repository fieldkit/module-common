#include "power_management.h"
#include "debug.h"

namespace fk {

constexpr uint32_t Interval = 5000;
constexpr const char Log[] = "Power";

void Power::setup() {
    gauge.powerOn();
}

TaskEval Power::task() {
    if (millis() > time) {
        time = millis() + Interval;
        auto percentage = gauge.stateOfCharge();
        auto voltage = gauge.cellVoltage();
        state->updateBattery(percentage, voltage);
    }

    return TaskEval::idle();
}

}
