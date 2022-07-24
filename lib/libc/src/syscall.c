#include <sys/syscall.h>
#include <sys/types.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

int32_t syscall(uint32_t id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);

int errno;

#define TRANSFORM_ERRNO(ret) \
do \
{ \
	if ((ret) < 0) \
	{ \
		errno = -(ret); \
		ret = -1; \
	} \
} while (0)

static ssize_t basic_syscall(uint32_t id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6)
{
	ssize_t ret = syscall(id, arg1, arg2, arg3, arg4, arg5, arg6);
	TRANSFORM_ERRNO(ret);
	return ret;
}

void _exit(int status)
{
	(void)basic_syscall(SYS_EXIT, status, 0, 0, 0, 0, 0);
}

pid_t fork(void)
{
	return basic_syscall(SYS_FORK, 0, 0, 0, 0, 0, 0);
}

ssize_t read(int fd, void *buffer, size_t count)
{
	return basic_syscall(SYS_READ, fd, (uint32_t)buffer, count, 0, 0, 0);
}

ssize_t write(int fd, const void *buffer, size_t count)
{
	return basic_syscall(SYS_WRITE, fd, (uint32_t)buffer, count, 0, 0, 0);
}

int open(const char *path, int flags, ...)
{
	mode_t mode;
	va_list va_arg;
	va_start(va_arg, flags);
	if (flags & O_CREAT)
	{
		mode = va_arg(va_arg, mode_t);
	}
	else
	{
		mode = 0;
	}
	va_end(va_arg);
	return basic_syscall(SYS_OPEN, (uint32_t)path, flags, mode, 0, 0, 0);
}

int close(int fd)
{
	return basic_syscall(SYS_CLOSE, fd, 0, 0, 0, 0, 0);
}

int stat(const char *pathname, struct stat *statbuf)
{
	return basic_syscall(SYS_STAT, (intptr_t)pathname, (intptr_t)statbuf, 0, 0, 0, 0);
}

int lstat(const char *pathname, struct stat *statbuf)
{
	return basic_syscall(SYS_LSTAT, (intptr_t)pathname, (intptr_t)statbuf, 0, 0, 0, 0);
}

int getdents(int fd, struct sys_dirent *dirp, unsigned count)
{
	return basic_syscall(SYS_GETDENTS, fd, (intptr_t)dirp, count, 0, 0, 0);
}

pid_t getpid(void)
{
	return basic_syscall(SYS_GETPID, 0, 0, 0, 0, 0, 0);
}

pid_t getppid(void)
{
	return basic_syscall(SYS_GETPPID, 0, 0, 0, 0, 0, 0);
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	return basic_syscall(SYS_EXECVE, (uint32_t)pathname, (uint32_t)argv, (uint32_t)envp, 0, 0, 0);
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
	return basic_syscall(SYS_READLINK, (uint32_t)pathname, (uint32_t)buf, (uint32_t)bufsiz, 0, 0, 0);
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	void *ret = (void*)syscall(SYS_MMAP, (uint32_t)addr, len, prot, flags, fd, off);
	int32_t r = (int32_t)ret;
	if (r < 0 && r >= -1000) /* hack, consider maximum errno to be 1000 */
	{
		errno = -r;
		return (void*)-1;
	}
	return ret;
}

int munmap(void *addr, size_t len)
{
	return basic_syscall(SYS_MUNMAP, (uint32_t)addr, len, 0, 0, 0, 0);
}

int dup(int oldfd)
{
	return basic_syscall(SYS_DUP, oldfd, 0, 0, 0, 0, 0);
}

int dup2(int oldfd, int newfd)
{
	return basic_syscall(SYS_DUP2, oldfd, newfd, 0, 0, 0, 0);
}

off_t lseek(int fd, off_t offset, int whence)
{
	return basic_syscall(SYS_LSEEK, fd, offset, whence, 0, 0, 0);
}
