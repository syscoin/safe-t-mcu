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

#include <stdint.h>
#include <string.h>

#include "signatures.h"
#include "ecdsa.h"
#include "secp256k1.h"
#include "sha2.h"
#include "bootloader.h"

#define PUBKEYS 5

static const uint8_t * const pubkey[PUBKEYS] = {
	(const uint8_t *)"\x04\x2f\xf3\x57\xe8\x80\x47\x48\x42\x26\x88\x86\xb5\xc4\x36\x28\x5c\x9a\xfc\x15\x44\x04\xba\x89\xa2\x45\x2d\xd2\xd1\x58\x08\x39\x90\x2b\xb5\x8a\x76\x6f\x1e\x2e\x5d\x86\x2d\x16\x56\xd2\xd0\x60\x90\x77\xa4\xed\xee\x94\x11\x1d\x6c\x7d\xb8\xd3\xba\x5e\x42\x42\x5f",
	(const uint8_t *)"\x04\xa3\x73\x4e\x08\xf5\xa8\xa3\x40\x2f\xfa\x51\x1d\xf7\xd0\x5a\xcc\x0b\x38\x06\x20\x64\xb2\x66\xc1\xc8\xba\x2d\x14\x63\x84\x0b\x6e\x04\xbc\x84\x4e\x5d\xce\x4c\x8f\x05\x83\xbe\x16\xbf\xfa\x6b\x6e\x81\x63\x47\x6f\xa8\x45\xdd\xf0\x97\xcf\x5f\x38\x30\x1e\x21\x7f",
	(const uint8_t *)"\x04\xfa\x06\xb9\xaf\xda\x23\x6e\xe2\x27\x81\x2f\x8c\x34\x92\x5e\x19\x22\x4d\x19\xb3\x82\x6a\xbb\x25\xff\xb8\x9e\x09\xeb\x17\x1b\x90\x5d\xa0\x87\x5e\xd0\x9c\x81\x46\xec\x6e\xf0\x69\x94\xf4\x14\x46\x71\x99\x61\xa8\x17\x72\xe3\xa5\xae\xa9\xdb\x58\x1f\xab\x16\x94",
	(const uint8_t *)"\x04\x46\xce\x68\x19\xf8\xd1\x55\xfe\x02\x2b\x54\x0a\xba\x41\x45\x67\x34\xab\x63\x11\x25\xb1\x27\x05\xda\x1c\xb1\xe8\xfe\xde\xce\x29\x0d\xb8\x29\x1c\x08\xe5\xb9\xaa\x80\xf4\x14\x66\xbd\x79\xd2\x92\xb5\xb0\x33\x28\xbc\x4d\xbf\xbe\x46\x1c\x92\x71\xc5\x4d\x7e\xd7",
	(const uint8_t *)"\x04\x50\x6a\x11\x57\x5e\x4d\x0f\x7a\xca\x6d\xce\xbf\x4d\xf3\x1b\x52\x83\x2e\xa6\x8b\x97\x2b\x57\x21\xa8\xf9\x1a\x89\xdb\xbe\x7a\x75\x39\xbf\xf4\xa3\xdc\x50\x5f\x0e\x21\x4a\xba\x81\x2e\x80\xc9\x28\x0c\xf4\x8a\xc6\x2b\x06\x59\xea\xf7\x79\x11\x11\xf1\x5e\xe6\x81",
};

#define SIGNATURES 3

int signatures_ok(uint8_t *store_hash)
{
	const uint32_t codelen = *((const uint32_t *)FLASH_META_CODELEN);
	const uint8_t sigindex1 = *((const uint8_t *)FLASH_META_SIGINDEX1);
	const uint8_t sigindex2 = *((const uint8_t *)FLASH_META_SIGINDEX2);
	const uint8_t sigindex3 = *((const uint8_t *)FLASH_META_SIGINDEX3);

	uint8_t hash[32];
	sha256_Raw((const uint8_t *)FLASH_APP_START, codelen, hash);
	if (store_hash) {
		memcpy(store_hash, hash, 32);
	}

	if (sigindex1 < 1 || sigindex1 > PUBKEYS) return SIG_FAIL; // invalid index
	if (sigindex2 < 1 || sigindex2 > PUBKEYS) return SIG_FAIL; // invalid index
	if (sigindex3 < 1 || sigindex3 > PUBKEYS) return SIG_FAIL; // invalid index

	if (sigindex1 == sigindex2) return SIG_FAIL; // duplicate use
	if (sigindex1 == sigindex3) return SIG_FAIL; // duplicate use
	if (sigindex2 == sigindex3) return SIG_FAIL; // duplicate use

	if (0 != ecdsa_verify_digest(&secp256k1, pubkey[sigindex1 - 1], (const uint8_t *)FLASH_META_SIG1, hash)) { // failure
		return SIG_FAIL;
	}
	if (0 != ecdsa_verify_digest(&secp256k1, pubkey[sigindex2 - 1], (const uint8_t *)FLASH_META_SIG2, hash)) { // failure
		return SIG_FAIL;
	}
	if (0 != ecdsa_verify_digest(&secp256k1, pubkey[sigindex3 - 1], (const uint8_t *)FLASH_META_SIG3, hash)) { // failture
		return SIG_FAIL;
	}

	return SIG_OK;
}
