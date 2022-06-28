#include "fs/vfs.h"
#include "devfs.h"

#include <sys/stat.h>
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

static struct fs_node_op g_root_op =
{
	.lookup = root_lookup,
	.readdir = root_readdir,
};

struct fs_node g_devfs_root =
{
	.op = &g_root_op,
	.parent = &g_ramfs_root,
	.sb = &g_sb,
	.ino = 1,
	.uid = 0,
	.gid = 0,
	.mode = 0600,
	.refcount = 0,
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
	if (g_nodes)
		printf("g_nodes[0]->name: %p\n", g_nodes[0]->name);
	for (size_t i = 0; i < g_nodes_nb; ++i)
	{
		struct devfs_node *node = g_nodes[i];
		if (!node)
			continue;
		printf("strncmp\n");
		if (!strncmp(node->name, name, namelen))
		{
			printf("setting at %p\n", child);
			*child = node->node;
			return 0;
		}
		printf("nope\n");
	}
	return ENOENT;
}

static int root_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx)
{
	(void)node;
	(void)ctx;
	return 0;
}

static int find_ino(struct devfs_node *node)
{
	struct devfs_node **nodes = realloc(g_nodes, g_nodes_nb + 1, 0);
	if (!nodes)
		return ENOMEM;
	printf("realloc nodes\n");
	g_nodes = nodes;
	g_nodes[g_nodes_nb] = node;
	g_nodes_nb++;
	node->node->ino = g_nodes_nb;
	return 0;
}

int devfs_mkcdev(const char *name, uid_t uid, gid_t gid, mode_t mode, struct file_op *file_op, struct fs_cdev **cdev)
{
	if (!cdev)
		return EINVAL;
	int ret;
	struct fs_node *node;
	if (!root_lookup(&g_devfs_root, name, strlen(name), &node))
	{
		/* XXX: decref child */
		return EEXIST;
	}
	struct devfs_node *cnode = NULL;
	*cdev = NULL;
	char *namedup = strdup(name);
	if (!namedup)
	{
		ret = ENOMEM;
		goto err;
	}
	*cdev = malloc(sizeof(*cdev), 0);
	if (!*cdev)
	{
		ret = ENOMEM;
		goto err;
	}
	cnode = malloc(sizeof(*cnode), M_ZERO);
	if (!cnode)
	{
		ret = ENOMEM;
		goto err;
	}
	cnode->name = strdup(name);
	if (!cnode->name)
	{
		ret = ENOMEM;
		goto err;
	}
	printf("cnode name: %p\n", cnode->name);
	node = malloc(sizeof(*node), 0);
	if (!node)
	{
		ret = ENOMEM;
		goto err;
	}
	cnode->node = node;
	node->op = &g_node_op;
	node->fop = file_op;
	node->sb = &g_sb;
	node->parent = &g_devfs_root;
	node->uid = uid;
	node->gid = gid;
	node->mode = ((mode) & ~S_IFMT) | S_IFCHR;
	node->refcount = 1;
	ret = find_ino(cnode);
	if (ret)
		goto err;
	(*cdev)->node = node;
	printf("cnode name 2: %p\n", g_nodes[0]->name);
	return 0;

err:
	free(namedup);
	free(*cdev);
	if (cnode)
	{
		free(cnode->name);
		free(cnode);
	}
	free(node);
	return ret;
}
