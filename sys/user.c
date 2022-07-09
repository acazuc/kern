#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

typedef uint32_t mode_t;

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

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

static int stat(const char *pathname, struct stat *statbuf)
{
	int32_t ret = syscall(SYS_STAT, (intptr_t)pathname, (intptr_t)statbuf, 0, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int getdents(int fd, struct sys_dirent *dirp, unsigned count)
{
	int32_t ret = syscall(SYS_GETDENTS, fd, (intptr_t)dirp, count, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static int getpid(void)
{
	int32_t ret = syscall(SYS_GETPID, 0, 0, 0, 0, 0, 0);
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

static int strncmp(const char *s1, const char *s2, size_t n)
{
	size_t i;
	for (i = 0; i < n && s1[i] && s2[i]; ++i)
	{
		uint8_t d = ((uint8_t*)s1)[i] - ((uint8_t*)s2)[i];
		if (d)
			return d;
	}
	if (i == n)
		return 0;
	return ((uint8_t*)s1)[i] - ((uint8_t*)s2)[i];
}

static int g_fd;

static void exec_line(const char *line)
{
	if (!strcmp(line, "colors"))
	{
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
			write(g_fd, tmp, 56);
		}
		return;
	}
	if (!strncmp(line, "stat", 4))
	{
		line += 4;
		if (*line == ' ')
			line++;
		struct stat st;
		int res = stat(line, &st);
		if (res)
		{
			write(g_fd, "can't stat\n", 11);
			return;
		}
		char str[] = "mode: 000\n";
		str[6] = '0' + ((st.st_mode >> 6) & 0x7);
		str[7] = '0' + ((st.st_mode >> 3) & 0x7);
		str[8] = '0' + ((st.st_mode >> 0) & 0x7);
		write(g_fd, str, 10);
		return;
	}
	if (!strncmp(line, "ls", 2))
	{
		char buffer[128];
		line += 2;
		if (*line == ' ')
			line++;
		int fd = open(line, O_RDONLY);
		if (fd < 0)
		{
			write(g_fd, "can't open file\n", 16);
			return;
		}
		int res;
		write(g_fd, "files: ", 7);
		do
		{
			res = getdents(fd, (struct sys_dirent*)buffer, sizeof(buffer));
			if (res < 0)
			{
				write(g_fd, "can't getdents\n", 15);
				break;
			}
			for (int i = 0; i < res;)
			{
				struct sys_dirent *dirent = (struct sys_dirent*)&buffer[i];
				write(g_fd, dirent->name, strlen(dirent->name));
				write(g_fd, " ", 1);
				i += dirent->reclen;
			}
		} while (res);
		write(g_fd, "\n", 1);
		close(fd);
		return;
	}
	if (!strcmp(line, "fork"))
	{
		int pid = fork();
		if (pid < 0)
		{
			write(g_fd, "fork failed\n", 12);
			return;
		}
		if (pid)
		{
			write(g_fd, "parent\n", 7);
			return;
		}
		write(g_fd, "child\n", 6);
		while (1)
			exit(0);
	}
	if (!strcmp(line, "pid"))
	{
		int pid = getpid();
		char c[2] = {pid + '0', '\n'};
		write(g_fd, c, 2);
		return;
	}
	write(g_fd, "unknown command: ", 17);
	write(g_fd, line, strlen(line));
	write(g_fd, "\n", 1);
}

void userland()
{
	g_fd = open("/dev/tty0", O_RDONLY);
	if (g_fd < 0)
		goto loop;
	write(g_fd, "userland\n", 9);
	char buf[78] = "";
	char line[78];
	size_t buf_pos = 0;
	while (1)
	{
		write(g_fd, "\e[0m$ ", 6);
		char *eol = NULL;
		do
		{
			int rd = read(g_fd, buf + buf_pos, sizeof(buf) - buf_pos - 1);
			if (rd < 0)
			{
				if (errno != EAGAIN)
					write(g_fd, "rd < 0\n", 7);
				continue;
			}
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
	close(g_fd);
loop: goto loop;
}
