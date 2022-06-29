#include "fs/vfs.h"
#include <sys/file.h>
#include <sys/proc.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static int sys_exit(int code)
{
	(void)code;
	return -ENOSYS;
}

static int sys_fork()
{
	return -ENOSYS;
}

static int sys_read(int fd, void *data, size_t count)
{
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	if (!file->op || !file->op->read)
		return -EINVAL; /* XXX */
	size_t rd;
	int res = file->op->read(file, data, count, &rd);
	if (res)
		return -res;
	return rd;
}

static int sys_write(int fd, const void *data, size_t count)
{
	if (fd == -2)
	{
		for (size_t i = 0; i < count; ++i)
			printf("%c", ((char*)data)[i]);
		return count;
	}
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	if (!file->op || !file->op->write)
		return -EINVAL; /* XXX */
	size_t written;
	int res = file->op->write(file, data, count, &written);
	if (res)
		return -res;
	return written;
}

static int sys_open(const char *path, int flags, mode_t mode)
{
	(void)mode;
	(void)flags;
	struct fs_node *node;
	int ret = vfs_getnode(NULL, path, &node);
	if (ret)
		return -ret;
	struct file *file = malloc(sizeof(*file), 0);
	if (file == NULL)
	{
		/* XXX: decref node */
		return -ENOMEM;
	}
	file->op = node->fop;
	file->node = node;
	struct filedesc *fd = realloc(curproc->files, sizeof(*curproc->files) * (curproc->files_nb + 1), 0);
	if (!fd)
	{
		/* XXX: decref node */
		free(file);
		return -ENOMEM;
	}
	curproc->files = fd;
	curproc->files[curproc->files_nb].file = file;
	curproc->files_nb++;
	return curproc->files_nb - 1;
}

static int sys_close(int fd)
{
	if (fd < 0 || (unsigned)fd >= curproc->files_nb)
		return -EBADF;
	return -ENOSYS;
}

static int (*g_syscalls[])() =
{
	NULL,
	sys_exit,
	sys_fork,
	sys_read,
	sys_write,
	sys_open,
	sys_close,
};

uint32_t call_sys(uint32_t *args)
{
	uint32_t id = args[0];
	if (id >= sizeof(g_syscalls) / sizeof(*g_syscalls))
		return -ENOSYS;
	if (!g_syscalls[id])
		return -ENOSYS;
	return g_syscalls[id](args[1], args[2], args[3], args[4], args[5], args[6]);
}
