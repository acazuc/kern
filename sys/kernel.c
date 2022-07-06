#include <sys/proc.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "arch/x86/asm.h"
#include "arch/arch.h"
#include "fs/vfs.h"

static void infl(void)
{
loop:
	hlt();
	goto loop;
}

void userland(void);

extern struct fs_node g_ramfs_root;

struct proc *curproc;
struct thread *curthread;

static struct thread *proc_create_user(void *entry)
{
	struct proc *proc = malloc(sizeof(*proc), 0);
	if (!proc)
		panic("can't alloc proc\n");
	struct thread *thread = malloc(sizeof(*thread), 0);
	if (!thread)
		panic("can't alloc thread\n");
	thread->proc = proc;
	proc->root = &g_ramfs_root;
	proc->cwd = proc->root;
	proc->files = NULL;
	proc->files_nb = 0;
	proc->vmm_ctx = create_vmm_ctx();
	if (!proc->vmm_ctx)
		panic("vmm ctx allocation failed\n");
	LIST_INIT(&proc->threads);
	LIST_INSERT_HEAD(&proc->threads, thread, thread_chain);
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
	return thread;
}

void kernel_main(struct multiboot_info *mb_info)
{
	boot(mb_info);
	printf("boot end\n");
	struct thread *thread = proc_create_user(userland);
	curthread = thread;
	curproc = thread->proc;
	usermode(curproc->entrypoint);
	printf("past usermode\n");
	infl();
}
