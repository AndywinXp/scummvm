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

#include "common/endian.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/util.h"

#if defined(USE_OPENGL_SHADERS)
#include "graphics/opengl/context.h"
#endif
#include "graphics/renderer.h"

#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

enum {
	kPixelFlagIndexed8 = 0x20,
	kPixelFlagRgb = 0x40,
	kVideoMemoryTotal = 0x4000000
};

enum {
	kClipReject = 0,
	kClipEmpty = 1,
	kClipCopy = 2
};

struct SystemMode {
	int width;
	int height;
	float aspect;
	int bpp;
	int colorMode;
	int redBits;
	int greenBits;
	int blueBits;
	int redShift;
	int greenShift;
	int blueShift;
	int redLoss;
	int greenLoss;
	int blueLoss;
};

struct CopyRegion {
	int srcX;
	int srcY;
	int dstX;
	int dstY;
	int width;
	int rows;
};

static const byte dibSectionGuid[16] = {
	0x00, 0x9F, 0xA6, 0x92, 0xFA, 0x13, 0xD1, 0x11, 0x97, 0xC0, 0x00, 0xA0, 0x24, 0x29, 0x30, 0x05
};

static const StdDisplayModeDesc syntheticModes[1] = {
	{ 640, 480, 1280, kPixelFlagRgb, 16, 0xF800, 0x07E0, 0x001F }
};

static const SystemMode systemModes[10] = {
	{ 320, 200, 0.75f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 320, 240, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 400, 400, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 512, 384, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 640, 480, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1024, 768, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1280, 1024, 1.0f, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 320, 240, 1.0f, 16, 1, 5, 5, 5, 10, 5, 0, 3, 3, 3 },
	{ 640, 480, 1.0f, 16, 1, 5, 5, 5, 10, 5, 0, 3, 3, 3 },
	{ 640, 480, 1.0f, 16, 1, 5, 6, 5, 11, 5, 0, 3, 2, 3 }
};

static int log2Int(uint value) {
	int result = 0;

	while (value > 1) {
		value >>= 1;
		result++;
	}

	return result;
}

static int readMask(uint mask, int *pShift, int *pLoss, int *pBits) {
	if (!mask) {
		warning("StdDisplay::enumModesCallback(): empty color mask");
		return 0;
	}

	uint shifted = mask;
	int shift = 0;

	while ((shifted & 1) == 0) {
		shifted >>= 1;
		shift++;
	}

	*pShift = shift;
	*pLoss = log2Int(0xFF / (mask >> shift));

	int bits = 0;

	while ((shifted & 1) != 0) {
		shifted >>= 1;
		bits++;
	}

	*pBits = bits;
	return 1;
}

static int fillMode(StdDisplayMode *pMode, const StdDisplayModeDesc *pDesc, int bModeX) {
	memset(pMode, 0, sizeof(StdDisplayMode));

	pMode->bModeX = bModeX;
	pMode->tex.width = pDesc->width;
	pMode->tex.height = pDesc->height;
	pMode->tex.pitchInBytes = pDesc->pitch;

	if (pDesc->width == 320 && pDesc->height == 200)
		pMode->aspect = 0.75f;
	else
		pMode->aspect = 1.0f;

	StdColorFormat *pFormat = &pMode->tex.format;

	if (pDesc->pixelFlags & kPixelFlagIndexed8) {
		pFormat->colorMode = 0;
		pFormat->bpp = 8;
		pFormat->redBits = 0;
		pFormat->greenBits = 0;
		pFormat->blueBits = 0;
	} else if (pDesc->pixelFlags & kPixelFlagRgb) {
		pFormat->colorMode = 1;
		pFormat->bpp = pDesc->rgbBitCount;

		if (!readMask(pDesc->rBitMask, &pFormat->redShift, &pFormat->redLoss, &pFormat->redBits))
			return 0;

		if (!readMask(pDesc->gBitMask, &pFormat->greenShift, &pFormat->greenLoss, &pFormat->greenBits))
			return 0;

		if (!readMask(pDesc->bBitMask, &pFormat->blueShift, &pFormat->blueLoss, &pFormat->blueBits))
			return 0;
	} else {
		warning("StdDisplay::enumModesCallback(): unsupported pixel format");
		return 0;
	}

	switch (pFormat->bpp) {
	case 8:
		pMode->tex.pitchInPixels = pMode->tex.pitchInBytes;
		break;
	case 16:
		pMode->tex.pitchInPixels = (uint)pMode->tex.pitchInBytes >> 1;
		break;
	case 24:
		pMode->tex.pitchInPixels = pMode->tex.pitchInBytes / 3u;
		break;
	case 32:
		pMode->tex.pitchInPixels = (uint)pMode->tex.pitchInBytes >> 2;
		break;
	default:
		break;
	}

	pMode->tex.sizeInBytes = pMode->tex.width * pMode->tex.height * ((uint)pFormat->bpp >> 3);
	return 1;
}

