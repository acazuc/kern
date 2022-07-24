#ifndef MMAN_H
#define MMAN_H

#include <sys/types.h>
#include <stddef.h>

#define MAP_ANONYMOUS (1 << 0)

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);

#endif
