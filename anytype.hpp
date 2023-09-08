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
    unsigned char* bytes = nullptr;
    void* pointer = nullptr;
    long long integer = 0;
    long double lfloat = 0;
    bool boolean = false;
    std::vector<AnyType*> list;
    std::map<AnyType*, AnyType*> dict;

    void clear() {
        type = ANYNONE;
        str.clear();
        bytes = nullptr;
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
                    if (list[i] != arg.list[i]) return false;
                }

                return true;
            }
        }

        return false;
    }

    bool operator!=(const AnyType& arg) { return !(*this == arg); }
    bool operator<(const AnyType& arg) { return (*this == arg); }

    AnyType* clonePtr() {
        AnyType* obj = new AnyType();
        obj->type = type;
        obj->str = str;
        obj->integer = integer;
        obj->lfloat = lfloat;
        obj->boolean = boolean;
        for (auto i : list) obj->list.push_back(i->clonePtr());
        for (auto i : dict) obj->dict[i.first->clonePtr()] = i.second->clonePtr();

        return obj;        
    }

    AnyType clone() {
        AnyType* ptr = clonePtr();
        AnyType obj = *ptr;
        delete ptr;
        return obj;
    }
};

void anyPrint(AnyType obj, bool endline = true) {
    if (obj.type == ANYSTRING) std::cout << '"' << obj.str << '"';
    else if (obj.type == ANYINTEGER) std::cout << obj.integer;
    else if (obj.type == ANYFLOAT) std::cout << std::setprecision(15) << obj.lfloat;
    else if (obj.type == ANYBOOLEAN) std::cout << std::boolalpha << obj.boolean;
    else if (obj.type == ANYLIST) {
        std::cout << "[";
        anyPrint(*obj.list[0], false);
        for (int i = 1; i < obj.list.size(); i++) {
            std::cout << ", ";
            anyPrint(*obj.list[i], false);
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