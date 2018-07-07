#include "status.h"
#include "tuning.h"

namespace fk {

TaskEval Status::task() {
    if (millis() - lastTick > StatusInterval) {
        IpAddress4 ip{ state->getStatus().ip };
        auto now = clock.now();
        auto percentage = state->getStatus().batteryPercentage;
        auto voltage = state->getStatus().batteryVoltage;
        FormattedTime nowFormatted{ now };

        loginfof("Status", "%s (%" PRIu32 ") (%.2f%% / %.2fmv) (%" PRIu32 " free) (%s) (%s)",
                 nowFormatted.toString(), now.unixtime(),
                 percentage, voltage, fk_free_memory(),
                 deviceId.toString(), ip.toString());

        lastTick = millis();

        if (percentage < StatusBatteryBlinkThreshold) {
            auto batteryBlinks = (uint8_t)(percentage / 10.0f);
            leds->status(batteryBlinks);
        }
    }
    return TaskEval::idle();
}

}
