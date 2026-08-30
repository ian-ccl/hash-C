#ifndef PREPOCESSOR_HPP
#define PREPOCESSOR_HPP

#include "config.h"
#include "externs.hpp"
#include "types.hpp"
#include "idents.hpp"
#include <cstring>
#include <string>
#include <unordered_map>

Templates templates;
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
        templatedefine = templatedefine.substr(10);
    }
    size_t idx = 0;
    TemplateInfo _template;
    _template.name = read_qualified_identifier(templatedefine, idx);

    while (idx < templatedefine.size() && templatedefine[idx] != '\n') {
        while (idx < templatedefine.size() && isspace(static_cast<unsigned char>(templatedefine[idx])))
            idx++;

        if (idx >= templatedefine.size() || templatedefine[idx] == '\n')
            break;

        _template.args.push_back(read_qualified_identifier(templatedefine, idx));
    }
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
    
    for (const std::string &argument : _template.args) {
        std::string arg = "$" + argument;
        
        for (size_t pos = 0;_template.code.find(arg,pos) != std::string::npos; pos = _template.code.find(arg,pos)) {
            _template.code.replace(_template.code.find(arg), arg.size(), "$"+std::to_string(index));
            pos+=arg.size();
        }
        index++;
    }

    templates[_template.name] = _template;
    return end + 12 + (skip ? 10 : 0);
}

void get_template_defines(std::string &code) {
    size_t i = 0;

    while ((i = code.find("@template", i)) != std::string::npos) {
        size_t length = template_define(code.substr(i));

        code.erase(i, length);
    }
}

void replace_template_implements(std::string &code) {
    size_t i = 0;

    while ((i = code.find("@implement", i)) != std::string::npos) {
        size_t begin = i;

        // Saltar "@implement"
        size_t pos = i + 10;

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

        if (args.size() != templ.args.size()) {
            throw std::string(
                "template '" +
                name +
                "' expects " +
                std::to_string(templ.args.size()) +
                " arguments, got " +
                std::to_string(args.size())
            );
        }

        std::string expanded = templ.code;

        // Sustituir $0, $1, $2...
        for (size_t n = 0; n < args.size(); ++n) {
            std::string marker =
                "$" + std::to_string(n);

            size_t p = 0;

            while (
                (p = expanded.find(marker, p))
                != std::string::npos
            ) {
                expanded.replace(
                    p,
                    marker.size(),
                    args[n]
                );

                p += args[n].size();
            }
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