#ifndef FK_TWO_WIRE_TASK_H_INCLUDED
#define FK_TWO_WIRE_TASK_H_INCLUDED

#include "active_object.h"
#include "module_messages.h"
#include "pool.h"

namespace fk {

class TwoWireTask : public Task {
protected:
    QueryMessage query;
    ReplyMessage reply;

private:
    uint8_t address { 0 };
    uint32_t dieAt { 0 };
    uint32_t checkAt { 0 };

public:
    TwoWireTask(const char *name, Pool *pool, uint8_t address) :
        Task(name), query(pool), reply(pool), address(address) {
    }

    void enqueued() override {
        dieAt = 0;
        checkAt = 0;
    }

    TaskEval &task() override;

};

class QueryCapabilities : public TwoWireTask {
    static constexpr char Name[] = "QueryCapabilities";

public:
    QueryCapabilities(Pool *pool, uint8_t address, uint32_t now) : TwoWireTask(Name, pool, address) {
        query.m().type = fk_module_QueryType_QUERY_CAPABILITIES;
        query.m().queryCapabilities.version = FK_MODULE_PROTOCOL_VERSION;
        query.m().queryCapabilities.callerTime = 0;
    }

    size_t numberOfSensors() {
        return reply.m().capabilities.numberOfSensors;
    }

};

class QuerySensorCapabilities : public TwoWireTask {
    static constexpr char Name[] = "QuerySensorCapabilities";

public:
    QuerySensorCapabilities(Pool *pool, uint8_t address, uint8_t sensor) : TwoWireTask(Name, pool, address) {
        query.m().type = fk_module_QueryType_QUERY_SENSOR_CAPABILITIES;
        query.m().querySensorCapabilities.sensor = sensor;
    }

    uint8_t sensor() {
        return query.m().querySensorCapabilities.sensor;
    }

};

}

#endif
