#pragma once
#include <string>
#include <vector>
#include <map>

enum Types {
    ANYNONE,
    ANYSTRING,
    ANYINTEGER,
    ANYFLOAT,
    ANYBOOLEAN,
    ANYLIST,
    ANYDICT
};

struct AnyType {
    Types type = ANYNONE;
    std::string str;
    unsigned char* bytes = nullptr;
    void* pointer = nullptr;
    long long integer = 0;
    long double lfloat = 0;
    bool boolean = false;
    std::vector<AnyType*> list;
    std::map<AnyType*, AnyType*> dict;
};