#ifndef ARGS_HPP
#define ARGS_HPP
#include "types.hpp"
#include <cstring>
#include <string>
ArgsInfo argparse(args_t args)
{
    ArgsInfo info;
    bool have_main_file = false;
    for (int i = 1; i < args.len; ++i) {
        const char* arg = args[i];
        if (strcmp(arg, "-obj") == 0) {
            info.out.as =
                ArgsInfo::Out::Object;
        }
        else if (strcmp(arg, "-cpp") == 0) {
            info.out.as =
                ArgsInfo::Out::Cpp;
        }
        else if (strcmp(arg, "-out") == 0) {
            if (i + 1 >= args.len)
                throw std::string(
                    "-out requires a filename"
                );
            info.out.name =
                args[++i];
        }
        else if (strcmp(arg, "-cpp-compiler") == 0) {
            if (i + 1 >= args.len)
                throw std::string(
                    "-cpp-compiler requires a compiler"
                );
            info.ccpp =
                args[++i];
        }
        else {
            if (have_main_file)
                throw std::string(
                    "can't have more than 1 main file"
                );
            info.main_file =
                arg;
            have_main_file = true;
        }
    }
    return info;
}
#endif