#pragma once
#include <thread>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/prctl.h>

class process {
    pid_t mypid;
    bool tod = true; // terminate on destruct
    std::string name;
    int p[2];

public:
    process() { pipe(p); };
    process(std::string name) : name(name) { pipe(p); }

    ~process() {
        pipeclose();
        if (tod) terminate();
    }

    template<typename Ptr, typename... Args>
    process* start(Ptr func, Args... args) {

        // static_assert(std::__is_invocable<typename std::decay<Ptr>::type, typename std::decay<Args>::type...>::value, "process() arguments must be invocable after conversion to rvalues");

        if ((mypid = fork()) == 0) {
            if (name.size()) prctl(PR_SET_NAME, name.c_str());

            func(*this, args...);
            std::exit(errno);
        }

        return this;
    }

    int join() {
        int status;
        int preverrno = errno;
        if (waitpid(mypid, &status, 0) == -1) if (errno == ECHILD) errno = preverrno;
        return status;
    }

    process* detach() {
        tod = false;
        return this;
    }

    pid_t id() {
        return mypid;
    }

    void terminate() {
        kill(mypid, SIGTERM);
        join(); // Clear zombie process
    }

    ssize_t pipewrite(std::string str) {
        return pipewrite(str.c_str(), str.size());
    }

    ssize_t pipewrite(const void* buff, size_t size) {
        return write(p[1], buff, size);
    }

    std::string piperead(size_t size) {
        char buff[size];
        ssize_t len = piperead(buff, size);

        return std::string(buff, len);
    }

    ssize_t piperead(void* dest, size_t size) {
        return read(p[0], dest, size);
    }

    void pipeclose() {
        close(p[0]);
        close(p[1]);
    }
};
