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

#include "common/textconsole.h"

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/images/labitmap.h"
#include "engines/grim/emi/core/lmemoryservices.h"

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
	_colorFormat = format;
}

LString &LBitmap::getName() {
	return _bitmapName;
}

void LBitmap::setName(const char *newName) {
	_bitmapName = newName;
}

LABitmap::LABitmap(EmiEngine *engine) : LBitmap(engine) {
	_ownedPixels = nullptr;
	_pixels = nullptr;
	_width = 0;
	_height = 0;
	_stride = 0;
	_sizeBytes = 0;
}

LABitmap::LABitmap(EmiEngine *engine, int bitmapWidth, int bitmapHeight, const LColorFormat &format, void *bitmapPixels, bool useExternalPixels) : LBitmap(engine) {
	_colorFormat = format;
	_width = bitmapWidth;
	_height = bitmapHeight;
	_stride = bitmapWidth * format._bytesPerPixel;
	_sizeBytes = bitmapHeight * _stride;

	if (useExternalPixels) {
		_ownedPixels = nullptr;
		_pixels = bitmapPixels;
	} else {
		_ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)_sizeBytes * sizeof(byte));
		_pixels = _ownedPixels;

		if (_ownedPixels) {
			if (bitmapPixels)
				_engine->getMemoryServices()->memcpyImpl(_pixels, bitmapPixels, _sizeBytes);
			else
				_engine->getMemoryServices()->memsetImpl(_pixels, 0, _sizeBytes);
		}
	}
}

LABitmap::LABitmap(EmiEngine *engine, LABitmap *source, int x, int y, int rectWidth, int rectHeight) : LBitmap(engine) {
	_colorFormat = source->_colorFormat;
	_width = rectWidth;
	_height = rectHeight;
	_stride = rectWidth * _colorFormat._bytesPerPixel;
	_sizeBytes = rectHeight * _stride;
	_ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)_sizeBytes * sizeof(byte));
	_pixels = _ownedPixels;

	if (!_ownedPixels)
		return;

	_engine->getMemoryServices()->memsetImpl(_pixels, 0, _sizeBytes);

	// The original did not check the rectangle, let's avoid reading outside of the source...
	if (!source->getPixels() || rectWidth < 0 || rectHeight < 0 || x < 0 || y < 0 ||
			(int64)x + rectWidth > source->getWidth() || (int64)y + rectHeight > source->getHeight()) {
		warning("LABitmap::LABitmap(): rectangle (%d, %d, %d, %d) is outside of the source bitmap", x, y, rectWidth, rectHeight);

		return;
	}

	byte *sourceRow = (byte *)source->getPixels() + (int64)y * source->getStride() + (int64)x * _colorFormat._bytesPerPixel;
	byte *destinationRow = _ownedPixels;

	for (int row = 0; row < rectHeight; row++) {
		_engine->getMemoryServices()->memcpyImpl(destinationRow, sourceRow, _stride);
		sourceRow += source->getStride();
		destinationRow += _stride;
	}
}

LABitmap::~LABitmap() {
	if (_ownedPixels) {
		_engine->getMemoryServices()->freeImpl(_ownedPixels);
		_ownedPixels = nullptr;
	}
}

int LABitmap::getWidth() {
	return _width;
}

int LABitmap::getHeight() {
	return _height;
}

int LABitmap::getStride() {
	return _stride;
}

void *LABitmap::getPixels() {
	return _pixels;
}

void LABitmap::create(int bitmapWidth, int bitmapHeight, const LColorFormat &format, void *bitmapPixels) {
	_width = bitmapWidth;
	_height = bitmapHeight;
	_colorFormat = format;
	_stride = _width * _colorFormat._bytesPerPixel;
	_sizeBytes = _height * _stride;

	if (_ownedPixels)
		_engine->getMemoryServices()->freeImpl(_ownedPixels);

	if (bitmapPixels) {
		_pixels = bitmapPixels;
		_ownedPixels = nullptr;
	} else {
		_ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)_sizeBytes * sizeof(byte));
		_pixels = _ownedPixels;
	}

	if (_pixels)
		_engine->getMemoryServices()->memsetImpl(_pixels, 0, _sizeBytes);
}

void LABitmap::convertFrom(LBitmap *source, const LColorFormat &format) {
	if (_ownedPixels)
		_engine->getMemoryServices()->freeImpl(_ownedPixels);

	_width = source->getWidth();
	_height = source->getHeight();
	_colorFormat = format;
	_stride = _width * _colorFormat._bytesPerPixel;
	_sizeBytes = _height * _stride;

	_ownedPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)(_width * _height * format._bytesPerPixel) * sizeof(byte));
	_pixels = _ownedPixels;

	if (!_ownedPixels)
		return;

	if (format._formatId == source->_colorFormat._formatId)
		_engine->getMemoryServices()->memcpyImpl(_pixels, source->getPixels(), source->_colorFormat._bytesPerPixel * _height * _width);
	else
		source->_colorFormat.convertPixels((const byte *)source->getPixels(), _height * _width, format, _ownedPixels);
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

			byte alpha = rawPixels[format._bytesPerPixel * (rowStart + i) + 3];

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

			byte alpha = (byte)_colorFormat.getAlpha(&bitmapPixels[_colorFormat._bytesPerPixel * (i + rowStart)]);

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
	byte *rowStart = (byte *)_pixels;

	for (int y = 0; y < _height; y++) {
		byte *pixel = rowStart;

		for (int x = 0; x < _width; x++) {
			int red = _colorFormat.getRed(pixel);
			int green = _colorFormat.getGreen(pixel);
			int blue = _colorFormat.getBlue(pixel);
			int alpha = _colorFormat.getAlpha(pixel);
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

			_colorFormat.packPixel(pixel, alpha, red, green, blue);

			pixel += _colorFormat._bytesPerPixel;
		}

		rowStart += _colorFormat._bytesPerPixel * _width;
	}
}

void LABitmap::convertFormat(const LColorFormat &format) {
	if (format._formatId == _colorFormat._formatId)
		return;

	byte *newPixels = (byte *)_engine->getMemoryServices()->mallocImpl((uint)(_height * format._bytesPerPixel * _width) * sizeof(byte));

	if (!newPixels)
		return;

	_engine->getMemoryServices()->memsetImpl(newPixels, 0, _height * _width * format._bytesPerPixel);
	_colorFormat.convertPixels((const byte *)_pixels, _height * _width, format, newPixels);

	if (_ownedPixels)
		_engine->getMemoryServices()->freeImpl(_ownedPixels);

	_pixels = newPixels;
	_ownedPixels = newPixels;

	_colorFormat.setFormat(format._formatId);

	_stride = _width * _colorFormat._bytesPerPixel;
	_sizeBytes = _height * _stride;
}

int LABitmap::getSizeBytes() {
	return _sizeBytes;
}

void LABitmap::setPixels(void *newPixels) {
	_pixels = newPixels;
}

void LABitmap::setHeight(int newHeight) {
	_height = newHeight;
}

} // End of namespace Grim