static int compareDisplayModes(const void *pA, const void *pB) {
	const StdDisplayMode *pModeA = (const StdDisplayMode *)pA;
	const StdDisplayMode *pModeB = (const StdDisplayMode *)pB;

	if (pModeA->bModeX && !pModeB->bModeX)
		return pModeB->tex.width != pModeA->tex.width ? -1 : 1;

	if (!pModeA->bModeX && pModeB->bModeX)
		return pModeA->tex.width != pModeB->tex.width ? 1 : -1;

	if (pModeA->tex.format.bpp != pModeB->tex.format.bpp)
		return pModeA->tex.format.bpp - pModeB->tex.format.bpp;

	if (pModeA->tex.width != pModeB->tex.width)
		return pModeA->tex.width - pModeB->tex.width;

	return pModeA->tex.height - pModeB->tex.height;
}

#if defined(USE_OPENGL_SHADERS)

static void setupVideoBuffer(StdVBuffer *pBuffer, const StdDisplayMode *pMode, byte *pPalette) {
	uint bytesPerPixel = (uint)pMode->tex.format.bpp >> 3;

	pBuffer->type = 1;
	pBuffer->lockCount = 0;
	pBuffer->bVideoMemory = 0;
	pBuffer->tex = pMode->tex;
	pBuffer->tex.pitchInBytes = pMode->tex.width * bytesPerPixel;
	pBuffer->tex.pitchInPixels = pMode->tex.width;
	pBuffer->tex.sizeInBytes = pMode->tex.width * pMode->tex.height * bytesPerPixel;
	pBuffer->pPaletteRGB = pPalette;
	pBuffer->pPixels = nullptr;
	pBuffer->colorKey = 0;
	pBuffer->allocSize = 0;
	pBuffer->pSurface = nullptr;
	pBuffer->surfaceColorKey = -1;
}

static int clipBlitRect(StdRect *pSrcRect, int srcWidth, int srcHeight, int dstWidth, int dstHeight, int *pDstX, int *pDstY) {
	int srcX = pSrcRect->x;
	int srcY = pSrcRect->y;
	int width = pSrcRect->width;
	int height = pSrcRect->height;

	if (srcX < 0) {
		*pDstX -= srcX;
		width += srcX;
		srcX = 0;
	}

	if (srcY < 0) {
		*pDstY -= srcY;
		height += srcY;
		srcY = 0;
	}

	if (srcX + width > srcWidth)
		width = srcWidth - srcX;

	if (srcY + height > srcHeight)
		height = srcHeight - srcY;

	if (*pDstX < 0) {
		srcX -= *pDstX;
		width += *pDstX;
		*pDstX = 0;
	}

	if (*pDstY < 0) {
		srcY -= *pDstY;
		height += *pDstY;
		*pDstY = 0;
	}

	if (*pDstX + width > dstWidth)
		width = dstWidth - *pDstX;

	if (*pDstY + height > dstHeight)
		height = dstHeight - *pDstY;

	if (width <= 0 || height <= 0)
		return 0;

	pSrcRect->x = srcX;
	pSrcRect->y = srcY;
	pSrcRect->width = width;
	pSrcRect->height = height;
	return 1;
}

static int clipCopyRegion(const StdVBuffer *pDst, const StdVBuffer *pSrc, const StdRect *pSrcRect, int x, int y, CopyRegion *pRegion) {
	if (x >= pDst->tex.width)
		return kClipReject;

	int endX = x + pSrcRect->width;

	if (!endX)
		return kClipReject;

	uint firstRow = y;

	if (y >= pDst->tex.height || !(pSrcRect->height + y))
		return kClipReject;

	uint lastRow = pSrcRect->height + y - 1;
	uint lastColumn = endX - 1;
	int srcStartX = pSrcRect->x;
	int srcY = pSrcRect->y;

	if (y < 0) {
		srcY -= y;
		firstRow = 0;
	}

	if (lastRow >= (uint)pDst->tex.height)
		lastRow = pDst->tex.height - 1;

	if (x < 0) {
		srcStartX = pSrcRect->x - x;
		x = 0;
	}

	if (lastColumn >= (uint)pDst->tex.width)
		lastColumn = pDst->tex.width - 1;

	int copyWidth = lastColumn - x + 1;

	if (!copyWidth)
		return kClipEmpty;

	int rows = 0;

	if (firstRow <= lastRow)
		rows = lastRow - firstRow + 1;

	if (rows > 0 && (srcStartX < 0 || srcY < 0 || srcStartX + copyWidth > pSrc->tex.width || srcY + rows > pSrc->tex.height)) {
		warning("StdDisplay::copyBlt(): source rectangle outside the source buffer");
		return kClipReject;
	}

	pRegion->srcX = srcStartX;
	pRegion->srcY = srcY;
	pRegion->dstX = x;
	pRegion->dstY = firstRow;
	pRegion->width = copyWidth;
	pRegion->rows = rows;
	return kClipCopy;
}

