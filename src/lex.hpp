#ifndef LEX_HPP
#define LEX_HPP
#include "../libs/transcode/transcode.hpp"
#include <vector>
#include "types.hpp"
#include <iostream>
#define pass
namespace c {
    // ============================================================
    // LEXER
    // ============================================================

    class Lexer {
        const tc::Source& source;
        size_t pos = 0;

    public:
        explicit Lexer(const tc::Source& source)
            : source(source) {
        }

        tc::Opt<std::vector<Token>> lex() {
            std::vector<Token> result;

            while (true) {
                skip_whitespace_and_comments();

                if (pos >= source.source.size()) {
                    result.push_back({
                        TokenKind::End,
                        "",
                        pos
                    });

                    return result;
                }

                tc::Opt<Token> token = next_token();

                if (!token)
                    return tc::NullOpt;

                result.push_back(std::move(*token));
            }
        }

    private:
        void error(const tc::String& message) const {
            std::cerr << "lex error: " << message;

            if (pos < source.source.size()) {
                std::cerr
                    << " (got `"
                    << source.source[pos]
                    << "`)";
            }

            std::cerr << '\n';

            size_t begin = pos > 30 ? pos - 30 : 0;
            size_t end = pos + 30;

            if (end > source.source.size())
                end = source.source.size();

            std::cerr
                << source.source.substr(begin, end - begin)
                << '\n';

            size_t marker = pos - begin;

            for (size_t i = 0; i < marker; ++i)
                std::cerr << ' ';

            std::cerr << "\n";
        }

        void skip_whitespace_and_comments() {
            while (pos < source.source.size()) {
                char c = source.source[pos];

                if (std::isspace(static_cast<unsigned char>(c))) {
                    ++pos;
                    continue;
                }

                if (
                    c == '/' &&

    // ------------------------------------------------------------
    //
                    pos + 1 < source.source.size() &&
                    source.source[pos + 1] == '/'
                ) {
                    pos += 2;

                    while (
                        pos < source.source.size() &&
                        source.source[pos] != '\n'
                    ) {
                        ++pos;
                    }

                    continue;
                }

                if (
                    c == '/' &&
                    pos + 1 < source.source.size() &&
                    source.source[pos + 1] == '*'
                ) {
                    pos += 2;

                    while (pos + 1 < source.source.size()) {
                        if (
                            source.source[pos] == '*' &&
                            source.source[pos + 1] == '/'
                        ) {
                            pos += 2;
                            break;
                        }

                        ++pos;
                    }

                    continue;
                }

                break;
            }
        }

        tc::Opt<Token> next_token() {
            const size_t begin = pos;
            const char c = source.source[pos];

            if (c == '@')
                return at_sign();

            if (is_identifier_start(c))
                return identifier();

            if (std::isdigit(static_cast<unsigned char>(c)))
                return number();

            if (c == '"')
                return quoted(TokenKind::String, '"');

            if (c == '\'')
                return quoted(TokenKind::Character, '\'');

            return punctuation();
        }

        tc::Opt<Token> at_sign() {
            if (source.source.compare(pos, 4, "@c++") == 0)
                return cpp_code();
            error("invalid sign");
            return tc::NullOpt;
        }
        tc::Opt<Token> cpp_code() {
            tc::Opt<size_t> optposition = source.find("@c++end", pos, tc::MatchOptions::StrSafe).to_idx(source);

            if (!optposition) {
                error("non closed @c++");
                return tc::NullOpt;
            }

            size_t begin = pos+4;
            pos = *optposition+7;

            return Token{
                TokenKind::Cpp,
                source.source.substr(begin, pos-(begin+7)),
                begin-4
            };
        }

        static bool is_identifier_start(char c) {
            unsigned char x =
                static_cast<unsigned char>(c);

            return std::isalpha(x) || c == '_';
        }

        static bool is_identifier_char(char c) {
            unsigned char x =
                static_cast<unsigned char>(c);

            return std::isalnum(x) || c == '_';
        }

        tc::Opt<Token> identifier() {
            const size_t begin = pos;

            ++pos;

            while (
                pos < source.source.size() &&
                is_identifier_char(source.source[pos])
            ) {
                ++pos;
            }

            tc::String text =
                source.source.substr(begin, pos - begin);

            TokenKind kind = keyword(text);

            return Token{
                kind,
                std::move(text),
                begin
            };
        }

        static TokenKind keyword(const tc::StrView& text) {
            if (text == "fn")       return TokenKind::Fn;
            if (text == "struct")   return TokenKind::Struct;
            if (text == "cstruct")  return TokenKind::CStruct;
            if (text == "typedef")  return TokenKind::Typedef;
            if (text == "enum")     return TokenKind::Enum;

            if (text == "if")       return TokenKind::If;
            if (text == "else")     return TokenKind::Else;
            if (text == "elif")     return TokenKind::Elif;
            if (text == "while")    return TokenKind::While;
            if (text == "do")       return TokenKind::Do;
            if (text == "for")      return TokenKind::For;
            if (text == "switch")   return TokenKind::Switch;
            if (text == "case")     return TokenKind::Case;
            if (text == "default")  return TokenKind::Default;

            if (text == "break")    return TokenKind::Break;
            if (text == "continue") return TokenKind::Continue;
            if (text == "return")   return TokenKind::Return;

            if (text == "true")     return TokenKind::True;
            if (text == "false")    return TokenKind::False;
            if (text == "nullptr")  return TokenKind::Nullptr;
            if (text == "defer")    return TokenKind::Defer;
            if (text == "make")     return TokenKind::Make;

            return TokenKind::Identifier;
        }

