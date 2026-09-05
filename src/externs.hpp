#ifndef EXTERNS_HPP
#define EXTERNS_HPP
#include "types.hpp"
#include <unordered_set>
#include <utility>
#include <vector>
extern ArgsInfo info;
extern args_t args;
extern program_t program;
extern std::unordered_set<std::string> keywords;
extern std::unordered_set<std::string> types;
extern std::vector<std::string> tmp_files;
extern std::vector<todo> todo_list;
#pragma region Std
const std::string standard = 
R"(
#ifndef STD_______hc
#define STD_______hc
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <functional>
namespace hc {
    using std::size_t, std::ptrdiff_t, std::intmax_t, std::uintmax_t;
    template <typename T>
    struct slice {
        T* ptr;
        size_t size;

        template <size_t N>
        slice(T (&arr)[N]) : ptr(arr), size(N) {}

        slice(T* p = nullptr, size_t N = 0) : ptr(p), size(N) {}

        slice(const slice& s) : ptr(s.ptr), size(s.size) {}

        T& operator[](size_t i) {
            return ptr[i];
        }

        const T& operator[](size_t i) const {
            return ptr[i];
        }
    };

    template <>
    struct slice<void> {
        void* ptr;
        size_t size;

        template <typename T, size_t N>
        slice(T (&arr)[N]) : ptr(arr), size(N) {}

        slice(void* p = nullptr, size_t N = 0) : ptr(p), size(N) {}

        slice(const slice& s) : ptr(s.ptr), size(s.size) {}

        template <typename T>
        operator slice<T>() const {
            return {(T*)ptr, size};
        }
    };
    #define oper(op) inline __Byte operator op(const __Byte&other) const { return this->val op other.val; }
    #define unoper(op) inline __Byte operator op() const { return op this->val; }
    #define opers() oper(+) oper(-) oper(*) oper(/) oper(%) \
                    oper(&) oper(|) oper(<<) oper(>>) unoper(~) oper(^)
    #define defoper(op) inline __Byte& operator op##=(const __Byte&other) {this->val op##= other.val; return *this;};
    #define defopers() defoper(+) defoper(-) defoper(*) defoper(/) defoper(%) \
                    defoper(&) defoper(|) defoper(<<) defoper(>>) defoper(^)
    template <typename T>
    class __Byte {
        T val;
        public:
            __Byte(const T& value) : val(value) {}
            __Byte(const __Byte& value) = default;
            inline explicit operator T() const {
                return val;
            }
            opers()
            defopers()
            inline explicit operator bool() const {
                return val != 0;
            }
            inline bool operator==(const __Byte& other) const {
                return this->val == other.val;
            }
            inline bool operator!=(const __Byte& other) const {
                return this->val != other.val;
            }
            #if __cplusplus >= 202002L
            inline auto operator<=>(const __Byte& other) const = default;
            #endif
            inline bool operator&&(const __Byte& other) const {
                return this->val && other.val;
            }
            inline bool operator||(const __Byte& other) const {
                return this->val || other.val;
            }
            inline bool operator!() const {
                return !this->val;
            }
    };
    #undef oper
    #undef opers
    #undef unoper
    #undef defoper
    #undef defopers
    using ibyte = __Byte<signed char>;
    using ubyte = __Byte<unsigned char>;

    struct NonCopyable {
        NonCopyable(const NonCopyable&) = delete;
    };
    
    struct Defer {
        std::function<void()> func;
        inline Defer(std::function<void()> f) : func(f) {}
        inline ~Defer() { func(); }
    };
}

extern "C" void _I3std6printf(const char* fmt, ...);
extern "C" void _I3std8printfln(const char* fmt, ...);
#endif
)";
#pragma endregion




#pragma region StdDef
const std::string standarddefs = 
R"(
#include "std.tmp.hpp"

static void print_i(ptrdiff_t x) {
    printf("%td", x);
}

static void print_u(size_t x) {
    printf("%zu", x);
}

static void print_f(double x) {
    printf("%f", x);
}

static void print_mi(intmax_t x) {
    printf("%jd", x);
}

static void print_mu(uintmax_t x) {
    printf("%ju", x);
}

static void print_mf(long double x) {
    printf("%Lf", x);
}

static void print_s(hc::slice<char> s) {
    printf("%.*s", (int)s.size, s.ptr);
}

static void print_c(char c) {
    putchar((unsigned char)c);
}

static void print_ub(hc::ubyte x) {
    printf("%u", (unsigned)(unsigned char)x);
}

static void print_ib(hc::ibyte x) {
    printf("%d", (int)(signed char)x);
}

static void print_b(bool b) {
    puts(b ? "true" : "false");
}

void printdef(const char* fmt, va_list ap) {
    for (size_t i = 0; fmt[i] != '\0'; ++i) {
        if (fmt[i] != '%') {
            putchar((unsigned char)fmt[i]);
            continue;
        }

        ++i;

        switch (fmt[i]) {
            case 'i':
                print_i(va_arg(ap, ptrdiff_t));
                break;

            case 'u':
                print_u(va_arg(ap, size_t));
                break;

            case 'f':
                print_f(va_arg(ap, double));
                break;

            case 'm':
                switch (fmt[++i]) {
                    case 'i':
                        print_mi(va_arg(ap, intmax_t));
                        break;

                    case 'u':
                        print_mu(va_arg(ap, uintmax_t));
                        break;

                    case 'f':
                        print_mf(va_arg(ap, long double));
                        break;

                    default:
                        // formato inválido
                        break;
                }
                break;

            case 's':
                print_s(va_arg(ap, hc::slice<char>));
                break;

            case 'c':
                print_c((char)va_arg(ap, int));
                break;

            case 'b':
                if (fmt[i+1] == 'i') {
                    print_ib(va_arg(ap, hc::ibyte));
                    i++;
                    break;
                } else if (fmt[i+1] == 'u') {
                    print_ub(va_arg(ap, hc::ubyte));
                    i++;
                    break;
                }
                print_b((bool)va_arg(ap, int));

            case '%':
            default:
                putchar('%');
                break;
        }
    }
}

extern "C" void _I3std6printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    printdef(fmt, ap);

    va_end(ap);
}

extern "C" void _I3std8printfln(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    printdef(fmt, ap);
    print_c('\n');
    va_end(ap);
}
)";
#pragma endregion
#endif