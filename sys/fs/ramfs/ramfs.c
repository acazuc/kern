#include "ramfs.h"

#include <sys/queue.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <sys/std.h>
#include <sys/vmm.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <arch.h>
#include <vfs.h>

static int dir_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child);
static int dir_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx);

#define BIN_SYMBOL(name, suffix) _binary____bin_##name##_##name##_##suffix
#define LIB_SYMBOL(name, suffix) _binary____lib_##name##_##name##_so_##suffix

#define BIN_DEF(bin) \
extern uint8_t BIN_SYMBOL(bin, start); \
extern uint8_t BIN_SYMBOL(bin, end);

#define LIB_DEF(lib) \
extern uint8_t LIB_SYMBOL(lib, start); \
extern uint8_t LIB_SYMBOL(lib, end);

BIN_DEF(sh);
BIN_DEF(cat);
BIN_DEF(init);
BIN_DEF(ls);
LIB_DEF(libc);
LIB_DEF(ld);

static const struct fs_type g_type =
{
	.name = "ramfs",
	.flags = 0,
};

static const struct fs_sb_op g_sb_op =
{
};

/* XXX: shouldn't be a static var */
static struct fs_sb g_sb =
{
	.op = &g_sb_op,
	.type = &g_type,
};

static const struct file_op g_dir_fop =
{
};

static const struct fs_node_op g_dir_op =
{
	.lookup = dir_lookup,
	.readdir = dir_readdir,
};

static const struct fs_node_op g_reg_op =
{
};

static int reg_read(struct file *file, void *data, size_t count);
static off_t reg_seek(struct file *file, off_t off, int whence);
static int reg_mmap(struct file *file, struct vmm_ctx *vmm_ctx, void *addr, off_t off, size_t len);

static const struct file_op g_reg_fop =
{
	.read = reg_read,
	.seek = reg_seek,
	.mmap = reg_mmap,
};

struct ramfs_node
{
	struct fs_node node;
	char *name;
	LIST_ENTRY(ramfs_node) chain;
};

struct ramfs_dir
{
	struct ramfs_node node;
	LIST_HEAD(, ramfs_node) childs;
};

struct ramfs_reg
{
	struct ramfs_node node;
	uint8_t *data;
	size_t size;
};

static size_t g_ino; /* per sb */

static int reg_read(struct file *file, void *data, size_t count)
{
	struct ramfs_reg *reg = (struct ramfs_reg*)file->node;
	if (file->off < 0)
		return 0;
	if ((size_t)file->off >= reg->size)
		return 0;
	size_t rem = reg->size - file->off;
	if (count > rem)
		count = rem;
	memcpy(data, &reg->data[file->off], count);
	file->off += count;
	return count;
}

static off_t reg_seek(struct file *file, off_t off, int whence)
{
	struct ramfs_reg *reg = (struct ramfs_reg*)file->node;
	switch (whence)
	{
		case SEEK_SET:
			if (off < 0)
				return -EINVAL;
			file->off = off;
			return file->off;
		case SEEK_CUR:
			if (off < 0 && off < -file->off)
				return -EINVAL;
			file->off += off;
			return file->off;
		case SEEK_END:
			if (off < -(off_t)reg->size)
				return -EINVAL;
			file->off = reg->size + off;
			return file->off;
		default:
			return -EINVAL;
	}
}

static int reg_mmap(struct file *file, struct vmm_ctx *vmm_ctx, void *addr, off_t off, size_t len)
{
	if (off < 0)
		return EINVAL;
	struct ramfs_reg *reg = (struct ramfs_reg*)file->node;
	void *kaddr = vmap_user(vmm_ctx, addr, len);
	if (!kaddr)
		return ENOMEM;
	size_t cpy_len;
	if ((size_t)off >= reg->size)
		cpy_len = 0;
	else if (reg->size - off < len)
		cpy_len = reg->size - off;
	else
		cpy_len = len;
	memcpy(kaddr, &reg->data[off], cpy_len);
	vunmap(kaddr, len);
	return 0;
}

