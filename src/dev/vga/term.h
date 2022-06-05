#ifndef TERM_H
#define TERM_H

#include <stdint.h>
#include <stddef.h>

void term_initialize(void);
void term_setcolor(uint8_t color);
void term_putentryat(char c, uint8_t color, size_t x, size_t y);
void term_putchar(char c);
void term_put(const char *data, size_t size);
void term_putstr(const char *data);
void term_putint(int n);
void term_putuint(unsigned n);

#endif
