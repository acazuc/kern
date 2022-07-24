#ifndef DIRENT_H
#define DIRENT_H

#include <sys/types.h>
#include <stddef.h>

typedef struct DIR DIR;

struct dirent
{
	ino_t d_ino;
	off_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

DIR *opendir(const char *name);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif
