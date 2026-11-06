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

#include "common/stream.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputrender.h"

namespace Grim {

SputRender::SputRender(RenderDroid *pRd, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, Res *pRes, SputResource *pSputResource,
		SputPrimitive *pSputPrimitive, WalkSystem *pWalkSystem, StdVBuffer *pBackBuffer, StdVBuffer *pMainGfxBuffer) :
		_pRd(pRd), _pStdDisplay(pStdDisplay), _pStdBitmap(pStdBitmap), _pRes(pRes), _pSputResource(pSputResource),
		_pSputPrimitive(pSputPrimitive), _pWalkSystem(pWalkSystem), _pBackBuffer(pBackBuffer), _pMainGfxBuffer(pMainGfxBuffer),
		_pCurrentRoom(nullptr), _aActors(nullptr),
		_pHS(nullptr), _bInitialized(0), _bStarted(0),
		_gammaCorrection(1.0f), _fillColor(0), _pColorInfo(nullptr), _b3DAcceleration(0), _bAcceleratedZBuffers(1),
		_pDrawBuffer(nullptr), _pZBuffer(nullptr), _pCanvas(nullptr), _pCleanBuffer(nullptr), _pCleanArtBuffer(nullptr),
		_pCleanZBuffer(nullptr), _pDecompressionBuffer(0), _backBufferPixels(nullptr), _backBufferBytesPerPixel(0),
		_backBufferPitch(0), _numPages(1), _pDirtyRectQueue(nullptr), _pFreeDirtyRects(nullptr), _numDirtyRects(0),
		_nextSerialNumber(0), _pCacheRectList(nullptr), _reset(0), _irisStart(0), _irisX(0.0f), _irisY(320),
		_irisDuration(240), _renderFlags(0), _bSkipPrimitives(0), _pSetPaletteFunc(nullptr), _pPageFlipFunc(nullptr),
		_pageFlipEnable(1), _movieJustFinished(0), _gCamActorIndex(0), _debugFlags(8), _bDebugKey1Down(0),
		_bDebugKey2Down(0), _bDebugKey4Down(0), _bDebugKey8Down(0), _bDebugKey10Down(0), _renderEnable(1),
		_loadImageAllowed(1), _perfCounter(0), _waitBitmapTimerMs(0), _waitBitmapFrame(0),
		_ambient(0.2f), _curView(-1), _pLastRoom(nullptr), _pCamera(nullptr), _dirtyRectColor(0), _bPrintLuaStack(0),
		_numSortActors(0) {
	memset(_renderVars, 0, sizeof(_renderVars));
	memset(_colorLookup, 0, sizeof(_colorLookup));
	memset(&_waitBitmapRef, 0, sizeof(_waitBitmapRef));
	memset(_aColors, 0, sizeof(_aColors));
	memset(_aActorSortInfo, 0, sizeof(_aActorSortInfo));
	memset(_aSortedActors, 0, sizeof(_aSortedActors));
}

int SputRender::initialize(SysStruct *pHS) {
	_pHS = pHS;

	// TODO: register the control callback
	_bInitialized = 1;

	return 0;
}

int SputRender::terminate() {
	_pHS = nullptr;

	if (_bInitialized)
		_bInitialized = 0;

	return 0;
}

int SputRender::startup() {
	if (_bStarted)
		return 1;

	_bStarted = 1;
	_renderVars[0] = 4;
	_renderVars[1] = 3;
	_renderVars[2] = 1;
	_renderVars[3] = 1;
	_renderVars[4] = 0;
	_renderVars[5] = 0;
	_renderVars[6] = 2;

	return 0;
}

int SputRender::shutdown() {
	freeBuffers();

	while (_pDirtyRectQueue) {
		DirtyRect *pNext = _pDirtyRectQueue->pNext;

		delete _pDirtyRectQueue;
		_pDirtyRectQueue = pNext;
	}

	_numDirtyRects = 0;

	while (_pFreeDirtyRects) {
		DirtyRect *pNext = _pFreeDirtyRects->pNextFree;

		delete _pFreeDirtyRects;
		_pFreeDirtyRects = pNext;
	}

	_bStarted = 0;

	return 0;
}

int SputRender::save(Common::WriteStream *pStream) {
	pStream->writeSint32LE(_irisStart);
	pStream->writeFloatLE(_irisX);
	pStream->writeSint32LE(_irisY);
	pStream->writeSint32LE(_irisDuration);
	pStream->writeFloatLE(_gammaCorrection);
	pStream->writeSint32LE(_renderFlags);
	pStream->writeSint32LE(_pageFlipEnable);
	pStream->writeSint32LE(_gCamActorIndex);
	pStream->writeSint32LE(_movieJustFinished);

	if (pStream->err()) {
		warning("SputRender::save(): failed to write the render state");
		return 1;
	}

	return 0;
}

int SputRender::restore(Common::ReadStream *pStream) {
	_irisStart = pStream->readSint32LE();
	_irisX = pStream->readFloatLE();
	_irisY = pStream->readSint32LE();
	_irisDuration = pStream->readSint32LE();
	_gammaCorrection = pStream->readFloatLE();
	_renderFlags = pStream->readSint32LE();
	_pageFlipEnable = pStream->readSint32LE();
	_gCamActorIndex = pStream->readSint32LE();
	_movieJustFinished = pStream->readSint32LE();

	if (pStream->err() || pStream->eos()) {
		warning("SputRender::restore(): failed to read the render state");
		return 1;
	}

	return 0;
}

int SputRender::open() {
	// TODO: allocate the camera helper actor and set its first flag

	return 1;
}

void SputRender::close() {
	if (_pCanvas)
		_pRd->_canvas.destroy(_pCanvas);

	if (_pCleanArtBuffer)
		_pStdDisplay->vBufferFree(_pCleanArtBuffer);

	if (_pCleanZBuffer)
		_pStdDisplay->vBufferFree(_pCleanZBuffer);

	if (_pCleanBuffer)
		_pStdDisplay->vBufferFree(_pCleanBuffer);

	_pDecompressionBuffer = 0;
}

int SputRender::setDisplay(StdVBuffer *pVBuffer, StdVBuffer *pZBuffer, int xStart, int yStart, int xEnd, int yEnd, int *pOptions) {
	close();

	_pCanvas = _pRd->_canvas.create(1, pVBuffer, pZBuffer, xStart, yStart, xEnd, yEnd, 0);
	_pZBuffer = pZBuffer;
	_pDrawBuffer = pVBuffer;

	StdVBufferTexFmt format = pVBuffer->tex;

	_pCleanBuffer = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);

