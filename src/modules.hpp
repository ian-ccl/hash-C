#ifndef MODULES_HPP
#define MODULES_HPP

#include "types.hpp"
#include "fs.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <cctype>
#include <unordered_set>
#include <iostream>

namespace c {

    // ============================================================
    // module api { ... }
    // ============================================================

    static ModuleSource extract_module_api(
        const std::string& code
    )
    {
        constexpr std::string_view marker =
            "module api";

        size_t pos =
            code.find(marker);

        // Un archivo también puede no tener API.
        if (pos == std::string::npos)
            return {"", code};

        pos += marker.size();

        while (
            pos < code.size() &&
            std::isspace(
                static_cast<unsigned char>(
                    code[pos]
                )
            )
        ) {
            ++pos;
        }

        if (
            pos >= code.size() ||
            code[pos] != '{'
        ) {
            std::cerr
                << "expected '{' after 'module api'\n";

            throw 1;
        }

        ++pos;

        size_t begin = pos;
        size_t depth = 1;

        for (; pos < code.size(); ++pos) {
            if (code[pos] == '{') {
                ++depth;
            }
            else if (code[pos] == '}') {
                --depth;

                if (depth == 0) {
                    return ModuleSource{
                        code.substr(
                            begin,
                            pos - begin
                        ),
                        code.substr(pos + 1)
                    };
                }
            }
        }

        std::cerr
            << "module api: '{' was not closed\n";

        throw 1;
    }

    // ============================================================
    // @import foo
    // ============================================================

    static std::string parse_import_name(
        const std::string& code,
        size_t& pos
    )
    {
        while (
            pos < code.size() &&
            (
                code[pos] == ' ' ||
                code[pos] == '\t'
            )
        ) {
            ++pos;
        }

        size_t begin = pos;

        while (pos < code.size()) {
            char c = code[pos];

            if (
                c == ' ' ||
                c == '\t' ||
                c == '\n' ||
                c == '\r' ||
                c == ';'
            ) {
                break;
            }

            ++pos;
        }

        if (begin == pos) {
            std::cerr
                << "expected module name after @import\n";

            throw 1;
        }

        return code.substr(
            begin,
            pos - begin
        );
    }

    static std::vector<std::string> get_imports(
        const std::string& code
    )
    {
        std::vector<std::string> imports;

        size_t pos = 0;

        while (
            (pos = code.find("@import", pos))
            != std::string::npos
        ) {
            pos += 7;

            imports.push_back(
                parse_import_name(
                    code,
                    pos
                )
            );
        }

        return imports;
    }

    

    // ============================================================
    // Elimina @import del código.
    // ============================================================

    static std::string remove_imports(
        const std::string& code
    )
    {
        std::string result;

        size_t begin = 0;
        size_t pos = 0;

        while (
            (pos = code.find("@import", begin))
            != std::string::npos
        ) {
            result += code.substr(
                begin,
                pos - begin
            );

            pos += 7;

            parse_import_name(
                code,
                pos
            );

            if (
                pos < code.size() &&
                code[pos] == ';'
            ) {
                ++pos;
            }

            begin = pos;
        }

        result += code.substr(begin);

        return result;
    }

    // ============================================================
    // Limpia la indentación de la API.
    // ============================================================

    static std::string clean_api(
        const std::string& api
    )
    {
        std::string result;

        size_t pos = 0;

        while (pos < api.size()) {
            size_t line_end =
                api.find('\n', pos);

            if (line_end == std::string::npos)
                line_end = api.size();

            size_t begin = pos;

            while (
                begin < line_end &&
                std::isspace(
                    static_cast<unsigned char>(
                        api[begin]
                    )
                )
            ) {
                ++begin;
            }

            result += api.substr(
                begin,
                line_end - begin
            );

            if (line_end < api.size())
                result += '\n';

            pos =
                line_end < api.size()
                    ? line_end + 1
                    : line_end;
        }

        return result;
    }

    // ============================================================
    // Busca un módulo ya cargado.
    // ============================================================

