#include "shell.h"

#include "dev/vga/vga.h"
#include "sys/std.h"
#include "dev/pit/pit.h"

#include <stddef.h>
#include <stdbool.h>

#define INPUT_PREFIX "#> "
#define INPUT_PREFIX_LEN (sizeof(INPUT_PREFIX) - 1)

static uint8_t g_input_row;
static uint8_t g_input_col;
static uint8_t g_input_color;
static uint8_t g_row;
static uint8_t g_col;
static uint8_t g_color;
static char g_input[VGA_WIDTH + 1];
static bool g_input_init;
static bool g_first_char;

static void reset_input(void)
{
	uint16_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	for (size_t i = INPUT_PREFIX_LEN; i < VGA_WIDTH; ++i)
		vga_set_char(i, VGA_HEIGHT - 1, ' ', color);
	g_input_col = INPUT_PREFIX_LEN;
	g_input[g_input_col] = '\0';
}

void shell_char_evt(const kbd_char_evt_t *evt)
{
	if (!g_input_init)
		return;
	if (g_input_col >= VGA_WIDTH)
		return;
	/* XXX: utf8 */
	vga_set_char(g_input_col, g_input_row, evt->utf8[0], g_input_color);
	g_input[g_input_col++] = evt->utf8[0];
	g_input[g_input_col] = '\0';
}

static void print_time(void)
{
	struct timespec ts;
	pit_gettime(&ts);
	printf("[%llu] ", ts.tv_sec);
}

void shell_key_evt(const kbd_key_evt_t *evt)
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
	g_input_init = true;
	g_first_char = true;
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
		g_first_char = false;
		print_time();
	}

	if (c == '\n')
	{
		g_row++;
		g_col = 0;
		g_first_char = true;
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
