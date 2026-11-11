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

#if defined(USE_OPENGL_GAME)

#include "common/textconsole.h"

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/coglcontext.h"
#include "engines/grim/emi/cogltexture.h"

namespace Grim {

COGLTexture::COGLTexture(COGLContext *ctx, LABitmap *bitmap, bool textureFlag) {
	glGenTextures(1, &glId);

	if (ctx)
		CDrawContext::context = ctx;

	setup(bitmap);
}

COGLTexture::~COGLTexture() {
	glDeleteTextures(1, &glId);
}

int COGLTexture::getSize() {
	if (CDrawContext::context)
		return CDrawContext::context->getTextureCost(this);

	return 0;
}

void COGLTexture::applyFlags() {
	glBindTexture(GL_TEXTURE_2D, glId);
	// Not in the disasm (GL_CLAMP), the current drivers blend the edge with the border color
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (flags & 0x100) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (flags & 0x200) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & 2) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (flags & 1) ? GL_LINEAR : GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void COGLTexture::uploadBlankImage(int width, int height) {
	if (width <= 0 || height <= 0) {
		warning("COGLTexture::uploadBlankImage(): invalid size %d x %d", width, height);

		return;
	}

	byte *blank = new byte[width * height * 4]();

	glBindTexture(GL_TEXTURE_2D, glId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, blank);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] blank;
}

void COGLTexture::setup(LABitmap *bitmap) {
	int width = bitmap->getWidth();
	int height = bitmap->getHeight();
	void *pixels = bitmap->getPixels();

	if (!pixels) {
		uploadBlankImage(width, height);
		applyFlags();

		return;
	}

	GLenum format;

	if (bitmap->colorFormat.formatId == 9) {
		format = GL_RGBA;
	} else if (bitmap->colorFormat.formatId == 6) {
		format = GL_RGB;
	} else {
		warning("COGLTexture::setup(): unsupported bitmap format %d", bitmap->colorFormat.formatId);
		uploadBlankImage(width, height);
		applyFlags();

		return;
	}

	GLint internalFormat;

	if (!CDrawContext::context || CDrawContext::context->displayMode.bpp == 32) {
		internalFormat = GL_RGBA8;
	} else {
		int alphaKind = bitmap->scanAlpha();

		if (alphaKind == 0)
			internalFormat = GL_RGB5;
		else if (alphaKind == 1)
			internalFormat = GL_RGB5_A1;
		else
			internalFormat = GL_RGBA4;
	}

	glBindTexture(GL_TEXTURE_2D, glId);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	applyFlags();
}

void COGLTexture::updateTexture(const LColorFormat *format, uint width, uint height, int stride, const byte *pixels) {
	GLenum pixelFormat;
	GLenum pixelType;

	if (format->formatId == 9) {
		pixelFormat = GL_RGBA;
		pixelType = GL_UNSIGNED_BYTE;
	} else {
		pixelFormat = GL_RGB;
		pixelType = GL_UNSIGNED_SHORT_5_6_5;
	}

	uint rowBytes = width * format->bytesPerPixel;
	byte *rows = new byte[height * rowBytes];
	byte *destination = rows;

	for (uint i = 0; i < height; i++) {
		memcpy(destination, pixels, rowBytes);
		destination += rowBytes;
		pixels += stride;
	}

	glBindTexture(GL_TEXTURE_2D, glId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, pixelFormat, pixelType, rows);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] rows;

	applyFlags();
}

} // End of namespace Grim

#endif
