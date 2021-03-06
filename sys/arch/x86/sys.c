#include "dev/pit/pit.h"
#include "x86.h"

#include <sys/syscall.h>
#include <sys/sched.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/pcpu.h>
#include <inttypes.h>
#include <sys/vmm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <vfs.h>

static int verify_userdata(const void *ptr, size_t count)
{
	if (!ptr)
		return 0;
	/* XXX */
	(void)ptr;
	(void)count;
	return 1;
}

static int verify_userstr(const char *str)
{
	if (!str)
		return 0;
	/* XXX */
	(void)str;
	return 1;
}

static int verify_userstra(const char *const *array)
{
	if (!array)
		return 0;
	/* XXX */
	(void)array;
	return 1;
}

static int sys_exit(int code)
{
	struct thread *oldthread = curthread;
	sched_switch(idlethread);
	oldthread->state = THREAD_ZOMBIE;
	/* XXX mark zombie, delete proc ? */
	return curthread->tf.eax; /* hack: return current eax value to not change anything */
}

static int sys_fork()
{
	struct thread *thread = proc_fork(curthread);
	if (!thread)
		return -EAGAIN;
	thread->tf.eax = 0;
	return thread->proc->pid;
}

static int sys_read(int fd, void *data, size_t count)
{
	if (!verify_userdata(data, count))
		return -EFAULT;
	if (fd < 0 || (size_t)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
	if (!file->op || !file->op->read)
		return -EINVAL; /* XXX */
	return file->op->read(file, data, count);
}

static int sys_write(int fd, const void *data, size_t count)
{
	if (!verify_userdata(data, count))
		return -EFAULT;
	if (fd < 0 || (size_t)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
	if (!file->op || !file->op->write)
		return -EINVAL; /* XXX */
	return file->op->write(file, data, count);
}

static int reserve_fd(struct proc *proc)
{
	for (size_t i = 0; i < proc->files_nb; ++i)
	{
		if (!proc->files[i])
			return i;
	}
	struct file **fd = realloc(proc->files, sizeof(*proc->files) * (proc->files_nb + 1), M_ZERO);
	if (!fd)
		return -1;
	proc->files = fd;
	proc->files_nb++;
	return proc->files_nb - 1;
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
	file->refcount = 1;
	if (node->fop && node->fop->open)
	{
		ret = node->fop->open(file, node);
		if (ret)
		{
			file_decref(file);
			return -ret;
		}
	}
	file->op = node->fop;
	file->node = node;
	file->off = 0;
	int fd = reserve_fd(curthread->proc);
	if (fd == -1)
	{
		if (file->op && file->op->close)
			file->op->close(file);
		file_decref(file);
		return -EMFILE;
	}
	curthread->proc->files[fd] = file;
	return fd;
}

static int sys_close(int fd)
{
	if (fd < 0 || (unsigned)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
	if (file->op && file->op->close)
		file->op->close(file);
	curthread->proc->files[fd] = NULL;
	file_decref(file);
	return 0;
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
	return curthread->proc->pid;
}

static int sys_getuid()
{
	return curthread->proc->uid;
}

static int sys_getgid()
{
	return curthread->proc->gid;
}

static int sys_setuid(uid_t uid)
{
	if (!curthread->proc->uid || !curthread->proc->euid)
	{
		curthread->proc->uid = uid;
		curthread->proc->euid = uid;
		curthread->proc->suid = uid;
		return 0;
	}
	if (uid != curthread->proc->uid && uid != curthread->proc->euid && uid != curthread->proc->suid)
		return -EPERM;
	curthread->proc->uid = uid;
	return 0;
}

static int sys_setgid(gid_t gid)
{
	if (!curthread->proc->uid || !curthread->proc->euid)
	{
		curthread->proc->gid = gid;
		curthread->proc->egid = gid;
		curthread->proc->sgid = gid;
		return 0;
	}
	if (gid != curthread->proc->gid && gid != curthread->proc->egid && gid != curthread->proc->sgid)
		return -EPERM;
	curthread->proc->gid = gid;
	return 0;
}

static int sys_geteuid()
{
	return curthread->proc->euid;
}

static int sys_getegid()
{
	return curthread->proc->egid;
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
	return curthread->proc->ppid;
}

static int sys_getpgrp()
{
	return curthread->proc->pgrp;
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
		curthread->proc->uid = ruid;
	if (euid != (uid_t)-1)
		curthread->proc->euid = euid;
	return 0;
}

static int sys_setregid(gid_t rgid, gid_t egid)
{
	/* XXX */
	if (rgid != (gid_t)-1)
		curthread->proc->gid = rgid;
	if (egid != (gid_t)-1)
		curthread->proc->egid = egid;
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
		curthread->proc->uid = ruid;
	if (euid != (uid_t)-1)
		curthread->proc->euid = euid;
	if (suid != (uid_t)-1)
		curthread->proc->suid = suid;
	return 0;
}

static int sys_getresuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
	/* XXX */
	if (!verify_userdata(ruid, sizeof(*ruid))
	 || !verify_userdata(euid, sizeof(*euid))
	 || !verify_userdata(suid, sizeof(*suid)))
		return -EINVAL;
	*ruid = curthread->proc->uid;
	*euid = curthread->proc->euid;
	*suid = curthread->proc->suid;
	return 0;
}

static int sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	/* XXX */
	if (rgid != (gid_t)-1)
		curthread->proc->gid = rgid;
	if (egid != (gid_t)-1)
		curthread->proc->egid = egid;
	if (sgid != (gid_t)-1)
		curthread->proc->sgid = sgid;
	return 0;
}

static int sys_getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid)
{
	/* XXX */
	if (!verify_userdata(rgid, sizeof(*rgid))
	 || !verify_userdata(egid, sizeof(*egid))
	 || !verify_userdata(sgid, sizeof(*sgid)))
		return -EINVAL;
	*rgid = curthread->proc->gid;
	*egid = curthread->proc->egid;
	*sgid = curthread->proc->sgid;
	return 0;
}

