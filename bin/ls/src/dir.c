#include "ls.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>

static void check_lengths(struct env *env, struct dir *dir, struct file *file)
{
	int len;

	if (!env->opt_l)
		return;
	if ((len = strlen(file->links)) > dir->links_len)
		dir->links_len = len;
	if ((len = strlen(file->user)) > dir->user_len)
		dir->user_len = len;
	if ((len = strlen(file->group)) > dir->group_len)
		dir->group_len = len;
	if ((len = strlen(file->size)) > dir->size_len)
		dir->size_len = len;
	if ((len = strlen(file->date)) > dir->date_len)
		dir->date_len = len;
}

static int insert(struct env *env, struct file *lf, struct file *cf)
{
	if (env->opt_f)
		return 0;
	if (env->opt_S && cf->sort_size != lf->sort_size)
	{
		if (env->opt_r)
			return cf->sort_size < lf->sort_size;
		else
			return cf->sort_size > lf->sort_size;
	}
	if (env->opt_t && cf->sort_date != lf->sort_date)
	{
		if (env->opt_r)
			return cf->sort_date < lf->sort_date;
		else
			return cf->sort_date > lf->sort_date;
	}
	else
	{
		if (env->opt_r)
			return strcmp(cf->name, lf->name) > 0;
		else
			return strcmp(cf->name, lf->name) < 0;
	}
}

static void add_file(struct env *env, struct dir *dir, struct file *file)
{
	if (TAILQ_EMPTY(&dir->files))
	{
		TAILQ_INSERT_HEAD(&dir->files, file, chain);
		return;
	}
	struct file *lst;
	TAILQ_FOREACH(lst, &dir->files, chain)
	{
		if (insert(env, lst, file))
		{
			TAILQ_INSERT_BEFORE(lst, file, chain);
			return;
		}
	}
	TAILQ_INSERT_TAIL(&dir->files, file, chain);
}

void dir_add_file(struct env *env, struct dir *dir, const char *name)
{
	struct file *file;

	file = malloc(sizeof(*file));
	if (!file)
	{
		perror("ls: file allocation failed");
		exit(EXIT_FAILURE);
	}
	load_file(env, file, name, dir);
	check_lengths(env, dir, file);
	add_file(env, dir, file);
}

void dir_init(struct dir *dir, const char *path)
{
	memset(dir, 0, sizeof(*dir));
	dir->path = path;
	TAILQ_INIT(&dir->files);
}

struct dir *load_dir(struct env *env, const char *path)
{
	struct dir *dir;
	DIR *fdir;
	struct dirent *dirent;

	fdir = opendir(path);
	if (!fdir)
	{
		fprintf(stderr, "ls: %s: %s", path, strerror(errno));
		return NULL;
	}
	dir = malloc(sizeof(*dir));
	if (!dir)
	{
		perror("ls: dir allocation failed\n");
		exit(EXIT_FAILURE);
	}
	dir_init(dir, path);
	while ((dirent = readdir(fdir)))
	{
		if (dirent->d_name[0] == '.')
		{
			if (!(env->opt_A && strcmp(dirent->d_name, ".") && strcmp(dirent->d_name, "..")))
				continue;
		}
		dir_add_file(env, dir, dirent->d_name);
	}
	closedir(fdir);
	return dir;
}