        tc::Opt<Token> number() {
            size_t begin = pos;
            if (
                source.source[pos] == '0' &&
                pos + 1 < source.source.size()
            ) {
                char next = source.source[pos + 1];

                if (next == 'x' || next == 'X') {
                    pos += 2;

                    while (
                        pos < source.source.size() &&
                        std::isxdigit(
                            static_cast<unsigned char>(
                                source.source[pos]
                            )
                        )
                    ) {
                        ++pos;
                    }

                    return Token{
                        TokenKind::Integer,
                        source.source.substr(begin, pos - begin),
                        begin
                    };
                }

                if (next == 'b' || next == 'B') {
                    pos += 2;

                    while (
                        pos < source.source.size() &&
                        (
                            source.source[pos] == '0' ||
                            source.source[pos] == '1'
                        )
                    ) {
                        ++pos;
                    }

                    return Token{
                        TokenKind::Integer,
                        source.source.substr(begin, pos - begin),
                        begin
                    };
                }

                if (next == 'o' || next == 'O') {
                    pos += 2;

                    while (
                        pos < source.source.size() &&
                        source.source[pos] >= '0' &&
                        source.source[pos] <= '7'
                    ) {
                        ++pos;
                    }

                    return Token{
                        TokenKind::Integer,
                        "0" + source.source.substr(begin+2, pos - begin-2),
                        begin
                    };
                }
            }

            while (
                pos < source.source.size() &&
                std::isdigit(
                    static_cast<unsigned char>(
                        source.source[pos]
                    )
                )
            ) {
                ++pos;
            }

            TokenKind kind = TokenKind::Integer;

            if (
                pos < source.source.size() &&
                source.source[pos] == '.'
            ) {
                kind = TokenKind::Float;
                ++pos;

                while (
                    pos < source.source.size() &&
                    std::isdigit(
                        static_cast<unsigned char>(
                            source.source[pos]
                        )
                    )
                ) {
                    ++pos;
                }
            }
            tc::String num = source.source.substr(begin, pos - begin);
            size_t i = 0;
            for (;i < num.size() && num[i] == '0'; i++);
            if (i == num.size()) num = "0";
            else if (i == 0)     pass;
            else                 num = num.substr(i-1);
            return Token{
                kind,
                num,
                begin+(i ? i-1 : 0)
            };
        }

        tc::Opt<Token> quoted(
            TokenKind kind,
            char quote
        ) {
            const size_t begin = pos;

            ++pos;

            while (pos < source.source.size()) {
                if (source.source[pos] == '\\') {
                    if (pos + 1 >= source.source.size()) {
                        error(tc::String("non closed quote ")+quote);
                        return tc::NullOpt;
                    }

                    pos += 2;
                    continue;
                }

                if (source.source[pos] == quote) {
                    ++pos;

                    return Token{
                        kind,
                        source.source.substr(begin, pos - begin),
                        begin
                    };
                }

                ++pos;
            }
            error(tc::String("non closed quote ")+quote);
            return tc::NullOpt;
        }

        tc::Opt<Token> punctuation() {
            struct Op {
                tc::StrView text;
                TokenKind kind;
            };

            static constexpr Op ops[] = {
                {"++", TokenKind::PlusPlus},
                {"--", TokenKind::MinusMinus},
                {"+=", TokenKind::PlusEqual},
                {"-=", TokenKind::MinusEqual},
                {"*=", TokenKind::StarEqual},
                {"/=", TokenKind::SlashEqual},
                {"%=", TokenKind::PercentEqual},
                {"==", TokenKind::EqualEqual},
                {"!=", TokenKind::NotEqual},
                {"<=", TokenKind::LessEqual},
                {">=", TokenKind::GreaterEqual},
                {"&&", TokenKind::LogicalAnd},
                {"||", TokenKind::LogicalOr},
                {"->", TokenKind::Arrow},

                {"+", TokenKind::Plus},
                {"-", TokenKind::Minus},
                {"*", TokenKind::Star},
                {"/", TokenKind::Slash},
                {"%", TokenKind::Percent},

                {"=", TokenKind::Equal},

                {"<", TokenKind::Less},
                {">", TokenKind::Greater},

                {"&", TokenKind::BitAnd},
                {"|", TokenKind::BitOr},
                {"^", TokenKind::BitXor},
                {"~", TokenKind::BitNot},
                {"!", TokenKind::LogicalNot},

                {"?", TokenKind::Question},
                {":", TokenKind::Colon},

                {".", TokenKind::Dot},
                {",", TokenKind::Comma},
                {";", TokenKind::Semicolon},

                {"(", TokenKind::LParen},
                {")", TokenKind::RParen},
                {"{", TokenKind::LBrace},
                {"}", TokenKind::RBrace},
                {"[", TokenKind::LBracket},
                {"]", TokenKind::RBracket}
            };

            for (const Op& op : ops) {
                if (
                    source.source.size() - pos >= op.text.size() &&
                    source.source.compare(
                        pos,
                        op.text.size(),
                        op.text
                    ) == 0
                ) {
                    size_t begin = pos;

                    pos += op.text.size();

                    return Token{
                        op.kind,
                        tc::String(op.text),
                        begin
                    };
                }
            }
            error("invalid operator");
            return tc::NullOpt;
        }
    };
}
#endif