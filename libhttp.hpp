#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
// #include "cpplibs/libsocket.hpp"
#include "../cpplibs/libstrmanip.hpp"



template<typename T>
static size_t vector_indexOf(const std::vector<T>& vec, const T& t) {
    auto it = std::find(vec.begin(), vec.end(), t);
    return it != vec.end() ? std::distance(vec.begin(), it) : -1;
}

namespace http {
    enum HTTPMethod {
        HTTP_GET,
        HTTP_HEAD,
        HTTP_POST,
        HTTP_PUT,
        HTTP_DELETE,
        HTTP_CONNECT,
        HTTP_PATCH,
        HTTP_TRACE
    };

    std::map<std::string, std::string> contentType = { {"html", "text/html"}, {"htm", "text/html"}, {"txt", "text/plain"}, {"ico", "image/x-icon"}, {"css", "text/css"}, {"js", "text/javascript"}, {"jpg", "image/jpeg"},{"png", "image/png"}, {"gif", "image/gif"}, {"mp3", "audio/mpeg"}, {"ogg", "audio/ogg"}, {"wav", "audio/wav"}, {"opus", "audio/opus"}, {"m4a", "audio/mp4"}, {"mp4", "video/mp4"}, {"webm", "video/webm"}, {"pdf", "application/pdf"}, {"json", "text/json"}, {"xml", "text/xml"}, {"other", "application/octet-stream"} };
    std::map<int, std::string> strcode = { {200, "OK"}, {206, "Partial Content"}, {400, "Bad Request"}, {403, "Forbidden"}, {404, "Not Found"}, {500, "Internal Server Error"}, {501, "Not Implemented"} };
    vector_string implMethods = { "GET", "HEAD", "POST" };

    struct HTTPHeaderParam {
        bool valid = false;
        std::string value1;
        std::string value2;
        std::map<std::string, std::string> args;
    };

    using HTTPHeaderValue = std::vector<HTTPHeaderParam>;
    using HTTPHeaders = std::map<std::string, HTTPHeaderValue>;

    struct HTTPRequest {
        void setVersion(std::string v) { version = v; }
        void setMethod(HTTPMethod m) { method = m; }
        void setURI(std::string u) { uri = u; }
        void setBody(std::string b) { body = b; }

        void addHeader(std::string name, std::string value, std::string value2 = "") {
            HTTPHeaderParam param;
            param.value1 = value;
            param.value2 = value2;
            param.valid = true;

            headers[name].push_back(param);
        }

        void addHeader(std::string name, std::string value, std::map<std::string, std::string> params) {
            HTTPHeaderParam param;
            param.value1 = value;
            param.args = params;
            param.valid = true;

            headers[name].push_back(param);
        }

        void addHeader(std::string name, vector_string values) {
            for (std::string value : values) {
                HTTPHeaderParam param;
                param.value1 = value;
                param.valid = true;

                headers[name].push_back(param);
            }
        }

        void addHeader(std::string name, HTTPHeaderParam param) {
            param.valid = true;

            headers[name].push_back(param);
        }

        void addHeader(std::string name, HTTPHeaderValue param) {
            headers[name] = param;
        }

        void removeHeader(std::string name) {
            headers.erase(name);
        }

        HTTPMethod getMethod() const {
            return method;
        }

        std::string getURI() const {
            return uri;
        }

        std::string getVersion() const {
            return version;
        }

        std::string getBody() const {
            return body;
        }

        bool findHeader(std::string name) const {
            return headers.find(name) != headers.end();
        }

        HTTPHeaderValue getHeader(std::string name) const {
            return headers.at(name);
        }

        HTTPHeaders& getHeaders() {
            return headers;
        }

private:
        HTTPMethod method;
        std::string uri;
        std::string version;
        HTTPHeaders headers;
        std::string body;
    };

    struct HTTPResponse {
        void setCode(int c) { code = c; }
        void setVersion(std::string v) { version = v; }
        void setBody(std::string b) { body = b; }

