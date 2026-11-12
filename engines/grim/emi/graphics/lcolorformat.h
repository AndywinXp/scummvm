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

	static const LColorFormat format1;
	static const LColorFormat format2;
	static const LColorFormat format3;
	static const LColorFormat format4;
	static const LColorFormat format5;
	static const LColorFormat format6;
	static const LColorFormat format9;
	static const LColorFormat format10;
	static const LColorFormat format11;
	static const LColorFormat format12;
	static const LColorFormat format13;
	static const LColorFormat format14;
	static const LColorFormat format15;
	static const LColorFormat format16;

	int formatId;
	int bytesPerPixel;
	int alphaBits;
	uint maskR;
	uint maskG;
	uint maskB;
	uint maskA;
	int lossR;
	int lossG;
	int lossB;
	int lossA;
	int shiftR;
	int shiftG;
	int shiftB;
	int shiftA;

private:
	uint readPixel(const byte *pixel) const;
	int getChannel(uint pixelValue, uint mask, int shift) const;
};

} // End of namespace Grim

#endif
