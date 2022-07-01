#include "vfs.h"

#include <sys/proc.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

extern struct fs_node g_ramfs_root;

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
	if (!*path)
	{
		if (!dir)
			return EINVAL;
		*node = dir;
		return 0;
	}
	if (!dir)
	{
		if (*path != '/')
			return vfs_getnode(curproc->cwd, path, node);
		return vfs_getnode(curproc->root, path + 1, node);
	}
	if (*path == '/')
		return vfs_getnode(curproc->root, path + 1, node);
	if (!S_ISDIR(dir->mode))
		return ENOTDIR;
	if (path[0] == '.' && path[1] == '.' && (path[2] == '/' || path[2] == '\0'))
		return vfs_getnode(dir->parent, path + 3, node);
	const char *next;
	if (path[0] == '.' && (path[1] == '/' || path[1] == '\0'))
		next = path + 1;
	else
		next = strchrnul(path, '/');
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
	/* XXX: do so,ething */
}