        void addHeader(std::string name, std::string value, std::string value2 = "") {
            HTTPHeaderParam param;
            param.value1 = value;
            param.value2 = value2;
            param.valid = true;

            headers[name].push_back(param);
        }

        void addHeader(std::string name, std::string value, std::map<std::string, std::string> params) {
            HTTPHeaderParam param;
            param.value1 = value;
            param.args = params;
            param.valid = true;

            headers[name].push_back(param);
        }

        void addHeader(std::string name, vector_string values) {
            for (std::string value : values) {
                HTTPHeaderParam param;
                param.value1 = value;
                param.valid = true;

                headers[name].push_back(param);
            }
        }

        void addHeader(std::string name, HTTPHeaderParam param) {
            param.valid = true;

            headers[name].push_back(param);
        }

        void addHeader(std::string name, HTTPHeaderValue param) {
            headers[name] = param;
        }

        void removeHeader(std::string name) {
            headers.erase(name);
        }

        int getCode() const {
            return code;
        }

        std::string getVersion() const {
            return version;
        }

        std::string getBody() const {
            return body;
        }

        bool findHeader(std::string name) const {
            return headers.find(name) != headers.end();
        }

        HTTPHeaderValue getHeader(std::string name) const {
            return headers.at(name);
        }

        HTTPHeaders& getHeaders() {
            return headers;
        }

private:
        int code;
        std::string version;
        HTTPHeaders headers;
        std::string body;
    };

    HTTPHeaderParam findHeaderParam(std::string name, HTTPHeaderValue values){
        for (auto i : values) if (i.value1 == name) {
            i.valid = true;
            return i;
        }

        return {};
    }

    std::map<std::string, std::string> parse_http_uri_params(std::string str) {
        std::map<std::string, std::string> ret;

        std::string strparams = split(str, "?", 1)[1];

        for (auto& i : split(strparams, "&")) {
            auto tmp = split(i, "=", 1);

            ret[tmp[0]] = tmp[1];
        }

        return ret;
    }

    HTTPHeaderValue parse_http_header_params(std::string params) {
        HTTPHeaderValue ret;

        if (params.front() == '{') {
            HTTPHeaderParam param;
            param.value1 = params;
            param.valid = true;

            ret.push_back(param);

            return ret;
        }

        // while (params.find(' ') != std::string::npos) params.erase(params.find(' '), 1);
        
        replaceAll(params, ", ", ",");
        replaceAll(params, "; ", ";");

        vector_string str_args = split(params, ",");

        for (std::string& i : str_args) {
            HTTPHeaderParam param;
            param.valid = true;

            vector_string param_arg = split(i, ";", 1);
            vector_string _param_tmp = split(param_arg.back(), "=", 1);

            param.args[_param_tmp.front()] = _param_tmp.back();

            vector_string param_tmp = split(param_arg.front(), "=", 1);

            param.value1 = param_tmp.front();
            if (param_tmp.size() > 1) param.value2 = param_tmp.back();

            ret.push_back(param);
        }

        return ret;
    }

    HTTPRequest parse_http_request(std::string str) {
        HTTPRequest ret;

        vector_string head_body = split(str, "\r\n\r\n", 1);

        vector_string temp_head = split(head_body.front(), "\r\n", 1);
        vector_string head = split(temp_head.front(), " ");

        ret.setMethod((HTTPMethod)vector_indexOf(implMethods, head[0]));
        ret.setURI(head[1]);
        ret.setVersion(head[2]);
        ret.setBody(head_body.back());

        vector_string headers = split(temp_head.back(), "\r\n");

        for (std::string& i : headers) {
            vector_string header_temp = split(i, ": ", 1);

            if (header_temp.front() == "User-Agent") ret.addHeader(header_temp.front(), header_temp.back());
            else ret.addHeader(header_temp.front(), parse_http_header_params(header_temp.back()));
        }

        return ret;
    }

