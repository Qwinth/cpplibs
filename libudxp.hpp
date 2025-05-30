#pragma once
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "libstrmanip.hpp"
#include "libbytesarray.hpp"

namespace udxp {
    struct __udxp_param_seq {
        std::string param;
        bool is_named;
    };

    class UDXPHeader {
        std::string name;

        vector_string positionalParams;
        std::map<std::string, vector_string> namedParams;

        std::vector<__udxp_param_seq> paramSeq;

        bool null;
    public:
        UDXPHeader() {}
        UDXPHeader(bool _null) : null(_null) {}

        std::string getName() {
            return name;
        }

        void setName(std::string _name) {
            name = _name;
        }

        bool hasParam(std::string param) {
            return std::find(positionalParams.begin(), positionalParams.end(), param) != positionalParams.end();
        }

        bool hasNamedParam(std::string param) {
            return namedParams.find(param) != namedParams.end();
        }

        std::string getParam(int index) {
            if (index < 0 || index >= positionalParams.size()) return {};
            return positionalParams[index];
        }

        vector_string getNamedParam(std::string param) {
            auto it = namedParams.find(param);

            if (it != namedParams.end()) return it->second;

            return {};
        }

        std::string operator[](int index) {
            return getParam(index);
        }

        vector_string operator[](std::string key) {
            return getNamedParam(key);
        }

        void addNamedParam(std::string key, vector_string value) {
            for (auto i : value) namedParams[key].push_back(i);
            paramSeq.push_back({key, true});
        }

        void addNamedParam(std::string key, std::string value) {
            addNamedParam(key, split(value, '|'));
        }

        void addParam(std::string param) {
            if (param.find('=') != std::string::npos) {
                vector_string tmp = split(param, '=');

                addNamedParam(tmp.front(), tmp.back());
            }
            
            else {
                positionalParams.push_back(param);
                paramSeq.push_back({param, false});
            }
        }

        void setNull(bool _null) {
            null = _null;
        }

        bool IsNull() {
            return null;
        }

        std::vector<__udxp_param_seq> __get_param_seq() {
            return paramSeq;
        }
    };

    class UDXPPacket {
        std::vector<UDXPHeader> headers;
        BytesArray data;

        bool null;
    public:
        UDXPPacket() {}
        UDXPPacket(bool _null) : null(_null) {}

        std::vector<UDXPHeader> getHeaders() {
            return headers;
        }

        void addHeader(UDXPHeader header) {
            headers.push_back(header);
        }

        void addHeader(std::string name, vector_string params) {
            UDXPHeader header;
            header.setName(name);
            
            for (auto i : params) header.addParam(i);

            headers.push_back(header);
        }

        void addHeader(std::string header) {
            vector_string header_tmp = split(header, ':', 1);
            vector_string param_tmp = split(header_tmp.back(), ',');

            addHeader(header_tmp.front(), param_tmp);
        }

        void removeHeader(std::string name) {
            for (int i = 0; i < headers.size(); i++) if (headers[i].getName() == name) {
                headers.erase(headers.begin() + i);
                break;
            }
        }

        UDXPHeader findHeader(std::string name) {
            for (auto i : headers) if (i.getName() == name) return i;

            return true;
        }

        BytesArray getData() {
            return data;
        }

        void setData(BytesArray _data) {
            data = _data;
        }

        bool IsNull() {
            return null;
        }

        void setNull(bool _null) {
            null = _null;
        }
    };

    bool checkHeadFull(std::string head) {
        return head.find('[') != head.npos && head.find(']') != head.npos;
    }

    UDXPPacket parse(std::string text) {
        if (!checkHeadFull(text)) return true;

        UDXPPacket packet;

        size_t startPos = text.find('[');
        size_t endPos = text.find(']');

        text.erase(startPos, 1);

        replaceAll(text, "\r", "");
        replaceAll(text, "\n", "");

        vector_string tmp = split(text, ']');

        std::string head = tmp.front();
        packet.setData(tmp.back());

        vector_string headers_str = split(head, ';');

        for (auto i : headers_str) packet.addHeader(i);

        packet.setNull(false);

        return packet;
    }

    UDXPPacket parse(BytesArray data) {
        return parse(data.toString());
    }

    std::string __dump_header_params_value(vector_string values) {
        std::string ret;

        for (auto i : values) ret += i + "|";
        ret.pop_back();

        return ret;
    }

    std::string __dump_header_params(UDXPHeader header) {
        std::string ret;
        std::vector<__udxp_param_seq> seq = header.__get_param_seq();

        for (auto i : seq) {
            ret += i.param;

            if (i.is_named) ret += "=" + __dump_header_params_value(header[i.param]);

            ret += ",";
        }

        ret.pop_back();

        return ret;
    }

    std::string dump(UDXPPacket packet) {
        std::string ret;

        if (packet.IsNull()) return {};

        ret += "[";

        for (auto i : packet.getHeaders()) ret += i.getName() + ":" + __dump_header_params(i) + ";";
        ret.pop_back();

        ret += "]";
        ret += packet.getData().toString();
        
        return ret;
    }
};