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

#include "engines/grim/emi/graphics/lcolorformat.h"

namespace Grim {

const LColorFormat LColorFormat::kFormat1(1);
const LColorFormat LColorFormat::kFormat2(2);
const LColorFormat LColorFormat::kFormat3(3);
const LColorFormat LColorFormat::kFormat4(4);
const LColorFormat LColorFormat::kFormat5(5);
const LColorFormat LColorFormat::kFormat6(6);
const LColorFormat LColorFormat::kFormat9(9);
const LColorFormat LColorFormat::kFormat10(10);
const LColorFormat LColorFormat::kFormat11(11);
const LColorFormat LColorFormat::kFormat12(12);
const LColorFormat LColorFormat::kFormat13(13);
const LColorFormat LColorFormat::kFormat14(14);
const LColorFormat LColorFormat::kFormat15(15);
const LColorFormat LColorFormat::kFormat16(16);

LColorFormat::LColorFormat() {
}

LColorFormat::LColorFormat(int format) {
	setFormat(format);
}

LColorFormat::~LColorFormat() {
}

void LColorFormat::setFormat(int format) {
	_formatId = format;

	switch (format) {
	case 2:
		_shiftA = 0;
		_shiftR = 0;
		_shiftG = 0;
		_shiftB = 0;
		_maskA = 0;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_maskR = 0xFF000000;
		_maskG = 0xFF000000;
		_maskB = 0xFF000000;
		_alphaBits = 1;
		_bytesPerPixel = 1;
		break;
	case 3:
		_shiftR = 10;
		_shiftA = 0;
		_shiftB = 0;
		_maskA = 0;
		_lossA = 0;
		_shiftG = 5;
		_maskR = 0x7C00;
		_maskG = 0x03E0;
		_maskB = 0x001F;
		_lossR = 3;
		_lossG = 3;
		_lossB = 3;
		_alphaBits = 3;
		_bytesPerPixel = 2;
		break;
	case 4:
		_shiftA = 15;
		_shiftR = 10;
		_shiftG = 5;
		_shiftB = 0;
		_maskA = 0x8000;
		_maskR = 0x7C00;
		_maskG = 0x03E0;
		_maskB = 0x001F;
		_lossA = 7;
		_lossR = 3;
		_lossG = 3;
		_lossB = 3;
		_alphaBits = 4;
		_bytesPerPixel = 2;
		break;
	case 5:
		_shiftA = 0;
		_shiftB = 0;
		_maskA = 0;
		_lossA = 0;
		_shiftR = 11;
		_shiftG = 5;
		_maskR = 0xF800;
		_maskG = 0x07E0;
		_maskB = 0x001F;
		_lossR = 3;
		_lossG = 2;
		_lossB = 3;
		_alphaBits = 3;
		_bytesPerPixel = 2;
		break;
	case 6:
	case 7:
		_shiftR = 16;
		_shiftA = 0;
		_shiftG = 8;
		_shiftB = 0;
		_maskA = 0;
		_maskR = 0x00FF0000;
		_maskG = 0x0000FF00;
		_maskB = 0x000000FF;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 3;
		_bytesPerPixel = 3;
		break;
	case 8:
		_shiftG = 8;
		_shiftA = 0;
		_shiftR = 0;
		_shiftB = 16;
		_maskA = 0;
		_maskR = 0x000000FF;
		_maskG = 0x0000FF00;
		_maskB = 0x00FF0000;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 3;
		_bytesPerPixel = 3;
		break;
	case 9:
	case 10:
		_shiftR = 24;
		_shiftA = 0;
		_shiftG = 16;
		_shiftB = 8;
		_maskA = 0x000000FF;
		_maskR = 0xFF000000;
		_maskG = 0x00FF0000;
		_maskB = 0x0000FF00;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 4;
		_bytesPerPixel = 4;
		break;
	case 11:
		_shiftA = 24;
		_shiftR = 0;
		_shiftG = 8;
		_shiftB = 16;
		_maskA = 0xFF000000;
		_maskR = 0x000000FF;
		_maskG = 0x0000FF00;
		_maskB = 0x00FF0000;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 4;
		_bytesPerPixel = 4;
		break;
	case 12:
		_shiftA = 24;
		_shiftR = 16;
		_shiftG = 8;
		_shiftB = 0;
		_maskA = 0xFF000000;
		_maskR = 0x00FF0000;
		_maskG = 0x0000FF00;
		_maskB = 0x000000FF;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 4;
		_bytesPerPixel = 4;
		break;
	case 13:
		_shiftR = 8;
		_shiftA = 0;
		_shiftG = 16;
		_shiftB = 24;
		_maskA = 0x000000FF;
		_maskR = 0x0000FF00;
		_maskG = 0x00FF0000;
		_maskB = 0xFF000000;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 4;
		_bytesPerPixel = 4;
		break;
	case 14:
		_shiftA = 12;
		_shiftR = 8;
		_shiftG = 4;
		_shiftB = 0;
		_maskA = 0xF000;
		_maskR = 0x0F00;
		_maskG = 0x00F0;
		_maskB = 0x000F;
		_lossA = 4;
		_lossR = 4;
		_lossG = 4;
		_lossB = 4;
		_alphaBits = 4;
		_bytesPerPixel = 2;
		break;
	case 15:
		_shiftA = 0;
		_shiftB = 1;
		_maskA = 0x0001;
		_shiftR = 11;
		_shiftG = 6;
		_maskR = 0xF800;
		_maskG = 0x07C0;
		_maskB = 0x003E;
		_lossA = 7;
		_lossR = 3;
		_lossG = 3;
		_lossB = 3;
		_alphaBits = 4;
		_bytesPerPixel = 2;
		break;
	case 16:
		_shiftA = 0;
		_shiftR = 12;
		_shiftG = 8;
		_shiftB = 4;
		_maskA = 0x000F;
		_maskR = 0xF000;
		_maskG = 0x0F00;
		_maskB = 0x00F0;
		_lossA = 4;
		_lossR = 4;
		_lossG = 4;
		_lossB = 4;
		_alphaBits = 4;
		_bytesPerPixel = 2;
		break;
	default:
		_formatId = 0;
		_shiftA = 0;
		_shiftR = 0;
		_shiftG = 0;
		_shiftB = 0;
		_maskA = 0;
		_maskR = 0;
		_maskG = 0;
		_maskB = 0;
		_lossA = 0;
		_lossR = 0;
		_lossG = 0;
		_lossB = 0;
		_alphaBits = 0;
		_bytesPerPixel = 0;
		break;
	}
}

uint LColorFormat::readPixel(const byte *pixel) const {
	switch (_bytesPerPixel) {
	case 1:
		return pixel[0];
	case 2:
		return READ_LE_UINT16(pixel);
	case 3:
		return (pixel[0] << 16) | (pixel[1] << 8) | pixel[2];
	case 4:
		return READ_BE_UINT32(pixel);
	default:
		warning("LColorFormat::readPixel(): unsupported bytes per pixel %d", _bytesPerPixel);
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
	return getChannel(readPixel(pixel), _maskR, _shiftR);
}

int LColorFormat::getGreen(const byte *pixel) const {
	return getChannel(readPixel(pixel), _maskG, _shiftG);
}

int LColorFormat::getBlue(const byte *pixel) const {
	return getChannel(readPixel(pixel), _maskB, _shiftB);
}

int LColorFormat::getAlpha(const byte *pixel) const {
	if (_alphaBits == 3)
		return 255;

	return getChannel(readPixel(pixel), _maskA, _shiftA);
}

void LColorFormat::packPixel(byte *dest, uint alpha, uint red, uint green, uint blue) const {
	uint value = (_maskB & ((blue >> _lossB) << _shiftB)) |
		(_maskG & ((green >> _lossG) << _shiftG)) |
		(_maskR & ((red >> _lossR) << _shiftR)) |
		(_maskA & ((alpha >> _lossA) << _shiftA));

	switch (_bytesPerPixel) {
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
	int destBytesPerPixel = destFormat._bytesPerPixel;

	for (int i = 0; i < count; i++) {
		const byte *pixel = &source[_bytesPerPixel * i];

		byte alpha = (byte)getAlpha(pixel);
		byte red = (byte)getRed(pixel);
		byte green = (byte)getGreen(pixel);
		byte blue = (byte)getBlue(pixel);

		destFormat.packPixel(dest, alpha, red, green, blue);

		dest += destBytesPerPixel;
	}
}

} // End of namespace Grim
