#include <sys/proc.h>
#include <stdlib.h>
#include <string.h>

#include "arch/x86/asm.h"
#include "fs/vfs.h"

extern struct fs_node g_ramfs_root;
extern uint8_t g_userland_stack[];

struct thread *curthread;
struct proc *curproc;

static struct thread *proc_create(const char *name, void *entry)
{
	struct proc *proc = malloc(sizeof(*proc), M_ZERO);
	assert(proc, "can't alloc proc\n");
	struct thread *thread = malloc(sizeof(*thread), M_ZERO);
	assert(thread, "can't alloc thread\n");
	thread->proc = proc;
	proc->name = strdup(name);
	assert(proc->name, "can' dup proc name");
	proc->root = &g_ramfs_root;
	proc->cwd = proc->root;
	proc->files = NULL;
	proc->files_nb = 0;
	proc->vmm_ctx = create_vmm_ctx();
	if (!proc->vmm_ctx)
		panic("vmm ctx allocation failed\n");
	TAILQ_INIT(&proc->threads);
	TAILQ_INSERT_HEAD(&proc->threads, thread, thread_chain);
	proc->entrypoint = entry;
	proc->ppid = 1;
	proc->pid = 1;
	proc->pgid = 1;
	proc->pgrp = 1;
	proc->uid = 0;
	proc->euid = 0;
	proc->suid = 0;
	proc->gid = 0;
	proc->egid = 0;
	proc->sgid = 0;
	thread->state = THREAD_PAUSED;
	/* XXX: frame is arch-dependant */
	thread->frame.eax = 0;
	thread->frame.ebx = 0;
	thread->frame.ecx = 0;
	thread->frame.edx = 0;
	thread->frame.esi = 0;
	thread->frame.edi = 0;
	thread->frame.esp = (uint32_t)&g_userland_stack[4096 * 4];
	thread->frame.ebp = (uint32_t)&g_userland_stack[4096 * 4];
	thread->frame.eip = (uint32_t)entry;
	return thread;
}

struct thread *uproc_create(const char *name, void *entry)
{
	struct thread *thread = proc_create(name, entry);
	if (!thread)
		return NULL;
	/* XXX: frame is arch-dependant */
	thread->frame.cs = 0x1B;
	thread->frame.ds = 0x23;
	thread->frame.es = 0x23;
	thread->frame.fs = 0x23;
	thread->frame.gs = 0x23;
	thread->frame.ss = 0x23;
	thread->frame.ef = getef() | (1 << 9); /* IF */
	return thread;
}

struct thread *kproc_create(const char *name, void *entry)
{
	struct thread *thread = proc_create(name, entry);
	if (!thread)
		return NULL;
	/* XXX: frame is arch-dependant */
	thread->frame.cs = 0x08;
	thread->frame.ds = 0x10;
	thread->frame.es = 0x10;
	thread->frame.fs = 0x10;
	thread->frame.gs = 0x10;
	thread->frame.ss = 0x10;
	thread->frame.ef = getef();
	return thread;
}