static void copyRows(byte *pDstRow, int dstStride, const byte *pSrcRow, int srcStride, int width, int rows, int bpp, int bColorKey, int colorKey) {
	int bytesPerPixel = (uint)bpp >> 3;

	for (int row = 0; row < rows; row++) {
		if (!bColorKey) {
			memcpy(pDstRow, pSrcRow, width * bytesPerPixel);
		} else {
			for (int column = 0; column < width; column++) {
				if (bpp == 8) {
					if (pSrcRow[column] != (byte)colorKey)
						pDstRow[column] = pSrcRow[column];
				} else if (bpp == 16) {
					uint16 value = READ_LE_UINT16(pSrcRow + 2 * column);

					if (value != (uint16)colorKey)
						WRITE_LE_UINT16(pDstRow + 2 * column, value);
				} else {
					uint32 value = READ_LE_UINT32(pSrcRow + 4 * column);

					if (value != (uint32)colorKey)
						WRITE_LE_UINT32(pDstRow + 4 * column, value);
				}
			}
		}

		pSrcRow += srcStride;
		pDstRow += dstStride;
	}
}

static void copyPixelRows(StdVBuffer *pDst, const StdVBuffer *pSrc, const CopyRegion *pRegion, int bColorKey) {
	int bpp = pSrc->tex.format.bpp;

	if (bpp != 8 && bpp != 16 && bpp != 32)
		return;

	int bytesPerPixel = (uint)bpp >> 3;
	int srcStride = bytesPerPixel * pSrc->tex.pitchInPixels;
	int dstStride = bytesPerPixel * pDst->tex.pitchInPixels;
	const byte *pSrcRow = pSrc->pPixels + bytesPerPixel * (pSrc->tex.pitchInPixels * pRegion->srcY + pRegion->srcX);
	byte *pDstRow = pDst->pPixels + bytesPerPixel * (pDst->tex.pitchInPixels * pRegion->dstY + pRegion->dstX);

	copyRows(pDstRow, dstStride, pSrcRow, srcStride, pRegion->width, pRegion->rows, bpp, bColorKey, pSrc->colorKey);
}

static int intersectsGpuDirty(const Std3DSurface *pSurface, const StdRect *pRect) {
	if (!pSurface->gpuDirty)
		return 0;

	return pRect->x < pSurface->dirtyRight && pRect->x + pRect->width > pSurface->dirtyLeft &&
		pRect->y < pSurface->dirtyBottom && pRect->y + pRect->height > pSurface->dirtyTop;
}

static int shadowCoversRect(const Std3DSurface *pSurface, const StdRect *pRect) {
	return pSurface->shadowValid && !intersectsGpuDirty(pSurface, pRect);
}

static void copyRectToReference(Std3DSurface *pSurface, const byte *pShadow, const StdRect *pRect) {
	if (!pSurface->pReference)
		return;

	int pitch = pSurface->width * 2;

	for (int y = pRect->y; y < pRect->y + pRect->height; y++) {
		int offset = y * pitch + pRect->x * 2;

		memcpy(pSurface->pReference + offset, pShadow + offset, pRect->width * 2);
	}
}

static int findChangedRect(const Std3DSurface *pSurface, const byte *pShadow, StdRect *pRect) {
	int pitch = pSurface->width * 2;
	int top = -1;
	int bottom = -1;
	int left = pSurface->width;
	int right = -1;

	for (int y = 0; y < pSurface->height; y++) {
		const byte *pRow = pShadow + y * pitch;
		const byte *pRef = pSurface->pReference + y * pitch;

		if (!memcmp(pRow, pRef, pitch))
			continue;

		if (top < 0)
			top = y;

		bottom = y;

		int firstByte = 0;
		int lastByte = pitch - 1;

		while (pRow[firstByte] == pRef[firstByte])
			firstByte++;

		while (pRow[lastByte] == pRef[lastByte])
			lastByte--;

		left = MIN(left, firstByte >> 1);
		right = MAX(right, lastByte >> 1);
	}

	if (top < 0)
		return 0;

	pRect->x = left;
	pRect->y = top;
	pRect->width = right - left + 1;
	pRect->height = bottom - top + 1;
	return 1;
}

#endif

int StdDisplay::startup() {
	_bStartup = 1;
	return 1;
}

void StdDisplay::shutdown() {
	_bStartup = 0;
}

int StdDisplay::enumerateDevices() {
	StdDisplayDevice *pDevice = &_aDevices[0];

	memcpy(pDevice->guid, dibSectionGuid, sizeof(pDevice->guid));
	_bEnumerated = 1;
	_numDevices = 0;

	Common::strlcpy(pDevice->aName, "Window Display Device", sizeof(pDevice->aName));
	Common::strlcpy(pDevice->aDescription, "DIBsection windowed display", sizeof(pDevice->aDescription));
	pDevice->type = 0;
	pDevice->bHasGuid = 0;
	pDevice->b3D = 0;
	pDevice->bPrimary = 1;
	pDevice->bHasHardware = 0;
	pDevice->vidMemTotal = 2000000;
	pDevice->vidMemFree = 2000000;
	_numDevices = 1;

	if (Graphics::Renderer::getAvailableTypes() & Graphics::kRendererTypeOpenGLShaders)
		enumDevicesCallback(nullptr, "ScummVM OpenGL display", "ScummVM OpenGL");

	return 1;
}

