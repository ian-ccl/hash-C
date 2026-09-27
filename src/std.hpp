#ifndef STD_HPP
#define STD_HPP

constexpr const char* stdhpp = R"(
#ifndef STD_______hc
#define STD_______hc
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <functional>
#include <array>
#include <cstddef>
#include <type_traits>
namespace hc {
    using std::size_t, std::ptrdiff_t, std::intmax_t, std::uintmax_t, std::array;

    template <typename T>
    struct constant_struct {
        using type = T const;
    };
    template <typename T>
    struct constant_struct<T const> {};

    template <typename T>
    using constant = typename constant_struct<T>::type;

    template <typename T>
    struct slice {
        T* ptr;
        size_t size;

        template <size_t N>
        slice(T (&arr)[N]) : ptr(arr), size(N) {}

        slice(T* p = nullptr, size_t N = 0) : ptr(p), size(N) {}
        slice& operator=(slice&&) = default;
        slice(slice&&) = default;

        T& operator[](size_t i) {
            return ptr[i];
        }

        const T& operator[](size_t i) const {
            return ptr[i];
        }
        operator slice<void>() const;
    };

    template <>
    struct slice<void> {
        void* ptr;
        size_t size;

        slice(void* p, size_t N) : ptr(p), size(N) {}
    };
    template <typename T>
    slice<T>::operator slice<void>() const {
        return {static_cast<void*>(ptr), size};
    }

    template <typename T>
    struct ptr {
        T* _p;

        T& operator[](size_t i) {
            return _p[i];
        }

        const T& operator[](size_t i) const {
            return _p[i];
        }

        ptr(T* const &p) : _p(p) {}

        ptr& operator=(const ptr&) = default;
        ptr& operator=(ptr&&) = default;
        ptr(const ptr&) = default;
        ptr(ptr&&) = default;

        T& operator*() const {
            return *_p;
        }

        operator ptr<void>() const;
    };
    template <>
    struct ptr<void> {
        void* _p;
        template <typename Type>
        ptr(Type* const &p) : _p((void*)p) {}

        ptr& operator=(const ptr&) = default;
        ptr& operator=(ptr&&) = default;
        ptr(const ptr&) = default;
        ptr(ptr&&) = default;

        template <typename T>
        operator ptr<T>() const {
            return {(T*)_p};
        }
    };

    template <typename T>
    ptr<T>::operator ptr<void>() const {
        return {static_cast<void*>(_p)};
    }

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
        NonCopyable() {}
    };
    template <typename F>
    struct Defer {
        F func;
        inline Defer(F&& f) : func(f) {}
        inline ~Defer() { func(); }
    };
}

extern "C" void _I3std6printf(const char* fmt, ...);
extern "C" void _I3std8printfln(const char* fmt, ...);
extern "C" void _I3std6readln(hc::slice<char> buf, hc::ptr<hc::ptrdiff_t> errcode);
extern "C" hc::ptr<void> _I3std5alloc(size_t size);
extern "C" hc::slice<void> _I3std5alloc4list(size_t elemsize, size_t elems);
extern "C" void _I3std4free(hc::ptr<void> p);
extern "C" void _I3std4free4list(hc::slice<void> p);
#endif
)";

#endif