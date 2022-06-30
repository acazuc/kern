#ifndef DEV_PIT_H
#define DEV_PIT_H

#include <time.h>

void pit_init(void);
void pit_gettime(struct timespec *ts);

#endif
