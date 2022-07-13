#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

static int32_t errno;
static int g_fd;

#define TRANSFORM_ERRNO(ret) \
do \
{ \
	if ((ret) < 0) \
	{ \
		errno = -(ret); \
		ret = -1; \
	} \
} while (0)

int32_t syscall(uint32_t id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);

static int exit(int error_code)
{
	int32_t ret = syscall(SYS_EXIT, error_code, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int write(int fd, const void *buffer, size_t count)
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

static int open(const char *path, int flags, ...)
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

int main(int ac, char **av, char **ev)
{
	g_fd = open("/dev/tty0", O_RDONLY);
	if (g_fd < 0)
		return 1;
	write(g_fd, "ld.so\n", 6);
	return 0;
}

void _start(void)
{
	exit(main(0, NULL, NULL));
}
