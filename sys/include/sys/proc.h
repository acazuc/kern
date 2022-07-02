#ifndef SYS_PROC_H
#define SYS_PROC_H

#include <sys/queue.h>
#include <sys/types.h>

struct vmm_ctx;
struct fs_node;
struct filedesc;

struct proc
{
	struct vmm_ctx *vmm_ctx;
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
	LIST_HEAD(, thread) threads;
};

struct thread
{
	struct proc *proc;
	LIST_ENTRY(thread) thread_chain;
};

extern struct proc *curproc;
extern struct thread *curthread;

#endif
