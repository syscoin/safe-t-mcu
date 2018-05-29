/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "trezor.h"
#include "debug.h"
#include "oled.h"
#include "util.h"

#if DEBUG_LOG

void oledDebug(const char *line)
{
	static const char *lines[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	static char id = 3;
	for (int i = 0; i < 7; i++) {
		lines[i] = lines[i + 1];
	}
	lines[7] = line;
	oledClear();
	for (int i = 0; i < 8; i++) {
		if (lines[i]) {
			oledDrawChar(0, i * 8, '0' + (id + i) % 10, FONT_STANDARD);
			oledDrawString(8, i * 8, lines[i], FONT_STANDARD);
		}
	}
	oledRefresh();
	id = (id + 1) % 10;
}

void __attribute__ ((noinline)) debugLog(int level, const char *bucket, const char *text)
{
#if DEBUG_GDB
	asm volatile(""::"r" (level), "r" (bucket), "r" (text)); // to prevent the compiler from optimizing away calls to the function
#else
	(void)level;
	(void)bucket;
#if EMULATOR
	puts(text);
#else
	oledDebug(text);
#endif
#endif
}

char *debugInt(const uint32_t i)
{
	static uint8_t n = 0;
	static char id[8][9];
	uint32hex(i, id[n]);
	debugLog(0, "", id[n]);
	char *ret = (char *)id[n];
	n = (n + 1) % 8;
	return ret;
}


void debugHex(const uint8_t i)
{
#define NIBBLE_TO_HEX(x) (x) > 9 ? 'A' + ((x)-10) : '0' + (x)

	static char s[3];

	s[0] = NIBBLE_TO_HEX(i>>4);
	s[1] = NIBBLE_TO_HEX(i&0xF);
	s[2] = 0;
	debugLog(0, "", s);
}

void debugHexDump(const char *text, const uint8_t *p, const uint8_t len)
{
	{
		static char s[256];
#include <string.h>
		strcpy(s, text);
		char *hex_string = s + strlen(s);

		for (int i=0; i<len;i ++) {
			*hex_string++ = NIBBLE_TO_HEX(*p>>4);
			*hex_string++ = NIBBLE_TO_HEX((*p)&0xF);
			*hex_string++ = ' ';
			p++;
		}
		--hex_string;
		*hex_string = 0;

		debugLog(0, "", s);
	}
}
#endif
