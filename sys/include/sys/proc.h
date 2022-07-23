#ifndef SYS_PROC_H
#define SYS_PROC_H

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/pcpu.h>
#include <arch.h>

struct vmm_ctx;
struct fs_node;
struct filedesc;

struct proc
{
	struct vmm_ctx *vmm_ctx;
	char *name;
	void *entrypoint;
	struct file **files;
	size_t files_nb;
	struct fs_node *root;
	struct fs_node *cwd;
	mode_t umask;
	pid_t ppid;
	pid_t pid;
	pid_t pgid;
	pid_t pgrp;
	uid_t uid;
	uid_t euid;
	uid_t suid;
	gid_t gid;
	gid_t egid;
	gid_t sgid;
	pri_t pri;
	TAILQ_HEAD(, thread) threads;
};

enum thread_state
{
	THREAD_RUNNING, /* currently executed */
	THREAD_PAUSED,  /* paused by scheduler */
	THREAD_WAITING, /* waiting for lock */
	THREAD_ZOMBIE,  /* dead */
};

struct thread
{
	struct proc *proc;
	enum thread_state state;
	struct trapframe tf;
	size_t stack_size;
	uint8_t *stack;
	size_t int_stack_size;
	uint8_t *int_stack;
	cpumask_t affinity;
	pid_t tid;
	pri_t pri;
	TAILQ_ENTRY(thread) sched_chain;
	TAILQ_ENTRY(thread) thread_chain;
	TAILQ_ENTRY(thread) queue_chain;
};

void init_trapframe_kern(struct thread *thread);
void init_trapframe_user(struct thread *thread);

struct thread *uproc_create(const char *name, void *entry, const char * const *av, const char * const *ev);
struct thread *kproc_create(const char *name, void *entry, const char * const *av, const char * const *ev);
struct thread *proc_fork(struct thread *thread);
struct thread *uproc_create_elf(const char *name, struct file *file, const char * const *av, const char * const *ev);

void proc_push_argv_envp(struct thread *thread, const char * const * argv, const char * const *envp);

int elf_createctx(struct file *file, struct vmm_ctx *vmm_ctx, void **entry);

void proc_delete(struct proc *proc);

#endif
