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

#include "common/algorithm.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/blasttext.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

void SputRender::drawDebuggingToggle() {
	if (_debugFlags & 1)
		_debugFlags &= ~1;
	else
		_debugFlags |= 1;

	if (_debugFlags & 1)
		return;

	RdCamera *pCamera = _pRd->_camera.getCurrent();

	if (!pCamera || !pCamera->pCanvas) {
		warning("SputRender::drawDebuggingToggle(): no current camera canvas");
		return;
	}

	StdRect rect;

	rect.x = pCamera->pCanvas->xStart;
	rect.y = pCamera->pCanvas->yStart;
	rect.width = pCamera->pCanvas->xEnd - pCamera->pCanvas->xStart + 1;
	rect.height = pCamera->pCanvas->yEnd - pCamera->pCanvas->yStart + 1;

	for (int i = 0; i < _numPages; i++)
		markDirtyRectInternal(&rect, 1, i);
}

void SputRender::syncCamera(Room *pRoom, int setup) {
	RoomSetup *pSetup = &pRoom->pSetups[setup];
	ResRef *pColormapRef = &pRoom->apColormaps[pRoom->field_88];

	if (!pColormapRef->valid)
		error("SputRender::syncCamera(): Bad Colormap Reference");

	_pRd->_colormap.setCurrent(static_cast<RdColormap *>(pColormapRef->handle));
	_pRd->_camera.setCurrent(pSetup->camera.prdCamera);

	if (_pCanvas)
		_pRd->_camera.setCanvas(pSetup->camera.prdCamera, _pCanvas);

	_pRd->_camera.setAspectRatio(pSetup->camera.prdCamera, 1.0f);
	_pRd->_camera.update(pSetup->camera.posMatrix);
}

void SputRender::useSetup(Room *pRoom, int setup) {
	if (!pRoom)
		return;

	if (!_pCleanBuffer || !_pCleanArtBuffer || !_pCleanZBuffer || !_pCanvas || !_aActors) {
		warning("SputRender::useSetup(): the display or the actors are not set");
		return;
	}

	RoomSetup *pSetup = &pRoom->pSetups[setup];

	freeBuffers();
	syncCamera(pRoom, setup);
	_renderFlags &= ~3;
	_pCamera = &pSetup->camera;

	SputActor *pCamActor = _pSputActor->actorDeref(_gCamActorIndex);
	float (*pOrient)[3] = _pSputActor->getOrientMatrix(pCamActor);

	memcpy(pOrient, pSetup->camera.posMatrix, sizeof(RdMatrix34));

	StdVBuffer *pBackground = _pSputRoom->getSetupBackground(pRoom, setup);

	if (pBackground) {
		if (pBackground->type == 2) {
			fillDrawBuffer(pBackground);
			_pStdDisplay->vBufferCopy(_pCleanArtBuffer, _pCleanBuffer, 0, 0, nullptr, 0);
		} else {
			_pStdDisplay->vBufferCopy(_pCleanArtBuffer, pBackground, 0, 0, nullptr, 0);
		}
	}

	StdVBuffer *pZBackground = _pSputRoom->getSetupZBackground(pRoom, setup);

	if (pZBackground) {
		if (pZBackground->type == 2) {
			_pCleanBuffer->tex = _pCleanZBuffer->tex;

			if (_pCleanZBuffer->bVideoMemory) {
				_pStdDisplay->vBufferDeCompressTo(pZBackground, _pCleanBuffer);
				_pStdDisplay->vBufferCopy(_pCleanZBuffer, _pCleanBuffer, 0, 0, nullptr, 0);
			} else {
				_pStdDisplay->vBufferDeCompressTo(pZBackground, _pCleanZBuffer);
			}
		} else {
			_pStdDisplay->vBufferCopy(_pCleanZBuffer, pZBackground, 0, 0, nullptr, 0);
		}
	} else {
		if (!_pCleanZBuffer->pPixels || _pCleanZBuffer->tex.sizeInBytes < 0x96000)
			warning("SputRender::useSetup(): the Z buffer is smaller than 640x480x16");
		else
			memset(_pCleanZBuffer->pPixels, 0, 0x96000);

		_renderFlags |= 2;
	}

	for (SputObjectState *pState = pSetup->ObjectStates; pState; pState = pState->pNext) {
		pState->loadResources();
		pState->reset(_pCleanArtBuffer, _pCleanZBuffer);
	}

	StdRect rect;

	rect.x = _pCanvas->xStart;
	rect.y = _pCanvas->yStart;
	rect.width = _pCanvas->xEnd - _pCanvas->xStart + 1;
	rect.height = _pCanvas->yEnd - _pCanvas->yStart + 1;
	markAllPagesDirty(&rect, 1);
}

