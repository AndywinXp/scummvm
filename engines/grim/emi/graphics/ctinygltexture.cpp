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

#include "common/scummsys.h"

#if defined(USE_TINYGL)

#include "common/textconsole.h"

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/graphics/ctinyglcontext.h"
#include "engines/grim/emi/graphics/ctinygltexture.h"
#include "engines/grim/emi/emiengine.h"

namespace Grim {

CTinyGLTexture::CTinyGLTexture(CTinyGLContext *owner, LABitmap *bitmap, bool textureFlag) {
	_owner = owner;
	_engine = owner->getEngine();
	_pixels = nullptr;
	_width = 0;
	_height = 0;
	_internalFormat = TGL_RGBA;
	_pixelFormat = TGL_RGBA;
	_pixelType = TGL_UNSIGNED_BYTE;
	_uploaded = false;
	_uploadedFilter = 0;
	boundFlush = -1;

	tglGenTextures(1, &glId);

	setup(bitmap);
}

CTinyGLTexture::~CTinyGLTexture() {
	// TinyGL frees its textures together with the context
	if (_engine->getDrawContext())
		tglDeleteTextures(1, &glId);

	delete[] _pixels;
	_pixels = nullptr;
}

int CTinyGLTexture::getSize() {
	// The owner is gone once the engine has no draw context
	if (_engine->getDrawContext())
		return _owner->getTextureCost(this);

	return 0;
}

void CTinyGLTexture::applyFlags() {
	// TinyGL has no per-texture filter: it is fixed when the pixels are loaded
	if (!_pixels || (_uploaded && (flags & 3) == _uploadedFilter))
		return;

	upload();
}

void CTinyGLTexture::upload() {
	// TinyGL draws are queued: they must not see the new pixels
	if (boundFlush == _owner->flushCount)
		_owner->flushDrawCalls();

	tglBindTexture(TGL_TEXTURE_2D, glId);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, (flags & 2) ? TGL_LINEAR : TGL_NEAREST);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, (flags & 1) ? TGL_LINEAR : TGL_NEAREST);
	tglTexImage2D(TGL_TEXTURE_2D, 0, _internalFormat, _width, _height, 0, _pixelFormat, _pixelType, _pixels);
	tglBindTexture(TGL_TEXTURE_2D, 0);

	_uploaded = true;
	_uploadedFilter = flags & 3;
}

void CTinyGLTexture::storeBlank(int width, int height) {
	delete[] _pixels;
	_pixels = nullptr;
	_uploaded = false;

	if (width <= 0 || height <= 0) {
		warning("CTinyGLTexture::storeBlank(): invalid size %d x %d", width, height);

		return;
	}

	_pixels = new byte[width * height * 4]();
	_width = width;
	_height = height;
	_internalFormat = TGL_RGBA;
	_pixelFormat = TGL_RGBA;
	_pixelType = TGL_UNSIGNED_BYTE;
}

void CTinyGLTexture::store8BitPixels(const byte *source, int width, int height, int bytesPerPixel) {
	delete[] _pixels;
	_pixels = nullptr;
	_uploaded = false;

	if (width <= 0 || height <= 0) {
		warning("CTinyGLTexture::store8BitPixels(): invalid size %d x %d", width, height);

		return;
	}

	uint size = width * height * bytesPerPixel;

	_pixels = new byte[size];
	memcpy(_pixels, source, size);
	_width = width;
	_height = height;
	_internalFormat = (bytesPerPixel == 4) ? TGL_RGBA : TGL_RGB;
	_pixelFormat = _internalFormat;
	_pixelType = TGL_UNSIGNED_BYTE;
}

void CTinyGLTexture::store16BitPixels(const byte *source, int width, int height, int bytesPerPixel, int alphaKind) {
	delete[] _pixels;
	_pixels = nullptr;
	_uploaded = false;

	if (width <= 0 || height <= 0) {
		warning("CTinyGLTexture::store16BitPixels(): invalid size %d x %d", width, height);

		return;
	}

	uint count = width * height;

	_pixels = new byte[count * sizeof(uint16)];

	uint16 *packed = (uint16 *)_pixels;

	for (uint i = 0; i < count; i++) {
		const byte *pixel = source + i * bytesPerPixel;
		uint red = pixel[0];
		uint green = pixel[1];
		uint blue = pixel[2];
		uint alpha = (bytesPerPixel == 4) ? pixel[3] : 255;

		if (alphaKind == 2) {
			packed[i] = (((red * 15 + 127) / 255) << 12) | (((green * 15 + 127) / 255) << 8) | (((blue * 15 + 127) / 255) << 4) | ((alpha * 15 + 127) / 255);
		} else {
			uint alphaBit = (alphaKind == 1) ? ((alpha + 127) / 255) : 1;

			packed[i] = (((red * 31 + 127) / 255) << 11) | (((green * 31 + 127) / 255) << 6) | (((blue * 31 + 127) / 255) << 1) | alphaBit;
		}
	}

	_width = width;
	_height = height;
	_internalFormat = (alphaKind == 0) ? TGL_RGB : TGL_RGBA;
	_pixelFormat = TGL_RGBA;
	_pixelType = (alphaKind == 2) ? TGL_UNSIGNED_SHORT_4_4_4_4 : TGL_UNSIGNED_SHORT_5_5_5_1;
}

void CTinyGLTexture::setup(LABitmap *bitmap) {
	int width = bitmap->getWidth();
	int height = bitmap->getHeight();
	const byte *source = (const byte *)bitmap->getPixels();

	if (!source) {
		storeBlank(width, height);
		applyFlags();

		return;
	}

	int formatId = bitmap->colorFormat.formatId;

	if (formatId != 9 && formatId != 6) {
		warning("CTinyGLTexture::setup(): unsupported bitmap format %d", formatId);
		storeBlank(width, height);
		applyFlags();

		return;
	}

	int bytesPerPixel = (formatId == 9) ? 4 : 3;

	// TinyGL has no GL_RGB5/GL_RGBA4: the 16 bit precision is applied by hand
	if (_owner->displayMode.bpp == 32)
		store8BitPixels(source, width, height, bytesPerPixel);
	else
		store16BitPixels(source, width, height, bytesPerPixel, bitmap->scanAlpha());

	applyFlags();
}

void CTinyGLTexture::updateTexture(const LColorFormat *format, uint width, uint height, int stride, const byte *pixels) {
	delete[] _pixels;
	_pixels = nullptr;
	_uploaded = false;

	if (width == 0 || height == 0) {
		warning("CTinyGLTexture::updateTexture(): invalid size %u x %u", width, height);

		return;
	}

	uint rowBytes = width * format->bytesPerPixel;

	_pixels = new byte[height * rowBytes];

	byte *destination = _pixels;

	for (uint i = 0; i < height; i++) {
		memcpy(destination, pixels, rowBytes);
		destination += rowBytes;
		pixels += stride;
	}

	_width = width;
	_height = height;

	if (format->formatId == 9) {
		_internalFormat = TGL_RGBA;
		_pixelFormat = TGL_RGBA;
		_pixelType = TGL_UNSIGNED_BYTE;
	} else {
		_internalFormat = TGL_RGB;
		_pixelFormat = TGL_RGB;
		_pixelType = TGL_UNSIGNED_SHORT_5_6_5;
	}

	applyFlags();
}

} // End of namespace Grim

#endif
