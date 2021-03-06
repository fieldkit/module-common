#include "data_messages.h"
#include "rtc.h"
#include "core_state.h"
#include "device_id.h"

namespace fk {

size_t DataRecordMessage::calculateSize() {
    size_t size;

    if (!pb_get_encoded_size(&size, fk_data_DataRecord_fields, &m())) {
        return 0;
    }

    return size + ProtoBufEncodeOverhead;
}

DataRecordMetadataMessage::DataRecordMetadataMessage(CoreState &state, Pool &pool) : DataRecordMessage(pool) {
    auto numberOfSensors = state.numberOfSensors();
    auto sensorIndex = 0;
    auto numberOfModules = 0;
    for (auto m = state.attachedModules(); m != nullptr; m = m->np) {
        for (size_t i = 0; i < m->numberOfSensors; ++i) {
            sensors[sensorIndex].number = i;
            sensors[sensorIndex].name.funcs.encode = pb_encode_string;
            sensors[sensorIndex].name.arg = (void *)m->sensors[i].name;
            sensors[sensorIndex].unitOfMeasure.funcs.encode = pb_encode_string;
            sensors[sensorIndex].unitOfMeasure.arg = (void *)m->sensors[i].unitOfMeasure;

            sensorIndex++;
        }

        modules[numberOfModules].position = numberOfModules;
        modules[numberOfModules].address = m->address;
        modules[numberOfModules].name.funcs.encode = pb_encode_string;
        modules[numberOfModules].name.arg = (void *)m->name;

        numberOfModules++;
    }

    modulesArray = {
        .length = (size_t)numberOfModules,
        .itemSize = sizeof(fk_data_ModuleInfo),
        .buffer = modules,
        .fields = fk_data_ModuleInfo_fields,
    };

    sensorsArray = {
        .length = numberOfSensors,
        .itemSize = sizeof(fk_data_SensorInfo),
        .buffer = sensors,
        .fields = fk_data_SensorInfo_fields,
    };

    deviceIdData = {
        .length = deviceId.length(),
        .buffer = deviceId.toBuffer(),
    };

    auto time = clock.getTime();

    m().metadata.time = time;
    m().metadata.resetCause = fk_system_reset_cause_get();
    m().metadata.deviceId.funcs.encode = pb_encode_data;
    m().metadata.deviceId.arg = (void *)&deviceIdData;
    m().metadata.git.funcs.encode = pb_encode_string;
    m().metadata.git.arg = (void *)firmware_version_get();
    m().metadata.build.funcs.encode = pb_encode_string;
    m().metadata.build.arg = (void *)firmware_build_get();
    m().metadata.sensors.funcs.encode = pb_encode_array;
    m().metadata.sensors.arg = (void *)&sensorsArray;
    m().metadata.modules.funcs.encode = pb_encode_array;
    m().metadata.modules.arg = (void *)&modulesArray;
    m().metadata.firmware.git.funcs.encode = pb_encode_string;
    m().metadata.firmware.git.arg = (void *)firmware_version_get();
    m().metadata.firmware.build.funcs.encode = pb_encode_string;
    m().metadata.firmware.build.arg = (void *)firmware_build_get();

    m().status.time = time;
    m().status.uptime = fk_uptime();
    m().status.battery = 0.0f;
    m().status.memory = 0;
    m().status.busy = 0;
}

DataRecordStatusMessage::DataRecordStatusMessage(CoreState &state, Pool &pool) : DataRecordMessage(pool) {
    m().status.time = clock.getTime();
    m().status.uptime = fk_uptime();
    m().status.battery = state.getStatus().battery.percentage;
    m().status.memory = fk_free_memory();
    m().status.busy = 0;
}

}
