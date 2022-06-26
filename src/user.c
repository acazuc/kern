#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

typedef uint32_t mode_t;

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

uint8_t g_userland_stack[4096 * 4];

int32_t errno;

int32_t syscall(uint32_t id, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6);

#define TRANSFORM_ERRNO(ret) \
do \
{ \
	if ((ret) < 0) \
	{ \
		errno = -(ret); \
		ret = -1; \
	} \
} while (0)

static int exit(int error_code)
{
	int32_t ret = syscall(0x1, error_code, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int fork()
{
	int32_t ret = syscall(0x2, 0, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int read(int fd, void *buffer, size_t count)
{
	int32_t ret = syscall(0x3, fd, (uint32_t)buffer, count, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int write(int fd, const void *buffer, size_t count)
{
	int32_t ret = syscall(0x4, fd, (uint32_t)buffer, count, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int sys_open(const char *path, int flags, mode_t mode)
{
	int32_t ret = syscall(0x5, (uint32_t)path, flags, mode, 0, 0, 0);
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

static int close(int fd)
{
	int32_t ret = syscall(0x6, fd, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

#define OUT(s) write(1, s, sizeof(s))

void userland()
{
	OUT("userland\n");
	OUT("tes\n");
	OUT("tes\n");
	OUT("tes\n");
	OUT("tes\n");
	OUT("tes\n");
	int ret = open("/dev/tty0", O_RDONLY);
	if (ret < 0)
		OUT("failed to open /dev/tty0\n");
	else if (ret > 0)
		OUT("ret > 0\n");
	else
		OUT("open ok\n");
loop: goto loop;
}
