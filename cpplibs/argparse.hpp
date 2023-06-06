#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdarg>
#include <sstream>
#include "anytype.hpp"
#include "strlib.hpp"

struct arg_config {
    std::string flag1;
    std::string flag2;
    bool without_value;
    bool nargs = false;
    std::vector<std::string> action;
    std::string type = "string";
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
            if (raw_args[i] == flag) return i;
        }

        return -1;
    }

public:
    ArgumentParser(int argc, char** argv) {
        raw_args = std::vector<std::string>(argv, argv + argc);
    }

    void add_argument(arg_config config, ...) {
        va_list list;
        va_start(list, config);
        args_config.push_back(config);
    }

    std::map<std::string, AnyType> parse() {

        for (auto i : raw_args) {
            if (i.find("=") != i.npos && find_argsconfig(split(i, "=")[0]) >= 0 && split(i, "=").size() > 1) {
                parsed_args[split(i, "=")[0]] = {"string", split(i, "=")[1]};
                args_config.erase(args_config.begin() + find_argsconfig(split(i, "=")[0]));
            }
        }

        for (int j = 0; j < args_config.size(); j++) {
            auto i = args_config[j];
            if (find_raw_args(i.flag1) >= 0 || find_raw_args(i.flag2) >= 0) {
                std::string name = (find_raw_args(i.flag1) >= 0) ? i.flag1 : i.flag2;

                if (i.without_value) {
                    parsed_args[name] = {"boolean", .boolean = true};
                } else {
                    if (i.nargs) {
                        AnyType obj;
                        obj.type = "list";

                        for (int k = find_raw_args(name) + 1; k < raw_args.size(); k++) {
                            if (find_argsconfig(raw_args[k]) >= 0) break;
                            obj.list.push_back({"string", .str = raw_args[k]});
                        }

                        parsed_args[name] = obj;

                    } else {
                        if (i.type == "string" && find_raw_args(name) + 1 < raw_args.size()) parsed_args[name] = {"string", .str = raw_args[find_raw_args(name) + 1]};
                        else if (i.type == "int" && find_raw_args(name) + 1 < raw_args.size()) parsed_args[name] = {"int", .integer = stoll(raw_args[find_raw_args(name) + 1])};
                        else if (i.type == "float" && find_raw_args(name) + 1 < raw_args.size()) parsed_args[name] = {"float", .lfloat = stold(raw_args[find_raw_args(name) + 1])};
                        else if (i.type == "bool" && find_raw_args(name) + 1 < raw_args.size()) {
                            bool b;
                            std::istringstream(raw_args[find_raw_args(name) + 1]) >> std::boolalpha >> b;
                            parsed_args[name] = {"boolean", .boolean = b};
                        } else {
                            parsed_args[name] = {"string", "false"};
                        }
                    }
                }
            } else {
                if (i.flag1.length() > 0) parsed_args[i.flag1] = {"boolean", .str = "false", .boolean = false};
                else if (i.flag2.length() > 0) parsed_args[i.flag2] = {"boolean", .str = "false", .boolean = false};
            }
        }

        return parsed_args;
    }
};
