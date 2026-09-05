#ifndef PREPOCESSOR_HPP
#define PREPOCESSOR_HPP

#include "config.h"
#include "externs.hpp"
#include "types.hpp"
#include "idents.hpp"
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

Templates templates;
std::vector<todo> todo_list;
std::unordered_set<std::string> implemented_templates;

/*
returns index on end "@templateend"
example
@template nulltemplate arg
//HI!
@templateend
//          ^
//          idx
*/
size_t template_define(std::string templatedefine) {
    bool skip = templatedefine.starts_with("@template");
    if (skip) {
        templatedefine = templatedefine.substr(9);
    }
    size_t idx = 0;
    while (idx < templatedefine.size() && std::isspace(static_cast<unsigned char>(templatedefine[idx])))
        ++idx;
    TemplateInfo _template;
    _template.name = read_qualified_identifier(templatedefine, idx); 
    std::vector<std::string> args;
    
    while (idx < templatedefine.size() && templatedefine[idx] != '\n') {
        while (idx < templatedefine.size() && isspace(static_cast<unsigned char>(templatedefine[idx])) && templatedefine[idx] != '\n')
            idx++;

        if (idx >= templatedefine.size() || templatedefine[idx] == '\n')
            break;

        if (templatedefine[idx] == '<') {
            std::string name = "<";
            idx++;

            while (idx < templatedefine.size() &&
                templatedefine[idx] != '>' &&
                !isspace(static_cast<unsigned char>(templatedefine[idx]))) {
                name += templatedefine[idx++];
            }

            if (idx >= templatedefine.size() || templatedefine[idx] != '>')
                throw std::string("expected '>' for template property");

            name += '>';
            idx++;

            if (name == "<strsafe>" || name == "<typeonly>") {
                args.push_back(name);
                continue;
            }

            throw std::string("unknown template property: ") + name;
        }

        args.push_back(read_qualified_identifier(templatedefine, idx));
    }
    size_t n = args.size();
    for (size_t i = 0; i < n; i++) {
        if (args[i] == "<strsafe>") {
            _template.is_strsafe = true;
            args.erase(args.begin() + i);
            i--;
            n--;
        } else if (args[i] == "<typeonly>") {
            _template.is_type_only = true;
            args.erase(args.begin() + i);
            i--;
            n--;
        }
    }
    _template.args = args.size();
    while(idx < templatedefine.size() && (isspace(static_cast<unsigned char>(templatedefine[idx])))) idx++;
    size_t end = templatedefine.find("@templateend", idx);
    if (end == std::string::npos)
        throw std::string("expected @templateend for template: ") + _template.name;
    _template.code =
        templatedefine.substr(
            idx,
            end - idx
        );
    size_t index = 0;
    
    for (const std::string &argument : args) {
        std::string arg = "$" + argument;
        
        for (size_t pos = 0; (pos = _template.code.find(arg, pos)) != std::string::npos;) {
            std::string replacement = "$" + std::to_string(index);
            _template.code.replace(pos, arg.size(), replacement);
            pos += replacement.size();
        }
        index++;
    }

    templates[_template.name] = _template;
    return end + 12 + (skip ? 9 : 0);
}

void get_template_defines(std::string &code) {
    size_t i = 0;

    while ((i = code.find("@template", i)) != std::string::npos) {
        size_t length = template_define(code.substr(i));

        code.erase(i, length);
    }
}

