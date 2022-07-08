#include <sys/proc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sched.h>
#include <stdio.h>

#include "arch/x86/asm.h"
#include "fs/vfs.h"

struct thread *curthread;
struct proc *curproc;

static pid_t g_pid;

static struct thread *proc_create(const char *name, void *entry)
{
	struct proc *proc = malloc(sizeof(*proc), M_ZERO);
	assert(proc, "can't alloc proc\n");
	struct thread *thread = malloc(sizeof(*thread), M_ZERO);
	assert(thread, "can't alloc thread\n");
	thread->proc = proc;
	proc->name = strdup(name);
	assert(proc->name, "can' dup proc name");
	proc->root = g_vfs_root;
	proc->cwd = proc->root;
	proc->files = NULL;
	proc->files_nb = 0;
	proc->vmm_ctx = create_vmm_ctx();
	assert(proc->vmm_ctx, "vmm ctx allocation failed\n");
	TAILQ_INIT(&proc->threads);
	TAILQ_INSERT_HEAD(&proc->threads, thread, thread_chain);
	proc->entrypoint = entry;
	proc->pid = ++g_pid;
	proc->ppid = 1;
	proc->pgid = 1;
	proc->pgrp = 1;
	proc->uid = 0;
	proc->euid = 0;
	proc->suid = 0;
	proc->gid = 0;
	proc->egid = 0;
	proc->sgid = 0;
	thread->state = THREAD_PAUSED;
	thread->stack_size = 1024 * 16;
	thread->stack = vmalloc_user(proc->vmm_ctx, thread->stack_size);
	assert(thread->stack, "can't allocate thread stack\n");
	return thread;
}

struct thread *uproc_create(const char *name, void *entry)
{
	struct thread *thread = proc_create(name, entry);
	if (!thread)
		return NULL;
	init_trapframe_user(thread);
	return thread;
}

struct thread *kproc_create(const char *name, void *entry)
{
	struct thread *thread = proc_create(name, entry);
	if (!thread)
		return NULL;
	init_trapframe_kern(thread);
	return thread;
}

struct proc *proc_fork(struct proc *proc)
{
	struct proc *newp = malloc(sizeof(*newp), M_ZERO);
	assert(newp, "can't allocate new proc\n");
	newp->name = strdup(proc->name);
	assert(newp->name, "can't allocate new proc name\n");
	newp->vmm_ctx = vmm_ctx_dup(proc->vmm_ctx);
	newp->entrypoint = proc->entrypoint;
	newp->files = proc->files; /* XXX: incref all the files */
	newp->files_nb = proc->files_nb;
	newp->root = proc->root; /* XXX incref */
	newp->cwd = proc->cwd; /* incref */
	newp->umask = proc->umask;
	newp->pid = ++g_pid;
	newp->ppid = proc->pid;
	newp->pgid = proc->pgid;
	newp->pgrp = proc->pgrp;
	newp->uid = proc->uid;
	newp->euid = proc->euid;
	newp->suid = proc->suid;
	newp->gid = proc->gid;
	newp->egid = proc->egid;
	newp->sgid = proc->sgid;
	newp->pri = proc->pri;
	TAILQ_INIT(&newp->threads);
	struct thread *thread;
	TAILQ_FOREACH(thread, &proc->threads, thread_chain)
	{
		struct thread *newt = malloc(sizeof(*newt), M_ZERO);
		assert(newt, "can't allocate new thread\n");
		newt->proc = newp;
		newt->state = thread->state;
		if (newt->state == THREAD_RUNNING)
			newt->state = THREAD_PAUSED;
		newt->trapframe = thread->trapframe;
		newt->stack_size = thread->stack_size; /* XXX handle */
		newt->stack = thread->stack; /* XXX already copied by vmm_ctx_dup, what to do ? */
		newt->tid = thread->tid; /* XXX */
		newt->pri = thread->pri; /* XXX */
		if (thread == curthread) /* XXX: hask, set ret of fork to 0 for child */
			newt->trapframe.eax = 0;
		TAILQ_INSERT_TAIL(&newp->threads, newt, thread_chain);
	}
	TAILQ_FOREACH(thread, &newp->threads, thread_chain)
	{
		sched_add(thread);
		if (thread->state == THREAD_PAUSED)
			sched_run(thread);
	}
	return newp;
}
