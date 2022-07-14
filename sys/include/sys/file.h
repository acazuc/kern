#ifndef SYS_FILE_H
#define SYS_FILE_H

#include <sys/types.h>
#include <stddef.h>

struct file_op;
struct fs_node;

struct file
{
	const struct file_op *op;
	struct fs_node *node;
	off_t off;
	size_t refcount;
	void *userptr;
};

struct file_op
{
	int (*open)(struct file *file, struct fs_node *node);
	int (*close)(struct file *file);
	int (*write)(struct file *file, const void *data, size_t count);
	int (*read)(struct file *file, void *data, size_t count);
	int (*ioctl)(struct file *file, unsigned long request, intptr_t data);
	int (*mmap)(struct file *file, struct vmm_ctx *vmm_ctx, void *dst, off_t off, size_t len);
	off_t (*seek)(struct file *file, off_t off, int whence);
};

void file_incref(struct file *file);
void file_decref(struct file *file);

#endif
