// version 1.0
#include <cstdlib>
#include <cstring>
#include <string>
#pragma once

class MsgPacket {
    char* msgBuffer = nullptr;
    size_t msgSize = 0;

    void resize(size_t size) {
        char* buf = new char[size];

        memcpy(buf, msgBuffer, std::min(size, msgSize));

        delete[] msgBuffer;
        msgBuffer = buf;
        msgSize = size;
    }
public:
    void write(const char* data, size_t size) {
        size_t dataPointer = msgSize;
        resize(msgSize + size);

        memcpy(msgBuffer + dataPointer, data, size);
    }

    void write(std::string data) {
        write(data.c_str(), data.size());
    }

    size_t size() const {
        return msgSize;
    }

    const char* c_str() const {
        return msgBuffer;
    }

    std::string str() const {
        return std::string(msgBuffer, msgSize);
    }

    void clear() {
        delete[] msgBuffer;

        msgBuffer = nullptr;
        msgSize = 0;
    }
};