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

#include <string>

#ifndef __WSPR_H__
#define __WSPR_H__

namespace wspr {

#define MESSAGE_COUNT	11
#define SYMBOL_COUNT    162
#define BIT_COUNT       162
	
void encode(const std::string& callsign, const std::string& log, const uint8_t dbm, uint8_t * symbols);

} /* namespace wspr */

#endif/*__WSPR_H__*/
