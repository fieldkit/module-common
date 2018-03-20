#ifndef FK_UTILS_H_INCLUDED
#define FK_UTILS_H_INCLUDED

#include <WiFi101.h>
#include <IPAddress.h>

#include <cstdint>
#include <cstdio>

namespace fk {

class IpAddress4 {
private:
    char str[3 * 4 + 4] = { 0 };
    union {
        uint8_t bytes[4];
        uint32_t dword;
    } a;

public:
    IpAddress4() {
    }

    IpAddress4(uint32_t ip) {
        a.dword = ip;
    }

public:
    const char *toString() {
        snprintf(str, sizeof(str), "%d.%d.%d.%d", a.bytes[0], a.bytes[1], a.bytes[2], a.bytes[3]);
        return str;
    }
};

class CachedDnsResolution {
private:
    ::IPAddress cachedAddress;

public:
    bool cached(const char *hostname) {
        if ((uint32_t)cachedAddress == (uint32_t)0) {
            if (!WiFi.hostByName(hostname, cachedAddress)) {
                return false;
            }
        }
        return true;
    }

    uint32_t ip() {
        return cachedAddress;
    }

};

struct Url {
public:
    char *server{ nullptr };
    char *path{ nullptr };
    uint16_t port{ 80 };

public:
    Url(char *url) {
        for (auto p = url; p[0] != 0 && p[1] != 0; ++p) {
            if (server == nullptr && p[0] == '/' && p[1] == '/') {
                p += 2;
                server = p;
            } else if (server != nullptr && p[0] == ':') {
                p[0] = 0;
                auto portBegin = ++p;
                for ( ; p[0] != 0; ++p) {
                    if (p[0] == '/') {
                        p[0] = 0;
                        port = atoi(portBegin);
                        path = p + 1;
                        break;
                    }
                }
                break;
            } else if (server != nullptr && p[0] == '/') {
                p[0] = 0;
                path = p + 1;
                break;
            }
        }
    }
};

class PrintSizeCalculator : public Print {
private:
    size_t size{ 0 };

public:
    size_t getSize() {
        return size;
    }

    size_t write(uint8_t c) override {
        size++;
        return 1;
    }
};

class BufferPrinter : public Print {
public:
    char *buffer;
    size_t size;
    size_t pos{ 0 };

public:
    BufferPrinter(char *buffer, size_t size) : buffer(buffer), size(size) {
    }

    size_t write(uint8_t c) override {
        if (pos < size) {
            buffer[pos++] = c;
            buffer[pos] = 0;
            return 1;
        }
        return 0;
    }

};

class HttpResponseParser {
    static constexpr size_t MaxStatusCodeLength = 4;

private:
    // This only needs to be big enough to hold a status code.
    char buffer[MaxStatusCodeLength];
    uint8_t spacesSeen{ 0 };
    uint8_t pos{ 0 };
    uint16_t statusCode{ 0 };

public:
    void begin();
    void write(uint8_t c);

public:
    uint16_t getStatusCode() {
        return statusCode;
    }

};

class HttpResponseWriter {
private:
    WiFiClient &wcl;

public:
    explicit HttpResponseWriter(WiFiClient &wcl);

public:
    void writeHeaders(Url &url, const char *contentType, uint32_t contentLength);

};

const char *getWifiStatus(uint8_t status);

const char *getWifiStatus();

constexpr size_t MaximumSerialNumberLengthDwords = 4;
constexpr size_t MaximumSerialNumberLengthBytes = MaximumSerialNumberLengthDwords * 4;

class SerialNumber {
private:
    uint32_t values[MaximumSerialNumberLengthDwords];
    char buffer[37];

public:
    SerialNumber();

public:
    const char *toString();
    uint32_t *toInts() {
        return values;
    }

};

uint32_t crc32_update(uint32_t crc, uint8_t data);

uint32_t crc32_checksum(uint8_t *data, size_t size);

}

#endif
