#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char **argv, char **envp)
{
	close(0);
	close(1);
	close(2);
	int fdin = open("/dev/tty0", O_RDWR);
	int fdout = open("/dev/tty0", O_WRONLY);
	int fderr = open("/dev/tty0", O_WRONLY);
	if (fdin != 0 || fdout != 1 || fderr != 2)
		return EXIT_FAILURE;
	int pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "fork failed\n");
		return EXIT_FAILURE;
	}
	if (pid == 0)
	{
		while (1);
	}
	char * const sh_argv[] = {"/bin/sh", NULL};
	execve("/bin/sh", sh_argv, envp);
	return EXIT_FAILURE;
}

void _start(int argc, char **argv, char **envp)
{
	exit(main(argc, argv, envp));
}
