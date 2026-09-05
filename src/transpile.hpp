#ifndef TRANSPILE_HPP
#define TRANSPILE_HPP
#include "externs.hpp"
#include "types.hpp"
#include "idents.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
inline std::unordered_set<std::string> keywords = {
    "fn",
    "struct",
    "typedef",
    "for",
    "while",
    "if",
    "else",
    "elif",
    "switch",
    "continue",
    "break",
    "do",
    "enum",
    "extern",
    "case",
    "default",
    "nullptr",
    "nullslice",
    "cstruct",
    "defer",
    "const",
    "return"
};
inline std::unordered_set<std::string> types = {
    "int",
    "uint",
    "float",
    "bool",
    "char",
    "ubyte",
    "ibyte",
    "fmax",
    "imax",
    "umax",
    "void",
    "_AutoType"
};
static std::unique_ptr<tree::node> clone_node(
    const tree::node* source,
    tree::node* parent = nullptr
)
{
    if (!source)
        return nullptr;

    auto result =
        std::make_unique<tree::node>(
            source->value,
            parent
        );

    result->kind =
        source->kind;

    for (const auto& child : source->children) {
        result->children.push_back(
            clone_node(
                child.get(),
                result.get()
            )
        );
    }

    return result;
}
static tree clone_tree(
    const tree& source
)
{
    tree result;
    if (!source.root())
        return result;
    result.emplace_root(
        source.root()->value
    );
    for (const auto& child : source.root()->children) {
        result.root()->children.push_back(
            clone_node(
                child.get(),
                result.root()
            )
        );
    }
    return result;
}
static std::string mangle(
    const std::string& name
)
{
    if (name.find(':') == std::string::npos)
        return name;
    std::string result = "_I";
    size_t begin = 0;
    while (begin < name.size()) {
        size_t end =
            name.find(
                ':',
                begin
            );
        if (end == begin) {
            throw std::string(
                "invalid qualified name: "
            ) + name;
        }
        if (end == std::string::npos)
            end = name.size();
        std::string part =
            name.substr(
                begin,
                end - begin
            );
        result +=
            std::to_string(part.size());
        result +=
            part;
        if (end == name.size())
            break;
        begin =
            end + 1;
        if (begin == name.size()) {
            throw std::string(
                "invalid qualified name: "
            ) + name;
        }
    }
    return result;
}
static std::string mangle_name(
    const std::string& name
)
{
    return mangle(name);
}

