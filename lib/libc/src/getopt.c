#include <unistd.h>
#include <string.h>

char *optarg;
int optind;
int opterr;
int optopt;

int getopt(int argc, char * const argv[], const char *optstring)
{
	if (optind == 0)
		optind++;
	if (optind >= argc)
		return -1;
	char *arg = argv[optind];
	if (arg[0] != '-' || !strcmp(arg, "-") || !strcmp(arg, "--"))
		return -1;
	return '?';
}
