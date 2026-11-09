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

#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/std/std_bitmap.h"

namespace Grim {

void SputRender::freeBuffers() {
	while (_pCacheRectList) {
		CacheRect *pRect = _pCacheRectList;

		_pCacheRectList = pRect->pNext;

		if (pRect->pColor)
			_pStdDisplay->vBufferFree(pRect->pColor);

		if (pRect->pZ)
			_pStdDisplay->vBufferFree(pRect->pZ);

		delete pRect;
	}
}

CacheRect *SputRender::createCacheRect(int x, int y, int width, int height) {
	if (!_pCleanArtBuffer || !_pCleanZBuffer) {
		warning("SputRender::createCacheRect(): the display is not set");
		return nullptr;
	}

	CacheRect *pRect = new CacheRect();

	pRect->magic = 0x11111111;
	pRect->rect.x = x;
	pRect->rect.y = y;
	pRect->rect.width = width;
	pRect->rect.height = height;

	StdVBufferTexFmt format = _pCleanArtBuffer->tex;

	format.height = height;
	format.width = width;
	pRect->pColor = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);
	_pStdDisplay->vBufferCopy(pRect->pColor, _pCleanArtBuffer, 0, 0, &pRect->rect, 0);

	format = _pCleanZBuffer->tex;
	format.width = width;
	format.height = height;
	pRect->pZ = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);
	_pStdDisplay->vBufferCopy(pRect->pZ, _pCleanZBuffer, 0, 0, &pRect->rect, 0);

	pRect->pNext = _pCacheRectList;
	_pCacheRectList = pRect;

	return pRect;
}

void SputRender::restoreCacheRect() {
	CacheRect *pRect = _pCacheRectList;

	if (!pRect || !_pCleanArtBuffer || !_pCleanZBuffer) {
		warning("SputRender::restoreCacheRect(): no cache rectangle to restore");
		return;
	}

	_pStdDisplay->vBufferCopy(_pCleanArtBuffer, pRect->pColor, pRect->rect.x, pRect->rect.y, nullptr, 0);
	_pStdDisplay->vBufferCopy(_pCleanZBuffer, pRect->pZ, pRect->rect.x, pRect->rect.y, nullptr, 0);
	markAllPagesDirty(&pRect->rect, 1);
}

void SputRender::addAnimationBitmap(void *pBitmap, int layer) {
	for (AnimationBitmap *pNode = _pAnimationList; pNode; pNode = pNode->pNext) {
		if (!pNode->bitmapRef.valid)
			warning("SputRender::addAnimationBitmap(): Bad Bitmap Reference");

		if (pNode->bitmapRef.handle == pBitmap)
			return;
	}

	AnimationBitmap *pNode = new AnimationBitmap();

	memset(pNode, 0, sizeof(AnimationBitmap));
	pNode->pNext = _pAnimationList;

	if (_pAnimationList)
		_pAnimationList->pPrev = pNode;

	_pAnimationList = pNode;
	pNode->layer = layer;
	_pRes->setReference(&pNode->bitmapRef, pBitmap);
}

void SputRender::removeAnimationBitmap(void *pBitmap) {
	AnimationBitmap *pNode = _pAnimationList;

	while (pNode && !(pNode->bitmapRef.valid && pNode->bitmapRef.handle == pBitmap))
		pNode = pNode->pNext;

	if (!pNode)
		return;

	if (pNode->pNext)
		pNode->pNext->pPrev = pNode->pPrev;

	if (pNode->pPrev)
		pNode->pPrev->pNext = pNode->pNext;
	else
		_pAnimationList = pNode->pNext;

	_pRes->removeReference(&pNode->bitmapRef);
	delete pNode;
}

