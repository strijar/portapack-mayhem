/*
 * Copyright (C) 2020 Strijar
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "portapack_persistent_memory.hpp"
#include "wspr.hpp"
#include "digi.hpp"

using namespace portapack;

namespace wspr {

const	uint8_t sync_vector[WSPR_SYMBOL_COUNT] = {
  1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0,
  1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
  0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
  0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0,
  1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
  0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
  1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0
};

uint8_t code(char c);
void bit_packing(const std::string& callsign, const std::string& loc, const uint8_t dbm, uint8_t * c);
void interleave(uint8_t * s);
void merge_sync_vector(uint8_t * g, uint8_t * symbols);

void encode(const std::string& callsign, const std::string& loc, const uint8_t dbm, uint8_t * symbols) {
	uint8_t	c[WSPR_MESSAGE_COUNT];
	uint8_t	s[WSPR_SYMBOL_COUNT];

	bit_packing(callsign, loc, dbm, c);
	digi::convolve(c, s, WSPR_MESSAGE_COUNT, WSPR_BIT_COUNT);
	interleave(s);
	merge_sync_vector(s, symbols);
}

uint8_t code(char c) {
	switch (c) {
		case ' ':
			return 36;

		case '0'...'9':
			return c - '0';

		case 'A'...'Z':
			return c - 'A' + 10;

		case 'a'...'z':
			return c - 'a' + 10;

		default:
			return 0;
	}
}

void bit_packing(const std::string& callsign, const std::string& loc, const uint8_t dbm, uint8_t * c) {
	uint32_t	n, m;

	n = code(callsign[0]);
	n = n * 36 + code(callsign[1]);
	n = n * 10 + code(callsign[2]);
	n = n * 27 + (code(callsign[3]) - 10);
	n = n * 27 + (code(callsign[4]) - 10);
	n = n * 27 + (code(callsign[5]) - 10);

	m = ((179 - 10 * (loc[0] - 'A') - (loc[2] - '0')) * 180) + (10 * (loc[1] - 'A')) + (loc[3] - '0');
	m = (m * 128) + dbm + 64;

	c[3] = (uint8_t)((n & 0x0f) << 4);	n = n >> 4;
	c[2] = (uint8_t)(n & 0xff);			n = n >> 8;
	c[1] = (uint8_t)(n & 0xff);			n = n >> 8;
	c[0] = (uint8_t)(n & 0xff);

	c[6] = (uint8_t)((m & 0x03) << 6);	m = m >> 2;
	c[5] = (uint8_t)(m & 0xff);			m = m >> 8;
	c[4] = (uint8_t)(m & 0xff);			m = m >> 8;
	c[3] |= (uint8_t)(m & 0x0f);
	c[7] = 0;
	c[8] = 0;
	c[9] = 0;
	c[10] = 0;
}


void interleave(uint8_t * s) {
	uint8_t	d[WSPR_BIT_COUNT];
	uint8_t	rev, index_temp, i, j, k;

	i = 0;

	for (j = 0; j < 255; j++) {
		// Bit reverse the index
		index_temp = j;
		rev = 0;

		for (k = 0; k < 8; k++) {
			if (index_temp & 0x01) {
				rev = rev | (1 << (7 - k));
			}
			index_temp = index_temp >> 1;
		}

		if (rev < WSPR_BIT_COUNT) {
			d[rev] = s[i];
			i++;
		}

		if (i >= WSPR_BIT_COUNT) {
			break;
		}
	}

	memcpy(s, d, WSPR_BIT_COUNT);
}

void merge_sync_vector(uint8_t * g, uint8_t * symbols) {
	uint8_t	i;

	for (i = 0; i < WSPR_SYMBOL_COUNT; i++)
		symbols[i] = sync_vector[i] + (2 * g[i]);
}

} /* namespace wspr */
