#include "core_module.h"
#include "hardware.h"
#include "device_id.h"
#include "status.h"

namespace fk {

void CoreModule::begin() {
    MainServicesState::services(mainServices);
    WifiServicesState::services(wifiServices);

    fsm_list::start();

    pinMode(Hardware::SD_PIN_CS, OUTPUT);
    pinMode(Hardware::WIFI_PIN_CS, OUTPUT);
    pinMode(Hardware::RFM95_PIN_CS, OUTPUT);
    pinMode(Hardware::FLASH_PIN_CS, OUTPUT);

    digitalWrite(Hardware::SD_PIN_CS, HIGH);
    digitalWrite(Hardware::WIFI_PIN_CS, HIGH);
    digitalWrite(Hardware::RFM95_PIN_CS, HIGH);
    digitalWrite(Hardware::FLASH_PIN_CS, HIGH);

    leds.setup();
    watchdog.setup();
    bus.begin();
    power.setup();
    button.enqueued();

    fk_assert(deviceId.initialize(bus));

    SerialNumber serialNumber;
    loginfof("Core", "Serial(%s)", serialNumber.toString());
    loginfof("Core", "DeviceId(%s)", deviceId.toString());
    loginfof("Core", "Hash(%s)", firmware_version_get());
    loginfof("Core", "Build(%s)", firmware_build_get());

    delay(10);

    #ifdef FK_DISABLE_FLASH
    loginfof("Core", "Flash memory disabled");
    #else
    fk_assert(flashStorage.initialize(Hardware::FLASH_PIN_CS));
    delay(100);
    #endif

    #ifdef FK_ENABLE_RADIO
    if (!radioService.setup(deviceId)) {
        loginfof("Core", "Radio service unavailable");
    }
    else {
        loginfof("Core", "Radio service ready");
    }
    #else
    loginfof("Core", "Radio service disabled");
    #endif

    fk_assert(fileSystem.setup());

    watchdog.started();

    bus.begin();

    state.setDeviceId(deviceId.toString());

    clock.begin();

    FormattedTime nowFormatted{ clock.now() };
    loginfof("Core", "Now: %s", nowFormatted.toString());
    loginfof("Core", "API: %s", WifiApiUrlIngestionStream);

    state.started();
}

void CoreModule::run() {
    while (true) {
        CoreDevice::current().task();
    }
}

}

