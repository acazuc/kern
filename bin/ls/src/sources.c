#include "ls.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static void parse_empty(int argc, char **argv)
{
	for (int i = 0; i < argc; ++i)
	{
		if (!argv[i][0])
		{
			fprintf(stderr, "ls: open: No such file or directory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static int insert(struct env *env, struct source *ls, struct source *cs)
{
	if (env->opt_f)
		return 0;
	if (env->opt_t && cs->sort_date != ls->sort_date)
	{
		if (env->opt_r)
			return cs->sort_date < ls->sort_date;
		else
			return cs->sort_date > ls->sort_date;
	}
	else
	{
		if (env->opt_r)
			return strcmp(cs->display_path, ls->display_path) > 0;
		else
			return strcmp(cs->display_path, ls->display_path) < 0;
	}
}

static void push(struct env *env, struct source *source)
{
	if (TAILQ_EMPTY(&env->sources))
	{
		TAILQ_INSERT_HEAD(&env->sources, source, chain);
		return;
	}
	struct source *lst;
	TAILQ_FOREACH(lst, &env->sources, chain)
	{
		if (insert(env, lst, source))
		{
			TAILQ_INSERT_BEFORE(lst, source, chain);
			return;
		}
	}
	TAILQ_INSERT_TAIL(&env->sources, source, chain);
}

static void print_dir_fast(struct env *env, struct dir *dir)
{
	struct file *lst, *nxt;
	TAILQ_FOREACH_SAFE(lst, &dir->files, chain, nxt)
	{
		print_file(env, lst, dir);
		free_file(lst);
	}
}

static void push_source(struct env *env, char *path, char *display_path , struct stat *info)
{
	struct source *new;

	new = malloc(sizeof(*new));
	if (!new)
	{
		fprintf(stderr, "source allocation failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	new->display_path = display_path;
	new->path = path;
	new->sort_date = file_time(env, info);
	push(env, new);
}

static int add_source(struct env *env, char *path, struct dir *dir , char *display_path)
{
	struct stat info;
	struct stat linfo;

	if (stat(path, &info) == -1 && lstat(path, &linfo) == -1)
	{
		fprintf(stderr, "stat failed: %s\n", strerror(errno));
		return 0;
	}
	if (S_ISDIR(info.st_mode) && (!env->opt_l || S_ISDIR(linfo.st_mode)))
	{
		push_source(env, path, display_path, &info);
		return 0;
	}
	dir_add_file(env, dir, path);
	return 1;
}

void parse_sources(struct env *env, int argc, char **argv)
{
	struct dir dir;
	char *display_path;
	int printed_file;
	int printed;

	dir_init(&dir, ".");
	printed = 0;
	printed_file = 0;
	parse_empty(argc, argv);
	for (int i = 0; i < argc; ++i)
	{
		display_path = strdup(argv[i]);
		size_t len = strlen(argv[i]);
		if (argv[i][len - 1] == '/' && len > 1)
			argv[i][len - 1] = '\0';
		printed_file += add_source(env, argv[i], &dir, display_path);
		printed = 1;
	}
	print_dir_fast(env, &dir);
	if (!printed)
		print_dir(env, ".", 0, ".");
	else
		print_sources(env, printed_file || argc >= 2);
}
