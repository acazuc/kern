#include "ps2.h"

#include "arch/x86/io.h"
#include "sys/std.h"
#include "sys/kbd.h"
#include "sys/utf8.h"
#include "shell.h"

#include <stdbool.h>
#include <stdint.h>

/* qwerty scan code set 1 table */
static const enum kbd_key g_qwerty_table[128] =
{
	KBD_KEY_NONE,
	KBD_KEY_ESCAPE,
	KBD_KEY_1,
	KBD_KEY_2,
	KBD_KEY_3,
	KBD_KEY_4,
	KBD_KEY_5,
	KBD_KEY_6,
	KBD_KEY_7,
	KBD_KEY_8,
	KBD_KEY_9,
	KBD_KEY_0,
	KBD_KEY_MINUS,
	KBD_KEY_EQUAL,
	KBD_KEY_BACKSPACE,
	KBD_KEY_TAB,
	KBD_KEY_Q,
	KBD_KEY_W,
	KBD_KEY_E,
	KBD_KEY_R,
	KBD_KEY_T,
	KBD_KEY_Y,
	KBD_KEY_U,
	KBD_KEY_I,
	KBD_KEY_O,
	KBD_KEY_P,
	KBD_KEY_LBRACKET,
	KBD_KEY_RBRACKET,
	KBD_KEY_ENTER,
	KBD_KEY_LCONTROL,
	KBD_KEY_A,
	KBD_KEY_S,
	KBD_KEY_D,
	KBD_KEY_F,
	KBD_KEY_G,
	KBD_KEY_H,
	KBD_KEY_J,
	KBD_KEY_K,
	KBD_KEY_L,
	KBD_KEY_SEMICOLON,
	KBD_KEY_SQUOTE,
	KBD_KEY_TILDE,
	KBD_KEY_LSHIFT,
	KBD_KEY_ANTISLASH,
	KBD_KEY_Z,
	KBD_KEY_X,
	KBD_KEY_C,
	KBD_KEY_V,
	KBD_KEY_B,
	KBD_KEY_N,
	KBD_KEY_M,
	KBD_KEY_COMMA,
	KBD_KEY_DOT,
	KBD_KEY_SLASH,
	KBD_KEY_RSHIFT,
	KBD_KEY_KP_MULT,
	KBD_KEY_LALT,
	KBD_KEY_SPACE,
	KBD_KEY_CAPS_LOCK,
	KBD_KEY_F1,
	KBD_KEY_F2,
	KBD_KEY_F3,
	KBD_KEY_F4,
	KBD_KEY_F5,
	KBD_KEY_F6,
	KBD_KEY_F7,
	KBD_KEY_F8,
	KBD_KEY_F9,
	KBD_KEY_F10,
	KBD_KEY_NUM_LOCK,
	KBD_KEY_SCROLL_LOCK,
	KBD_KEY_KP_7,
	KBD_KEY_KP_8,
	KBD_KEY_KP_9,
	KBD_KEY_KP_MINUS,
	KBD_KEY_KP_4,
	KBD_KEY_KP_5,
	KBD_KEY_KP_6,
	KBD_KEY_KP_PLUS,
	KBD_KEY_KP_1,
	KBD_KEY_KP_2,
	KBD_KEY_KP_3,
	KBD_KEY_KP_0,
	KBD_KEY_KP_DOT,
	KBD_KEY_NONE,
	KBD_KEY_NONE,
	KBD_KEY_NONE,
	KBD_KEY_F11,
	KBD_KEY_F12,
};

