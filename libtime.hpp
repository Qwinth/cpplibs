// version 1.0
#pragma once
#include <cstdlib>
#include <ctime>

#ifdef _WIN32
#include <windows.h>

// void usleep(__int64 usec) { 
//     HANDLE timer; 
//     LARGE_INTEGER ft; 

//     ft.QuadPart = -(10*usec);

//     timer = CreateWaitableTimer(NULL, TRUE, NULL); 
//     SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
//     WaitForSingleObject(timer, INFINITE); 
//     CloseHandle(timer); 
// }

int clock_gettime(int, struct timespec *spec)      //C-file part
{  __int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -=116444736000000000LL;  //1jan1601 to 1jan1970
   spec->tv_sec  =wintime / 10000000LL;           //seconds
   spec->tv_nsec =wintime % 10000000LL*100;      //nano-seconds
   return 0;
}

#else
#include <unistd.h>
#include <sys/timerfd.h>
#endif

#include "libfd.hpp"

namespace Time {
    enum ClockType {
        SYSTEM_CLOCK,
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

    double clock(ClockType type, TimeFraction frac = MICROSECONDS) {
        int64_t raw_time = clock_raw(type);

        return time_cast(raw_time, NANOSECONDS, frac);
    }

    void sleep(double time, TimeFraction frac = MICROSECONDS) {
        double sleep_time = time_cast(time, frac, MICROSECONDS);

        usleep(sleep_time);
    }

    FileDescriptor fd_timer(double interval, TimeFraction frac = MICROSECONDS, ClockType clock = SYSTEM_CLOCK) {
        FileDescriptor fd = timerfd_create(clock, 0);

        double time = time_cast(interval, frac, SECONDS);

        long time_sec = time;
        long time_nsec = time_cast(time - time_sec, SECONDS, NANOSECONDS);

        itimerspec tspec{0};
        tspec.it_value.tv_sec = time_sec;
        tspec.it_value.tv_nsec = time_nsec;
        tspec.it_interval.tv_sec = time_sec;
        tspec.it_interval.tv_nsec = time_nsec;

        timerfd_settime(fd, 0, &tspec, nullptr);

        return fd;
    }
}