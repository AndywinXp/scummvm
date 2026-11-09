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

#include "engines/grim/std/std_3d_config.h"

#if defined(GRIM_STD3D_TINYGL)

#include "common/endian.h"
#include "common/list.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/util.h"

#include "graphics/pixelformat.h"
#include "graphics/surface.h"

#include "engines/grim/std/std_3d_tinygl.h"

namespace Grim {

enum {
	kCmpCapsGreater = 0x10,
	kBitDepth16 = 0x400
};

enum {
	kMaxTextureSize = 4096,
	kTextureSize = 256,
	kDrawCallMemory = 8 * 1024 * 1024,
	kQueueFlushThreshold = 6 * 1024 * 1024,
	kBytesPerVertex = 152,
	kBytesPerDrawCall = 512,
	kBytesPerSmallCall = 256,
	kZShift = 14,
	kTexCoordShift = 14,
	kTextureBudget = 2 * 1024 * 1024
};

static const float tinyGlZOffset = (float)(8192.0 / (1073741824.0 - 0.5));

static const float tinyGlTexCoordLimit = (float)(2147483648.0 / (double)((kTextureSize << kTexCoordShift) - 1));

static const byte syntheticDeviceGuid[16] = {
	0x53, 0x63, 0x75, 0x6D, 0x6D, 0x56, 0x4D, 0x20, 0x54, 0x69, 0x6E, 0x79, 0x47, 0x4C, 0x00, 0x02
};

struct Std3DTinyGlSurface {
	byte *pCpu;
	TinyGL::ContextHandle *pContext;
	byte *pPixels;
	int pitch;
	int bPendingColor;
	uint queuedBytes;
};

static Graphics::PixelFormat displayFormat() {
	return Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
}

static Std3DTinyGlSurface *tinyGlData(const Std3DSurface *pSurface) {
	return (Std3DTinyGlSurface *)pSurface->pBackendData;
}

static TGLenum tinyGlDepthFunc(uint d3dFunc) {
	switch (d3dFunc) {
	case 1:
		return TGL_NEVER;
	case 2:
		return TGL_GREATER;
	case 3:
		return TGL_EQUAL;
	case 4:
		return TGL_GEQUAL;
	case 5:
		return TGL_LESS;
	case 6:
		return TGL_NOTEQUAL;
	case 7:
		return TGL_LEQUAL;
	case 8:
		return TGL_ALWAYS;
	default:
		warning("Std3DTinyGlBackend: invalid z compare function %u", d3dFunc);
		return TGL_ALWAYS;
	}
}

static TGLenum tinyGlAlphaFunc(uint d3dFunc) {
	switch (d3dFunc) {
	case 1:
		return TGL_NEVER;
	case 2:
		return TGL_LESS;
	case 3:
		return TGL_EQUAL;
	case 4:
		return TGL_LEQUAL;
	case 5:
		return TGL_GREATER;
	case 6:
		return TGL_NOTEQUAL;
	case 7:
		return TGL_GEQUAL;
	case 8:
		return TGL_ALWAYS;
	default:
		warning("Std3DTinyGlBackend: invalid alpha compare function %u", d3dFunc);
		return TGL_ALWAYS;
	}
}

Std3DTinyGlBackend::Std3DTinyGlBackend() : _bModeReady(0), _bWarnedZFunc(0), _bWarnedTexCoord(0), _pTextureContext(nullptr) {
}

Std3DTinyGlBackend::~Std3DTinyGlBackend() {
	while (!_surfaces.empty())
		destroySurface(_surfaces.back());
}

int Std3DTinyGlBackend::openGraphicsMode(int width, int height) {
	Graphics::PixelFormat format = displayFormat();

	_bModeReady = 0;
	initGraphics(width, height, &format);

	if (g_system->getScreenFormat() != format) {
		warning("Std3DTinyGlBackend::openGraphicsMode(): the screen does not support the RGB565 pixel format");
		return 0;
	}

	_bModeReady = 1;
	return 1;
}

void Std3DTinyGlBackend::fillDeviceDesc(Std3DDeviceDesc *pDesc) {
	pDesc->zCmpCaps = kCmpCapsGreater;
	pDesc->deviceRenderBitDepth = kBitDepth16;
	pDesc->deviceZBufferBitDepth = kBitDepth16;
}

int Std3DTinyGlBackend::isSupported() {
	if (_bModeReady)
		return 1;

	Graphics::PixelFormat format = displayFormat();
	Common::List<Graphics::PixelFormat> formats = g_system->getSupportedFormats();

	for (Common::List<Graphics::PixelFormat>::const_iterator it = formats.begin(); it != formats.end(); ++it) {
		if (*it == format)
			return 1;
	}

	return 0;
}

int Std3DTinyGlBackend::canProbeWithoutMode() {
	return 1;
}

int Std3DTinyGlBackend::ensureReady() {
	return _bModeReady;
}

const char *Std3DTinyGlBackend::getName() {
	return "TinyGL";
}

const char *Std3DTinyGlBackend::getDescription() {
	return "ScummVM TinyGL";
}

const byte *Std3DTinyGlBackend::getGuid() {
	return syntheticDeviceGuid;
}

int Std3DTinyGlBackend::getMaxTextureSize() {
	return kMaxTextureSize;
}

uint Std3DTinyGlBackend::getTextureBudget() {
	return kTextureBudget;
}

void Std3DTinyGlBackend::removeSurface(Std3DSurface *pSurface) {
	for (uint i = 0; i < _surfaces.size(); i++) {
		if (_surfaces[i] == pSurface) {
			_surfaces[i] = _surfaces.back();
			_surfaces.pop_back();
			return;
		}
	}
}

void Std3DTinyGlBackend::flushQueue(Std3DTinyGlSurface *pData) {
	TinyGL::setContext(pData->pContext);
	TinyGL::presentBuffer();
	pData->bPendingColor = 0;
	pData->queuedBytes = 0;
}

void Std3DTinyGlBackend::prepareColorAccess(Std3DTinyGlSurface *pData) {
	if (pData->pContext && pData->bPendingColor)
		flushQueue(pData);
}

void Std3DTinyGlBackend::reserveQueue(Std3DTinyGlSurface *pData, uint bytes) {
	if (pData->queuedBytes + bytes > kQueueFlushThreshold)
		flushQueue(pData);

	pData->queuedBytes += bytes;
}

void Std3DTinyGlBackend::releaseContext(Std3DTinyGlSurface *pData, int width, int height, int bKeepPixels) {
	TinyGL::ContextHandle *pContext = pData->pContext;

	TinyGL::setContext(pContext);

	if (_pTextureContext == pContext) {
		for (uint i = 0; i < _textures.size(); i++) {
			TGLuint texture = _textures[i];

			tglDeleteTextures(1, &texture);
		}

		_textures.clear();
		_pTextureContext = nullptr;
	}

	if (bKeepPixels) {
		flushQueue(pData);

		int pitch = width * 2;
		byte *pCpu = new byte[pitch * height];

		for (int y = 0; y < height; y++)
			memcpy(pCpu + y * pitch, pData->pPixels + y * pData->pitch, pitch);

		pData->pCpu = pCpu;
		pData->pPixels = pCpu;
		pData->pitch = pitch;
	} else {
		pData->pPixels = nullptr;
	}

	TinyGL::destroyContext(pContext);
	pData->pContext = nullptr;
	pData->bPendingColor = 0;
	pData->queuedBytes = 0;
}

int Std3DTinyGlBackend::createSurface(Std3DSurface *pSurface, int bHasDepth, int zBufferBitDepth) {
	Std3DTinyGlSurface *pData = new Std3DTinyGlSurface;

	memset(pData, 0, sizeof(Std3DTinyGlSurface));
	pData->pitch = pSurface->width * 2;
	pData->pCpu = new byte[pData->pitch * pSurface->height];
	memset(pData->pCpu, 0, pData->pitch * pSurface->height);
	pData->pPixels = pData->pCpu;

	pSurface->pBackendData = pData;
	_surfaces.push_back(pSurface);

	if (bHasDepth && !attachDepth(pSurface, zBufferBitDepth)) {
		destroySurface(pSurface);
		return 0;
	}

	return 1;
}

void Std3DTinyGlBackend::destroySurface(Std3DSurface *pSurface) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	if (!pData)
		return;