static std::vector<Token> lex(
    const Module& module
)
{
    const std::string& code =
        module.code;
    std::vector<Token> result;
    size_t i = 0;
    while (i < code.size()) {
        char c = code[i];
        if (
            i + 1 < code.size() &&
            c == '/' &&
            code[i + 1] == '/'
        ) {
            i += 2;
            while (
                i < code.size() &&
                code[i] != '\n'
            ) {
                ++i;
            }
            continue;
        }
        if (
            i + 1 < code.size() &&
            c == '/' &&
            code[i + 1] == '*'
        ) {
            i += 2;
            bool closed = false;
            while (i + 1 < code.size()) {
                if (
                    code[i] == '*' &&
                    code[i + 1] == '/'
                ) {
                    i += 2;
                    closed = true;
                    break;
                }
                ++i;
            }
            if (!closed) {
                throw std::string(
                    "unterminated block comment"
                );
            }
            continue;
        }
        if (
            std::isspace(
                static_cast<unsigned char>(c)
            )
        ) {
            ++i;
            continue;
        }
        if (c == '"') {
            ++i;
            std::string value;
            bool closed = false;
            while (i < code.size()) {
                char current =
                    code[i];
                if (current == '"') {
                    ++i;
                    closed = true;
                    break;
                }
                if (current == '\\') {
                    ++i;
                    if (i >= code.size()) {
                        throw std::string(
                            "unterminated string"
                        );
                    }
                    char escaped =
                        code[i];
                    switch (escaped) {
                        case 'n':
                            value += '\n';
                            break;
                        case 'r':
                            value += '\r';
                            break;
                        case 't':
                            value += '\t';
                            break;
                        case '\\':
                            value += '\\';
                            break;
                        case '"':
                            value += '"';
                            break;
                        case '0':
                            value += '\0';
                            break;
                        default:
                            value += escaped;
                            break;
                    }
                    ++i;
                    continue;
                }
                value += current;
                ++i;
            }
            
            if (!closed) {
                throw std::string(
                    "unterminated string"
                );
            }
            result.emplace_back(
                "string",
                std::move(value)
            );
            continue;
        }
        if (is_ident_start(c)) {
            std::string value =
                read_qualified_identifier(
                    code,
                    i
                );
            if (
                value.find(':') == std::string::npos &&
                keywords.contains(value)
            ) {
                result.emplace_back(
                    "keyword",
                    value
                );
                if (
                    value == "struct" ||
                    value == "cstruct" ||
                    value == "typedef"
                ) {
                    size_t j = i;
                    while (
                        j < code.size() &&
                        std::isspace(
                            static_cast<unsigned char>(
                                code[j]
                            )
                        )
                    ) {
                        ++j;
                    }
                    if (
                        j >= code.size() ||
                        !is_ident_start(code[j])
                    ) {
                        throw std::string(
                            "invalid identifier after "
                        ) + value;
                    }
                    std::string type_name =
                        read_qualified_identifier(
                            code,
                            j
                        );
                    types.insert(
                        type_name
                    );
                }
                continue;
            }
            if (types.contains(value)) {
                while (i < code.size()) {
                    if (code[i] == '*') {
                        value += '*';
                        ++i;
                        continue;
                    }
                    if (code[i] == '[') {
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
                            i < code.size() &&
                            code[i] == ']'
                        ) {
                            value += "[]";
                            ++i;
                            continue;
                        }
                        size_t number_begin =
                            i;
                        while (
                            i < code.size() &&
                            std::isdigit(
                                static_cast<unsigned char>(
                                    code[i]
                                )
                            )
                        ) {
                            ++i;
                        }
                        if (
                            number_begin == i
                        ) {
                            throw std::string(
                                "cannot exist array with "
                                "non number length"
                            );
                        }
                        size_t length = 0;
                        try {
                            length =
                                std::stoull(
                                    code.substr(
                                        number_begin,
                                        i - number_begin
                                    )
                                );
                        }
                        catch (...) {
                            throw std::string(
                                "invalid array length"
                            );
                        }
                        if (length == 0) {
                            throw std::string(
                                "cannot exist array with "
                                "0 elements"
                            );
                        }
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
                            code[i] != ']'
                        ) {
                            throw std::string(
                                "expected ']' after "
                                "array length"
                            );
                        }
                        ++i;
                        value +=
                            "[" +
                            std::to_string(length) +
                            "]";
                        continue;
                    }
                    break;
                }
                result.emplace_back(
                    "type",
                    std::move(value)
                );
                continue;
            }
            result.emplace_back(
                "identifier",
                std::move(value)
            );
            continue;
        }
        if (c == '@') {
            if (code.compare(i, 4, "@c++") == 0) {
                i += 4;

                size_t begin = i;
                size_t end = code.find("@c++end", i);

                if (end == std::string::npos) {
                    throw std::string(
                        "unterminated @c++ block"
                    );
                }

                std::string value =
                    code.substr(
                        begin,
                        end - begin
                    );

                result.emplace_back(
                    "c++",
                    std::move(value)
                );

                i = end + 7;
                continue;
            } else if (code.compare(i, 3, "@os") == 0) {
                i+= 3;
                while (
                    i < code.size() &&
                    std::isspace(
                        static_cast<unsigned char>(code[i])
                    ) && code[i] != '\n'
                ) {
                    ++i;
                }
                std::string os = read_qualified_identifier(
                    code,
                    i
                );
                if (os != "windows" && os != "linux" && os != "other" && os != "apple") {
                    throw std::string(
                        "invalid OS name after @os: "
                    ) + os;
                }
                result.emplace_back(
                    "@os",
                    os
                );
                continue;
            } else if (code.compare(i, 6, "@osend") == 0) {
                i+= 6;
                
                result.emplace_back(
                    "@os",
                    "end"
                );
                continue;
            } else if (code.compare(i, 6, "@os_else") == 0) {
                i+= 6;
                
                result.emplace_back(
                    "@os",
                    "else"
                );
                continue;
            }

            while (
                i < code.size() &&
                code[i] != '\n'
            ) {
                ++i;
            }

            continue;
        }
        if (std::isdigit(
                static_cast<unsigned char>(c)
            )
        ) {
            size_t begin = i;

            if (
                c == '0' &&
                i + 1 < code.size() &&
                (
                    code[i + 1] == 'x' ||
                    code[i + 1] == 'X' ||
                    code[i + 1] == 'b' ||
                    code[i + 1] == 'B' ||
                    code[i + 1] == 'o' ||
                    code[i + 1] == 'O'
                )
            ) {
                char base_char = code[i + 1];

                int base;
                if (base_char == 'x' || base_char == 'X')
                    base = 16;
                else if (base_char == 'b' || base_char == 'B')
                    base = 2;
                else
                    base = 8;

                i += 2;

                size_t digits_begin = i;

                while (i < code.size()) {
                    char digit = code[i];
                    bool valid = false;

                    if (base == 16) {
                        valid =
                            std::isdigit(
                                static_cast<unsigned char>(digit)
                            ) ||
                            (digit >= 'a' && digit <= 'f') ||
                            (digit >= 'A' && digit <= 'F');
                    }
                    else if (base == 8) {
                        valid =
                            digit >= '0' &&
                            digit <= '7';
                    }
                    else {
                        valid =
                            digit == '0' ||
                            digit == '1';
                    }

                    if (!valid)
                        break;

                    ++i;
                }

                if (digits_begin == i) {
                    throw std::string(
                        "expected digits after integer prefix"
                    );
                }

                result.emplace_back(
                    "integer",
                    code.substr(
                        begin,
                        i - begin
                    )
                );

                continue;
            }

            ++i;

            while (
                i < code.size() &&
                std::isdigit(
                    static_cast<unsigned char>(code[i])
                )
            ) {
                ++i;
            }

            result.emplace_back(
                "integer",
                code.substr(
                    begin,
                    i - begin
                )
            );

            continue;
        }
        if (i + 1 < code.size()) {
            std::string op =
                code.substr(
                    i,
                    2
                );
            if (
                op == "&&" ||
                op == "||" ||
                op == "==" ||
                op == "!=" ||
                op == "<=" ||
                op == ">="
            ) {
                result.emplace_back(
                    "operator",
                    std::move(op)
                );
                i += 2;
                continue;
            }
        }
        switch (c) {
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ',':
            case ';':
            case ':':
            case '.':
                result.emplace_back(
                    "punctuation",
                    std::string(1, c)
                );
                ++i;
                continue;
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '=':
            case '<':
            case '>':
            case '!':
            case '~':
            case '&':
            case '|':
                result.emplace_back(
                    "operator",
                    std::string(1, c)
                );
                ++i;
                continue;
        }
        if (c == '@') {
            while (
                i < code.size() &&
                code[i] != '\n'
            ) {
                ++i;
            }
            continue;
        }
        throw std::string(
            "unexpected character '"
        ) + c + "'";
    }
    if constexpr (DEBUG) {
        std::cout
            << "TOKENS\n\n";
        std::cout
            << "[\n";
        for (const Token& token : result) {
            std::cout
                << "\t{kind: "
                << token.kind
                << ", val: "
                << token.val
                << "},\n";
        }
        std::cout
            << "]\n";
    }
    return result;
}
static std::vector<ParsedToken> parse(
    std::vector<Token> tokens
)
{
    std::vector<ParsedToken> result;
    size_t i = 0;
    auto token_value =
        [&](size_t index) -> const std::string& {
            if (index >= tokens.size()) {
                throw std::string(
                    "token index out of range"
                );
            }
            return tokens[index].val;
        };
    auto is_kind =
        [&](size_t index,
            const std::string& kind) {
            return
                index < tokens.size() &&
                tokens[index].kind == kind;
        };
    auto is_operator =
        [&](size_t index) {
            return
                index < tokens.size() &&
                tokens[index].kind == "operator";
        };
    auto is_punctuation =
        [&](size_t index,
            const std::string& value) {
            return
                index < tokens.size() &&
                tokens[index].kind == "punctuation" &&
                tokens[index].val == value;
        };
    auto make_token =
        [&](const Token& token) {
            return ParsedToken(
                token.val,
                {
                    token.val,
                    token.kind
                }
            );
        };
    auto precedence =
        [&](const std::string& op) -> int {
            if (op == "=")
                return 1;
            if (op == "||")
                return 2;
            if (op == "&&")
                return 3;
            if (op == "|")
                return 4;
            if (op == "&")
                return 5;
            if (
                op == "==" ||
                op == "!="
            ) {
                return 6;
            }
            if (
                op == "<" ||
                op == ">" ||
                op == "<=" ||
                op == ">="
            ) {
                return 7;
            }
            if (
                op == "+" ||
                op == "-"
            ) {
                return 8;
            }
            if (
                op == "*" ||
                op == "/" ||
                op == "%"
            ) {
                return 9;
            }
            return -1;
        };
    std::function<tree(size_t&)> parse_expression;
    std::function<tree(size_t&, int)> parse_binary;
    std::function<tree(size_t&)> parse_primary;
    parse_primary =
        [&](size_t& pos) -> tree {
            if (pos >= tokens.size()) {
                throw std::string(
                    "expected expression"
                );
            }

            if (is_operator(pos)) {
                const std::string& op =
                    token_value(pos);

                if (
                    op == "+" ||
                    op == "-" ||
                    op == "!" ||
                    op == "~" ||
                    op == "*" ||
                    op == "&"
                ) {
                    ++pos;

                    tree operand =
                        parse_primary(pos);

                    tree result(op);

                    result.root()->children.push_back(
                        clone_node(
                            operand.root(),
                            result.root()
                        )
                    );

                    return result;
                }
            }

            tree result;

            if (is_punctuation(pos, "(")) {
                ++pos;

                result =
                    parse_expression(pos);

                if (
                    !is_punctuation(
                        pos,
                        ")"
                    )
                ) {
                    throw std::string(
                        "expected ')'"
                    );
                }

                ++pos;
            }
            else {
                if (
                    !is_kind(pos, "identifier") &&
                    !is_kind(pos, "integer") &&
                    !is_kind(pos, "type") &&
                    !is_kind(pos, "keyword") &&
                    !is_kind(pos, "string")
                ) {
                    throw std::string(
                        "expected expression"
                    );
                }

                result.emplace_root(
                    token_value(pos)
                );

                // Conservar el tipo original del token.
                result.root()->kind =
                    tokens[pos].kind;

                ++pos;
            }

            while (pos < tokens.size()) {
                if (is_punctuation(pos, "(")) {
                    ++pos;

                    tree call("call");

                    call.root()->children.push_back(
                        clone_node(
                            result.root(),
                            call.root()
                        )
                    );

                    if (!is_punctuation(pos, ")")) {
                        while (true) {
                            tree arg =
                                parse_expression(pos);

                            call.root()->children.push_back(
                                clone_node(
                                    arg.root(),
                                    call.root()
                                )
                            );

                            if (
                                is_punctuation(
                                    pos,
                                    ")"
                                )
                            ) {
                                break;
                            }

                            if (
                                !is_punctuation(
                                    pos,
                                    ","
                                )
                            ) {
                                throw std::string(
                                    "expected ',' or ')'"
                                );
                            }

                            ++pos;
                        }
                    }

                    ++pos;

                    result =
                        std::move(call);

                    continue;
                }

                if (is_punctuation(pos, "[")) {
                    ++pos;

                    tree index("[]");

                    index.root()->children.push_back(
                        clone_node(
                            result.root(),
                            index.root()
                        )
                    );

                    tree expr =
                        parse_expression(pos);

                    index.root()->children.push_back(
                        clone_node(
                            expr.root(),
                            index.root()
                        )
                    );

                    if (
                        !is_punctuation(
                            pos,
                            "]"
                        )
                    ) {
                        throw std::string(
                            "expected ']'"
                        );
                    }

                    ++pos;

                    result =
                        std::move(index);

                    continue;
                }

                if (is_punctuation(pos, ".")) {
                    ++pos;

                    if (
                        pos >= tokens.size() ||
                        !is_kind(
                            pos,
                            "identifier"
                        )
                    ) {
                        throw std::string(
                            "expected identifier after '.'"
                        );
                    }

                    tree member(".");

                    member.root()->children.push_back(
                        clone_node(
                            result.root(),
                            member.root()
                        )
                    );

                    auto child =
                        std::make_unique<tree::node>(
                            token_value(pos),
                            member.root()
                        );

                    child->kind =
                        tokens[pos].kind;

                    member.root()->children.push_back(
                        std::move(child)
                    );

                    ++pos;

                    result =
                        std::move(member);

                    continue;
                }

                break;
            }

            return result;
        };
    parse_binary =
        [&](size_t& pos,
            int minimum_precedence) -> tree {
            tree left =
                parse_primary(pos);
            while (
                pos < tokens.size() &&
                is_operator(pos)
            ) {
                const std::string& op =
                    token_value(pos);
                int current_precedence =
                    precedence(op);
                if (
                    current_precedence <
                    minimum_precedence
                ) {
                    break;
                }
                ++pos;
                int next_precedence =
                    current_precedence +
                    (op == "=" ? 0 : 1);
                tree right =
                    parse_binary(
                        pos,
                        next_precedence
                    );
                tree expression(op);
                expression.root()->children.push_back(
                    clone_node(
                        left.root(),
                        expression.root()
                    )
                );
                expression.root()->children.push_back(
                    clone_node(
                        right.root(),
                        expression.root()
                    )
                );
                left =
                    std::move(expression);
            }
            return left;
        };
    parse_expression =
        [&](size_t& pos) -> tree {
            return parse_binary(
                pos,
                1
            );
        };
    while (i < tokens.size()) {
        bool after_fn =
            !result.empty() &&
            !result.back().are_tree() &&
            !result.back().more_info.empty() &&
            result.back().more_info.size() >= 2 &&
            result.back().more_info[0] == "fn" &&
            result.back().more_info[1] == "keyword";
        bool after_type =
            i > 0 &&
            tokens[i - 1].kind == "type";
        if (
            !after_fn &&
            !after_type &&
            (
                tokens[i].kind == "identifier" ||
                tokens[i].kind == "integer" ||
                tokens[i].kind == "string"
            )
        ) {
            size_t begin = i;
            try {
                tree expression =
                    parse_expression(i);
                if (i > begin) {
                    result.emplace_back(
                        std::move(expression)
                    );
                    continue;
                }
            }
            catch (...) {
                i = begin;
            }
        }
        result.push_back(
            make_token(
                tokens[i]
            )
        );
        ++i;
    }
    if constexpr (DEBUG) {
        std::cout
            << "PARSED\n\n";
        std::cout
            << "[\n";
        for (
            const ParsedToken& elem
            : result
        ) {
            if (elem.are_tree()) {
                std::cout
                    << "\t{tree},\n";
            }
            else {
                std::cout
                    << "\t{kind: ";
                if (
                    elem.more_info.size() >= 2
                ) {
                    std::cout
                        << elem.more_info[1];
                }
                else {
                    std::cout
                        << "?";
                }
                std::cout
                    << ", val: "
                    << elem.gets()
                    << "},\n";
            }
        }
        std::cout
            << "]\n";
    }
    return result;
}
static std::string cpp_type(
    const std::string& type
)
{
    size_t i = 0;
    while (
        i < type.size() &&
        (
            is_ident_char(type[i]) ||
            type[i] == ':'
        )
    ) {
        ++i;
    }
    std::string base =
        type.substr(
            0,
            i
        );
    std::string result;
    if (base == "int")
        result = "hc::ptrdiff_t";
    else if (base == "uint")
        result = "hc::size_t";
    else if (base == "float")
        result = "double";
    else if (base == "ubyte")
        result = "hc::ubyte";
    else if (base == "ibyte")
        result = "hc::ibyte";
    else if (base == "char")
        result = "char";
    else if (base == "bool")
        result = "bool";
    else if (base == "fmax")
        result = "long double";
    else if (base == "imax")
        result = "hc::intmax_t";
    else if (base == "umax")
        result = "hc::uintmax_t";
    else if (base == "_AutoType")
        result = "auto";
    else {
        if (!types.contains(base)) {
            throw std::string(
                "not defined type: "
            ) + base;
        }
        result =
            mangle_name(base);
    }
    while (i < type.size()) {
        if (type[i] == '*') {
            result += '*';
            ++i;
            continue;
        }
        if (type[i] == '[') {
            size_t end =
                type.find(
                    ']',
                    i
                );
            if (
                end == std::string::npos
            ) {
                throw std::string(
                    "invalid array type: "
                ) + type;
            }
            std::string length =
                type.substr(
                    i + 1,
                    end - i - 1
                );
            if (length.empty()) {
                result =
                    "hc::slice<" +
                    result +
                    ">";
            }
            else {
                result +=
                    "[" +
                    length +
                    "]";
            }
            i =
                end + 1;
            continue;
        }
        throw std::string(
            "invalid type: "
        ) + type;
    }
    return result;
}
static std::string escape_cpp_string(
    const std::string& value
)
{
    std::string result;

    for (char c : value) {
        switch (c) {
            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '"':
                result += "\\\"";
                break;

            case '\0':
                result += "\\0";
                break;

            default:
                result += c;
                break;
        }
    }

    return result;
}

