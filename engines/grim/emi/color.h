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

#ifndef GRIM_EMI_COLOR_H
#define GRIM_EMI_COLOR_H

#include "common/scummsys.h"

namespace Grim {

class Color {
public:
	Color();
	Color(const Color &other);
	Color(uint packed);

	Color &operator=(const Color &other);

	void clamp();
	void getRGB(byte *r, byte *g, byte *b);
	Color *setRGB(int newR, int newG, int newB);

	int a;
	int r;
	int g;
	int b;
};

class PackedColor {
public:
	PackedColor *modulate(PackedColor *out, const PackedColor *other);

	byte b;
	byte g;
	byte r;
	byte a;
};

} // End of namespace Grim

#endif
