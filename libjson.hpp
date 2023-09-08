#pragma once
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include <fstream>
#include "anytype.hpp"
#include "strlib.hpp"

enum JsonTypes {
    JSONNONE,
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
    JsonTypes type = JSONNONE;
    std::string str;
    long long integer = 0;
    long double lfloat = 0;
    bool boolean = false;
    std::vector<JsonNode> array;
    std::vector<std::string> objectsOrder;
    std::map<std::string, JsonNode> objects;

    JsonNode operator[](std::string str) { return objects[str]; }
    JsonNode operator[](size_t num) { return array[num]; }

    bool is_null() { return type == JSONNULL; }

    size_t size() {
        if (type == JSONOBJECT) return objects.size();
        else if (type == JSONARRAY) return array.size();

        return 0;
    }

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
    void addPair(std::string key, std::string value) { objects[key] = {.type = JSONSTRING, .str = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, const char* value) { objects[key] = {.type = JSONSTRING, .str = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, short value) { objects[key] = {.type = JSONINTEGER, .integer = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, int value) { objects[key] = {.type = JSONINTEGER, .integer = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, long value) { objects[key] = {.type = JSONINTEGER, .integer = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, long long value) { objects[key] = {.type = JSONINTEGER, .integer = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, float value) { objects[key] = {.type = JSONFLOAT, .lfloat = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, double value) { objects[key] = {.type = JSONFLOAT, .lfloat = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, long double value) { objects[key] = {.type = JSONFLOAT, .lfloat = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, bool value) { objects[key] = {.type = JSONBOOLEAN, .boolean = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, std::vector<JsonNode> value) { objects[key] = {.type = JSONARRAY, .array = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, std::map<std::string, JsonNode> value) { objects[key] = {.type = JSONOBJECT, .objects = value}; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, JsonNode value) { objects[key] = value; objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, AnyType value) { objects[key] = (AnyType_to_JsonNode(value)); objectsOrder.push_back(key); type = JSONOBJECT; }
    void arrayAppend(JsonNode value) { array.push_back(value); type = JSONARRAY; }
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
        std::vector<size_t> poss;
        poss.push_back(str.size());
        poss.push_back(str.find(',', pos));
        poss.push_back(str.find('}', pos));
        poss.push_back(str.find(']', pos));        

        return *std::min_element(poss.begin(), poss.end());
    }

    size_t getBrClosePos(std::string& str, size_t pos, char brOpen, char brClose) {
        size_t ret = std::string::npos;
        size_t i;
        bool quotes = false;
        int br = 0;

        for (i = pos; i < str.size(); i++) {
            char j = str[i];

            if (j == '"' && (i - 1 < 0 || str[i - 1] != '\\')) quotes = !quotes;

            if (j == brOpen && !quotes) br++;
            else if (j == brClose && !quotes) br--;

            if (br == 0) { ret = i; break; }
        }

        return ret;
    }

    bool is_float(std::string& str, size_t pos) {
        size_t endpos = getEndPos(str, pos);

        for (size_t i = pos; i < endpos; i++) {
            char j = str[i];

            if (j == '-') continue;
            if (isdigit(str[i - 1]) && j == '.' && isdigit(str[i + 1])) return true;
            else if (toupper(j) == 'E' && str[i + 1] == '-') return true;
        }

        return false;
    }

    size_t findStart(std::string& str, size_t pos) {
        size_t startpos = pos;

        for (; startpos < str.size(); startpos++) if (str[startpos] > 32) break;

        return startpos;
    } 

    std::pair<JsonNode, size_t> parseAny(std::string& str, size_t pos) {
        if (str[pos] == '"') return parseString(str, pos);
        else if ((isdigit(str[pos]) || (str[pos] == '-' && isdigit(str[pos + 1]))) && !is_float(str, pos)) return parseInt(str, pos);
        else if ((isdigit(str[pos]) || (str[pos] == '-' && isdigit(str[pos + 1]))) && is_float(str, pos)) return parseFloat(str, pos);
        else if (str[pos] == 't' || str[pos] == 'f') return parseBool(str, pos);
        else if (str[pos] == '[') return parseArray(str, pos);
        else if (str[pos] == '{') return parseObject(str, pos);
        else if (!str.size()) return { {}, pos };

        return { {.type = JSONNULL}, pos };
    }

    std::pair<JsonNode, size_t> parseString(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos + 1;

        for (; i < str.size(); i++) {
            char j = str[i];

            if (j == '"') break;
            else if (j == '\\' && str[i + 1] == 'u') {
                tmp += decodeUnicodeSequence(str.substr(i, 6));
                i += 5;
                continue;
            }
            else if (j == '\\') {
                tmp += escape_sequences_reverse[std::string(1, j) + str[i + 1]];
                i++;
                continue;
            } else {
                tmp += j;
            }
        }

        return { { .type = JSONSTRING, .str = tmp }, i };
    }

    std::pair<JsonNode, size_t> parseInt(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j) || j == '-' || j == '+' || toupper(j) == 'E') tmp += j; 
        }

        return { { .type = JSONINTEGER, .integer = std::stoll(unpackNumber(tmp)) }, i - 1 };
    }

    std::pair<JsonNode, size_t> parseFloat(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j) || j == '.' || j == '-' || j == '+' || toupper(j) == 'E') tmp += j; 
        }

        return { { .type = JSONFLOAT, .lfloat = std::stold(unpackNumber(tmp)) }, i - 1 };
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

        if (endpos == std::string::npos) { std::cout << "Syntax error: array is not closed" << std::endl; exit(1); }

        for (; i <= endpos; i++) {
            char token = str[i];

            if (token == ',' || token == ']') { node.arrayAppend(tmpobj); }

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

        if (endpos == std::string::npos) { std::cout << "Syntax error: object is not closed" << std::endl; exit(1); }
        
        std::string key;
        bool iskey = false;
        bool isvalue = false;

        if (str[i] == '}' && str[i - 1] == '{') return { node, i };

        for (; i <= endpos; i++) {
            char token = str[i];

            if (token == '}') if (isvalue) { node.addPair(key, tmpobj); iskey = false; isvalue = false;} else { std::cout << "Syntax error: object missing value" << std::endl; exit(1); }

            else if (token == ':') if (iskey) key = tmpobj.str; else { std::cout << "Syntax error: object missing key" << std::endl; exit(1); }

            else if (token == ',') if (isvalue) { node.addPair(key, tmpobj); iskey = false; isvalue = false;} else { std::cout << "Syntax error: object missing value" << std::endl; exit(1); }

            else if (token > 32 && token < 127) {
                if (iskey == false && token != '"') { std::cout << "Syntax error: value must be a string" << std::endl; exit(1); }

                auto tmp = parseAny(str, i);
                tmpobj = tmp.first;
                i = tmp.second;

                if (iskey && isvalue) { std::cout << "Syntax error: expected comma" << std::endl; exit(1); }
                if (iskey) isvalue = true;
                iskey = true;
                // std::cout << "char: " << token << " pos: " << i << std::endl;
            }
        }
        

        return { node, i - 1 };
    }
public:
    JsonNode parse(std::string str) { return parseAny(str, findStart(str, 0)).first; }
    JsonNode parse(std::ifstream& file) {
        std::stringstream tmp;
        tmp << file.rdbuf();
        std::string str = tmp.str();

        return parseAny(str, findStart(str, 0)).first;
    }

    std::string dump(JsonNode node) {
        std::string str;

        switch (node.type) {
        case JSONNULL: {
            str += "null";
            break;
        }

        case JSONSTRING: {
            replaceAll(node.str, "\\", "\\\\");
            for (auto i : escape_sequences) {
                if (i.second == "\\\\" || i.second == "\\'") continue;
                replaceAll(node.str, i.first, i.second);
            }

            str +=  '"' + encodeUnicodeSequence(node.str) + '"';
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
            std::vector<std::string> usedKeys;
            str += "{";

            for (auto i : node.objectsOrder) {
                auto j = node.objects[i];

                if (find(usedKeys.begin(), usedKeys.end(), i) == usedKeys.end()) {
                    usedKeys.push_back(i);

                    str += '"' + encodeUnicodeSequence(i) + "\": ";
                    str += dump(j);

                    if (distance(node.objectsOrder.begin(), find(node.objectsOrder.begin(), node.objectsOrder.end(), i)) + 1 < node.objects.size()) str += ", ";
                }

                else std::cout << "Warning: object dublicated key" << std::endl;
            }
            str += "}";
            break;
        }
        }
        return str;
    }
};