static std::string gen_expr(
    const tree::node* node
)
{
    if (!node)
        return "";

    const std::string& op =
        node->value;

    // Hoja
    if (node->children.empty()) {
        if (node->kind == "string") {
            return
                "\"" +
                escape_cpp_string(node->value) +
                "\"";
        }

        if (node->kind == "integer") {
            return node->value;
        }

        if (node->kind == "keyword") {
            if (node->value == "nullptr")
                return "nullptr";
            if (node->value == "nullslice")
                return "hc::slice<void>{nullptr, 0}";

            return node->value;
        }
        return mangle_name(node->value);
    }

    if (
        node->children.size() == 2 &&
        (
            op == "+" ||
            op == "-" ||
            op == "*" ||
            op == "/" ||
            op == "%" ||
            op == "=" ||
            op == "==" ||
            op == "!=" ||
            op == "<" ||
            op == ">" ||
            op == "<=" ||
            op == ">=" ||
            op == "&&" ||
            op == "||"
        )
    ) {
        return
            "(" +
            gen_expr(
                node->children[0].get()
            ) +
            " " +
            op +
            " " +
            gen_expr(
                node->children[1].get()
            ) +
            ")";
    }

    if (
        node->children.size() == 1 &&
        (
            op == "+" ||
            op == "-" ||
            op == "!" ||
            op == "~" ||
            op == "*" ||
            op == "&"
        )
    ) {
        return
            "(" +
            op +
            gen_expr(
                node->children[0].get()
            ) +
            ")";
    }

    if (op == "call") {
        if (node->children.empty()) {
            throw std::string(
                "invalid call expression"
            );
        }

        std::string result =
            gen_expr(
                node->children[0].get()
            );

        result += "(";

        for (
            size_t i = 1;
            i < node->children.size();
            ++i
        ) {
            if (i != 1)
                result += ", ";

            result +=
                gen_expr(
                    node->children[i].get()
                );
        }

        result += ")";

        return result;
    }

    if (op == "[]") {
        if (node->children.size() != 2) {
            throw std::string(
                "invalid index expression"
            );
        }

        return
            gen_expr(
                node->children[0].get()
            ) +
            "[" +
            gen_expr(
                node->children[1].get()
            ) +
            "]";
    }

    if (op == ".") {
        if (node->children.size() != 2) {
            throw std::string(
                "invalid member expression"
            );
        }

        return
            gen_expr(
                node->children[0].get()
            ) +
            "." +
            gen_expr(
                node->children[1].get()
            );
    }

    throw std::string(
        "cannot generate C++ for expression tree: "
    ) + op;
}
static std::string parsed_value(
    const ParsedToken& token
)
{
    if (token.are_tree()) {
        return gen_expr(
            token.gett().root()
        );
    }
    if (token.more_info.empty()) {
        throw std::string(
            "parsed token has no value"
        );
    }
    return token.more_info[0];
}
static std::string get_function_name(
    const std::vector<ParsedToken>& parsed,
    size_t start,
    size_t& next
)
{
    if (
        start >= parsed.size() ||
        !parsed[start].are_token()
    ) {
        throw std::string(
            "expected function name "
            "after fn"
        );
    }
    const ParsedToken& first =
        parsed[start];
    if (
        first.more_info.size() < 2 ||
        first.more_info[1] != "identifier"
    ) {
        throw std::string(
            "expected function name "
            "after fn"
        );
    }
    next =
        start + 1;
    return first.more_info[0];
}

