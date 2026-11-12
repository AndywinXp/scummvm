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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/labitmap.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const int kDitherTable[16] = {
	0, 4, 1, 5,
	6, 2, 7, 3,
	1, 5, 0, 4,
	7, 3, 6, 2
};

LBitmap::~LBitmap() {
}

void LBitmap::setColorFormat(const LColorFormat &format) {
	colorFormat = format;
}

LString &LBitmap::getName() {
	return bitmapName;
}

void LBitmap::setName(const char *newName) {
	bitmapName = newName;
}

LABitmap::LABitmap(EmiEngine *engine) : LBitmap(engine) {
	ownedPixels = nullptr;
	pixels = nullptr;
	width = 0;
	height = 0;
	stride = 0;
	sizeBytes = 0;
}

LABitmap::LABitmap(EmiEngine *engine, int bitmapWidth, int bitmapHeight, const LColorFormat &format, void *bitmapPixels, bool useExternalPixels) : LBitmap(engine) {
	colorFormat = format;
	width = bitmapWidth;
	height = bitmapHeight;
	stride = bitmapWidth * format.bytesPerPixel;
	sizeBytes = bitmapHeight * stride;

	if (useExternalPixels) {
		ownedPixels = nullptr;
		pixels = bitmapPixels;
	} else {
		ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)sizeBytes * sizeof(byte));
		pixels = ownedPixels;

		if (ownedPixels) {
			if (bitmapPixels)
				_engine->getMemoryServices()->memcpyImpl(pixels, bitmapPixels, sizeBytes);
			else
				_engine->getMemoryServices()->memsetImpl(pixels, 0, sizeBytes);
		}
	}
}

LABitmap::~LABitmap() {
	if (ownedPixels) {
		_engine->getMemoryServices()->freeImpl(ownedPixels);
		ownedPixels = nullptr;
	}
}

int LABitmap::getWidth() {
	return width;
}

int LABitmap::getHeight() {
	return height;
}

int LABitmap::getStride() {
	return stride;
}

void *LABitmap::getPixels() {
	return pixels;
}

void LABitmap::create(int bitmapWidth, int bitmapHeight, const LColorFormat &format, void *bitmapPixels) {
	width = bitmapWidth;
	height = bitmapHeight;
	colorFormat = format;
	stride = width * colorFormat.bytesPerPixel;
	sizeBytes = height * stride;

	if (ownedPixels)
		_engine->getMemoryServices()->freeImpl(ownedPixels);

	if (bitmapPixels) {
		pixels = bitmapPixels;
		ownedPixels = nullptr;
	} else {
		ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)sizeBytes * sizeof(byte));
		pixels = ownedPixels;
	}

	if (pixels)
		_engine->getMemoryServices()->memsetImpl(pixels, 0, sizeBytes);
}

void LABitmap::convertFrom(LBitmap *source, const LColorFormat &format) {
	if (ownedPixels)
		_engine->getMemoryServices()->freeImpl(ownedPixels);

	width = source->getWidth();
	height = source->getHeight();
	colorFormat = format;
	stride = width * colorFormat.bytesPerPixel;
	sizeBytes = height * stride;

	ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)(width * height * format.bytesPerPixel) * sizeof(byte));
	pixels = ownedPixels;

	if (!ownedPixels)
		return;

	if (format.formatId == source->colorFormat.formatId)
		_engine->getMemoryServices()->memcpyImpl(pixels, source->getPixels(), source->colorFormat.bytesPerPixel * height * width);
	else
		source->colorFormat.convertPixels((const byte *)source->getPixels(), height * width, format, ownedPixels);
}

int LABitmap::scanAlphaRaw(uint rawWidth, uint rawHeight, const LColorFormat &format, const byte *rawPixels) {
	uint rowStart = 0;
	uint row = 0;
	bool hasPartialAlpha = false;
	bool hasZeroAlpha = false;

	if (rawHeight == 0)
		return 0;

	do {
		if (hasPartialAlpha)
			return 2;

		for (uint i = 0; i < rawWidth; i++) {
			if (hasPartialAlpha)
				break;

			byte alpha = rawPixels[format.bytesPerPixel * (rowStart + i) + 3];

			if (alpha)
				hasPartialAlpha = alpha != 255;
			else
				hasZeroAlpha = true;
		}

		rowStart += rawWidth;
		row++;
	} while (row < rawHeight);

	if (hasPartialAlpha)
		return 2;

	return hasZeroAlpha ? 1 : 0;
}

int LABitmap::scanAlpha() {
	int bitmapWidth = getWidth();
	int bitmapHeight = getHeight();
	const byte *bitmapPixels = (const byte *)getPixels();
	int rowStart = 0;
	int row = 0;
	bool hasPartialAlpha = false;
	bool hasZeroAlpha = false;

	if (bitmapHeight <= 0)
		return 0;

	do {
		if (hasPartialAlpha)
			return 2;

		for (int i = 0; i < bitmapWidth; i++) {
			if (hasPartialAlpha)
				break;

			byte alpha = (byte)colorFormat.getAlpha(&bitmapPixels[colorFormat.bytesPerPixel * (i + rowStart)]);

			if (alpha)
				hasPartialAlpha = alpha != 255;
			else
				hasZeroAlpha = true;
		}

		rowStart += bitmapWidth;
		row++;
	} while (row < bitmapHeight);

	if (hasPartialAlpha)
		return 2;

	return hasZeroAlpha ? 1 : 0;
}

void LABitmap::ditherPixels() {
	byte *rowStart = (byte *)pixels;

	for (int y = 0; y < height; y++) {
		byte *pixel = rowStart;

		for (int x = 0; x < width; x++) {
			int red = colorFormat.getRed(pixel);
			int green = colorFormat.getGreen(pixel);
			int blue = colorFormat.getBlue(pixel);
			int alpha = colorFormat.getAlpha(pixel);
			int dither = kDitherTable[4 * (y & 3) + (x & 3)];

			red += dither;
			if (red > 255)
				red = 255;

			green += dither;
			if (green > 255)
				green = 255;

			blue += dither;
			if (blue > 255)
				blue = 255;

			colorFormat.packPixel(pixel, alpha, red, green, blue);

			pixel += colorFormat.bytesPerPixel;
		}

		rowStart += colorFormat.bytesPerPixel * width;
	}
}

void LABitmap::convertFormat(const LColorFormat &format) {
	if (format.formatId == colorFormat.formatId)
		return;

	byte *newPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)(height * format.bytesPerPixel * width) * sizeof(byte));

	if (!newPixels)
		return;

	_engine->getMemoryServices()->memsetImpl(newPixels, 0, height * width * format.bytesPerPixel);
	colorFormat.convertPixels((const byte *)pixels, height * width, format, newPixels);

	if (ownedPixels)
		_engine->getMemoryServices()->freeImpl(ownedPixels);

	pixels = newPixels;
	ownedPixels = newPixels;

	colorFormat.setFormat(format.formatId);

	stride = width * colorFormat.bytesPerPixel;
	sizeBytes = height * stride;
}

int LABitmap::getSizeBytes() {
	return sizeBytes;
}

void LABitmap::setPixels(void *newPixels) {
	pixels = newPixels;
}

void LABitmap::setHeight(int newHeight) {
	height = newHeight;
}

} // End of namespace Grim
