/*
 * pckeymap.c
 *
 * Mappings from IBM-PC scancodes to local key codes.
 */

#include "input.h"

int keymap[][2] =
{
	{ 0x0C, K_ESC },
	{ 1, '1' },
	{ 2, '2' },
	{ 3, '3' },
	{ 4, '4' },
	{ 5, '5' },
	{ 6, '6' },
	{ 7, '7' },
	{ 8, '8' },
	{ 9, '9' },
	{ 0, '0' },
	{ 0x3B, K_ENTER }, // menu: start
	{ 0x2D, K_CTRL }, // info: b
	{ 0x55, K_ALT }, // opt: a
	{ 0x52, ' ' }, // mark: select
	{ 0x0E, K_UP },
	{ 0x2F, K_LEFT },
	{ 0x2E, K_RIGHT },
	{ 0x0F, K_DOWN },
	{ 0x20, K_INS }, // radio: save state
	{ 0x27, K_DEL }, // tv: return to saved state

	{ 0, 0 }
};