static std::string gen_cpp(
    std::vector<ParsedToken> parsed,
    bool imports = true
)
{
    std::string result;
    size_t i = 0;
    bool on_main = false;
    bool on_os_block = false;
    while (i < parsed.size()) {
        ParsedToken& token =
            parsed[i];
        if (token.are_tree()) {
            result +=
                gen_expr(
                    token.gett().root()
                );
            ++i;
            continue;
        }
        if (
            token.more_info.empty()
        ) {
            throw std::string(
                "parsed token has no value"
            );
        }
        const std::string& value =
            token.more_info[0];
        std::string kind;
        if (
            token.more_info.size() >= 2
        ) {
            kind =
                token.more_info[1];
        }
        if (
            kind == "keyword" &&
            value == "fn"
        ) {
            size_t name_end = 0;
            std::string name =
                get_function_name(
                    parsed,
                    i + 1,
                    name_end
                );
            if (name == "main") on_main = true;
            result +=
                "extern \"C\" auto " +
                (on_main ? "__hc_main__" : mangle_name(name));
            i = name_end;
            if (
                i >= parsed.size() ||
                !parsed[i].are_token() ||
                parsed[i].more_info[0] != "("
            ) {
                throw std::string(
                    "expected '(' after function name"
                );
            }
            result += "(";
            ++i;
            int depth = 1;
            while (
                i < parsed.size() &&
                depth > 0
            ) {
                if (
                    parsed[i].are_token() &&
                    parsed[i].more_info.size() >= 1
                ) {
                    const std::string& v =
                        parsed[i].more_info[0];
                    if (v == "(") {
                        ++depth;
                        result += "(";
                    }
                    else if (v == ")") {
                        --depth;
                        result += ")";
                    }
                    else if (
                        parsed[i].more_info.size() >= 2 &&
                        parsed[i].more_info[1] == "type"
                    ) {
                        result +=
                            cpp_type(v);
                    }
                    else {
                        result += v;
                    }
                }
                else {
                    if (parsed[i].are_token())
                        result +=
                            parsed[i].gets();
                    else {
                        result += gen_expr(parsed[i].gett().root());
                    }
                }
                ++i;
            }
            if (
                i < parsed.size() &&
                parsed[i].are_token() &&
                parsed[i].more_info.size() >= 2 &&
                parsed[i].more_info[1] == "type"
            ) {
                result +=
                    " -> ";
                
                result +=
                    cpp_type(
                        parsed[i].more_info[0]
                    );
                
                ++i;
                
            } else {
                result+= " -> void ";
                ++i;
            }
            
            continue;
        }
        if (kind == "@os") {
            std::string os = (value == "windows" ? "_Win32" : (value == "linux" ? "__linux__" : (value == "apple" ? "__APPLE__" : (value == "other" ? "!defined(__linux__) && !defined(_Win32) && !defined(__APPLE__)" : (os == "end" ? "end" : (value == "else" ? "else" : "invalid"))))));
            if (os == "invalid") {
                throw std::string(
                    "invalid OS name after @os: "
                ) + value;
            }
            if (os == "end") {
                if (!on_os_block) {
                    throw std::string(
                        "@osend without matching @os"
                    );
                }
                result += "#endif\n";
                on_os_block = false;
                ++i;
                continue;
            }
            if (os == "end") {
                if (!on_os_block) {
                    throw std::string(
                        "@osend without matching @os"
                    );
                }
                result += "#else\n";
                ++i;
                continue;
            }

            if (os.starts_with("!")) {
                result += (on_os_block ? "#elif " : "if ") + os + "\n";
            } else {
                result += (on_os_block ? "#elif " : "if ");
                result += " defined(" + os + ")\n";
            }
            on_os_block = true;
        }
        if (
            kind == "keyword" &&
            value == "typedef"
        ) {
            result += "typedef";
            ++i;
            if (
                i < parsed.size() &&
                parsed[i].are_token()
            ) {
                ParsedToken& name_token =
                    parsed[i];
                if (
                    name_token.more_info.size() >= 2 &&
                    name_token.more_info[1] == "identifier"
                ) {
                    result += " ";
                    std::string _typename =
                        mangle_name(
                            name_token.more_info[0]
                        );
                    result += _typename;
                    ++i;
                    continue;
                }
            }
            result += " ";
            continue;
        }
        if (kind == "c++") {
            result += value;
            ++i;
            continue;
        }
        if (kind == "type") {
            result +=
                cpp_type(value) + " ";
            ++i;
            continue;
        }
        
        if (kind == "string") {
            result += '"';
            for (char c : value) {
                switch (c) {
                    case '\n':
                        result += "\\n";
                        break;
                    case '\r':
                        result += "\\r";
                        break;
                    case '\t':
                        result += "\\t";
                        break;
                    case '\\':
                        result += "\\\\";
                        break;
                    case '"':
                        result += "\\\"";
                        break;
                    case '\0':
                        result += "\\0";
                        break;
                    default:
                        result += c;
                        break;
                }
            }
            result += '"';
            ++i;
            continue;
        }
        if (kind == "punctuation") {
            result += value;
            if (value == ";") {
                result += '\n';
            }
            ++i;
            continue;
        }
        if (kind == "keyword") {
            if (value == "nullptr") {
                result +=
                    "nullptr";
            }
            else if (value == "elif") {
                result +=
                    "else if";
            }
            else if (value == "cstruct") {
                result +=
                    "struct ";

                if (parsed[i+1].more_info[1] != "type")
                    throw std::string(
                        "cstruct need a name"
                    );
            }
            else if (value == "struct") {
                result +=
                    "struct ";
                
                if (parsed[i+1].more_info[1] != "type" && parsed[i+1].are_token())
                    throw std::string(
                        "struct need a name but it's a "
                    );
                i++;
                result += mangle_name(parsed[i].gets());
                result += " : hc::NonCopyable ";
            }
            else if (value == "defer") {
                std::string num = std::to_string(i);
                result +=
                    "hc::Defer _I5defer" + std::to_string(num.size()+1) + "_" + num + " = hc::Defer{[]{";
                size_t begin = i +1, end = begin;
                while (end < parsed.size()) {
                    if (parsed[end].are_token() &&
                        parsed[end].more_info[0] == ";")
                        break;

                    ++end;
                }
                std::vector<ParsedToken> body;
                for (size_t j = begin; j < end; ++j) {
                    if (parsed[j].are_token() && parsed[j].more_info[0] == "defer") {
                        throw std::string(
                            "nested defer is not allowed"
                        );
                    }
                    if (parsed[j].are_token() && parsed[j].more_info[0] == "return") {
                        throw std::string(
                            "return statement is not allowed inside defer"
                        );
                    }
                    if (parsed[j].are_token() && parsed[j].more_info[0] == "fn") {
                        throw std::string(
                            "function definition is not allowed inside defer"
                        );
                    }
                    if (parsed[j].are_token() && parsed[j].more_info[0] == "struct") {
                        throw std::string(
                            "struct definition is not allowed inside defer"
                        );
                    }
                    if (parsed[j].are_token() && parsed[j].more_info[0] == "typedef") {
                        throw std::string(
                            "typedef is not allowed inside defer"
                        );
                    }
                    if (parsed[j].are_token() && parsed[j].more_info[0] == "cstruct") {
                        throw std::string(
                            "cstruct definition is not allowed inside defer"
                        );
                    }

                    if (parsed[j].are_token()) {
                        body.emplace_back(parsed[j].gets(), parsed[j].more_info);
                    }

                    if (parsed[j].are_tree()) {
                        body.emplace_back(clone_tree(parsed[j].gett()));
                    }
                   
                }
                body.emplace_back(";", std::vector<std::string>{";", "punctuation"});
                result += gen_cpp(std::move(body), false);
                result += "}};";
                i = end;
            }
            else {
                result +=
                    value + " ";
            }
            ++i;
            continue;
        }
        if (kind == "identifier") {
            result +=
                mangle_name(value);
            ++i;
            continue;
        }
        result += value;
        ++i;
    }
    if (imports) {
        result = "#include \"std.tmp.hpp\"\n" + result;
    }
    if (on_main) {
        result+= R"(
            int main(int argc, char** argv) {
                hc::slice<char>* args = new hc::slice<char>[argc];

                for (int i = 0; i < argc; ++i) {
                    std::size_t len = 0;

                    while (argv[i][len] != '\0')
                        ++len;

                    args[i] = {
                        argv[i],
                        len
                    };
                }

                __hc_main__(
                    hc::slice<hc::slice<char>>{
                        args,
                        static_cast<std::size_t>(argc)
                    }
                );
                delete[] args;
                return 0;
            }
        )";
    }
    return result;
}
std::vector<std::string> tmp_files;
inline void transpile()
{
    for (
        const auto& [module_name, module]
        : program.Modules
    ) {
        if constexpr (DEBUG) {
            std::cout
                << "TRANSPILING MODULE: "
                << module_name
                << "\n";
        }
        try {
            std::vector<Token> tokens =
                lex(module);

            for (const todo& item : todo_list) {
                if (item.id == todo_id::TYPE_ID) {
                    if (! item()) {
                        throw std::string(
                            "expected type definition for: "
                        ) + item.args[1] + ", got: " + item.args[0];
                    }
                }
            }
            std::vector<ParsedToken> parsed =
                parse(
                    std::move(tokens)
                );
            std::string cpp =
                gen_cpp(
                    std::move(parsed)
                );
            std::string filename =
                module_name +
                ".cpp";
            std::ofstream file(
                filename,
                std::ios::out |
                std::ios::trunc
            );
            if (!file.is_open()) {
                throw std::string(
                    "cannot create transpiled file: "
                ) + filename;
            }
            file << cpp;
            if (!file) {
                throw std::string(
                    "error writing transpiled file: "
                ) + filename;
            }
            file.close();
            tmp_files.push_back(filename);
            if constexpr (DEBUG) {
                std::cout
                    << "generated "
                    << filename
                    << "\n";
            }
        }
        catch (const std::string& error) {
            throw std::string(
                "[module " +
                module_name +
                "] " +
                error
            );
        }
    }


    std::ofstream file(
        "std.tmp.hpp",
        std::ios::out |
        std::ios::trunc
    );

    file << standard;

    std::ofstream file2(
        "std.tmp.cpp",
        std::ios::out |
        std::ios::trunc
    );

    file2 << standarddefs;
}
#endif