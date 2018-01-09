#include <SPI.h>

#include "wifi.h"
#include "utils.h"
#include "hardware.h"

namespace fk {

constexpr uint32_t WifiAwakenInterval = 1000 * 60 * 1;

TaskEval ConnectToWifiAp::task() {
    if (networkNumber >= MaximumRememberedNetworks) {
        networkNumber = 0;
        log("No more networks (%s)", getWifiStatus());
        return TaskEval::error();
    }

    auto settings = state->getNetworkSettings();
    auto network = settings.networks[networkNumber];

    if (network.ssid[0] == 0) {
        log("N[%d] No network configured (%s)", networkNumber, getWifiStatus());
        networkNumber++;
        return TaskEval::yield();
    }

    log("N[%d] Connecting to AP '%s'... (%s)", networkNumber, network.ssid, getWifiStatus());
    if (WiFi.begin(network.ssid, network.password) != WL_CONNECTED) {
        log("N[%d] Failed (%s)", networkNumber, getWifiStatus());
        networkNumber++;
        return TaskEval::yield();
    }

    IpAddress4 ip{ WiFi.localIP() };
    log("N[%d] Connected (%s) (%s)", networkNumber, getWifiStatus(), ip.toString());

    return TaskEval::done();
}

TaskEval CreateWifiAp::task() {
    auto name = "FK-HELP";

    log("Creating AP '%s'... (%s)", name, getWifiStatus());
    auto status = WiFi.beginAP(name);
    if (status != WL_AP_LISTENING) {
        return TaskEval::error();
    }

    return TaskEval::done();
}

Wifi::Wifi(CoreState &state, AppServicer &servicer)
    : ActiveObject("Wifi"), state(&state), connectToWifiAp(state), createWifiAp(state), listen(ServerPort, servicer) {
}

void Wifi::begin() {
    WiFi.setPins(Hardware::WIFI_PIN_CS, Hardware::WIFI_PIN_IRQ, Hardware::WIFI_PIN_RST, Hardware::WIFI_PIN_EN);

    if (WiFi.status() == WL_NO_SHIELD) {
        log("Error: no wifi");
        return;
    }
}

void Wifi::done(Task &task) {
}

void Wifi::error(Task &task) {
    if (areSame(task, connectToWifiAp)) {
        push(createWifiAp);
    }
    else {
        push(delay);
    }
}

bool Wifi::readyToServe() {
    return status == WL_CONNECTED || status == WL_AP_CONNECTED;
}

bool Wifi::isListening() {
    return status == WL_AP_LISTENING;
}

void Wifi::ensureDisconnected() {
    if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_IDLE_STATUS) {
        return;
    }

    WiFi.disconnect();
    while (!(WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_IDLE_STATUS)) {
        ::delay(1000);
        log("Disconnecting(%s)...", getWifiStatus());
    }

    log("Disconnected(%s)", getWifiStatus());
}

void Wifi::idle() {
    if (disabled) {
        if (millis() - lastActivityAt > WifiAwakenInterval) {
            log("Enabling...");
            begin();
            log("Enabled");
            disabled = false;

        }
        return;
    }

    uint8_t newStatus = WiFi.status();
    if (newStatus != status) {
        log("Changed: %s", getWifiStatus());
        status = newStatus;
        state->updateIp(WiFi.localIP());
    }

    if (status == WL_NO_SHIELD) {
        return;
    }

    auto settings = state->getNetworkSettings();
    if (version == settings.version) {
        if (readyToServe()) {
            service(listen);
        }
        if (listen.inactive()) {
            if (isListening() || readyToServe())  {
                WiFi.end();
                lastActivityAt = millis();
                disabled = true;
                version = 0;
                state->updateIp(0);

                // Allow me to explain:
                // I was seeing this very strange problem where after a Disable
                // the WDT would kick off. It was always preceeded by fkfs
                // activity and so things eventually led me to the SPI bus code.
                // I was inside of sd_raw, in the code for sd_raw_command.
                // There's a call to sd_raw_flush and that's where the hang
                // occured. In there it waits until the "Data Register is Empty"
                // (ATSAMD SerCOM.CPP line 305) I'm assuming there's some kind
                // of flush that doesn't happen. This "reset" fixes the problem.
                // If I do this, and don't do the call to maxLowPowerMode
                // anymore and also move the log call you see below to after the
                // reset of the bus things work more often. There is still a WDT
                // reset occuring, though just doesn't seem to happen around here.
                SPI.end();
                SPI.begin();

                log("Disabled");
            }
        }
        return;
    }

    log("New configuration...");
    version = settings.version;
    listen.end();
    ensureDisconnected();
    cancel();

    push(connectToWifiAp);
}

}
