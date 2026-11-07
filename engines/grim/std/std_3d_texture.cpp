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

#if defined(USE_OPENGL_SHADERS)

#include "common/endian.h"
#include "common/textconsole.h"

#include "graphics/opengl/context.h"
#include "graphics/opengl/shader.h"

#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

static byte expandChannel(uint value, int bits) {
	if (bits <= 0 || bits > 8)
		return 0;

	uint result = value;
	int filled = bits;

	while (filled < 8) {
		result = (result << bits) | value;
		filled += bits;
	}

	return (byte)(result >> (filled - 8));
}

static void texelToRgba(uint texel, const StdColorFormat *pFormat, byte *pOut) {
	uint red = (texel >> pFormat->redShift) & ((1u << pFormat->redBits) - 1);
	uint green = (texel >> pFormat->greenShift) & ((1u << pFormat->greenBits) - 1);
	uint blue = (texel >> pFormat->blueShift) & ((1u << pFormat->blueBits) - 1);

	pOut[0] = expandChannel(red, pFormat->redBits);
	pOut[1] = expandChannel(green, pFormat->greenBits);
	pOut[2] = expandChannel(blue, pFormat->blueBits);

	if (pFormat->alphaBits > 0) {
		uint alpha = (texel >> pFormat->alphaShift) & ((1u << pFormat->alphaBits) - 1);
		pOut[3] = expandChannel(alpha, pFormat->alphaBits);
	} else {
		pOut[3] = 255;
	}
}

static int isValidFormat(const StdColorFormat *pFormat) {
	return pFormat->bpp == 16 && pFormat->redBits > 0 && pFormat->redBits <= 8 && pFormat->greenBits > 0 && pFormat->greenBits <= 8 &&
		pFormat->blueBits > 0 && pFormat->blueBits <= 8 && pFormat->alphaBits >= 0 && pFormat->alphaBits <= 8 &&
		pFormat->redShift >= 0 && pFormat->redShift < 16 && pFormat->greenShift >= 0 && pFormat->greenShift < 16 &&
		pFormat->blueShift >= 0 && pFormat->blueShift < 16 && pFormat->alphaShift >= 0 && pFormat->alphaShift < 16;
}

int Std3D::setCurrentPalette(const byte *pPalette, byte alpha) {
	if (!_pCurDevice || !_pTexFormatOpaque) {
		warning("Std3D::setCurrentPalette(): device not started");
		return 0;
	}

	memcpy(_aPalette, pPalette, sizeof(_aPalette));

	if (_pTexFormatOpaque->format.colorMode == 1 && _pTexFormatOpaque->format.bpp == 16)
		_pStdBitmap->buildPaletteOpaque(pPalette, _aOpaquePaletteTable, &_pTexFormatOpaque->format);

	if (_pCurDevice->bAlphaTexture) {
		if (_pTexFormatAlpha->format.colorMode == 2 && _pTexFormatAlpha->format.bpp == 16)
			_pStdBitmap->buildPaletteAlpha(pPalette, _aAlphaPaletteTable, &_pTexFormatAlpha->format);

		if (!_pCurDevice->bAlphaBlend && _pTexFormatColorKey->format.colorMode == 2 && _pTexFormatColorKey->format.bpp == 16)
			_pStdBitmap->buildPaletteColorKey(pPalette, _aColorKeyPaletteTable, &_pTexFormatColorKey->format, alpha);
	}

	return 1;
}

void Std3D::getValidDimensions(uint width, uint height, uint *pWidth, uint *pHeight) {
	int bSquareTexturesOnly = 0;

	if (_pCurDevice) {
		bSquareTexturesOnly = _pCurDevice->bSquareTexturesOnly;
	} else {
		warning("Std3D::getValidDimensions(): device not started");
	}

	uint w = width;

	if (width) {
		if (width > _maxTexWidth)
			w = _maxTexWidth;
	} else {
		w = 1;
	}

	uint h = height;

	if (height) {
		if (height > _maxTexWidth)
			h = _maxTexWidth;
	} else {
		h = 1;
	}

	if (w >= _minTexWidth && h >= _minTexHeight && (!bSquareTexturesOnly || w == h)) {
		*pWidth = w;
		*pHeight = h;
		return;
	}

	if (!bSquareTexturesOnly || w == h) {
		if (_minTexWidth > w)
			w = _minTexWidth;

		if (_minTexHeight > h)
			h = _minTexHeight;

		*pWidth = w;
		*pHeight = h;
		return;
	}

	if (w <= h)
		w = h;

	*pWidth = w;
	*pHeight = w;
}

