#ifndef FK_HARDWARE_H_INCLUDED
#define FK_HARDWARE_H_INCLUDED

#include <cinttypes>

#include "platform.h"
#include "serial_port.h"

/**
 * The FkNat board uses the pin usually mapepd to PIN_LED_TXL for the CS on
 * Serial Flash. Unfortunately, the USB driver code uses this pin to blinky
 * blink while sending and receiving. So as long as that's defined we can't
 * use the serial flash.
 */
#ifdef PIN_LED_TXL
#pragma message "Pin collision with PIN_LED_TXL. Please remove this from variant.h."
#endif // PIN_LED_TXL

#ifdef PIN_LED_RXL
#pragma message "Pin collision with PIN_LED_RXL. Please remove this from variant.h."
#endif // PIN_LED_RXL

namespace fk {

class Hardware {
private:
    static uint32_t modules_on_at_;
    static uint32_t peripherals_on_at_;

public:
    static constexpr uint8_t WIFI_PIN_CS = 7;
    static constexpr uint8_t WIFI_PIN_IRQ = 16;
    static constexpr uint8_t WIFI_PIN_RST = 15;
    static constexpr uint8_t WIFI_PIN_EN = 38;
    static constexpr uint8_t WIFI_PIN_WAKE = 8;

    static constexpr uint8_t RFM95_PIN_CS = 5;
    static constexpr uint8_t RFM95_PIN_RESET = 3;
    static constexpr uint8_t RFM95_PIN_ENABLE = 0;
    static constexpr uint8_t RFM95_PIN_D0 = 2;

    // PIN_LED_TXL;
    static constexpr uint8_t FLASH_PIN_CS = (26u);

    // PIN_LED_RXL;
    static constexpr uint8_t PERIPHERALS_ENABLE_PIN = (25u);

    // A5
    static constexpr uint8_t MODULES_ENABLE_PIN = (19ul); // Was 9ul

    // A4
    static constexpr uint8_t GPS_ENABLE_PIN = (18ul);

    static constexpr uint8_t USER_BUTTON_PIN = 6;

    static constexpr uint8_t SD_PIN_CS = 12;

    static constexpr uint8_t SPI_PIN_MISO = 22; // PIN_SPI_MISO
    static constexpr uint8_t SPI_PIN_MOSI = 23; // PIN_SPI_MOSI
    static constexpr uint8_t SPI_PIN_SCK = 24;  // PIN_SPI_SCK

    static Uart &gpsUart;

    static void disableModules();

    static void enableModules();

    static void cycleModules();

    static void disablePeripherals();

    static void enablePeripherals();

    static bool peripheralsEnabled();

    static void cyclePeripherals();

    static void disableGps();

    static void enableGps();

    static bool modulesReady();

private:
    static void disableSpi();
    static void enableSpi();

    static void disableModuleI2c();
    static void enableModuleI2c();

};

}

#endif