int StdDisplay::enumDevicesCallback(const byte *pGuid, const char *pDescription, const char *pName) {
	if (_numDevices >= kStdDisplayMaxDevices)
		return 0;

	StdDisplayDevice *pDevice = &_aDevices[_numDevices];

	pDevice->type = 1;
	pDevice->bHasHardware = 1;

	if (pGuid) {
		memcpy(pDevice->guid, pGuid, sizeof(pDevice->guid));
		pDevice->bPrimary = 0;
	} else {
		memset(pDevice->guid, 0, sizeof(pDevice->guid));
		pDevice->bPrimary = 1;
	}

	Common::strlcpy(pDevice->aDescription, pDescription, sizeof(pDevice->aDescription));
	Common::strlcpy(pDevice->aName, pName, sizeof(pDevice->aName));
	pDevice->bHasGuid = pGuid ? 1 : 0;
	pDevice->b3D = 1;
	pDevice->vidMemTotal = kVideoMemoryTotal;
	pDevice->vidMemFree = kVideoMemoryTotal;
	_numDevices++;
	return 1;
}

void StdDisplay::restoreDisplayMode() {
	_bEnumerated = 0;
}

int StdDisplay::enumerateModes(uint deviceIndex) {
	if (_bDisplayOpen)
		return 0;

	if (deviceIndex >= _numDevices)
		return 0;

	_curDeviceIndex = deviceIndex;
	_pCurDevice = &_aDevices[deviceIndex];

	if (_pCurDevice->type == 1) {
		if (!createDirectDraw())
			return 0;
	} else if (_pCurDevice->type == 0) {
		for (int i = 0; i < ARRAYSIZE(systemModes); i++) {
			const SystemMode *pInfo = &systemModes[i];
			StdDisplayMode *pMode = &_aModes[i];
			int bytesPerPixel = pInfo->bpp >> 3;

			memset(pMode, 0, sizeof(StdDisplayMode));
			pMode->bModeX = 0;
			pMode->aspect = pInfo->aspect;
			pMode->tex.width = pInfo->width;
			pMode->tex.height = pInfo->height;
			pMode->tex.pitchInBytes = pInfo->width * bytesPerPixel;
			pMode->tex.pitchInPixels = pInfo->width;
			pMode->tex.sizeInBytes = pInfo->width * pInfo->height * bytesPerPixel;
			pMode->tex.format.colorMode = pInfo->colorMode;
			pMode->tex.format.bpp = pInfo->bpp;
			pMode->tex.format.redBits = pInfo->redBits;
			pMode->tex.format.greenBits = pInfo->greenBits;
			pMode->tex.format.blueBits = pInfo->blueBits;
			pMode->tex.format.redShift = pInfo->redShift;
			pMode->tex.format.greenShift = pInfo->greenShift;
			pMode->tex.format.blueShift = pInfo->blueShift;
			pMode->tex.format.redLoss = pInfo->redLoss;
			pMode->tex.format.greenLoss = pInfo->greenLoss;
			pMode->tex.format.blueLoss = pInfo->blueLoss;
		}

		_numModes = ARRAYSIZE(systemModes);
	}

	Grim::sort(_aModes, _numModes, sizeof(StdDisplayMode), compareDisplayModes);
	_bDisplayOpen = 1;
	return 1;
}

int StdDisplay::createDirectDraw() {
	if (!(Graphics::Renderer::getAvailableTypes() & Graphics::kRendererTypeOpenGLShaders)) {
		warning("StdDisplay::createDirectDraw(): OpenGL with shaders is not available");
		return 0;
	}

	_numModes = 0;

	for (int i = 0; i < ARRAYSIZE(syntheticModes); i++) {
		if (!enumModesCallback(&syntheticModes[i]))
			break;
	}

	for (int i = 0; i < ARRAYSIZE(syntheticModes); i++) {
		if (!enumModesCallback2(&syntheticModes[i]))
			break;
	}

	return 1;
}

int StdDisplay::enumModesCallback(const StdDisplayModeDesc *pDesc) {
	if (_numModes >= kStdDisplayMaxModes)
		return 0;

	if (pDesc->width == 320) {
		if (fillMode(&_aModes[_numModes], pDesc, 1))
			_numModes++;
	}

	return 1;
}

int StdDisplay::enumModesCallback2(const StdDisplayModeDesc *pDesc) {
	if (_numModes >= kStdDisplayMaxModes)
		return 0;

	if (fillMode(&_aModes[_numModes], pDesc, 0))
		_numModes++;

	return 1;
}

uint StdDisplay::findClosestDevice(const StdDisplayDeviceFlags *pWanted) {
	if (!_numDevices)
		return 0;

	int bestScore = 0;
	uint best = 0;

	for (uint i = 0; i < _numDevices; i++) {
		const StdDisplayDevice *pDevice = &_aDevices[i];
		int score = 0;

		if (pDevice->bHasHardware == pWanted->bHasHardware) {
			score = 1;

			if (pDevice->type == pWanted->type) {
				score = 2;

				if (pDevice->bPrimary == pWanted->bPrimary) {
					score = 3;

					if (pDevice->b3D == pWanted->b3D) {
						score = 4;

						if (pDevice->bHasGuid == pWanted->bHasGuid)
							return i;
					}
				}
			}
		}

		if (score > bestScore) {
			best = i;
			bestScore = score;
		}
	}

	return best;
}

