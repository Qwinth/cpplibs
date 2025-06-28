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
    vector_string implMethods = { "GET", "HEAD", "POST", "PUT" };

    bool error_flag = false;

    struct __http_param_seq {
        std::string paramName;
        bool is_named;
    };

    struct __http_param_arg {
        std::string paramName;
        std::string argName;
        std::string argExtraValue;
        bool extraValue;
    };

    class HTTPHeader {
        std::string name;

        vector_string positionalParams;
        std::map<std::string, std::string> namedParams;
        std::string nonStandardParams;

        std::vector<__http_param_seq> paramSeq;
        std::vector<__http_param_arg> paramArgs;

        bool null;
public:
        HTTPHeader() {};
        HTTPHeader(bool _null) : null(_null) {}

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

        std::string getNamedParam(std::string param) {
            auto it = namedParams.find(param);

            if (it != namedParams.end()) return it->second;

            return {};
        }

        std::string operator[](int index) {
            if (index < 0 || index >= positionalParams.size()) return {};
            return positionalParams[index];
        }

        std::string operator[](std::string key) {
            return getNamedParam(key);
        }

        void addNamedParam(std::string key, std::string value) {
            for (auto i : value) namedParams[key].push_back(i);
            paramSeq.push_back({key, true});

            setNull(false);
        }

        void addParam(std::string param) {
            replaceAll(param, ", ", ",");
            replaceAll(param, "; ", ";");

            vector_string param_arg = split(param, ";");
            param = param_arg.front();

            param_arg.erase(param_arg.begin());

            vector_string tmp = split(param, '=');

            if (tmp.size() > 1) addNamedParam(tmp.front(), tmp.back());            
            else {
                positionalParams.push_back(param);
                paramSeq.push_back({param, false});

                setNull(false);
            }

            if (param_arg.size()) {
                for (auto i : param_arg) {
                    __http_param_arg arg;
                    arg.paramName = tmp.front();
                    
                    vector_string tmp2 = split(i, "=");

                    arg.argName = tmp2.front();
                    arg.argExtraValue = tmp2.back();
                    arg.extraValue = tmp2.size() > 1;

                    addParamArg(arg);
                }
            }
        }

        void addParamRaw(std::string param) {
            nonStandardParams = param;
        }

        std::string getParamRaw() {
            return nonStandardParams;
        }

        void addParamArg(__http_param_arg arg) {
            paramArgs.push_back(arg);
        }

        std::string findParamArg(std::string paramName, std::string argName) {
            for (auto i : paramArgs) if (i.paramName == paramName && i.argName == argName) return i.argExtraValue;
            
            return {};
        }

        bool IsNull() {
            return null;
        }

        void setNull(bool n) {
            null = n;
        }

        std::vector<__http_param_seq> __get_param_seq() {
            return paramSeq;
        }

        std::vector<__http_param_arg> __get_param_arg() {
            return paramArgs;
        }
    };

    using HTTPHeaders = std::vector<HTTPHeader>;

    class HTTPRequest {
        HTTPMethod method;
        std::string uri;
        std::string version;
        HTTPHeaders headers;
        std::string body;
public:
        void setVersion(std::string v) { version = v; }
        void setMethod(HTTPMethod m) { method = m; }
        void setURI(std::string u) { uri = u; }
        void setBody(std::string b) { body = b; }

        void addHeader(std::string name, vector_string params) {
            HTTPHeader header;
            header.setName(name);

            for (auto i : params) header.addParam(i);

            headers.push_back(header);
        }

        void addHeader(std::string name, std::string params) {
            addHeader(name, split(params, ","));
        }

        void addHeader(std::string header) {
            vector_string tmp = split(header, ":");

            addHeader(tmp.front(), tmp.back());
        }

        void addHeaderRaw(std::string name, std::string param) {
            HTTPHeader header;
            header.setName(name);
            header.addParamRaw(param);

            headers.push_back(header);
        }

        void removeHeader(std::string name) {
            for (size_t i = 0; i < headers.size(); i++) if (headers[i].getName() == name) headers.erase(headers.begin() + i);
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

        HTTPHeader findHeader(std::string name) {
            for (auto i : headers) if (i.getName() == name) return i;

            return true;
        }

        HTTPHeaders& getHeaders() {
            return headers;
        }
    };

    class HTTPResponse {
        int code;
        std::string version;
        HTTPHeaders headers;
        std::string body;
public:
        void setCode(int c) { code = c; }
        void setVersion(std::string v) { version = v; }
        void setBody(std::string b) { body = b; }

        void addHeader(std::string name, vector_string params) {
            HTTPHeader header;
            header.setName(name);

            for (auto i : params) header.addParam(i);

            headers.push_back(header);

        }

        void addHeader(std::string name, std::string params) {
            addHeader(name, split(params, ","));
        }

        void addHeader(std::string header) {
            vector_string tmp = split(header, ":");

            addHeader(tmp.front(), tmp.back());
        }

        void addHeaderRaw(std::string name, std::string param) {
            HTTPHeader header;
            header.setName(name);
            header.addParamRaw(param);

            headers.push_back(header);
        }

        void removeHeader(std::string name) {
            for (size_t i = 0; i < headers.size(); i++) if (headers[i].getName() == name) headers.erase(headers.begin() + i);
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

        HTTPHeader findHeader(std::string name) {
            for (auto i : headers) if (i.getName() == name) return i;

            HTTPHeader ret;
            ret.setNull(true);

            return ret;
        }

        HTTPHeaders& getHeaders() {
            return headers;
        }
    };

    std::map<std::string, std::string> parseHttpUriParams(std::string str) {
        std::map<std::string, std::string> ret;

        std::string strparams = split(str, "?", 1)[1];

        for (auto& i : split(strparams, "&")) {
            auto tmp = split(i, "=", 1);

            ret[tmp[0]] = tmp[1];
        }

        return ret;
    }

    // HTTPHeaderValue parse_http_header_params(std::string params) {
    //     HTTPHeaderValue ret;

    //     if (params.front() == '{') {
    //         HTTPHeaderParam param;
    //         param.value1 = params;
    //         param.valid = true;

    //         ret.push_back(param);

    //         return ret;
    //     }

    //     // while (params.find(' ') != std::string::npos) params.erase(params.find(' '), 1);
        
        

    //     vector_string str_args = split(params, ",");

    //     for (std::string& i : str_args) {
    //         HTTPHeaderParam param;
    //         param.valid = true;

    //         vector_string param_arg = split(i, ";", 1);
    //         vector_string _param_tmp = split(param_arg.back(), "=", 1);

    //         param.args[_param_tmp.front()] = _param_tmp.back();

    //         vector_string param_tmp = split(param_arg.front(), "=", 1);

    //         param.value1 = param_tmp.front();
    //         if (param_tmp.size() > 1) param.value2 = param_tmp.back();

    //         ret.push_back(param);
    //     }

    //     return ret;
    // }

    HTTPRequest parseHttpRequest(std::string str) {
        error_flag = false;
        HTTPRequest ret;

        vector_string head_body = split(str, "\r\n\r\n", 1);

        if (head_body.size() < 2) {
            error_flag = true;
            return {};
        }

        vector_string temp_head = split(head_body.front(), "\r\n", 1);

        if (temp_head.size() < 2) {
            error_flag = true;
            return {};
        }

        vector_string head = split(temp_head.front(), " ");

        if (head.size() < 3 || vector_indexOf(implMethods, head[0]) < 0 || split(head[2], "/").front() != "HTTP") {
            error_flag = true;
            return {};
        }

        ret.setMethod((HTTPMethod)vector_indexOf(implMethods, head[0]));
        ret.setURI(head[1]);
        ret.setVersion(head[2]);

        if (head_body.size() > 1) ret.setBody(head_body.back());

        vector_string headers = split(temp_head.back(), "\r\n");

        for (std::string& i : headers) {
            if (!i.size()) continue;
            
            vector_string header_temp = split(i, ": ", 1);

            if (header_temp.size() < 2 || !header_temp.back().size()) {
                error_flag = true;
                return {};
            }

            if (header_temp.front() == "User-Agent") ret.addHeaderRaw(header_temp.front(), header_temp.back());
            else ret.addHeader(header_temp.front(), header_temp.back());
        }

        return ret;
    }

    HTTPResponse parseHttpResponse(std::string str) {
        error_flag = false;
        HTTPResponse ret;

        vector_string head_body = split(str, "\r\n\r\n", 1);

        if (head_body.size() < 2) {
            error_flag = true;
            return {};
        }

        vector_string temp_head = split(head_body.front(), "\r\n", 1);

        if (temp_head.size() < 2) {
            error_flag = true;
            return {};
        }

        vector_string head = split(temp_head.front(), " ");

        while (head.size() > 2) head.erase(head.end());

        if (split(head.front(), "/").front() != "HTTP" || !std::all_of(head.back().begin(), head.back().end(), ::isdigit)) {
            error_flag = true;
            return {};
        }

        ret.setVersion(head.front());
        ret.setCode(stoi(head.back()));
        ret.setBody(head_body.back());

        vector_string headers = split(temp_head.back(), "\r\n");

        for (std::string& i : headers) {
            if (!i.size()) continue;

            vector_string header_temp = split(i, ": ", 1);

            if (header_temp.size() < 2 || !header_temp.back().size()) {
                error_flag = true;
                return {};
            }

            if (header_temp.front() == "last-modified") ret.addHeaderRaw(header_temp.front(), header_temp.back());
            else if (header_temp.front() == "date") ret.addHeaderRaw(header_temp.front(), header_temp.back());
            else if (header_temp.back().front() == '{') ret.addHeaderRaw(header_temp.front(), header_temp.back());
            else ret.addHeader(header_temp.front(), header_temp.back());
        }

        error_flag = false;

        return ret;
    }

    std::string dumpHttpHeaderParams(HTTPHeader value) {
        if (value.getParamRaw().size()) return value.getParamRaw();

        std::string ret;

        for (auto i : value.__get_param_seq()) {           
            ret += i.paramName;

            if (i.is_named) ret += "=" + value.getNamedParam(i.paramName);
            
            for (auto j : value.__get_param_arg()) {
                if (j.paramName != i.paramName) continue;

                if (!j.extraValue) ret += ";" + j.argName;
                else ret += ";" + j.argName + "=" + j.argExtraValue;
            }
            
            ret += ",";
        }

        ret.erase(ret.end() - 1);

        return ret;
    }

    std::string dumpHttpRequest(HTTPRequest request) {
        std::string ret;

        ret += strformat("%s %s %s\r\n", implMethods[request.getMethod()].c_str(), request.getURI().c_str(), request.getVersion().c_str());

        for (auto i : request.getHeaders())ret += strformat("%s: %s\r\n", i.getName().c_str(), dumpHttpHeaderParams(i).c_str());

        ret += "\r\n";
        ret += request.getBody();

        return ret;
    }

    std::string dumpHttpResponse(HTTPResponse response) {
        std::string ret;

        ret += strformat("%s %d %s\r\n", response.getVersion().c_str(), response.getCode(), strcode[response.getCode()].c_str());

        for (auto i : response.getHeaders())ret += strformat("%s: %s\r\n", i.getName().c_str(), dumpHttpHeaderParams(i).c_str());

        ret += "\r\n";
        ret += response.getBody();

        return ret;
    }

    // HTTPResponse http_request(std::string url) {
    //     // std::function<void(int, short)> 

    //     return {};
    // }

    // struct HTTPFile {
    //     std::string name;
    //     std::string data;
    // };

    // struct HTTPMultipartFormData {
    //     std::map<std::string, std::string> formData;
    //     std::map<std::string, HTTPFile> formDataFiles;
    //     HTTPHeaders headers;
    // };

    // struct HTTPEndpointRequest : public HTTPRequest {
    //     std::map<std::string, std::string> endpointParams;
    //     std::map<std::string, HTTPFile> endpointFiles;
    // };

    // struct HTTPEndpointResponse : public HTTPResponse {
    //     std::map<std::string, std::string> endpointParams;
    // };

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

    // class HTTPEndpointServer {
    //     // Socket server_socket;
    // };
};