static int dir_lookup(struct fs_node *node, const char *name, uint32_t namelen, struct fs_node **child)
{
	struct ramfs_dir *dir = (struct ramfs_dir*)node;
	struct ramfs_node *tmp;
	LIST_FOREACH(tmp, &dir->childs, chain)
	{
		if (strlen(tmp->name) != namelen || strncmp(name, tmp->name, namelen))
			continue;
		*child = &tmp->node;
		return 0;
	}
	return ENOENT;
}

static int dir_readdir(struct fs_node *node, struct fs_readdir_ctx *ctx)
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
	size_t i = 2;
	struct ramfs_node *tmp;
	struct ramfs_dir *dir = (struct ramfs_dir*)node;
	LIST_FOREACH(tmp, &dir->childs, chain)
	{
		if (i == (size_t)ctx->off)
		{
			struct ramfs_node *dnode = tmp;
			res = ctx->fn(ctx, dnode->name, strlen(dnode->name), i, dnode->node.ino, dnode->node.mode >> 12);
			if (res)
				return written;
			written++;
			ctx->off++;
		}
		i++;
	}
	return written;
}

static struct ramfs_node *mknode(struct ramfs_dir *parent, const char *name, uid_t uid, gid_t gid, mode_t mode, size_t st_size)
{
	struct ramfs_node *node = malloc(st_size, M_ZERO);
	assert(node, "can't allocate ramfs node");
	node->node.parent = parent ? &parent->node.node : &node->node;
	node->node.sb = &g_sb;
	node->node.ino = g_ino++;
	node->node.uid = uid;
	node->node.gid = gid;
	node->node.mode = mode;
	node->node.refcount = 1;
	node->name = strdup(name);
	assert(node->name, "can't strdup dir name");
	if (parent)
		LIST_INSERT_HEAD(&parent->childs, node, chain);
	return node;
}

static struct ramfs_dir *mkdir(struct ramfs_dir *parent, const char *name)
{
	struct ramfs_dir *dir = (struct ramfs_dir*)mknode(parent, name, 0, 0, 0755 | S_IFDIR, sizeof(*dir));
	assert(dir, "can't allocate ramfs dir");
	dir->node.node.fop = &g_dir_fop;
	dir->node.node.op = &g_dir_op;
	LIST_INIT(&dir->childs);
	return dir;
}

static struct ramfs_reg *mkreg(struct ramfs_dir *parent, const char *name)
{
	struct ramfs_reg *reg = (struct ramfs_reg*)mknode(parent, name, 0, 0, 0644 | S_IFREG, sizeof(*reg));
	assert(reg, "can't allocate ramfs reg");
	reg->node.node.fop = &g_reg_fop;
	reg->node.node.op = &g_reg_op;
	reg->data = NULL;
	reg->size = 0;
	return reg;
}

struct fs_sb *ramfs_init(void)
{
	struct ramfs_dir *root = mkdir(NULL, "");
	struct ramfs_dir *dev = mkdir(root, "dev");
	struct ramfs_dir *bin = mkdir(root, "bin");
	struct ramfs_dir *lib = mkdir(root, "lib");
	struct ramfs_reg *sh = mkreg(bin, "sh");
	sh->data = &BIN_SYMBOL(sh, start);
	sh->size = &BIN_SYMBOL(sh, end) - sh->data;
	struct ramfs_reg *cat = mkreg(bin, "cat");
	cat->data = &BIN_SYMBOL(cat, start);
	cat->size = &BIN_SYMBOL(cat, end) - cat->data;
	struct ramfs_reg *init = mkreg(bin, "init");
	init->data = &BIN_SYMBOL(init, start);
	init->size = &BIN_SYMBOL(init, end) - init->data;
	struct ramfs_reg *ls = mkreg(bin, "ls");
	ls->data = &BIN_SYMBOL(ls, start);
	ls->size = &BIN_SYMBOL(ls, end) - ls->data;
	struct ramfs_reg *libc = mkreg(lib, "libc.so");
	libc->data = &LIB_SYMBOL(libc, start);
	libc->size = &LIB_SYMBOL(libc, end) - libc->data;
	struct ramfs_reg *ld = mkreg(lib, "ld.so");
	ld->data = &LIB_SYMBOL(ld, start);
	ld->size = &LIB_SYMBOL(ld, end) - ld->data;
	g_sb.root = &root->node.node;
	return &g_sb;
}
