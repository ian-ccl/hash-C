#include "../libs/transcode/transcode.hpp"
#include "types.hpp"
#include "transpile.hpp"
#include "compile.hpp"
#include "templates.hpp"
#include "modules.hpp"
#include "fs.hpp"
#include "std.hpp"
#include "stddef.hpp"
#include "hctypes.hpp"
#include <iostream>
#include <filesystem>
#include <cstring>
using namespace std::string_literals;
int main(int argc, char** argv)
{
    try {
        if (argc < 2) {
            std::cerr
                << "usage: "
                << argv[0]
                << " <file>\n";

            return 1;
        }
        bool is_combrobe_compilation = false;
        bool generate_object_file = false;
        std::vector<const char*> cppargs;
        if (argc >= 3) {
            for (size_t i = 2; i < argc; i++) {
                const char* s = argv[i];
                if (strcmp(s, "--comprobe") == 0) {
                    is_combrobe_compilation = true;
                    continue;
                }
                if (strcmp(s, "-c") == 0) {
                    generate_object_file = true;
                    continue;
                }
                cppargs.push_back(s);
            }
        }

        std::filesystem::path input_path(
            argv[1]
        );
        
        std::vector<c::LoadedModule> modules =
            c::process_modules(
                input_path.string()
            );
        std::vector<std::filesystem::path> files;
        tc::Set<tc::String> types = {
            "int",
            "uint",
            "float",
            "bool",
            "void",
            "char"
        };
        for (
            c::LoadedModule& mod : modules
        ) {
            tc::Source source;

            source.name =
                mod.filename;

            source.source =
                std::move(mod.code);
            c::get_templates(source);
            c::remove_templates(source);
            c::get_types(source, types);
            mod.code = std::move(source.source);
        }
        for (
            c::LoadedModule& mod : modules
        ) {
            tc::Source source;

            source.name =
                mod.filename;

            source.source =
                std::move(mod.code);
            
            c::remove_templates(source);
            mod.code = std::move(source.source);
            c::get_types(source, types);
        }
        
        for (
            const c::LoadedModule& mod
            : modules
        ) {
            if constexpr (DEBUG)
                std::cout << mod.name << "\n---\n" << mod.code << "\n---\n";
            
            tc::Source source;

            source.name =
                mod.filename;

            source.source =
                mod.code;

            c::implement_templates(source, types);
            

            tc::Opt<std::string> result =
                c::compile(source, types);

            if (!result) {
                std::cerr
                    << "compilation failed for "
                    << source.name
                    << '\n';

                return 1;
            }
            if (is_combrobe_compilation) throw 0;
            if constexpr(DEBUG) {
                std::cout << "=== MODULE ===\n";
                std::cout << "name: " << mod.name << '\n';
                std::cout << "file: " << mod.filename << '\n';
                std::cout << "--- API ---\n";
                std::cout << mod.api << '\n';
                std::cout << "--- CODE ---\n";
                std::cout << mod.code << '\n';
                std::cout << "--- C++  ---\n";
                std::cout << *result << '\n';
                std::cout << "==============\n";
            }
            std::filesystem::path name = std::filesystem::path{mod.filename}.stem();
            name += ".cpp";
            files.push_back(name);
            c::write_file(name.string(), *result); 
        }
        c::write_file("std.tmp.cpp", stdcpp); 
        c::write_file("std.tmp.hpp", stdhpp); 
        std::string cmd = "g++ std.tmp.cpp";
        for (auto file : files) {
            cmd += " " + file.string();
        }
        for (const auto &arg : cppargs) {
            cmd += " "s + arg;
        }
        if (generate_object_file)
            cmd += " -c";
        c::ProcessResult proc = c::shell(cmd);
        if (proc.exit_code != 0) {
            std::cerr << proc.stderr << '\n';
            throw 1;
        }
        
        for (auto file : files) {
            std::filesystem::remove(file);
        } 
        std::filesystem::remove("std.tmp.cpp");
        std::filesystem::remove("std.tmp.hpp");
    } catch (int exit) {
        return exit;
    }

    return 0;
}