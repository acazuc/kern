#ifndef TIME_H
#define TIME_H

#include <sys/types.h>

struct timeval
{
	time_t tv_sec;
	time_t tv_usec;
};

struct timespec
{
	time_t tv_sec;
	time_t tv_nsec;
};

#endif
