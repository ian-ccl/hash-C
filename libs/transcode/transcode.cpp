#include "transcode.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstdint>

#define iter_args(o) std::begin(o), std::end(o)


bool tc::Position::validate(const Source& s) const noexcept {
    if (line == 0 || column == 0)
        return false;
    if (*this == tc::Position::npos)
        return false;
    

    return to_idx(s).has_value();
}


tc::Opt<size_t> tc::Position::to_idx(const Source& s) const noexcept {
    if (line == 0 || column == 0)
        return tc::NullOpt;

    size_t current_line = 1;
    size_t current_column = 1;

    for (size_t i = 0; i < s.source.size(); ++i) {
        if (current_line == line && current_column == column)
            return i;

        if (s.source[i] == '\n') {
            ++current_line;
            current_column = 1;
        } else {
            ++current_column;
        }
    }

    // Position immediately after the final character.
    if (current_line == line && current_column == column)
        return s.source.size();

    return tc::NullOpt;
}

tc::Opt<size_t> tc::SourcePosition::this_to_idx() const noexcept {
    return to_idx(src);
}

bool tc::SourcePosition::this_validate() const noexcept {
    return validate(src);
}

bool tc::SourcePosition::operator==(const Position& pos) const {
    return (*dynamic_cast<const Position*>(this)) == pos;
}
tc::SourcePosition& tc::SourcePosition::operator=(const Position& pos) {
    this->column = pos.column;
    this->line = pos.line;
    return *this;
}

const tc::Position tc::Position::npos = {SIZE_MAX, SIZE_MAX};


tc::Position tc::Source::position(size_t n) const noexcept {
    if (n > source.size())
        return Position::npos;

    Position pos{1, 1};

    for (size_t i = 0; i < n; ++i) {
        if (source[i] == '\n') {
            ++pos.line;
            pos.column = 1;
        } else {
            ++pos.column;
        }
    }

    return pos;
}


tc::Position tc::Source::position(size_t line, size_t column) const noexcept {
    Position pos{line, column};

    if (!pos.validate(*this))
        return Position::npos;

    return pos;
}


tc::Opt<size_t> tc::Source::index(Position pos) const noexcept {
    return pos.to_idx(*this);
}


tc::Opt<tc::Ref<const char>> tc::Source::at(Position pos) const noexcept {
    tc::Opt<size_t> p = index(pos);

    if (!p || *p >= source.size())
        return tc::NullOpt;

    return source[*p];
}


bool tc::Source::replace(Position pos, size_t n, const StrView& s) noexcept {
    tc::Opt<size_t> p = index(pos);

    if (!p || *p > source.size())
        return false;

    source.replace(*p, n, s);
    return true;
}


bool tc::Source::is_word_char(size_t pos) const noexcept {
    if (pos >= source.size())
        return false;

    unsigned char c = static_cast<unsigned char>(source[pos]);
    return std::isalnum(c) || c == '_';
}


bool tc::Source::is_word_char(Position pos) const noexcept {
    tc::Opt<size_t> p = index(pos);

    if (!p)
        return false;

    return is_word_char(*p);
}


bool tc::Source::is_inside_comment(size_t pos) const noexcept {
    if (pos > source.size())
        return false;

    enum class State {
        Normal,
        String,
        Char,
        LineComment,
        BlockComment
    };

    State state = State::Normal;

    for (size_t i = 0; i < pos; ++i) {
        char c = source[i];

        switch (state) {
        case State::Normal:
            if (c == '"') {
                state = State::String;
            } else if (c == '\'') {
                state = State::Char;
            } else if (c == '/' && i + 1 < pos && source[i + 1] == '/') {
                state = State::LineComment;
                ++i;
            } else if (c == '/' && i + 1 < pos && source[i + 1] == '*') {
                state = State::BlockComment;
                ++i;
            }
            break;

        case State::String:
            if (c == '\\') {
                if (i + 1 < pos)
                    ++i;
            } else if (c == '"') {
                state = State::Normal;
            }
            break;

        case State::Char:
            if (c == '\\') {
                if (i + 1 < pos)
                    ++i;
            } else if (c == '\'') {
                state = State::Normal;
            }
            break;

        case State::LineComment:
            if (c == '\n')
                state = State::Normal;
            break;

        case State::BlockComment:
            if (c == '*' && i + 1 < pos && source[i + 1] == '/') {
                state = State::Normal;
                ++i;
            }
            break;
        }
    }

    return state == State::LineComment || state == State::BlockComment;
}


bool tc::Source::is_inside_comment(Position pos) const noexcept {
    tc::Opt<size_t> p = index(pos);

    if (!p)
        return false;

    return is_inside_comment(*p);
}

