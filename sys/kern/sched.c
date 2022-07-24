#include <sys/proc.h>
#include <sys/pcpu.h>
#include <inttypes.h>
#include <sys/std.h>
#include <sys/vmm.h>
#include <stddef.h>
#include <stdio.h>
#include <arch.h>

static TAILQ_HEAD(, thread) g_threads; /* prio-ordered */
static TAILQ_HEAD(, thread) g_queue; /* schedulable threads */

void sched_init(void)
{
	TAILQ_INIT(&g_threads);
	TAILQ_INIT(&g_queue);
}

void sched_add(struct thread *thread)
{
	struct thread *it;
	TAILQ_FOREACH(it, &g_threads, sched_chain)
	{
		if (thread->pri >= it->pri)
			continue;
		TAILQ_INSERT_BEFORE(it, thread, sched_chain);
		return;
	}
	TAILQ_INSERT_TAIL(&g_threads, thread, sched_chain);
}

void sched_run(struct thread *thread)
{
	struct thread *it;
	TAILQ_FOREACH(it, &g_queue, queue_chain)
	{
		if (thread->pri >= it->pri)
			continue;
		TAILQ_INSERT_BEFORE(it, thread, queue_chain);
		return;
	}
	TAILQ_INSERT_TAIL(&g_queue, thread, queue_chain);
}

void sched_rm(struct thread *thread)
{
	TAILQ_REMOVE(&g_threads, thread, sched_chain);
}

void sched_switch(struct thread *thread)
{
	if (thread == curthread)
		return;
#if 0
	printf("changing thread from %p (%s; %#" PRIx32 ") to %p (%s; %#" PRIx32 ")\n", curthread, curthread ? curthread->proc->name : "", curthread ? curthread->tf.eip : 0, thread, thread->proc->name, thread->tf.eip);
#endif
	if (curthread)
	{
		if (curthread->state == THREAD_RUNNING)
			sched_run(curthread);
		curthread->state = THREAD_PAUSED;
	}
	curthread = thread;
	curthread->proc = thread->proc;
	curthread->state = THREAD_RUNNING;
	TAILQ_REMOVE(&g_queue, thread, queue_chain);
	vmm_setctx(curthread->proc->vmm_ctx);
}

static struct thread *find_thread(void)
{
	struct thread *thread;
	TAILQ_FOREACH(thread, &g_queue, queue_chain)
	{
		if (thread->state == THREAD_PAUSED)
			return thread;
	}
	return NULL;
}

void sched_tick(void)
{
	/* XXX: do a scheduling tick only on time interval */
	if (curthread && curthread->state == THREAD_PAUSED)
	{
		struct thread *thread = find_thread();
		if (thread)
		{
			sched_switch(thread);
			return;
		}
		panic("can't find paused thread to run\n");
		return;
	}
	struct thread *thread = NULL;
	TAILQ_FOREACH(thread, &g_queue, queue_chain)
	{
		if (thread == curthread) /* XXX: shouldn't happen */
			continue;
		if (curthread && thread->pri > curthread->pri)
			return;
		if (thread->state == THREAD_PAUSED)
			break;
	}
	if (thread && thread->state == THREAD_PAUSED)
		sched_switch(thread);
}
