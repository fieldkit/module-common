#ifndef FK_WIFI_H_INCLUDED
#define FK_WIFI_H_INCLUDED

#include <WiFi101.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

#include "active_object.h"
#include "module_controller.h"
#include "wifi_connections.h"
#include "live_data.h"

namespace fk {

struct NetworkSettings {
    const char *ssid;
    const char *password;
    uint16_t port;
};

class Wifi : public ActiveObject {
private:
    NetworkSettings *settings;
    WiFiServer server;
    Listen listen;

public:
    Wifi(NetworkSettings &settings, LiveData &liveData, CoreState &state);

public:
    void begin();

};

}

#endif
