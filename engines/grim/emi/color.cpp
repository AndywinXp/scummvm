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

#include "engines/grim/emi/color.h"

namespace Grim {

Color Color::kWhite(0xFFFFFFFF);
Color Color::kYellow(0xFFFFFF00);
Color Color::kGreen(0xFF00FF00);
Color Color::kRed(0xFFFF0000);
Color Color::kCyan(0xFF00FFFF);
Color Color::kBlue(0xFF0000FF);

Color::Color() {
	b = 0;
	g = 0;
	r = 0;
	a = 255;
}

Color::Color(const Color &other) {
	*this = other;
}

Color::Color(uint packed) {
	a = packed >> 24;
	r = (packed >> 16) & 0xFF;
	g = (packed >> 8) & 0xFF;
	b = packed & 0xFF;
}

Color &Color::operator=(const Color &other) {
	if (this != &other) {
		a = other.a;
		r = other.r;
		g = other.g;
		b = other.b;
	}

	return *this;
}

void Color::clamp() {
	if ((uint)a > 0xFF)
		a = 255;

	uint red = r;

	if (red > 0xFF || (uint)g > 0xFF || (uint)b > 0xFF) {
		uint green = g;
		uint blue = b;
		uint largest = r;

		if (green > red)
			largest = g;

		if (blue > largest)
			largest = b;

		r = 255 * red / largest;
		g = 255 * green / largest;
		b = 255 * blue / largest;
	}
}

void Color::getRGB(byte *outR, byte *outG, byte *outB) {
	uint red = r;

	if (red > 0xFF)
		red = 255;

	*outR = red;

	uint green = g;

	if (green > 0xFF)
		green = 255;

	*outG = green;

	uint blue = b;

	if (blue > 0xFF)
		blue = 255;

	*outB = blue;
}

Color *Color::setRGB(int newR, int newG, int newB) {
	r = newR;
	g = newG;
	b = newB;

	return this;
}

PackedColor *PackedColor::modulate(PackedColor *out, const PackedColor *other) {
	uint newR = r * other->r / 255;
	uint newG = g * other->g / 255;
	uint newB = b * other->b / 255;
	uint newA = a * other->a / 255;
	PackedColor result;

	result.r = newR <= 0xFF ? newR : 255;
	result.g = newG <= 0xFF ? newG : 255;
	result.b = newB <= 0xFF ? newB : 255;
	result.a = newA <= 0xFF ? newA : 255;

	*out = result;

	return out;
}

uint32 PackedColor::pack() const {
	return ((uint32)a << 24) | ((uint32)r << 16) | ((uint32)g << 8) | b;
}

} // End of namespace Grim
