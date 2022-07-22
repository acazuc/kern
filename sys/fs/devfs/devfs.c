#include "devfs.h"

#include <sys/stat.h>
#include <sys/file.h>
#include <sys/std.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <arch.h>

static int root_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child);
static int root_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx);

struct devfs
{
	struct devfs_node **nodes;
	size_t nodes_nb;
	size_t ino;
	struct fs_cdev *node_mem;
	struct fs_cdev *node_null;
	struct fs_cdev *node_zero;
	struct fs_cdev *node_random;
	struct fs_cdev *node_urandom;
};

static struct devfs g_devfs;

static const struct fs_type g_type =
{
	.name = "devfs",
	.flags = 0,
};

static const struct fs_sb_op g_sb_op =
{
};

/* XXX: shouldn't be a static var, I guess */
static struct fs_sb g_sb =
{
	.op = &g_sb_op,
	.type = &g_type,
};

static const struct file_op g_root_fop =
{
};

static const struct fs_node_op g_root_op =
{
	.lookup = root_lookup,
	.readdir = root_readdir,
};

static const struct fs_node_op g_node_op =
{
};

struct devfs_node
{
	struct fs_node *node;
	char *name;
};

static int root_lookup(struct fs_node *dir, const char *name, uint32_t namelen, struct fs_node **child)
{
	if (dir != g_sb.root)
		return ENOENT;
	for (size_t i = 0; i < g_devfs.nodes_nb; ++i)
	{
		struct devfs_node *node = g_devfs.nodes[i];
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
	for (size_t i = ctx->off - 2; i < g_devfs.nodes_nb; ++i)
	{
		struct devfs_node *dnode = g_devfs.nodes[i];
		res = ctx->fn(ctx, dnode->name, strlen(dnode->name), i + 2, dnode->node->ino, dnode->node->mode >> 12);
		if (res)
			return written;
		written++;
		ctx->off++;
	}
	return written;
}

static const struct file_op g_mem_fop =
{
	/* XXX */
};

static int null_write(struct file *file, const void *data, size_t count)
{
	(void)file;
	(void)data;
	return count;
}

static int null_read(struct file *file, void *data, size_t count)
{
	(void)file;
	(void)data;
	(void)count;
	return 0;
}

static const struct file_op g_null_fop =
{
	.write = null_write,
	.read = null_read,
};

static int zero_write(struct file *file, const void *data, size_t count)
{
	(void)file;
	(void)data;
	return count;
}

static int zero_read(struct file *file, void *data, size_t count)
{
	(void)file;
	memset(data, 0, count);
	return count;
}

static const struct file_op g_zero_fop =
{
	.write = zero_write,
	.read = zero_read,
};

static int random_write(struct file *file, const void *data, size_t count)
{
	(void)file;
	(void)data;
	return count;
}

static int random_read(struct file *file, void *data, size_t count)
{
	/* XXX */
	return 0;
}

static const struct file_op g_random_fop =
{
	.write = random_write,
	.read = random_read,
};

static int urandom_write(struct file *file, const void *data, size_t count)
{
	(void)file;
	(void)data;
	return count;
}

static int urandom_read(struct file *file, void *data, size_t count)
{
	/* XXX */
	return 0;
}

static const struct file_op g_urandom_fop =
{
	.write = urandom_write,
	.read = urandom_read,
};

static int find_ino(struct devfs_node *node)
{
	struct devfs_node **nodes = realloc(g_devfs.nodes, sizeof(*nodes) * (g_devfs.nodes_nb + 1), 0);
	if (!nodes)
		return ENOMEM;
	g_devfs.nodes = nodes;
	g_devfs.nodes[g_devfs.nodes_nb] = node;
	g_devfs.nodes_nb++;
	node->node->ino = g_devfs.nodes_nb;
	return 0;
}

int devfs_mkcdev(const char *name, uid_t uid, gid_t gid, mode_t mode, dev_t rdev, const struct file_op *file_op, struct fs_cdev **cdev)
{
	if (!cdev)
		return EINVAL;
	int ret;
	struct fs_node *node;
	if (!root_lookup(g_sb.root, name, strlen(name), &node))
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
	node->parent = g_sb.root;
	node->ino = ++g_devfs.ino;
	node->uid = uid;
	node->gid = gid;
	node->mode = (mode & ~S_IFMT) | S_IFCHR;
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

struct fs_sb *devfs_init(struct fs_node *dir)
{
	struct fs_node *root = malloc(sizeof(*root), M_ZERO);
	assert(root, "can't create devfs root");
	root->fop = &g_root_fop;
	root->op = &g_root_op;
	root->parent = dir;
	root->sb = &g_sb;
	root->ino = ++g_devfs.ino;
	root->uid = 0;
	root->gid = 0;
	root->mode = 0755 | S_IFDIR;
	root->refcount = 1;
	g_sb.root = root;
	assert(!devfs_mkcdev("mem", 0, 0, 0400, makedev(1, 1), &g_mem_fop, &g_devfs.node_mem), "can't create /dev/mem");
	assert(!devfs_mkcdev("null", 0, 0, 0666, makedev(1, 3), &g_null_fop, &g_devfs.node_null), "can't create /dev/null");
	assert(!devfs_mkcdev("zero", 0, 0, 0666, makedev(1, 5), &g_zero_fop, &g_devfs.node_zero), "can't create /dev/zero");
	assert(!devfs_mkcdev("random", 0, 0, 0666, makedev(1, 8), &g_random_fop, &g_devfs.node_random), "can't create /dev/random");
	assert(!devfs_mkcdev("urandom", 0, 0, 0666, makedev(1, 9), &g_urandom_fop, &g_devfs.node_urandom), "can't create /dev/urandom");
	dir->mount = &g_sb;
	return &g_sb;
}
