#ifndef SHELL_H
#define SHELL_H

#include "sys/kbd.h"

void shell_init(void);
void shell_input_init(void);
void shell_char_evt(const kbd_char_evt_t *evt);
void shell_key_evt(const kbd_key_evt_t *evt);
void shell_putstr(const char *s);
void shell_putline(const char *s);

#endif
