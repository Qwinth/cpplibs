#pragma once
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include "anytype.hpp"
#include "strlib.hpp"

enum JsonTypes {
    JSONNULL,
    JSONSTRING,
    JSONINTEGER,
    JSONFLOAT,
    JSONBOOLEAN,
    JSONLIST,
    JSONOBJECT
};

class JsonNode {
public:
    JsonTypes type = JSONNULL;
    std::string str;
    long long integer = 0;
    long double lfloat = 0;
    bool boolean = false;
    std::vector<JsonNode*> list;
    std::map<std::string, JsonNode*> objects;

    JsonNode operator[](std::string str) { return *objects[str]; }

    bool is_null() { return type == JSONNULL; }

private:
    JsonNode AnyType_to_JsonNode(AnyType value) {
        JsonNode node;
        node.type = (JsonTypes)value.type;

        switch (value.type) {
        case ANYSTRING: {
            node.str = value.str;
            break;
        }

        case ANYINTEGER: {
            node.integer = value.integer;
            break;
        }

        case ANYFLOAT: {
            node.lfloat = value.lfloat;
            break;
        }

        case ANYBOOLEAN: {
            node.boolean = value.boolean;
            break;
        }

        case ANYLIST: {
            for (AnyType* i : value.list) node.list.push_back(new JsonNode(AnyType_to_JsonNode(*i)));
            break;
        }
        }

        return node;
    }

public:
    void addPair(std::string key, std::string value) { objects[key] = new JsonNode({.type = JSONSTRING, .str = value}); }
    void addPair(std::string key, const char* value) { objects[key] = new JsonNode({.type = JSONSTRING, .str = value}); }
    void addPair(std::string key, short value) { objects[key] = new JsonNode({.type = JSONINTEGER, .integer = value}); }
    void addPair(std::string key, int value) { objects[key] = new JsonNode({.type = JSONINTEGER, .integer = value}); }
    void addPair(std::string key, long value) { objects[key] = new JsonNode({.type = JSONINTEGER, .integer = value}); }
    void addPair(std::string key, long long value) { objects[key] = new JsonNode({.type = JSONINTEGER, .integer = value}); }
    void addPair(std::string key, float value) { objects[key] = new JsonNode({.type = JSONFLOAT, .lfloat = value}); }
    void addPair(std::string key, double value) { objects[key] = new JsonNode({.type = JSONFLOAT, .lfloat = value}); }
    void addPair(std::string key, long double value) { objects[key] = new JsonNode({.type = JSONFLOAT, .lfloat = value}); }
    void addPair(std::string key, bool value) { objects[key] = new JsonNode({.type = JSONBOOLEAN, .boolean = value}); }
    void addPair(std::string key, std::vector<JsonNode*> value) { objects[key] = new JsonNode({.type = JSONLIST, .list = value}); }
    void addPair(std::string key, std::map<std::string, JsonNode*> value) { objects[key] = new JsonNode({.type = JSONOBJECT, .objects = value}); }
    void addPair(std::string key, AnyType value) { objects[key] = new JsonNode(AnyType_to_JsonNode(value)); }
};

class Json {

    bool check_prev(std::string& str, size_t pos, char arg) {
        if (pos - 1 < 0) return false;
        return str[pos - 1] == arg;
    }

    bool is_digit(std::string& str) {
        for (char i : str) {
            if (!std::isdigit(i)) return false;
        }

        return true;
    }

    size_t getEndPos(std::string& str, size_t pos) {
        size_t tmppos = str.find(',', pos);
        return (tmppos != std::string::npos) ? tmppos : str.find('}', pos);
    }

    bool is_float(std::string& str, size_t pos) {
        size_t endpos = getEndPos(str, pos);

        for (size_t i = pos; i < endpos; i++) {
            char j = str[i];

            if (isdigit(str[i - 1]) && j == '.' && isdigit(str[i + 1])) return true;
        }

        return false;
    }

    std::pair<AnyType, size_t> anyParse(std::string& str, size_t pos) {
        if (str[pos] == '"') return parseString(str, pos);
        else if (isdigit(str[pos]) && !is_float(str, pos)) return parseInt(str, pos);
        else if (isdigit(str[pos]) && is_float(str, pos)) return parseFloat(str, pos);
        else if (str[pos] == 't' || str[pos] == 'f') return parseBool(str, pos);
        return { {}, pos };
    }

    std::pair<AnyType, size_t> parseString(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos + 1;
        
        for (; i < str.size(); i++) {
            char j = str[i];

            if (j == '"') break;
            else if (j == '\\') {
                tmp += str[i + 1];
                i++;
                continue;
            } else {
                tmp += j;
            }
        }

        return { { .type = ANYSTRING, .str = tmp }, i };
    }

    std::pair<AnyType, size_t> parseInt(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j)) tmp += j; 
        }

        return { { .type = ANYINTEGER, .integer = std::stoll(tmp) }, i - 1 };
    }

    std::pair<AnyType, size_t> parseFloat(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j) || j == '.') tmp += j; 
        }

        return { { .type = ANYFLOAT, .lfloat = std::stold(tmp) }, i - 1 };
    }

    std::pair<AnyType, size_t> parseBool(std::string& str, size_t pos) {
        if (str.substr(pos, 4) == "true") return { { .type = ANYBOOLEAN, .boolean = true }, pos + 3 };
        return { { .type = ANYBOOLEAN, .boolean = false }, pos + 4 };
    }
public:
    JsonNode parse(std::string str) {
        JsonNode node;
        node.type = JSONOBJECT;
        bool obj_opened = str[0] == '{';
        AnyType tmpobj;

        std::string key;

        for (size_t i = 0; i < str.size() && obj_opened; i++) {
            char token = str[i];

            if (token == '}') {
                obj_opened = false;
                node.addPair(key, tmpobj);
                key.clear();
                tmpobj.clear();
            }

            else if (token == ':') key = tmpobj.str;

            else if (token == ',') {
                node.addPair(key, tmpobj);
                key.clear();
                tmpobj.clear();
            }

            else if (token > 32 && token < 127) {
                auto tmp = anyParse(str, i);
                tmpobj = tmp.first;
                i = tmp.second;
            }
        }

        return node;
    }

    std::string dump(JsonNode node) {
        std::string str = "{";

        for (auto i : node.objects) {

            switch (i.second->type) {
            case JSONNULL: {
                str += '"' + i.first + "\": " + "null";
                break;
            }

            case JSONSTRING: {
                
                str += '"' + i.first + "\": " + '"' + i.second->str + '"';
                break;
            }

            case JSONINTEGER: {
                str += '"' + i.first + "\": " + std::to_string(i.second->integer);
                break;
            }

            case JSONFLOAT: {
                str += '"' + i.first + "\": " + to_stringWp(i.second->lfloat, 15);
                break;
            }

            case JSONBOOLEAN: {
                std::stringstream s;
                s << std::boolalpha << i.second->boolean;
                str += '"' + i.first + "\": " + s.str();
                break;
            }
            }
            str += ", ";
        }

        str.erase(str.end() - 2, str.end());
        str += "}";

        return str;
    }
};