// version 1.5.2
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <utility>
#include <sstream>
#include <fstream>

#pragma once

#include "libstrmanip.hpp"
#include "libunicodeseq.hpp"

enum JsonTypes {
    JSONNONE,
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

    bool parsing_error = false;

    JsonNode operator[](std::string str) { return (objects.find(str) != objects.end()) ? objects[str] : JsonNode(); }
    JsonNode operator[](size_t num) { return (num < array.size()) ? array[num] : JsonNode(); }

    bool is_null() { return type == JSONNONE; }

    size_t size() {
        if (type == JSONOBJECT) return objects.size();
        else if (type == JSONARRAY) return array.size();

        return 0;
    }

    JsonNode() {}
    JsonNode(std::string value) {
        type = JSONSTRING;
        str = value;
    }

    JsonNode(const char* value) {
        type = JSONSTRING;
        str = value;
    }

    JsonNode(short value) {
        type = JSONINTEGER;
        integer = value;
    }

    JsonNode(int value) {
        type = JSONINTEGER;
        integer = value;
    }

    JsonNode(long value) {
        type = JSONINTEGER;
        integer = value;
    }

    JsonNode(long long value) {
        type = JSONINTEGER;
        integer = value;
    }

    JsonNode(float value) {
        type = JSONFLOAT;
        lfloat = value;
    }

    JsonNode(double value) {
        type = JSONFLOAT;
        lfloat = value;
    }

    JsonNode(long double value) {
        type = JSONFLOAT;
        lfloat = value;
    }

    JsonNode(bool value) {
        type = JSONBOOLEAN;
        boolean = value;
    }

    JsonNode(std::vector<JsonNode> value) {
        type = JSONARRAY;
        array = value;
    }

    JsonNode(std::map<std::string, JsonNode> value) {
        type = JSONOBJECT;
        objects = value;
    }

    // void addPair(std::string key, std::string value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, const char* value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, short value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, int value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, long value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, long long value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, float value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, double value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, long double value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    // void addPair(std::string key, bool value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, std::vector<JsonNode> value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, std::map<std::string, JsonNode> value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    void addPair(std::string key, JsonNode value) { objects[key] = value; if (find(objectsOrder.begin(), objectsOrder.end(), key) == objectsOrder.end()) objectsOrder.push_back(key); type = JSONOBJECT; }
    void arrayAppend(JsonNode value) { array.push_back(value); type = JSONARRAY; }

    void clear() {
        type = JSONNONE;
        str.clear();
        integer = 0;
        lfloat = 0;
        boolean = false;

        for (auto i : array) i.clear();
        array.clear();

        objectsOrder.clear();

        for (auto i : objects) i.second.clear();
        objects.clear();
    }
};

class Json {
    bool check_prev(std::string& str, ssize_t pos, char arg) {
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
        ssize_t i;
        bool quotes = false;
        int br = 0;

        for (i = pos; i < (long)str.size(); i++) {
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

        return { {}, pos + 4 };
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

        return { tmp, i };
    }

    std::pair<JsonNode, size_t> parseInt(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j) || j == '-' || j == '+' || toupper(j) == 'E') tmp += j; 
        }

        return { std::stoll(unpackNumber(tmp)), i - 1 };
    }

    std::pair<JsonNode, size_t> parseFloat(std::string& str, size_t pos) {
        std::string tmp;
        size_t i = pos;
        size_t endpos = getEndPos(str, pos);

        for (; i < endpos; i++) {
            char j = str[i];
            if (isdigit(j) || j == '.' || j == '-' || j == '+' || toupper(j) == 'E') tmp += j; 
        }

        return { std::stold(unpackNumber(tmp)), i - 1 };
    }

    std::pair<JsonNode, size_t> parseBool(std::string& str, size_t pos) {
        if (str.substr(pos, 4) == "true") return { true, pos + 3 };
        return { false, pos + 4 };
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

        if (endpos == std::string::npos) { throw std::runtime_error("Syntax error: object is not closed"); }
        
        std::string key;
        bool iskey = false;
        bool isvalue = false;

        if (str[i] == '}' && str[i - 1] == '{') return { node, i };

        for (; i <= endpos; i++) {
            char token = str[i];

            if (token == '}') if (isvalue) { node.addPair(key, tmpobj); iskey = false; isvalue = false;} else { throw std::runtime_error("Syntax error: object missing value"); }

            else if (token == ':') if (iskey) key = tmpobj.str; else {  throw std::runtime_error("Syntax error: object missing key"); }

            else if (token == ',') if (isvalue) { node.addPair(key, tmpobj); iskey = false; isvalue = false;} else { throw std::runtime_error("Syntax error: object missing value"); }

            else if (token > 32 && token < 127) {
                auto tmp = parseAny(str, i);
                tmpobj = tmp.first;
                i = tmp.second;

                if (iskey == false && (tmp.first.type != JSONSTRING && tmp.first.type != JSONNONE)) { throw std::runtime_error("Syntax error: value must be a string"); }
                if (iskey && isvalue) { throw std::runtime_error("Syntax error: expected comma"); }
                if (iskey) isvalue = true;
                iskey = true;
            }
        }
        

        return { node, i - 1 };
    }
public:
    JsonNode parse(std::string str, bool _noexcept = false) {
        if (_noexcept) {
            try {
                return parseAny(str, findStart(str, 0)).first;
            } catch (...) {
                JsonNode ret;
                ret.parsing_error = true;

                return ret;
            }
        }

        else return parseAny(str, findStart(str, 0)).first;
    }

    JsonNode parse(std::ifstream& file, bool _noexcept = false) {
        std::stringstream tmp;
        tmp << file.rdbuf();
        std::string str = tmp.str();

        return parse(str, _noexcept);
    }

    std::string dump(JsonNode node) {
        std::string str;

        switch (node.type) {
        case JSONNONE: {
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
            str += std::to_string(node.lfloat);
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

            for (size_t i = 0; i < node.array.size(); i++) {
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

                    if (distance(node.objectsOrder.begin(), find(node.objectsOrder.begin(), node.objectsOrder.end(), i)) + 1 < (ssize_t)node.objects.size()) str += ", ";
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
