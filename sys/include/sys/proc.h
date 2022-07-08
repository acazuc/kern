#ifndef SYS_PROC_H
#define SYS_PROC_H

#include "arch/arch.h"
#include <sys/queue.h>
#include <sys/types.h>

struct vmm_ctx;
struct fs_node;
struct filedesc;

struct proc
{
	struct vmm_ctx *vmm_ctx;
	char *name;
	void *entrypoint;
	struct filedesc *files;
	uint32_t files_nb;
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
};

struct thread
{
	struct proc *proc;
	enum thread_state state;
	struct trapframe trapframe;
	size_t stack_size;
	uint8_t *stack;
	pid_t tid;
	pri_t pri;
	TAILQ_ENTRY(thread) sched_chain;
	TAILQ_ENTRY(thread) thread_chain;
	TAILQ_ENTRY(thread) queue_chain;
};

struct thread *uproc_create(const char *name, void *entry);
struct thread *kproc_create(const char *name, void *entry);
struct proc *proc_fork(struct proc *proc);

extern struct proc *curproc;
extern struct thread *curthread;

#endif
