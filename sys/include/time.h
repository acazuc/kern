#ifndef TIME_H
#define TIME_H

#include <sys/types.h>

struct timespec
{
	time_t tv_sec;
	time_t tv_nsec;
};

#endif
