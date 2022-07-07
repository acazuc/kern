#include "fs/vfs.h"
#include "dev/pit/pit.h"
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/sys.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

static int verify_userdata(const void *ptr, size_t count)
{
	(void)ptr;
	(void)count;
	return 1;
}

static int verify_userstr(const char *str)
{
	(void)str;
	return 1;
}

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
	if (!verify_userdata(data, count))
		return -EFAULT;
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	if (!file->op || !file->op->read)
		return -EINVAL; /* XXX */
	return file->op->read(file, data, count);
}

static int sys_write(int fd, const void *data, size_t count)
{
	if (!verify_userdata(data, count))
		return -EFAULT;
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	if (!file->op || !file->op->write)
		return -EINVAL; /* XXX */
	return file->op->write(file, data, count);
}

static int sys_open(const char *path, int flags, mode_t mode)
{
	(void)mode;
	(void)flags;
	if (!verify_userstr(path))
		return -EFAULT;
	struct fs_node *node;
	int ret = vfs_getnode(NULL, path, &node);
	if (ret)
		return -ret;
	struct file *file = malloc(sizeof(*file), 0);
	if (file == NULL)
	{
		fs_node_decref(node);
		return -ENOMEM;
	}
	if (node->fop && node->fop->open)
	{
		ret = node->fop->open(file, node);
		if (ret)
		{
			fs_node_decref(node);
			free(file);
			return -ret;
		}
	}
	file->op = node->fop;
	file->node = node;
	file->off = 0;
	struct filedesc *fd = realloc(curproc->files, sizeof(*curproc->files) * (curproc->files_nb + 1), 0);
	if (!fd)
	{
		if (file->op && file->op->close)
			file->op->close(file);
		free(file); /* XXX: deref */
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
	struct file *file = curproc->files[fd].file;
	if (!file)
		return -EBADF;
	if (file->op && file->op->close)
		file->op->close(file);
	free(file); /* XXX: decref */
	return -ENOSYS;
}

static int sys_time(time_t *tloc)
{
	if (tloc && !verify_userdata(tloc, sizeof(*tloc)))
		return -EFAULT;
	struct timespec ts;
	pit_gettime(&ts);
	time_t t = ts.tv_sec * 10000000000 + ts.tv_nsec;
	if (tloc)
		*tloc = t;
	return t;
}

static int sys_getpid()
{
	return curproc->pid;
}

static int sys_getuid()
{
	return curproc->uid;
}

static int sys_getgid()
{
	return curproc->gid;
}

static int sys_setuid(uid_t uid)
{
	if (!curproc->uid || !curproc->euid)
	{
		curproc->uid = uid;
		curproc->euid = uid;
		curproc->suid = uid;
		return 0;
	}
	if (uid != curproc->uid && uid != curproc->euid && uid != curproc->suid)
		return -EPERM;
	curproc->uid = uid;
	return 0;
}

static int sys_setgid(gid_t gid)
{
	if (!curproc->uid || !curproc->euid)
	{
		curproc->gid = gid;
		curproc->egid = gid;
		curproc->sgid = gid;
		return 0;
	}
	if (gid != curproc->gid && gid != curproc->egid && gid != curproc->sgid)
		return -EPERM;
	curproc->gid = gid;
	return 0;
}

static int sys_geteuid()
{
	return curproc->euid;
}

static int sys_getegid()
{
	return curproc->egid;
}

static int sys_setpgid(pid_t pid, pid_t pgid)
{
	/* XXX */
	(void)pid;
	(void)pgid;
	return -ENOSYS;
}

static int sys_getppid()
{
	return curproc->ppid;
}

static int sys_getpgrp()
{
	return curproc->pgrp;
}

static int sys_setsid()
{
	/* XXX */
	return -ENOSYS;
}

static int sys_setreuid(uid_t ruid, uid_t euid)
{
	/* XXX */
	if (ruid != (uid_t)-1)
		curproc->uid = ruid;
	if (euid != (uid_t)-1)
		curproc->euid = euid;
	return 0;
}

static int sys_setregid(gid_t rgid, gid_t egid)
{
	/* XXX */
	if (rgid != (gid_t)-1)
		curproc->gid = rgid;
	if (egid != (gid_t)-1)
		curproc->egid = egid;
	return 0;
}

static int sys_getgroups(int size, gid_t *list)
{
	/* XXX */
	if (size < 0)
		return -EINVAL;
	if (!verify_userdata(list, size * sizeof(*list)))
		return -EFAULT;
	return -ENOSYS;
}

static int sys_setgroups(size_t size, const gid_t *list)
{
	/* XXX */
	if (!verify_userdata(list, size * sizeof(*list)))
		return -EFAULT;
	return -ENOSYS;
}

static int sys_setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
	/* XXX */
	if (ruid != (uid_t)-1)
		curproc->uid = ruid;
	if (euid != (uid_t)-1)
		curproc->euid = euid;
	if (suid != (uid_t)-1)
		curproc->suid = suid;
	return 0;
}