	if (pVBuffer->bVideoMemory)
		_pCleanArtBuffer = _pStdDisplay->vBufferNew(&format, 1, 1, nullptr);
	else
		_pCleanArtBuffer = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);

	int bVideoMemory = (_b3DAcceleration && !_bAcceleratedZBuffers) ? 1 : 0;

	_pCleanZBuffer = _pStdDisplay->vBufferNew(&format, bVideoMemory, bVideoMemory, nullptr);

	if (!_pCleanArtBuffer || !_pCleanZBuffer)
		return 0;

	_backBufferPixels = _pBackBuffer->pPixels;
	_backBufferBytesPerPixel = (uint)_pBackBuffer->tex.format.bpp >> 3;
	// TODO: set the BlastText screen height
	_backBufferPitch = _pBackBuffer->tex.pitchInBytes;
	// TODO: set the BlastText screen width

	pOptions[3] |= 2;
	setRenderOptions(pOptions);

	Room *pRoom = _pCurrentRoom; // TODO: sputRoom_GetCurrentRoom()

	if (pRoom)
		_pRd->_camera.setAmbientLight(pRoom->pSetups[pRoom->curSetup].camera.prdCamera, _ambient);

	_pColorInfo = &_pBackBuffer->tex.format;
	loadSetupBackground(_pColorInfo); // TODO: pass grim_colorInfoCopy
	_pDecompressionBuffer = 1;

	pRoom = _pCurrentRoom; // TODO: sputRoom_GetCurrentRoom()

	if (pRoom)
		useSetup(pRoom, pRoom->curSetup);

	forceRefresh();

	return 1;
}

int SputRender::queryDisplayState() {
	if (!_pCanvas) {
		warning("SputRender::queryDisplayState(): the display is not set");
		return 0;
	}

	return _pCanvas->pVBuffer->type == 1;
}

void SputRender::using3DHardware(int b3D) {
	_b3DAcceleration = b3D;
}

void SputRender::pageCountIs(int numPages) {
	_numPages = 1;

	if (numPages >= 1)
		_numPages = numPages;
}

void SputRender::setPaletteCallback(SputRenderPaletteFunc pFunc) {
	_pSetPaletteFunc = pFunc;
}

void SputRender::setPageFlipCallback(SputRenderPageFlipFunc pFunc) {
	_pPageFlipFunc = pFunc;
}

SputRenderPaletteFunc SputRender::getPaletteFunc() {
	return _pSetPaletteFunc;
}