void Std3D::releaseTexture(RdTexCacheEntry *pEntry) {
	if (pEntry->hTexture && _bGlReady && OpenGLContext.type != OpenGL::kContextNone) {
		GLuint texture = pEntry->hTexture;
		glDeleteTextures(1, &texture);
	}

	pEntry->hTexture = 0;
}

void Std3D::purgeTextureCache() {
	while (_pCacheHead) {
		RdTexCacheEntry *pEntry = _pCacheHead;

		releaseTexture(pEntry);
		pEntry->bInCache = 0;
		pEntry->lastFrame = 0;
		cacheListRemove(pEntry);
	}
}

int Std3D::addToTextureCache(StdVBuffer *pSrc, RdTexCacheEntry *pEntry, int bAlpha, int bColorKey) {
	if (!_pCurDevice || !_pTexFormatOpaque || !ensureGl()) {
		warning("Std3D::addToTextureCache(): device not started");
		return 0;
	}

	if (!pSrc || !pEntry || pSrc->tex.width <= 0 || pSrc->tex.height <= 0 || !pSrc->pPixels) {
		warning("Std3D::addToTextureCache(): invalid source");
		return 0;
	}

	StdVBuffer *pBuf = pSrc;
	StdVBuffer *pDst = nullptr;
	uint width = pSrc->tex.width;

	if (width > 0x100)
		width = 256;

	uint srcWidth = width;
	uint height = pSrc->tex.height;

	if (height > 0x100)
		height = 256;

	uint srcHeight = height;

	if (width < _minTexWidth || height < _minTexHeight || (_pCurDevice->bSquareTexturesOnly && width != height)) {
		StdVBufferTexFmt format = pSrc->tex;

		if (!_pCurDevice->bSquareTexturesOnly || width == height) {
			if (_minTexWidth > width)
				width = _minTexWidth;

			if (_minTexHeight > height)
				height = _minTexHeight;
		} else {
			if (width <= height)
				width = height;

			height = width;
		}

		int tilesX = (int)(float)((double)width / (double)srcWidth + 0.5);
		int tilesY = (int)(float)((double)height / (double)srcHeight + 0.5);

		format.width *= tilesX;
		format.height *= tilesY;
		pDst = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);

		if (!pDst) {
			warning("Std3D::addToTextureCache(): could not create the tiled texture");
			pEntry->hTexture = 0;
			pEntry->bInCache = 0;
			pEntry->lastFrame = 0;
			return 0;
		}

		int y = 0;

		for (int row = 0; row < tilesY; row++) {
			int tileX = 0;

			for (int column = 0; column < tilesX; column++) {
				_pStdDisplay->vBufferCopy(pDst, pBuf, tileX, y, nullptr, 0);
				tileX += srcWidth;
			}

			y += srcHeight;
		}

		pBuf = pDst;
		srcWidth = format.width;
		srcHeight = format.height;
		height = format.height;
		width = format.width;
	}

	uint size = width * height;
	const Std3DTexFormat *pTexFormat;

	if (bAlpha && _pCurDevice->bAlphaTexture) {
		pEntry->bColorKey = 1;

		if (bColorKey)
			pTexFormat = _pTexFormatColorKey;
		else
			pTexFormat = _pTexFormatAlpha;
	} else if (bColorKey) {
		pEntry->bColorKey = 1;
		pTexFormat = _pTexFormatColorKey;
	} else {
		pEntry->bColorKey = 0;
		pTexFormat = _pTexFormatOpaque;
	}

	if (!pTexFormat || !isValidFormat(&pTexFormat->format) || (int)width > OpenGLContext.maxTextureSize || (int)height > OpenGLContext.maxTextureSize) {
		warning("Std3D::addToTextureCache(): invalid texture format or size");

		if (pDst)
			_pStdDisplay->vBufferFree(pDst);

		pEntry->hTexture = 0;
		pEntry->bInCache = 0;
		pEntry->lastFrame = 0;
		return 0;
	}

	byte *pRgba = new byte[width * height * 4];
	memset(pRgba, 0, width * height * 4);

	const StdColorFormat *pTexelFormat = &pTexFormat->format;
	int colorMode = pBuf->tex.format.colorMode;
	uint16 colorKeyValue = 0;
	int bUseColorKey = 0;

	if (colorMode == 0) {
		_pStdDisplay->vBufferLock(pBuf);

		const uint16 *pTable;

		if (bAlpha && _pCurDevice->bAlphaTexture) {
			if (bColorKey)
				pTable = _aColorKeyPaletteTable;
			else
				pTable = _aAlphaPaletteTable;
		} else if (bColorKey) {
			pTable = _aColorKeyPaletteTable;
		} else {
			pTable = _aOpaquePaletteTable;
		}

		for (uint i = 0; i < srcHeight; i++) {
			const byte *pRow = &pBuf->pPixels[pBuf->tex.pitchInBytes * i];

			for (uint x = 0; x < srcWidth; x++)
				texelToRgba(pTable[pRow[x]], pTexelFormat, &pRgba[(i * width + x) * 4]);
		}

		_pStdDisplay->vBufferUnlock(pBuf);
	} else if (colorMode > 0 && colorMode <= 2) {
		_pStdDisplay->vBufferLock(pBuf);

		for (uint i = 0; i < srcHeight; i++) {
			const byte *pRow = &pBuf->pPixels[pBuf->tex.pitchInBytes * i];

			for (uint x = 0; x < srcWidth; x++)
				texelToRgba(READ_LE_UINT16(pRow + 2 * x), pTexelFormat, &pRgba[(i * width + x) * 4]);
		}

		_pStdDisplay->vBufferUnlock(pBuf);
	} else {
		warning("Std3D::addToTextureCache(): unsupported source color mode");
	}

	if (bAlpha && !_pCurDevice->bAlphaTexture) {
		if (colorMode == 0) {
			colorKeyValue = _aOpaquePaletteTable[0];
			bUseColorKey = 1;
		} else if (colorMode == 1) {
			colorKeyValue = (uint16)pBuf->colorKey;
			bUseColorKey = 1;
		}

		if (bUseColorKey) {
			for (uint i = 0; i < srcHeight; i++) {
				const byte *pRow = &pBuf->pPixels[pBuf->tex.pitchInBytes * i];

				for (uint x = 0; x < srcWidth; x++) {
					uint16 texel = colorMode == 0 ? _aOpaquePaletteTable[pRow[x]] : READ_LE_UINT16(pRow + 2 * x);

					if (texel == colorKeyValue)
						pRgba[(i * width + x) * 4 + 3] = 0;
				}
			}
		}
	}

	if (size > _pCurDevice->texMemFree) {
		evictTextures(size);

		if (size > _pCurDevice->texMemFree) {
			delete[] pRgba;

			if (pDst)
				_pStdDisplay->vBufferFree(pDst);

			pEntry->hTexture = 0;
			pEntry->bInCache = 0;
			pEntry->lastFrame = 0;
			return 0;
		}
	}

	GLuint texture = 0;

	glGenTextures(1, &texture);

	if (!texture) {
		delete[] pRgba;

		if (pDst)
			_pStdDisplay->vBufferFree(pDst);

		pEntry->hTexture = 0;
		pEntry->bInCache = 0;
		pEntry->lastFrame = 0;
		return 0;
	}

	int bPowerOfTwo = (width & (width - 1)) == 0 && (height & (height - 1)) == 0;
	GLint wrapMode = GL_REPEAT;

	if (!bPowerOfTwo && OpenGLContext.type == OpenGL::kContextGLES2)
		wrapMode = GL_CLAMP_TO_EDGE;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _bTexMinLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _bTexMagLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pRgba);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete[] pRgba;

	if (pDst)
		_pStdDisplay->vBufferFree(pDst);

	pEntry->hTexture = texture;
	pEntry->width = width;
	pEntry->height = srcHeight;
	pEntry->size = size;
	pEntry->bInCache = 1;
	pEntry->lastFrame = _frameCounter;
	cacheListAppend(pEntry);
	return 1;
}

