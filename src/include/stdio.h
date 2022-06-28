#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

int vprintf(const char *fmt, va_list va_arg);
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vsnprintf(char *d, size_t n, const char *fmt, va_list va_arg);
int snprintf(char *d, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

#endif
