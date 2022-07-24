#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>

DIR *opendir(const char *name)
{
	DIR *dir = malloc(sizeof(*dir));
	if (!dir)
		return NULL;
	dir->fd = open(name, O_RDONLY);
	if (dir->fd == -1)
	{
		free(dir);
		return NULL;
	}
	dir->buf_pos = 0;
	dir->buf_end = 0;
	return dir;
}

DIR *fdopendir(int fd)
{
	DIR *dir = malloc(sizeof(*dir));
	if (!dir)
		return NULL;
	dir->fd = fd;
	dir->buf_pos = 0;
	dir->buf_end = 0;
	return dir;
}

struct dirent *readdir(DIR *dirp)
{
	if (dirp->dirent)
	{
		free(dirp->dirent);
		dirp->dirent = NULL;
	}
	if (dirp->buf_pos == dirp->buf_end)
	{
		dirp->buf_pos = 0;
		dirp->buf_end = 0;
		ssize_t res = getdents(dirp->fd, (struct sys_dirent*)dirp->buf, sizeof(dirp->buf));
		if (res <= 0)
			return NULL;
		dirp->buf_end = res;
	}
	struct sys_dirent *sysd = (struct sys_dirent*)&dirp->buf[dirp->buf_pos];
	size_t namesize = sysd->reclen - sizeof(*sysd);
	struct dirent *dirent = malloc(sizeof(*dirent) + namesize);
	if (!dirent)
		return NULL;
	dirent->d_ino = sysd->ino;
	dirent->d_off = sysd->off;
	dirent->d_reclen = sysd->reclen;
	dirent->d_type = sysd->type;
	memcpy(dirent->d_name, sysd->name, namesize);
	dirp->dirent = dirent;
	dirp->buf_pos += sysd->reclen;
	return dirent;
}

int closedir(DIR *dirp)
{
	if (!dirp)
		return 0;
	if (dirp->dirent)
	{
		free(dirp->dirent);
		dirp->dirent = NULL;
	}
	close(dirp->fd);
	free(dirp);
	return 0;
}
