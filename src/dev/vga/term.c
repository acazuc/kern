#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/string.h>

enum vga_color
{
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

size_t term_row;
size_t term_column;
uint8_t term_color;
static uint16_t * const g_term_buffer = (uint16_t*)0xB8000;

void term_initialize()
{
	term_row = 0;
	term_column = 0;
	term_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	for (size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			g_term_buffer[y * VGA_WIDTH + x] = vga_entry(' ', term_color);
		}
	}
}

void term_setcolor(uint8_t color)
{
	term_color = color;
}

void term_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	g_term_buffer[index] = vga_entry(c, color);
}

static void scroll_up(void)
{
	for (size_t y = 1; y < VGA_HEIGHT; ++y)
	{
		for (size_t x = 0; x < VGA_WIDTH; ++x)
			g_term_buffer[(y - 1) * VGA_WIDTH + x] = g_term_buffer[y * VGA_WIDTH + x];
	}
	for (size_t x = 0; x < VGA_WIDTH; ++x)
		g_term_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = 0;
}

void term_putchar(char c)
{
	if (term_row == VGA_HEIGHT)
	{
		scroll_up();
		term_row--;
	}
	if (c == '\n')
	{
		term_row++;
		term_column = 0;
		return;
	}

	term_putentryat(c, term_color, term_column, term_row);
	if (++term_column == VGA_WIDTH)
	{
		term_row++;
		term_column = 0;
	}
}

void term_put(const char *data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		term_putchar(data[i]);
}

void term_putstr(const char *data)
{
	term_put(data, strlen(data));
}

void term_putint(int n)
{
	if (n < 0)
	{
		term_putchar('-');
		n = -n;
	}
	if (n >= 10)
		term_putint(n / 10);
	term_putchar('0' + n % 10);
}

void term_putuint(unsigned n)
{
	if (n >= 10)
		term_putuint(n / 10);
	term_putchar('0' + n % 10);
}
