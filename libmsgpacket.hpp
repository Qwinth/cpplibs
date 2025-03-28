// version 1.0-c1
#pragma once
#include <cstdlib>
#include <cstring>
#include <string>

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
    void write(const void* data, size_t size) {
        size_t dataPointer = msgSize;
        resize(msgSize + size);

        memcpy(msgBuffer + dataPointer, data, size);
    }

    template<typename T>
    void write(T data) {
        write(&data, sizeof(T));
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