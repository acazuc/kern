#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stdint.h>

typedef uint64_t time_t;
typedef uint64_t off_t;
typedef uint64_t ino_t;
typedef uint32_t mode_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint32_t pid_t;
typedef uint32_t dev_t;
typedef uint32_t nlink_t;
typedef uint32_t blksize_t;
typedef uint32_t blkcnt_t;
typedef uint32_t pri_t;

#define major(n) ((n) << 8)
#define minor(n) (n)
#define makedev(ma, mi) (major(ma) | minor(mi))

#endif
