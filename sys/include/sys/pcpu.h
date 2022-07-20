#ifndef SYS_PCPU_H
#define SYS_PCPU_H

#include "arch/arch.h"

struct pcpu
{
	uint8_t *stack;
	size_t stack_size;
	struct thread *thread;
	struct thread *idlethread;
};

extern struct pcpu pcpu[MAXCPU];

#define curthread pcpu[curcpu()].thread
#define idlethread pcpu[curcpu()].idlethread

#define CPUMASK_WORDS() ((MAXCPU + sizeof(size_t) * 8 - 1) / (sizeof(size_t) * 8))
#define CPUMASK_BIT(n) (1 << ((n) % (sizeof(size_t) * 8)))
#define CPUMASK_OFF(n) ((n) / (sizeof(size_t) * 8))
#define CPUMASK_FILL(m) \
do \
{ \
	for (size_t i = 0; i < CPUMASK_WORDS(); ++i) \
		(m)->mask[i] = (size_t)-1; \
} while (0)
#define CPUMASK_CLEAR(m) \
do \
{ \
	for (size_t i = 0; i < CPUMASK_WORDS(); ++i) \
		(m)->mask[i] = 0; \
} while (0)

typedef struct cpumask
{
	size_t mask[CPUMASK_WORDS()];
} cpumask_t;

#define CPUMASK_GET(m, n) (mask->mask[CPUMASK_OFF(n)] & CPUMASK_BIT(n))
#define CPUMASK_SET(m, n, v) \
do \
{ \
	if (v) \
		(m)->mask[CPUMASK_OFF(n)] |= CPUMASK_BIT(n); \
	else \
		(m)->mask[CPUMASK_OFF(n)] &= ~CPUMASK_BIT(n); \
} while (0)

#endif