uint StdDisplay::findClosestMode(const StdDisplayMode *pWanted, const StdDisplayMode *aModes, uint numModes) {
	if (!_numModes)
		return 0;

	int bestScore = 0;
	uint best = 0;
	int colorMode = pWanted->tex.format.colorMode;

	for (uint i = 0; i < numModes; i++) {
		const StdDisplayMode *pMode = &aModes[i];
		int score = 0;

		if (pMode->tex.format.colorMode == colorMode) {
			score = 1;

			if (pMode->tex.format.bpp == pWanted->tex.format.bpp) {
				score = 2;

				if (pMode->tex.width == pWanted->tex.width) {
					score = 3;

					if (pMode->tex.height == pWanted->tex.height) {
						score = 4;

						if (colorMode != 1 || (pMode->tex.format.redBits == pWanted->tex.format.redBits &&
								pMode->tex.format.greenBits == pWanted->tex.format.greenBits &&
								pMode->tex.format.blueBits == pWanted->tex.format.blueBits))
							return i;
					}
				}
			}
		}

		if (score > bestScore) {
			best = i;
			bestScore = score;
		}
	}

	return best;
}

int StdDisplay::openDisplay(uint modeIndex, byte *pPalette, int bFullscreen) {
	if (modeIndex >= _numModes)
		return 0;

	if (!_pCurDevice) {
		warning("StdDisplay::openDisplay(): no device selected");
		return 0;
	}

	if (_bSurfacesCreated)
		releaseSurfaces();

	_bFullscreen = bFullscreen;
	_curModeIndex = modeIndex;
	_numDrawBuffers = 1;
	_pDisplayMode = &_aModes[modeIndex];

	if (_pCurDevice->type == 1) {
		if (!initDisplay(&_aModes[modeIndex], pPalette))
			return 0;
	} else {
		// TODO: DIB display device
		warning("StdDisplay::openDisplay(): only the video memory device is supported");
		return 0;
	}

	vBufferFill(_pBackBuffer, 0, nullptr);
	update();

	if (bFullscreen)
		vBufferFill(_pBackBuffer, 0, nullptr);

	_bSurfacesCreated = 1;
	return 1;
}

void StdDisplay::closeDisplay() {
	if (!_bDisplayOpen)
		return;

	if (_bSurfacesCreated)
		releaseSurfaces();

	_bDisplayOpen = 0;
}

#if defined(USE_OPENGL_SHADERS)

int StdDisplay::initDisplay(StdDisplayMode *pMode, byte *pPalette) {
	if (!_pStd3D || !_pMainGfxBuffer || !_pBackBuffer) {
		warning("StdDisplay::initDisplay(): the Std3D module and the display buffers are not set");
		return 0;
	}

	if (pMode->tex.format.bpp != 16 || pMode->tex.format.colorMode == 0) {
		warning("StdDisplay::initDisplay(): only 16 bpp RGB display modes are supported");
		return 0;
	}

	initGraphics3d(pMode->tex.width, pMode->tex.height);

	if (OpenGLContext.type == OpenGL::kContextNone || !OpenGLContext.enginesShadersSupported || !OpenGLContext.framebufferObjectSupported) {
		warning("StdDisplay::initDisplay(): the OpenGL context with shaders is not available");
		return 0;
	}

	setupVideoBuffer(_pMainGfxBuffer, pMode, pPalette);
	_pMainGfxBuffer->pSurface = _pStd3D->createSurface(pMode->tex.width, pMode->tex.height, 0);

	if (!_pMainGfxBuffer->pSurface)
		return 0;

	_pMainGfxBuffer->bVideoMemory = 1;

	if (_bFullscreen) {
		_numDrawBuffers = 2;
		setupVideoBuffer(_pBackBuffer, pMode, pPalette);
		_pBackBuffer->pSurface = _pStd3D->createSurface(pMode->tex.width, pMode->tex.height, 0);

		if (!_pBackBuffer->pSurface) {
			_pStd3D->destroySurface(_pMainGfxBuffer->pSurface);
			return 0;
		}

		_pBackBuffer->bVideoMemory = 1;
	} else {
		_numDrawBuffers = 1;
		*_pBackBuffer = *_pMainGfxBuffer;
	}

	_pStd3D->setDisplayMode(&pMode->tex);
	_pStd3D->setRenderTarget(_pBackBuffer->pSurface);
	return 1;
}

#endif

void StdDisplay::releaseSurfaces() {
	if (_bSurfacesCreated) {
		if (_pCurDevice && _pCurDevice->type == 1)
			freeBuffers();
		// TODO: DIB display device
	}

	_bSurfacesCreated = 0;
}

#if defined(USE_OPENGL_SHADERS)

void StdDisplay::freeBuffers() {
	if (!_pMainGfxBuffer || !_pBackBuffer)
		return;

	if (_bFullscreen) {
		vBufferFreeEntry(_pBackBuffer);
	} else {
		_pBackBuffer->pSurface = nullptr;
		_pBackBuffer->pPixels = nullptr;
		_pBackBuffer->allocSize = 0;
	}

	vBufferFreeEntry(_pMainGfxBuffer);

	if (_pStd3D)
		vBufferFreeEntry(_pStd3D->getZBuffer());
}

#endif

