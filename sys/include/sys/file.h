#ifndef SYS_FILE_H
#define SYS_FILE_H

#include <stddef.h>

struct file_op;
struct fs_node;

struct file
{
	const struct file_op *op;
	struct fs_node *node;
};

struct file_op
{
	int (*open)(struct file *file, struct fs_node *node);
	int (*close)(struct file *file);
	int (*write)(struct file *file, const void *data, size_t count);
	int (*read)(struct file *file, void *data, size_t count);
	int (*ioctl)(struct file *file, unsigned long request, intptr_t data);
};

struct filedesc
{
	struct file *file;
};

#endif
