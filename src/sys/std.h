#ifndef SYS_STD_H
#define SYS_STD_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

typedef uint64_t time_t;

struct timespec
{
	time_t tv_sec;
	time_t tv_nsec;
};

void *memset(void *d, int c, size_t n);
void bzero(void *d, size_t n);
void *memcpy(void *d, const void *s, size_t n);
void *memccpy(void *d, const void *s, int c, size_t n);
void *memmove(void *d, const void *s, size_t n);
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strcpy(char *d, const char *s);
char *strncpy(char *d, const char *s, size_t n);
size_t strlcpy(char *d, const char *s, size_t n);
char *strcat(char *d, const char *s);
char *strncat(char *d, const char *s, size_t n);
size_t strlcat(char *d, const char *s, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *s1, const char *s2);
char *strnstr(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

size_t wcslen(const wchar_t *ws);

int atoi(const char *s);
int atoin(const char *s, size_t n);
void lltoa(char *d, long long int n);
void ulltoa(char *d, unsigned long long int n);
void ulltoa_base(char *d, unsigned long long int n, const char *base);

int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isascii(int c);
int isprint(int c);
int isspace(int c);
int toupper(int c);
int tolower(int c);

int vprintf(const char *fmt, va_list va_arg);
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vsnprintf(char *d, size_t n, const char *fmt, va_list va_arg);
int snprintf(char *d, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

void panic();

#endif
