#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

struct FILE
{
	int fd;
	char buffer[4096];
	size_t buf_pos;
	int eof;
	int err;
};

FILE g_stdin = {.fd = 0};
FILE g_stdout = {.fd = 1};
FILE g_stderr = {.fd = 2};

FILE *stdin = &g_stdin;
FILE *stdout = &g_stdout;
FILE *stderr = &g_stderr;

int fputc(int c, FILE *fp)
{
	unsigned char cc = c;
	write(fp->fd, &cc, 1);
	return cc;
}

int fputs(const char *s, FILE *fp)
{
	size_t len = strlen(s);
	write(fp->fd, s, len);
	return len + 1;
}

int putc(int c, FILE *fp)
{
	return fputc(c, fp);
}

int putchar(int c)
{
	return fputc(c, stdout);
}

int puts(const char *s)
{
	size_t len = strlen(s);
	write(stdout->fd, s, len);
	putchar('\n');
	return len + 1;
}

void perror(const char *s)
{
	fprintf(stderr, "%s: %s\n", s, strerror(errno));
}

static FILE *mkfp(void)
{
	FILE *fp = malloc(sizeof(*fp));
	if (!fp)
		return NULL;
	fp->buf_pos = 0;
	fp->eof = 0;
	fp->err = 0;
	fp->fd = -1;
	return fp;
}

FILE *fopen(const char *pathname, const char *mode)
{
	FILE *fp = mkfp();
	if (!fp)
		return NULL;
	int flags = 0; /* XXX */
	fp->fd = open(pathname, flags, 0666);
	if (fp->fd == -1)
	{
		free(fp);
		return NULL;
	}
	return fp;
}

FILE *fdopen(int fd, const char *mode)
{
	FILE *fp = mkfp();
	if (!fp)
		return NULL;
	fp->fd = fd;
	/* XXX mode */
	return fp;
}

FILE *freopen(const char *pathname, const char *mode, FILE *fp)
{
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	ssize_t res = read(fp->fd, ptr, size * nmemb);
	if (res == -1)
	{
		fp->err = 1;
		return 0;
	}
	if (res == 0)
	{
		fp->eof = 0;
		return 0;
	}
	return res;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
	ssize_t res = write(fp->fd, ptr, size * nmemb);
	if (res == -1)
	{
		fp->err = 1;
		return 0;
	}
	return res;
}

int fseek(FILE *fp, long offset, int whence)
{
	if (lseek(fp->fd, offset, whence) == -1)
		return -1;
	return 0;
}

long ftell(FILE *fp)
{
	long res = lseek(fp->fd, 0, SEEK_CUR);
	if (res == -1)
		return -1;
	return res;
}

void rewind(FILE *fp)
{
	fseek(fp, 0, SEEK_SET);
}

int fgetpos(FILE *fp, fpos_t *pos)
{
	long res = lseek(fp->fd, 0, SEEK_CUR);
	if (res == -1)
		return -1;
	*pos = res;
	return 0;
}

int fsetpos(FILE *fp, const fpos_t *pos)
{
	long res = lseek(fp->fd, *pos, SEEK_SET);
	if (res == -1)
		return -1;
	return 0;
}

int fflush(FILE *fp)
{
}

void clearerr(FILE *fp)
{
	fp->eof = 0;
	fp->err = 0;
}

int feof(FILE *fp)
{
	return fp->eof;
}

int ferror(FILE *fp)
{
	return fp->err;
}

int fileno(FILE *fp)
{
	return fp->fd;
}

int fclose(FILE *fp)
{
	int err = 0;
	if (fflush(fp))
		err = EOF;
	if (close(fp->fd))
		err = EOF;
	free(fp);
	return err;
}
