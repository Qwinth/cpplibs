// version 1.3
#pragma once
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>

template<typename T>
class RingBuffer {
    size_t readIndex = 0;
    size_t writeIndex = 0;

    size_t bufferSize = 0;
    size_t bufferSizeUsed = 0;

    std::unique_ptr<std::mutex> mtx;
    
    T* buffer = nullptr;

    void copy(const RingBuffer& obj) {
        free();

        buffer = new T[obj.bufferSize];
        memcpy(buffer, obj.buffer, obj.bufferSize);

        bufferSize = obj.bufferSize;
        bufferSizeUsed = obj.bufferSizeUsed;
        readIndex = obj.readIndex;
        writeIndex = obj.writeIndex;
    }

    void swap(RingBuffer& obj) {
        free();

        std::swap(buffer, obj.buffer);
        std::swap(bufferSize, obj.bufferSize);
        std::swap(bufferSizeUsed, obj.bufferSizeUsed);
        std::swap(readIndex, obj.readIndex);
        std::swap(writeIndex, obj.writeIndex);
    }
public:
    RingBuffer() {
        mtx = std::make_unique<std::mutex>();
    }
    
    RingBuffer(size_t size) {
        resize(size);

        mtx = std::make_unique<std::mutex>();
    }

    RingBuffer(const RingBuffer& obj) {
        copy(obj);

        mtx = std::make_unique<std::mutex>();
    }

    RingBuffer(RingBuffer&& obj) {
        swap(obj);

        mtx = std::make_unique<std::mutex>();
    }

    ~RingBuffer() {
        delete[] buffer;
    }

    void resize(size_t size) {
        T* newbuffer = new T[size];

        memcpy(newbuffer, buffer, std::min(bufferSize, size));

        delete[] buffer;
        buffer = newbuffer;
        bufferSize = size;
    }

    size_t read(T* dest, size_t size) {
        if (!bufferSizeUsed) return 0;

        mtx->lock();

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

        mtx->unlock();

        return retsize;
    }

    size_t write(const T* buff, size_t size) {
        if (bufferSizeUsed == bufferSize) return 0;

        mtx->lock();

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

        mtx->unlock();

        return retsize;
    }
    
    size_t peek(T* dest, size_t size) {
        if (!bufferSizeUsed) return 0;

        mtx->lock();

        size_t tmpReadIndex = readIndex;

        size_t destIndex = 0;
        size_t readsize = std::min(size, bufferSizeUsed);
        size_t retsize = readsize;

        if (readIndex + readsize > bufferSize) {
            memcpy(&dest[destIndex], &buffer[readIndex], bufferSize - readIndex);

            destIndex += bufferSize - readIndex;
            readsize -= bufferSize - readIndex;
            readIndex = 0;
        }

        memcpy(&dest[destIndex], &buffer[readIndex], readsize);
        
        readIndex = tmpReadIndex;

        mtx->unlock();

        return retsize;
    }

    T get() {
        if (!bufferSizeUsed) return 0;

        T ret = buffer[readIndex++];
        bufferSizeUsed--;

        if (readIndex == bufferSize) readIndex = 0;

        return ret;
    }

    void put(T data) {
        if (bufferSizeUsed == bufferSize) return;

        buffer[writeIndex++] = data;
        bufferSizeUsed++;

        if (writeIndex == bufferSize) writeIndex = 0;
    }

    size_t usage() {
        return bufferSizeUsed;
    }

    size_t unused_space() {
        return bufferSize - bufferSizeUsed;
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

    void free() {
        delete[] buffer;
        buffer = nullptr;

        bufferSize = 0;
        bufferSizeUsed = 0;

        readIndex = 0;
        writeIndex = 0;
    }
};