static int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
	/* XXX */
	if (!verify_userdata(ruid, sizeof(*ruid))
	 || !verify_userdata(euid, sizeof(*euid))
	 || !verify_userdata(suid, sizeof(*suid)))
		return -EINVAL;
	*ruid = curproc->uid;
	*euid = curproc->euid;
	*suid = curproc->suid;
	return 0;
}

static int sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	/* XXX */
	if (rgid != (gid_t)-1)
		curproc->gid = rgid;
	if (egid != (gid_t)-1)
		curproc->egid = egid;
	if (sgid != (gid_t)-1)
		curproc->sgid = sgid;
	return 0;
}

static int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
	/* XXX */
	if (!verify_userdata(rgid, sizeof(*rgid))
	 || !verify_userdata(egid, sizeof(*egid))
	 || !verify_userdata(sgid, sizeof(*sgid)))
		return -EINVAL;
	*rgid = curproc->gid;
	*egid = curproc->egid;
	*sgid = curproc->sgid;
	return 0;
}

static int sys_getpgid()
{
	return curproc->pgid;
}

static int sys_ioctl(int fd, unsigned long req, intptr_t data)
{
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	if (!file->op || !file->op->ioctl)
		return -EINVAL; /* XXX */
	return file->op->ioctl(file, req, data);
}

static int sys_stat(const char *pathname, struct stat *statbuf)
{
	if (!verify_userstr(pathname) || !verify_userdata(statbuf, sizeof(*statbuf)))
		return -EFAULT;
	struct fs_node *node;
	int ret = vfs_getnode(NULL, pathname, &node);
	if (ret)
		return -ret;
	statbuf->st_dev = node->sb->dev;
	statbuf->st_ino = node->ino;
	statbuf->st_mode = node->mode;
	statbuf->st_nlink = node->nlink;
	statbuf->st_uid = node->uid;
	statbuf->st_gid = node->gid;
	statbuf->st_rdev = node->rdev;
	statbuf->st_size = node->size;
	statbuf->st_blksize = node->blksize;
	statbuf->st_blocks = node->blocks;
	statbuf->st_atim = node->atime;
	statbuf->st_mtim = node->mtime;
	statbuf->st_ctim = node->ctime;
	fs_node_decref(node);
	return 0;
}

static int sys_fstat(int fd, struct stat *statbuf)
{
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	if (!file->op || !file->op->read)
		return -EINVAL; /* XXX */
	struct fs_node *node = file->node;
	if (!node)
		return -EINVAL;
	statbuf->st_dev = node->sb->dev;
	statbuf->st_ino = node->ino;
	statbuf->st_mode = node->mode;
	statbuf->st_nlink = node->nlink;
	statbuf->st_uid = node->uid;
	statbuf->st_gid = node->gid;
	statbuf->st_rdev = node->rdev;
	statbuf->st_size = node->size;
	statbuf->st_blksize = node->blksize;
	statbuf->st_blocks = node->blocks;
	statbuf->st_atim = node->atime;
	statbuf->st_mtim = node->mtime;
	statbuf->st_ctim = node->ctime;
	fs_node_decref(node);
	return 0;
}

struct getdents_ctx
{
	struct fs_readdir_ctx readdir_ctx;
	int res;
	struct sys_dirent *dirp;
	size_t count;
	size_t off;
};

