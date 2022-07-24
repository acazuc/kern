#include <ld.h>

#define TRANSFORM_ERRNO(ret) \
do \
{ \
	if ((ret) < 0) \
	{ \
		errno = -(ret); \
		ret = -1; \
	} \
} while (0)

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

int errno;

ssize_t syscall(uint32_t id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);

int exit(int error_code)
{
	int32_t ret = syscall(SYS_EXIT, error_code, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

ssize_t read(int fd, void *buffer, size_t count)
{
	int32_t ret = syscall(SYS_READ, fd, (uint32_t)buffer, count, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

ssize_t write(int fd, const void *buffer, size_t count)
{
	int32_t ret = syscall(SYS_WRITE, fd, (uint32_t)buffer, count, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int sys_open(const char *path, int flags, mode_t mode)
{
	int32_t ret = syscall(SYS_OPEN, (uint32_t)path, flags, mode, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
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
	return sys_open(path, flags, mode);
}

int close(int fd)
{
	int32_t ret = syscall(SYS_CLOSE, fd, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

off_t lseek(int fd, off_t off, int whence)
{
	int32_t ret = syscall(SYS_LSEEK, fd, off, whence, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
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
