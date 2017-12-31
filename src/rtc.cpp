#include "rtc.h"
#include "debug.h"

namespace fk {

Clock clock;

void Clock::begin() {
    rtc.begin();
    valid = false;
}

void Clock::setTime(DateTime dt) {
    rtc.setYear(dt.year() - 2000);
    rtc.setMonth(dt.month());
    rtc.setDay(dt.day());
    rtc.setHours(dt.hour());
    rtc.setMinutes(dt.minute());
    rtc.setSeconds(dt.second());
    valid = true;
}

void Clock::setTime(uint32_t unix) {
    if (unix == 0) {
        debugfpln("Clock", "Ignoring invalid time");
        return;
    }

    setTime(DateTime(unix));

    FormattedTime nowFormatted{ clock.now() };
    debugfpln("Clock", "Clock changed: %s (%lu)", nowFormatted.toString(), unix);
}

DateTime Clock::now() {
    return DateTime(rtc.getYear(),
                    rtc.getMonth(),
                    rtc.getDay(),
                    rtc.getHours(),
                    rtc.getMinutes(),
                    rtc.getSeconds());
}

uint32_t Clock::getTime() {
    return now().unixtime();
}

}