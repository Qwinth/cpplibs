// version 1.0-c2
#include <string>
#include <cstring>
#include <cstdint>

class Base64 {
    std::string enctable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    std::string enc3bytes(std::string buffer) {
        std::string ret;

        if (buffer.size()) {
            uint8_t chr0 = buffer[0];
            uint8_t chr1 = buffer[1];
            uint8_t chr2 = buffer[2];

            ret.push_back(chr0 >> 2);

            if (buffer.size() > 1) {
                ret.push_back((chr1 >> 4) | ((chr0 & 0x03) << 4));

                if (buffer.size() > 2) {
                    ret.push_back((chr2 >> 6) | ((chr1 & 0xF) << 2));
                    ret.push_back(chr2 & 0x3F);
                }
                
                else {
                    ret.push_back((chr1 & 0xF) << 2);
                    ret.push_back(0x40);
                }
            }
            
            else {
                ret.push_back((chr0 & 0x03) << 4);
                ret.push_back(0x40);
                ret.push_back(0x40);
            }
        }

        return ret;
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

    std::string encodeString(std::string buffer) {
        std::string ret;

        for (size_t i = 0; i < buffer.size(); i += 3) {
            std::string tmp = enc3bytes(buffer.substr(i, 3));

            for (uint8_t i : tmp) ret += enctable[i];
        }

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

    int encode(const void* buffer, void* dest, size_t size) {
        std::string ret = encodeString(std::string((char*)buffer, size));
        memcpy(dest, ret.c_str(), ret.size());

        return ret.size();
    }

    int decode(const void* buffer, void* dest, size_t size) {
        std::string ret = decodeString(std::string((char*)buffer, size));
        memcpy(dest, ret.c_str(), ret.size());

        return ret.size();
    }
};