int StdDisplay::prepareWindowForDraw() {
	if (!_bDisplayOpen || !_bSurfacesCreated || !_pCurDevice)
		return 0;

	return _pCurDevice->type - 1;
}

#if defined(USE_OPENGL_SHADERS)

void StdDisplay::prepareOutputForDraw() {
	if (_bDisplayOpen && _bSurfacesCreated && _pCurDevice && _pCurDevice->type == 1 && _pStd3D)
		_pStd3D->purgeTextureCache();
}

int StdDisplay::update() {
	if (!_pCurDevice || _pCurDevice->type != 1) {
		// TODO: DIB display device
		return 0;
	}

	if (!_pStd3D || !_pMainGfxBuffer || !_pBackBuffer || !_pMainGfxBuffer->pSurface) {
		warning("StdDisplay::update(): the display is not open");
		return 0;
	}

	if (_bFullscreen) {
		if (_pBackBuffer->lockCount || !_pBackBuffer->pSurface)
			return 0;

		if (!_pStd3D->blit(_pMainGfxBuffer->pSurface, _pBackBuffer->pSurface, nullptr, 0, 0, 0, 0))
			return 0;
	}

	if (!_pStd3D->presentSurface(_pMainGfxBuffer->pSurface))
		return 0;

	g_system->updateScreen();
	return 1;
}

int StdDisplay::isZBuffer(StdVBuffer *pVBuffer) {
	return _pStd3D && pVBuffer == _pStd3D->getZBuffer();
}

void StdDisplay::presentIfPrimary(StdVBuffer *pVBuffer) {
	if (!_bFullscreen || !_pStd3D || !_pMainGfxBuffer || !pVBuffer->pSurface)
		return;

	if (pVBuffer->pSurface != _pMainGfxBuffer->pSurface)
		return;

	_pStd3D->presentSurface(pVBuffer->pSurface);
	g_system->updateScreen();
}

byte *StdDisplay::surfaceLock(StdVBuffer *pVBuffer) {
	if (!_pStd3D) {
		warning("StdDisplay::surfaceLock(): the Std3D module is not set");
		return nullptr;
	}

	if (pVBuffer->lockCount)
		return pVBuffer->pPixels;

	if (isZBuffer(pVBuffer)) {
		int zPitch = pVBuffer->tex.width * ((uint)pVBuffer->tex.format.bpp >> 3);
		int zSize = zPitch * pVBuffer->tex.height;

		if (!pVBuffer->pPixels) {
			pVBuffer->pPixels = new byte[zSize];
			pVBuffer->allocSize = zSize;
		}

		pVBuffer->tex.pitchInBytes = zPitch;
		pVBuffer->tex.pitchInPixels = pVBuffer->tex.width;

		warning("StdDisplay::surfaceLock(): the Z buffer can not be read back, it is locked as empty");
		memset(pVBuffer->pPixels, 0, zSize);
		return pVBuffer->pPixels;
	}

	if (!pVBuffer->pSurface) {
		warning("StdDisplay::surfaceLock(): the buffer has no surface");
		return nullptr;
	}

	if (!syncShadow(pVBuffer))
		return nullptr;

	return pVBuffer->pPixels;
}

int StdDisplay::syncShadow(StdVBuffer *pVBuffer) {
	Std3DSurface *pSurface = pVBuffer->pSurface;

	if (!_pStd3D || !pSurface)
		return 0;

	if (pVBuffer->tex.format.bpp != 16) {
		warning("StdDisplay::syncShadow(): only 16 bpp video memory buffers are supported");
		return 0;
	}

	int pitch = pVBuffer->tex.width * 2;
	int size = pitch * pVBuffer->tex.height;

	if (!pVBuffer->pPixels) {
		pVBuffer->pPixels = new byte[size];
		pVBuffer->allocSize = size;
		pSurface->shadowValid = 0;
	}

	if (!pSurface->pReference)
		pSurface->pReference = new byte[size];

	pVBuffer->tex.pitchInBytes = pitch;
	pVBuffer->tex.pitchInPixels = pVBuffer->tex.width;

	if (!pSurface->shadowValid) {
		if (!_pStd3D->readPixels(pSurface, nullptr, pVBuffer->pPixels, pitch))
			return 0;

		memcpy(pSurface->pReference, pVBuffer->pPixels, size);
		pSurface->shadowValid = 1;
		pSurface->gpuDirty = 0;
		return 1;
	}

	if (pSurface->gpuDirty) {
		StdRect rect;

		rect.x = pSurface->dirtyLeft;
		rect.y = pSurface->dirtyTop;
		rect.width = pSurface->dirtyRight - pSurface->dirtyLeft;
		rect.height = pSurface->dirtyBottom - pSurface->dirtyTop;

		if (!_pStd3D->readPixels(pSurface, &rect, pVBuffer->pPixels + rect.y * pitch + rect.x * 2, pitch))
			return 0;

		copyRectToReference(pSurface, pVBuffer->pPixels, &rect);
		pSurface->gpuDirty = 0;
	}

	return 1;
}

