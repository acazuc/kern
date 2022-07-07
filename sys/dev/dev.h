#ifndef DEV_H
#define DEV_H

enum device_type
{
	DEVICE_TIMER,
};

struct dev_s
{
	enum device_type type;
};

#endif
