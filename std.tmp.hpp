#ifndef STD_______hc
#define STD_______hc
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <string>
namespace hc {
    using std::size_t, std::ptrdiff_t, std::intmax_t, std::uintmax_t;
    template <typename T>
    struct slice {
        T* ptr;
        size_t size;

        template <size_t N>
        slice(T (&arr)[N]) : ptr(arr), size(N) {}

        slice(T* p, size_t N) : ptr(p), size(N) {}

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

        slice(void* p, size_t N) : ptr(p), size(N) {}

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
            inline auto operator<=>(const __Byte& other) const = default;
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

}

extern "C" void _I3std6printf(const char* fmt, ...);
extern "C" void _I3std8printfln(const char* fmt, ...);
#endif