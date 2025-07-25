// version 1.1
#pragma once
#include <vector>
#include <algorithm>
#include <mutex>

#ifdef _WIN32
#include "windowsHeader.hpp"

#ifndef _WINSOCKAPI_
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#endif

#define mpoll(a, b, c) (WSAPoll(a, b, c))

#elif __linux__
#include <sys/poll.h>
#endif

#include "libfd.hpp"

struct pollEvent {
    FileDescriptor fd;
    short event;
};

using pollEvents = std::vector<pollEvent>;

class Poller {
    std::mutex poll_mtx;
    std::vector<pollfd> fds;
    
public:
    void addDescriptor(const FileDescriptor& descriptor, short events) {
        poll_mtx.lock();

        fds.push_back({descriptor.fd(), events, 0});

        poll_mtx.unlock();
    }

    void removeDescriptor(const FileDescriptor& descriptor) {
        poll_mtx.lock();

        for (size_t i = 0; i < fds.size(); i++) if (fds[i].fd == descriptor.fd()) { fds.erase(fds.begin() + i); break; }

        poll_mtx.unlock();
    }

    void removeAllDescriptors() {
        fds.clear();
    }

    std::vector<pollfd>& getDescriptors() {
        return fds;
    }

    pollEvents poll(int timeout = -1) {
        pollEvents events;

        poll_mtx.lock();

        std::vector<pollfd> __fds = fds;

        poll_mtx.unlock();


#ifdef _WIN32
        int num_events = mpoll(__fds.data(), __fds.size(), timeout);
#elif __linux__
        int num_events = ::poll(__fds.data(), __fds.size(), timeout);
#endif

        if (num_events) {
            int n_events = 0;

            for (pollfd& i : __fds) {
                if (i.revents) {
                    events.push_back({i.fd, i.revents});
                    
                    i.revents = 0;
                    n_events++;
                }

                if (n_events >= num_events) break;
            }
        }

        return events;
    }
};