    HTTPResponse parse_http_response(std::string str) {
        HTTPResponse ret;

        vector_string head_body = split(str, "\r\n\r\n", 1);

        vector_string temp_head = split(head_body.front(), "\r\n", 1);
        vector_string head = split(temp_head.front(), " ");

        while (head.size() > 2) head.erase(head.end());

        ret.setVersion(head.front());
        ret.setCode(stoi(head.back()));
        ret.setBody(head_body.back());

        vector_string headers = split(temp_head.back(), "\r\n");

        for (std::string& i : headers) {
            vector_string header_temp = split(i, ": ", 1);

            if (header_temp.front() == "last-modified") ret.addHeader(header_temp.front(), header_temp.back());
            else if (header_temp.front() == "date") ret.addHeader(header_temp.front(), header_temp.back());
            else ret.addHeader(header_temp.front(), parse_http_header_params(header_temp.back()));
        }

        return ret;
    }

    std::string dump_http_header_value(HTTPHeaderValue value) {
        std::string ret;

        for (auto i : value) {
            if (!i.valid) continue;
            
            ret += i.value1;

            if (i.value2.size()) ret += "=" + i.value2;
            if (i.args.size()) for (auto [key, value] : i.args) ret += ";" + key + "=" + value;
            
            ret += ",";
        }

        ret.erase(ret.end() - 1);

        return ret;
    }

    std::string dump_http_request(HTTPRequest request) {
        std::string ret;

        ret += strformat("%s %s %s\r\n", implMethods[request.getMethod()].c_str(), request.getURI().c_str(), request.getVersion().c_str());

        for (auto [key, value] : request.getHeaders())ret += strformat("%s: %s\r\n", key.c_str(), dump_http_header_value(value).c_str());

        ret += "\r\n";
        ret += request.getBody();

        return ret;
    }

    std::string dump_http_response(HTTPResponse response) {
        std::string ret;

        ret += strformat("%s %d %s\r\n", response.getVersion().c_str(), response.getCode(), strcode[response.getCode()].c_str());

        for (auto [key, value] : response.getHeaders())ret += strformat("%s: %s\r\n", key.c_str(), dump_http_header_value(value).c_str());

        ret += "\r\n";
        ret += response.getBody();

        return ret;
    }

    HTTPResponse http_request(std::string url) {
        // std::function<void(int, short)> 

        return {};
    }

    struct HTTPFile {
        std::string name;
        std::string data;
    };

    struct HTTPMultipartFormData {
        std::map<std::string, std::string> formData;
        std::map<std::string, HTTPFile> formDataFiles;
        HTTPHeaders headers;
    };

    struct HTTPEndpointRequest : public HTTPRequest {
        std::map<std::string, std::string> endpointParams;
        std::map<std::string, HTTPFile> endpointFiles;
    };

    struct HTTPEndpointResponse : public HTTPResponse {
        std::map<std::string, std::string> endpointParams;
    };

    // HTTPMultipartFormData parse_http_multipart_form_data(std::string data, std::string boundary) {
    //     HTTPMultipartFormData ret;

    //     vector_string parts = split(data, "--" + boundary);

    //     if (parts.size()) parts.pop_back();

    //     for (std::string& i : parts) {
    //         vector_string head = split(i, "\r\n\r\n", 1);
    //         vector_string part_headers = split(head.front(), "\r\n");

    //         for (std::string& i : part_headers) {
    //             vector_string header_temp = split(i, ": ", 1);

    //             ret.headers[header_temp.front()] = parse_http_header_params(header_temp.back());
    //         }

    //         if (!findHeaderParam("form-data", ret.headers.at("Content-Disposition")).valid) throw std::runtime_error("parse_http_multipart_form_data(): only requests supported");
    //     }
    // }

    class HTTPEndpointServer {
        Socket server_socket;
    };
};