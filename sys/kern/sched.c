#include <sys/proc.h>
#include <stddef.h>

static TAILQ_HEAD(, thread) g_threads; /* prio-ordered */

void sched_init(void)
{
	TAILQ_INIT(&g_threads);
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

void sched_rm(struct thread *thread)
{
	TAILQ_REMOVE(&g_threads, thread, sched_chain);
}

static void change_thread(struct thread *thread)
{
	printf("changing thread from %p (%s) to %p (%s)\n", curthread, curthread->proc->name, thread, thread->proc->name);
	curthread->state = THREAD_PAUSED;
	curthread = thread;
	curproc = thread->proc;
	curthread->state = THREAD_RUNNING;
}

static struct thread *find_thread(void)
{
	struct thread *thread;
	TAILQ_FOREACH(thread, &g_threads, sched_chain)
	{
		if (thread->state == THREAD_PAUSED)
			return thread;
	}
	return NULL;
}

void sched_tick(void)
{
	if (curthread->state == THREAD_PAUSED)
	{
		struct thread *thread = find_thread();
		if (thread)
		{
			change_thread(thread);
			return;
		}
		panic("can't find paused thread to run");
		return;
	}
	struct thread *thread = NULL;
	TAILQ_FOREACH(thread, &g_threads, sched_chain)
	{
		if (thread->pri >= curthread->pri)
			return;
		if (thread->state == THREAD_PAUSED)
			break;
	}
	if (thread && thread->state == THREAD_PAUSED)
		change_thread(thread);
}