void SputRender::update() {
	int mode = 2; // TODO: grimlib_mode

	switch (mode) {
	case 0:
		return;
	case 3: {
		// TODO: set the SMUSH target to the back buffer
		int bMovieFinished = 0; // TODO: sputSmush_Periodic()

		if (bMovieFinished) {
			// TODO: set grimlib_mode to 2
			StdRect rect;

			rect.x = 0;
			rect.y = 0;
			rect.width = 640;
			rect.height = 480;

			for (int i = 0; i < _numPages; i++)
				markDirtyRectInternal(&rect, 1, i);

			forceRefresh();
			_movieJustFinished = 1;
		}

		float frameTime = 0.0f; // TODO: sputTime_GetFrameTimeFlex()

		_pSputActor->updateSpeechOnly(frameTime);
		_pStdDisplay->vBufferLock(_pBackBuffer);
		_pBlastText->getSurface()->pPixels = _pBackBuffer->pPixels;
		_pSputText->update(0);
		_pStdDisplay->vBufferUnlock(_pBackBuffer);
		pageFlip();
		return;
	}
	case 4:
		if (_reset) {
			// TODO: grimlib_UserPaint
		}

		_reset = 0;
		return;
	default:
		break;
	}

	if (_bPrintLuaStack) {
		// TODO: lua_printstack(0)
	}

	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom)
		return;

	if (_pLastRoom != pRoom || pRoom->curSetup != _curView || _reset) {
		freeCacheRects();
		_pLastRoom = pRoom;
		_curView = _pLastRoom->curSetup;
		useSetup(_pLastRoom, _curView);
		// TODO: sputLua_PostCameraChange(_curView)
	}

	RoomSetup *pSetup = &_pLastRoom->pSetups[_curView];

	syncCamera(_pLastRoom, _curView);

	for (SputObjectState *pState = pSetup->ObjectStates; pState; pState = pState->pNext)
		pState->update(3, _pCleanArtBuffer, _pCleanZBuffer);

	// TODO: sputMain_SetPerfCounter(4)

	if (_pageFlipEnable)
		restoreDirtyRects(_pBackBuffer, _pZBuffer);

	// TODO: sputMain_QueryPerfCounterDiff(4)
	// TODO: set the SMUSH target to the back buffer
	// TODO: sputSmush_Periodic()
	_pStdDisplay->vBufferLock(_pBackBuffer);
	drawPrimitives(1);
	_pStdDisplay->vBufferUnlock(_pBackBuffer);

	for (SputObjectState *pState = pSetup->ObjectStates; pState; pState = pState->pNext)
		pState->update(1, _pBackBuffer, _pZBuffer);

	_pRd->advanceFrame();
	sortActors();

	if (_numSortActors || (_debugFlags & 1)) {
		// TODO: sputMain_SetPerfCounter(5)
		int numSorted = 0;

		for (int i = 0; i < _numSortActors; i++) {
			if (_aSortedActors[i]->kind == 0) {
				SputActor *pEnumActor = &_aActors[_aSortedActors[i]->actorIndex];

				_pSputActor->enumCostumeComponents(pEnumActor->pCostumeStack->hCostumeInstance);
			}

			numSorted++;
		}

		// TODO: sputMain_QueryPerfCounterDiff(5)

		if (_debugFlags & 1) {
			_pStdDisplay->vBufferLock(_pBackBuffer);

			if (_debugFlags & 8)
				drawWalkPlanes();
			else
				drawBoundingBox();

			_pStdDisplay->vBufferUnlock(_pBackBuffer);
		}

		// TODO: sputMain_SetPerfCounter(6)

		for (int i = numSorted - 1; i >= 0; i--) {
			ActorSortInfo *pEntry = _aSortedActors[i];
			SputActor *pActor = &_aActors[pEntry->actorIndex];
			float (*pOrient)[3] = _pSputActor->getOrientMatrix(pActor);

			if (pActor->scale != 1.0f) {
				RdVector3 scale;

				scale[0] = pActor->scale;
				scale[1] = pActor->scale;
				scale[2] = pActor->scale;
				_pRd->preScale34(pOrient, scale);
			}

			if (pEntry->kind == 0) {
				RdColormap *pColormap;

				if (!pActor->pColorMap)
					warning("SputRender::update(): actor without colormap reference");

				if (pActor->pColorMap && pActor->pColorMap->valid) {
					pColormap = static_cast<RdColormap *>(pActor->pColorMap->handle);
				} else {
					ResRef *pColormapRef = &_pLastRoom->apColormaps[_pLastRoom->field_88];

					if (!pColormapRef->valid)
						error("SputRender::update(): Bad Colormap Reference");

					pColormap = static_cast<RdColormap *>(pColormapRef->handle);
				}

				_pRd->_colormap.setCurrent(pColormap);

				if (_pSetPaletteFunc)
					_pSetPaletteFunc(pColormap->aColors, 0);

				_pSputActor->drawCostume(pActor->pCostumeStack, pOrient);

				if (_debugFlags & 1)
					drawActorDebug(pActor);
			} else if (pEntry->kind == 1) {
				if (!_b3DAcceleration)
					_pStdDisplay->vBufferLock(_pBackBuffer);

				drawActor(pActor);

				if (!_b3DAcceleration)
					_pStdDisplay->vBufferUnlock(_pBackBuffer);

				markDirtyRect(&pActor->drawnRect, 1);
			}

			if (pActor->scale != 1.0f) {
				RdVector3 scale;

				scale[0] = (float)(1.0 / (double)pActor->scale);
				scale[1] = scale[0];
				scale[2] = scale[0];
				pOrient = _pSputActor->getOrientMatrix(pActor);
				_pRd->preScale34(pOrient, scale);
			}
		}

		// TODO: sputMain_QueryPerfCounterDiff(6)
		// TODO: sputMain_SetPerfCounter(7)
		_pRd->finishFrame();
		// TODO: sputMain_QueryPerfCounterDiff(7)

		for (SputObjectState *pState = pSetup->ObjectStates; pState; pState = pState->pNext)
			pState->update(2, _pBackBuffer, _pZBuffer);
	}

	_pStdDisplay->vBufferLock(_pBackBuffer);
	drawPrimitives(2);
	_pStdDisplay->vBufferUnlock(_pBackBuffer);

	if (_debugFlags & 1) {
		Room *pDebugRoom = _pSputRoom->getCurrentRoom();

		if (_debugFlags & 2)
			drawCameraDebug(pDebugRoom);

		if (_debugFlags & 4)
			drawActorBoxes(pDebugRoom, 0xFFFFFFFF);
	}

	if (_renderFlags & 4) {
		int gameTime = 0; // TODO: sputTime_GetGameTime()
		double ratio;

		if (_irisX == 0.0f) {
			// The original could divide by zero here, let's avoid that...
			warning("SputRender::update(): the iris duration is zero");
			ratio = 2.0;
		} else {
			ratio = (double)(uint)((uint)gameTime - (uint)_irisStart) / (double)_irisX;
		}

		if (ratio < 0.0)
			ratio = -ratio;

		if (!(ratio > 1.0)) {
			_renderFlags &= ~8;
		} else {
			ratio = 1.0;

			if (_irisX < 0.0f)
				_renderFlags &= ~4;
			else
				_renderFlags |= 8;
		}

		float percent;

		if (_irisX <= 0.0f)
			percent = (float)(1.0 - ratio);
		else
			percent = (float)ratio;

		getIris(percent, _irisY, _irisDuration);

		if (_irisX < 0.0f) {
			StdRect rect;

			rect.x = 0;
			rect.y = 0;
			rect.width = 640;
			rect.height = 480;
			markAllPagesDirty(&rect, 0);
		}
	}

	_pStdDisplay->vBufferLock(_pBackBuffer);
	_pBlastText->getSurface()->pPixels = _pBackBuffer->pPixels;
	_pSputText->update(1);
	_pStdDisplay->vBufferUnlock(_pBackBuffer);
	_reset = 0;
}

