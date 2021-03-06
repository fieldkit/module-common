#include "gather_readings.h"
#include "leds.h"
#include "tuning.h"

namespace fk {

GatherReadings::GatherReadings(uint32_t remaining, CoreState &state, Leds &leds, ModuleCommunications &communications) :
    Task("GatherReadings"), remaining(remaining), state(&state), leds(&leds), protocol(communications) {
}

void GatherReadings::enqueued() {
    if (!peripherals.twoWire1().tryAcquire(this)) {
        log("TwoWire unavailable.");
        return;
    }

    if (state->numberOfModules(fk_module_ModuleType_SENSOR) == 0) {
        log("No attached modules.");
        return;
    }

    beginTakeReading.remaining(remaining);

    state->takingReadings();
    leds->notifyReadingsBegin();
    protocol.push(8, beginTakeReading);
    startedAt = 0;
}

TaskEval GatherReadings::task() {
    if (!protocol.isBusy()) {
        return TaskEval::done();
    }

    if (startedAt == 0) {
        startedAt = fk_uptime();
    }

    auto finished = protocol.handle();
    if (finished) {
        if (finished.error()) {
            return error(finished);
        }
        else {
            return done(finished);
        }
    }

    return TaskEval::idle();
}

TaskEval GatherReadings::done(ModuleProtocolHandler::Finished &finished) {
    if (finished.is(beginTakeReading)) {
        auto delay = 300;
        if (beginTakeReading.getBackoff() > 0) {
            log("Using backoff of %lu", beginTakeReading.getBackoff());
            delay = beginTakeReading.getBackoff();
        }
        protocol.push(8, queryReadingStatus, delay);
    }
    else if (finished.is(queryReadingStatus)) {
        if (queryReadingStatus.isBegin() || queryReadingStatus.isBusy()) {
            auto delay = 300;
            if (queryReadingStatus.getBackoff() > 0) {
                log("Using backoff of %lu", queryReadingStatus.getBackoff());
                delay = queryReadingStatus.getBackoff();
            }
            protocol.push(8, queryReadingStatus, delay);
        }
        else if (queryReadingStatus.isDone()) {
            state->merge(8, *finished.reply);
            protocol.push(8, queryReadingStatus);
        }
        else {
            log("Readings done after %lums", fk_uptime() - startedAt);
            return TaskEval::done();
        }
    }

    retries = 0;

    return TaskEval::idle();
}

TaskEval GatherReadings::error(ModuleProtocolHandler::Finished &finished) {
    if (finished.is(beginTakeReading)) {
        return TaskEval::error();
    } else if (finished.is(queryReadingStatus)) {
        return TaskEval::error();
    }
    return TaskEval::idle();
}

void GatherReadings::error() {
    if (peripherals.twoWire1().isOwner(this)) {
        peripherals.twoWire1().release(this);
    }
    leds->notifyReadingsDone();
}

void GatherReadings::done() {
    if (peripherals.twoWire1().isOwner(this)) {
        peripherals.twoWire1().release(this);
    }
    leds->notifyReadingsDone();
}

}
