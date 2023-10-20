#include <include/types.h>
#include <include/stdargs.h>
#include <include/mem.h>

#define FMT_PUT(dst, len, c) { \
    if (!(len))                \
        goto end;              \
    *(dst)++ = (c);            \
    len--;                     \
}

#define IS_DIGIT(c) (c >= '0' && c <= '9')

const char *digits_upper = "0123456789ABCDEF";
const char *digits_lower = "0123456789abcdef";

char *num_fmt(long i, uint32_t base, uint32_t padding, char pad_with, bool upper, int len) {
    bool neg = i < 0;

    if (neg && base != 16) {
        i *= -1;
    }

    static char buf[100];
    memclear((void *) buf, 100);
    char *ptr = buf + 49;
    *ptr = '\0';

    const char *digits = upper ? digits_upper : digits_lower;

    do {
        *--ptr = digits[(uint64_t) i % base];
        if (padding)
            padding--;
        if (len > 0)
            len--;
    } while ((i = (uint64_t) i / base) != 0 && (len == -1 || len));

    while (padding) {
        *--ptr = pad_with;
        padding--;
    }

    if (neg && base != 16)
        *--ptr = '-';

    return ptr;
}

void vsnprintf(char *buf, uint64_t len, char *fmt, va_list arg) {
    uint64_t i;
    char *s;

    while (*fmt && len) {
        if (*fmt != '%') {
            *buf++ = *fmt;
            fmt++;
            continue;
        }

        fmt++;
        int padding = 0;
        char pad_with = ' ';
        bool wide = 0, upper = 0;

        if (*fmt == '0') {
            pad_with = '0';
            fmt++;
        }

        while (IS_DIGIT(*fmt)) {
            padding *= 10;
            padding += *fmt++ - '0';
        }

        while (*fmt == 'l') {
            wide = 1;
            fmt++;
        }

        upper = *fmt == 'X' || *fmt == 'P';

        switch (*fmt) {
            case 'c': {
                i = va_arg(arg, int);
                FMT_PUT(buf, len, i)
                break;
            }

            case 'd': {
                if (wide)
                    i = va_arg(arg, long int);
                else
                    i = va_arg(arg, int);

                char *c = num_fmt(i, 10, padding, pad_with, false, -1);
                while (*c) {
                    FMT_PUT(buf, len, *c);
                    c++;
                }
                break;
            }

            case 'u': {
                if (wide)
                    i = va_arg(arg, long int);
                else
                    i = va_arg(arg, int);

                char *c = num_fmt(i, 10, padding, pad_with, false, -1);
                while (*c) {
                    FMT_PUT(buf, len, *c);
                    c++;
                }
                break;
            }

            case 'o': {
                if (wide)
                    i = va_arg(arg, long int);
                else
                    i = va_arg(arg, int);

                char *c = num_fmt(i, 8, padding, pad_with, false, -1);
                while (*c) {
                    FMT_PUT(buf, len, *c);
                    c++;
                }
                break;
            }

            case 'X':
            case 'x': {
                if (wide)
                    i = va_arg(arg, long int);
                else
                    i = va_arg(arg, int);

                char *c = num_fmt(i, 16, padding, pad_with, upper, wide ? 16 : 8);
                while (*c) {
                    FMT_PUT(buf, len, *c);
                    c++;
                }
                break;
            }

            case 'P':
            case 'p': {
                i = (uint64_t)(va_arg(arg, void *));

                char *c = num_fmt(i, 16, padding, pad_with, upper, 16);
                while (*c) {
                    FMT_PUT(buf, len, *c);
                    c++;
                }
                break;
            }

            case 's': {
                s = va_arg(arg, char *);
                while (*s) {
                    FMT_PUT(buf, len, *s);
                    s++;
                }
                break;
            }

            case '%': {
                FMT_PUT(buf, len, '%');
                break;
            }
        }

        fmt++;
    }

    end:
    if (!len) {
        *buf++ = '.';
        *buf++ = '.';
        *buf++ = '.';
    }

    *buf++ = '\0';
}
