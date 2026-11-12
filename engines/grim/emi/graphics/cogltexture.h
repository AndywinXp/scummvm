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

#ifndef GRIM_EMI_COGLTEXTURE_H
#define GRIM_EMI_COGLTEXTURE_H

#include "common/scummsys.h"

#if defined(USE_OPENGL_GAME)

#include "graphics/opengl/system_headers.h"

#include "engines/grim/emi/graphics/ctexture.h"

namespace Grim {

class COGLContext;
class EmiEngine;

class COGLTexture : public CTexture {
public:
	COGLTexture(COGLContext *owner, LABitmap *bitmap, bool textureFlag);
	~COGLTexture() override;

	void applyFlags() override;
	void updateTexture(const LColorFormat *format, uint width, uint height, int stride, const byte *pixels) override;
	void setup(LABitmap *bitmap) override;
	int getSize() override;

	GLuint _glId;

private:
	void uploadBlankImage(int width, int height);

	COGLContext *_owner;
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif

#endif