void SputRender::drawAnimationBitmaps(int layer) {
	for (AnimationBitmap *pNode = _pAnimationList; pNode; pNode = pNode->pNext) {
		if (pNode->layer != layer)
			continue;

		if (!pNode->bitmapRef.valid) {
			warning("SputRender::drawAnimationBitmaps(): Bad Bitmap Reference");
			continue;
		}

		StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pNode->bitmapRef.handle);
		int frame = pBitmap->curFrame;

		if (frame < 0)
			continue;

		if (frame >= (int)pBitmap->numFrames) {
			warning("SputRender::drawAnimationBitmaps(): frame %d is out of range", frame);
			continue;
		}

		StdVBuffer *pFrame = pBitmap->apFrames[frame];

		if (pFrame->type == 2) {
			fillDrawBuffer(pFrame);
			_pStdDisplay->vBufferCopy(_pBackBuffer, _pCleanBuffer, pBitmap->xPos, pBitmap->yPos, nullptr, 1);
		} else {
			_pStdDisplay->vBufferCopy(_pBackBuffer, pFrame, pBitmap->xPos, pBitmap->yPos, nullptr, 1);
		}
	}
}

StdVBuffer *SputRender::drawBuffer() {
	return _pBackBuffer;
}

StdVBuffer *SputRender::cleanArtBuffer() {
	return _pCleanArtBuffer;
}

StdVBuffer *SputRender::cleanZBuffer() {
	return _pCleanZBuffer;
}

int SputRender::copyGFXBuffer() {
	if (!_pCleanArtBuffer) {
		warning("SputRender::copyGFXBuffer(): the display is not set");
		return 0;
	}

	return _pStdDisplay->vBufferCopy(_pCleanArtBuffer, _pBackBuffer, 0, 0, nullptr, 0);
}

int SputRender::cleanDrawBuffer() {
	int mode = _pGrim->getLibMode();
	int result = mode;

	if (mode == 4) {
		if (!_pCleanArtBuffer) {
			warning("SputRender::cleanDrawBuffer(): the display is not set");
			return 0;
		}

		StdRect rect;

		rect.x = 0;
		rect.y = 0;
		rect.width = 640;
		rect.height = 480;

		return _pStdDisplay->vBufferCopy(_pBackBuffer, _pCleanArtBuffer, 0, 0, &rect, 0);
	}

	return result;
}

void SputRender::fillDrawBuffer(StdVBuffer *pSrc) {
	if (!_pCleanBuffer) {
		warning("SputRender::fillDrawBuffer(): the display is not set");
		return;
	}

	int size = pSrc->tex.width * pSrc->tex.height;

	_pStdDisplay->vBufferDeCompressTo(pSrc, _pCleanBuffer);
	_pCleanBuffer->tex = pSrc->tex;
	_pCleanBuffer->colorKey = 63519;

	if (pSrc->tex.format.colorMode != 5) {
		colorCorrect((uint16 *)_pCleanBuffer->pPixels, size);
		_pCleanBuffer->colorKey = _colorLookup[63519];
	} else if (_pGrim->getGameFlags() & ADGF_DEMO) {
		_pCleanBuffer->colorKey = _colorLookup[63519];
	}
}

