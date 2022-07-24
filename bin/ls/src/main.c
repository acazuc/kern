#include "ls.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static void usage(void)
{
	printf("ls -[aAcfFgilnopPrRStTuU1]\n");
}

int main(int argc, char **argv, char **envp)
{
	struct env env;
	char c;

	(void)envp;
	memset(&env, 0, sizeof(env));
	TAILQ_INIT(&env.sources);
	while ((c = getopt(argc, argv, "aAcfFgilnopPrRStTuU1")) != -1)
	{
		switch (c)
		{
			case 'a':
				env.opt_a = 1;
				break;
			case 'A':
				env.opt_A = 1;
				break;
			case 'l':
				env.opt_l = 1;
				break;
			case 'R':
				env.opt_R = 1;
				break;
			case 'r':
				env.opt_r = 1;
				break;
			case 't':
				env.opt_t = 1;
				break;
			case 'u':
				env.opt_u = 1;
				env.opt_U = 0;
				env.opt_c = 0;
				break;
			case 'U':
				env.opt_U = 1;
				env.opt_u = 0;
				env.opt_c = 0;
				break;
			case 'g':
				env.opt_l = 1;
				env.opt_g = 1;
				break;
			case 'o':
				env.opt_o = 1;
				env.opt_l = 1;
				break;
			case 'f':
				env.opt_f = 1;
				env.opt_a = 1;
				break;
			case 'S':
				env.opt_S = 1;
				env.opt_t = 0;
				break;
			case 'i':
				env.opt_i = 1;
				break;
			case 'n':
				env.opt_n = 1;
				env.opt_l = 1;
				break;
			case 'p':
				env.opt_p = 1;
				env.opt_F = 0;
				break;
			case 'P':
				env.opt_P = 1;
				break;
			case '1':
				env.opt_1 = 1;
				break;
			case 'T':
				env.opt_T = 1;
				break;
			case 'c':
				env.opt_c = 1;
				env.opt_u = 0;
				env.opt_U = 0;
				break;
			case 'F':
				env.opt_F = 1;
				env.opt_p = 0;
				break;
			default:
				usage();
				return EXIT_FAILURE;
		}
	}
	/* XXX remove debug opt */
	env.opt_l = 1;
	env.opt_a = 1;
	env.opt_n = 1;
	parse_sources(&env, argc - optind, &argv[optind]);
	return EXIT_SUCCESS;
}

void _start(int argc, char **argv, char **envp)
{
	exit(main(argc, argv, envp));
}
