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

#ifndef GRIM_EMI_LCOLORFORMAT_H
#define GRIM_EMI_LCOLORFORMAT_H

#include "common/scummsys.h"

namespace Grim {

class LColorFormat {
public:
	LColorFormat();
	LColorFormat(int format);
	virtual ~LColorFormat();

	void setFormat(int format);
	int getRed(const byte *pixel) const;
	int getGreen(const byte *pixel) const;
	int getBlue(const byte *pixel) const;
	int getAlpha(const byte *pixel) const;
	void packPixel(byte *dest, uint alpha, uint red, uint green, uint blue) const;
	void convertPixels(const byte *source, int count, const LColorFormat &destFormat, byte *dest) const;

	static const LColorFormat kFormat1;
	static const LColorFormat kFormat2;
	static const LColorFormat kFormat3;
	static const LColorFormat kFormat4;
	static const LColorFormat kFormat5;
	static const LColorFormat kFormat6;
	static const LColorFormat kFormat9;
	static const LColorFormat kFormat10;
	static const LColorFormat kFormat11;
	static const LColorFormat kFormat12;
	static const LColorFormat kFormat13;
	static const LColorFormat kFormat14;
	static const LColorFormat kFormat15;
	static const LColorFormat kFormat16;

	int _formatId;
	int _bytesPerPixel;
	int _alphaBits;
	uint _maskR;
	uint _maskG;
	uint _maskB;
	uint _maskA;
	int _lossR;
	int _lossG;
	int _lossB;
	int _lossA;
	int _shiftR;
	int _shiftG;
	int _shiftB;
	int _shiftA;

private:
	uint readPixel(const byte *pixel) const;
	int getChannel(uint pixelValue, uint mask, int shift) const;
};

} // End of namespace Grim

#endif
