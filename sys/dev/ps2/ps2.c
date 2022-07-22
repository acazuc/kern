#include "ps2.h"
#include "arch/x86/asm.h"
#include "arch/x86/x86.h"

#include <sys/utf8.h>
#include <sys/kbd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <tty.h>

/* qwerty scan code set 1 table */
static const enum kbd_key g_qwerty_table[128] =
{
	/* 0x00 */ KBD_KEY_NONE  , KBD_KEY_ESCAPE  , KBD_KEY_1          , KBD_KEY_2,
	/* 0x04 */ KBD_KEY_3     , KBD_KEY_4       , KBD_KEY_5          , KBD_KEY_6,
	/* 0x08 */ KBD_KEY_7     , KBD_KEY_8       , KBD_KEY_9          , KBD_KEY_0,
	/* 0x0C */ KBD_KEY_MINUS , KBD_KEY_EQUAL   , KBD_KEY_BACKSPACE  , KBD_KEY_TAB,
	/* 0x10 */ KBD_KEY_Q     , KBD_KEY_W       , KBD_KEY_E          , KBD_KEY_R,
	/* 0x14 */ KBD_KEY_T     , KBD_KEY_Y       , KBD_KEY_U          , KBD_KEY_I,
	/* 0x18 */ KBD_KEY_O     , KBD_KEY_P       , KBD_KEY_LBRACKET   , KBD_KEY_RBRACKET,
	/* 0x1C */ KBD_KEY_ENTER , KBD_KEY_LCONTROL, KBD_KEY_A          , KBD_KEY_S,
	/* 0x20 */ KBD_KEY_D     , KBD_KEY_F       , KBD_KEY_G          , KBD_KEY_H,
	/* 0x24 */ KBD_KEY_J     , KBD_KEY_K       , KBD_KEY_L          , KBD_KEY_SEMICOLON,
	/* 0x28 */ KBD_KEY_SQUOTE, KBD_KEY_TILDE   , KBD_KEY_LSHIFT     , KBD_KEY_ANTISLASH,
	/* 0x2C */ KBD_KEY_Z     , KBD_KEY_X       , KBD_KEY_C          , KBD_KEY_V,
	/* 0x30 */ KBD_KEY_B     , KBD_KEY_N       , KBD_KEY_M          , KBD_KEY_COMMA,
	/* 0x34 */ KBD_KEY_DOT   , KBD_KEY_SLASH   , KBD_KEY_RSHIFT     , KBD_KEY_KP_MULT,
	/* 0x38 */ KBD_KEY_LALT  , KBD_KEY_SPACE   , KBD_KEY_CAPS_LOCK  , KBD_KEY_F1,
	/* 0x3C */ KBD_KEY_F2    , KBD_KEY_F3      , KBD_KEY_F4         , KBD_KEY_F5,
	/* 0x40 */ KBD_KEY_F6    , KBD_KEY_F7      , KBD_KEY_F8         , KBD_KEY_F9,
	/* 0x44 */ KBD_KEY_F10   , KBD_KEY_NUM_LOCK, KBD_KEY_SCROLL_LOCK, KBD_KEY_KP_7,
	/* 0x48 */ KBD_KEY_KP_8  , KBD_KEY_KP_9    , KBD_KEY_KP_MINUS   , KBD_KEY_KP_4,
	/* 0x4C */ KBD_KEY_KP_5  , KBD_KEY_KP_6    , KBD_KEY_KP_PLUS    , KBD_KEY_KP_1,
	/* 0x50 */ KBD_KEY_KP_2  , KBD_KEY_KP_3    , KBD_KEY_KP_0       , KBD_KEY_KP_DOT,
	/* 0x54 */ KBD_KEY_NONE  , KBD_KEY_NONE    , KBD_KEY_NONE       , KBD_KEY_F11,
	/* 0x58 */ KBD_KEY_F12,
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

static const uint32_t g_scancodes_normal[KBD_KEY_LAST] =
{
	[KBD_KEY_A]         = 0x61,
	[KBD_KEY_B]         = 0x62,
	[KBD_KEY_C]         = 0x63,
	[KBD_KEY_D]         = 0x64,
	[KBD_KEY_E]         = 0x65,
	[KBD_KEY_F]         = 0x66,
	[KBD_KEY_G]         = 0x67,
	[KBD_KEY_H]         = 0x68,
	[KBD_KEY_I]         = 0x69,
	[KBD_KEY_J]         = 0x6A,
	[KBD_KEY_K]         = 0x6B,
	[KBD_KEY_L]         = 0x6C,
	[KBD_KEY_M]         = 0x6D,
	[KBD_KEY_N]         = 0x6E,
	[KBD_KEY_O]         = 0x6F,
	[KBD_KEY_P]         = 0x70,
	[KBD_KEY_Q]         = 0x71,
	[KBD_KEY_R]         = 0x72,
	[KBD_KEY_S]         = 0x73,
	[KBD_KEY_T]         = 0x74,
	[KBD_KEY_U]         = 0x75,
	[KBD_KEY_V]         = 0x76,
	[KBD_KEY_W]         = 0x77,
	[KBD_KEY_X]         = 0x78,
	[KBD_KEY_Y]         = 0x79,
	[KBD_KEY_Z]         = 0x7A,
	[KBD_KEY_0]         = 0x30,
	[KBD_KEY_1]         = 0x31,
	[KBD_KEY_2]         = 0x32,
	[KBD_KEY_3]         = 0x33,
	[KBD_KEY_4]         = 0x34,
	[KBD_KEY_5]         = 0x35,
	[KBD_KEY_6]         = 0x36,
	[KBD_KEY_7]         = 0x37,
	[KBD_KEY_8]         = 0x38,
	[KBD_KEY_9]         = 0x39,
	[KBD_KEY_KP_0]      = 0x30,
	[KBD_KEY_KP_1]      = 0x31,
	[KBD_KEY_KP_2]      = 0x32,
	[KBD_KEY_KP_3]      = 0x33,
	[KBD_KEY_KP_4]      = 0x34,
	[KBD_KEY_KP_5]      = 0x35,
	[KBD_KEY_KP_6]      = 0x36,
	[KBD_KEY_KP_7]      = 0x37,
	[KBD_KEY_KP_8]      = 0x38,
	[KBD_KEY_KP_9]      = 0x39,
	[KBD_KEY_MINUS]     = 0x2D,
	[KBD_KEY_EQUAL]     = 0x3D,
	[KBD_KEY_LBRACKET]  = 0x5B,
	[KBD_KEY_RBRACKET]  = 0x5D,
	[KBD_KEY_SEMICOLON] = 0x3B,
	[KBD_KEY_SQUOTE]    = 0x27,
	[KBD_KEY_TILDE]     = 0x60,
	[KBD_KEY_ANTISLASH] = 0x5C,
	[KBD_KEY_COMMA]     = 0x2C,
	[KBD_KEY_DOT]       = 0x2E,
	[KBD_KEY_SLASH]     = 0x2F,
	[KBD_KEY_SPACE]     = 0x20,
	[KBD_KEY_ENTER]     = 0x0A,
	[KBD_KEY_BACKSPACE] = 0x08,
	[KBD_KEY_DELETE]    = 0x7F,
	[KBD_KEY_KP_MULT]   = 0x2A,
	[KBD_KEY_KP_MINUS]  = 0x2D,
	[KBD_KEY_KP_PLUS]   = 0x2B,
	[KBD_KEY_KP_DOT]    = 0x2E,
	[KBD_KEY_KP_SLASH]  = 0x2F,
	[KBD_KEY_KP_ENTER]  = 0x0A,
};

static const uint32_t g_scancodes_shift[KBD_KEY_LAST] =
{
	[KBD_KEY_A]         = 0x41,
	[KBD_KEY_B]         = 0x42,
	[KBD_KEY_C]         = 0x43,
	[KBD_KEY_D]         = 0x44,
	[KBD_KEY_E]         = 0x45,
	[KBD_KEY_F]         = 0x46,
	[KBD_KEY_G]         = 0x47,
	[KBD_KEY_H]         = 0x48,
	[KBD_KEY_I]         = 0x49,
	[KBD_KEY_J]         = 0x4A,
	[KBD_KEY_K]         = 0x4B,
	[KBD_KEY_L]         = 0x4C,
	[KBD_KEY_M]         = 0x4D,
	[KBD_KEY_N]         = 0x4E,
	[KBD_KEY_O]         = 0x4F,
	[KBD_KEY_P]         = 0x50,
	[KBD_KEY_Q]         = 0x51,
	[KBD_KEY_R]         = 0x52,
	[KBD_KEY_S]         = 0x53,
	[KBD_KEY_T]         = 0x54,
	[KBD_KEY_U]         = 0x55,
	[KBD_KEY_V]         = 0x56,
	[KBD_KEY_W]         = 0x57,
	[KBD_KEY_X]         = 0x58,
	[KBD_KEY_Y]         = 0x59,
	[KBD_KEY_Z]         = 0x5A,
	[KBD_KEY_0]         = 0x30,
	[KBD_KEY_1]         = 0x31,
	[KBD_KEY_2]         = 0x32,
	[KBD_KEY_3]         = 0x33,
	[KBD_KEY_4]         = 0x34,
	[KBD_KEY_5]         = 0x35,
	[KBD_KEY_6]         = 0x36,
	[KBD_KEY_7]         = 0x37,
	[KBD_KEY_8]         = 0x38,
	[KBD_KEY_9]         = 0x39,
	[KBD_KEY_KP_0]      = 0x30,
	[KBD_KEY_KP_1]      = 0x31,
	[KBD_KEY_KP_2]      = 0x32,
	[KBD_KEY_KP_3]      = 0x33,
	[KBD_KEY_KP_4]      = 0x34,
	[KBD_KEY_KP_5]      = 0x35,
	[KBD_KEY_KP_6]      = 0x36,
	[KBD_KEY_KP_7]      = 0x37,
	[KBD_KEY_KP_8]      = 0x38,
	[KBD_KEY_KP_9]      = 0x39,
	[KBD_KEY_MINUS]     = 0x5F,
	[KBD_KEY_EQUAL]     = 0x2B,
	[KBD_KEY_LBRACKET]  = 0x7B,
	[KBD_KEY_RBRACKET]  = 0x7D,
	[KBD_KEY_SEMICOLON] = 0x3A,
	[KBD_KEY_SQUOTE]    = 0x22,
	[KBD_KEY_TILDE]     = 0x7E,
	[KBD_KEY_ANTISLASH] = 0x7C,
	[KBD_KEY_COMMA]     = 0x3C,
	[KBD_KEY_DOT]       = 0x3E,
	[KBD_KEY_SLASH]     = 0x3F,
	[KBD_KEY_SPACE]     = 0x20,
	[KBD_KEY_KP_MULT]   = 0x2A,
	[KBD_KEY_KP_MINUS]  = 0x2D,
	[KBD_KEY_KP_PLUS]   = 0x2B,
	[KBD_KEY_KP_DOT]    = 0x2E,
	[KBD_KEY_KP_SLASH]  = 0x2F,
};

static const uint32_t g_scancodes_control[KBD_KEY_LAST] =
{
	[KBD_KEY_A]         = 0x01,
	[KBD_KEY_B]         = 0x02,
	[KBD_KEY_C]         = 0x03,
	[KBD_KEY_D]         = 0x04,
	[KBD_KEY_E]         = 0x05,
	[KBD_KEY_F]         = 0x06,
	[KBD_KEY_G]         = 0x07,
	[KBD_KEY_H]         = 0x08,
	[KBD_KEY_I]         = 0x09,
	[KBD_KEY_J]         = 0x0A,
	[KBD_KEY_K]         = 0x0B,
	[KBD_KEY_L]         = 0x0C,
	[KBD_KEY_M]         = 0x0D,
	[KBD_KEY_N]         = 0x0E,
	[KBD_KEY_O]         = 0x0F,
	[KBD_KEY_P]         = 0x10,
	[KBD_KEY_Q]         = 0x11,
	[KBD_KEY_R]         = 0x12,
	[KBD_KEY_S]         = 0x13,
	[KBD_KEY_T]         = 0x14,
	[KBD_KEY_U]         = 0x15,
	[KBD_KEY_V]         = 0x16,
	[KBD_KEY_W]         = 0x17,
	[KBD_KEY_X]         = 0x18,
	[KBD_KEY_Y]         = 0x19,
	[KBD_KEY_Z]         = 0x1A,
	[KBD_KEY_LBRACKET]  = 0x1B,
	[KBD_KEY_ANTISLASH] = 0x1C,
	[KBD_KEY_RBRACKET]  = 0x1D,
};

static uint8_t g_kbd_buffer[6];
static size_t g_kbd_buffer_size;

static enum kbd_mod g_kbd_mods;

static void kbd_interrupt(const struct int_ctx *ctx);

static uint32_t get_scancode(enum kbd_key key, uint32_t mods)
{
	if (mods & (KBD_MOD_ALT | KBD_MOD_CONTROL))
		return 0;
	if (mods & KBD_MOD_SHIFT)
		return g_scancodes_shift[key];
	if (mods & KBD_MOD_CONTROL)
		return g_scancodes_control[key];
	return g_scancodes_normal[key];
}

void ps2_init()
{
	set_isa_irq_handler(ISA_IRQ_KBD, kbd_interrupt);
	enable_isa_irq(ISA_IRQ_KBD);
}

static void kbd_interrupt(const struct int_ctx *ctx)
{
	(void)ctx;
	uint8_t c = inb(0x60);
	if (c == 0xE0)
	{
		if (g_kbd_buffer_size != 0)
		{
			printf("invalid kbd escape code (too long)");
			goto end;
		}
		g_kbd_buffer[g_kbd_buffer_size++] = c;
		goto end;
	}
	int release = 0;
	if (c & 0x80)
		release = 1;
	enum kbd_key key;
	g_kbd_buffer[g_kbd_buffer_size++] = c;
	if (g_kbd_buffer_size == 1)
	{
		key = g_qwerty_table[g_kbd_buffer[0] & 0x7F];
		g_kbd_buffer_size = 0;
		if (key == KBD_KEY_NONE)
			goto end;
	}
	else if (g_kbd_buffer_size == 2 && g_kbd_buffer[0] == 0xE0)
	{
		key = g_qwerty_table_ext[g_kbd_buffer[1] & 0x7F];
		g_kbd_buffer_size = 0;
		if (key == KBD_KEY_NONE)
			goto end;
	}
	else
	{
		goto end;
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
		struct kbd_char_evt char_evt;
		char_evt.scancode = get_scancode(key, g_kbd_mods);
		if (char_evt.scancode)
		{
			char *dst = (char*)char_evt.utf8;
			memset(char_evt.utf8, 0, sizeof(char_evt.utf8));
			if (utf8_encode(&dst, char_evt.scancode))
			{
				tty_input(curtty, char_evt.utf8, strlen(char_evt.utf8));
			}
			else
			{
				printf("can't utf8 encode\n");
			}
		}
	}
end:
	isa_eoi(ISA_IRQ_KBD);
}
