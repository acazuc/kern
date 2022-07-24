#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF -1

typedef struct FILE FILE;
typedef off_t fpos_t;

int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *fmt, va_list va_arg);
int snprintf(char *d, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int vsnprintf(char *d, size_t n, const char *fmt, va_list va_arg);
int fprintf(FILE *file, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int vfprintf(FILE *file, const char *fmt, va_list va_arg);

int fputc(int c, FILE *fp);
int fputs(const char *s, FILE *fp);
int putc(int c, FILE *fp);
int putchar(int c);
int puts(const char *s);

void perror(const char *s);

FILE *fopen(const char *pathname, const char *mode);
FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *pathname, const char *mode, FILE *fp);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
int fseek(FILE *fp, long offset, int whence);
long ftell(FILE *fp);
void rewind(FILE *fp);
int fgetpos(FILE *fp, fpos_t *pos);
int fsetpos(FILE *fp, const fpos_t *pos);
int fflush(FILE *fp);
void clearerr(FILE *fp);
int feof(FILE *fp);
int ferror(FILE *fp);
int fileno(FILE *fp);
int fclose(FILE *fp);

extern FILE *stdout;
extern FILE *stdin;
extern FILE *stderr;

#endif
