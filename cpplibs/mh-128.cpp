#include <vector>
#include <string>

int rightrotate(int n, unsigned int d) {
    return (n >> d)|(n << (sizeof(int) * 8 - d));
}

string toUpper(string str) {
    string ret;
    for (char i : str) {
        ret += toupper(i);
    }
    return ret;
}

std::string mhash(std::string str) {
    std::vector<uint8_t> t;
    std::vector<uint32_t> w;

    uint32_t a = 0x8DA47A5;
    uint32_t b = 0xB297D42;
    uint32_t c = 0x671A315;
    uint32_t d = 0x158A2577;
    uint32_t e = 0x2510FFC5;
    uint32_t f = 0x121482CB;
    uint32_t g = 0x1AA96BEA;
    uint32_t h = 0x39151886;

    uint32_t res0 = 0;
    uint32_t res1 = 0;
    uint32_t res2 = 0;
    uint32_t res3 = 0;

    for (char i : str) {
        t.push_back(i);
    }

    for (int i = t.size(); i > 64; i--) {
        for (int j = 1; j < t.size(); j++) {
            t[j] = (((t[j] << 4) ^ t[t.size() - i]) ^ t[j]);
        }
        t.erase(t.begin() + (t.size() - i));
    }
    
    for (int i = t.size(); i < 64; i++) {
        t.push_back(0);
    }

    t.push_back(str.length() * 8);


    for (int i = 0; i < t.size() - 1; i++) {
        t[i] = (t[i] ^ t.back()) - 4000;
    }

    t.pop_back();

    for (int i = 0; i < 16; i++) {
        uint32_t l = 0;
        for (int j = 0; j < 4; j++) {
            l <<= 8;
            l += t[0];
            t.erase(t.begin());
        }
        w.push_back(l);
    }

    for (int i = w.size(); i < 64; i++) {
        w.push_back(0);
    }

    for (int i = 16; i < w.size(); i++) {
        int s0 = rightrotate(w[i - 15], 7) ^ rightrotate(w[i - 15], 13) ^ (w[i - 15] >> 3);
        int s1 = rightrotate(w[i - 2], 3) ^ rightrotate(w[i - 2], 16) ^ (w[i - 2] >> 8);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    for (int i = 0; i < 16; i++) {
        res0 += w[i];
        res1 += w[i + 16];
        res2 += w[i + 16 * 2];
        res3 += w[i + 16 * 3];
    }

    res0 *= (a ^ c) * (h ^ g);
    res1 *= (b ^ d) * (h ^ g);
    res2 *= (c ^ e) * (h ^ g);
    res3 *= (d ^ f) * (h ^ g);
}