	if (pData->pContext)
		releaseContext(pData, pSurface->width, pSurface->height, 0);

	delete[] pData->pCpu;
	delete pData;
	pSurface->pBackendData = nullptr;
	removeSurface(pSurface);
}

int Std3DTinyGlBackend::uploadPixels(Std3DSurface *pSurface, const StdRect *pRect, const byte *pPixels, int pitchInBytes) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	prepareColorAccess(pData);

	for (int y = 0; y < pRect->height; y++) {
		const byte *pIn = pPixels + y * pitchInBytes;
		uint16 *pOut = (uint16 *)(pData->pPixels + (pRect->y + y) * pData->pitch) + pRect->x;

		for (int x = 0; x < pRect->width; x++)
			pOut[x] = READ_LE_UINT16(pIn + 2 * x);
	}

	return 1;
}

int Std3DTinyGlBackend::readPixels(Std3DSurface *pSurface, const StdRect *pRect, byte *pOut, int pitchInBytes) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	prepareColorAccess(pData);

	for (int y = 0; y < pRect->height; y++) {
		const uint16 *pIn = (const uint16 *)(pData->pPixels + (pRect->y + y) * pData->pitch) + pRect->x;
		byte *pRow = pOut + y * pitchInBytes;

		for (int x = 0; x < pRect->width; x++)
			WRITE_LE_UINT16(pRow + 2 * x, pIn[x]);
	}

	return 1;
}

