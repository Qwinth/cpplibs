// version 1.0-c2
#include <string>
#include <cstring>
#include <cstdint>
#pragma once

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

    std::string dec3bytes(std::string buffer) {
        std::string ret;

        uint8_t chr0 = buffer[0];
        uint8_t chr1 = buffer[1];
        uint8_t chr2 = buffer[2];
        uint8_t chr3 = buffer[3];

        ret.push_back((chr0 << 2) | (chr1 >> 4));
        if (chr2 != 0x40 && chr3 != 0x40) ret.push_back((chr1 << 4) | (chr2 >> 2));

        if (chr3 != 0x40) ret.push_back((chr2 << 6) | chr3);

        return ret;
    }

public:
    size_t baseCalculate(size_t size) {
        return ((4 * size / 3) + 3) & ~3;
    }

    std::string encodeString(std::string buffer) {
        std::string ret;

        size_t base64_size = baseCalculate(buffer.size());
        char* destbuff = new char[base64_size];

        encode(buffer.c_str(), destbuff, buffer.size());       

        ret = destbuff;

        delete[] destbuff;
        return ret;
    }

    std::string decodeString(std::string buffer) {
        std::string ret;

        for (size_t i = 0; i < buffer.size(); i += 4) {
            std::string tmp;

            for (uint8_t i : buffer.substr(i, 4)) tmp += enctable.find(i);
            
            ret += dec3bytes(tmp);
        }

        return ret;
    }

    size_t encode(const char* buffer, char* dest, ssize_t size) {
        size_t retsize = baseCalculate(size);
        char* ret = new char[retsize];
        char* retb = ret;
        
        char tmp1[3];
        char tmp2[4];

        for (size_t i = 0; i < size; i += 3) {
            memcpy(tmp1, &buffer[i], (size - (i + 3) >= 0) ? 3 : size - (i + 3));
            enc3bytes(tmp1, tmp2, size);

            for (int i = 0; i < 4; i++) (*(retb++)) = enctable[tmp2[i]];
        }

        memcpy(dest, ret, retsize);

        delete[] ret;
        return retsize;
    }

    int decode(const char* buffer, char* dest, size_t size) {
        std::string ret = decodeString(std::string((char*)buffer, size));
        memcpy(dest, ret.c_str(), ret.size());

        return ret.size();
    }
};