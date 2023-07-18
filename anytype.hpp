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

void anyPrint(AnyType obj) {
    if (obj.type == ANYSTRING) std::cout << obj.str << std::endl;
    else if (obj.type == ANYINTEGER) std::cout << obj.integer << std::endl;
    else if (obj.type == ANYFLOAT) std::cout << std::setprecision(15) << obj.lfloat << std::endl;
    else if (obj.type == ANYBOOLEAN) std::cout << std::boolalpha << obj.boolean << std::endl;
    else if (obj.type == ANYLIST) {
        std::cout << "[" << obj.list[0]->str;
        for (int i = 1; i < obj.list.size(); i++) {
            std::cout << ", " << obj.list[i]->str;
        }
        std::cout << "]" << std::endl;
    }

    else if (obj.type == ANYDICT) {
        std::stringstream s;
        s << "{" << obj.dict[0]->str;
        for (auto i : obj.dict) {
            s << i.first << ": " << i.second << ", ";
        }

        std::string out = s.str();
        std::cout << out.substr(0, out.size() - 2) << "}" << std::endl;
    }
}