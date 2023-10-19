#pragma once
#include <string>
#include <vector>
#include <stdarg.h>
#include <random>
#include <sstream>
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <regex>
#ifdef _WIN32
#include <Windows.h>
#else
#include <codecvt>
#endif


std::string operator*(std::string str, size_t num) {
    std::string newstr;
    for (size_t i = 0; i < num; i++) newstr += str;
    return newstr;
}

std::map<std::string, std::string> escape_sequences = { {"\\", "\\\\"}, {"\'", "\\'"}, {"\"", "\\\""}, {"\a", "\\a"}, {"\b", "\\b"}, {"\f", "\\f"}, {"\n", "\\n"}, {"\r", "\\r"}, {"\t", "\\t"}, {"\v", "\\v"} };
std::map<std::string, std::string> escape_sequences_reverse = { {"\\'", "\'"}, {"\\\"", "\""}, {"\\\\", "\\"}, {"\\a", "\a"}, {"\\b", "\b"}, {"\\f", "\f"}, {"\\n", "\n"}, {"\\r", "\r"}, {"\\t", "\t"}, {"\\v", "\v"} };

struct Bytes {
    size_t length = 0;
    char* data;
};

std::string toUpper(std::string str) {
    std::string ret;
    for (char i : str) {
        ret += std::toupper(i);
    }
    return ret;
}

std::string strformat(const char* fmt, ...){
    int size = 512;
    char* buffer = 0;
    buffer = new char[size];
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    if(size<=nsize){ //fail delete buffer and try again
        delete[] buffer;
        buffer = 0;
        buffer = new char[nsize+1]; //+1 for /0
        nsize = vsnprintf(buffer, size, fmt, vl);
    }
    std::string ret(buffer);
    va_end(vl);
    delete[] buffer;
    return ret;
}

std::vector<std::string> split(std::string s, std::string delimiter, int limit = 0) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;
    int num = 0;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos && (num < limit || limit == 0)) {

        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
        if (limit > 0) num++;
    }

    res.push_back(s.substr(pos_start));
    return res;
}

std::vector<std::string> split(std::string s, char delimiter, int limit = 0) {
    size_t pos_start = 0, pos_end, delim_len = 1;
    std::string token;
    std::vector<std::string> res;
    int num = 0;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos && (num < limit || limit == 0)) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
        if (limit > 0) num++;
    }

    res.push_back(s.substr(pos_start));
    return res;
}

std::string urlDecode(std::string &SRC) {
    std::string ret;
    char ch;
    int i, ii;
    for (i=0; i<SRC.length(); i++) {
        if (int(SRC[i])==37) {
#ifdef _WIN32
            sscanf_s(SRC.substr(i+1,2).c_str(), "%x", &ii);
#else
            sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
#endif
            ch=static_cast<char>(ii);
            ret+=ch;
            i=i+2;
        } else {
            ret+=SRC[i];
        }
    }
    return (ret);
}

std::string urlEncode(std::string value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}



std::string uuid4() {
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    std::stringstream ss;
    int i;
    ss << std::hex;
    for (i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    ss << dis2(gen);
    for (i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    for (i = 0; i < 12; i++) {
        ss << dis(gen);
    };
    return ss.str();
}

#ifndef _WIN32
std::wstring str2wstr(const std::string& utf8Str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(utf8Str);
}

std::string wstr2str(const std::wstring& utf16Str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(utf16Str);
}
#else
std::wstring str2wstr(const std::string& string)
{
    if (string.empty())
    {
        return L"";
    }

    const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), nullptr, 0);
    if (size_needed <= 0)
    {
        throw std::runtime_error("MultiByteToWideChar() failed: " + std::to_string(size_needed));
    }

    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), &result.at(0), size_needed);
    return result;
}

std::string wstr2str(const std::wstring& wide_string)
{
    if (wide_string.empty())
    {
        return "";
    }

    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
    }

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
    return result;
}
#endif

#if __cplusplus >= 202002L || (_WIN32 && __cplusplus >= 199711L)
std::string u8tostr(std::u8string str) {
    return (char*)str.c_str();
}

std::u8string strtou8(std::string str) {
    return (char8_t*)str.c_str();
}
#endif

