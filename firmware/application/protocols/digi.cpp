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
#include "digi.hpp"

using namespace portapack;

namespace digi {

uint8_t parity(uint32_t reg) {
	uint8_t bit = 0;
	
	for (uint8_t k = 0; k < 32; k++) {
		bit = bit ^ (reg & 0x01);
		reg = reg >> 1;
	}
	
	return bit;
}

void convolve(uint8_t * c, uint8_t * s, uint8_t message_size, uint8_t bit_size) {
	uint32_t	reg_0 = 0;
	uint32_t	reg_1 = 0;
	uint8_t		input_bit;
	uint8_t		bit_count = 0;

	for (uint8_t i = 0; i < message_size; i++) {
		uint8_t x = c[i];
	
		for (uint8_t j = 0; j < 8; j++) {
    		// Set input bit according the MSB of current element
    		input_bit = (((x << j) & 0x80) == 0x80) ? 1 : 0;

    		// Shift both registers and put in the new input bit
    		reg_0 = reg_0 << 1;
    		reg_1 = reg_1 << 1;
    		reg_0 |= (uint32_t)input_bit;
    		reg_1 |= (uint32_t)input_bit;

    		// AND Register 0 with feedback taps, calculate parity
    		s[bit_count] = parity(reg_0 & 0xf2d05351);
    		bit_count++;

    		// AND Register 1 with feedback taps, calculate parity
    		s[bit_count] = parity(reg_1 & 0xe4613c47);
    		bit_count++;

    		if (bit_count >= bit_size) {
      			break;
    		}
		}
	}
}

} /* namespace digi */
