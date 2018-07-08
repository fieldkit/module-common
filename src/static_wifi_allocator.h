#ifndef FK_STATIC_WIFI_ALLOCATOR_H_INCLUDED
#define FK_STATIC_WIFI_ALLOCATOR_H_INCLUDED

#include <type_traits>

#include <WiFiSocket.h>

#include "tuning.h"

namespace fk {

class StaticWiFiAllocator : public WiFiAllocator {
private:
    static constexpr size_t NumberOfBuffers = 2;
    typename std::aligned_storage<sizeof(uint8_t) * WifiSocketBufferSize, alignof(uint8_t)>::type data[NumberOfBuffers];
    bool available[NumberOfBuffers] = { true, true };

public:
    void *malloc(size_t size) override;
    void free(void *ptr) override;

};

}

#endif
