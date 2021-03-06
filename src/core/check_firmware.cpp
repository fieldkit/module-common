#include <WiFi101.h>

#include "check_firmware.h"
#include "configuration.h"

#include "firmware_health_check.h"
#include "transmit_files.h"
#include "upgrade_module_firmware.h"
#include "device_id.h"

#include "http_response_parser.h"
#include "http_response_writer.h"

#include "firmware_header.h"
#include "firmware_storage.h"

namespace fk {

class CheckFirmware : public WifiState {
private:
    FirmwareBank bank_;
    const char *module_;
    uint32_t compiled_{ 0 };

public:
    CheckFirmware() : module_(nullptr) {
    }

    CheckFirmware(FirmwareBank bank, const char *module, uint32_t compiled) : bank_(bank), module_(module), compiled_(compiled) {
    }

public:
    const char *name() const override {
        return "CheckFirmware";
    }

public:
    void task() override;

private:
    void check();
};

void CheckAllAttachedFirmware::task() {
    auto state = services().state;

    if (index_ < state->numberOfModules()) {
        auto module = state->getModuleByIndex(index_);
        index_++;
        transit_into<CheckFirmware>(FirmwareBank::Incoming, module->module, module->compiled);
    }
    else if (index_ == state->numberOfModules()) {
        index_++;
        transit_into<CheckFirmware>(FirmwareBank::Pending, configuration.module_name, firmware_compiled_get());
    }
    else {
        transit<WifiTransmitFiles>();
    }
}

void CheckFirmware::task() {
    if (module_ == nullptr) {
        log("No module given");
        back();
        return;
    }

    check();
}

void CheckFirmware::check() {
    Url parsed(configuration.wifi.firmware_url, deviceId.toString(), module_);
    FirmwareStorage firmwareStorage{ *services().flashState, *services().flashFs };
    HttpResponseParser parser;
    WiFiClient wcl;

    firmware_header_t header;
    firmwareStorage.header(bank_, header);

    log("GET http://%s:%d/%s", parsed.server, parsed.port, parsed.path);

    if (wcl.connect(parsed.server, parsed.port)) {
        OutgoingHttpHeaders headers{
            nullptr,
            firmware_version_get(),
            firmware_build_get(),
            compiled_,
            deviceId.toString(),
            header.version != FIRMWARE_VERSION_INVALID ? header.etag : nullptr,
        };
        HttpHeadersWriter httpWriter(&wcl);
        HttpResponseParser httpParser;

        log("Connected!");

        httpWriter.writeHeaders(parsed, "GET", headers);

        auto writer = (lws::Writer *)nullptr;
        auto activity = fk_uptime();
        auto total = (uint32_t)0;

        while (wcl.connected() || wcl.available()) {
            services().alive();

            if (fk_uptime() - activity > WifiConnectionTimeout) {
                error("Failed! (%lu)", fk_uptime() - activity);
                break;
            }

            while (wcl.available()) {
                if (httpParser.reading_header()) {
                    httpParser.write(wcl.read());
                }
                else {
                    uint8_t buffer[WifiSocketBufferSize];

                    auto bytes = wcl.read(buffer, sizeof(buffer));
                    if (bytes > 0) {
                        if (httpParser.status_code() == 200) {
                            if (writer == nullptr) {
                                writer = firmwareStorage.write();

                                firmware_header_t header;
                                memset(&header, 0, sizeof(firmware_header_t));
                                header.version = 1;
                                header.time = 0;
                                header.size = httpParser.content_length();
                                strncpy(header.etag, httpParser.etag(), sizeof(header.etag) - 1);

                                auto headerBytes = writer->write((uint8_t *)&header, sizeof(firmware_header_t));
                                if (headerBytes != sizeof(firmware_header_t)) {
                                    error("Writing header failed.");
                                }
                            }
                            writer->write(buffer, bytes);
                            total += bytes;
                            activity = fk_uptime();
                        }
                    }
                }
            }
        }

        if (total > 0 && writer != nullptr) {
            if (total != httpParser.content_length()) {
                error("Status: %d (Size mismatch!) total=%lu expected=%lu etag='%s'", httpParser.status_code(),
                      total, httpParser.content_length(), httpParser.etag());
            }
            else {
                log("Status: %d total=%lu etag='%s'", httpParser.status_code(), total, httpParser.etag());

                writer->close();

                if (firmwareStorage.verify(firmwareStorage.beginningOfOpenFile(), total + sizeof(firmware_header_t))) {
                    firmwareStorage.update(bank_, writer);

                    if (bank_ == FirmwareBank::Pending) {
                        transit_into<FirmwareSelfFlash>();
                    }
                    else {
                        transit<UpgradeModuleFirmware>();
                    }
                }

                return;
            }
        }
        else {
            log("Status: %d", httpParser.status_code());
        }

        wcl.stop();

        log("Done!");
    }
    else {
        error("Connection failed!");
    }

    back();
}

}
