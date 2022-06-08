#ifndef DEV_PIT_H
#define DEV_PIT_H

#include "sys/std.h"

void pit_init(void);
void pit_interrupt(void);
void pit_gettime(struct timespec *ts);

#endif