/* extended qwerty scan code 1 table (0xE0 prefixed) */
static const enum kbd_key g_qwerty_table_ext[128] =
{
	[0x10] = KBD_KEY_MM_PREV_TRACK,
	[0x19] = KBD_KEY_MM_NEXT_TRACK,
	[0x1C] = KBD_KEY_KP_ENTER,
	[0x1D] = KBD_KEY_RCONTROL,
	[0x20] = KBD_KEY_MM_MUTE,
	[0x21] = KBD_KEY_MM_CALC,
	[0x22] = KBD_KEY_MM_PLAY,
	[0x24] = KBD_KEY_MM_STOP,
	[0x2E] = KBD_KEY_MM_VOLUME_DOWN,
	[0x30] = KBD_KEY_MM_VOLUME_UP,
	[0x32] = KBD_KEY_MM_WWW,
	[0x35] = KBD_KEY_KP_SLASH,
	[0x38] = KBD_KEY_RALT,
	[0x47] = KBD_KEY_HOME,
	[0x48] = KBD_KEY_CURSOR_UP,
	[0x49] = KBD_KEY_PGUP,
	[0x4B] = KBD_KEY_CURSOR_LEFT,
	[0x4D] = KBD_KEY_CURSOR_RIGHT,
	[0x4F] = KBD_KEY_END,
	[0x50] = KBD_KEY_CURSOR_DOWN,
	[0x51] = KBD_KEY_PGDOWN,
	[0x52] = KBD_KEY_INSERT,
	[0x53] = KBD_KEY_DELETE,
	[0x5B] = KBD_KEY_LGUI,
	[0x5C] = KBD_KEY_RGUI,
	[0x5D] = KBD_KEY_APPS,
	[0x5E] = KBD_KEY_ACPI_POWER,
	[0x5F] = KBD_KEY_ACPI_SLEEP,
	[0x63] = KBD_KEY_ACPI_WAKE,
	[0x65] = KBD_KEY_MM_WWW_SEARCH,
	[0x66] = KBD_KEY_MM_WWW_FAVORITES,
	[0x67] = KBD_KEY_MM_WWW_REFRESH,
	[0x68] = KBD_KEY_MM_WWW_STOP,
	[0x69] = KBD_KEY_MM_WWW_FORWARD,
	[0x6A] = KBD_KEY_MM_WWW_BACK,
	[0x6B] = KBD_KEY_MM_WWW_COMPUTER,
	[0x6C] = KBD_KEY_MM_EMAIL,
	[0x6D] = KBD_KEY_MM_MEDIA_SELECT,
};

static const uint32_t g_scancodes[KBD_KEY_LAST * 2] =
{
	[KBD_KEY_A * 2] = 0x61, 0x41,
	0x62, 0x42,
	0x63, 0x43,
	0x64, 0x44,
	0x65, 0x45,
	0x66, 0x46,
	0x67, 0x47,
	0x68, 0x48,
	0x69, 0x49,
	0x6A, 0x4A,
	0x6B, 0x4B,
	0x6C, 0x4C,
	0x6D, 0x4D,
	0x6E, 0x4E,
	0x6F, 0x4F,
	0x70, 0x50,
	0x71, 0x51,
	0x72, 0x52,
	0x73, 0x53,
	0x74, 0x54,
	0x75, 0x55,
	0x76, 0x56,
	0x77, 0x57,
	0x78, 0x58,
	0x79, 0x59,
	0x7A, 0x5A,
	[KBD_KEY_0 * 2] = 0x30, 0x30,
	0x31, 0x31,
	0x32, 0x32,
	0x33, 0x33,
	0x34, 0x34,
	0x35, 0x35,
	0x36, 0x36,
	0x37, 0x37,
	0x38, 0x38,
	0x39, 0x39,
	[KBD_KEY_KP_0 * 2] = 0x30, 0x30,
	0x31, 0x31,
	0x32, 0x32,
	0x33, 0x33,
	0x34, 0x34,
	0x35, 0x35,
	0x36, 0x36,
	0x37, 0x37,
	0x38, 0x38,
	0x39, 0x39,
	[KBD_KEY_MINUS * 2]     = 0x2D, 0x5F,
	[KBD_KEY_EQUAL * 2]     = 0x3D, 0x2B,
	[KBD_KEY_LBRACKET * 2]  = 0x5B, 0x7B,
	[KBD_KEY_RBRACKET * 2]  = 0x5D, 0x7D,
	[KBD_KEY_SEMICOLON * 2] = 0x3B, 0x3A,
	[KBD_KEY_SQUOTE * 2]    = 0x27, 0x22,
	[KBD_KEY_TILDE * 2]     = 0x60, 0x7E,
	[KBD_KEY_ANTISLASH * 2] = 0x5C, 0x7C,
	[KBD_KEY_COMMA * 2]     = 0x2C, 0x3C,
	[KBD_KEY_DOT * 2]       = 0x2E, 0x3E,
	[KBD_KEY_SLASH * 2]     = 0x2F, 0x3F,
	[KBD_KEY_SPACE * 2]     = 0x20, 0x20,
	[KBD_KEY_KP_MULT * 2]   = 0x2A, 0x2A,
	[KBD_KEY_KP_MINUS * 2]  = 0x2D, 0x2D,
	[KBD_KEY_KP_PLUS * 2]   = 0x2B, 0x2B,
	[KBD_KEY_KP_DOT * 2]    = 0x2E, 0x2E,
	[KBD_KEY_KP_SLASH * 2]  = 0x2F, 0x2F,
};