bool tc::Source::is_inside_string(size_t idx) const noexcept  {
    if (idx >= source.size()) {
        return false;
    }

    bool inside = false;

    for (size_t i = 0; i <= idx; ++i) {
        if (source[i] == '"') { 
            size_t backslashes = 0;
            size_t j = i;
            while (j > 0 && source[j - 1] == '\\') {
                backslashes++;
                j--;
            }

            if (backslashes % 2 == 0) {
                if (i == idx) {
                    return inside;
                }
                inside = !inside;
            }
        }
    }

    return inside;
}

bool tc::Source::is_inside_string(Position pos) const noexcept {
    tc::Opt<size_t> idx = index(pos);
    if (!idx) return false;
    return is_inside_string(*idx);
}

tc::Position tc::Source::find(const StrView& s, size_t p, tc::MatchOptions opts) const noexcept {
    size_t pos = p;
    while (true) {
        pos = source.find(s, pos);

        if (pos == tc::String::npos)
            return Position::npos;

        if (opts & tc::MatchOptions::WholeWord) {
            bool left_ok = pos == 0 || !is_word_char(pos - 1);
            bool right_ok = pos + s.size() >= source.size() || !is_word_char(pos + s.size());

            if (!left_ok || !right_ok) {
                ++pos;
                continue;
            }
        }

        if (opts & tc::MatchOptions::WholeLine) {
            size_t line_begin = source.rfind('\n', pos);
            size_t line_end = source.find('\n', pos);

            if (line_begin == tc::String::npos)
                line_begin = 0;
            else
                ++line_begin;

            if (line_end == tc::String::npos)
                line_end = source.size();

            if (pos != line_begin || pos + s.size() != line_end) {
                ++pos;
                continue;
            }
        }

        if (!(opts & tc::MatchOptions::AcceptComment) && is_inside_comment(pos)) {
            ++pos;
            continue;
        }

        if ((opts & MatchOptions::StrSafe) && is_inside_string(pos)) {
            ++pos;
            continue;
        }

        return position(pos);
    }
}
bool tc::Source::replace(const StrView& text,
                         const StrView& replacement,
                         size_t startidxfind,
                         MatchOptions options) noexcept {
    Position pos = find(text, startidxfind, options);

    if (pos == Position::npos)
        return false;

    return replace(pos, text.size(), replacement);
}


size_t tc::Source::replace_all(const StrView& text,
                               const StrView& replacement,
                               MatchOptions options) noexcept {
    size_t count = 0;
    size_t search_idx = 0;

    while (search_idx <= source.size()) {
        Position pos = find(text, search_idx, options);

        if (pos == Position::npos)
            break;

        Opt<size_t> idx = index(pos);

        if (!idx)
            break;

        size_t i = *idx;

        if (!replace(pos, text.size(), replacement))
            break;

        ++count;

        // Continuar después del reemplazo.
        search_idx = i + replacement.size();

        // Evita volver a encontrar infinitamente una cadena vacía.
        if (text.empty())
            break;
    }

    return count;
}

using Kind = tc::Pattern::Kind;
using Token = tc::Pattern::Token;

Kind pattern_kind(tc::StrView name) {
    if (name == "w") return Kind::Whitespace;
    if (name == "mw") return Kind::MaybeWhitespace;
    if (name == "n") return Kind::Newline;
    if (name == "ident") return Kind::Identifier;
    if (name == "identm") return Kind::MangledIdentifier;
    if (name == "type") return Kind::Type;
    if (name == "num") return Kind::Number;
    if (name == "str") return Kind::String;
    if (name == "char") return Kind::Character;
    if (name == "?") return Kind::AnyChar;
    if (name == "any") return Kind::Any;

    return Kind::Literal;
}

bool is_ident_start(char c) {
    unsigned char x = static_cast<unsigned char>(c);
    return std::isalpha(x) || c == '_';
}

bool is_ident_char(char c) {
    unsigned char x = static_cast<unsigned char>(c);
    return std::isalnum(x) || c == '_';
}

bool match_identifier(tc::StrView source, size_t& pos) {
    if (pos >= source.size() || !is_ident_start(source[pos]))
        return false;

    ++pos;

    while (pos < source.size() && is_ident_char(source[pos]))
        ++pos;

    return true;
}

bool match_identifier_mangled(tc::StrView source, size_t& pos) {
    if (!match_identifier(source, pos))
        return false;

    while (pos < source.size() && source[pos] == ':') {
        ++pos;

        if (!match_identifier(source, pos))
            return false;
    }

    return true;
}

bool match_type(tc::StrView source, size_t& pos) {
    if (!match_identifier_mangled(source, pos))
        return false;

    while (pos < source.size()) {
        if (source[pos] == '*') {
            ++pos;
            continue;
        }

        break;
    }

    return true;
}

