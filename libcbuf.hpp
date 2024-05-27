// version 1.1
#include <algorithm>
#include <cstring>
#pragma once

class CircularBuffer {
    size_t readIndex = 0;
    size_t writeIndex = 0;

    size_t bufferSize = 0;
    size_t bufferSizeUsed = 0;
    
    char* buffer = nullptr;
public:
    CircularBuffer() {}
    CircularBuffer(size_t size) {
        resize(size);
    }

    ~CircularBuffer() {
        delete[] buffer;
    }

    void resize(size_t size) {
        char* newbuffer = new char[size];

        memcpy(newbuffer, buffer, std::min(bufferSize, size));

        delete[] buffer;
        buffer = newbuffer;
        bufferSize = size;
    }

    size_t read(char* dest, size_t size) {
        if (!bufferSizeUsed) return 0;

        size_t destIndex = 0;
        size_t readsize = std::min(size, bufferSizeUsed);
        size_t retsize = readsize;

        if (readIndex + readsize > bufferSize) {
            memcpy(&dest[destIndex], &buffer[readIndex], bufferSize - readIndex);

            destIndex += bufferSize - readIndex;
            readsize -= bufferSize - readIndex;
            bufferSizeUsed -= bufferSize - readIndex;
            readIndex = 0;
        }

        memcpy(&dest[destIndex], &buffer[readIndex], readsize);

        bufferSizeUsed -= readsize;
        readIndex += readsize;

        return retsize;
    }

    size_t write(const char* buff, size_t size) {
        if (bufferSizeUsed == bufferSize) return 0;

        size_t bufferSizeFree = bufferSize - bufferSizeUsed;
        size_t writesize = std::min(size, bufferSizeFree);
        size_t retsize = writesize;
        size_t buffIndex = 0;

        if (writeIndex + writesize > bufferSize) {
            memcpy(&buffer[writeIndex], &buff[buffIndex], bufferSize - writeIndex);

            buffIndex += bufferSize - writeIndex;
            writesize -= bufferSize - writeIndex;
            bufferSizeUsed += bufferSize - writeIndex;
            writeIndex = 0;
        }

        memcpy(&buffer[writeIndex], &buff[buffIndex], writesize);

        bufferSizeUsed += writesize;
        writeIndex += writesize;

        if (writeIndex == bufferSize) writeIndex = 0;

        return retsize;
    }

    char get() {
        if (!bufferSizeUsed) return 0;

        char ret = buffer[readIndex++];
        bufferSizeUsed--;

        if (readIndex == bufferSize) readIndex = 0;

        return ret;
    }

    void put() {
        if (bufferSizeUsed == bufferSize) return;

        char ret = buffer[writeIndex++];
        bufferSizeUsed++;

        if (writeIndex == bufferSize) writeIndex = 0;
    }

    size_t usage() {
        return bufferSizeUsed;
    }

    size_t size() {
        return bufferSize;
    }

    void clear() {
        memset(buffer, 0, bufferSize);

        readIndex = 0;
        writeIndex = 0;
        bufferSizeUsed = 0;
    }

    bool full() {
        return bufferSizeUsed == bufferSize;
    }

    bool empty() {
        return !bufferSizeUsed;
    }

    char* data() {
        return buffer;
    }
};