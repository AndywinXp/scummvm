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

#ifndef GRIM_EMI_LABITMAP_H
#define GRIM_EMI_LABITMAP_H

#include "engines/grim/emi/images/lbitmap.h"

namespace Grim {

class LABitmap : public LBitmap {
public:
	explicit LABitmap(EmiEngine *engine);
	LABitmap(EmiEngine *engine, int bitmapWidth, int bitmapHeight, const LColorFormat &format, void *bitmapPixels, bool useExternalPixels);
	LABitmap(EmiEngine *engine, LABitmap *source, int x, int y, int rectWidth, int rectHeight);
	~LABitmap();

	int getWidth() override;
	int getHeight() override;
	int getStride() override;
	void *getPixels() override;
	void create(int bitmapWidth, int bitmapHeight, const LColorFormat &format, void *bitmapPixels) override;

	virtual void convertFrom(LBitmap *source, const LColorFormat &format);
	virtual int scanAlphaRaw(uint rawWidth, uint rawHeight, const LColorFormat &format, const byte *rawPixels);
	virtual int scanAlpha();
	virtual void ditherPixels();
	virtual void convertFormat(const LColorFormat &format);
	virtual int getSizeBytes();
	virtual void setPixels(void *newPixels);
	virtual void setHeight(int newHeight);

	byte *_ownedPixels;
	void *_pixels;
	int _width;
	int _height;
	int _stride;
	int _sizeBytes;
};

} // End of namespace Grim

#endif
