// version 1.3
#pragma once
#include <iostream>
#include <iomanip>
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
    long long integer = 0;
    long double lfloat = 0;
    bool boolean = false;
    std::vector<AnyType*> list;
    std::map<AnyType*, AnyType*> dict;

    AnyType() = default;
    AnyType(std::string val) { str = val; type = ANYSTRING; }
    AnyType(const char* val) { str = val; type = ANYSTRING; }
    AnyType(char val) { integer = val; type = ANYINTEGER; }
    AnyType(short val) { integer = val; type = ANYINTEGER; }
    AnyType(int val) { integer = val; type = ANYINTEGER; }
    AnyType(long val) { integer = val; type = ANYINTEGER; }
    AnyType(long long val) { integer = val; type = ANYINTEGER; }
    AnyType(float val) { lfloat = val; type = ANYFLOAT; }
    AnyType(double val) { lfloat = val; type = ANYFLOAT; }
    AnyType(long double val) { lfloat = val; type = ANYFLOAT; }
    AnyType(bool val) { boolean = val; type = ANYBOOLEAN; }
    AnyType(std::vector<AnyType*> val) { list = val; type = ANYLIST; }
    AnyType(std::map<AnyType*, AnyType*> val) { dict = val; type = ANYDICT; }
    AnyType(const AnyType& obj) {
        clear();
        copy(obj);

        // std::cout << "Copy constructor" << std::endl;
    }

    AnyType(AnyType&& obj) {
        clear();
        swap(obj);

        // std::cout << "Move constructor" << std::endl;
    }

    ~AnyType() {
        clear();
    }

    AnyType& operator=(const AnyType& obj) {
        clear();
        copy(obj);

        // std::cout << "Copy operator" << std::endl;

        return *this;
    }

    AnyType& operator=(AnyType&& obj) {
        clear();
        swap(obj);

        // std::cout << "Move operator" << std::endl;

        return *this;
    }

    void copy(const AnyType& obj) {
        type = obj.type;
        str = obj.str;
        integer = obj.integer;
        lfloat = obj.lfloat;
        boolean = obj.boolean;

        for (AnyType* i : obj.list) list.push_back(new AnyType(*i));
        for (auto i : obj.dict) dict[new AnyType(*i.first)] = (new AnyType(*i.second));
    }

    void swap(AnyType& obj) {
        std::swap(type, obj.type);
        std::swap(str, obj.str);
        std::swap(integer, obj.integer);
        std::swap(lfloat, obj.lfloat);
        std::swap(boolean, obj.boolean);
        std::swap(list, obj.list);
        std::swap(dict, obj.dict);
    }

    void clear() {
        type = ANYNONE;
        str.clear();
        integer = 0;
        lfloat = 0;
        boolean = false;

        if (list.size()) {
            for (auto i : list) {
                i->clear();
                delete i;
            }
        }

        if (dict.size()) {
            for (auto i : dict) {
                i.second->clear();
                delete i.first;
                delete i.second;
            }
        }

        list.clear();
        dict.clear();
    }

    bool operator==(const AnyType& arg) {
        if (arg.type == type) {
            if (type == ANYNONE) return true;
            else if (type == ANYSTRING) return str == arg.str;
            else if (type == ANYINTEGER) return integer == arg.integer;
            else if (type == ANYFLOAT) return lfloat == arg.lfloat;
            else if (type == ANYBOOLEAN) return boolean == arg.boolean;
            else if (type == ANYLIST) {
                for (size_t i = 0; i < list.size(); i++) {
                    if (*list[i] != *arg.list[i]) return false;
                }

                return true;
            }
        }

        return false;
    }

    bool operator!=(const AnyType& arg) { return !(*this == arg); }

};

void anyPrint(AnyType obj, bool endline = true) {
    if (obj.type == ANYSTRING) std::cout << std::quoted(obj.str);
    else if (obj.type == ANYINTEGER) std::cout << obj.integer;
    else if (obj.type == ANYFLOAT) std::cout << std::setprecision(15) << obj.lfloat;
    else if (obj.type == ANYBOOLEAN) std::cout << std::boolalpha << obj.boolean;
    else if (obj.type == ANYLIST) {
        std::cout << "[";

        anyPrint(obj.list[0], false);

        for (size_t i = 1; i < obj.list.size(); i++) {
            std::cout << ", ";

            anyPrint(obj.list[i], false);
        }

        std::cout << "]";
    }

    else if (obj.type == ANYDICT) {
        std::cout << "{";

        for (auto i : obj.dict) {
            anyPrint(*i.first, false);
            std::cout << ": ";

            anyPrint(*i.second, false);
            std::cout << ", ";
        }

        std::cout << "}";
    }

    if (endline) std::cout << std::endl;
}

AnyType str2any(std::string string, Types type) {
    if (type == ANYSTRING) return string;
    else if (type == ANYINTEGER) return stoll(string);
    else if (type == ANYFLOAT) return stold(string);
    else if (type == ANYBOOLEAN) {
        bool b = false;
        std::istringstream(string) >> std::boolalpha >> b;
        return b;
    } else return { ANYNONE };
}
