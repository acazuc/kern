#ifndef DEVFS_H
#define DEVFS_H

#include "fs/vfs.h"

struct fs_cdev
{
	struct fs_node *node;
};

struct fs_bdev
{
	struct fs_node *node;
};

int devfs_mkcdev(const char *name, uid_t uid, gid_t gid, mode_t mode, struct file_op *file_op, struct fs_cdev **cdev);

#endif
