#ifndef DEV_RTC_H
#define DEV_RTC_H

#include <time.h>

void rtc_init(void);
void rtc_gettime(struct timespec *ts);

#endif
