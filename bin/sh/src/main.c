#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

static void exec_line(const char *line)
{
	if (!strcmp(line, "colors"))
	{
		for (size_t i = 0; i < 10; ++i)
		{
			char tmp[] = "\e[0;30m0;30 \e[1;30m1;30 \e[0;40m0;40\e[0m \e[1;40m1;40\e[0m";
			tmp[5]  = '0' + i;
			tmp[10] = '0' + i;
			tmp[17] = '0' + i;
			tmp[22] = '0' + i;
			tmp[29] = '0' + i;
			tmp[34] = '0' + i;
			tmp[45] = '0' + i;
			tmp[50] = '0' + i;
			puts(tmp);
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
			fputs("can't stat", stderr);
			return;
		}
		char str[] = "mode: 000";
		str[6] = '0' + ((st.st_mode >> 6) & 0x7);
		str[7] = '0' + ((st.st_mode >> 3) & 0x7);
		str[8] = '0' + ((st.st_mode >> 0) & 0x7);
		puts(str);
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
			fprintf(stderr, "failed to open file\n");
			return;
		}
		int res;
		printf("files: ");
		DIR *dir = fdopendir(fd);
		if (!dir)
		{
			fprintf(stderr, "fdopendir failed\n");
			return;
		}
		struct dirent *dirent;
		while ((dirent = readdir(dir)))
			printf("%s ", dirent->d_name);
		closedir(dir);
		puts("");
		close(fd);
		return;
	}
	if (!strcmp(line, "fork"))
	{
		int pid = fork();
		if (pid < 0)
		{
			fprintf(stderr, "fork failed\n");
			return;
		}
		if (pid)
		{
			puts("parent\n");
			return;
		}
		puts("child\n");
		exit(0);
	}
	if (!strcmp(line, "pid"))
	{
		int pid = getpid();
		printf("%d\n", pid);
		return;
	}
	fprintf(stderr, "unknown command: %s\n", line);
	int pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "fork failed\n");
		return;
	}
	if (pid)
	{
		printf("parent\n");
		return;
	}
	const char *bin = line;
	const char * const argv[] = {bin, NULL};
	const char *null = NULL;
	execve(bin, (char * const*)argv, (char * const*)&null);
	printf("failed to exec %s\n", bin);
	exit(0);
}

int main(int ac, char **av, char **ev)
{
	char buf[78] = "";
	char line[78];
	size_t buf_pos = 0;
	while (1)
	{
		printf("\e[0m# ");
		char *eol = NULL;
		do
		{
			int rd = read(0, buf + buf_pos, sizeof(buf) - buf_pos - 1);
			if (rd < 0)
			{
				if (errno != EAGAIN)
					printf("rd < 0\n");
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
}

void _start(int argc, char **argv, char **envp)
{
	exit(main(argc, argv, envp));
}
