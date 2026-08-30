#ifndef MODULES_HPP
#define MODULES_HPP
#include "types.hpp"
#include "fs.hpp"
#include "externs.hpp"
#include <string>
#include <vector>
static std::string extract_api(
    const std::string& code,
    size_t& code_after_api
)
{
    constexpr std::string_view marker =
        "module api";
    size_t pos =
        code.find(marker);
    if (pos == std::string::npos) {
        throw std::string(
            "module api { ... } not found"
        );
    }
    pos += marker.size();
    while (
        pos < code.size() &&
        (
            code[pos] == ' ' ||
            code[pos] == '\t' ||
            code[pos] == '\n' ||
            code[pos] == '\r'
        )
    ) {
        ++pos;
    }
    if (
        pos >= code.size() ||
        code[pos] != '{'
    ) {
        throw std::string(
            "expected '{' after 'module api'"
        );
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
                code_after_api =
                    pos + 1;
                return code.substr(
                    begin,
                    pos - begin
                );
            }
        }
    }
    throw std::string(
        "module api: '{' was not closed"
    );
}
static void validate_api(
    const std::string& api
)
{
    int braces = 0;
    for (char c : api) {
        if (c == '{')
            ++braces;
        else if (c == '}')
            --braces;
        if (braces < 0) {
            throw std::string(
                "invalid module api: "
                "unexpected '}'"
            );
        }
    }
    if (braces != 0) {
        throw std::string(
            "invalid module api: "
            "unbalanced braces"
        );
    }
}
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
        throw std::string(
            "expected module name after @import"
        );
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
        (pos = code.find(
            "@import",
            pos
        ))
        != std::string::npos
    ) {
        pos += 7;
        std::string name =
            parse_import_name(
                code,
                pos
            );
        imports.push_back(
            name
        );
    }
    return imports;
}
static void load_module(
    const std::string& module_name,
    const std::string& importer
)
{
    if (
        program.Modules.contains(
            module_name
        )
    ) {
        return;
    }
    program.loading.insert(
        module_name
    );
    try {
        std::string filename =
            module_path(
                module_name,
                importer
            );
        std::string source =
            read_file(filename);
        size_t after_api = 0;
        std::string api =
            extract_api(
                source,
                after_api
            );
        validate_api(api);
        std::string implementation =
            source.substr(
                after_api
            );
        program.Modules.emplace(
            module_name,
            Module{
                api,
                implementation,
                filename
            }
        );
        std::vector<std::string> imports =
            get_imports(
                implementation
            );
        for (
            const std::string& dependency
            : imports
        ) {
            load_module(
                dependency,
                filename
            );
        }
        program.loading.erase(
            module_name
        );
    }
    catch (...) {
        program.loading.erase(
            module_name
        );
        throw;
    }
}
static std::string inject_apis(
    const std::string& code,
    const std::vector<std::string>& imports
)
{
    std::string result;
    for (
        const std::string& module_name
        : imports
    ) {
        auto it =
            program.Modules.find(
                module_name
            );
        if (
            it == program.Modules.end()
        ) {
            throw std::string(
                "internal error: module '"
            ) + module_name +
            "' was not loaded";
        }
        const std::string& api =
            it->second.api;
        size_t pos = 0;
        while (pos < api.size()) {
            size_t line_end =
                api.find('\n', pos);
            if (line_end == std::string::npos)
                line_end = api.size();
            size_t poscopy = pos;
            for (; pos < line_end - poscopy && isspace(api[pos]); pos++);
            std::string line =
                api.substr(
                    pos,
                    line_end - poscopy
                );
            result += line;
            if (line_end < api.size())
                result += '\n';
            pos =
                line_end < api.size()
                    ? line_end + 1
                    : line_end;
        }
        result += '\n';
    }
    result += code;
    return result;
}
#endif