size_t find_out_str(const std::string& s, const std::string& pattern, size_t start = 0) {
    bool in_string = false;
    bool in_char = false;
    bool escaped = false;

    for (size_t i = start; i < s.size(); ++i) {
        char c = s[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if ((in_string || in_char) && c == '\\') {
            escaped = true;
            continue;
        }

        if (in_string) {
            if (c == '"')
                in_string = false;

            continue;
        }

        if (in_char) {
            if (c == '\'')
                in_char = false;

            continue;
        }

        if (c == '"') {
            in_string = true;
            continue;
        }

        if (c == '\'') {
            in_char = true;
            continue;
        }

        if (s.compare(i, pattern.size(), pattern) == 0)
            return i;
    }

    return std::string::npos;
}

void replace_1_implement(size_t argno, std::string &code, const std::string &arg,  bool is_unique, TemplateInfo &templ) {


    if (templ.is_type_only && !types.contains(arg)) {
        auto todo_fn = [](const std::vector<std::string> &args) {
            if (args.size() != 2) return false;
            return types.contains(args[0]);
        };

        todo_list.push_back({{arg, templ.name}, todo_fn, todo_id::TYPE_ID});
    }
    std::string marker = "$" + std::to_string(argno);

    size_t p = 0;

    std::function<size_t(const std::string&, const std::string&, size_t)> find_marker = [](const std::string& s, const std::string& pattern, size_t start = 0) {
        return s.find(pattern, start);
    };
    
    if(templ.is_strsafe) 
        find_marker = find_out_str;

    while ((p = find_marker(code, marker, p)) != std::string::npos) {
        code.replace(p, marker.size(), arg);

        p += arg.size();
    }
}

void replace_template_implements(std::string &code) {
    size_t i = 0;

    while ((i = code.find("@implement", i)) != std::string::npos) { 
        bool is_unique = false;

        size_t begin = i;

        // Saltar "@implement"
        size_t pos = i + 10;

        if (code.compare(pos, 7, "_unique") == 0) {
            is_unique = true;
            //saltar "_unique"
            pos += 7;
        }

        // Saltar espacios
        while (
            pos < code.size() &&
            std::isspace(static_cast<unsigned char>(code[pos])) &&
            code[pos] != '\n'
        ) {
            ++pos;
        }

        // Leer nombre del template
        size_t name_begin = pos;

        while (
            pos < code.size() &&
            !std::isspace(static_cast<unsigned char>(code[pos]))
        ) {
            ++pos;
        }

        if (name_begin == pos)
            throw std::string("expected template name after @implement");

        std::string name =
            code.substr(
                name_begin,
                pos - name_begin
            );

        auto it = templates.find(name);

        if (it == templates.end()) {
            throw std::string(
                "template not defined: "
            ) + name;
        }

        TemplateInfo &templ = it->second;

        std::vector<std::string> args;

        // Leer argumentos hasta '\n'
        while (true) {
            while (
                pos < code.size() &&
                code[pos] != '\n' &&
                std::isspace(
                    static_cast<unsigned char>(code[pos])
                )
            ) {
                ++pos;
            }

            if (
                pos >= code.size() ||
                code[pos] == '\n'
            ) {
                break;
            }

            size_t arg_begin = pos;

            while (
                pos < code.size() &&
                !std::isspace(
                    static_cast<unsigned char>(code[pos])
                )
            ) {
                ++pos;
            }

            args.push_back(
                code.substr(
                    arg_begin,
                    pos - arg_begin
                )
            );
        }

        if (args.size() != templ.args) {
            throw std::string(
                "template '" +
                name +
                "' expects " +
                std::to_string(templ.args) +
                " arguments, got " +
                std::to_string(args.size())
            );
        }
        std::string id = name + '|';

        for (const std::string& arg : args) {
            id += arg + '|';
        }

        if (implemented_templates.contains(id) && is_unique) {
            size_t end = pos;

            if (end < code.size() && code[end] == '\n')
                ++end;

            code.erase(begin, end - begin);

            i = begin;
            continue;
        }
        implemented_templates.insert(id);

        std::string expanded = templ.code;

        // Sustituir $0, $1, $2...
        for (size_t n = 0; n < args.size(); ++n) {
            replace_1_implement(n, expanded, args[n], is_unique, templ);
        }

        // Quitar hasta el final de la línea
        size_t end = pos;

        if (
            end < code.size() &&
            code[end] == '\n'
        ) {
            ++end;
        }

        code.replace(
            begin,
            end - begin,
            expanded
        );

        // Seguir después del código generado
        i = begin + expanded.size();
    }
}

#endif