static void cp2utf1(char* out, const char* in) {
    static const int table[128] = {
        0x82D0,0x83D0,0x9A80E2,0x93D1,0x9E80E2,0xA680E2,0xA080E2,0xA180E2,
        0xAC82E2,0xB080E2,0x89D0,0xB980E2,0x8AD0,0x8CD0,0x8BD0,0x8FD0,
        0x92D1,0x9880E2,0x9980E2,0x9C80E2,0x9D80E2,0xA280E2,0x9380E2,0x9480E2,
        0,0xA284E2,0x99D1,0xBA80E2,0x9AD1,0x9CD1,0x9BD1,0x9FD1,
        0xA0C2,0x8ED0,0x9ED1,0x88D0,0xA4C2,0x90D2,0xA6C2,0xA7C2,
        0x81D0,0xA9C2,0x84D0,0xABC2,0xACC2,0xADC2,0xAEC2,0x87D0,
        0xB0C2,0xB1C2,0x86D0,0x96D1,0x91D2,0xB5C2,0xB6C2,0xB7C2,
        0x91D1,0x9684E2,0x94D1,0xBBC2,0x98D1,0x85D0,0x95D1,0x97D1,
        0x90D0,0x91D0,0x92D0,0x93D0,0x94D0,0x95D0,0x96D0,0x97D0,
        0x98D0,0x99D0,0x9AD0,0x9BD0,0x9CD0,0x9DD0,0x9ED0,0x9FD0,
        0xA0D0,0xA1D0,0xA2D0,0xA3D0,0xA4D0,0xA5D0,0xA6D0,0xA7D0,
        0xA8D0,0xA9D0,0xAAD0,0xABD0,0xACD0,0xADD0,0xAED0,0xAFD0,
        0xB0D0,0xB1D0,0xB2D0,0xB3D0,0xB4D0,0xB5D0,0xB6D0,0xB7D0,
        0xB8D0,0xB9D0,0xBAD0,0xBBD0,0xBCD0,0xBDD0,0xBED0,0xBFD0,
        0x80D1,0x81D1,0x82D1,0x83D1,0x84D1,0x85D1,0x86D1,0x87D1,
        0x88D1,0x89D1,0x8AD1,0x8BD1,0x8CD1,0x8DD1,0x8ED1,0x8FD1
    };

    while (*in)
        if (*in & 0x80) {
            int v = table[(int)(0x7f & *in++)];
            if (!v)
                continue;
            *out++ = (char)v;
            *out++ = (char)(v >> 8);
            if (v >>= 16)
                *out++ = (char)v;
        }
        else
            *out++ = *in++;
    *out = 0;
}
std::string cp1251toutf8(std::string s) {
    int c, i;
    size_t len = s.size();
    std::string ns;
    for (i = 0; i < len; i++) {
        c = s[i];
        char buf[4], in[2] = { 0, 0 };
        *in = c;
        cp2utf1(buf, in);
        ns += std::string(buf);
    }
    return ns;
}

std::string strjoin(std::vector<std::string> vec, std::string delim) {
    std::ostringstream ostr;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(ostr, delim.c_str()));
    return ostr.str();
}

template<class T, class U>
bool vectorfind(T& vec, U elem) {
    return std::find(vec.begin(), vec.end(), elem) != vec.end();
}

std::string randstring(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

void replaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while (std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

std::string to_stringWp(long double arg, int precision = 0) {
    std::stringstream s;
    if (precision) s << std::setprecision(precision) << std::fixed << arg;
    else s << std::setprecision(15) << std::fixed << arg;
    return s.str();
}

std::string decodeUnicodeSequence(const std::string& str) {
    std::wstring input = str2wstr(str);
    std::wregex unicodeRegex(L"\\\\u([0-9a-fA-F]{4})");
    
    std::wstring decodedString;
    std::wsregex_iterator it(input.begin(), input.end(), unicodeRegex);
    std::wsregex_iterator end;
    size_t lastPos = 0;

    while (it != end) {
        decodedString.append(input, lastPos, it->position() - lastPos);
        wchar_t unicodeChar = static_cast<wchar_t>(std::stoi(it->str(1), nullptr, 16));
        decodedString.push_back(unicodeChar);
        lastPos = it->position() + it->str().size();
        ++it;
    }

    decodedString.append(input, lastPos, input.size() - lastPos);
    
    return wstr2str(decodedString);
}

bool isNonASCII(wchar_t ch) {
    return static_cast<unsigned int>(ch) > 127 || static_cast<unsigned int>(ch) < 32;
}

std::string encodeUnicodeSequence(const std::string& str) {
    std::wstring input = str2wstr(str);
    std::wstringstream result;
    for (wchar_t c : input) if (isNonASCII(c)) result << L"\\u" << std::hex << std::setw(4) << std::setfill(L'0') << static_cast<int>(c); else result << c;
    return wstr2str(result.str());
}

std::string unpackNumber(std::string& str) {
    std::string ret;
    std::string delim = "E";
    if (str.find('e') != std::string::npos) delim = 'e';

    std::vector<std::string> tmp = split(str, delim);
    if (tmp.size() > 1) {
        if (tmp[1].find('-') != std::string::npos) {
            long double mynum = std::stold(tmp[0]);
            int num = std::stoi(tmp[1].substr(1));

            for (int i = 0; i < num; i++) mynum /= 10;
            ret = to_stringWp(mynum);
        }

        else {
            long double mynum = std::stold(tmp[0]);
            int num = std::stoi((tmp[1].find('+') != std::string::npos) ? tmp[1].substr(1) : tmp[1]);

            for (int i = 0; i < num; i++) mynum *= 10;
            ret = to_stringWp(mynum);
        }
    }

    else ret = tmp[0];

    return ret;
}

void substr(size_t pos, size_t len, const char* src, char* dest, size_t destpos = 0) {
    memcpy(&dest[destpos], &src[pos], len);
}

std::string tolowerString(std::string& str) {
    std::string ret;

    for (char i : str) {
        ret += tolower(i);
    }

    return ret;
}

std::string toupperString(std::string& str) {
    std::string ret;

    for (char i : str) {
        ret += toupper(i);
    }

    return ret;
}


std::string sstrerror(int e) {
#ifdef _WIN32
    char buff[100];
    strerror_s(buff, 100, e);
    return std::string(buff, 100);
#else
    return strerror(e);
#endif
}
