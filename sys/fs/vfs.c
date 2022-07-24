#include "devfs/devfs.h"
#include "ramfs/ramfs.h"

#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/pcpu.h>
#include <sys/std.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <arch.h>
#include <vfs.h>

struct fs_node *g_vfs_root;

struct vfs_getnode_ctx
{
	struct fs_readdir_ctx readdir_ctx;
	const char *name;
	size_t name_len;
	ino_t ret_ino;
};

int vfs_getnode(struct fs_node *dir, const char *path, struct fs_node **node)
{
	if (!path)
		return EINVAL;
	if (!dir)
	{
		if (*path == '/')
			return vfs_getnode(curthread ? curthread->proc->root : g_vfs_root, path + 1, node);
		return vfs_getnode(curthread ? curthread->proc->cwd : g_vfs_root, path, node);
	}
	if (!*path)
	{
		if (!dir)
			return EINVAL;
		if (dir->mount)
			*node = dir->mount->root;
		else
			*node = dir;
		return 0;
	}
	if (*path == '/')
		return vfs_getnode(curthread->proc->root, path + 1, node);
	if (dir->mount)
		dir = dir->mount->root;
	if (!S_ISDIR(dir->mode))
		return ENOTDIR;
	if (path[0] == '.' && path[1] == '.')
	{
		if (path[2] == '/')
			return vfs_getnode(dir->parent, path + 3, node);
		if (path[2] == '\0')
			return vfs_getnode(dir->parent, path + 2, node);
	}
	else if (path[0] == '.')
	{
		if (path[1] == '/')
			return vfs_getnode(dir, path + 2, node);
		if (path[1] == '\0')
			return vfs_getnode(dir, path + 1, node);
	}
	const char *next = strchrnul(path, '/');
	size_t pathlen = next - path;
	while (*next == '/')
		next++;
	struct fs_node *child;
	if (!dir->op->lookup)
		return ENOTDIR;
	int ret = dir->op->lookup(dir, path, pathlen, &child);
	if (ret)
		return ret;
	return vfs_getnode(child, next, node);
}

void fs_node_decref(struct fs_node *node)
{
	node->refcount--;
	/* XXX: do something */
}

void fs_node_incref(struct fs_node *node)
{
	node->refcount++;
	/* XXX do something */
}

void vfs_init(void)
{
	struct fs_sb *ramfs = ramfs_init();
	struct fs_node *devnode;
	assert(!vfs_getnode(ramfs->root, "dev", &devnode), "can't find /dev");
	struct fs_sb *devfs = devfs_init(devnode);
	g_vfs_root = ramfs->root;
}
