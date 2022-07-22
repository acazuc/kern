#ifndef MUTEX_H
#define MUTEX_H

#include <stddef.h>

typedef uintptr_t mutex_t;

static inline void mutex_init(mutex_t *mutex)
{
	*mutex = 0;
}

static inline void mutex_lock(mutex_t *mutex)
{
	while (__atomic_fetch_add(mutex, 1, __ATOMIC_ACQUIRE));
}

static inline int mutex_trylock(mutex_t *mutex)
{
	return __atomic_fetch_add(mutex, 1, __ATOMIC_ACQUIRE) != 0;
}

static inline void mutex_unlock(mutex_t *mutex)
{
	*mutex = 0;
}

#endif
