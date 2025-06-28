#pragma once
#include <string>

#ifdef __linux__
#include <unistd.h>
typedef uint16_t fd_t;

#define _read(fd, dest, size) read(fd, dest, size)
#define _write(fd, src, size) write(fd, src, size)

#elif _WIN32
#include <io.h>
typedef SOCKET fd_t;
#endif

class FileDescriptor {
protected:
    fd_t desc = -1;
    
public:
    FileDescriptor() = default;
    FileDescriptor(fd_t desc) : desc(desc) {};

    virtual ~FileDescriptor() = default;
    virtual fd_t fd() const { return desc; }

    virtual int read(void* dest, int size) {
        return ::_read(desc, dest, size);
    }

    virtual int write(const void* buff, int size) {
        return ::_write(desc, buff, size);
    }

    virtual uint8_t readByte() {
        char byte[1] = {0};

        ::_read(desc, byte, 1);

        return byte[0];
    }

    virtual int writeByte(uint8_t b) {
        return ::_write(desc, &b, 1);
    }

    virtual std::string readString(int size) {
        std::string ret;
        ret.reserve(size);

        ret.resize(::_read(desc, (char*)ret.data(), size));

        return ret;
    }

    virtual int writeString(const std::string buff) {
        return ::_write(desc, buff.c_str(), buff.size());
    }

    virtual operator int() const {
        return desc;
    }
};