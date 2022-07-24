#ifndef LS_H
#define LS_H

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stddef.h>

struct dir
{
	const char *path;
	const char *name;
	int links_len;
	int user_len;
	int group_len;
	int size_len;
	int date_len;
	int total_links;
	TAILQ_HEAD(, file) files;
	TAILQ_ENTRY(dir) chain;
};

struct file
{
	char *name;
	char *lnk_name;
	char perms[12];
	char *user;
	char *group;
	char size[32];
	char *date;
	char links[32];
	char ino[32];
	mode_t mode;
	time_t sort_date;
	off_t sort_size;
	TAILQ_ENTRY(file) chain;
};

struct source
{
	char *path;
	char *display_path;
	int sort_date;
	TAILQ_ENTRY(source) chain;
};

struct env
{
	int opt_a;
	int opt_A;
	int opt_c;
	int opt_f;
	int opt_F;
	int opt_g;
	int opt_i;
	int opt_l;
	int opt_n;
	int opt_o;
	int opt_p;
	int opt_P;
	int opt_r;
	int opt_R;
	int opt_S;
	int opt_t;
	int opt_T;
	int opt_u;
	int opt_U;
	int opt_1;
	int printed_file;
	TAILQ_HEAD(, source) sources;
};

void parse_sources(struct env *env, int ac, char **argv);
void print_dir(struct env *env, const char *path, int is_recur, const char *display_path);
void print_file(struct env *env, struct file *file, struct dir *dir);
void dir_add_file(struct env *env, struct dir *dir, const char *name);
void dir_init(struct dir *dir, const char *path);
struct dir *load_dir(struct env *env, const char *path);
void free_file(struct file *file);
void load_file(struct env *env, struct file *file, const char *name, struct dir *dir);
time_t file_time(const struct env *env, const struct stat *info);
void print_sources(struct env *env, int recur);

#endif
