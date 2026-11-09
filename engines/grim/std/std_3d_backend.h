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

#ifndef GRIM_STD_STD_3D_BACKEND_H
#define GRIM_STD_STD_3D_BACKEND_H

#include "common/scummsys.h"

#include "engines/grim/std/std_3d_config.h"
#include "engines/grim/std/std_3d.h"

namespace Grim {

// Compare functions (zCompareFunc, alphaFunc) use the D3D numbering:
// 1 never, 2 less, 3 equal, 4 less-equal, 5 greater, 6 not-equal, 7 greater-equal, 8 always
struct Std3DDrawList {
	Std3DSurface *pTarget;
	const Std3DVertex *pVertices;
	uint numVertices;
	const uint16 *pIndices;
	uint numIndices;
	const Std3DCommand *pCommands;
	uint numCommands;
	uint initialRenderState;
	int bZEnable;
	uint zCompareFunc;
	uint alphaFunc;
	float fogStart;
	float fogEnd;
	uint fogRed;
	uint fogGreen;
	uint fogBlue;
};

class Std3DBackend {
public:
	virtual ~Std3DBackend() {}

	virtual int openGraphicsMode(int width, int height) = 0;
	virtual void fillDeviceDesc(Std3DDeviceDesc *pDesc) = 0;
	virtual int isSupported() = 0;
	virtual int canProbeWithoutMode() = 0;
	virtual int ensureReady() = 0;
	virtual const char *getName() = 0;
	virtual const char *getDescription() = 0;
	virtual const byte *getGuid() = 0;
	virtual int getMaxTextureSize() = 0;
	virtual uint getTextureBudget() = 0;

	virtual int createSurface(Std3DSurface *pSurface, int bHasDepth, int zBufferBitDepth) = 0;
	virtual void destroySurface(Std3DSurface *pSurface) = 0;
	virtual int uploadPixels(Std3DSurface *pSurface, const StdRect *pRect, const byte *pPixels, int pitchInBytes) = 0;
	virtual int readPixels(Std3DSurface *pSurface, const StdRect *pRect, byte *pOut, int pitchInBytes) = 0;
	virtual int fillRect(Std3DSurface *pSurface, const StdRect *pRect, uint16 color) = 0;
	virtual int canBlitOntoSelf() = 0;
	virtual int blit(Std3DSurface *pDst, Std3DSurface *pSrc, int srcX, int srcY, int width, int height, int dstX, int dstY, int bColorKey, uint16 colorKey) = 0;
	virtual int presentSurface(Std3DSurface *pSurface) = 0;

	virtual int attachDepth(Std3DSurface *pSurface, int zBufferBitDepth) = 0;
	virtual void detachDepth(Std3DSurface *pSurface) = 0;
	virtual int hasDepth(const Std3DSurface *pSurface) = 0;
	virtual int supportsDepthUpload(const Std3DSurface *pSurface) = 0;
	virtual int clearDepth(Std3DSurface *pSurface, const StdRect *pRect, float depthValue) = 0;
	virtual int uploadDepth(Std3DSurface *pSurface, const StdRect *pRect, const byte *pZData, int pitchInBytes) = 0;

	virtual uint createTexture(const byte *pRgba, int width, int height, int bMagLinear, int bMinLinear) = 0;
	virtual void deleteTexture(uint hTexture) = 0;

	virtual int drawList(const Std3DDrawList *pList) = 0;
};

#if defined(GRIM_STD3D_GL)
Std3DBackend *createGlBackend();
#endif

#if defined(GRIM_STD3D_TINYGL)
Std3DBackend *createTinyGlBackend();
#endif

} // End of namespace Grim

#endif
