#ifndef TEMPLATES_HPP
#define TEMPLATES_HPP

#include "types.hpp"
#include "../libs/transcode/transcode.hpp"
#include <iostream>
namespace c {
    Templates& templates() {
        static Templates templates;
        return templates;
    }
    
    inline void get_templates(const tc::Source source) {
        
        size_t idx = 0;
        tc::SourcePosition pos = {{1,1}, tc::MakeRef(source)};
        while (true) {
            pos = source.find("@template ", idx, tc::MatchOptions::StrSafe);
            if (!pos.this_validate()) {
                return;
            }

            idx = *pos.this_to_idx() + 10;

            // Nombre de la template
            size_t name_begin = idx;

            while (idx < source.source.size() &&
                (std::isalnum(static_cast<unsigned char>(source.source[idx])) ||
                    source.source[idx] == '_')) {
                ++idx;
            }

            std::string name =
                source.source.substr(name_begin, idx - name_begin);

            // Buscar ':'
            while (idx < source.source.size() && source.source[idx] != '\n' &&
                std::isspace(static_cast<unsigned char>(source.source[idx]))) {
                ++idx;
            }

            if (idx >= source.source.size() || source.source[idx] != ':') {
                std::cerr << "can't define a template without args\nerror in file " << source.name << "\n";
                throw 1;
            }

            ++idx;

            // Argumentos
            std::vector<std::string> args;

            while (idx < source.source.size()) {
                while (idx < source.source.size() &&  source.source[idx] != '\n' &&
                    std::isspace(static_cast<unsigned char>(source.source[idx]))) {
                    ++idx;
                }

                if (source.source[idx] == '\n') break;

                if (idx >= source.source.size()) {
                    std::cerr << "non terminated template header\nerror in file " << source.name << "\n";
                    throw 1;
                }

                // Comienzan los modificadores
                if (source.source[idx] == '<') {
                    break;
                }

                size_t arg_begin = idx;

                while (idx < source.source.size() &&
                    (std::isalnum(static_cast<unsigned char>(source.source[idx])) ||
                        source.source[idx] == '_')) {
                    ++idx;
                }

                if (arg_begin == idx) {
                    // argumento inválido
                    std::cerr << "invalid argument name\nerror in file "<< source.name << "\n";
                    throw 1;
                }

                args.push_back(
                    source.source.substr(arg_begin, idx - arg_begin)
                );
            }

            // Flags de la template
            bool typeonly = false;
            bool strsafe = false;

            while (idx < source.source.size()) {
                while (idx < source.source.size() &&
                    std::isspace(static_cast<unsigned char>(source.source[idx]))) {
                    ++idx;
                }

                if (idx >= source.source.size() || source.source[idx] != '<') {
                    break;
                }

                ++idx;

                size_t modifier_begin = idx;

                while (idx < source.source.size() &&
                    (std::isalnum(static_cast<unsigned char>(source.source[idx])) ||
                        source.source[idx] == '_')) {
                    ++idx;
                }

                std::string modifier =
                    source.source.substr(modifier_begin, idx - modifier_begin);

                if (idx >= source.source.size() || source.source[idx] != '>') {
                    // modificador sin cerrar
                    break;
                }

                ++idx;

                if (modifier == "typeonly") {
                    typeonly = true;
                }
                else if (modifier == "strsafe") {
                    strsafe = true;
                }
                else {
                    // modificador desconocido
                    break;
                }
            }

            // Buscar @templateend
            tc::SourcePosition end =
                {source.find("@templateend", idx, tc::MatchOptions::StrSafe), source};

            if (!end.this_validate()) {
                std::cerr << "non closed template\nerror in file " << source.name << "\n";
                throw 1;
            }

            size_t body_begin = idx;
            size_t body_end = *end.this_to_idx();

            tc::Source body =
                {"",source.source.substr(
                    body_begin,
                    body_end - body_begin
                )};
            for (size_t i = 0; i < args.size(); ++i) {
                std::string_view arg = args[i];

                std::string replacement =
                    "$" + std::to_string(i);

                size_t search = 0;

                while (true) {
                    tc::Opt<size_t> found = body.find(
                        "$" + std::string(arg),
                        search,
                        (strsafe ? tc::MatchOptions::StrSafe : tc::MatchOptions::None) | tc::MatchOptions::WholeWord
                    ).to_idx(body);

                    if (!found) {
                        break;
                    }

                    body.source.replace(
                        *found,
                        arg.size() + 1,
                        replacement
                    );

                    search = *found + replacement.size();
                }
            }

            // Registrar template
            TemplateInfo templ;

            templ.name = std::move(name);
            templ.args = args.size();
            templ.is_type_only = typeonly;
            templ.is_strsafe = strsafe;
            templ.code = std::move(body.source);

            templates().insert(
                templ
            );

            // Continuar después de @templateend
            idx = body_end + 12;
        }
    }
    inline void remove_templates(tc::Source& source) {
    size_t idx = 0;
    size_t removed = 0;

    while (true) {
        tc::SourcePosition begin_pos =
            {
                source.find(
                    "@template ",
                    idx,
                    tc::MatchOptions::StrSafe
                ),
                source
            };

        if (!begin_pos.this_validate())
            break;

        size_t begin = *begin_pos.this_to_idx();

        tc::SourcePosition end_pos =
            {
                source.find(
                    "@templateend",
                    begin + 10,
                    tc::MatchOptions::StrSafe
                ),
                source
            };

        if (!end_pos.this_validate()) {
            std::cerr
                << "non closed template\n"
                << "error in file "
                << source.name
                << "\n";

            throw 1;
        }

        size_t end = *end_pos.this_to_idx() + 12;

        source.source.erase(
            begin,
            end - begin
        );

        ++removed;

        idx = begin;
    }
    if constexpr (DEBUG)
        std::cerr
            << "removed "
            << removed
            << " template(s) from "
            << source.name
            << "\n";
}
    inline void implement_templates(tc::Source& source, tc::Set<tc::String>& types) {
        size_t idx = 0;

        while (true) {
            tc::SourcePosition pos =
                {source.find(
                    "@implement ",
                    idx,
                    tc::MatchOptions::StrSafe
                ), source};

            if (!pos.this_validate())
                break;

            size_t begin = *pos.this_to_idx();
            idx = begin + 11;

            // Nombre de la template
            size_t name_begin = idx;

            while (idx < source.source.size() &&
                (std::isalnum(
                        static_cast<unsigned char>(source.source[idx])
                    ) ||
                    source.source[idx] == '_')) {
                ++idx;
            }

            std::string name =
                source.source.substr(
                    name_begin,
                    idx - name_begin
                );

            auto templ = templates().find(tc::StrView{name});

            if (templ == templates().end()) {
                std::cerr << "unknown template: " << name << '\n' << source.name << "\n";
                throw 1;
            }

            // Argumentos
            std::vector<std::string> args;

            while (idx < source.source.size()) {
                while (idx < source.source.size() &&
                    source.source[idx] != '\n' &&
                    std::isspace(
                        static_cast<unsigned char>(source.source[idx])
                    )) {
                    ++idx;
                }

                if (idx >= source.source.size() ||
                    source.source[idx] == '\n') {
                    break;
                }

                size_t arg_begin = idx;

                while (idx < source.source.size() &&
                    !std::isspace(
                        static_cast<unsigned char>(source.source[idx])
                    )) {
                    ++idx;
                }

                args.push_back(
                    source.source.substr(
                        arg_begin,
                        idx - arg_begin
                    )
                );
            }

            if (args.size() != templ->args) {
                std::cerr << "invalid number of template arguments\nerror in file " << source.name << "\n";
                std::cerr << "expected " << templ->args << " got " << args.size() << '\n';
                std::cout << '[';
                for (auto &arg : args) {
                    std::cout << arg;
                    std::cout << ", ";
                }
                std::cout << ']' << '\n';
                throw 1;
            }

            if (templ->is_type_only) {
                todo to_do;
                to_do.args.push_back(templ->name);
                for(const std::string& s : args) {
                    to_do.args.push_back(s);
                }
                to_do.func = [&](const std::vector<std::string>& args) {
                    for (size_t i = 1; i < args.size(); i++) {
                        if (! types.contains(args[i])) 
                            return false;
                    }
                    return true;
                };
            }

            // Expandir template
            std::string result = templ->code;

            for (size_t i = 0; i < args.size(); ++i) {
                std::string target =
                    "$" + std::to_string(i);

                tc::Source body = {"", result};

                size_t search = 0;

                tc::MatchOptions options =
                    tc::MatchOptions::WholeWord;

                if (templ->is_strsafe)
                    options |= tc::MatchOptions::StrSafe;

                while (true) {
                    tc::Opt<size_t> found =
                        body.find(
                            target,
                            search,
                            options
                        ).to_idx(body);

                    if (!found)
                        break;

                    body.source.replace(
                        *found,
                        target.size(),
                        args[i]
                    );

                    search =
                        *found + args[i].size();
                }

                result = std::move(body.source);
            }

            // Reemplazar @implement completo
            size_t end = idx;

            source.source.replace(
                begin,
                end - begin,
                result
            );

            idx = begin + result.size();
        }
    }
}
#endif