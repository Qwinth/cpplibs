// version 1.8.3
#include <string>
#include <vector>
#include <map>
#include <cstdarg>
#include <random>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <iterator>

#ifndef _WIN32
#include <codecvt>
#endif

#pragma once

#include "librandom.hpp"
#include "windowsHeader.hpp"

std::string operator*(std::string str, size_t num) {
    std::string newstr;
    for (size_t i = 0; i < num; i++) newstr += str;
    return newstr;
}

std::string toUpper(std::string str) {
    std::string ret;
    for (char i : str) ret += std::toupper(i);

    return ret;
}

std::string toLower(std::string str) {
    std::string ret;
    for (char i : str) ret += std::tolower(i);

    return ret;
}

std::string strformat(const std::string fmt, ...) {
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}


std::vector<std::string> split(std::string src, std::string delim, size_t limit = 0) {
    std::vector<std::string> ret;
    size_t startpos = 0;
    size_t endpos = 0;
    size_t count = 0;

    while ((count++ < limit || limit == 0) && (endpos = src.find(delim, startpos)) != std::string::npos) {
        ret.push_back(src.substr(startpos, endpos - startpos));
        startpos = endpos + delim.size();
    }

    ret.push_back(src.substr(startpos));

    return ret;
}

std::vector<std::string> split(std::string src, char delim, size_t limit = 0) {
    std::string delimiter;
    delimiter += delim;

    return split(src, delimiter, limit);
}

std::string strjoin(std::vector<std::string> vec, std::string delim) {
    std::ostringstream ostr;
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(ostr, delim.c_str()));
    return ostr.str();
}

std::string randstring(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::string tmp_s;

    for (int i = 0; i < len; ++i) tmp_s += alphanum[random(0, sizeof(alphanum) - 1)];

    return tmp_s;
}

std::string randnums(const int len) {  
    std::string tmp_s;

    for (int i = 0; i < len; ++i) tmp_s += std::to_string(random(0, 9));

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

    if (precision) s << std::fixed << std::setprecision(precision);
    else s << std::fixed << std::setprecision(15);

    s << arg;

    return s.str();
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

std::pair<int, int> buffDiv(int divnum, int size, const void* source, void* front, void* back) {
    int front_size = std::min(divnum, size);
    int back_size = std::max(0, size - divnum);

    std::memcpy(front, source, std::min(divnum, size));
    std::memcpy(back, ((char*)source) + divnum, std::max(0, size - divnum));

    return {front_size, back_size};
}

std::pair<std::string, std::string> stringDiv(size_t divnum, std::string& source) {
    return {source.substr(0, divnum), source.substr(divnum)};
}

std::string strcrypto(std::string data_string, std::string pass) {
    size_t pass_point = 0;
    size_t str_point = 0;

    for (size_t i = 0; i < std::max(data_string.size(), pass.size()); i++) {
        if (pass_point >= pass.size()) pass_point = 0;
        if (str_point >= data_string.size()) str_point = 0;

        data_string[str_point++] ^= pass[pass_point++];
    }

    return data_string;
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

struct as_string {
    std::string data;

    as_string() {}
    as_string(std::string i) { data = i; }
    as_string(int i) { data = to_string(i); }
    as_string(long i) { data = to_string(i); }
    as_string(double i) { data = to_string(i); }
    as_string(bool i) {
        std::stringstream ss;
        ss << boolalpha << i;

        data = ss.str();
    }

    as_string(char i) {
        char b[1] = {i};
        data.append(b, 1);
    }

    operator const std::string() const {
        return data;
    }
};