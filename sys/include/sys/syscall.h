#ifndef SYS_SYSCALL_H
#define SYS_SYSCALL_H

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

#define SYS_DUP       41

#define SYS_IOCTL     54

#define SYS_SETPGID   57

#define SYS_DUP2      63
#define SYS_GETPPID   64
#define SYS_GETPGRP   65
#define SYS_SETSID    66

#define SYS_LSTAT     84
#define SYS_READLINK  85

#define SYS_MMAP      90
#define SYS_MUNMAP    91

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


#endif
