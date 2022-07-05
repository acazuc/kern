#include "fs/vfs.h"

#include <sys/file.h>
#include <sys/stat.h>
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

static struct file_op g_root_fop =
{
};

static struct fs_node_op g_root_op =
{
	.lookup = root_lookup,
	.readdir = root_readdir,
};

struct fs_node g_ramfs_root =
{
	.fop = &g_root_fop,
	.op = &g_root_op,
	.parent = &g_ramfs_root,
	.sb = &g_root_sb,
	.ino = 1,
	.uid = 0,
	.gid = 0,
	.mode = 0600 | S_IFDIR,
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
	int res;
	int written = 0;
	if (ctx->off == 0)
	{
		res = ctx->fn(ctx, ".", 1, 0, 1, DT_DIR);
		if (res)
			return written;
		written++;
		ctx->off++;
	}
	if (ctx->off == 1)
	{
		res = ctx->fn(ctx, "..", 2, 1, node->parent ? node->parent->ino : node->ino, DT_DIR);
		if (res)
			return written;
		written++;
		ctx->off++;
	}
	if (ctx->off == 2)
	{
		res = ctx->fn(ctx, "dev", 3, 2, 2, DT_DIR);
		if (res)
			return written;
		written++;
		ctx->off++;
	}
	return written;
}
