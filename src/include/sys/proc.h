#ifndef SYS_PROC_H
#define SYS_PROC_H

#include <sys/queue.h>
#include <sys/types.h>

struct vmm_ctx;

struct proc
{
	struct vmm_ctx *vmm_ctx;
	void *entrypoint;
	struct filedesc *files;
	uint32_t files_nb;
	pid_t pid;
	uid_t uid;
	gid_t gid;
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
