#ifndef IDENTS_HPP
#define IDENTS_HPP
#include <string>
#include <cctype>
#include <iostream>
static bool is_ident_start(
    char c
)
{
    return std::isalpha(
        static_cast<unsigned char>(c)
    ) || c == '_';
}
static bool is_ident_char(
    char c
)
{
    return std::isalnum(
        static_cast<unsigned char>(c)
    ) || c == '_';
}
static std::string read_qualified_identifier(
    const std::string& code,
    size_t& i
)
{
    if (
        i >= code.size() ||
        !is_ident_start(code[i])
    ) {
        throw std::string(
            "expected identifier " 
        ) + (i >= code.size() ? "invalid position" : (std::string("got: ") + code[i]));
    }
    std::string result;
    size_t begin = i;
    ++i;
    while (
        i < code.size() &&
        is_ident_char(code[i])
    ) {
        ++i;
    }
    result +=
        code.substr(
            begin,
            i - begin
        );
    while (true) {
        size_t colon = i;
        while (
            i < code.size() &&
            std::isspace(
                static_cast<unsigned char>(
                    code[i]
                )
            )
        ) {
            ++i;
        }
        if (
            i >= code.size() ||
            code[i] != ':'
        ) {
            i = colon;
            break;
        }
        ++i;
        while (
            i < code.size() &&
            std::isspace(
                static_cast<unsigned char>(
                    code[i]
                )
            )
        ) {
            ++i;
        }
        if (
            i >= code.size() ||
            !is_ident_start(code[i])
        ) {
            throw std::string(
                "expected identifier after ':'"
            );
        }
        begin = i;
        ++i;
        while (
            i < code.size() &&
            is_ident_char(code[i])
        ) {
            ++i;
        }
        result += ':';
        result +=
            code.substr(
                begin,
                i - begin
            );
    }
    return result;
}
#endif