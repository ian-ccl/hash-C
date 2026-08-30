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