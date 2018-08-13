/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2018 Pavol Rusnak <stick@satoshilabs.com>
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

#include <stdint.h>
#include <string.h>
#include <libopencm3/stm32/flash.h>
#include "bl_data.h"
#include "memory.h"
#include "layout.h"
#include "layout2.h"
#include "gettext.h"
#include "util.h"

int known_bootloader(int r, const uint8_t *hash) {
	if (r != 32) return 0;

	// T2 Bootloader
	if (0 == memcmp(hash, "\xb5\xe8\xec\x4a\xae\x58\x2b\xf9\xe3\x9a\x5d\xfe\xcb\x5e\x39\xbb\xc6\xdb\xb3\xa8\xb9\xcb\x81\x81\x16\xf3\xed\x64\xb3\xad\xe9\x5b", 32)) return 1;
	// Bootloader Fixed descriptor
	if (0 == memcmp(hash, "\xd0\xa1\x57\x5e\x1b\xdc\x70\x4b\x6f\x1d\xc3\xbd\xb0\xfc\x6a\xcc\x31\x2e\xff\x7b\x16\xb5\x4c\xc9\x23\x11\x3d\xbf\xa5\x98\xc4\x59", 32)) return 1;
	// Bootloader MP0
	if (0 == memcmp(hash, "\x67\x43\xa2\x84\x66\x1a\xa2\x58\x21\x6a\xe7\x1d\xe4\xde\x65\xb9\xb9\xbe\x5f\x68\x4b\x26\x56\x6f\x2e\x47\x83\x9e\x4c\xdf\x78\x7c", 32)) return 1;
	// Bootloader blsafet1.5.1
	if (0 == memcmp(hash, "\x1f\xed\x1d\xe6\x90\xc8\xbf\xf7\x56\xde\xea\x85\xcc\x92\x65\x92\x33\x8e\xc0\x1e\x5e\x82\xfd\xb1\xfb\xeb\xbd\xde\x35\xf3\xc4\xb2", 32)) return 1;

	return 0;
}

void check_bootloader(void)
{
#if MEMORY_PROTECT
	uint8_t hash[32];
	int r = memory_bootloader_hash(hash);

	if (!known_bootloader(r, hash)) {
		layoutDialogSplit(&bmp_icon_error, NULL, NULL, NULL, _("Unknown bootloader detected.\n\nUnplug your Safe-T\ncontact our support."));
		shutdown();
	}

	if (r == 32 && 0 == memcmp(hash, bl_hash, 32)) {
		// all OK -> done
		return;
	}
#if UPDATE_BOOTLOADER
	// ENABLE THIS AT YOUR OWN RISK
	// ATTEMPTING TO OVERWRITE BOOTLOADER WITH UNSIGNED FIRMWARE MAY BRICK
	// YOUR DEVICE.

	// unlock sectors
	memory_write_unlock();

	// replace bootloader
	flash_unlock();
	for (int i = FLASH_BOOT_SECTOR_FIRST; i <= FLASH_BOOT_SECTOR_LAST; i++) {
		flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
	}
	for (int i = 0; i < FLASH_BOOT_LEN / 4; i++) {
		const uint32_t *w = (const uint32_t *)(bl_data + i * 4);
		flash_program_word(FLASH_BOOT_START + i * 4, *w);
	}
	flash_lock();

	// show info and halt
	layoutDialogSplit(&bmp_icon_info, NULL, NULL, NULL, _("Update finished successfully.\n\nPlease reconnect the device."));
	shutdown();
#endif
#endif
}
