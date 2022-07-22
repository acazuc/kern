#ifndef SYS_TYPES_H
#define SYS_TYPES_H

#include <stdint.h>

#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

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

#define major(n) ((n) << 8)
#define minor(n) (n)
#define makedev(ma, mi) (major(ma) | minor(mi))

#else
# error "unknown arch"
#endif

#endif