StdVBuffer *SputRender::screenShot(int width, int height) {
	if (!_pCleanArtBuffer) {
		warning("SputRender::screenShot(): the display is not set");
		return nullptr;
	}

	int oldMode = _pGrim->getLibMode();

	_pGrim->setLibMode(2);
	_reset = 1;
	dim(1, 0);
	_pGrim->setLibMode(oldMode);

	StdVBuffer *pClean = _pCleanArtBuffer;

	if (width < 0 || height < 0 || pClean->tex.format.bpp != 16 || pClean->tex.width < 640 || pClean->tex.height < 480) {
		warning("SputRender::screenShot(): unsupported buffer or size");
		return nullptr;
	}

	StdVBufferTexFmt format = pClean->tex;

	format.width = width;
	format.height = height;

	StdVBuffer *pShot = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);

	// The original could divide by zero here, let's avoid that...
	float xStep = 0.0f;
	float yStep = 0.0f;

	if (width > 0)
		xStep = (float)(640.0 / (double)width);

	if (height > 0)
		yStep = (float)(480.0 / (double)height);

	_pStdDisplay->vBufferLock(pShot);
	_pStdDisplay->vBufferLock(pClean);

	double y = 0.0;

	for (int row = 0; row < height; row++) {
		const uint16 *pSrcRow = (const uint16 *)(pClean->pPixels + pClean->tex.pitchInBytes * (int)y);
		uint16 *pDstRow = (uint16 *)(pShot->pPixels + pShot->tex.pitchInBytes * row);
		double x = 0.0;

		for (int column = 0; column < width; column++) {
			pDstRow[column] = pSrcRow[(int)x];
			x = x + (double)xStep;
		}

		y = y + (double)yStep;
	}

	_pStdDisplay->vBufferUnlock(pShot);
	_pStdDisplay->vBufferUnlock(pClean);
	_reset = 1;

	return pShot;
}

void SputRender::saveScreen(const char *path) {
	StdColorFormat format;

	format.redBits = 8;
	format.greenBits = 8;
	format.blueBits = 8;
	format.greenShift = 8;
	format.colorMode = 1;
	format.bpp = 24;
	format.redShift = 16;
	format.blueShift = 0;
	format.redLoss = 0;
	format.greenLoss = 0;
	format.blueLoss = 0;
	format.alphaBits = 0;
	format.alphaShift = 0;
	format.alphaLoss = 0;

	if (path) {
		StdBitmapStruct *pBitmap = _pStdBitmap->vBufferToBitmap(_pBackBuffer, 0, 0);

		if (!pBitmap) {
			warning("SputRender::saveScreen(): failed to convert the back buffer");
			return;
		}

		_pStdBitmap->convertColorFormat(&format, pBitmap);

		Common::String fileName(path);

		_pStdBitmap->loadLbm2(fileName, pBitmap);
		_pStdBitmap->unloadBitmap(pBitmap);
	}
}

void SputRender::saveZBuffer(const char *path) {
	if (!_pZBuffer || !path) {
		warning("SputRender::saveZBuffer(): no Z buffer or no file name");
		return;
	}

	_pStdDisplay->vBufferLock(_pZBuffer);
	_pStdDisplay->vBufferUnlock(_pZBuffer);

	StdBitmapStruct *pBitmap = _pStdBitmap->vBufferToBitmap(_pZBuffer, 0, 0);

	if (!pBitmap) {
		warning("SputRender::saveZBuffer(): failed to convert the Z buffer");
		return;
	}

	_pStdBitmap->write(path, pBitmap);
	_pStdBitmap->unloadBitmap(pBitmap);
}

void SputRender::saveDrawBuffer(const char *path) {
	StdColorFormat format;

	format.redBits = 8;
	format.greenBits = 8;
	format.blueBits = 8;
	format.greenShift = 8;
	format.colorMode = 1;
	format.bpp = 24;
	format.redShift = 16;
	format.blueShift = 0;
	format.redLoss = 0;
	format.greenLoss = 0;
	format.blueLoss = 0;
	format.alphaBits = 0;
	format.alphaShift = 0;
	format.alphaLoss = 0;

	if (!path) {
		warning("SputRender::saveDrawBuffer(): no file name");
		return;
	}

	StdBitmapStruct *pBitmap = _pStdBitmap->vBufferToBitmap(_pBackBuffer, 0, 0);

	if (!pBitmap) {
		warning("SputRender::saveDrawBuffer(): failed to convert the back buffer");
		return;
	}

	_pStdBitmap->convertColorFormat(&format, pBitmap);

	Common::String fileName(path);

	_pStdBitmap->loadLbm2(fileName, pBitmap);
	_pStdBitmap->unloadBitmap(pBitmap);
}

} // End of namespace Grim
