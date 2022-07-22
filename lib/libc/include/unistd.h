#ifndef UNISTD_H
#define UNISTD_H

#include <sys/types.h>
#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct stat;
struct sys_dirent;

ssize_t write(int fd, const void *buffer, size_t count);
ssize_t read(int fd, void *buffer, size_t count);
pid_t fork(void);
void _exit(int status);
int close(int fd);
int stat(const char *pathname, struct stat *statbuf);
int getdents(int fd, struct sys_dirent *dirp, unsigned count);
pid_t getpid(void);
pid_t getppid(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);

#endif