static uint8_t g_kbd_buffer[6];
static size_t g_kbd_buffer_size;

static enum kbd_mod g_kbd_mods;

static uint32_t get_scancode(enum kbd_key key, uint32_t mods)
{
	if (mods & (KBD_MOD_ALT | KBD_MOD_CONTROL))
		return 0;
	uint32_t idx = key * 2;
	if (mods & KBD_MOD_SHIFT)
		idx++;
	return g_scancodes[idx];
}

void ps2_init()
{
}

void ps2_interrupt()
{
	uint8_t c = inb(0x60);
	if (c == 0xE0)
	{
		if (g_kbd_buffer_size != 0)
		{
			printf("invalid kbd escape code (too long)");
			return;
		}
		g_kbd_buffer[g_kbd_buffer_size++] = c;
		return;
	}
	bool release = false;
	if (c & 0x80)
		release = true;
	enum kbd_key key;
	g_kbd_buffer[g_kbd_buffer_size++] = c;
	if (g_kbd_buffer_size == 1)
	{
		key = g_qwerty_table[g_kbd_buffer[0] & 0x7F];
		g_kbd_buffer_size = 0;
		if (key == KBD_KEY_NONE)
			return;
	}
	else if (g_kbd_buffer_size == 2 && g_kbd_buffer[0] == 0xE0)
	{
		key = g_qwerty_table_ext[g_kbd_buffer[1] & 0x7F];
		g_kbd_buffer_size = 0;
		if (key == KBD_KEY_NONE)
			return;
	}
	else
	{
		return;
	}
	switch (key)
	{
		case KBD_KEY_LCONTROL: /* FALLTHROUGH */
		case KBD_KEY_RCONTROL:
			if (release)
				g_kbd_mods &= ~KBD_MOD_CONTROL;
			else
				g_kbd_mods |= KBD_MOD_CONTROL;
			break;
		case KBD_KEY_LSHIFT: /* FALLTHROUGH */
		case KBD_KEY_RSHIFT:
			if (release)
				g_kbd_mods &= ~KBD_MOD_SHIFT;
			else
				g_kbd_mods |= KBD_MOD_SHIFT;
			break;
		case KBD_KEY_LALT: /* FALLTHROUGH */
		case KBD_KEY_RALT:
			if (release)
				g_kbd_mods &= ~KBD_MOD_ALT;
			else
				g_kbd_mods |= KBD_MOD_ALT;
			break;
		default:
			break;
	}
	/* XXX: generic input events handlers */
	if (!release)
	{
		kbd_char_evt_t char_evt;
		char_evt.scancode = get_scancode(key, g_kbd_mods);
		if (char_evt.scancode)
		{
			char *dst = (char*)char_evt.utf8;
			if (utf8_encode(&dst, char_evt.scancode))
			{
				shell_char_evt(&char_evt);
			}
			else
			{
				printf("can't utf8 encode\n");
			}
		}
	}
	kbd_key_evt_t key_evt;
	key_evt.key = key;
	key_evt.mod = g_kbd_mods;
	key_evt.pressed = !release;
	shell_key_evt(&key_evt);
}
