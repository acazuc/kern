#ifndef SYS_STD_H
#define SYS_STD_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

typedef uint64_t time_t;
typedef uint64_t off_t;
typedef uint64_t ino_t;
typedef uint32_t mode_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t pid_t;

#define DT_BLK     1
#define DT_CHR     2
#define DT_DIR     3
#define DT_FIFO    4
#define DT_LNK     5
#define DT_REG     6
#define DT_SOCK    7
#define DT_UNKNOWN 8

#define S_IXOTH (1 << 0)
#define S_IWOTH (1 << 1)
#define S_IROTH (1 << 2)
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH)
#define S_IXGRP (1 << 3)
#define S_IWGRP (1 << 4)
#define S_IRGRP (1 << 5)
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IXUSR (1 << 6)
#define S_IWUSR (1 << 7)
#define S_IRUSR (1 << 8)
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#define S_ISVTX (1 << 9)
#define S_ISGID (1 << 10)
#define S_ISUID (1 << 11)
#define S_IFMT (0xFF << 12)

#define S_IFBLK (DT_BLK << 12)
#define S_IFCHR (DT_CHR << 12)
#define S_IFDIR (DT_DIR << 12)
#define S_IFIFO (DT_FIFO << 12)
#define S_IFLNK (DT_LNK << 12)
#define S_IFREG (DT_REG << 12)
#define S_IFSOCK (DT_SOCK << 12)

#define S_ISBLK(mode) ((mode & S_IFMT) == S_IFBLK)
#define S_ISCHR(mode) ((mode & S_IFMT) == S_IFCHR)
#define S_ISDIR(mode) ((mode & S_IFMT) == S_IFDIR)
#define S_ISIFO(mode) ((mode & S_IFMT) == S_IFIFO)
#define S_ISLNK(mode) ((mode & S_IFMT) == S_IFLNK)
#define S_ISREG(mode) ((mode & S_IFMT) == S_IFREG)
#define S_ISSOCK(mode) ((mode & S_IFMT) == S_IFSOCK)

struct timespec
{
	time_t tv_sec;
	time_t tv_nsec;
};

void *memset(void *d, int c, size_t n);
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
char *strchrnul(const char *s, int c);
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

void *malloc(size_t size, uint32_t flags);
void free(void *ptr);
void *realloc(void *ptr, size_t size, uint32_t flags);
void show_alloc_mem(void);

#endif
