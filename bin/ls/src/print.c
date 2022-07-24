#include "ls.h"
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void putspaces(int number)
{
	while (--number >= 0)
		printf(" ");
}

static void put_user_group(struct env *env, struct file *file, struct dir *dir)
{
	if (!env->opt_g)
		printf(" %-*s ", dir->user_len, file->user);
	if (!env->opt_o)
		printf(" %-*s", dir->group_len, file->group);
}

static void print_l(struct env *env, struct file *file, struct dir *dir)
{
	if (!env->opt_l)
		return;
	printf("%s %*s ", file->perms, dir->links_len, file->links);
	put_user_group(env, file, dir);
	printf(" %*s %*s ", dir->size_len, file->size, dir->date_len, file->date);
}

static void print_ext(struct env *env, struct file *file)
{
	if (env->opt_F)
	{
		if (S_ISLNK(file->mode))
			putchar('@');
		else if (S_ISDIR(file->mode))
			putchar('/');
		else if (S_ISSOCK(file->mode))
			putchar('=');
		else if (file->mode & (S_IXUSR | S_IXGRP | S_IXOTH))
			putchar('*');
		else if (S_ISFIFO(file->mode))
			putchar('|');
	}
	else if (env->opt_p && S_ISDIR(file->mode))
	{
		putchar('/');
	}
}

void print_file(struct env *env, struct file *file, struct dir *dir)
{
	env->printed_file = 1;
	if (env->opt_i)
		printf("%ld ", (long)file->ino);
	print_l(env, file, dir);
	fputs(file->name, stdout);
	print_ext(env, file);
	if (file->lnk_name)
		printf(" -> %s", file->lnk_name);
	putchar('\n');
}

void print_subdirs(struct env *env, struct dir *dir)
{
	struct file *lst, *nxt;
	TAILQ_FOREACH_SAFE(lst, &dir->files, chain, nxt)
	{
		if (S_ISDIR(lst->mode))
		{
			if (strcmp(lst->name, ".")
			 && strcmp(lst->name, ".."))
			{
				char tmp[MAXPATHLEN];
				snprintf(tmp, sizeof(tmp), "%s/%s", dir->path, lst->name);
				print_dir(env, tmp, 1, tmp);
			}
		}
		free_file(lst);
	}
}

void print_dir(struct env *env, const char *path, int is_recur, const char *display_path)
{
	struct dir *dir;
	struct file *lst;

	if (is_recur)
	{
		if (env->printed_file)
			putchar('\n');
		printf("%s:\n", display_path);
	}
	dir = load_dir(env, path);
	if (!dir)
		return;
	if (env->opt_l && !TAILQ_EMPTY(&dir->files))
		printf("total %ld\n", (long)dir->total_links);
	TAILQ_FOREACH(lst, &dir->files, chain)
		print_file(env, lst, dir);
	env->printed_file = 1;
	if (env->opt_R)
		print_subdirs(env, dir);
	free(dir);
}

void print_sources(struct env *env, int recur)
{
	struct source *lst;
	TAILQ_FOREACH(lst, &env->sources, chain)
		print_dir(env, lst->path, recur, lst->display_path);
}
