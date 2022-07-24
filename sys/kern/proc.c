#include <sys/sched.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/vmm.h>
#include <sys/std.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <arch.h>
#include <vfs.h>

static pid_t g_pid;
static pid_t g_tid;

static struct thread *proc_create(const char *name, struct vmm_ctx *vmm_ctx, void *entry)
{
	struct proc *proc = malloc(sizeof(*proc), M_ZERO);
	assert(proc, "can't alloc proc\n");
	struct thread *thread = malloc(sizeof(*thread), M_ZERO);
	assert(thread, "can't alloc thread\n");
	thread->proc = proc;
	proc->name = strdup(name);
	assert(proc->name, "can' dup proc name");
	proc->root = g_vfs_root;
	fs_node_incref(proc->root);
	proc->cwd = proc->root;
	fs_node_incref(proc->root);
	proc->files = NULL;
	proc->files_nb = 0;
	proc->vmm_ctx = vmm_ctx;
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
	CPUMASK_FILL(&thread->affinity);
	thread->state = THREAD_PAUSED;
	thread->stack_size = 1024 * 16;
	thread->stack = vmalloc_user(proc->vmm_ctx, (void*)(0xC0000000 - thread->stack_size), thread->stack_size); /* XXX aslr */
	assert(thread->stack, "can't allocate thread stack\n");
	thread->int_stack_size = 1024 * 4;
	thread->int_stack = vmalloc(thread->int_stack_size);
	assert(thread->int_stack, "can't allocate thread int stack\n");
	memset(thread->int_stack, 0, thread->int_stack_size); /* memory must be mapped */
	return thread;
}

void proc_push_argv_envp(struct thread *thread, const char * const *argv, const char * const *envp)
{
	/* XXX: check bounds of stack for cpy */
	/* XXX: don't map all the stack ? */
	char *stackorg = vmap_user(thread->proc->vmm_ctx, thread->stack, thread->stack_size);
	assert(stackorg, "failed to map stackp\n");
	char *stackp = stackorg + thread->stack_size;
	size_t argc = 0;
	size_t envc = 0;
	for (size_t i = 0; argv[i]; ++i)
	{
		size_t len = strlen(argv[i]);
		stackp -= len + 1;
		memcpy(stackp, argv[i], len + 1);
		argc++;
	}
	for (size_t i = 0; envp[i]; ++i)
	{
		size_t len = strlen(envp[i]);
		stackp -= len + 1;
		memcpy(stackp, envp[i], len + 1);
		envc++;
	}
	char *org = (char*)thread->stack + thread->stack_size;
	char *stack_argv;
	char *stack_envp;
	stackp -= sizeof(char*);
	*(char**)stackp = NULL;
	for (size_t i = argc; i > 0; --i)
	{
		org -= strlen(argv[i - 1]) + 1;
		stackp -= sizeof(char*);
		*(char**)stackp = org;
	}
	stack_argv = (char*)thread->stack + (stackp - stackorg);
	stackp -= sizeof(char*);
	*(char**)stackp = NULL;
	for (size_t i = envc; i > 0; --i)
	{
		org -= strlen(envp[i - 1]) + 1;
		stackp -= sizeof(char*);
		*(char**)stackp = org;
	}
	stack_envp = (char*)thread->stack + (stackp - stackorg);
	/* XXX parameters to _start are platform dependant */
	stackp -= sizeof(char*);
	*(char**)stackp = stack_envp;
	stackp -= sizeof(char*);
	*(char**)stackp = stack_argv;
	stackp -= sizeof(char*);
	*(int*)stackp = argc;
	stackp -= sizeof(char*); /* stack return value */
	*(int*)stackp = 0;
	thread->tf.esp = (uint32_t)(thread->stack + (stackp - stackorg));
	vunmap(stackorg, thread->stack_size);
}

