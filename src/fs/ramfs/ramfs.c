#include "fs/vfs.h"

#include <string.h>
#include <errno.h>

static int root_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child);
static int root_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx);

static struct fs_type g_type =
{
	.name = "ramfs",
	.flags = 0,
};

static struct fs_sb_op g_sb_op =
{
};

/* XXX: shouldn't be a static var */
static struct fs_sb g_root_sb =
{
	.op = &g_sb_op,
	.type = &g_type,
};

static struct fs_node_op g_root_op =
{
	.lookup = root_lookup,
	.readdir = root_readdir,
};

struct fs_node g_ramfs_root =
{
	.op = &g_root_op,
	.parent = &g_ramfs_root,
	.sb = &g_root_sb,
	.ino = 1,
	.uid = 0,
	.gid = 0,
	.mode = 0600,
	.refcount = 1,
};

extern struct fs_node g_devfs_root;

static int root_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child)
{
	(void)node;
	if (!strncmp(name, "dev", namelen))
	{
		*child = &g_devfs_root;
		return 0;
	}
	return ENOENT;
}

static int root_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx)
{
	(void)node;
	(void)ctx;
	return EINVAL;
}
