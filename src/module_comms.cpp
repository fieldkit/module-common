#include "module_comms.h"

namespace fk {

ModuleCommunications::ModuleCommunications(TwoWireBus &bus, TaskQueue &queue, Pool &pool) :
    Task("ModuleCommunications"), bus(&bus), queue(&queue), pool(&pool), query(pool), reply(pool), twoWireTask("ModuleTwoWire", bus, outgoing.getReader(), incoming.getWriter(), 0) {
}

void ModuleCommunications::enqueue(uint8_t destination, ModuleQuery &mq) {
    fk_assert(!hasQuery);

    address = destination;
    pending = &mq;
    hasQuery = true;
}

bool ModuleCommunications::available() {
    return hasReply;
}

ModuleReplyMessage &ModuleCommunications::dequeue() {
    fk_assert(hasReply);
    hasReply = false;
    return reply;
}

TaskEval ModuleCommunications::task() {
    if (address > 0) {
        if (hasQuery) {
            incoming.clear();
            outgoing.clear();
            pool->clear();
            query.clear();
            pending->query(query);

            auto protoWriter = ProtoBufMessageWriter{ outgoing.getWriter() };
            protoWriter.write(fk_module_WireMessageQuery_fields, &query.m());
            outgoing.getWriter().close();

            twoWireTask = TwoWireTask{ pending->name(), *bus, outgoing.getReader(), incoming.getWriter(), address };
            queue->prepend(twoWireTask);

            hasQuery = false;
            hasReply = false;
        }

        if (twoWireTask.completed()) {
            if (twoWireTask.received() > 0) {
                auto protoReader = ProtoBufMessageReader{ incoming.getReader() };

                if (!protoReader.read<SERIAL_BUFFER_SIZE>(fk_module_WireMessageReply_fields, reply.forDecode())) {
                    log("Error: Unable to read reply.");
                }
                else {
                    if (reply.m().type == fk_module_ReplyType_REPLY_RETRY) {
                        log("Retry!");
                        incoming.clear();
                        outgoing.clear();
                        pool->clear();
                        twoWireTask = TwoWireTask{ pending->name(), *bus, incoming.getWriter(), address };
                        queue->prepend(twoWireTask);
                        return TaskEval::idle();
                    }
                    else {
                        hasReply = true;
                    }
                }
            }
            address = 0;
        }
    }

    return TaskEval::idle();
}

ModuleProtocolHandler::ModuleProtocolHandler(ModuleCommunications &communications) : communications(&communications) {
}

void ModuleProtocolHandler::push(uint8_t address, ModuleQuery &query, uint32_t delay) {
    active = Queued{ };
    pending = Queued{ address, &query, delay > 0 ? millis() + delay : 0 };
}

bool ModuleProtocolHandler::isBusy() {
    return active || pending;
}

ModuleProtocolHandler::Finished ModuleProtocolHandler::handle() {
    if (!communications->busy()) {
        if (pending.query != nullptr) {
            if (millis() > pending.delay) {
                communications->enqueue(pending.address, *pending.query);
                active = pending;
                pending = Queued{};
            }
        }
    }

    if (active.query != nullptr) {
        if (communications->available()) {
            ModuleQuery *finished = active.query;
            auto &reply = communications->dequeue();

            active.query->reply(reply);
            active = Queued{};

            return Finished { finished, &reply };
        }
        if (!communications->busy()) {
            ModuleQuery *finished = active.query;

            active = Queued{};

            return Finished { finished, nullptr };
        }
    }

    return Finished{ nullptr, nullptr };
}

}