int SputRender::loadSetupBackground(const StdColorFormat *pFormat) {
	if (!pFormat || !_pColorInfo) {
		warning("SputRender::loadSetupBackground(): the display is not set");
		return 0;
	}

	if (pFormat->redShift < 0 || pFormat->redShift > 31 || pFormat->greenShift < 0 || pFormat->greenShift > 31 ||
			pFormat->blueShift < 0 || pFormat->blueShift > 31 || pFormat->redLoss < 0 || pFormat->redLoss > 31 ||
			pFormat->greenLoss < 0 || pFormat->greenLoss > 31 || pFormat->blueLoss < 0 || pFormat->blueLoss > 31) {
		warning("SputRender::loadSetupBackground(): unsupported color format");
		return 0;
	}

	int redShift = pFormat->redShift;
	int greenShift = pFormat->greenShift;
	int blueShift = pFormat->blueShift;
	int redLoss = pFormat->redLoss;
	int greenLoss = pFormat->greenLoss;
	int blueLoss = pFormat->blueLoss;

	buildGammaTable(_pColorInfo, _gammaCorrection);

	_aColors[2] = _colorLookup[0];
	_aColors[4] = _colorLookup[(uint16)((uint)(128 >> redLoss) << redShift) | (uint16)((uint)(128 >> blueLoss) << blueShift) |
			(uint16)((uint)(128 >> greenLoss) << greenShift)];
	_aColors[3] = _colorLookup[(uint16)((uint)(255 >> greenLoss) << greenShift) | (uint16)((uint)(255 >> redLoss) << redShift) |
			(uint16)((uint)(255 >> blueLoss) << blueShift)];
	_aColors[5] = _colorLookup[(uint16)((uint)(255 >> greenLoss) << greenShift) | (uint16)((uint)(255 >> redLoss) << redShift)];
	_aColors[6] = _colorLookup[(uint16)((uint)(255 >> redLoss) << redShift)];
	_aColors[7] = _colorLookup[(uint16)((uint)(255 >> greenLoss) << greenShift)];
	_aColors[8] = _colorLookup[(uint16)((uint)(255 >> blueLoss) << blueShift)];
	_aColors[10] = _colorLookup[(uint16)((uint)(128 >> redLoss) << redShift)];
	_aColors[11] = _colorLookup[(uint16)((uint)(128 >> greenLoss) << greenShift)];
	_aColors[12] = _colorLookup[(uint16)((uint)(128 >> redLoss) << redShift) | (uint16)((uint)(128 >> greenLoss) << greenShift)];
	_aColors[13] = _colorLookup[(uint16)((uint)(128 >> blueLoss) << blueShift)];
	_aColors[9] = _colorLookup[(uint16)((uint)(128 >> redLoss) << redShift) | (uint16)((uint)(128 >> blueLoss) << blueShift)];
	_aColors[14] = _colorLookup[(uint16)((uint)(128 >> blueLoss) << blueShift) | (uint16)((uint)(128 >> greenLoss) << greenShift)];
	_aColors[15] = _colorLookup[(uint16)((uint)(192 >> redLoss) << redShift) | (uint16)((uint)(192 >> blueLoss) << blueShift) |
			(uint16)((uint)(192 >> greenLoss) << greenShift)];
	_aColors[16] = _colorLookup[(uint16)((uint)(192 >> redLoss) << redShift) | (uint16)((uint)(192 >> blueLoss) << blueShift) |
			(uint16)((uint)(220 >> greenLoss) << greenShift)];
	_aColors[17] = _colorLookup[(uint16)((uint)(166 >> redLoss) << redShift) | (uint16)((uint)(202 >> greenLoss) << greenShift) |
			(uint16)((uint)(240 >> blueLoss) << blueShift)];
	_aColors[18] = _colorLookup[(uint16)((uint)(180 >> redLoss) << redShift) | (uint16)((uint)(180 >> blueLoss) << blueShift) |
			(uint16)((uint)(180 >> greenLoss) << greenShift)];
	_aColors[19] = _colorLookup[(uint16)((uint)(100 >> redLoss) << redShift) | (uint16)((uint)(100 >> blueLoss) << blueShift) |
			(uint16)((uint)(100 >> greenLoss) << greenShift)];
	_aColors[20] = _colorLookup[(uint16)((uint)(255 >> redLoss) << redShift) | (uint16)((uint)(255 >> blueLoss) << blueShift)];
	_aColors[21] = _colorLookup[(uint16)((uint)(255 >> greenLoss) << greenShift) | (uint16)((uint)(255 >> blueLoss) << blueShift)];

	_pSputResource->loadBknd();

	return _pSputResource->cmap();
}

