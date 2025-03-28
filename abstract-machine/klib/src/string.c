#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

char *strcpy(char *dst, const char *src) {
    char *original_dst = dst;
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
    return original_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *original_dest = dst;
    while (n > 0 && *src != '\0') {
        *dst = *src;
        dst++;
        src++;
        n--;
    }
    while (n > 0) {
        *dst = '\0';
        dst++;
        n--;
    }
    return original_dest;
}

char *strcat(char *dst, const char *src) {
    char *ptr = dst;
    while (*ptr != '\0') {
        ptr++;
    }
    while (*src != '\0') {
        *ptr = *src;
        ptr++;
        src++;
    }
    // 在新字符串的末尾添加字符串结束符
    *ptr = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (*s1 - *s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0) {
        if (*s1 != *s2) {
            return (*(unsigned char *)s1 - *(unsigned char *)s2);
        } else if (*s1 == '\0') {
            return 0;
        }
        s1++;
        s2++;
        n--;
    }
    return 0;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
    char       *d = (char *)dst;
    const char *s = (const char *)src;

    if (d < s) {
        // 从前往后复制
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        // 从后往前复制，处理重叠情况
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }

    return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
    char       *d = (char *)out;
    const char *s = (const char *)in;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (p1[i] < p2[i]) ? -1 : 1;
        }
    }
    return 0;
}

#endif
