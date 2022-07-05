#include "fs/vfs.h"
#include "devfs.h"

#include <sys/stat.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int root_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child);
static int root_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx);

extern struct fs_node g_ramfs_root;

static struct fs_type g_type =
{
	.name = "devfs",
	.flags = 0,
};

static struct fs_sb_op g_sb_op =
{
};

/* XXX: shouldn't be a static var, I guess */
static struct fs_sb g_sb =
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

struct fs_node g_devfs_root =
{
	.fop = &g_root_fop,
	.op = &g_root_op,
	.parent = &g_ramfs_root,
	.sb = &g_sb,
	.ino = 1,
	.uid = 0,
	.gid = 0,
	.mode = 0600 | S_IFDIR,
	.refcount = 1,
};

static struct fs_node_op g_node_op =
{
};

struct devfs_node
{
	struct fs_node *node;
	char *name;
};

static struct devfs_node **g_nodes = NULL;
static size_t g_nodes_nb = 0;

static int root_lookup(struct fs_node *dir, const char *name, uint32_t namelen, struct fs_node **child)
{
	if (dir != &g_devfs_root)
		return ENOENT;
	for (size_t i = 0; i < g_nodes_nb; ++i)
	{
		struct devfs_node *node = g_nodes[i];
		if (!node)
			continue;
		if (!strncmp(node->name, name, namelen))
		{
			*child = node->node;
			return 0;
		}
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
	for (size_t i = ctx->off - 2; i < g_nodes_nb; ++i)
	{
		struct devfs_node *dnode = g_nodes[i];
		res = ctx->fn(ctx, dnode->name, strlen(dnode->name), i + 2, dnode->node->ino, dnode->node->mode >> 12);
		if (res)
			return written;
		written++;
		ctx->off++;
	}
	return written;
}

static int find_ino(struct devfs_node *node)
{
	struct devfs_node **nodes = realloc(g_nodes, sizeof(*nodes) * (g_nodes_nb + 1), 0);
	if (!nodes)
		return ENOMEM;
	g_nodes = nodes;
	g_nodes[g_nodes_nb] = node;
	g_nodes_nb++;
	node->node->ino = g_nodes_nb;
	return 0;
}

int devfs_mkcdev(const char *name, uid_t uid, gid_t gid, mode_t mode, dev_t rdev, struct file_op *file_op, struct fs_cdev **cdev)
{
	if (!cdev)
		return EINVAL;
	int ret;
	struct fs_node *node;
	if (!root_lookup(&g_devfs_root, name, strlen(name), &node))
	{
		fs_node_decref(node);
		return EEXIST;
	}
	struct devfs_node *devnode = NULL;
	*cdev = NULL;
	char *namedup = strdup(name);
	if (!namedup)
	{
		ret = ENOMEM;
		goto err;
	}
	*cdev = malloc(sizeof(**cdev), 0);
	if (!*cdev)
	{
		ret = ENOMEM;
		goto err;
	}
	devnode = malloc(sizeof(*devnode), M_ZERO);
	if (!devnode)
	{
		ret = ENOMEM;
		goto err;
	}
	devnode->name = namedup;
	node = malloc(sizeof(*node), M_ZERO);
	if (!node)
	{
		ret = ENOMEM;
		goto err;
	}
	devnode->node = node;
	node->op = &g_node_op;
	node->fop = file_op;
	node->sb = &g_sb;
	node->parent = &g_devfs_root;
	node->uid = uid;
	node->gid = gid;
	node->mode = ((mode) & ~S_IFMT) | S_IFCHR;
	node->rdev = rdev;
	node->refcount = 1;
	ret = find_ino(devnode);
	if (ret)
		goto err;
	(*cdev)->node = node;
	return 0;

err:
	free(namedup);
	free(*cdev);
	free(devnode);
	free(node);
	return ret;
}
