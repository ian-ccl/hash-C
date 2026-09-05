#ifndef COMPILE_HPP
#define COMPILE_HPP

#include "config.h"
#include "externs.hpp"

#include <vector>
#include <string> 
#include <cstdlib>
#include <iostream>

void compile() {

    if (info.out.as == ArgsInfo::Out::As::Cpp) return;

    std::string cmd = info.ccpp + " std.tmp.cpp";
    for (size_t i = 0; i < tmp_files.size(); i++) {
        cmd += " " + tmp_files[i];
    }

    if (info.out.as == ArgsInfo::Out::As::Object) {
        cmd += " -c";
    }

    for (const std::string& flag : info.cpp_flags) {
        cmd += " " + flag;
    }
    cmd += " -o " + info.out.name;

    ProcessResult out = shell(cmd);

    if (out.exit_code != 0) {
        std::cerr << "\033[33m##########\nc++ errors\n##########\n" << out.stderr << "\n##########\n\033[0m";
    }

    for (size_t i = 0; i < tmp_files.size(); i++) {
        int code = std::remove(tmp_files[i].c_str());

        if (code != 0) {
            std::cerr << "error removing files\n";
        }
    }

    {
        int code = std::remove("std.tmp.cpp");

        if (code != 0) {
            std::cerr << "error removing files\n";
        }
    }
    {
        int code = std::remove("std.tmp.hpp");

        if (code != 0) {
            std::cerr << "error removing files\n";
        }
    }
}

#endif