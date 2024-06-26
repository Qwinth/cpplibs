// version 1.0
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include "windowsHeader.hpp"

#ifndef _WINSOCKAPI_
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#define mpoll(a, b, c) (WSAPoll(a, b, c))

#endif

#elif __linux__
#include <sys/poll.h>
#endif

#pragma once

struct pollEvent {
    int fd;
    short event;
};

class Poller {
    std::vector<pollfd> fds;
public:
    void addDescriptor(int fd, short events) {
        fds.push_back({fd, events, 0});
    }

    void removeDescriptor(int fd) {
        for (size_t i = 0; i < fds.size(); i++) if (fds[i].fd == fd) { fds.erase(fds.begin() + i); break; }
    }

    std::vector<pollEvent> poll(int timeout = -1) {
        std::vector<pollEvent> events;

#ifdef _WIN32
        int num_events = mpoll(fds.data(), fds.size(), timeout);
#elif __linux__
        int num_events = ::poll(fds.data(), fds.size(), timeout);
#endif

        if (num_events) {
            int n_events = 0;

            for (pollfd& i : fds) {
                if (i.revents) {
                    events.push_back({i.fd, i.revents});
                    i.revents = 0;
                    n_events++;
                }

                if (n_events == num_events) break;
            }
        }

        return events;
    }
};