void SputRender::preRendered(int bBitmap, int bZBuffer) {
	_renderFlags |= 3;

	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom)
		return;

	RoomSetup *pSetup = &pRoom->pSetups[pRoom->curSetup];

	if (bBitmap && pSetup->bitmapRef.valid)
		_renderFlags &= ~1;

	if (bZBuffer && pSetup->zbufferRef.valid)
		_renderFlags &= ~2;

	RdCamera *pCamera = _pRd->_camera.getCurrent();

	if (!pCamera || !pCamera->pCanvas) {
		warning("SputRender::preRendered(): no current camera canvas");
		return;
	}

	StdRect rect;

	rect.x = pCamera->pCanvas->xStart;
	rect.y = pCamera->pCanvas->yStart;
	rect.width = pCamera->pCanvas->xEnd - pCamera->pCanvas->xStart + 1;
	rect.height = pCamera->pCanvas->yEnd - pCamera->pCanvas->yStart + 1;

	for (int i = 0; i < _numPages; i++)
		markDirtyRectInternal(&rect, 1, i);
}

void SputRender::reverseProject(int x, int y, float *pOut) {
	RdCamera *pCamera = _pRd->_camera.getCurrent();

	if (!pCamera)
		return;

	RdCanvas *pCanvas = pCamera->pCanvas;

	if (!pCanvas)
		return;

	if (!_pZBuffer || !_pZBuffer->pPixels || !_pCamera || x < 0 || y < 0 || x >= _pZBuffer->tex.width || y >= _pZBuffer->tex.height) {
		warning("SputRender::reverseProject(): the point is outside the Z buffer");
		return;
	}

	uint16 zPixel = ((uint16 *)_pZBuffer->pPixels)[_pZBuffer->tex.width * y + x];
	int scaledZ = (int)((uint)zPixel << (pCanvas->zScaleShift & 31));
	float depth = (float)((double)scaledZ * 0.0000152587890625);
	double focalDepth = (double)pCamera->focalLen * (double)depth;

	if (depth == 0.0f || focalDepth == 0.0) {
		// The original could divide by zero here, let's avoid that...
		warning("SputRender::reverseProject(): the Z buffer value is zero");
		return;
	}

	RdVector3 point;

	point[1] = (float)(1.0 / (double)depth - 1.0);
	point[0] = (float)(((double)x - (double)pCanvas->xCenter) * (1.0 / focalDepth));
	point[2] = (float)(((double)y - (double)pCanvas->yCenter) * (-1.0 / focalDepth));
	_pRd->transformPoint34(pOut, point, _pCamera->posMatrix);
}