int Std3DTinyGlBackend::fillRect(Std3DSurface *pSurface, const StdRect *pRect, uint16 color) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	prepareColorAccess(pData);

	for (int y = 0; y < pRect->height; y++) {
		uint16 *pOut = (uint16 *)(pData->pPixels + (pRect->y + y) * pData->pitch) + pRect->x;

		for (int x = 0; x < pRect->width; x++)
			pOut[x] = color;
	}

	return 1;
}

int Std3DTinyGlBackend::canBlitOntoSelf() {
	return 1;
}

int Std3DTinyGlBackend::blit(Std3DSurface *pDst, Std3DSurface *pSrc, int srcX, int srcY, int width, int height, int dstX, int dstY, int bColorKey, uint16 colorKey) {
	Std3DTinyGlSurface *pDstData = tinyGlData(pDst);
	Std3DTinyGlSurface *pSrcData = tinyGlData(pSrc);

	prepareColorAccess(pDstData);

	if (pSrcData != pDstData)
		prepareColorAccess(pSrcData);

	int bReverseRows = pDst == pSrc && dstY > srcY;
	int bReverseColumns = pDst == pSrc && dstY == srcY && dstX > srcX;

	for (int i = 0; i < height; i++) {
		int y = bReverseRows ? height - 1 - i : i;
		const uint16 *pIn = (const uint16 *)(pSrcData->pPixels + (srcY + y) * pSrcData->pitch) + srcX;
		uint16 *pOut = (uint16 *)(pDstData->pPixels + (dstY + y) * pDstData->pitch) + dstX;

		if (!bColorKey) {
			memmove(pOut, pIn, width * 2);
		} else if (bReverseColumns) {
			for (int x = width - 1; x >= 0; x--) {
				if (pIn[x] != colorKey)
					pOut[x] = pIn[x];
			}
		} else {
			for (int x = 0; x < width; x++) {
				if (pIn[x] != colorKey)
					pOut[x] = pIn[x];
			}
		}
	}

	return 1;
}

int Std3DTinyGlBackend::presentSurface(Std3DSurface *pSurface) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	prepareColorAccess(pData);
	g_system->copyRectToScreen(pData->pPixels, pData->pitch, 0, 0, pSurface->width, pSurface->height);
	return 1;
}

int Std3DTinyGlBackend::attachDepth(Std3DSurface *pSurface, int zBufferBitDepth) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	(void)zBufferBitDepth;

	if (pData->pContext)
		return 1;

	int width = pSurface->width;
	int height = pSurface->height;
	TinyGL::ContextHandle *pContext = TinyGL::createContext(width, height, displayFormat(), kTextureSize, false, false, kDrawCallMemory);

	tglDisable(TGL_LIGHTING);
	tglDisable(TGL_CULL_FACE);
	tglDisable(TGL_FOG);
	tglDisable(TGL_STENCIL_TEST);
	tglDisable(TGL_POLYGON_OFFSET_FILL);
	tglDisable(TGL_SCISSOR_TEST);
	tglDisable(TGL_POLYGON_STIPPLE);
	tglShadeModel(TGL_SMOOTH);
	tglEnable(TGL_ALPHA_TEST);
	tglAlphaFunc(TGL_GREATER, 0.0f);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_S, TGL_REPEAT);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_WRAP_T, TGL_REPEAT);
	tglTexEnvi(TGL_TEXTURE_ENV, TGL_TEXTURE_ENV_MODE, TGL_MODULATE);

	tglMatrixMode(TGL_PROJECTION);
	tglLoadIdentity();
	tglOrtho(0.0, width - 0.5, height - 0.5, 0.0, -1.0, 0.0);
	tglMatrixMode(TGL_MODELVIEW);
	tglLoadIdentity();
	tglMatrixMode(TGL_TEXTURE);
	tglLoadIdentity();
	tglMatrixMode(TGL_MODELVIEW);
	tglViewport(0, 0, width, height);

	Graphics::Surface frame;

	TinyGL::getSurfaceRef(frame);

	byte *pFramePixels = (byte *)frame.getPixels();
	int framePitch = frame.pitch;

	for (int y = 0; y < height; y++)
		memcpy(pFramePixels + y * framePitch, pData->pCpu + y * pData->pitch, width * 2);

	delete[] pData->pCpu;
	pData->pCpu = nullptr;
	pData->pContext = pContext;
	pData->pPixels = pFramePixels;
	pData->pitch = framePitch;
	pData->bPendingColor = 0;
	pData->queuedBytes = 0;

	if (!_pTextureContext)
		_pTextureContext = pContext;

	return 1;
}

