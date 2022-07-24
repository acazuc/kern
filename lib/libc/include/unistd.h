#ifndef UNISTD_H
#define UNISTD_H

#include <sys/types.h>
#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

struct stat;
struct sys_dirent;

ssize_t write(int fd, const void *buffer, size_t count);
ssize_t read(int fd, void *buffer, size_t count);
pid_t fork(void);
void _exit(int status);
int close(int fd);
int stat(const char *pathname, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
int getdents(int fd, struct sys_dirent *dirp, unsigned count);
pid_t getpid(void);
pid_t getppid(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);

int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#endif