int SputRender::buildShadowBitmap(float *pOut, float *pA, float *pB, float a, float b, float c, float d) {
	double deltaX = (double)pB[0] - (double)pA[0];
	float deltaY = (float)((double)pB[1] - (double)pA[1]);
	float deltaZ = (float)((double)pB[2] - (double)pA[2]);
	double distA = (((double)b * (double)pA[1] + (double)c * (double)pA[2]) + (double)a * (double)pA[0]) + (double)d;
	float distB = (float)((((double)b * (double)pB[1] + (double)c * (double)pB[2]) + (double)a * (double)pB[0]) + (double)d);
	float distAStored = (float)distA;
	double difference = distA - (double)distB;
	float differenceStored = (float)difference;
	double absDifference = (double)differenceStored;

	if (!(difference >= 0.0))
		absDifference = -absDifference;

	if (!(absDifference >= 0.1))
		return 0;

	double t = (double)distAStored / (double)differenceStored;

	pOut[0] = (float)(t * deltaX + (double)pA[0]);
	pOut[1] = (float)(t * (double)deltaY + (double)pA[1]);
	pOut[2] = (float)(t * (double)deltaZ + (double)pA[2]);

	return 1;
}

void SputRender::restoreDirtyRects(StdVBuffer *pDst, StdVBuffer *pZBuffer) {
	DirtyRect *pNode = _pDirtyRectQueue;
	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom)
		return;

	if (!pDst || !_pCleanArtBuffer || !_pCleanZBuffer || !_pCanvas) {
		warning("SputRender::restoreDirtyRects(): the display is not set");
		return;
	}

	if (_debugFlags & 1) {
		StdRect canvasRect;

		canvasRect.x = _pCanvas->xStart;
		canvasRect.y = _pCanvas->yStart;
		canvasRect.width = _pCanvas->xEnd - _pCanvas->xStart + 1;
		canvasRect.height = _pCanvas->yEnd - _pCanvas->yStart + 1;
		_pStdDisplay->vBufferCopy(pDst, _pCleanArtBuffer, canvasRect.x, canvasRect.y, &canvasRect, 0);
	}

	_dirtyRectColor++;

	while (pNode) {
		StdRect *pRect = &pNode->rect;

		if (pZBuffer && pNode->page) {
			if (_renderFlags & 2) {
				if (_b3DAcceleration) {
					// TODO: std3D_SetActiveRect(pRect)
					// TODO: std3D_ClearZBuffer()
				} else {
					_pStdDisplay->vBufferFill(pZBuffer, 0, pRect);
				}
			} else {
				_pStdDisplay->vBufferCopy(pZBuffer, _pCleanZBuffer, pRect->x, pRect->y, pRect, 0);
			}
		}

		int remaining = (int)pNode->pageIndex;

		pNode->pageIndex = (uint)remaining - 1;

		if (remaining > 0) {
			pNode = pNode->pNext;
			continue;
		}

		if (_renderFlags & 1)
			_pStdDisplay->vBufferFill(pDst, 0, pRect);
		else
			_pStdDisplay->vBufferCopy(pDst, _pCleanArtBuffer, pRect->x, pRect->y, pRect, 0);

		if (pNode->pNext)
			pNode->pNext->pPrev = pNode->pPrev;

		if (pNode->pPrev)
			pNode->pPrev->pNext = pNode->pNext;

		if (pNode == _pDirtyRectQueue)
			_pDirtyRectQueue = pNode->pNext;

		RdCamera *pCamera = _pRd->_camera.getCurrent();

		if ((_debugFlags & 1) && (_debugFlags & 0x10)) {
			if (!pCamera || !pCamera->pCanvas) {
				warning("SputRender::restoreDirtyRects(): no current camera canvas");
			} else {
				_pStdDisplay->vBufferLock(_pBackBuffer);
				_pRd->_primit2.drawRectangle(pCamera->pCanvas, pRect->x, pRect->y, pRect->x + pRect->width, pRect->y + pRect->height,
						_dirtyRectColor, 0xFFFFFFFF);
				_pStdDisplay->vBufferUnlock(_pBackBuffer);
			}
		}

		DirtyRect *pFreed = pNode;

		pNode = pNode->pNext;
		_numDirtyRects--;
		pFreed->pNextFree = _pFreeDirtyRects;
		_pFreeDirtyRects = pFreed;
	}
}

