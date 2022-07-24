#include <unistd.h>
#include <string.h>
#include <stdio.h>

FILE g_stdin = {0};
FILE g_stdout = {1};
FILE g_stderr = {2};

FILE *stdin = &g_stdin;
FILE *stdout = &g_stdout;
FILE *stderr = &g_stderr;

int fputc(int c, FILE *stream)
{
	unsigned char cc = c;
	write(stream->fd, &cc, 1);
	return cc;
}

int fputs(const char *s, FILE *stream)
{
	size_t len = strlen(s);
	write(stream->fd, s, len);
	return len + 1;
}

int putc(int c, FILE *stream)
{
	return fputc(c, stream);
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