int StdDisplay::surfaceUnlock(StdVBuffer *pVBuffer, int bUpload) {
	if (pVBuffer->lockCount > 1 || !bUpload || isZBuffer(pVBuffer))
		return 1;

	Std3DSurface *pSurface = pVBuffer->pSurface;

	if (!_pStd3D || !pSurface || !pVBuffer->pPixels || !pSurface->pReference || !pSurface->shadowValid) {
		warning("StdDisplay::surfaceUnlock(): the buffer has no surface");
		return 0;
	}

	StdRect changed;

	if (!findChangedRect(pSurface, pVBuffer->pPixels, &changed))
		return 1;

	int pitch = pSurface->width * 2;

	if (!_pStd3D->uploadPixels(pSurface, &changed, pVBuffer->pPixels + changed.y * pitch + changed.x * 2, pitch))
		return 0;

	copyRectToReference(pSurface, pVBuffer->pPixels, &changed);
	presentIfPrimary(pVBuffer);
	return 1;
}

void StdDisplay::unlockForRead(StdVBuffer *pVBuffer) {
	if (pVBuffer->type != 1) {
		vBufferUnlock(pVBuffer);
		return;
	}

	if (pVBuffer->lockCount) {
		surfaceUnlock(pVBuffer, 0);
		pVBuffer->lockCount--;
	}
}

int StdDisplay::surfaceBlt(StdVBuffer *pVBuffer, int color, const StdRect *pRect) {
	if (pRect && (pRect->width == 0 || pRect->height == 0))
		return 0;

	if (pVBuffer->lockCount)
		return 0;

	if (!_pStd3D || !pVBuffer->pSurface) {
		warning("StdDisplay::surfaceBlt(): the buffer has no surface, the Z buffer can not be filled");
		return 0;
	}

	if (!_pStd3D->fillRect(pVBuffer->pSurface, pRect, (uint16)color))
		return 0;

	presentIfPrimary(pVBuffer);
	return 1;
}

int StdDisplay::copyToZBuffer(StdVBuffer *pSrc, int x, int y, const StdRect *pSrcRect) {
	Std3DSurface *pTarget = _pStd3D->getRenderTarget();

	if (!pTarget || !pSrc->pSurface) {
		warning("StdDisplay::copyToZBuffer(): no render target or no source surface");
		return 0;
	}

	StdRect srcRect = *pSrcRect;
	int dstX = x;
	int dstY = y;

	if (!clipBlitRect(&srcRect, pSrc->tex.width, pSrc->tex.height, pTarget->width, pTarget->height, &dstX, &dstY))
		return 1;

	byte *pData = new byte[srcRect.width * srcRect.height * 2];
	int result = 1;

	if (shadowCoversRect(pSrc->pSurface, &srcRect) && pSrc->pPixels) {
		int srcPitch = pSrc->tex.width * 2;

		for (int row = 0; row < srcRect.height; row++)
			memcpy(pData + row * srcRect.width * 2, pSrc->pPixels + (srcRect.y + row) * srcPitch + srcRect.x * 2, srcRect.width * 2);
	} else {
		result = _pStd3D->readPixels(pSrc->pSurface, &srcRect, pData, srcRect.width * 2);
	}

	if (result) {
		StdRect dstRect;

		dstRect.x = dstX;
		dstRect.y = dstY;
		dstRect.width = srcRect.width;
		dstRect.height = srcRect.height;
		result = _pStd3D->uploadDepth(pTarget, &dstRect, pData, srcRect.width * 2);
	}

	delete[] pData;
	return result;
}

int StdDisplay::copyBltB(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey) {
	if (pDst->lockCount || pSrc->lockCount)
		return 0;

	if (pDst->type != 1 || pSrc->type != 1 || !_pStd3D || !pSrc->pSurface) {
		warning("StdDisplay::copyBltB(): both buffers must be video memory buffers");
		return 0;
	}

	StdRect srcRect;

	if (pRect) {
		if (!pRect->width || !pRect->height)
			return 0;

		srcRect = *pRect;
	} else {
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = pSrc->tex.width;
		srcRect.height = pSrc->tex.height;
	}

	if (isZBuffer(pDst))
		return copyToZBuffer(pSrc, x, y, &srcRect);

	if (!pDst->pSurface) {
		warning("StdDisplay::copyBltB(): the destination has no surface");
		return 0;
	}

	int bKey = (bColorKey & 1) != 0;

	if (bKey && pSrc->surfaceColorKey < 0) {
		warning("StdDisplay::copyBltB(): the source has no color key");
		return 0;
	}

	int result = _pStd3D->blit(pDst->pSurface, pSrc->pSurface, &srcRect, x, y, bKey, (uint16)pSrc->surfaceColorKey);

	if (result)
		presentIfPrimary(pDst);

	return result;
}