void SputRender::drawPrimitives(int layerMask) {
	if (_bSkipPrimitives)
		return;

	RdCamera *pCamera = _pRd->_camera.getCurrent();

	if (!pCamera) {
		warning("SputRender::drawPrimitives(): no current camera");
		return;
	}

	_pSputPrimitive->getCurrent()->draw(pCamera->pCanvas, layerMask);
}

void SputRender::sortActors() {
	resetActorSortList();

	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom || !pRoom->pActorNode || !_aActors || !_pCamera) {
		warning("SputRender::sortActors(): no current room, actors or camera");
		return;
	}

	for (RoomActorLink *pLink = pRoom->pActorNode->pActors; pLink; pLink = pLink->pNext) {
		SputActor *pActor = &_aActors[pLink->actorIndex];

		pActor->onScreen = 0;

		if ((pActor->flags & 3) == 0) {
			ActorSortInfo *pEntry = allocActorSortEntry();

			if (!pEntry)
				return;

			RdVector3 vec;
			RdVector3 out;

			vec[0] = (float)((double)pActor->actorPos[0] - (double)_pCamera->position[0]);
			vec[1] = (float)((double)pActor->actorPos[1] - (double)_pCamera->position[1]);
			vec[2] = (float)((double)pActor->actorPos[2] - (double)_pCamera->position[2]);
			_pRd->transformVectorOrtho34(out, vec, _pCamera->posMatrix);
			pEntry->depth = out[1];

			if (pActor->pCostumeStack) {
				pEntry->kind = 0;
				pEntry->actorIndex = (int)(pActor - _aActors);
			} else {
				pEntry->actorIndex = (int)(pActor - _aActors);

				CostumeNode *pCostumeStack = pActor->pCostumeStack;

				if (pCostumeStack && pCostumeStack->pHeadThing)
					pEntry->kind = 1;
				else
					pEntry->kind = -1;
			}
		}
	}

	sort(_aSortedActors, _numSortActors, sizeof(ActorSortInfo *), compareActorDepth);
}

