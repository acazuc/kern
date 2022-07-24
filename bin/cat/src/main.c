#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv, char **envp)
{
	return 0;
}

void _start(int argc, char **argv, char **envp)
{
	exit(main(argc, argv, envp));
}
