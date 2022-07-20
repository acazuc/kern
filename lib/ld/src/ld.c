#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <elf.h>

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR   (O_RDONLY | O_WRONLY)
#define O_APPEND (1 << 2)
#define O_ASYNC  (1 << 3)
#define O_CREAT  (1 << 4)
#define O_TRUNC  (1 << 5)

static int32_t errno;
int g_fd;

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

static int lseek(int fd, off_t off, int whence)
{
	int32_t ret = syscall(SYS_LSEEK, fd, off, whence, 0, 0, 0);
	TRANSFORM_ERRNO(ret);
	return ret;
}

static void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
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

static size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
	return i;
}

static void puts(const char *s)
{
	write(g_fd, s, strlen(s));
}

int main(int argc, char **argv, char **envp)
{
	if (argc < 1)
	{
		puts("no binary given\n");
		return 0;
	}
	int fd = open(argv[0], O_RDONLY);
	if (fd < 0)
	{
		puts("failed to open file\n");
		return 0;
	}
	off_t off = lseek(fd, 0, SEEK_END);
	if (off < 0)
	{
		puts("can't get file size\n");
		goto end;
	}
	size_t len = (size_t)off;
	if (lseek(fd, 0, SEEK_SET) < 0)
	{
		puts("can't reset file to beggining\n");
		goto end;
	}
	Elf32_Ehdr hdr;
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
	{
		puts("failed to read ehdr\n");
		goto end;
	}
	if (hdr.e_ident[EI_MAG0] != ELFMAG0
	 || hdr.e_ident[EI_MAG1] != ELFMAG1
	 || hdr.e_ident[EI_MAG2] != ELFMAG2
	 || hdr.e_ident[EI_MAG3] != ELFMAG3)
	{
		puts("invalid header magic\n");
		goto end;
	}
	if (hdr.e_ident[EI_CLASS] != ELFCLASS32)
	{
		puts("invalid class\n");
		goto end;
	}
	if (hdr.e_ident[EI_DATA] != ELFDATA2LSB)
	{
		puts("invalid data mode\n");
		goto end;
	}
	if (hdr.e_ident[EI_VERSION] != EV_CURRENT)
	{
		puts("invalid ident version\n");
		goto end;
	}
	if (hdr.e_type != ET_DYN)
	{
		puts("not dynamic\n");
		goto end;
	}
	if (hdr.e_machine != EM_386)
	{
		puts("not i386\n");
		goto end;
	}
	if (hdr.e_version != EV_CURRENT)
	{
		puts("invalid version\n");
		goto end;
	}
	if (hdr.e_shoff >= len)
	{
		puts("section header outside of file\n");
		goto end;
	}
	if (hdr.e_shentsize < sizeof(Elf32_Shdr))
	{
		puts("section entry size too low\n");
		goto end;
	}
	if (hdr.e_shoff + hdr.e_shentsize * hdr.e_shnum > len)
	{
		puts("section header end outside of file\n");
		goto end;
	}
	if (hdr.e_phoff >= len)
	{
		puts("program header outside of file\n");
		goto end;
	}
	if (hdr.e_phentsize < sizeof(Elf32_Phdr))
	{
		puts("program entry size too low\n");
		goto end;
	}
	if (hdr.e_phoff + hdr.e_phentsize * hdr.e_phnum > len)
	{
		puts("program header end outside of file\n");
		goto end;
	}

	//while (1);

end:
	close(fd);
	return 0;
}

void _start(int argc, char **argv, char **envp)
{
	g_fd = open("/dev/tty0", O_RDONLY);
	if (g_fd < 0)
		exit(1);
	write(g_fd, "ld.so\n", 6);
	int ret = main(argc, argv, envp);
	close(g_fd);
	exit(ret);
	while (1){}
}