int StdDisplay::copyBltC(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey) {
	if (pDst->lockCount)
		return 0;

	if (pDst->type != 1 || pSrc->type != 0 || !pSrc->pPixels || !_pStd3D) {
		warning("StdDisplay::copyBltC(): the destination must be a video memory buffer and the source a system memory buffer");
		return 0;
	}

	StdRect srcRect;

	if (pRect) {
		if (!pRect->width || !pRect->height)
			return 0;

		srcRect = *pRect;
	} else {
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = pSrc->tex.width;
		srcRect.height = pSrc->tex.height;
	}

	CopyRegion region;
	int clipResult = clipCopyRegion(pDst, pSrc, &srcRect, x, y, &region);

	if (clipResult == kClipReject)
		return 0;

	if (clipResult == kClipEmpty)
		return 1;

	if (isZBuffer(pDst)) {
		if (pSrc->tex.format.bpp != 16) {
			warning("StdDisplay::copyBltC(): the Z buffer needs a 16 bpp source");
			return 0;
		}

		if (region.rows > 0) {
			StdRect dstRect;

			dstRect.x = region.dstX;
			dstRect.y = region.dstY;
			dstRect.width = region.width;
			dstRect.height = region.rows;

			const byte *pSrcRows = pSrc->pPixels + 2 * (pSrc->tex.pitchInPixels * region.srcY + region.srcX);

			_pStd3D->uploadDepth(_pStd3D->getRenderTarget(), &dstRect, pSrcRows, 2 * pSrc->tex.pitchInPixels);
		}

		return 1;
	}

	Std3DSurface *pSurface = pDst->pSurface;

	if (!pSurface || pSrc->tex.format.bpp != 16 || pDst->tex.format.bpp != 16) {
		warning("StdDisplay::copyBltC(): only 16 bpp video memory buffers with a surface are supported");
		return 0;
	}

	if (region.rows <= 0)
		return 1;

	StdRect dstRect;

	dstRect.x = region.dstX;
	dstRect.y = region.dstY;
	dstRect.width = region.width;
	dstRect.height = region.rows;

	int bKey = (bColorKey & 1) != 0;
	int srcPitch = 2 * pSrc->tex.pitchInPixels;
	const byte *pSrcRows = pSrc->pPixels + 2 * (pSrc->tex.pitchInPixels * region.srcY + region.srcX);
	int result = 0;

	if (shadowCoversRect(pSurface, &dstRect) && pDst->pPixels) {
		int dstPitch = pSurface->width * 2;
		byte *pDstRows = pDst->pPixels + region.dstY * dstPitch + region.dstX * 2;

		copyRows(pDstRows, dstPitch, pSrcRows, srcPitch, region.width, region.rows, 16, bKey, pSrc->colorKey);
		result = _pStd3D->uploadPixels(pSurface, &dstRect, pDstRows, dstPitch);
		copyRectToReference(pSurface, pDst->pPixels, &dstRect);
	} else if (!bKey) {
		result = _pStd3D->uploadPixels(pSurface, &dstRect, pSrcRows, srcPitch);
	} else {
		int tempPitch = region.width * 2;
		byte *pTemp = new byte[tempPitch * region.rows];

		result = _pStd3D->readPixels(pSurface, &dstRect, pTemp, tempPitch);

		if (result) {
			copyRows(pTemp, tempPitch, pSrcRows, srcPitch, region.width, region.rows, 16, 1, pSrc->colorKey);
			result = _pStd3D->uploadPixels(pSurface, &dstRect, pTemp, tempPitch);
		}

		delete[] pTemp;
	}

	if (!result)
		return 0;

	presentIfPrimary(pDst);
	return 1;
}

int StdDisplay::copyBltD(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey) {
	if (pSrc->lockCount)
		return 0;

	if (pDst->type != 0 || pSrc->type != 1 || !pDst->pPixels || !_pStd3D) {
		warning("StdDisplay::copyBltD(): the source must be a video memory buffer and the destination a system memory buffer");
		return 0;
	}

	StdRect srcRect;

	if (pRect) {
		if (!pRect->width || !pRect->height)
			return 0;

		srcRect = *pRect;
	} else {
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.width = pSrc->tex.width;
		srcRect.height = pSrc->tex.height;
	}

	CopyRegion region;
	int clipResult = clipCopyRegion(pDst, pSrc, &srcRect, x, y, &region);

	if (clipResult == kClipReject)
		return 0;

	if (clipResult == kClipEmpty)
		return 1;

	if (isZBuffer(pSrc)) {
		if (!vBufferLock(pSrc)) {
			warning("StdDisplay::copyBltD(): could not lock the source");
			return 0;
		}

		copyPixelRows(pDst, pSrc, &region, (bColorKey & 1) != 0);
		unlockForRead(pSrc);
		return 1;
	}

	if (!pSrc->pSurface) {
		warning("StdDisplay::copyBltD(): the source has no surface");
		return 0;
	}

	if (region.rows <= 0)
		return 1;

	StdRect srcArea;

	srcArea.x = region.srcX;
	srcArea.y = region.srcY;
	srcArea.width = region.width;
	srcArea.height = region.rows;

	if (!shadowCoversRect(pSrc->pSurface, &srcArea) && !syncShadow(pSrc)) {
		warning("StdDisplay::copyBltD(): could not read the source");
		return 0;
	}

	copyPixelRows(pDst, pSrc, &region, (bColorKey & 1) != 0);
	return 1;
}

#else

int StdDisplay::initDisplay(StdDisplayMode *pMode, byte *pPalette) {
	return 0;
}

void StdDisplay::freeBuffers() {
}

int StdDisplay::update() {
	return 0;
}

#endif

} // End of namespace Grim
