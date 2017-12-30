#include <Arduino.h>

#include "leds.h"

namespace fk {

TaskEval Blinker::task() {
    switch (kind) {
    case BlinkerKind::Fatal:  {
        if (nextChange < millis()) {
            auto value = !digitalRead(A1);
            digitalWrite(A3, value);
            digitalWrite(A4, value);
            digitalWrite(A5, value);
            nextChange = millis() + 100;
        }
        break;
    }
    case BlinkerKind::NoAttachedModules:  {
        if (nextChange < millis()) {
            digitalWrite(A3, !digitalRead(A3));
            nextChange = millis() + 500;
        }
        break;
    }
    case BlinkerKind::Alive:  {
        break;
    }
    case BlinkerKind::Reading:  {
        digitalWrite(A4, HIGH);
        break;
    }
    default: {
        break;
    }
    }

    return TaskEval::idle();
}

void Blinker::clear() {
    switch (kind) {
    case BlinkerKind::Reading:  {
        digitalWrite(A4, LOW);
        break;
    }
    default: {
        break;
    }
    }

    kind = BlinkerKind::None;
}

Leds::Leds() : ActiveObject("LEDs") {
}

void Leds::setup() {
    // Note that at least one board right now uses 13 for other purposes so
    // ths should be done before that happens.
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);

    pinMode(A3, OUTPUT);
    pinMode(A4, OUTPUT);
    pinMode(A5, OUTPUT);

    all(LOW);
}

void Leds::idle() {
    for (auto i = 0; i < MaximumBlinkers; ++i) {
        blinkers[i].task();
    }
}

void Leds::all(bool value) {
    digitalWrite(A3, value);
    digitalWrite(A4, value);
    digitalWrite(A5, value);
}

void Leds::alive() {
    digitalWrite(A5, HIGH);
    delay(100);
    digitalWrite(A5, LOW);
}

void Leds::push(BlinkerKind kind) {
    for (auto i = 0; i < MaximumBlinkers; ++i) {
        if (blinkers[i].isIdle()) {
            log("Blinker #%d is %d", i, kind);
            blinkers[i] = Blinker{ kind };
            return;
        }
    }

    log("No available blinkers for %d", kind);
}

void Leds::clear(BlinkerKind kind) {
    for (auto i = 0; i < MaximumBlinkers; ++i) {
        if (blinkers[i].isOfKind(kind)) {
            log("Clearing blinker #%d", i);
            blinkers[i].clear();
        }
    }
}

void Leds::fatal() {
    push(BlinkerKind::Fatal);
}

void Leds::beginReading() {
    push(BlinkerKind::Reading);
}

void Leds::doneReading() {
    clear(BlinkerKind::Reading);
}

void Leds::noAttachedModules() {
    push(BlinkerKind::NoAttachedModules);
}

}