static int getdents_fn(struct fs_readdir_ctx *ctx, const char *name, uint32_t namelen, off_t off, ino_t ino, uint32_t type)
{
	struct getdents_ctx *getdents_ctx = ctx->userptr;
	uint32_t entry_size = sizeof(struct sys_dirent) + namelen + 1;
	if (entry_size > getdents_ctx->count)
		return EINVAL;
	getdents_ctx->dirp->ino = ino;
	getdents_ctx->dirp->off = getdents_ctx->off;
	getdents_ctx->dirp->reclen = entry_size;
	getdents_ctx->dirp->type = type;
	memcpy(getdents_ctx->dirp->name, name, namelen);
	getdents_ctx->dirp->name[namelen] = 0;
	getdents_ctx->count -= entry_size;
	getdents_ctx->dirp = (struct sys_dirent*)((char*)getdents_ctx->dirp + entry_size);
	getdents_ctx->off += entry_size;
	return 0;
}

static int sys_getdents(int fd, struct sys_dirent *dirp, size_t count)
{
	if (!verify_userdata(dirp, count))
		return -EFAULT;
	if (fd < 0 || (size_t)fd >= curproc->files_nb)
		return -EBADF;
	struct file *file = curproc->files[fd].file;
	struct fs_node *node = file->node;
	if (!node)
		return -EINVAL; /* XXX */
	if (!node->op || !node->op->readdir)
		return -EINVAL; /* XXX */
	struct getdents_ctx getdents_ctx;
	struct fs_readdir_ctx readdir_ctx;
	readdir_ctx.fn = getdents_fn;
	readdir_ctx.off = file->off;
	readdir_ctx.userptr = &getdents_ctx;
	getdents_ctx.res = 0;
	getdents_ctx.dirp = dirp;
	getdents_ctx.count = count;
	getdents_ctx.off = 0;
	int res = node->op->readdir(node, &readdir_ctx);
	file->off = readdir_ctx.off;
	if (res < 0)
		return res;
	if (getdents_ctx.res)
		return -getdents_ctx.res;
	return getdents_ctx.off;
};

static int (*g_syscalls[])() =
{
	[SYS_EXIT]      = sys_exit,
	[SYS_FORK]      = sys_fork,
	[SYS_READ]      = sys_read,
	[SYS_WRITE]     = sys_write,
	[SYS_OPEN]      = sys_open,
	[SYS_CLOSE]     = sys_close,
	[SYS_TIME]      = sys_time,
	[SYS_GETPID]    = sys_getpid,
	[SYS_GETUID]    = sys_getuid,
	[SYS_GETGID]    = sys_getgid,
	[SYS_SETUID]    = sys_setuid,
	[SYS_SETGID]    = sys_setgid,
	[SYS_GETEUID]   = sys_geteuid,
	[SYS_GETEGID]   = sys_getegid,
	[SYS_SETPGID]   = sys_setpgid,
	[SYS_GETPPID]   = sys_getppid,
	[SYS_GETPGRP]   = sys_getpgrp,
	[SYS_SETSID]    = sys_setsid,
	[SYS_SETREUID]  = sys_setreuid,
	[SYS_SETREGID]  = sys_setregid,
	[SYS_GETGROUPS] = sys_getgroups,
	[SYS_SETGROUPS] = sys_setgroups,
	[SYS_SETRESUID] = sys_setresuid,
	[SYS_GETRESUID] = sys_getresuid,
	[SYS_SETRESGID] = sys_setresgid,
	[SYS_GETRESGID] = sys_getresgid,
	[SYS_GETPGID]   = sys_getpgid,
	[SYS_IOCTL]     = sys_ioctl,
	[SYS_STAT]      = sys_stat,
	[SYS_FSTAT]     = sys_fstat,
	[SYS_GETDENTS]  = sys_getdents,
};

void call_sys(const struct int_ctx *ctx)
{
	uint32_t id = ctx->frame.eax;
	uint32_t ret;
	if (id >= sizeof(g_syscalls) / sizeof(*g_syscalls))
	{
		ret = -ENOSYS;
		goto end;
	}
	if (!g_syscalls[id])
	{
		ret = -ENOSYS;
		goto end;
	}
	ret = g_syscalls[id](ctx->frame.ebx, ctx->frame.ecx, ctx->frame.edx, ctx->frame.esi, ctx->frame.edi, ctx->frame.ebp);
end:
	curthread->frame.eax = ret;
}
