#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

typedef struct FILE FILE;

/* XXX: move to some internal file */
struct FILE
{
	int fd;
};

int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *fmt, va_list va_arg);
int snprintf(char *d, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char *d, size_t n, const char *fmt, va_list va_arg);
int fprintf(FILE *file, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int vfprintf(FILE *file, const char *fmt, va_list va_arg);

int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);

extern FILE *stdout;
extern FILE *stdin;
extern FILE *stderr;

#endif