void SputRender::resetActorSortList() {
	_numSortActors = 0;

	for (int i = 0; i < 128; i++)
		_aSortedActors[i] = &_aActorSortInfo[i];
}

ActorSortInfo *SputRender::allocActorSortEntry() {
	if (_numSortActors >= 128)
		return nullptr;

	return &_aActorSortInfo[_numSortActors++];
}

int SputRender::compareActorDepth(const void *pA, const void *pB) {
	const ActorSortInfo *pInfoA = *(const ActorSortInfo *const *)pA;
	const ActorSortInfo *pInfoB = *(const ActorSortInfo *const *)pB;

	if (!(pInfoA->depth >= pInfoB->depth))
		return 1;

	return 0;
}

void SputRender::drawActor(SputActor *pActor) {
	if (!pActor->pCostumeStack || !_pDrawBuffer) {
		warning("SputRender::drawActor(): the actor has no costume or the display is not set");
		return;
	}

	int *pViewport = _pRd->_cache.getViewportRect();

	pViewport[0] = _pBackBuffer->tex.width;
	pViewport[1] = _pDrawBuffer->tex.height;
	pViewport[2] = 0;
	pViewport[3] = 0;
	pActor->drawnRect.x = 0;
	pActor->drawnRect.y = 0;
	pActor->drawnRect.width = 0;
	pActor->drawnRect.height = 0;

	float (*pOrient)[3] = _pSputActor->getOrientMatrix(pActor);

	if (_pRd->_thing.draw(pActor->pCostumeStack->pHeadThing, pOrient)) {
		pActor->onScreen = 1;
		pActor->drawnRect.x = pViewport[0];
		pActor->drawnRect.y = pViewport[1];
		pActor->drawnRect.width = pViewport[2] - pViewport[0];
		pActor->drawnRect.height = pViewport[3] - pViewport[1];
	}
}

} // End of namespace Grim
