#ifndef TC_HPP
#define TC_HPP

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <optional>
#include <bitset>
#include <type_traits>
#include <cstddef>
#include <vector>
namespace tc {

    template <typename T = size_t>
    struct Range {
        T _begin;
        T _end;

        struct iter {
            T val;

            iter& operator++() noexcept {
                ++val;
                return *this;
            }

            iter operator++(int) noexcept {
                iter cpy = *this;
                ++val;
                return cpy;
            }

            iter& operator--() noexcept {
                --val;
                return *this;
            }

            iter operator--(int) noexcept {
                iter cpy = *this;
                --val;
                return cpy;
            }

            T operator*() const noexcept {
                return val;
            }

            bool operator==(const iter&) const = default;
            bool operator!=(const iter&) const = default;
        };

        iter begin() const noexcept {
            return {_begin};
        }

        iter end() const noexcept {
            return {_end};
        }
    };

    template <typename K, typename V>
    using Map = std::unordered_map<K, V>;
    template <typename T, typename Hash = std::hash<T>, typename Equal = std::equal_to<T>>
    using Set = std::unordered_set<T, Hash, Equal>;

    using String = std::string;
    using StrView = std::string_view;

    template <typename T>
    using Ref = std::reference_wrapper<T>;

    constexpr const std::nullopt_t NullOpt = std::nullopt;

    template <typename T>
    Ref<T> MakeRef(T& o) noexcept {
        return {o};
    }

    template <typename T>
    Ref<const T> MakeRef(const T& o) noexcept {
        return {o};
    }

    template <typename T>
    using Opt = std::optional<T>;

    template <typename T, size_t N>
    using CArray = T[N];


    class MatchOptions {
    public:
        static constexpr size_t optionsno = 4;

    private:
        std::bitset<optionsno> val;

        explicit MatchOptions(std::bitset<optionsno> v) noexcept : val(v) {}
        MatchOptions(const CArray<bool, optionsno>&) noexcept;

    public:
        MatchOptions(MatchOptions&&) = default;
        MatchOptions(const MatchOptions&) = default;

        MatchOptions operator|(const MatchOptions&) const noexcept;
        bool operator&(const MatchOptions&) const noexcept;
        bool operator==(const MatchOptions&) const noexcept = default;

        MatchOptions& operator=(const MatchOptions&) = default;
        MatchOptions& operator=(MatchOptions&&) = default;
        MatchOptions& operator|=(const MatchOptions&) noexcept;

        static const MatchOptions None;
        static const MatchOptions WholeWord;
        static const MatchOptions AcceptComment;
        static const MatchOptions WholeLine;
        static const MatchOptions StrSafe;
    };


    struct Source;

    struct Position {
        size_t line;
        size_t column;

        bool validate(const Source&) const noexcept;
        Opt<size_t> to_idx(const Source&) const noexcept;

        bool operator==(const Position&) const = default;

        static const Position npos;
    };

    struct SourcePosition : Position {
        Ref<const Source> src;
        inline SourcePosition(Position pos, Ref<const Source> src) : Position(pos), src(src) {}
        bool this_validate() const noexcept;
        Opt<size_t> this_to_idx() const noexcept;
        bool operator==(const SourcePosition&) const = default;
        bool operator==(const Position&) const;
        tc::SourcePosition& operator=(const Position& pos);
    };

    struct Source {
        String name;
        String source;

        Position position(size_t) const noexcept;
        Position position(size_t line, size_t column) const noexcept;
        bool replace(const StrView& text, const StrView& replacement, size_t startidxfind, MatchOptions options) noexcept;

        size_t replace_all(const StrView& text, const StrView& replacement, MatchOptions options) noexcept;

        Opt<size_t> index(Position) const noexcept;

        Opt<Ref<const char>> at(Position) const noexcept;

        bool replace(Position, size_t, const StrView&) noexcept;

        Position find(const StrView&, size_t, MatchOptions) const noexcept;
        Position find_pattern(const StrView&, size_t, MatchOptions) const noexcept;
        bool is_word_char(size_t) const noexcept;
        bool is_word_char(Position) const noexcept;

        bool is_inside_comment(size_t) const noexcept;
        bool is_inside_comment(Position) const noexcept;

        bool is_inside_string(size_t) const noexcept;
        bool is_inside_string(Position) const noexcept;
    };

    class Pattern {
        public:
            enum class Kind {
                Literal,
                Whitespace,        // %w
                MaybeWhitespace,  // %mw
                Newline,           // %n
                Identifier,        // %ident
                MangledIdentifier, // %identm
                Type,              // %type
                Number,            // %num
                String,            // %str
                Character,         // %char
                AnyChar,           // %?
                Any,               // %any
                Group              // %(...)
            };

            struct Token {
                Kind kind;
                String text;
                std::vector<Token> group;
            };

        private:
            std::vector<Token> tokens;

        public:
            static Opt<Pattern> parse(const StrView&) noexcept;

            const std::vector<Token>& get_tokens() const noexcept {
                return tokens;
            }
        };


    class Modules {
        Map<String, Source> mods;

    public:
        Opt<Ref<Source>> get(const StrView&) noexcept;
        void insert(Source) noexcept;
    };

    


    template<typename... Args>
    std::enable_if_t<(std::is_same_v<Args, MatchOptions> && ...), MatchOptions>
    join(const Args&... args) noexcept {
        return (MatchOptions::None | ... | args);
    }

}

std::ostream& operator<<(std::ostream& os, tc::Position p);

#endif