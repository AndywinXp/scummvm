/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "topgun/display.h"

namespace Topgun {

enum {
	kNumPaletteEntries = 256,
	kNumStaticColorsPerEnd = 10,
	kPaletteEntrySize = 4
};

static const byte kStaticColors[2 * kNumStaticColorsPerEnd][3] = {
	{   0,   0,   0 }, { 128,   0,   0 }, {   0, 128,   0 }, { 128, 128,   0 }, {   0,   0, 128 },
	{ 128,   0, 128 }, {   0, 128, 128 }, { 192, 192, 192 }, { 192, 220, 192 }, { 166, 202, 240 },
	{ 255, 251, 240 }, { 160, 160, 164 }, { 128, 128, 128 }, { 255,   0,   0 }, {   0, 255,   0 },
	{ 255, 255,   0 }, {   0,   0, 255 }, { 255,   0, 255 }, {   0, 255, 255 }, { 255, 255, 255 }
};

Display::Display(uint16 width, uint16 height) : _width(width), _height(height) {
}

Display::~Display() {
}

void Display::getSystemPaletteEntries(byte *destination, uint16 first, uint16 count) const {
	for (uint16 i = 0; i < count; i++) {
		uint32 index = (uint32)first + i;
		if (index >= kNumPaletteEntries)
			break;

		byte *entry = destination + i * kPaletteEntrySize;
		entry[0] = 0;
		entry[1] = 0;
		entry[2] = 0;
		entry[3] = 0;

		int staticIndex = -1;
		if (index < kNumStaticColorsPerEnd)
			staticIndex = index;
		else if (index >= kNumPaletteEntries - kNumStaticColorsPerEnd)
			staticIndex = kNumStaticColorsPerEnd + (index - (kNumPaletteEntries - kNumStaticColorsPerEnd));

		if (staticIndex >= 0) {
			entry[0] = kStaticColors[staticIndex][0];
			entry[1] = kStaticColors[staticIndex][1];
			entry[2] = kStaticColors[staticIndex][2];
		}
	}
}

} // End of namespace Topgun
