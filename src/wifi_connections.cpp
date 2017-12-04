#include "wifi_connections.h"
#include "wifi_message_buffer.h"
#include "utils.h"

namespace fk {

constexpr char Listen::Name[];

HandleConnection::HandleConnection(WiFiClient wcl, ModuleController &modules, CoreState &state, Pool &pool)
    : AppServicer("HandleConnection", modules, state, pool), wcl(wcl) {
}

constexpr uint32_t ConnectionTimeout = 5000;

TaskEval HandleConnection::task() {
    if (dieAt == 0) {
        dieAt = millis() + ConnectionTimeout;
    } else if (millis() > dieAt) {
        wcl.stop();
        log("Connection timed out.");
        return TaskEval::error();
    }
    if (wcl.available()) {
        WifiMessageBuffer incoming;
        auto bytesRead = incoming.read(wcl);
        if (bytesRead > 0) {
            log("Read %d bytes", bytesRead);
            if (!read(incoming)) {
                wcl.stop();
                log("Error parsing query");
                return TaskEval::error();
            } else {
                auto e = AppServicer::task();
                if (!e.isIdle()) {
                    auto &buffer = outgoingBuffer();
                    auto bytesWritten = wcl.write(buffer.ptr(), buffer.position());
                    log("Wrote %d bytes", bytesWritten);
                    fk_assert(bytesWritten == buffer.position());
                    buffer.clear();
                    wcl.stop();
                    return e;
                }
            }
        }
    }

    return TaskEval::idle();
}

Listen::Listen(WiFiServer &server, ModuleController &modules, CoreState &state)
    : Task(Name), pool("WifiService", 128), server(&server), modules(&modules), state(&state),
      handleConnection(WiFiClient(), modules, state, pool) {
}

TaskEval Listen::task() {
    if (WiFi.status() == WL_AP_CONNECTED || WiFi.status() == WL_CONNECTED) {
        if (!connected) {
            IpAddress4 ip{ WiFi.localIP() };
            log("Connected ip: %s", ip.toString());
            connected = true;
        }
    } else {
        if (connected) {
            log("Disconnected");
            connected = false;
        }
    }

    if (connected) {
        // WiFiClient is 1480 bytes. Only has one buffer of the size
        // SOCKET_BUFFER_TCP_SIZE. Where SOCKET_BUFFER_TCP_SIZE is 1446.
        auto wcl = server->available();
        if (wcl) {
            log("Accepted!");
            pool.clear();
            handleConnection = HandleConnection{ wcl, *modules, *state, pool };
            return TaskEval::pass(handleConnection);
        }
    }

    return TaskEval::idle();
}

}
