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

#include "engines/grim/emi/graphics/color.h"

namespace Grim {

const Color Color::kWhite(0xFFFFFFFF);
const Color Color::kYellow(0xFFFFFF00);
const Color Color::kGreen(0xFF00FF00);
const Color Color::kRed(0xFFFF0000);
const Color Color::kCyan(0xFF00FFFF);
const Color Color::kBlue(0xFF0000FF);

Color::Color() {
	_b = 0;
	_g = 0;
	_r = 0;
	_a = 255;
}

Color::Color(const Color &other) {
	*this = other;
}

Color::Color(uint packed) {
	_a = packed >> 24;
	_r = (packed >> 16) & 0xFF;
	_g = (packed >> 8) & 0xFF;
	_b = packed & 0xFF;
}

Color &Color::operator=(const Color &other) {
	if (this != &other) {
		_a = other._a;
		_r = other._r;
		_g = other._g;
		_b = other._b;
	}

	return *this;
}

void Color::clamp() {
	if ((uint)_a > 0xFF)
		_a = 255;

	uint red = _r;

	if (red > 0xFF || (uint)_g > 0xFF || (uint)_b > 0xFF) {
		uint green = _g;
		uint blue = _b;
		uint largest = _r;

		if (green > red)
			largest = _g;

		if (blue > largest)
			largest = _b;

		_r = 255 * red / largest;
		_g = 255 * green / largest;
		_b = 255 * blue / largest;
	}
}

void Color::getRGB(byte *outR, byte *outG, byte *outB) const {
	uint red = _r;

	if (red > 0xFF)
		red = 255;

	*outR = red;

	uint green = _g;

	if (green > 0xFF)
		green = 255;

	*outG = green;

	uint blue = _b;

	if (blue > 0xFF)
		blue = 255;

	*outB = blue;
}

Color *Color::setRGB(int newR, int newG, int newB) {
	_r = newR;
	_g = newG;
	_b = newB;

	return this;
}

PackedColor *PackedColor::modulate(PackedColor *out, const PackedColor *other) {
	uint newR = _r * other->_r / 255;
	uint newG = _g * other->_g / 255;
	uint newB = _b * other->_b / 255;
	uint newA = _a * other->_a / 255;
	PackedColor result;

	result._r = newR <= 0xFF ? newR : 255;
	result._g = newG <= 0xFF ? newG : 255;
	result._b = newB <= 0xFF ? newB : 255;
	result._a = newA <= 0xFF ? newA : 255;

	*out = result;

	return out;
}

uint32 PackedColor::pack() const {
	return ((uint32)_a << 24) | ((uint32)_r << 16) | ((uint32)_g << 8) | _b;
}

} // End of namespace Grim
