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

#include "ui_wspr.hpp"
#include "wspr.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
#include "hackrf_gpio.hpp"
#include "portapack_shared_memory.hpp"
#include "ui_textentry.hpp"
#include "string_format.hpp"

#include <cstring>
#include <stdio.h>

using namespace portapack;
using namespace hackrf::one;

namespace ui {

void WSPRView::focus() {
}

WSPRView::~WSPRView() {
	transmitter_model.disable();
	baseband::shutdown();
}

bool WSPRView::start_tx() {
	progressbar.set_max(WSPR_SYMBOL_COUNT);
	
	transmitter_model.set_sampling_rate(12000*256);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	return true;
}

void WSPRView::on_tx_progress(const uint32_t progress, const bool done) {
	if (done) {
		transmitter_model.disable();
		progressbar.set_value(0);
		tx_view.set_transmitting(false);
	} else
		progressbar.set_value(progress);
}

WSPRView::WSPRView(NavigationView& nav) : nav_ (nav) {
	baseband::run_image(portapack::spi_flash::image_tag_mfsk);
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
		};
	};
	
	tx_view.on_start = [this]() {
		if (start_tx())
			tx_view.set_transmitting(true);
	};
	
	tx_view.on_stop = [this]() {
		transmitter_model.disable();
		baseband::kill_tone();
		tx_view.set_transmitting(false);
	};
}

} /* namespace ui */
