// version 1.3
#pragma once
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <functional>
#include "libbytesarray.hpp"

template<typename T>
class RingBuffer {
    int64_t readIndex = 0;
    int64_t writeIndex = 0;

    int64_t bufferSize = 0;
    int64_t bufferSizeUsed = 0;

    std::function<void(ByteArray)> overflowCallback = nullptr;
    std::function<ByteArray(int64_t)> underflowCallback = nullptr;

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
    
    RingBuffer(int64_t size) {
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

    void resize(int64_t size) {
        T* newbuffer = new T[size];

        memcpy(newbuffer, buffer, std::min(bufferSize, size));

        delete[] buffer;
        buffer = newbuffer;
        bufferSize = size;
    }

    int64_t read(T* dest, int64_t size) {
        std::lock_guard lock(*mtx);

        if (empty()) return 0;
        // if (underflowCallback == nullptr) return 0;
        // else underflowCallback(size);

        int64_t readsize = std::min(size, bufferSizeUsed);
        int64_t firstPart = std::min(readsize, bufferSize - readIndex);
        int64_t secondPart = readsize - firstPart;

        memcpy(dest, &buffer[readIndex], firstPart);
        memcpy(&dest[firstPart], buffer, secondPart);

        bufferSizeUsed -= readsize;
        readIndex += readsize;

        if (readIndex >= bufferSize) readIndex -= bufferSize;

        return readsize;
    }

    int64_t write(const T* buff, int64_t size) {
        std::lock_guard lock(*mtx);

        if (full() || !buff) return 0;

        int64_t writesize = std::min(size, available());
        int64_t buffIndex = 0;

        int64_t firstPart = std::min(writesize, bufferSize - writeIndex);
        int64_t secondPart = writesize - firstPart;

        memcpy(&buffer[writeIndex], &buff[buffIndex], firstPart);
        memcpy(buffer, &buff[buffIndex + firstPart], secondPart);

        bufferSizeUsed += writesize;
        writeIndex += writesize;

        if (writeIndex >= bufferSize) writeIndex -= bufferSize;

        return writesize;
    }
    
    // int64_t peek(T* dest, int64_t size) {
    //     if (!bufferSizeUsed) return 0;

    //     mtx->lock();

    //     int64_t tmpReadIndex = readIndex;

    //     int64_t destIndex = 0;
    //     int64_t readsize = std::min(size, bufferSizeUsed);
    //     int64_t retsize = readsize;

    //     if (readIndex + readsize > bufferSize) {
    //         memcpy(&dest[destIndex], &buffer[readIndex], bufferSize - readIndex);

    //         destIndex += bufferSize - readIndex;
    //         readsize -= bufferSize - readIndex;
    //         readIndex = 0;
    //     }

    //     memcpy(&dest[destIndex], &buffer[readIndex], readsize);
        
    //     readIndex = tmpReadIndex;

    //     mtx->unlock();

    //     return retsize;
    // }

    T get() {
        std::lock_guard<std::mutex> lock(*mtx);

        if (empty()) return T();

        T ret = buffer[readIndex++];
        bufferSizeUsed--;

        if (readIndex == bufferSize) readIndex = 0;

        return ret;
    }

    void put(T data) {
        std::lock_guard lock(*mtx);

        if (full()) return;

        buffer[writeIndex++] = data;
        if (bufferSizeUsed < bufferSize) bufferSizeUsed++;

        if (writeIndex == bufferSize) writeIndex = 0;
    }

    int64_t usage() {
        return bufferSizeUsed;
    }

    int64_t available() {
        return bufferSize - bufferSizeUsed;
    } 

    int64_t size() {
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