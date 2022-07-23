#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

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
		exit(0);
		write(g_fd, "whoopsie\n", 9);
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
	const char *bin = line;
	const char *argv[] = {bin, NULL};
	const char *null = NULL;
	execve(bin, argv, &null);
	write(g_fd, "failed to exec /bin/sh\n", 23);
	exit(0);
}

int main(int ac, char **av, char **ev)
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
				{
					write(g_fd, "rd < 0\n", 7);
					while (1) ;
				}
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

void _start(int argc, char **argv, char **envp)
{
	exit(main(argc, argv, envp));
}
