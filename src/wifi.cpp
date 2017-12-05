#include "wifi.h"

namespace fk {

Wifi::Wifi(NetworkSettings &settings, AppServicer &servicer)
    : ActiveObject("Wifi", listen), settings(&settings), server(settings.port),
      listen(server, servicer) {
}

void Wifi::begin() {
    WiFi.setPins(8, 7, 4, 2);

    if (WiFi.status() == WL_NO_SHIELD) {
        log("No wifi");
        return;
    } else {
        if (false) {
            log("Creating AP");

            auto status = WiFi.beginAP(settings->ssid);
            if (status != WL_AP_LISTENING) {
                log("Error creating AP");
                return;
            }
        } else {
            log("Connecting to AP");
            WiFi.begin(settings->ssid, settings->password);
            log("Waiting on connection...");
        }
    }

    server.begin();
}

}