bool match_number(tc::StrView source, size_t& pos) {
    size_t begin = pos;

    if (pos < source.size() &&
        (source[pos] == '+' || source[pos] == '-'))
        ++pos;

    bool has_digit = false;

    while (pos < source.size() &&
           std::isdigit(static_cast<unsigned char>(source[pos]))) {
        ++pos;
        has_digit = true;
    }

    if (pos < source.size() && source[pos] == '.') {
        ++pos;

        while (pos < source.size() &&
               std::isdigit(static_cast<unsigned char>(source[pos]))) {
            ++pos;
            has_digit = true;
        }
    }

    return has_digit && pos > begin;
}

bool match_quoted(tc::StrView source, size_t& pos, char quote) {
    if (pos >= source.size() || source[pos] != quote)
        return false;

    ++pos;

    while (pos < source.size()) {
        if (source[pos] == '\\') {
            if (pos + 1 >= source.size())
                return false;

            pos += 2;
            continue;
        }

        if (source[pos] == quote) {
            ++pos;
            return true;
        }

        ++pos;
    }

    return false;
}

bool match_token(
    const tc::Source& source,
    const Token& token,
    size_t& pos
) {
    const tc::StrView text = source.source;

    switch (token.kind) {
    case Kind::Literal:
        if (pos + token.text.size() > text.size())
            return false;

        if (text.substr(pos, token.text.size()) != token.text)
            return false;

        pos += token.text.size();
        return true;

    case Kind::Whitespace:
        if (pos >= text.size() ||
            !std::isspace(static_cast<unsigned char>(text[pos])) ||
            text[pos] == '\n')
            return false;

        do {
            ++pos;
        } while (pos < text.size() &&
                 std::isspace(static_cast<unsigned char>(text[pos])) &&
                 text[pos] != '\n');

        return true;

    case Kind::MaybeWhitespace:
        while (pos < text.size() &&
               std::isspace(static_cast<unsigned char>(text[pos])) &&
               text[pos] != '\n') {
            ++pos;
        }

        return true;

    case Kind::Newline:
        if (pos >= text.size() || text[pos] != '\n')
            return false;

        ++pos;
        return true;

    case Kind::Identifier:
        return match_identifier(text, pos);

    case Kind::MangledIdentifier:
        return match_identifier_mangled(text, pos);

    case Kind::Type:
        return match_type(text, pos);

    case Kind::Number:
        return match_number(text, pos);

    case Kind::String:
        return match_quoted(text, pos, '"');

    case Kind::Character:
        return match_quoted(text, pos, '\'');

    case Kind::AnyChar:
        if (pos >= text.size())
            return false;

        ++pos;
        return true;

    case Kind::Any:
        pos = text.size();
        return true;

    case Kind::Group:
        return false;
    }

    return false;
}

bool match_tokens(
    const tc::Source& source,
    const std::vector<Token>& tokens,
    size_t index,
    size_t& pos
) {
    if (index == tokens.size())
        return true;

    const Token& token = tokens[index];

    if (token.kind == Kind::Group) {
        size_t next = pos;

        if (!match_tokens(source, token.group, 0, next))
            return false;

        return match_tokens(source, tokens, index + 1, next)
            ? (pos = next, true)
            : false;
    }

    return match_token(source, token, pos) &&
           match_tokens(source, tokens, index + 1, pos);
}

