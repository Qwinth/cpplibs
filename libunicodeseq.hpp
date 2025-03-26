#pragma once
#include <string>
#include <regex>
#include <iomanip>

#include "libstrconvert.hpp"

std::map<std::string, std::string> escape_sequences = { {"\\", "\\\\"}, {"\'", "\\'"}, {"\"", "\\\""}, {"\a", "\\a"}, {"\b", "\\b"}, {"\f", "\\f"}, {"\n", "\\n"}, {"\r", "\\r"}, {"\t", "\\t"}, {"\v", "\\v"} };
std::map<std::string, std::string> escape_sequences_reverse = { {"\\'", "\'"}, {"\\\"", "\""}, {"\\\\", "\\"}, {"\\a", "\a"}, {"\\b", "\b"}, {"\\f", "\f"}, {"\\n", "\n"}, {"\\r", "\r"}, {"\\t", "\t"}, {"\\v", "\v"} };

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