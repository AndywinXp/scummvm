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

#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/emi/lcolorformat.h"

namespace Grim {

const LColorFormat LColorFormat::format1(1);
const LColorFormat LColorFormat::format2(2);
const LColorFormat LColorFormat::format3(3);
const LColorFormat LColorFormat::format4(4);
const LColorFormat LColorFormat::format5(5);
const LColorFormat LColorFormat::format6(6);
const LColorFormat LColorFormat::format9(9);
const LColorFormat LColorFormat::format10(10);
const LColorFormat LColorFormat::format11(11);
const LColorFormat LColorFormat::format12(12);
const LColorFormat LColorFormat::format13(13);
const LColorFormat LColorFormat::format14(14);
const LColorFormat LColorFormat::format15(15);
const LColorFormat LColorFormat::format16(16);

LColorFormat::LColorFormat() {
}

LColorFormat::LColorFormat(int format) {
	setFormat(format);
}

LColorFormat::~LColorFormat() {
}

void LColorFormat::setFormat(int format) {
	formatId = format;

	switch (format) {
	case 2:
		shiftA = 0;
		shiftR = 0;
		shiftG = 0;
		shiftB = 0;
		maskA = 0;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		maskR = 0xFF000000;
		maskG = 0xFF000000;
		maskB = 0xFF000000;
		alphaBits = 1;
		bytesPerPixel = 1;
		break;
	case 3:
		shiftR = 10;
		shiftA = 0;
		shiftB = 0;
		maskA = 0;
		lossA = 0;
		shiftG = 5;
		maskR = 0x7C00;
		maskG = 0x03E0;
		maskB = 0x001F;
		lossR = 3;
		lossG = 3;
		lossB = 3;
		alphaBits = 3;
		bytesPerPixel = 2;
		break;
	case 4:
		shiftA = 15;
		shiftR = 10;
		shiftG = 5;
		shiftB = 0;
		maskA = 0x8000;
		maskR = 0x7C00;
		maskG = 0x03E0;
		maskB = 0x001F;
		lossA = 7;
		lossR = 3;
		lossG = 3;
		lossB = 3;
		alphaBits = 4;
		bytesPerPixel = 2;
		break;
	case 5:
		shiftA = 0;
		shiftB = 0;
		maskA = 0;
		lossA = 0;
		shiftR = 11;
		shiftG = 5;
		maskR = 0xF800;
		maskG = 0x07E0;
		maskB = 0x001F;
		lossR = 3;
		lossG = 2;
		lossB = 3;
		alphaBits = 3;
		bytesPerPixel = 2;
		break;
	case 6:
	case 7:
		shiftR = 16;
		shiftA = 0;
		shiftG = 8;
		shiftB = 0;
		maskA = 0;
		maskR = 0x00FF0000;
		maskG = 0x0000FF00;
		maskB = 0x000000FF;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 3;
		bytesPerPixel = 3;
		break;
	case 8:
		shiftG = 8;
		shiftA = 0;
		shiftR = 0;
		shiftB = 16;
		maskA = 0;
		maskR = 0x000000FF;
		maskG = 0x0000FF00;
		maskB = 0x00FF0000;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 3;
		bytesPerPixel = 3;
		break;
	case 9:
	case 10:
		shiftR = 24;
		shiftA = 0;
		shiftG = 16;
		shiftB = 8;
		maskA = 0x000000FF;
		maskR = 0xFF000000;
		maskG = 0x00FF0000;
		maskB = 0x0000FF00;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 4;
		bytesPerPixel = 4;
		break;
	case 11:
		shiftA = 24;
		shiftR = 0;
		shiftG = 8;
		shiftB = 16;
		maskA = 0xFF000000;
		maskR = 0x000000FF;
		maskG = 0x0000FF00;
		maskB = 0x00FF0000;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 4;
		bytesPerPixel = 4;
		break;
	case 12:
		shiftA = 24;
		shiftR = 16;
		shiftG = 8;
		shiftB = 0;
		maskA = 0xFF000000;
		maskR = 0x00FF0000;
		maskG = 0x0000FF00;
		maskB = 0x000000FF;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 4;
		bytesPerPixel = 4;
		break;
	case 13:
		shiftR = 8;
		shiftA = 0;
		shiftG = 16;
		shiftB = 24;
		maskA = 0x000000FF;
		maskR = 0x0000FF00;
		maskG = 0x00FF0000;
		maskB = 0xFF000000;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 4;
		bytesPerPixel = 4;
		break;
	case 14:
		shiftA = 12;
		shiftR = 8;
		shiftG = 4;
		shiftB = 0;
		maskA = 0xF000;
		maskR = 0x0F00;
		maskG = 0x00F0;
		maskB = 0x000F;
		lossA = 4;
		lossR = 4;
		lossG = 4;
		lossB = 4;
		alphaBits = 4;
		bytesPerPixel = 2;
		break;
	case 15:
		shiftA = 0;
		shiftB = 1;
		maskA = 0x0001;
		shiftR = 11;
		shiftG = 6;
		maskR = 0xF800;
		maskG = 0x07C0;
		maskB = 0x003E;
		lossA = 7;
		lossR = 3;
		lossG = 3;
		lossB = 3;
		alphaBits = 4;
		bytesPerPixel = 2;
		break;
	case 16:
		shiftA = 0;
		shiftR = 12;
		shiftG = 8;
		shiftB = 4;
		maskA = 0x000F;
		maskR = 0xF000;
		maskG = 0x0F00;
		maskB = 0x00F0;
		lossA = 4;
		lossR = 4;
		lossG = 4;
		lossB = 4;
		alphaBits = 4;
		bytesPerPixel = 2;
		break;
	default:
		formatId = 0;
		shiftA = 0;
		shiftR = 0;
		shiftG = 0;
		shiftB = 0;
		maskA = 0;
		maskR = 0;
		maskG = 0;
		maskB = 0;
		lossA = 0;
		lossR = 0;
		lossG = 0;
		lossB = 0;
		alphaBits = 0;
		bytesPerPixel = 0;
		break;
	}
}

uint LColorFormat::readPixel(const byte *pixel) const {
	switch (bytesPerPixel) {
	case 1:
		return pixel[0];
	case 2:
		return READ_LE_UINT16(pixel);
	case 3:
		return (pixel[0] << 16) | (pixel[1] << 8) | pixel[2];
	case 4:
		return READ_BE_UINT32(pixel);
	default:
		warning("LColorFormat::readPixel(): unsupported bytes per pixel %d", bytesPerPixel);
		return 0;
	}
}

int LColorFormat::getChannel(uint pixelValue, uint mask, int shift) const {
	int divisor = (int)(mask >> shift);

	// The original could divide by zero here, let's avoid that...
	if (divisor == 0)
		return 0;

	return (int)((double)((pixelValue & mask) >> shift) / (double)divisor * 255.0);
}

int LColorFormat::getRed(const byte *pixel) const {
	return getChannel(readPixel(pixel), maskR, shiftR);
}

int LColorFormat::getGreen(const byte *pixel) const {
	return getChannel(readPixel(pixel), maskG, shiftG);
}

int LColorFormat::getBlue(const byte *pixel) const {
	return getChannel(readPixel(pixel), maskB, shiftB);
}

int LColorFormat::getAlpha(const byte *pixel) const {
	if (alphaBits == 3)
		return 255;

	return getChannel(readPixel(pixel), maskA, shiftA);
}

void LColorFormat::packPixel(byte *dest, uint alpha, uint red, uint green, uint blue) const {
	uint value = (maskB & ((blue >> lossB) << shiftB)) |
		(maskG & ((green >> lossG) << shiftG)) |
		(maskR & ((red >> lossR) << shiftR)) |
		(maskA & ((alpha >> lossA) << shiftA));

	switch (bytesPerPixel) {
	case 1:
		dest[0] = value;
		break;
	case 2:
		WRITE_LE_UINT16(dest, value);
		break;
	case 3:
		dest[0] = value >> 16;
		dest[1] = value >> 8;
		dest[2] = value;
		break;
	case 4:
		WRITE_BE_UINT32(dest, value);
		break;
	default:
		break;
	}
}

void LColorFormat::convertPixels(const byte *source, int count, const LColorFormat &destFormat, byte *dest) const {
	int destBytesPerPixel = destFormat.bytesPerPixel;

	for (int i = 0; i < count; i++) {
		const byte *pixel = &source[bytesPerPixel * i];

		byte alpha = (byte)getAlpha(pixel);
		byte red = (byte)getRed(pixel);
		byte green = (byte)getGreen(pixel);
		byte blue = (byte)getBlue(pixel);

		destFormat.packPixel(dest, alpha, red, green, blue);

		dest += destBytesPerPixel;
	}
}

} // End of namespace Grim
