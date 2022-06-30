#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>

#define M_ZERO (1 << 0)

void *malloc(size_t size, uint32_t flags);
void free(void *ptr);
void *realloc(void *ptr, size_t size, uint32_t flags);
void show_alloc_mem(void);

int atoi(const char *s);
int atoin(const char *s, size_t n);
void lltoa(char *d, long long int n);
void ulltoa(char *d, unsigned long long int n);
void ulltoa_base(char *d, unsigned long long int n, const char *base);

#endif
