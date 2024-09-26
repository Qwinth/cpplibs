// version 1.0
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#pragma once

namespace Time {
    enum ClockType {
        SYTEM_CLOCK,
        TIMER_CLOCK
    };

    enum TimeFraction {
        HOURS,
        MINUTES,
        SECONDS,
        MILLISECONDS,
        MICROSECONDS,
        NANOSECONDS
    };

    int64_t to_nanoseconds(double value, TimeFraction frac) {
        switch (frac) {
            case HOURS: return value * 3600000000000LL;
            case MINUTES: return value * 60000000000LL;
            case SECONDS: return value * 1000000000LL;
            case MILLISECONDS: return value * 1000000LL;
            case MICROSECONDS: return value * 1000LL;
            case NANOSECONDS: return static_cast<int64_t>(value);
            default: return 0;
        }
    }

    double from_nanoseconds(int64_t nanoseconds, TimeFraction frac) {
        switch (frac) {
            case HOURS: return nanoseconds / 3600000000000.0;
            case MINUTES: return nanoseconds / 60000000000.0;
            case SECONDS: return nanoseconds / 1000000000.0;
            case MILLISECONDS: return nanoseconds / 1000000.0;
            case MICROSECONDS: return nanoseconds / 1000.0;
            case NANOSECONDS: return static_cast<double>(nanoseconds);
            default: return 0.0;
        }
    }

    double time_cast(double time, TimeFraction from, TimeFraction to) {
        return from_nanoseconds(to_nanoseconds(time, from), to);
    }

    int64_t clock_raw(ClockType type) {
        timespec ts;
        clock_gettime(type, &ts);

        return ts.tv_sec * 1'000'000'000 + ts.tv_nsec;
    }

    double clock(ClockType type, TimeFraction frac) {
        int64_t raw_time = clock_raw(type);

        return time_cast(raw_time, NANOSECONDS, frac);
    }

    void sleep(double time, TimeFraction frac = MICROSECONDS) {
        double sleep_time = time_cast(time, frac, MICROSECONDS);

        usleep(sleep_time);
    }
}