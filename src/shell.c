#include "shell.h"
#include "dev/vga/vga.h"
#include "dev/pit/pit.h"
#include "dev/com/com.h"
#include "arch/arch.h"

#include <sys/std.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define INPUT_PREFIX "#> "
#define INPUT_PREFIX_LEN (sizeof(INPUT_PREFIX) - 1)

static uint8_t g_input_row;
static uint8_t g_input_col;
static uint8_t g_input_color;
static uint8_t g_row;
static uint8_t g_col;
static uint8_t g_color;
static char g_input[VGA_WIDTH + 1];
static int g_input_init;
static int g_first_char;

static void reset_input(void)
{
	uint16_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	for (size_t i = INPUT_PREFIX_LEN; i < VGA_WIDTH; ++i)
		vga_set_char(i, VGA_HEIGHT - 1, ' ', color);
	g_input_col = INPUT_PREFIX_LEN;
	vga_set_cursor(g_input_col, g_input_row);
	g_input[g_input_col] = '\0';
}

void shell_char_evt(const struct kbd_char_evt *evt)
{
	if (!g_input_init)
		return;
	if (g_input_col >= VGA_WIDTH)
		return;
	/* XXX: utf8 */
	vga_set_char(g_input_col, g_input_row, evt->utf8[0], g_input_color);
	g_input[g_input_col++] = evt->utf8[0];
	g_input[g_input_col] = '\0';
	vga_set_cursor(g_input_col, g_input_row);
}

static void print_time(void)
{
	struct timespec ts;
	pit_gettime(&ts);
	printf("[%llu] ", ts.tv_sec);
}

void shell_key_evt(const struct kbd_key_evt *evt)
{
	if (!g_input_init)
		return;
	if (!evt->pressed)
		return;
	if (evt->key == KBD_KEY_ENTER || evt->key == KBD_KEY_KP_ENTER)
	{
		const char *cmd = &g_input[INPUT_PREFIX_LEN];
		if (*cmd)
		{
			if (!strcmp(cmd, "time"))
			{
				struct timespec ts;
				pit_gettime(&ts);
				printf("%llu.%09llu\n", ts.tv_sec, ts.tv_nsec);
			}
			else if (!strcmp(cmd, "mem"))
			{
				show_alloc_mem();
				paging_dumpinfo();
			}
			else if (!strcmp(cmd, "panic"))
			{
				panic("shell\n");
			}
			else
			{
				shell_putstr("unknown command: ");
				shell_putline(cmd);
			}
			reset_input();
		}
		return;
	}
	if (evt->key == KBD_KEY_BACKSPACE)
	{
		if (g_input_col > INPUT_PREFIX_LEN)
		{
			g_input_col--;
			g_input[g_input_col] = '\0';
			vga_set_char(g_input_col, g_input_row, ' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
			vga_set_cursor(g_input_col, g_input_row);
		}
		return;
	}
}

void shell_input_init()
{
	g_input_row = VGA_HEIGHT - 1;
	g_input_col = INPUT_PREFIX_LEN;
	for (size_t i = 0; i < INPUT_PREFIX_LEN; ++i)
		vga_set_char(i, VGA_HEIGHT - 1, INPUT_PREFIX[i], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
	g_input[0] = '\0';
	g_input_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	g_input_init = 1;
	g_first_char = 1;
	vga_enable_cursor();
	vga_set_cursor(g_input_col, g_input_row);
}

void shell_init()
{
	g_row = 0;
	g_col = 0;
	g_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void scroll_up(void)
{
	for (size_t y = 1; y < VGA_HEIGHT - 1; ++y)
	{
		for (size_t x = 0; x < VGA_WIDTH; ++x)
			vga_set_val(x, y - 1, vga_get_val(x, y));
	}
	for (size_t x = 0; x < VGA_WIDTH; ++x)
		vga_set_val(x, VGA_HEIGHT - 2, 0);
}

void shell_putchar(char c)
{
	if (g_row == VGA_HEIGHT - 1)
	{
		scroll_up();
		g_row--;
		g_col = 0;
	}

	if (g_col == VGA_WIDTH - 1)
	{
		scroll_up();
		g_col = 0;
	}

	if (g_first_char)
	{
		g_first_char = 0;
		print_time();
	}

	com_putchar(c);
	if (c == '\n')
		com_putchar('\r');

	if (c == '\n')
	{
		g_row++;
		g_col = 0;
		g_first_char = 1;
		return;
	}

	vga_set_char(g_col, g_row, c, g_color);
	g_col++;
}

void shell_putstr(const char *s)
{
	for (size_t i = 0; s[i]; ++i)
		shell_putchar(s[i]);
}

void shell_putline(const char *s)
{
	shell_putstr(s);
	shell_putchar('\n');
}
