#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
    char    buf[256]; // 临时缓冲区
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    for (int i = 0; i < len; i++) {
        putch(buf[i]); 
    }
    return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    return vsnprintf(out, (size_t)-1, fmt, ap); // 无缓冲区限制
}

int sprintf(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(out, (size_t)-1, fmt, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    char       *p = out;
    const char *fmt_ptr = fmt;
    size_t      written = 0;

    while (*fmt_ptr && written < n) {
        if (*fmt_ptr != '%') {
            *p++ = *fmt_ptr++;
            written++;
            continue;
        }

        fmt_ptr++; // 跳过'%'
        char spec = *fmt_ptr++;
        switch (spec) {
            case 'd': {
                int  num = va_arg(ap, int);
                char buffer[16];
                int  len = 0;
                if (num < 0) {
                    if (written + 1 >= n)
                        break;
                    *p++ = '-';
                    written++;
                    num = -num;
                }
                do {
                    buffer[len++] = '0' + (num % 10);
                    num /= 10;
                } while (num > 0);
                while (len-- > 0 && written < n) {
                    *p++ = buffer[len];
                    written++;
                }
                break;
            }
            case 'u': {
                unsigned int num = va_arg(ap, unsigned int);
                char         buffer[16];
                int          len = 0;
                do {
                    buffer[len++] = '0' + (num % 10);
                    num /= 10;
                } while (num > 0);
                while (len-- > 0 && written < n) {
                    *p++ = buffer[len];
                    written++;
                }
                break;
            }
            case 's': {
                const char *str = va_arg(ap, const char *);
                while (*str && written < n) {
                    *p++ = *str++;
                    written++;
                }
                break;
            }
            case 'c': {
                char c = (char)va_arg(ap, int);
                if (written < n) {
                    *p++ = c;
                    written++;
                }
                break;
            }
            default:
                if (written < n) {
                    *p++ = spec; // 未识别的格式符原样输出
                    written++;
                }
                break;
        }
    }

    if (n > 0)
        *p = '\0'; // 确保终止符
    return written;
}

#endif
