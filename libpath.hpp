#pragma once
#include <string>
#include <vector>
#include <cctype>

#include "libstrmanip.hpp"

class Path {
    vector_string pathObjects;

    std::string source;
    std::string vol;
    std::string sysPathType;

    bool absPath;
    bool dirPath;
    
    void clearPath() {
        replaceAll(source, "\\", "/");
        replaceAll(source, "\r", "");
        replaceAll(source, "\n", "");
        replaceAll(source, "\t", "");
        replaceAll(source, "?", "");
        replaceAll(source, ":", "");
        replaceAll(source, "*", "");
        replaceAll(source, "\"", "");
        replaceAll(source, "<", "");
        replaceAll(source, ">", "");
        replaceAll(source, "|", "");
    }

    void splitPath() {
        pathObjects = split(source, "/");
    }

    void recognizePath() {
        if (source[0] == '/' || (isalpha(source[0]) && source[1] == ':')) {
            absPath = true;

            if (source[0] == '/') {
                sysPathType = "unix";
                vol = "/";
            }

            else if (isalpha(source[0]) && source[1] == ':') {
                sysPathType = "win";
                vol = source[0];
                source.erase(0, 1);
            }
        }

        else {
            absPath = false;
            sysPathType = "relative";
        }

        dirPath = source.back() == '/' || source.back() == '\\';
    }

    void clear() {
        pathObjects.clear();
        vol.clear();
        sysPathType.clear();
        absPath = false;
    }
public:
    Path() {}
    Path(std::string path) {
        fromString(path);
    }

    Path(const char* path) {
        fromString(path);
    }

    Path(vector_string pathObjs, std::string type, std::string __vol, bool __abs) {
        pathObjects = pathObjs;
        sysPathType = type;
        vol = __vol;
        absPath = __abs;
    }

    Path& operator=(const std::string path) {
        fromString(path);

        return *this;
    }

    Path& operator=(const char* path) {
        fromString(path);

        return *this;
    }

    bool empty() {
        return !pathObjects.size();
    }

    std::string type() {
        return sysPathType;
    }

    std::string volume() {
        return vol;
    }

    void simplify() {
        for (int i = 0; i < pathObjects.size(); i++) {
            if (pathObjects[i] == "" || pathObjects[i] == ".") pathObjects.erase(pathObjects.begin() + i--);
            else if (pathObjects[i] == "..") {
                pathObjects.erase(pathObjects.begin() + i--);
                if (i >= 0) pathObjects.erase(pathObjects.begin() + i--);
            }
        }
    }

    vector_string pathParentDirs() {
        vector_string tmp = pathObjects;
        tmp.pop_back();

        return tmp;
    }

    std::string filename() {
        return (pathObjects.size()) ? pathObjects.back() : "";
    }

    std::string toString() {
        if (!pathObjects.size()) return "";

        std::string ret;

        if (absPath) {
            if (sysPathType == "unix") ret = vol;
            else if (sysPathType == "win") ret = toUpper(vol) + ":\\";
        }

        for (std::string& i : pathObjects) if (i.size()) ret += i + ((sysPathType == "win") ? "\\" : "/");

        if (ret != "/" && ret != toUpper(vol) + ":\\") {
            ret.pop_back();

            if (dirPath) ret += (sysPathType == "win") ? "\\" : "/";
        }

        return ret;
    }

    void fromString(std::string strpath) {
        clear();

        if (!strpath.size()) return;

        source = strpath;

        recognizePath();
        clearPath();
        splitPath();
    }
};

Path operator+(Path path1, Path path2) {
    if (path2.empty()) return path1;

    std::string p1 = path1.toString();
    std::string p2 = path2.toString();

    if (path2.type() == "win") p2 = p2.substr(3);
    else if (path2.type() == "unix") p2 = p2.substr(1);

    return Path(p1 + "/" + p2);
}

Path operator/(Path path1, Path path2) {
    return path1 + path2;
}

std::ostream& operator<<(std::ostream& ostr, Path path) {
    ostr << path.toString();

    return ostr;
}