void Std3DTinyGlBackend::detachDepth(Std3DSurface *pSurface) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	if (!pData || !pData->pContext)
		return;

	releaseContext(pData, pSurface->width, pSurface->height, 1);
}

int Std3DTinyGlBackend::hasDepth(const Std3DSurface *pSurface) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	return pData && pData->pContext;
}

int Std3DTinyGlBackend::supportsDepthUpload(const Std3DSurface *pSurface) {
	return hasDepth(pSurface);
}

int Std3DTinyGlBackend::clearDepth(Std3DSurface *pSurface, const StdRect *pRect, float depthValue) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	if (depthValue != 0.0f) {
		warning("Std3DTinyGlBackend::clearDepth(): only the greater z compare function is supported");
		return 0;
	}

	reserveQueue(pData, kBytesPerSmallCall);
	TinyGL::setContext(pData->pContext);

	tglEnable(TGL_SCISSOR_TEST);
	tglScissor(pRect->x, pSurface->height - pRect->y - pRect->height, pRect->width, pRect->height);
	tglClearDepth(0.0);
	tglClear(TGL_DEPTH_BUFFER_BIT);
	tglDisable(TGL_SCISSOR_TEST);
	return 1;
}

int Std3DTinyGlBackend::uploadDepth(Std3DSurface *pSurface, const StdRect *pRect, const byte *pZData, int pitchInBytes) {
	Std3DTinyGlSurface *pData = tinyGlData(pSurface);

	reserveQueue(pData, kBytesPerSmallCall);
	TinyGL::setContext(pData->pContext);

	Graphics::Surface zSurface;

	zSurface.create((int16)pRect->width, (int16)pRect->height, Graphics::PixelFormat::createFormatRGBA32());

	for (int y = 0; y < pRect->height; y++) {
		const byte *pRow = pZData + y * pitchInBytes;
		uint32 *pOut = (uint32 *)zSurface.getBasePtr(0, y);

		for (int x = 0; x < pRect->width; x++)
			pOut[x] = (uint32)READ_LE_UINT16(pRow + 2 * x) << kZShift;
	}

	TinyGL::BlitImage *pImage = tglGenBlitImage();

	tglUploadBlitImage(pImage, zSurface, 0, false, true);
	tglBlitZBuffer(pImage, pRect->x, pRect->y);
	tglDeleteBlitImage(pImage);

	zSurface.free();
	return 1;
}

uint Std3DTinyGlBackend::createTexture(const byte *pRgba, int width, int height, int bMagLinear, int bMinLinear) {
	if (!_pTextureContext) {
		warning("Std3DTinyGlBackend::createTexture(): no rendering context");
		return 0;
	}

	TinyGL::setContext(_pTextureContext);

	TGLuint texture = 0;

	tglGenTextures(1, &texture);

	if (!texture)
		return 0;

	tglBindTexture(TGL_TEXTURE_2D, texture);
	TGLint minFilter = TGL_NEAREST;
	TGLint magFilter = TGL_NEAREST;

	if (bMinLinear)
		minFilter = TGL_LINEAR;

	if (bMagLinear)
		magFilter = TGL_LINEAR;

	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MIN_FILTER, minFilter);
	tglTexParameteri(TGL_TEXTURE_2D, TGL_TEXTURE_MAG_FILTER, magFilter);
	tglTexImage2D(TGL_TEXTURE_2D, 0, TGL_RGBA, width, height, 0, TGL_RGBA, TGL_UNSIGNED_BYTE, pRgba);
	tglBindTexture(TGL_TEXTURE_2D, 0);

	_textures.push_back(texture);
	return texture;
}