void Std3D::flushTextureCache() {
	RdTexCacheEntry *pEntry = _pCacheHead;

	while (pEntry) {
		RdTexCacheEntry *pNext = pEntry->pNext;

		releaseTexture(pEntry);
		pEntry->bInCache = 0;
		pEntry->lastFrame = 0;
		pEntry->pNext = nullptr;
		pEntry->pPrev = nullptr;
		pEntry = pNext;
	}

	_pCacheHead = nullptr;
	_pCacheTail = nullptr;
	_numCachedTextures = 0;

	if (_pCurDevice)
		_pCurDevice->texMemFree = _pCurDevice->texMemTotal;
	else
		warning("Std3D::flushTextureCache(): device not started");

	_frameCounter = 1;
}

void Std3D::cacheListAppend(RdTexCacheEntry *pEntry) {
	if (_pCacheHead) {
		_pCacheTail->pNext = pEntry;
		pEntry->pPrev = _pCacheTail;
		pEntry->pNext = nullptr;
		_pCacheTail = pEntry;
	} else {
		_pCacheTail = pEntry;
		_pCacheHead = pEntry;
		pEntry->pPrev = nullptr;
		pEntry->pNext = nullptr;
	}

	_numCachedTextures++;

	if (_pCurDevice)
		_pCurDevice->texMemFree -= pEntry->size;
	else
		warning("Std3D::cacheListAppend(): device not started");
}

