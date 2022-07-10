#include <sys/proc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sched.h>
#include <sys/file.h>
#include <stdio.h>

#include "arch/x86/asm.h"
#include "fs/vfs.h"

struct thread *curthread;
struct proc *curproc;

static pid_t g_pid;
static pid_t g_tid;

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
	thread->stack = vmalloc_user(proc->vmm_ctx, (void*)(0xC0000000 - thread->stack_size), thread->stack_size);
	assert(thread->stack, "can't allocate thread stack\n");
	thread->int_stack_size = 1024 * 4;
	thread->int_stack = vmalloc(thread->int_stack_size);
	assert(thread->int_stack, "can't allocate thread int stack\n");
	memset(thread->int_stack, 0, thread->int_stack_size); /* memory must be mapped */
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

struct thread *proc_fork(struct thread *thread)
{
	struct proc *proc = thread->proc;
	struct proc *newp = malloc(sizeof(*newp), M_ZERO);
	assert(newp, "can't allocate new proc\n");
	newp->name = strdup(proc->name);
	assert(newp->name, "can't allocate new proc name\n");
	newp->vmm_ctx = vmm_ctx_dup(proc->vmm_ctx);
	newp->entrypoint = proc->entrypoint;
	newp->files = malloc(sizeof(*newp->files) * proc->files_nb, 0);
	assert(newp->files, "can't allocate new proc files\n");
	for (size_t i = 0; i < proc->files_nb; ++i)
	{
		newp->files[i] = proc->files[i];
		if (newp->files[i])
			newp->files[i]->refcount++;
	}
	newp->files_nb = proc->files_nb;
	newp->root = proc->root;
	newp->root->refcount++;
	newp->cwd = proc->cwd;
	newp->cwd->refcount++;
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
	struct thread *newt = malloc(sizeof(*newt), M_ZERO);
	assert(newt, "can't allocate new thread\n");
	newt->proc = newp;
	newt->state = thread->state;
	if (newt->state == THREAD_RUNNING)
		newt->state = THREAD_PAUSED;
	memcpy(&newt->trapframe, &thread->trapframe, sizeof(newt->trapframe));
	newt->stack_size = thread->stack_size;
	newt->stack = thread->stack; /* already copied by vmm_ctx_dup */
	newt->int_stack_size = thread->int_stack_size;
	newt->int_stack = vmalloc(newt->int_stack_size);
	assert(newt->int_stack, "can't allocate new thread int stack\n");
	memset(newt->int_stack, 0, thread->int_stack_size); /* memory must be mapped */
	newt->tid = ++g_tid;
	newt->pri = thread->pri;
	TAILQ_INSERT_TAIL(&newp->threads, newt, thread_chain);
	sched_add(newt);
	if (newt->state == THREAD_PAUSED)
		sched_run(newt);
	return newt;
}