tc::Opt<tc::Pattern> tc::Pattern::parse(const tc::StrView& pattern) noexcept {
    Pattern result;

    std::vector<std::vector<Token>*> stack;
    stack.push_back(&result.tokens);

    for (size_t i = 0; i < pattern.size();) {

        // %% -> literal %
        if (pattern[i] == '%' &&
            i + 1 < pattern.size() &&
            pattern[i + 1] == '%') {

            stack.back()->push_back({
                Kind::Literal,
                "%",
                {}
            });

            i += 2;
            continue;
        }

        // %( -> begin group
        if (pattern[i] == '%' &&
            i + 1 < pattern.size() &&
            pattern[i + 1] == '(') {

            stack.push_back(new std::vector<Token>());
            i += 2;
            continue;
        }

        // %) -> end group
        if (pattern[i] == '%' &&
            i + 1 < pattern.size() &&
            pattern[i + 1] == ')') {

            if (stack.size() == 1)
                return tc::NullOpt;

            std::vector<Token>* group = stack.back();
            stack.pop_back();

            stack.back()->push_back({
                Kind::Group,
                {},
                std::move(*group)
            });

            delete group;
            i += 2;
            continue;
        }

        // %... -> repeat last group
        if (pattern[i] == '%' &&
            i + 3 <= pattern.size() &&
            pattern.substr(i, 4) == "%...") {

            if (stack.back()->empty())
                return tc::NullOpt;

            Token& last = stack.back()->back();

            if (last.kind != Kind::Group)
                return tc::NullOpt;

            Token repeated = last;
            stack.back()->push_back(std::move(repeated));

            i += 4;
            continue;
        }

        // %keyword
        if (pattern[i] == '%') {
            size_t begin = i + 1;
            size_t end = begin;

            while (end < pattern.size()) {
                unsigned char c =
                    static_cast<unsigned char>(pattern[end]);

                if (!std::isalnum(c) && pattern[end] != '_')
                    break;

                ++end;
            }

            if (begin == end)
                return tc::NullOpt;

            tc::StrView name = pattern.substr(begin, end - begin);
            Kind kind = pattern_kind(name);

            if (kind == Kind::Literal)
                return tc::NullOpt;

            stack.back()->push_back({
                kind,
                {},
                {}
            });

            i = end;
            continue;
        }

        // Literal
        size_t begin = i;

        while (i < pattern.size() && pattern[i] != '%')
            ++i;

        stack.back()->push_back({
            Kind::Literal,
            tc::String(pattern.substr(begin, i - begin)),
            {}
        });
    }

    // Unclosed %( ... %)
    if (stack.size() != 1)
        return tc::NullOpt;

    return result;
}

tc::Position tc::Source::find_pattern(
    const tc::StrView& pattern,
    size_t start,
    tc::MatchOptions opts
) const noexcept {
    tc::Opt<tc::Pattern> parsed = tc::Pattern::parse(pattern);

    if (!parsed)
        return tc::Position::npos;

    for (size_t i = start; i <= source.size(); ++i) {
        if (!(opts & tc::MatchOptions::AcceptComment) &&
            is_inside_comment(i)) {
            continue;
        }

        if ((opts & tc::MatchOptions::StrSafe) &&
            is_inside_string(i)) {
            continue;
        }

        size_t pos = i;

        if (match_tokens(*this, parsed->get_tokens(), 0, pos)) {

            if (opts & tc::MatchOptions::WholeWord) {
                bool left =
                    i == 0 || !is_word_char(i - 1);

                bool right =
                    pos >= source.size() ||
                    !is_word_char(pos);

                if (!left || !right)
                    continue;
            }

            return position(i);
        }
    }

    return tc::Position::npos;
}

tc::Opt<tc::Ref<tc::Source>> tc::Modules::get(const tc::StrView& name) noexcept {
    for (auto& [key, s] : mods) {
        if (key == name)
            return tc::MakeRef(s);
    }

    return tc::NullOpt;
}


void tc::Modules::insert(tc::Source s) noexcept {
    mods.emplace(s.name, std::move(s));
}


const tc::MatchOptions tc::MatchOptions::None =
    tc::CArray<bool, tc::MatchOptions::optionsno>{0, 0, 0, 0};

const tc::MatchOptions tc::MatchOptions::WholeWord =
    tc::CArray<bool, tc::MatchOptions::optionsno>{0, 0, 0, 1};

const tc::MatchOptions tc::MatchOptions::AcceptComment =
    tc::CArray<bool, tc::MatchOptions::optionsno>{0, 0, 1, 0};

const tc::MatchOptions tc::MatchOptions::WholeLine =
    tc::CArray<bool, tc::MatchOptions::optionsno>{0, 1, 0, 0};
const tc::MatchOptions tc::MatchOptions::StrSafe = 
    tc::CArray<bool, tc::MatchOptions::optionsno>{1, 0, 0, 0};

static inline std::bitset<tc::MatchOptions::optionsno>
cpy(const tc::CArray<bool, tc::MatchOptions::optionsno>& arr) {
    std::bitset<tc::MatchOptions::optionsno> val;

    for (size_t i = 0; i < tc::MatchOptions::optionsno; ++i)
        val[i] = arr[i];

    return val;
}


tc::MatchOptions::MatchOptions(
    const tc::CArray<bool, tc::MatchOptions::optionsno>& arr
) noexcept : val(cpy(arr)) {}


tc::MatchOptions tc::MatchOptions::operator|(
    const MatchOptions& other
) const noexcept {
    return MatchOptions(val | other.val);
}

tc::MatchOptions& tc::MatchOptions::operator|=(const tc::MatchOptions& other) noexcept {
    if (*this == other)
        return *this;
    *this = *this | other;
    return *this;
}

bool tc::MatchOptions::operator&(
    const MatchOptions& other
) const noexcept {
    return (val & other.val) == other.val;
}

std::ostream& operator<<(std::ostream& os, tc::Position p) {
    os << p.line << ':' << p.column;
    return os;
}