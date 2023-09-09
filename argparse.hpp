// version 1.0
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

    void add_argument(arg_config config) {
        args_config.push_back(config);
    }

    std::map<std::string, AnyType> parse() {

        for (auto i : raw_args) {
            if (i.find("=") != i.npos && find_argsconfig(split(i, "=")[0]) >= 0 && split(i, "=").size() > 1) {
                parsed_args[split(i, "=")[0]] = { ANYSTRING, split(i, "=")[1] };
                args_config.erase(args_config.begin() + find_argsconfig(split(i, "=")[0]));
            }
        }

        for (int j = 0; j < args_config.size(); j++) {
            auto i = args_config[j];
            if (find_raw_args(i.flag1) >= 0 || find_raw_args(i.flag2) >= 0) {
                std::string rawname = (find_raw_args(i.flag1) >= 0) ? i.flag1 : i.flag2;
                std::string retname = (i.flag2.length() > 0) ? i.flag2 : i.flag1;

                if (i.without_value) {
                    parsed_args[retname] = { .type = ANYBOOLEAN, .boolean = true };
                } else {
                    if (i.nargs) {
                        AnyType obj;
                        obj.type = ANYLIST;

                        for (int k = find_raw_args(rawname) + 1; k < raw_args.size(); k++) {
                            if (find_argsconfig(raw_args[k]) >= 0) break;
                            obj.list.push_back( new AnyType({ .type = ANYSTRING, .str = raw_args[k] }));
                        }

                        parsed_args[retname] = obj;

                    } else {
                        if (i.type == "string" && find_raw_args(rawname) + 1 < raw_args.size()) parsed_args[retname] = { .type = ANYSTRING, .str = raw_args[find_raw_args(rawname) + 1] };
                        else if (i.type == "int" && find_raw_args(rawname) + 1 < raw_args.size()) parsed_args[retname] = { .type = ANYINTEGER, .integer = stoll(raw_args[find_raw_args(rawname) + 1]) };
                        else if (i.type == "float" && find_raw_args(rawname) + 1 < raw_args.size()) parsed_args[retname] = { .type = ANYFLOAT, .lfloat = stold(raw_args[find_raw_args(rawname) + 1]) };
                        else if (i.type == "bool" && find_raw_args(rawname) + 1 < raw_args.size()) {
                            bool b;
                            std::istringstream(raw_args[find_raw_args(rawname) + 1]) >> std::boolalpha >> b;
                            parsed_args[retname] = { .type = ANYBOOLEAN, .boolean = b };
                        } else {
                            parsed_args[retname] = { ANYSTRING, "false" };
                        }
                    }
                }
            } else {
                if (i.flag2.length() > 0) parsed_args[i.flag2] = { .type = ANYBOOLEAN, .str = "false", .boolean = false };
                else if (i.flag1.length() > 0) parsed_args[i.flag1] = { .type = ANYBOOLEAN, .str = "false", .boolean = false };
            }
        }

        return parsed_args;
    }
};
