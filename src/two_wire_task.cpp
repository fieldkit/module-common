#include "two_wire_task.h"
#include "tuning.h"

namespace fk {

StreamTwoWireTask::StreamTwoWireTask(const char *name, TwoWireBus &bus, Reader &outgoing, Writer &incoming, uint8_t address) :
    Task(name), bus(&bus), outgoing(&outgoing), incoming(&incoming), address(address) {
}

StreamTwoWireTask::StreamTwoWireTask(const char *name, TwoWireBus &bus, Writer &incoming, uint8_t address) :
    Task(name), bus(&bus), outgoing(nullptr), incoming(&incoming), address(address) {
}

void StreamTwoWireTask::enqueued() {
    dieAt = 0;
    checkAt = 0;
    bytesReceived = 0;
    doneAt = 0;
    if (outgoing == nullptr) {
        checkAt = millis() + 200;
    }
}

TaskEval StreamTwoWireTask::task() {
    if (checkAt > 0 && millis() < checkAt) {
        return TaskEval::idle();
    }

    if (outgoing != nullptr) {
        if (dieAt == 0) {
            return send();
        }
    }
    else {
        dieAt = millis() + MaximumTwoWireReply;
    }

    if (millis() > dieAt) {
        log("Error: No reply in time.");
        return TaskEval::error();
    }

    return receive();
}

TaskEval StreamTwoWireTask::send() {
    uint8_t buffer[SERIAL_BUFFER_SIZE];
    auto bytes = outgoing->read(buffer, sizeof(buffer));
    if (bytes < 0) {
        return TaskEval::done();
    }
    if (bytes == 0) {
        return TaskEval::idle();
    }

    if (!bus->send(address, buffer, bytes)) {
        log("Error: Unable to send.");
        return TaskEval::error();
    }

    dieAt = millis() + MaximumTwoWireReply;
    // They won't be ready yet, check back soon, though.
    checkAt = millis() + 100;

    return TaskEval::idle();
}

TaskEval StreamTwoWireTask::receive() {
    uint8_t buffer[SERIAL_BUFFER_SIZE];
    bytesReceived = bus->receive(address, buffer, sizeof(buffer));
    if (bytesReceived == 0) {
        log("Error: Empty reply.");
        return TaskEval::error();
    }

    auto wrote = incoming->write(buffer, bytesReceived);
    if (wrote != (int32_t)bytesReceived) {
        log("Error: Out of buffer space (%lu != %d)", wrote, bytesReceived);
        return TaskEval::error();
    }

    // dieAt = 0;

    return TaskEval::done();
}

}