    static LoadedModule* find_loaded_module(
        std::vector<LoadedModule>& modules,
        const std::string& name
    )
    {
        for (
            LoadedModule& module
            : modules
        ) {
            if (module.name == name)
                return &module;
        }

        return nullptr;
    }

    // ============================================================
    // Carga recursivamente un módulo.
    // ============================================================

    static void collect_module(
        const std::string& module_name,
        const std::string& importer,
        std::vector<LoadedModule>& modules,
        std::unordered_set<std::string>& loading
    )
    {
        // Ya fue cargado durante esta resolución.
        if (
            find_loaded_module(
                modules,
                module_name
            ) != nullptr
        ) {
            return;
        }

        // Estamos intentando cargarlo nuevamente
        // mientras todavía está en la pila.
        if (
            loading.find(module_name) !=
            loading.end()
        ) {
            std::cerr
                << "circular module import involving '"
                << module_name
                << "'\n";

            throw 1;
        }

        loading.insert(
            module_name
        );

        std::string filename;

        try {
            filename =
                c::module_path(
                    module_name,
                    importer
                );

            std::string source =
                c::read_file(filename);

            ModuleSource source_parts =
                extract_module_api(
                    source
                );

            std::vector<std::string> imports =
                get_imports(
                    source_parts.implementation
                );

            std::string code =
                remove_imports(
                    source_parts.implementation
                );

            /*
             * Guardamos el módulo antes de recorrer
             * sus dependencias.
             */
            modules.push_back(
                LoadedModule{
                    module_name,
                    source_parts.api,
                    imports,
                    filename,
                    code
                }
            );

            for (
                const std::string& dependency
                : imports
            ) {
                collect_module(
                    dependency,
                    filename,
                    modules,
                    loading
                );
            }

            loading.erase(
                module_name
            );
        }
        catch (...) {
            loading.erase(
                module_name
            );

            throw;
        }
    }

    // ============================================================
    // Carga todos los módulos necesarios a partir de uno.
    // ============================================================

    static std::vector<LoadedModule> load_modules(
        const std::string& module_name,
        const std::string& importer = ""
    )
    {
        std::vector<LoadedModule> modules;

        std::unordered_set<std::string> loading;

        collect_module(
            module_name,
            importer,
            modules,
            loading
        );

        return modules;
    }

    // ============================================================
    // Procesa el archivo raíz.
    //
    // IMPORTANTE:
    // `filename` es el archivo real que se está compilando.
    // ============================================================

    static std::string inject_apis(
        const LoadedModule& module,
        const std::vector<LoadedModule>& modules
    )
    {
        std::string result;

        for (const std::string& imported : module.imports) {
            for (const LoadedModule& dependency : modules) {
                if (dependency.name == imported) {
                    result += clean_api(dependency.api);
                    result += '\n';
                    break;
                }
            }
        }

        result += module.code;

        return result;
    }

    static std::vector<LoadedModule> process_modules(
        const std::string& filename
    )
    {
        std::string source =
            c::read_file(filename);

        ModuleSource source_parts =
            extract_module_api(source);

        std::vector<std::string> imports =
            get_imports(
                source_parts.implementation
            );

        std::vector<LoadedModule> modules;

        std::unordered_set<std::string> loading;

        // El archivo raíz también es un LoadedModule.
        modules.push_back(
            LoadedModule{
                filename,
                source_parts.api,
                imports,
                filename,
                remove_imports(
                    source_parts.implementation
                )
            }
        );

        loading.insert(filename);

        for (
            const std::string& dependency
            : imports
        ) {
            collect_module(
                dependency,
                filename,
                modules,
                loading
            );
        }

        loading.erase(filename);

        // ------------------------------------------------------------
        // Ahora que todos los módulos están cargados,
        // inyectamos las APIs en cada .code.
        // ------------------------------------------------------------

        for (LoadedModule& module : modules) {
            module.code =
                inject_apis(
                    module,
                    modules
                );
        }

        return modules;
    }

}

#endif