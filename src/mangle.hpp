#ifndef MANGLE_HPP
#define MANGLE_HPP
#include "../libs/transcode/transcode.hpp"
#include <vector>
#include <algorithm>
namespace c {
    class Mangler {
        public:
            tc::Opt<tc::String> mangle(tc::String name) const {
                if (!verify(name))
                    return tc::NullOpt;
                if (! have_namespaces(name))
                    return name;
                size_t pos = 0;
                std::vector<tc::String> parts;
                parts.reserve(std::count(name.begin(), name.end(), ':') +1);
                while (true) {
                    size_t colonpos = name.find(':',pos);
                    if (colonpos == tc::String::npos) {
                        parts.push_back(name.substr(pos));
                        break;
                    }
                    parts.push_back(name.substr(pos, colonpos-pos));
                    pos = colonpos+1;
                }
                size_t i = 0;
                tc::String res = "_I";
                while (i < parts.size()) {
                    res += std::to_string(parts[i].size()) + parts[i];
                    i++;
                }
                return res;
            }
        private:
            bool verify(tc::StrView name) const{
                char actual = '\0';
                char previous = '\0';

                for (size_t i = 0;i < name.size(); i++) {
                    actual = name[i];

                    bool colon =
                        actual == ':';
                    bool alfa =
                        std::isalpha((unsigned char)actual);
                    bool digit = 
                        std::isdigit((unsigned char)actual);
                    bool underscore =
                        actual == '_';
                    bool any = 
                        colon || alfa || digit || underscore;
                    bool alnum =
                        alfa || digit;

                    bool invalid =
                        ! any                                 ||
                        digit && previous == '\0'             ||
                        colon && previous == ':'              ||
                        colon && i == name.size()-1
                    ;
                    if (invalid) 
                        return false;

                    previous = actual;
                }
                return !name.empty();
            }
            bool have_namespaces(tc::StrView name) const {
                if (!verify(name))
                    return false;
                return name.find(':') != tc::StrView::npos;
            }
    };
}

#endif