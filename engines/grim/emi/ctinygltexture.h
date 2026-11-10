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

#ifndef GRIM_EMI_CTINYGLTEXTURE_H
#define GRIM_EMI_CTINYGLTEXTURE_H

#include "common/scummsys.h"

#if defined(USE_TINYGL)

#include "graphics/tinygl/gl.h"

#include "engines/grim/emi/ctexture.h"

namespace Grim {

class CTinyGLContext;

class CTinyGLTexture : public CTexture {
public:
	CTinyGLTexture(CTinyGLContext *ctx, LABitmap *bitmap, bool textureFlag);
	~CTinyGLTexture() override;

	void applyFlags() override;
	void updateTexture(const LColorFormat *format, uint width, uint height, int stride, const byte *pixels) override;
	void setup(LABitmap *bitmap) override;
	int getSize() override;

	TGLuint glId;
	int boundFlush;

private:
	void storeBlank(int width, int height);
	void store8BitPixels(const byte *source, int width, int height, int bytesPerPixel);
	void store16BitPixels(const byte *source, int width, int height, int bytesPerPixel, int alphaKind);
	void upload();

	CTinyGLContext *_context;
	byte *_pixels;
	int _width;
	int _height;
	TGLint _internalFormat;
	TGLenum _pixelFormat;
	TGLenum _pixelType;
	bool _uploaded;
	uint _uploadedFilter;
};

} // End of namespace Grim

#endif

#endif
