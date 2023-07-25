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
    JSONARRAY,
    JSONOBJECT
};

class JsonNode {
public:
    JsonTypes type = JSONNULL;
    std::string str;
    long long integer = 0;
    long double lfloat = 0;
    bool boolean = false;
    std::vector<JsonNode> array;
    std::map<std::string, JsonNode> objects;

    JsonNode operator[](std::string str) { return objects[str]; }
    JsonNode operator[](size_t num) { return array[num]; }

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
            for (AnyType* i : value.list) node.array.push_back(JsonNode(AnyType_to_JsonNode(*i)));
            break;
        }
        }

        return node;
    }

public:
    void addPair(std::string key, std::string value) { objects[key] = {.type = JSONSTRING, .str = value}; }
    void addPair(std::string key, const char* value) { objects[key] = {.type = JSONSTRING, .str = value}; }
    void addPair(std::string key, short value) { objects[key] = {.type = JSONINTEGER, .integer = value}; }
    void addPair(std::string key, int value) { objects[key] = {.type = JSONINTEGER, .integer = value}; }
    void addPair(std::string key, long value) { objects[key] = {.type = JSONINTEGER, .integer = value}; }
    void addPair(std::string key, long long value) { objects[key] = {.type = JSONINTEGER, .integer = value}; }
    void addPair(std::string key, float value) { objects[key] = {.type = JSONFLOAT, .lfloat = value}; }
    void addPair(std::string key, double value) { objects[key] = {.type = JSONFLOAT, .lfloat = value}; }
    void addPair(std::string key, long double value) { objects[key] = {.type = JSONFLOAT, .lfloat = value}; }
    void addPair(std::string key, bool value) { objects[key] = {.type = JSONBOOLEAN, .boolean = value}; }
    void addPair(std::string key, std::vector<JsonNode> value) { objects[key] = {.type = JSONARRAY, .array = value}; }
    void addPair(std::string key, std::map<std::string, JsonNode> value) { objects[key] = {.type = JSONOBJECT, .objects = value}; }
    void addPair(std::string key, JsonNode value) { objects[key] = value; }
    void addPair(std::string key, AnyType value) { objects[key] = (AnyType_to_JsonNode(value)); }
    void arrayAppend(JsonNode value) { array.push_back(value); }
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
        size_t tmppos = std::string::npos;
        if ((tmppos = str.find(',', pos)) != std::string::npos) return tmppos;
        else if ((tmppos = str.find('}', pos)) != std::string::npos) return tmppos;
        else if ((tmppos = str.find(']', pos)) != std::string::npos) return tmppos;

        return tmppos;
    }

    size_t getBrClosePos(std::string& str, size_t pos, char brOpen, char brClose) {
        size_t i = pos;
        int br = 0;

        for (; i < str.size(); i++) {
            char j = str[i];

            if (j == brOpen) br++;
            else if (j == brClose) br--;

            if (br == 0) break;
        }

        return i;
    }

    bool is_float(std::string& str, size_t pos) {
        size_t endpos = getEndPos(str, pos);

        for (size_t i = pos; i < endpos; i++) {
            char j = str[i];

            if (isdigit(str[i - 1]) && j == '.' && isdigit(str[i + 1])) return true;
        }

        return false;
    }

    std::pair<JsonNode, size_t> parseAny(std::string& str, size_t pos) {
        if (str[pos] == '"') return parseString(str, pos);
        else if (isdigit(str[pos]) && !is_float(str, pos)) return parseInt(str, pos);
        else if (isdigit(str[pos]) && is_float(str, pos)) return parseFloat(str, pos);
        else if (str[pos] == 't' || str[pos] == 'f') return parseBool(str, pos);
        else if (str[pos] == '[') return parseArray(str, pos);
        else if (str[pos] == '{') return parseObject(str, pos);
        return { {}, pos };
    }

    std::pair<JsonNode, size_t> parseString(std::string& str, size_t pos) {
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

        tmp = decodeUnicodeSequence(tmp);

        return { { .type = JSONSTRING, .str = tmp }, i };
    }

    std::pair<JsonNode, size_t> parseInt(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j)) tmp += j; 
        }

        return { { .type = JSONINTEGER, .integer = std::stoll(tmp) }, i - 1 };
    }

    std::pair<JsonNode, size_t> parseFloat(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j) || j == '.') tmp += j; 
        }

        return { { .type = JSONFLOAT, .lfloat = std::stold(tmp) }, i - 1 };
    }

    std::pair<JsonNode, size_t> parseBool(std::string& str, size_t pos) {
        if (str.substr(pos, 4) == "true") return { { .type = JSONBOOLEAN, .boolean = true }, pos + 3 };
        return { { .type = JSONBOOLEAN, .boolean = false }, pos + 4 };
    }

    std::pair<JsonNode, size_t> parseArray(std::string& str, size_t pos) {
        JsonNode node;
        node.type = JSONARRAY;
        JsonNode tmpobj;
        
        size_t i = (str[pos] != '[') ? pos : pos + 1;
        size_t endpos = getBrClosePos(str, pos, '[', ']');

        for (; i <= endpos; i++) {
            char token = str[i];

            if (token == ',' || token == ']') {node.arrayAppend(tmpobj); }

            else if (token > 32 && token < 127) {
                auto tmp = parseAny(str, i);
                tmpobj = tmp.first;
                i = tmp.second;
            }
        }

        return { node, i - 1 };
    }

    std::pair<JsonNode, size_t> parseObject(std::string& str, size_t pos) {
        JsonNode node;
        node.type = JSONOBJECT;
        JsonNode tmpobj;
        
        size_t i = (str[pos] != '{') ? pos : pos + 1;
        size_t endpos = getBrClosePos(str, pos, '{', '}');
        std::string key;

        for (; i <= endpos; i++) {
            char token = str[i];

            if (token == '}') node.addPair(key, tmpobj);

            else if (token == ':') key = tmpobj.str;

            else if (token == ',') node.addPair(key, tmpobj);

            else if (token > 32 && token < 127) {
                auto tmp = parseAny(str, i);
                tmpobj = tmp.first;
                i = tmp.second;
            }
        }

        return { node, i - 1 };
    }
public:
    JsonNode parse(std::string str) { return parseAny(str, 0).first; }

    std::string dump(JsonNode node) {
        std::string str;

        switch (node.type) {
        case JSONNULL: {
            str += "null";
            break;
        }

        case JSONSTRING: {
            
            str +=  '"' + node.str + '"';
            break;
        }

        case JSONINTEGER: {
            str +=  std::to_string(node.integer);
            break;
        }

        case JSONFLOAT: {
            str += to_stringWp(node.lfloat, 15);
            break;
        }

        case JSONBOOLEAN: {
            std::stringstream s;
            s << std::boolalpha << node.boolean;
            str += s.str();
            break;
        }

        case JSONARRAY: {
            str += "[";

            for (int i = 0; i < node.array.size(); i++) {
                str += dump(node[i]);

                if (i + 1 < node.array.size()) str += ", ";
            }
            str += "]";
            break;
        }

        case JSONOBJECT: {
            str += "{";

            for (auto i : node.objects) {
                str += '"' + i.first + "\": ";
                str += dump(i.second);

                if (distance(node.objects.begin(), node.objects.find(i.first)) + 1 < node.objects.size()) str += ", ";
            }
            str += "}";
            break;
        }
        }
        return str;
    }
};