struct thread *uproc_create(const char *name, void *entry, const char * const *argv, const char * const *envp)
{
	struct vmm_ctx *vmm_ctx = vmm_ctx_create();
	assert(vmm_ctx, "can't create vmm ctx\n");
	struct thread *thread = proc_create(name, vmm_ctx, entry);
	if (!thread)
	{
		vmm_ctx_delete(vmm_ctx);
		return NULL;
	}
	init_trapframe_user(thread);
	proc_push_argv_envp(thread, argv, envp);
	return thread;
}

struct thread *kproc_create(const char *name, void *entry, const char * const *argv, const char * const *envp)
{
	struct vmm_ctx *vmm_ctx = vmm_ctx_create();
	assert(vmm_ctx, "can't create vmm ctx\n");
	struct thread *thread = proc_create(name, vmm_ctx, entry);
	if (!thread)
	{
		vmm_ctx_delete(vmm_ctx);
		return NULL;
	}
	init_trapframe_kern(thread);
	proc_push_argv_envp(thread, argv, envp);
	return thread;
}

struct thread *uproc_create_elf(const char *name, struct file *file, const char * const *argv, const char * const *envp)
{
	struct vmm_ctx *vmm_ctx = vmm_ctx_create();
	assert(vmm_ctx, "can't create vmm ctx\n");
	void *entry;
	assert(!elf_createctx(file, vmm_ctx, &entry), "can't parse elf\n");
	struct thread *thread = proc_create(name, vmm_ctx, entry);
	if (!thread)
	{
		vmm_ctx_delete(vmm_ctx);
		return NULL;
	}
	init_trapframe_user(thread);
	proc_push_argv_envp(thread, argv, envp);
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
			file_incref(newp->files[i]);
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
	memcpy(&newt->tf, &thread->tf, sizeof(newt->tf));
	newt->stack_size = thread->stack_size;
	newt->stack = thread->stack; /* already copied by vmm_ctx_dup */
	newt->int_stack_size = thread->int_stack_size;
	newt->int_stack = vmalloc(newt->int_stack_size);
	assert(newt->int_stack, "can't allocate new thread int stack\n");
	memset(newt->int_stack, 0, thread->int_stack_size); /* memory must be mapped */
	newt->affinity = thread->affinity;
	newt->tid = ++g_tid;
	newt->pri = thread->pri;
	TAILQ_INSERT_TAIL(&newp->threads, newt, thread_chain);
	sched_add(newt);
	if (newt->state == THREAD_PAUSED)
		sched_run(newt);
	return newt;
}

int proc_execve(struct thread *thread, struct file *file, const char * const *argv, const char * const *envp)
{
	struct vmm_ctx *vmm_ctx = vmm_ctx_create();
	if (!vmm_ctx)
		return ENOMEM;
	void *entry;
	int ret = elf_createctx(file, vmm_ctx, &entry);
	if (ret)
		return ret;
	thread->proc->entrypoint = entry;
	vmm_ctx_delete(thread->proc->vmm_ctx);
	thread->proc->vmm_ctx = vmm_ctx;
	thread->stack = vmalloc_user(thread->proc->vmm_ctx, (void*)(0xC0000000 - thread->stack_size), thread->stack_size); /* XXX ASLR */
	init_trapframe_user(thread);
	proc_push_argv_envp(thread, argv, envp);
	return 0;
}

void proc_delete(struct proc *proc)
{
	struct thread *thread, *next;
	TAILQ_FOREACH_SAFE(thread, &proc->threads, thread_chain, next)
	{
		vfree(thread->int_stack, thread->int_stack_size);
		free(thread);
	}
	vmm_ctx_delete(proc->vmm_ctx);
	free(proc->name);
	for (size_t i = 0; i < proc->files_nb; ++i)
	{
		struct file *file = proc->files[i];
		if (!file)
			continue;
		file_decref(file);
	}
	free(proc->files);
	fs_node_decref(proc->root);
	fs_node_decref(proc->cwd);
	free(proc);
}
