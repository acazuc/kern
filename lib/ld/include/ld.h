#ifndef LD_H

#define SYS_EXIT      1
#define SYS_FORK      2
#define SYS_READ      3
#define SYS_WRITE     4
#define SYS_OPEN      5
#define SYS_CLOSE     6

#define SYS_EXECVE    11

#define SYS_TIME      13

#define SYS_STAT      18
#define SYS_LSEEK     19
#define SYS_GETPID    20

#define SYS_FSTAT     28

#define SYS_IOCTL     54

#define SYS_SETPGID   57

#define SYS_GETPPID   64
#define SYS_GETPGRP   65
#define SYS_SETSID    66

#define SYS_MMAP      90

#define SYS_GETPGID   132

#define SYS_GETDENTS  145

#define SYS_GETUID    199
#define SYS_GETGID    200
#define SYS_GETEUID   201
#define SYS_GETEGID   202
#define SYS_SETREUID  203
#define SYS_SETREGID  204
#define SYS_GETGROUPS 205
#define SYS_SETGROUPS 206

#define SYS_SETRESUID 208
#define SYS_GETRESUID 209
#define SYS_SETRESGID 210
#define SYS_GETRESGID 211

#define SYS_SETUID    213
#define SYS_SETGID    214

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

typedef char *va_list;

#define va_rsize(T) ((sizeof(T) + 3) & ~3)
#define va_start(ap, lastarg) (ap = ((char*)&lastarg + va_rsize(lastarg)))
#define va_end(ap)
#define va_arg(ap, T) (ap += va_rsize(T), *((T*)(ap - va_rsize(T))))

#define NULL ((void*)0)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long int uintptr_t;
typedef long int intptr_t;
typedef long long intmax_t;
typedef unsigned long long uintmax_t;
typedef long ptrdiff_t;

typedef uint32_t size_t;
typedef int32_t ssize_t;

typedef int64_t time_t;
typedef int32_t off_t;
typedef uint32_t ino_t;
typedef uint32_t mode_t;
typedef int32_t uid_t;
typedef int32_t gid_t;
typedef int32_t pid_t;
typedef uint32_t dev_t;
typedef uint32_t nlink_t;
typedef int32_t blksize_t;
typedef int32_t blkcnt_t;
typedef uint32_t pri_t;
typedef int64_t clock_t;
typedef int32_t id_t;

#define INT64_MIN ((-9223372036854775807LL) - 1LL)
#define INT64_MAX 9223372036854775807LL

#define LLONG_MIN INT64_MIN
#define LLONG_MAX INT64_MAX

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define PAGE_SIZE 4096
#define MAXPATHLEN 1024

#else
# error "unknown arch"
#endif

int exit(int error_code);
ssize_t read(int fd, void *buffer, size_t count);
ssize_t write(int fd, const void *buffer, size_t count);
int open(const char *path, int flags, ...);
int close(int fd);
off_t lseek(int fd, off_t off, int whence);
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);

size_t strlen(const char *s);
void *memset(void *d, int c, size_t len);
size_t strlcpy(char *d, const char *s, size_t n);
char *strcpy(char *d, const char *s);
int strcmp(const char *s1, const char *s2);
void puts(const char *s);
void putx(long long n);
void puti(long long n);

extern int errno;
extern int g_stdout;

#endif
