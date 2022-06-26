#include <fs/vfs.h>

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

/* XXX: shouldn't be a static var */
static struct fs_sb g_root_sb =
{
	.op = &g_sb_op,
	.type = &g_type,
};

struct fs_node_op g_root_op =
{
	.lookup = root_lookup,
	.readdir = root_readdir,
};

struct fs_node g_devfs_root =
{
	.op = &g_root_op,
	.parent = &g_ramfs_root,
	.sb = &g_root_sb,
	.ino = 1,
	.uid = 0,
	.gid = 0,
	.mode = 0600,
	.refcount = 0,
};

static int root_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child)
{
}

static int root_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx)
{
}