void Std3D::cacheListRemove(RdTexCacheEntry *pEntry) {
	if (pEntry == _pCacheHead) {
		_pCacheHead = pEntry->pNext;

		if (_pCacheHead) {
			_pCacheHead->pPrev = nullptr;

			if (!_pCacheHead->pNext)
				_pCacheTail = _pCacheHead;
		} else {
			_pCacheTail = nullptr;
		}
	} else {
		RdTexCacheEntry *pPrev = pEntry->pPrev;

		if (pEntry == _pCacheTail) {
			_pCacheTail = pEntry->pPrev;
			pPrev->pNext = nullptr;
		} else {
			pPrev->pNext = pEntry->pNext;
			pEntry->pNext->pPrev = pEntry->pPrev;
		}
	}

	_numCachedTextures--;

	if (_pCurDevice)
		_pCurDevice->texMemFree += pEntry->size;
	else
		warning("Std3D::cacheListRemove(): device not started");
}

int Std3D::getTextureMemory(uint *pTotal, uint *pFree) {
	uint used = 0;

	for (RdTexCacheEntry *pEntry = _pCacheHead; pEntry; pEntry = pEntry->pNext)
		used += pEntry->size;

	*pTotal = kStd3DTextureMemory;
	*pFree = kStd3DTextureMemory - used;
	return 1;
}

void Std3D::cacheListTouch(RdTexCacheEntry *pEntry) {
	pEntry->lastFrame = _frameCounter;
	cacheListRemove(pEntry);
	cacheListAppend(pEntry);
}

int Std3D::evictTextures(uint size) {
	RdTexCacheEntry *pScan = _pCacheHead;
	uint bestDiff = (uint)-1;
	RdTexCacheEntry *pBest = nullptr;

	for (RdTexCacheEntry *pEntry = _pCacheHead; pEntry; pEntry = pEntry->pNext) {
		if (pEntry->lastFrame != _frameCounter) {
			uint entrySize = pEntry->size;

			if (entrySize == size) {
				releaseTexture(pEntry);
				pEntry->bInCache = 0;
				pEntry->lastFrame = 0;
				cacheListRemove(pEntry);
				return 1;
			}

			if (entrySize > size) {
				uint diff = entrySize - size;

				if (diff < bestDiff) {
					bestDiff = diff;
					pBest = pEntry;
				}
			}
		}
	}

	if (pBest) {
		releaseTexture(pBest);
		pBest->bInCache = 0;
		pBest->lastFrame = 0;
		cacheListRemove(pBest);
		return 1;
	}

	while (pScan) {
		RdTexCacheEntry *pNext = pScan->pNext;

		if (pScan->lastFrame != _frameCounter) {
			releaseTexture(pScan);
			pScan->bInCache = 0;
			pScan->lastFrame = 0;
			cacheListRemove(pScan);
		}

		pScan = pNext;
	}

	return 1;
}

} // End of namespace Grim

#endif
