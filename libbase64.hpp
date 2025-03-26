// version 1.1-c5
#pragma once
#include <string>
#include <cstring>
#include <cstdint>
#include <algorithm>

class Base64 {
    std::string enctable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    void enc3bytes(const char* buffer, char* dest, size_t size) {
        if (size) {
            uint8_t chr0 = buffer[0];
            uint8_t chr1 = buffer[1];
            uint8_t chr2 = buffer[2];

            dest[0] = chr0 >> 2;

            if (size > 1) {
                dest[1] = (chr1 >> 4) | ((chr0 & 0x03) << 4);

                if (size > 2) {
                    dest[2] = (chr2 >> 6) | ((chr1 & 0xF) << 2);
                    dest[3] = chr2 & 0x3F;
                }

                else {
                    dest[2] = (chr1 & 0xF) << 2;
                    dest[3] = 0x40;
                }
            }

            else {
                dest[1] = (chr0 & 0x03) << 4;
                dest[2] = 0x40;
                dest[3] = 0x40;
            }
        }
    }

    int dec3bytes(const char* buffer, char* dest) {
        uint8_t chr0 = buffer[0];
        uint8_t chr1 = buffer[1];
        uint8_t chr2 = buffer[2];
        uint8_t chr3 = buffer[3];

        int bytes_count = 0;

        dest[0] = (chr0 << 2) | (chr1 >> 4); bytes_count++;

        if (chr2 != 0x40) { dest[1] = (chr1 << 4) | (chr2 >> 2); bytes_count++; }

        if (chr3 != 0x40) { dest[2] = (chr2 << 6) | chr3; bytes_count++; }

        return bytes_count;
    }

public:
    size_t calculateEncodedSize(size_t size) {
        return ((size + 2) / 3) * 4;
    }

    size_t calculateDecodedSize(std::string str) {
        return ((str.size() * 3) / 4) - std::count(str.begin(), str.end(), '=');
    }

    std::string encodeString(std::string buffer) {
        std::string ret;

        size_t base64_size = calculateEncodedSize(buffer.size());
        char* destbuff = new char[base64_size];

        encode(buffer.c_str(), destbuff, buffer.size());

        ret.append(destbuff, base64_size);

        delete[] destbuff;
        return ret;
    }

    std::string decodeString(std::string buffer) {
        size_t base64_size = calculateDecodedSize(buffer);
        char* destbuff = new char[base64_size];

        decode(buffer.c_str(), destbuff, buffer.size());

        std::string ret(destbuff, base64_size);

        delete[] destbuff;

        return ret;
    }

    size_t encode(const char* buffer, char* dest, ssize_t size) {
        size_t retsize = calculateEncodedSize(size);
        size_t retptr = 0;

        char tmp1[3] = {0};
        char tmp2[4] = {0};

        for (ssize_t i = 0; i < size; i += 3) {
            size_t encsize = (size - (i + 3) >= 0) ? 3 : size - i;

            memcpy(tmp1, &buffer[i], encsize);
            enc3bytes(tmp1, tmp2, encsize);

            for (int i = 0; i < 4; i++) dest[retptr++] = enctable[tmp2[i]];
        }

        return retsize;
    }

    size_t decode(const char* buffer, char* dest, ssize_t size) {
        size_t retsize = calculateDecodedSize(buffer);
        size_t retptr = 0;

        char tmp1[4] = {0};
        char tmp2[3] = {0};

        for (ssize_t i = 0; i < size; i += 4) {
            std::string tmp;

            memcpy(tmp1, &buffer[i], (size - (i + 4) >= 0) ? 4 : size - i);

            for (int i = 0; i < 4; i++) tmp1[i] = enctable.find(tmp1[i]);

            int bsize = dec3bytes(tmp1, tmp2);

            memcpy(&dest[retptr], tmp2, bsize);
            retptr += bsize;
        }

        return retsize;
    }
};
