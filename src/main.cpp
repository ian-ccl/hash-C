#include "config.h"
#include "types.hpp"
#include "args.hpp"
#include "fs.hpp"
#include "modules.hpp"
#include "transpile.hpp"
#include "compile.hpp"
#include "preprocesor.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
ArgsInfo info;
args_t args = {
    nullptr,
    0
};
program_t program{
    {},
    info,
    {},
    {}
};
void read_main_file()
{
    info =
        argparse(args);
    if constexpr (DEBUG) {
        std::cout
            << "main_file:    "
            << info.main_file
            << '\n';
        std::cout
            << "out_name:     "
            << info.out.name
            << '\n';
        std::cout
            << "out_type:     "
            << info.out.as
            << '\n';
        std::cout
            << "cpp compiler: "
            << info.ccpp
            << '\n';
    }
    std::string source =
        read_file(
            info.main_file
        );
    size_t after_api = 0;
    std::string api;
    if (
        source.find(
            "module api"
        )
        != std::string::npos
    ) {
        api =
            extract_api(
                source,
                after_api
            );
        validate_api(api);
    }
    else {
        after_api = 0;
    }
    if (
        api.find(
            "fn main() void;"
        )
        == std::string::npos
    ) {
        api +=
            "\nfn main() void;\n";
    }
    std::string implementation;
    if (
        source.find(
            "module api"
        )
        != std::string::npos
    ) {
        implementation =
            source.substr(
                after_api
            );
    }
    else {
        implementation =
            source;
    }
    program.Modules[
        (info.main_file).substr(0, info.main_file.size()-3)
    ] =
        Module{
            api,
            implementation,
            normalize_path(
                info.main_file
            )
        };
    std::vector<std::string> imports =
        get_imports(
            implementation
        );
    for (
        const std::string& module
        : imports
    ) {
        load_module(
            module,
            info.main_file
        );
    }
    program.Modules[
        (info.main_file).substr(0, info.main_file.size()-3)
    ].code =
        inject_apis(
            implementation,
            imports
        );
    if constexpr (DEBUG) {
        std::cout
            << "\n========== MODULES ==========\n";
        for (
            const auto& [name, module]
            : program.Modules
        ) {
            std::cout
                << "\n[" << name << "]\n";
            std::cout
                << "API:\n";
            std::cout
                << module.api;
            std::cout
                << "\nCODE:\n";
            std::cout
                << module.code;
        }
        std::cout
            << "\n=============================\n";
        std::cout
            << "\n========== MAIN INJECTED ==========\n";
        std::cout
            << program.Modules[
                (info.main_file).substr(0, info.main_file.size()-3)
            ].code;
        std::cout
            << "\n====================================\n";
    }
}
int main(
    int argc,
    char** argv
)
{
    args = {
        argv,
        argc
    };
    try {
        //read
        read_main_file();
        //preproc
        if constexpr (DEBUG) std::cout << "after preproces\n";
        for (auto& [name, mod] : program.Modules) {
            get_template_defines(mod.code);
            replace_template_implements(mod.code);
            if constexpr (DEBUG) {
                std::cout << name << '\n' << mod.code << '\n';
            }
        }
        //compile
        transpile();
        compile();
    }
    //errors
    catch (
        const std::exception& err
    ) {
        std::cerr
            << "\033[1;31m[ERROR] "
            << err.what()
            << " [ERROR]\033[0m\n";
        return 1;
    }
    catch (
        const std::string& msg
    ) {
        std::cerr
            << "\033[1;31m[ERROR] "
            << msg
            << " [ERROR]\033[0m\n";
        return 1;
    }
    catch (
        std::string_view msg
    ) {
        std::cerr
            << "\033[1;31m[ERROR] "
            << msg
            << " [ERROR]\033[0m\n";
        return 1;
    }
    return 0;
}