static int sys_getpgid()
{
	return curthread->proc->pgid;
}

static int sys_ioctl(int fd, unsigned long req, intptr_t data)
{
	if (fd < 0 || (size_t)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
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

static int sys_lstat(const char *pathname, struct stat *statbuf)
{
	if (!verify_userstr(pathname) || !verify_userdata(statbuf, sizeof(*statbuf)))
		return -EFAULT;
	struct fs_node *node;
	int ret = vfs_getnode(NULL, pathname, &node); /* XXX noderef flag */
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
	if (fd < 0 || (size_t)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
	struct fs_node *node = file->node;
	if (!node)
		return -EINVAL; /* XXX */
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
	if (fd < 0 || (size_t)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
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
}

static int sys_execve(const char *pathname, const char * const *argv, const char * const *envp)
{
	if (!verify_userstr(pathname) || !verify_userstra(argv) || !verify_userstra(envp))
		return -EFAULT;
	/* XXX: argv, envpn */
	struct fs_node *node;
	int ret = vfs_getnode(NULL, pathname, &node);
	if (ret)
		return -ret;
	struct file file;
	file.refcount = 1;
	file.op = node->fop;
	file.node = node;
	file.off = 0;
	if (file.op && file.op->open)
	{
		ret = file.op->open(&file, node);
		if (ret)
		{
			file_decref(&file);
			return -ret;
		}
	}
	ret = proc_execve(curthread, &file, argv, envp);
	file_decref(&file);
	return -ret;
}

static int sys_lseek(int fd, off_t off, int whence)
{
	if (fd < 0 || (unsigned)fd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[fd];
	if (!file)
		return -EBADF;
	if (!file->op || !file->op->seek)
		return -EINVAL; /* XXX */
	return file->op->seek(file, off, whence);
}

static void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	struct file *file;
	if (!(flags & MAP_ANONYMOUS))
	{
		if (fd < 0 || (unsigned)fd >= curthread->proc->files_nb)
			return (void*)-EBADF;
		file = curthread->proc->files[fd];
		if (!file)
			return (void*)-EBADF;
		if (!file->op || !file->op->mmap)
			return (void*)-EINVAL; /* XXX */
	}
	/* XXX use flags, prot, addr */
	addr = vmalloc_user(curthread->proc->vmm_ctx, addr, len);
	if (!addr)
	{
		addr = vmalloc_user(curthread->proc->vmm_ctx, NULL, len);
		if (!addr)
			return (void*)-ENOMEM;
	}
	if (!(flags & MAP_ANONYMOUS))
	{
		int ret = file->op->mmap(file, curthread->proc->vmm_ctx, addr, off, len);
		if (ret)
			return (void*)-ret;
	}
	return addr;
}

static int sys_munmap(void *addr, size_t len)
{
	/* XXX */
	return -ENOSYS;
}

static int sys_readlink(const char *pathname, char *buf, size_t bufsize)
{
	if (!verify_userstr(pathname) || !verify_userdata(buf, bufsize))
		return -EFAULT;
	return -ENOSYS;
}

static int sys_dup(int oldfd)
{
	if (oldfd < 0 || (unsigned)oldfd >= curthread->proc->files_nb)
		return -EBADF;
	struct file *file = curthread->proc->files[oldfd];
	if (!file)
		return -EBADF;
	int newfd = reserve_fd(curthread->proc);
	if (newfd == -1)
		return -EMFILE;
	file_incref(file);
	curthread->proc->files[newfd] = file;
	return newfd;
}

static int sys_dup2(int oldfd, int newfd)
{
	if (oldfd < 0 || (unsigned)oldfd >= curthread->proc->files_nb)
		return -EBADF;
	if (newfd < 0)
		return -EBADF;
	struct file *file = curthread->proc->files[oldfd];
	if (!file)
		return -EBADF;
	if (oldfd == newfd)
		return newfd;
	if ((unsigned)newfd < curthread->proc->files_nb)
	{
		struct file *prv = curthread->proc->files[newfd];
		if (prv)
			file_decref(prv);
		file_incref(file);
		curthread->proc->files[newfd] = file;
		return newfd;
	}
	struct file **fd = realloc(curthread->proc->files, sizeof(*curthread->proc->files) * (newfd + 1), M_ZERO);
	if (!fd)
		return -ENOMEM;
	curthread->proc->files = fd;
	curthread->proc->files_nb = newfd + 1;
	file_incref(file);
	curthread->proc->files[newfd] = file;
	return newfd;
}

static int (*g_syscalls[])() =
{
	[SYS_EXIT]      = (void*)sys_exit,
	[SYS_FORK]      = (void*)sys_fork,
	[SYS_READ]      = (void*)sys_read,
	[SYS_WRITE]     = (void*)sys_write,
	[SYS_OPEN]      = (void*)sys_open,
	[SYS_CLOSE]     = (void*)sys_close,
	[SYS_TIME]      = (void*)sys_time,
	[SYS_GETPID]    = (void*)sys_getpid,
	[SYS_GETUID]    = (void*)sys_getuid,
	[SYS_GETGID]    = (void*)sys_getgid,
	[SYS_SETUID]    = (void*)sys_setuid,
	[SYS_SETGID]    = (void*)sys_setgid,
	[SYS_GETEUID]   = (void*)sys_geteuid,
	[SYS_GETEGID]   = (void*)sys_getegid,
	[SYS_SETPGID]   = (void*)sys_setpgid,
	[SYS_GETPPID]   = (void*)sys_getppid,
	[SYS_GETPGRP]   = (void*)sys_getpgrp,
	[SYS_SETSID]    = (void*)sys_setsid,
	[SYS_SETREUID]  = (void*)sys_setreuid,
	[SYS_SETREGID]  = (void*)sys_setregid,
	[SYS_GETGROUPS] = (void*)sys_getgroups,
	[SYS_SETGROUPS] = (void*)sys_setgroups,
	[SYS_SETRESUID] = (void*)sys_setresuid,
	[SYS_GETRESUID] = (void*)sys_getresuid,
	[SYS_SETRESGID] = (void*)sys_setresgid,
	[SYS_GETRESGID] = (void*)sys_getresgid,
	[SYS_GETPGID]   = (void*)sys_getpgid,
	[SYS_IOCTL]     = (void*)sys_ioctl,
	[SYS_STAT]      = (void*)sys_stat,
	[SYS_FSTAT]     = (void*)sys_fstat,
	[SYS_LSTAT]     = (void*)sys_lstat,
	[SYS_GETDENTS]  = (void*)sys_getdents,
	[SYS_EXECVE]    = (void*)sys_execve,
	[SYS_LSEEK]     = (void*)sys_lseek,
	[SYS_MMAP]      = (void*)sys_mmap,
	[SYS_MUNMAP]    = (void*)sys_munmap,
	[SYS_READLINK]  = (void*)sys_readlink,
	[SYS_DUP]       = (void*)sys_dup,
	[SYS_DUP2]      = (void*)sys_dup2,
};

void call_sys(const struct int_ctx *ctx)
{
	uint32_t id = ctx->trapframe.eax;
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
	ret = g_syscalls[id](ctx->trapframe.ebx, ctx->trapframe.ecx, ctx->trapframe.edx, ctx->trapframe.esi, ctx->trapframe.edi, ctx->trapframe.ebp);
end:
	curthread->tf.eax = ret;
}
