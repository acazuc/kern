#ifndef SHELL_H
#define SHELL_H

#include "sys/kbd.h"
#include <stdlib.h>

void shell_init(void);
void shell_input_init(void);
void shell_char_evt(const struct kbd_char_evt *evt);
void shell_key_evt(const struct kbd_key_evt *evt);
void shell_putstr(const char *s);
void shell_putdata(const void *d, size_t len);
void shell_putline(const char *s);

#endif
