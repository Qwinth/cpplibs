// version 1.2-c1
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdarg>
#include <sstream>
#include "anytype.hpp"
#include "strlib.hpp"
#pragma once

struct arg_config {
    std::string flag1;
    std::string flag2;
    bool without_value;
    bool nargs = false;
    std::vector<std::string> action;
    Types type = ANYSTRING;
};

class ArgumentParser {
    std::vector<std::string> raw_args;
    std::vector<arg_config> args_config;
    std::map<std::string, AnyType> parsed_args;

    int find_argsconfig(std::string flag) {
        for (int i = 0; i < args_config.size(); i++) {
            if (args_config[i].flag1 == flag || args_config[i].flag2 == flag) return i;
        }

        return -1;
    }

    int find_raw_args(std::string flag) {
        for (int i = 0; i < raw_args.size(); i++) {
            if (split(raw_args[i], "=")[0] == flag) return i;
        }

        return -1;
    }

public:
    ArgumentParser(int argc, char** argv) {
        raw_args = std::vector<std::string>(argv, argv + argc);
    }

    void add_argument(arg_config config) {
        args_config.push_back(config);
    }

    std::map<std::string, AnyType> parse() {
        for (int j = 0; j < args_config.size(); j++) {
            auto i = args_config[j];
            if (find_raw_args(i.flag1) >= 0 || find_raw_args(i.flag2) >= 0) {
                std::string rawname = (find_raw_args(i.flag1) >= 0) ? i.flag1 : i.flag2;
                std::string retname = (i.flag2.length() > 0) ? i.flag2 : i.flag1;

                if (i.without_value) parsed_args[retname] = { .type = ANYBOOLEAN, .boolean = true };
                else {
                    if (i.nargs) {
                        AnyType obj;
                        obj.type = ANYLIST;

                        for (int k = find_raw_args(rawname) + 1; k < raw_args.size(); k++) {
                            if (find_argsconfig(raw_args[k]) >= 0) break;
                            obj.list.push_back(new AnyType({ .type = ANYSTRING, .str = raw_args[k] }));
                        }

                        parsed_args[retname] = obj;

                    } else {
                        std::string tmparg = raw_args[find_raw_args(rawname)];

                        if (tmparg.find("=") != tmparg.npos && split(tmparg, "=").size() > 1) parsed_args[split(tmparg, "=")[0]] = str2any(split(tmparg, "=")[1], i.type);
                        else if (find_raw_args(rawname) + 1 < raw_args.size()) parsed_args[retname] = str2any(raw_args[find_raw_args(rawname) + 1], i.type);
                        else parsed_args[retname] = { .type = ANYNONE, .boolean = false };
                    }
                }
            } else {
                if (i.flag2.length() > 0) parsed_args[i.flag2] = { .type = ANYNONE, .boolean = false };
                else if (i.flag1.length() > 0) parsed_args[i.flag1] = { .type = ANYNONE, .boolean = false };
            }
        }

        return parsed_args;
    }
};