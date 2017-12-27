#include <FuelGauge.h>

#include "app_servicer.h"

namespace fk {

static void copy( ScheduledTask &to, fk_app_Schedule &from) {
    to.setSecond(TimeSpec{ (int8_t)from.second.fixed, (int8_t)from.second.interval });
    to.setMinute(TimeSpec{ (int8_t)from.minute.fixed, (int8_t)from.minute.interval });
    to.setHour(TimeSpec{ (int8_t)from.hour.fixed, (int8_t)from.hour.interval });
    to.setDay(TimeSpec{ (int8_t)from.day.fixed, (int8_t)from.day.interval });
}

static void copy(fk_app_Schedule &to, ScheduledTask &from) {
    to.second.fixed = from.getSecond().fixed;
    to.second.interval = from.getSecond().interval;
    to.second.offset = 0;
    to.minute.fixed = from.getMinute().fixed;
    to.minute.interval = from.getMinute().interval;
    to.minute.offset = 0;
    to.hour.fixed = from.getHour().fixed;
    to.hour.interval = from.getHour().interval;
    to.hour.offset = 0;
    to.day.fixed = from.getDay().fixed;
    to.day.interval = from.getDay().interval;
    to.day.offset = 0;
}

AppServicer::AppServicer(LiveData &liveData, CoreState &state, Scheduler &scheduler, FkfsReplies &fileReplies, Pool &pool)
    : Task("AppServicer"), query(&pool), liveData(&liveData), state(&state), scheduler(&scheduler), fileReplies(&fileReplies), pool(&pool) {
}

TaskEval AppServicer::task() {
    handle(query);

    return TaskEval::done();
}

bool AppServicer::handle(MessageBuffer &buffer) {
    this->buffer = &buffer;
    return this->buffer->read(query);
}

void AppServicer::handle(AppQueryMessage &query) {
    switch (query.m().type) {
    case fk_app_QueryType_QUERY_CAPABILITIES: {
        log("Query caps");

        auto *attached = state->attachedModules();
        auto numberOfSensors = state->numberOfSensors();
        auto sensorIndex = 0;
        fk_app_SensorCapabilities sensors[numberOfSensors];
        for (size_t moduleIndex = 0; attached[moduleIndex].address > 0; ++moduleIndex) {
            for (size_t i = 0; i < attached[moduleIndex].numberOfSensors; ++i) {
                sensors[sensorIndex].id = i;
                sensors[sensorIndex].name.funcs.encode = pb_encode_string;
                sensors[sensorIndex].name.arg = (void *)attached[moduleIndex].sensors[i].name;
                sensors[sensorIndex].unitOfMeasure.funcs.encode = pb_encode_string;
                sensors[sensorIndex].unitOfMeasure.arg = (void *)attached[moduleIndex].sensors[i].unitOfMeasure;
                sensors[sensorIndex].frequency = 60;

                log("%d / %d: %s", sensorIndex, numberOfSensors, sensors[sensorIndex].name.arg);

                sensorIndex++;
            }
        }

        pb_array_t sensors_array = {
            .length = numberOfSensors,
            .itemSize = sizeof(fk_app_SensorCapabilities),
            .buffer = sensors,
            .fields = fk_app_SensorCapabilities_fields,
        };

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_CAPABILITIES;
        reply.m().capabilities.version = FK_MODULE_PROTOCOL_VERSION;
        reply.m().capabilities.name.funcs.encode = pb_encode_string;
        reply.m().capabilities.name.arg = (void *)"NOAA-CTD";
        reply.m().capabilities.sensors.funcs.encode = pb_encode_array;
        reply.m().capabilities.sensors.arg = (void *)&sensors_array;

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_DATA_SET:
    case fk_app_QueryType_QUERY_DATA_SETS: {
        log("Query ds");

        fk_app_DataSet dataSets[] = {
            {
                .id = 0,
                .sensor = 0,
                .time = millis(),
                .size = 100,
                .pages = 10,
                .hash = 0,
                .name = {
                    .funcs = {
                        .encode = pb_encode_string,
                    },
                    .arg = (void *)"DS #1",
                },
            },
        };

        pb_array_t data_sets_array = {
            .length = sizeof(dataSets) / sizeof(fk_app_DataSet),
            .itemSize = sizeof(fk_app_DataSet),
            .buffer = &dataSets,
            .fields = fk_app_DataSet_fields,
        };

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_DATA_SETS;
        reply.m().dataSets.dataSets.funcs.encode = pb_encode_array;
        reply.m().dataSets.dataSets.arg = (void *)&data_sets_array;

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_DOWNLOAD_DATA_SET: {
        log("Download ds %d page=%d", query.m().downloadDataSet.id, query.m().downloadDataSet.page);

        uint8_t page[1024] = { 0 };
        pb_data_t data = {
            .length = 1024,
            .buffer = page,
        };

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_DOWNLOAD_DATA_SET;
        reply.m().dataSetData.time = millis();
        reply.m().dataSetData.page = query.m().downloadDataSet.page;
        reply.m().dataSetData.data.funcs.encode = pb_encode_data;
        reply.m().dataSetData.data.arg = (void *)&data;
        reply.m().dataSetData.hash = 0;

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_ERASE_DATA_SET: {
        log("Erase ds");

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_SUCCESS;

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_LIVE_DATA_POLL: {
        log("Live ds (interval = %d)", query.m().liveDataPoll.interval);

        if (query.m().liveDataPoll.interval > 0) {
            liveData->start(query.m().liveDataPoll.interval);
        }
        else {
            liveData->stop();
        }

        auto numberOfReadings = state->numberOfReadings();
        fk_app_LiveDataSample samples[numberOfReadings];

        for (size_t i = 0; i < numberOfReadings; ++i) {
            auto available = state->getReading(i);
            samples[i].sensor = i;
            samples[i].time = available.reading.time;
            samples[i].value = available.reading.value;
        }

        state->clearReadings();

        pb_array_t live_data_array = {
            .length = numberOfReadings,
            .itemSize = sizeof(fk_app_LiveDataSample),
            .buffer = samples,
            .fields = fk_app_LiveDataSample_fields,
        };

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_LIVE_DATA_POLL;
        reply.m().liveData.samples.funcs.encode = pb_encode_array;
        reply.m().liveData.samples.arg = (void *)&live_data_array;

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_CONFIGUE_SCHEDULES: {
        log("Configure schedules");

        auto &readings = scheduler->getTaskSchedule(ScheduleKind::Readings);
        auto &transmission = scheduler->getTaskSchedule(ScheduleKind::Transmission);
        auto &status = scheduler->getTaskSchedule(ScheduleKind::Status);
        auto &location = scheduler->getTaskSchedule(ScheduleKind::Location);

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_SCHEDULES;
        copy(readings, reply.m().schedules.readings);
        copy(transmission, reply.m().schedules.transmission);
        copy(status, reply.m().schedules.status);
        copy(location, reply.m().schedules.location);
        copy(reply.m().schedules.readings, readings);
        copy(reply.m().schedules.transmission, transmission);
        copy(reply.m().schedules.status, status);
        copy(reply.m().schedules.location, location);

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_SCHEDULES: {
        log("Query schedules");

        auto &readings = scheduler->getTaskSchedule(ScheduleKind::Readings);
        auto &transmission = scheduler->getTaskSchedule(ScheduleKind::Transmission);
        auto &status = scheduler->getTaskSchedule(ScheduleKind::Status);
        auto &location = scheduler->getTaskSchedule(ScheduleKind::Location);

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_SCHEDULES;
        copy(reply.m().schedules.readings, readings);
        copy(reply.m().schedules.transmission, transmission);
        copy(reply.m().schedules.status, status);
        copy(reply.m().schedules.location, location);

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_RESET: {
        log("Reset");

        fileReplies->resetAll();

        AppReplyMessage reply(pool);
        reply.m().type = fk_app_ReplyType_REPLY_SUCCESS;

        if (!buffer->write(reply)) {
            log("Error writing reply");
        }

        break;
    }
    case fk_app_QueryType_QUERY_FILES: {
        log("Query files");

        AppReplyMessage reply(pool);
        fileReplies->queryFilesReply(query, reply, *buffer);

        break;
    }
    case fk_app_QueryType_QUERY_DOWNLOAD_FILE: {
        log("Download file (%d / %d)", query.m().downloadFile.id, query.m().downloadFile.page);
        auto started = millis();

        AppReplyMessage reply(pool);
        fileReplies->downloadFileReply(query, reply, *buffer);

        log("Done (%d)", millis() - started);

        break;
    }
    case fk_app_QueryType_QUERY_NETWORK_SETTINGS: {
        networkSettingsReply();

        break;
    }
    case fk_app_QueryType_QUERY_CONFIGURE_NETWORK_SETTINGS: {
        configureNetworkSettings();
        networkSettingsReply();

        break;
    }
    case fk_app_QueryType_QUERY_IDENTITY: {
        identityReply();

        break;
    }
    case fk_app_QueryType_QUERY_CONFIGURE_IDENTITY: {
        configureIdentity();
        identityReply();

        break;
    }
    case fk_app_QueryType_QUERY_STATUS: {
        statusReply();

        break;
    }
    case fk_app_QueryType_QUERY_CONFIGURE_SENSOR:
    default: {
        AppReplyMessage reply(pool);
        reply.error("Unknown query");
        buffer->write(reply);

        break;
    }
    }

    pool->clear();
}

void AppServicer::configureNetworkSettings() {
    log("Configure network settings...");

    pb_array_t *networksArray = (pb_array_t *)query.m().networkSettings.networks.arg;
    auto newNetworks = (fk_app_NetworkInfo *)networksArray->buffer;

    log("Networks: %d", networksArray->length);

    auto settings = state->getNetworkSettings();
    settings.createAccessPoint = query.m().networkSettings.createAccessPoint;
    for (size_t i = 0; i < MaximumRememberedNetworks; ++i) {
        if (i < networksArray->length) {
            settings.networks[i] = NetworkInfo{
                (const char *)newNetworks[i].ssid.arg,
                (const char *)newNetworks[i].password.arg
            };
        }
        else {
            settings.networks[i] = NetworkInfo{};
        }
    }

    state->configure(settings);
}

void AppServicer::networkSettingsReply() {
    log("Network settings");

    auto currentSettings = state->getNetworkSettings();
    fk_app_NetworkInfo networks[MaximumRememberedNetworks];
    for (auto i = 0; i < MaximumRememberedNetworks; ++i) {
        networks[i].ssid.arg = currentSettings.networks[i].ssid;
        networks[i].ssid.funcs.encode = pb_encode_string;
        networks[i].password.arg = currentSettings.networks[i].password;
        networks[i].password.funcs.encode = pb_encode_string;
    }

    pb_array_t networksArray = {
        .length = sizeof(networks) / sizeof(fk_app_NetworkInfo),
        .itemSize = sizeof(fk_app_NetworkInfo),
        .buffer = &networks,
        .fields = fk_app_NetworkInfo_fields,
    };

    AppReplyMessage reply(pool);
    reply.m().type = fk_app_ReplyType_REPLY_NETWORK_SETTINGS;
    reply.m().networkSettings.createAccessPoint = currentSettings.createAccessPoint;
    reply.m().networkSettings.networks.arg = &networksArray;
    reply.m().networkSettings.networks.funcs.encode = pb_encode_array;
    if (!buffer->write(reply)) {
        log("Error writing reply");
    }
}

void AppServicer::statusReply() {
    log("Status");

    FuelGauge fuelGage;
    AppReplyMessage reply(pool);
    reply.m().type = fk_app_ReplyType_REPLY_STATUS;
    reply.m().status.uptime = millis();
    reply.m().status.batteryPercentage = fuelGage.stateOfCharge();
    reply.m().status.batteryVoltage = fuelGage.cellVoltage();
    reply.m().status.gpsHasFix = false;
    reply.m().status.gpsSatellites = 0;
    if (!buffer->write(reply)) {
        log("Error writing reply");
    }
}

void AppServicer::configureIdentity() {
    DeviceIdentity identity{
        (const char *)query.m().identity.device.arg,
        (const char *)query.m().identity.stream.arg,
    };

    state->configure(identity);
}

void AppServicer::identityReply() {
    log("Identity");

    auto identity = state->getIdentity();
    AppReplyMessage reply(pool);
    reply.m().type = fk_app_ReplyType_REPLY_IDENTITY;
    reply.m().identity.device.arg = identity.device;
    reply.m().identity.stream.arg = identity.stream;
    if (!buffer->write(reply)) {
        log("Error writing reply");
    }
}

}