void SputRender::setPageFlip(int enable) {
	_pageFlipEnable = enable;
}

int SputRender::pageFlip() {
	int result = _pageFlipEnable;

	if (_pageFlipEnable) {
		result = _movieJustFinished;

		if (!_movieJustFinished && _pPageFlipFunc)
			result = _pPageFlipFunc();
	}

	_movieJustFinished = 0;

	return result;
}

int SputRender::setRenderOptions(const int *pOptions) {
	memcpy(_renderVars, pOptions, sizeof(_renderVars));

	_pRd->setGeometryMode(_renderVars[0]);
	_pRd->setLightingMode(_renderVars[1]);
	_pRd->setTextureMode(_renderVars[2]);
	_pRd->setRenderOptions(_renderVars[3]);
	_pRd->setSortingMethod(_renderVars[4]);
	_pRd->setOcclusionMethod(_renderVars[5]);

	return _pRd->setZBufferMethod(_renderVars[6]);
}

int SputRender::controlCallback() {
	int result = 0;
	int bKeyDown;

	bKeyDown = 0; // TODO: grimlib_bDebugKeys
	if (!bKeyDown)
		return 0;

	bKeyDown = 0; // TODO: stdControl_ReadKey(29, 0, 0)
	if (!bKeyDown)
		return result;

	bKeyDown = 0; // TODO: stdControl_ReadKey(32, 0, 0)
	if (!bKeyDown)
		return result;

	result = 1;

	bKeyDown = 0; // TODO: stdControl_ReadKey(57, 0, 0)
	if (bKeyDown) {
		if (!_bDebugKey1Down) {
			drawDebuggingToggle();
			_bDebugKey1Down = 1;
		}
	} else {
		_bDebugKey1Down = 0;
	}

	if ((_debugFlags & 1) == 0)
		return result;

	bKeyDown = 0; // TODO: stdControl_ReadKey(48, 0, 0)
	if (bKeyDown) {
		if (!_bDebugKey8Down) {
			if (_debugFlags & 8) {
				_bDebugKey8Down = 1;
				_debugFlags &= ~8;
			} else {
				_bDebugKey8Down = 1;
				_debugFlags |= 8;
			}
		}
	} else {
		_bDebugKey8Down = 0;
	}

	bKeyDown = 0; // TODO: stdControl_ReadKey(38, 0, 0)
	if (bKeyDown) {
		if (!_bDebugKey2Down) {
			if (_debugFlags & 2) {
				_bDebugKey2Down = 1;
				_debugFlags &= ~2;
			} else {
				_bDebugKey2Down = 1;
				_debugFlags |= 2;
			}
		}
	} else {
		_bDebugKey2Down = 0;
	}

	bKeyDown = 0; // TODO: stdControl_ReadKey(46, 0, 0)
	if (bKeyDown) {
		if (!_bDebugKey4Down) {
			if (_debugFlags & 4) {
				_bDebugKey4Down = 1;
				_debugFlags &= ~4;
			} else {
				_bDebugKey4Down = 1;
				_debugFlags |= 4;
			}
		}
	} else {
		_bDebugKey4Down = 0;
	}

	bKeyDown = 0; // TODO: stdControl_ReadKey(19, 0, 0)
	if (!bKeyDown) {
		_bDebugKey10Down = 0;
		return result;
	}

	if (_bDebugKey10Down)
		return result;

	if (_debugFlags & 0x10) {
		_bDebugKey10Down = 1;
		_debugFlags &= ~0x10;
	} else {
		_bDebugKey10Down = 1;
		_debugFlags |= 0x10;
	}

	return result;
}

void SputRender::forceRefresh() {
	_reset = 1;
}

void SputRender::markDirtyRect(const StdRect *pRect, int page) {
	StdRect rect = *pRect;

	if (rect.width > 0 && rect.height > 0) {
		int width = rect.x % 4 + rect.width;

		rect.x = rect.x - rect.x % 4;
		rect.width = width;

		if (width % 4)
			rect.width = 4 - width % 4 + width;

		markDirtyRectInternal(&rect, page, _numPages - 1);
	}
}

void SputRender::markAllPagesDirty(const StdRect *pRect, int page) {
	StdRect rect = *pRect;

	if (rect.width > 0 && rect.height > 0) {
		int width = rect.x % 4 + rect.width;

		rect.x = rect.x - rect.x % 4;
		rect.width = width;

		if (width % 4)
			rect.width = 4 - width % 4 + width;

		for (int i = 0; i < _numPages; i++)
			markDirtyRectInternal(&rect, page, i);
	}
}

