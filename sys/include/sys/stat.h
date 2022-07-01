#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <time.h>

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

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
};

#endif
