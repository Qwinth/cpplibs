#include <string>
#include <sstream>
#include <iomanip>

#pragma once

std::string urlDecode(std::string SRC) {
    std::string ret;
    char ch;
    int ii;
    for (size_t i = 0; i<SRC.length(); i++) {
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