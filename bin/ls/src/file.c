#include "ls.h"
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

time_t file_time(const struct env *env, const struct stat *info)
{
	if (env->opt_u)
		return info->st_atime;
	else if (env->opt_c)
		return info->st_ctime;
	return info->st_mtime;
}

void free_file(struct file *file)
{
	if (!file)
		return;
	free(file->perms);
	free(file->user);
	free(file->group);
	free(file->date);
	free(file->lnk_name);
	free(file->name);
	free(file);
}

static char *load_date(struct env *env, struct stat *info)
{
	char *raw_time;
	char *result;
	time_t current_time;
	time_t ftime;
	long delta_time;

	return strdup("unimp");
#if 0
	ftime = file_time(env, info);
	raw_time = ctime(&ftime);
	if (env->opt_T)
		return strndup(raw_time + 4, strlen(raw_time) - 5);
	current_time = time(NULL);
	delta_time = current_time - ftime;
	if (delta_time <=  60 * 60 * 24 * 30 * 6
	 && delta_time >= -60 * 60 * 24 * 30 * 6)
		return strndup(raw_time + 4, 12);
	if (!(result = ft_strsub(brut_time, 4, 7)))
		error_quit("Failed to malloc time");
	if (!(result = ft_strjoin_free1(result, " ")))
		error_quit("Failed to malloc time");
	if (brut_time[ft_strlen(brut_time) - 6] != ' ')
	{
		if (!(result = ft_strjoin_free3(result, ft_strsub(brut_time
							, ft_strlen(brut_time) - 6, 5))))
			error_quit("Failed to malloc time");
	}
	else if (!(result = ft_strjoin_free3(result
					, ft_strsub(brut_time, 20, 4))))
		error_quit("Failed to malloc time");
	return result;
#endif
}

static char get_perm_0(mode_t mode)
{
	if (S_ISLNK(mode))
		return 'l';
	else if (S_ISSOCK(mode))
		return 's';
	else if (S_ISDIR(mode))
		return 'd';
	else if (S_ISCHR(mode))
		return 'c';
	else if (S_ISBLK(mode))
		return 'b';
	else if (S_ISFIFO(mode))
		return 'p';
	return '-';
}

static char	get_perm_3(mode_t mode)
{
	if (mode & S_ISUID)
		return mode & S_IXUSR ? 's' : 'S';
	else
		return mode & S_IXUSR ? 'x' : '-';
}

static char	get_perm_6(mode_t mode)
{
	if (mode & S_ISGID)
		return mode & S_IXGRP ? 's' : 'S';
	else
		return mode & S_IXGRP ? 'x' : '-';
}

static char get_perm_9(mode_t mode)
{
	if (mode & S_ISVTX)
		return mode & S_IXOTH ? 't' : 'T';
	else
		return mode & S_IXOTH ? 'x' : '-';
}

static void load_perms(struct file *file, mode_t mode)
{
	file->perms[0] = get_perm_0(mode);
	file->perms[1] = mode & S_IRUSR ? 'r' : '-';
	file->perms[2] = mode & S_IWUSR ? 'w' : '-';
	file->perms[3] = get_perm_3(mode);
	file->perms[4] = mode & S_IRGRP ? 'r' : '-';
	file->perms[5] = mode & S_IWGRP ? 'w' : '-';
	file->perms[6] = get_perm_6(mode);
	file->perms[7] = mode & S_IROTH ? 'r' : '-';
	file->perms[8] = mode & S_IWOTH ? 'w' : '-';
	file->perms[9] = get_perm_9(mode);
	file->perms[10] = ' ';
	file->perms[11] = '\0';
}

static void setinfos(struct env *env, struct file *file, struct stat *info)
{
	if (env->opt_n)
	{
		char tmp[16];
		snprintf(tmp, sizeof(tmp), "%ld", info->st_uid);
		file->user = strdup(tmp);
		snprintf(tmp, sizeof(tmp), "%ld", info->st_gid);
		file->group = strdup(tmp);
	}
	else
	{
#if 0
		struct passwd *pw = getpwuid(info->st_uid);
		struct group *gr = getgrgid(info->st_gid);
		file->user = ft_strdup(pw->pw_name ? pw->pw_name : "");
		file->group = ft_strdup(gr->gr_name ? gr->gr_name : "");
#else
		file->user = strdup("user");
		file->user = strdup("group");
#endif
	}
	load_perms(file, info->st_mode);
	if (file->perms[0] == 'c' || file->perms[0] == 'b')
		snprintf(file->size, sizeof(file->size), "%3d, %3d", (int)major(info->st_rdev), (int)minor(info->st_rdev));
	else
		snprintf(file->size, sizeof(file->size), "%ld", (long)info->st_size);
	file->date = load_date(env, info);
}

static int load_symb(struct env *env, struct file *file, struct stat *info, const char *rpath)
{
	ssize_t r;
	char *linkname;

	linkname = malloc(info->st_size + 1);
	if (!linkname)
	{
		perror("ls: linkname allocation failed");
		exit(EXIT_FAILURE);
	}
	r = readlink(rpath, linkname, info->st_size + 1);
	if (r < 0)
	{
		perror("ls: failed to read link");
		exit(EXIT_FAILURE);
	}
	linkname[info->st_size] = '\0';
	if (env->opt_l)
	{
		file->lnk_name = linkname;
		setinfos(env, file, info);
	}
	snprintf(file->ino, sizeof(file->ino), "%ld", (long)info->st_ino);
	file->sort_size = info->st_size;
	file->sort_date = file_time(env, info);
	return info->st_blocks;
}

void load_file(struct env *env, struct file *file, const char *name, struct dir *dir)
{
	struct stat linfo;
	struct stat ninfo;
	char path[MAXPATHLEN];
	int is_lnk;

	file->name = strdup(name);
	file->lnk_name = NULL;
	if (name[0] == '/')
		strlcpy(path, name, sizeof(path));
	else
		snprintf(path, sizeof(path), "%s/%s", dir->path, name);
	if (stat(path, &ninfo) == -1)
	{
		perror("ls: failed to stat file");
		exit(EXIT_FAILURE);
	}
	is_lnk = (lstat(path, &linfo) == 0) && (ninfo.st_ino != linfo.st_ino);
	snprintf(file->links, sizeof(file->links), "%ld", (long)ninfo.st_nlink);
	snprintf(file->ino, sizeof(file->ino), "%ld", (long)ninfo.st_ino);
	file->sort_size = ninfo.st_size;
	file->sort_date = file_time(env, &ninfo);
	if (!env->opt_P && is_lnk)
	{
		dir->total_links += load_symb(env, file, &linfo, path);
		return;
	}
	if (env->opt_l)
		setinfos(env, file, &ninfo);
	if (env->opt_l)
		dir->total_links += ninfo.st_blocks;
}
