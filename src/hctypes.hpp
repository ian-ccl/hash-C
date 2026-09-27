#ifndef HCTYPES_HPP
#define HCTYPES_HPP

#include "../libs/transcode/transcode.hpp"
namespace c {
    inline void get_types(
        const tc::Source& source,
        tc::Set<tc::String>& types
    ) {
        size_t idx = 0;

        while (idx < source.source.size()) {
            // Saltar espacios
            while (idx < source.source.size() &&
                std::isspace(
                    static_cast<unsigned char>(source.source[idx])
                )) {
                ++idx;
            }

            if (idx >= source.source.size())
                break;

            // Leer identificador
            if (!(std::isalpha(
                    static_cast<unsigned char>(source.source[idx])
                ) ||
                source.source[idx] == '_')) {
                ++idx;
                continue;
            }

            size_t word_begin = idx;

            while (idx < source.source.size() &&
                (std::isalnum(
                    static_cast<unsigned char>(source.source[idx])
                ) ||
                source.source[idx] == '_')) {
                ++idx;
            }

            std::string_view word(
                source.source.data() + word_begin,
                idx - word_begin
            );

            // typedef Nuevo Viejo;
            if (word == "typedef") {
                // Nuevo
                while (idx < source.source.size() &&
                    std::isspace(
                        static_cast<unsigned char>(source.source[idx])
                    )) {
                    ++idx;
                }

                if (idx >= source.source.size())
                    break;

                if (!(std::isalpha(
                        static_cast<unsigned char>(source.source[idx])
                    ) ||
                    source.source[idx] == '_')) {
                    continue;
                }

                size_t type_begin = idx;

                while (idx < source.source.size() &&
                    (std::isalnum(
                        static_cast<unsigned char>(source.source[idx])
                    ) ||
                    source.source[idx] == '_')) {
                    ++idx;
                }

                types.insert(
                    tc::String{
                        source.source.substr(
                            type_begin,
                            idx - type_begin
                        )
                    }
                );

                continue;
            }

            // enum Foo
            // struct Foo
            // cstruct Foo
            if (word == "enum" ||
                word == "struct" ||
                word == "cstruct") {

                while (idx < source.source.size() &&
                    std::isspace(
                        static_cast<unsigned char>(source.source[idx])
                    )) {
                    ++idx;
                }

                if (idx >= source.source.size())
                    break;

                if (!(std::isalpha(
                        static_cast<unsigned char>(source.source[idx])
                    ) ||
                    source.source[idx] == '_')) {
                    continue;
                }

                size_t type_begin = idx;

                while (idx < source.source.size() &&
                    (std::isalnum(
                        static_cast<unsigned char>(source.source[idx])
                    ) ||
                    source.source[idx] == '_')) {
                    ++idx;
                }

                types.insert(
                    tc::String{
                        source.source.substr(
                            type_begin,
                            idx - type_begin
                        )
                    }
                );
            }
        }
    }
}
#endif