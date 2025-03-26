#pragma once
#include <string>
#include <unistd.h>

class FileDescriptor {
protected:
    int desc = -1;

public:
    FileDescriptor() = default;
    FileDescriptor(int desc) : desc(desc) {};

    virtual ~FileDescriptor() = default;
    virtual int fd() const { return desc; }

    virtual int read(void* dest, int size) {
        return ::read(desc, dest, size);
    }

    virtual int write(const void* buff, int size) {
        return ::write(desc, buff, size);
    }

    virtual uint8_t readByte() {
        char byte[1] = {0};

        ::read(desc, byte, 1);

        return byte[0];
    }

    virtual int writeByte(uint8_t b) {
        char byte[1] = {(char)b};
        return ::write(desc, byte, 1);
    }

    virtual std::string readString(int size) {
        std::string ret;
        ret.reserve(size);

        ret.resize(::read(desc, (char*)ret.data(), size));

        return ret;
    }

    virtual int writeString(const std::string buff) {
        return ::write(desc, buff.c_str(), buff.size());
    }

    operator int() const {
        return desc;
    }
};