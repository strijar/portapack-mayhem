/*
 * Copyright (C) 2020 Belousov Oleg
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

#include "dsp_modulate.hpp"
#include "sine_table_int8.hpp"

namespace dsp {
namespace modulate {

Modulator::~Modulator() {
}

Mode Modulator::get_mode() {
	return mode;
}

void Modulator::set_mode(Mode new_mode) {
	mode = new_mode;
}

///

AM::AM() : hilbert() {
	mode = Mode::DSB;
}

void AM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer) {
	uint32_t	over = 64;
	int16_t		*audio_ptr = audio.p;
	int8_t		re = 0, im = 0;
	
	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % over == 0) {
			int32_t	sample = audio.p[counter / over];
			float	i = 0.0, q = 0.0;

			hilbert.execute(sample / 32768.0f, i, q);
	
			i *= 127.0f;
			q *= 127.0f;
	
			switch (mode) {
				case Mode::DSB:		re = i + q;	im = i + q;		break;
				case Mode::LSB:		re = q;		im = i;			break;
				case Mode::USB:		re = i;		im = q;			break;
				default:			re = 0;		im = 0;			break;
			}
		}
		
		buffer.p[counter] = { re, im };
	}
}

///

FM::FM() {
	mode = Mode::FM;
}

void FM::set_fm_delta(uint32_t new_delta) {
	fm_delta = new_delta;
}

void FM::execute(const buffer_s16_t& audio, const buffer_c8_t& buffer) {
	uint32_t	over = buffer.count / audio.count;
	int16_t		*audio_ptr = audio.p;
	int32_t		sample = 0;
	int8_t		re, im;

	for (size_t counter = 0; counter < buffer.count; counter++) {
		if (counter % over == 0) {
			sample = (*audio_ptr++) >> 8;
			delta = sample * fm_delta;
		}

		phase += delta;
		sphase = phase >> 24;

		re = (sine_table_i8[(sphase + 64) & 255]);
		im = (sine_table_i8[sphase]);
		
		buffer.p[counter] = { re, im };
	}
}

}
}
