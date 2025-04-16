#pragma once
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <string>

#include "libbase64.hpp"

class ByteArray {
    char* buffer = nullptr;
    size_t buf_size = 0;

    Base64 base64;

    void copy(const ByteArray& obj) {
        resize(obj.buf_size);

        std::memcpy(buffer, obj.buffer, obj.buf_size);
    }

    void swap(ByteArray& obj) {
        std::swap(buffer, obj.buffer);
        std::swap(buf_size, obj.buf_size);
    }
public:

    ByteArray() {}
    ByteArray(const char* new_data, size_t new_size) {
        set(new_data, new_size);
    }

    ByteArray(std::string data) {
        fromString(data);
    }

    ByteArray(const ByteArray& obj) { copy(obj); }
    ByteArray(ByteArray&& obj) { swap(obj); }
    ~ByteArray() {
        delete[] buffer;
    }

    ByteArray& operator=(const ByteArray& obj) {
        copy(obj);

        return *this;
    }

    ByteArray& operator=(ByteArray&& obj) {
        swap(obj);

        return *this;
    }

    void resize(size_t new_size) {
        char* new_buffer = new char[new_size];

        std::memcpy(new_buffer, buffer, std::min(buf_size, new_size));

        delete[] buffer;

        buf_size = new_size;
        buffer = new_buffer;
    }

    void append(const char* new_data, size_t size) {
        size_t old_size = buf_size;

        resize(old_size + size);

        std::memcpy(buffer + old_size, new_data, size);
    }

    void append(const ByteArray& obj) {
        append(obj.c_str(), obj.size());
    }

    void set(const char* new_data, size_t new_size) {
        resize(new_size);

        std::memcpy(buffer, new_data, new_size);
    }

    void set(const ByteArray& obj) {
        resize(obj.size());

        std::memcpy(buffer, obj.c_str(), obj.size());
    }

    void erase(size_t pos, size_t size) {
        for (size_t i = 0; i < size; i++) buffer[pos + i] = buffer[pos + size + i];
        
        resize(buf_size - size);
    }

    std::string toString() const {
        return std::string((char*)buffer, buf_size);
    }

    void fromString(std::string data) {
        set((const char*)data.c_str(), data.size());
    }

    ByteArray toBase64() {
        return base64.encodeString(toString());
    }

    ByteArray fromBase64() {
        return base64.decodeString(toString());
    }

    size_t size() const {
        return buf_size;
    }

    const char* c_str() const {
        return buffer;
    }

    char front() const {
        return buffer[0];
    }

    char back() const {
        return buffer[buf_size - 1];
    }

    char operator[](size_t index) const {
        if (index < 0 || index >= buf_size) return 0;

        return buffer[index];
    }
};

bool operator==(ByteArray& obj, ByteArray& obj2) {
    return obj.size() == obj2.size() && !memcmp(obj.c_str(), obj2.c_str(), obj.size());
}