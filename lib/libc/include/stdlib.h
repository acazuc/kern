#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void show_alloc_mem(void);
void alloc_init(void);

int atoi(const char *s);
int atoin(const char *s, size_t n);
void lltoa(char *d, long long int n);
void ulltoa(char *d, unsigned long long int n);
void ulltoa_base(char *d, unsigned long long int n, const char *base);

void exit(int status);

#endif
