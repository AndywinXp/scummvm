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

#ifndef GRIM_STD_STD_3D_TINYGL_H
#define GRIM_STD_STD_3D_TINYGL_H

#include "engines/grim/std/std_3d_config.h"

#if defined(GRIM_STD3D_TINYGL)

#include "common/array.h"

#include "graphics/tinygl/tinygl.h"

#include "engines/grim/std/std_3d_backend.h"

namespace Grim {

struct Std3DTinyGlSurface;

class Std3DTinyGlBackend : public Std3DBackend {
public:
	Std3DTinyGlBackend();
	~Std3DTinyGlBackend();

	int openGraphicsMode(int width, int height);
	void fillDeviceDesc(Std3DDeviceDesc *desc);
	int isSupported();
	int canProbeWithoutMode();
	int ensureReady();
	const char *getName();
	const char *getDescription();
	const byte *getGuid();
	int getMaxTextureSize();
	uint getTextureBudget();

	int createSurface(Std3DSurface *surface, int hasDepth, int zBufferBitDepth);
	void destroySurface(Std3DSurface *surface);
	int uploadPixels(Std3DSurface *surface, const StdRect *rectPointer, const byte *pixels, int pitchInBytes);
	int readPixels(Std3DSurface *surface, const StdRect *rectPointer, byte *out, int pitchInBytes);
	int fillRect(Std3DSurface *surface, const StdRect *rectPointer, uint16 color);
	int canBlitOntoSelf();
	int blit(Std3DSurface *dst, Std3DSurface *src, int srcX, int srcY, int width, int height, int dstX, int dstY, int colorKeyFlag, uint16 colorKey);
	int presentSurface(Std3DSurface *surface);

	int attachDepth(Std3DSurface *surface, int zBufferBitDepth);
	void detachDepth(Std3DSurface *surface);
	int hasDepth(const Std3DSurface *surface);
	int supportsDepthUpload(const Std3DSurface *surface);
	int clearDepth(Std3DSurface *surface, const StdRect *rect, float depthValue);
	int uploadDepth(Std3DSurface *surface, const StdRect *rectPointer, const byte *zData, int pitchInBytes);

	uint createTexture(const byte *rgba, int width, int height, int magLinear, int minLinear);
	void deleteTexture(uint textureHandle);

	int drawList(const Std3DDrawList *list);

private:
	void releaseContext(Std3DTinyGlSurface *data, int width, int height, int keepPixels);
	void flushQueue(Std3DTinyGlSurface *data);
	void prepareColorAccess(Std3DTinyGlSurface *data);
	void reserveQueue(Std3DTinyGlSurface *data, uint bytes);
	void applyRenderState(const Std3DDrawList *list, uint flags);
	void removeSurface(Std3DSurface *surface);

	int _modeReady;
	int _warnedZFunc;
	int _warnedTexCoord;
	TinyGL::ContextHandle *_textureContext;
	Common::Array<uint> _textures;
	Common::Array<Std3DSurface *> _surfaces;
};

} // End of namespace Grim

#endif

#endif
