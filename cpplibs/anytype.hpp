#include <string>
#include <vector>
#include <map>

class AnyType {
public:
    std::string type = "none";
    std::string str;
    unsigned char* bytes;
    long long integer;
    long double lfloat;
    bool boolean;
    std::vector<AnyType> list;
    std::map<AnyType, AnyType> dict;
};