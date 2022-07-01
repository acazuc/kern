#ifndef VFS_H
#define VFS_H

#include <sys/types.h>
#include <stddef.h>
#include <time.h>

struct fs_sb;
struct fs_node;
struct fs_type;
struct fs_sb_op;
struct fs_node_op;
struct file_op;

struct fs_type
{
	char *name;
	uint32_t flags;
};

struct fs_sb
{
	const struct fs_sb_op *op;
	struct fs_type *type;
	dev_t dev;
};

struct fs_node
{
	const struct fs_node_op *op;
	const struct file_op *fop;
	struct fs_node *parent;
	struct fs_sb *sb;
	struct timespec atime;
	struct timespec mtime;
	struct timespec ctime;
	blksize_t blksize;
	blkcnt_t blocks;
	nlink_t nlink;
	dev_t rdev;
	ino_t ino;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	off_t size;
	uint32_t refcount; /* XXX: to be used */
	void *userptr;
};

struct fs_sb_op
{
};

struct fs_readdir_ctx;

typedef int (*fs_dircb_t)(struct fs_readdir_ctx *ctx, const char *name, uint32_t namelen, off_t off, ino_t ino, uint32_t type);

struct fs_readdir_ctx
{
	fs_dircb_t fn;
	off_t off;
	void *userptr;
};

struct fs_node_op
{
	int (*lookup)(struct fs_node *dir, const char *name, uint32_t namelen, struct fs_node **child);
	int (*readdir)(struct fs_node *node, struct fs_readdir_ctx *ctx);
};

int vfs_getnode(struct fs_node *dir, const char *path, struct fs_node **node);
void fs_node_decref(struct fs_node *node);

#endif