void SputRender::markDirtyRectInternal(StdRect *pRect, int page, uint pageIndex) {
	int left = pRect->x;
	int right = pRect->x + pRect->width;
	int top = pRect->y;
	int bottom = pRect->y + pRect->height;
	DirtyRect *pNode = _pDirtyRectQueue;

	while (pNode) {
		if (pNode->pageIndex != pageIndex) {
			pNode = pNode->pNext;
			continue;
		}

		int nodeLeft = pNode->rect.x;
		int nodeTop = pNode->rect.y;
		int nodeRight = nodeLeft + pNode->rect.width;
		int nodeBottom = nodeTop + pNode->rect.height;

		if (left >= nodeRight || right <= nodeLeft || top >= nodeBottom || bottom <= nodeTop) {
			pNode = pNode->pNext;
			continue;
		}

		if (left >= nodeLeft && top >= nodeTop && right <= nodeRight && bottom <= nodeBottom) {
			if (page > pNode->page)
				pNode->page = page;

			return;
		}

		if (pNode->pNext)
			pNode->pNext->pPrev = pNode->pPrev;

		if (pNode->pPrev)
			pNode->pPrev->pNext = pNode->pNext;
		else
			_pDirtyRectQueue = pNode->pNext;

		if (pNode->page > page)
			page = pNode->page;

		DirtyRect *pFreed = pNode;

		pNode = pNode->pNext;
		_numDirtyRects--;
		pFreed->pNextFree = _pFreeDirtyRects;
		_pFreeDirtyRects = pFreed;

		if (nodeLeft >= left && nodeLeft <= right && nodeTop >= top && nodeTop <= bottom &&
				nodeRight >= left && nodeRight <= right && nodeBottom >= top && nodeBottom <= bottom)
			continue;

		StdRect strip;

		strip.x = MIN(nodeLeft, left);
		strip.y = MAX(nodeTop, top);
		strip.width = MAX(nodeRight, right) - strip.x;
		strip.height = MIN(nodeBottom, bottom) - strip.y;

		int bDirty = page != 0 || (pNode && pNode->page != 0);

		markDirtyRectInternal(&strip, bDirty, pageIndex);

		StdRect piece;

		if (nodeTop != top) {
			if (nodeTop < top) {
				piece.x = nodeLeft;
				piece.y = nodeTop;
				piece.width = nodeRight - nodeLeft;
				piece.height = strip.y - nodeTop;
			} else {
				piece.x = left;
				piece.y = top;
				piece.width = right - left;
				piece.height = strip.y - top;
			}

			bDirty = page != 0 || (pNode && pNode->page != 0);

			markDirtyRectInternal(&piece, bDirty, pageIndex);
		}

		if (nodeBottom != bottom) {
			int stripBottom = strip.y + strip.height;

			if (nodeBottom > bottom) {
				piece.x = nodeLeft;
				piece.y = stripBottom;
				piece.width = nodeRight - nodeLeft;
				piece.height = nodeBottom - stripBottom;
			} else {
				piece.x = left;
				piece.y = stripBottom;
				piece.width = right - left;
				piece.height = bottom - stripBottom;
			}

			bDirty = page != 0 || (pNode && pNode->page != 0);

			markDirtyRectInternal(&piece, bDirty, pageIndex);
		}

		return;
	}

	DirtyRect *pNew;

	if (_pFreeDirtyRects) {
		pNew = _pFreeDirtyRects;
		_pFreeDirtyRects = pNew->pNextFree;
		pNew->pNextFree = nullptr;
	} else {
		pNew = new DirtyRect();
	}

	_numDirtyRects++;
	pNew->rect = *pRect;
	pNew->page = page;
	pNew->serial = _nextSerialNumber++;
	pNew->pageIndex = pageIndex;
	pNew->pNext = _pDirtyRectQueue;

	if (_pDirtyRectQueue)
		_pDirtyRectQueue->pPrev = pNew;

	pNew->pPrev = nullptr;
	_pDirtyRectQueue = pNew;
}

void SputRender::freeCacheRects() {
	DirtyRect *pNode = _pDirtyRectQueue;

	while (pNode) {
		DirtyRect *pNext = pNode->pNext;

		pNode->pNextFree = _pFreeDirtyRects;
		_pFreeDirtyRects = pNode;
		pNode = pNext;
		_pDirtyRectQueue = pNext;
	}

	_numDirtyRects = 0;
}

} // End of namespace Grim
