#include <sys/sys.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>

typedef uint32_t mode_t;

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

uint8_t g_userland_stack[4096 * 4]; /* XXX: move to vmalloc_user allocation */

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
	int32_t ret = syscall(SYS_EXIT, error_code, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int fork()
{
	int32_t ret = syscall(SYS_FORK, 0, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int read(int fd, void *buffer, size_t count)
{
	int32_t ret = syscall(SYS_READ, fd, (uint32_t)buffer, count, 0, 0, 0);
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

static int close(int fd)
{
	int32_t ret = syscall(SYS_CLOSE, fd, 0, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
	return i;
}

static char *strchr(const char *s, int c)
{
	while (*s)
	{
		if (*s == (char)c)
			return (char*)s;
		s++;
	}
	if (!(char)c)
		return (char*)s;
	return NULL;
}

static int strcmp(const char *s1, const char *s2)
{
	size_t i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		i++;
	return (((uint8_t*)s1)[i] - ((uint8_t*)s2)[i]);
}

static void *memchr(const void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s)[i] == (uint8_t)c)
			return (uint8_t*)s + i;
	}
	return NULL;
}

static void *memcpy(void *d, const void *s, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		((uint8_t*)d)[i] = ((uint8_t*)s)[i];
	return d;
}

#define OUT(s) write(-2, s, sizeof(s))

static void exec_line(const char *line)
{
	write(0, "cmd: ", 5);
	write(0, line, strlen(line));
	write(0, "\n", 1);
}

void userland()
{
	OUT("userland\n");
	int fd = open("/dev/tty0", O_RDONLY);
	if (fd < 0)
	{
		OUT("failed to open /dev/tty0\n");
		goto loop;
	}
	for (size_t i = 0; i < 10; ++i)
	{
		char tmp[128] = "\e[0;30m0;30 \e[1;30m1;30 \e[0;40m0;40\e[0m \e[1;40m1;40\e[0m\n";
		tmp[5]  = '0' + i;
		tmp[10] = '0' + i;
		tmp[17] = '0' + i;
		tmp[22] = '0' + i;
		tmp[29] = '0' + i;
		tmp[34] = '0' + i;
		tmp[45] = '0' + i;
		tmp[50] = '0' + i;
		write(fd, tmp, 56);
	}
	char buf[78] = "";
	char line[78];
	size_t buf_pos = 0;
	while (1)
	{
		write(fd, "\e[0m$ ", 6);
		char *eol = NULL;
		do
		{
			int rd = read(fd, buf + buf_pos, sizeof(buf) - buf_pos - 1);
			if (rd < 0)
			{
				if (errno != EAGAIN)
					OUT("rd < 0");
				continue;
			}
			write(fd, buf + buf_pos, rd);
			buf_pos += rd;
			eol = memchr(buf, '\n', buf_pos);
		} while (!eol);
		size_t line_len = eol - buf;
		memcpy(line, buf, line_len);
		line[line_len] = '\0';
		memcpy(buf, eol + 1, buf_pos - line_len + 1);
		buf_pos -= line_len + 1;
		exec_line(line);
	}
	close(fd);
loop: goto loop;
}
