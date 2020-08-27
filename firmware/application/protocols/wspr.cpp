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

using namespace portapack;

namespace wspr {

const	uint8_t sync_vector[SYMBOL_COUNT] = {
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
void convolve(uint8_t * c, uint8_t * s, uint8_t message_size, uint8_t bit_size);
void interleave(uint8_t * s);
void merge_sync_vector(uint8_t * g, uint8_t * symbols);

void encode(const std::string& callsign, const std::string& loc, const uint8_t dbm, uint8_t * symbols) {
	uint8_t	c[MESSAGE_COUNT];
	uint8_t	s[SYMBOL_COUNT];

	bit_packing(callsign, loc, dbm, c);
	convolve(c, s, MESSAGE_COUNT, BIT_COUNT);
	interleave(s);
	merge_sync_vector(s, symbols);
}

uint8_t code(char c) {
	switch (c) {
		case '0'...'9':
			return (uint8_t)(c - 48);

		case 'A'...'Z':
			return (uint8_t)(c - 55);

		case ' ':
			return 36;

		default:
			return 255;
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

void convolve(uint8_t * c, uint8_t * s, uint8_t message_size, uint8_t bit_size) {
	uint32_t	reg_0 = 0;
	uint32_t	reg_1 = 0;
	uint32_t	reg_temp = 0;
	uint8_t		input_bit, parity_bit;
	uint8_t		bit_count = 0;
	uint8_t		i, j, k;

	for (i = 0; i < message_size; i++) {
		for (j = 0; j < 8; j++) {
    		// Set input bit according the MSB of current element
    		input_bit = (((c[i] << j) & 0x80) == 0x80) ? 1 : 0;

    		// Shift both registers and put in the new input bit
    		reg_0 = reg_0 << 1;
    		reg_1 = reg_1 << 1;
    		reg_0 |= (uint32_t)input_bit;
    		reg_1 |= (uint32_t)input_bit;

    		// AND Register 0 with feedback taps, calculate parity
    		reg_temp = reg_0 & 0xf2d05351;
    		parity_bit = 0;

    		for (k = 0; k < 32; k++) {
      			parity_bit = parity_bit ^ (reg_temp & 0x01);
      			reg_temp = reg_temp >> 1;
    		}

    		s[bit_count] = parity_bit;
    		bit_count++;

    		// AND Register 1 with feedback taps, calculate parity
    		reg_temp = reg_1 & 0xe4613c47;
    		parity_bit = 0;

    		for (k = 0; k < 32; k++) {
      			parity_bit = parity_bit ^ (reg_temp & 0x01);
      			reg_temp = reg_temp >> 1;
    		}

    		s[bit_count] = parity_bit;
    		bit_count++;

    		if (bit_count >= bit_size) {
      			break;
    		}
		}
	}
}

void interleave(uint8_t * s) {
	uint8_t	d[BIT_COUNT];
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

		if (rev < BIT_COUNT) {
			d[rev] = s[i];
			i++;
		}

		if (i >= BIT_COUNT) {
			break;
		}
	}

	memcpy(s, d, BIT_COUNT);
}

void merge_sync_vector(uint8_t * g, uint8_t * symbols) {
	uint8_t	i;

	for (i = 0; i < SYMBOL_COUNT; i++)
		symbols[i] = sync_vector[i] + (2 * g[i]);
}

} /* namespace wspr */