void Std3DTinyGlBackend::deleteTexture(uint hTexture) {
	if (!hTexture || !_pTextureContext)
		return;

	for (uint i = 0; i < _textures.size(); i++) {
		if (_textures[i] == hTexture) {
			_textures[i] = _textures.back();
			_textures.pop_back();
			break;
		}
	}

	TGLuint texture = hTexture;

	TinyGL::setContext(_pTextureContext);
	tglDeleteTextures(1, &texture);
}

void Std3DTinyGlBackend::applyRenderState(const Std3DDrawList *pList, uint flags) {
	if (flags & (kStd3DRsAlphaBlend | kStd3DRsAlphaTexture)) {
		tglEnable(TGL_BLEND);
		tglBlendFunc(TGL_SRC_ALPHA, TGL_ONE_MINUS_SRC_ALPHA);
	} else {
		tglDisable(TGL_BLEND);
	}

	if (pList->bZEnable) {
		tglEnable(TGL_DEPTH_TEST);

		if (flags & kStd3DRsZTest)
			tglDepthFunc(tinyGlDepthFunc(pList->zCompareFunc));
		else
			tglDepthFunc(TGL_ALWAYS);

		tglDepthMask((flags & kStd3DRsZWrite) != 0 ? TGL_TRUE : TGL_FALSE);
	} else {
		tglDisable(TGL_DEPTH_TEST);
		tglDepthMask(TGL_FALSE);
	}
}

int Std3DTinyGlBackend::drawList(const Std3DDrawList *pList) {
	Std3DTinyGlSurface *pData = tinyGlData(pList->pTarget);

	if (!pData || !pData->pContext) {
		warning("Std3DTinyGlBackend::drawList(): the target has no rendering context");
		return 0;
	}

	if (pList->zCompareFunc != 5 && !_bWarnedZFunc) {
		warning("Std3DTinyGlBackend::drawList(): only the greater z compare function is supported");
		_bWarnedZFunc = 1;
	}

	TinyGL::setContext(pData->pContext);

	int bTextured = 0;

	tglEnable(TGL_ALPHA_TEST);
	tglAlphaFunc(tinyGlAlphaFunc(pList->alphaFunc), 0.0f);
	tglDisable(TGL_TEXTURE_2D);
	tglBindTexture(TGL_TEXTURE_2D, 0);
	applyRenderState(pList, pList->initialRenderState);

	for (uint c = 0; c < pList->numCommands; c++) {
		const Std3DCommand &command = pList->pCommands[c];

		switch (command.type) {
		case kStd3DCommandRenderState:
			applyRenderState(pList, command.flags);
			break;
		case kStd3DCommandTexture:
			if (command.texture) {
				tglEnable(TGL_TEXTURE_2D);
				tglBindTexture(TGL_TEXTURE_2D, command.texture);
				bTextured = 1;
			} else {
				tglDisable(TGL_TEXTURE_2D);
				tglBindTexture(TGL_TEXTURE_2D, 0);
				bTextured = 0;
			}

			break;
		case kStd3DCommandTriangles:
			reserveQueue(pData, kBytesPerDrawCall + kBytesPerVertex * command.numIndices);
			tglBegin(TGL_TRIANGLES);

			for (uint i = 0; i < command.numIndices; i++) {
				const Std3DVertex &vertex = pList->pVertices[pList->pIndices[command.firstIndex + i]];

				tglColor4ub((vertex.color >> 16) & 0xFF, (vertex.color >> 8) & 0xFF, vertex.color & 0xFF, vertex.color >> 24);

				if (bTextured) {
					if (!_bWarnedTexCoord && (ABS(vertex.tu) >= tinyGlTexCoordLimit || ABS(vertex.tv) >= tinyGlTexCoordLimit)) {
						warning("Std3DTinyGlBackend::drawList(): texture coordinates beyond +-%.0f overflow in TinyGL", tinyGlTexCoordLimit);
						_bWarnedTexCoord = 1;
					}

					tglTexCoord2f(vertex.tu, vertex.tv);
				}

				tglVertex3f(vertex.sx + 0.5f, vertex.sy + 0.5f, vertex.sz - tinyGlZOffset);
			}

			tglEnd();
			pData->bPendingColor = 1;
			break;
		default:
			break;
		}
	}

	tglDisable(TGL_TEXTURE_2D);
	tglBindTexture(TGL_TEXTURE_2D, 0);
	tglDisable(TGL_BLEND);
	tglDisable(TGL_DEPTH_TEST);
	tglDepthMask(TGL_TRUE);
	return 1;
}

Std3DBackend *createTinyGlBackend() {
	return new Std3DTinyGlBackend();
}

} // End of namespace Grim

#endif
