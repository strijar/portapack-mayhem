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

#ifndef __WSPR_TX_H__
#define __WSPR_TX_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_transmitter.hpp"

#include "portapack.hpp"
#include "message.hpp"
#include "volume.hpp"
#include "audio.hpp"

#include <ch.h>

namespace ui {

class WSPRView : public View {
public:
	WSPRView(NavigationView& nav);
	~WSPRView();
	
	WSPRView(const WSPRView&) = delete;
	WSPRView(WSPRView&&) = delete;
	WSPRView& operator=(const WSPRView&) = delete;
	WSPRView& operator=(WSPRView&&) = delete;
	
	void focus() override;
	
	void on_tx_progress(const uint32_t progress, const bool done);
	
	std::string title() const override { return "WSPR TX"; };
private:
	NavigationView& nav_;
	
	bool start_tx();
	void on_set_text(NavigationView& nav);
	
	ProgressBar progressbar {
		{ 2 * 8, 28 * 8, 208, 16 }
	};
	
	TransmitterView tx_view {
		16 * 16,
		10000,
		12
	};
	
	MessageHandlerRegistration message_handler_tx_progress {
		Message::ID::TXProgress,
		[this](const Message* const p) {
			const auto message = *reinterpret_cast<const TXProgressMessage*>(p);
			this->on_tx_progress(message.progress, message.done);
		}
	};
};

} /* namespace ui */

#endif/*__WSPR_TX_H__*/
