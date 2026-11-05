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

#include "common/textconsole.h"

#include "engines/grim/renderdroid/rd.h"

namespace Grim {

namespace {

uint alignEdgeX(uint x) {
	if ((uint16)x)
		return (x + 0x10000) & 0xFFFF0000;

	return x;
}

} // End of anonymous namespace

typedef void (RenderDroid::AFRasterModule::*RdSetupFunc)(RdActiveFace *pFace, RdMatCel *pCel);
typedef void (RenderDroid::AFRasterModule::*RdSetupLightFunc)(RdActiveFace *pFace, RdMatCel *pCel, float light);

RenderDroid::ActiveModule::ActiveModule(RenderDroid *rd) : Module(rd), _drawnFaces(0), _numActiveEdges(0), _numActiveFaces(0), _numActiveSpans(0), _yCurScanLine(0), _yMaxEdge(0), _yMinEdge(0) {
	_aActiveEdges = new RdActiveEdge[4096]();
	_aActiveFaces = new RdActiveFace[1024]();
	_aActiveSpans = new RdActiveSpan[2048]();
	_pActiveEdgeHead = new RdActiveEdge();
	_pActiveEdgeTail = new RdActiveEdge();
	memset(_apNewActiveEdges, 0, sizeof(_apNewActiveEdges));
	memset(_apRemoveActiveEdges, 0, sizeof(_apRemoveActiveEdges));
}

RenderDroid::ActiveModule::~ActiveModule() {
	delete[] _aActiveEdges;
	delete[] _aActiveFaces;
	delete[] _aActiveSpans;
	delete _pActiveEdgeHead;
	delete _pActiveEdgeTail;
}

int RenderDroid::ActiveModule::startup() {
	return 1;
}

int RenderDroid::ActiveModule::advanceFrame() {
	RdCanvas *pCanvas = _rd->_camera.getCurrent()->pCanvas;
	int yStart = pCanvas->yStart;
	int numLines = pCanvas->yEnd - yStart + 1;

	if (yStart < 0 || numLines < 0 || yStart + numLines > 1024) {
		warning("RenderDroid::ActiveModule::advanceFrame(): canvas lines %d..%d out of range", yStart, pCanvas->yEnd);
		yStart = 0;
		numLines = 0;
	}

	_pActiveEdgeHead->x = -65536;
	_pActiveEdgeHead->pNext = _pActiveEdgeTail;
	_pActiveEdgeTail->pPrev = _pActiveEdgeHead;
	memset(&_apNewActiveEdges[yStart], 0, sizeof(RdActiveEdge *) * numLines);
	memset(&_apRemoveActiveEdges[yStart], 0, sizeof(RdActiveEdge *) * numLines);
	_yMinEdge = 0x7FFFFFFF;
	_yMaxEdge = 0;
	_pActiveEdgeHead->pPrev = nullptr;
	_pActiveEdgeTail->x = 0x7FFFFFFF;
	_pActiveEdgeTail->pNext = nullptr;
	_numActiveFaces = 0;
	_numActiveEdges = 0;
	_numActiveSpans = 0;

	return 0;
}

void RenderDroid::ActiveModule::clearFrameCounters() {
	_drawnFaces = 0;
}

uint RenderDroid::ActiveModule::drawScene() {
	buildEdges();
	_yCurScanLine = _yMinEdge;

	if (_yMinEdge <= _yMaxEdge) {
		RdActiveEdge *pNext = _pActiveEdgeHead->pNext;

		do {
			RdActiveEdge *pNewEdge = _apNewActiveEdges[_yCurScanLine];

			while (pNewEdge) {
				int x = pNewEdge->x;
				RdActiveEdge *pFollowing = pNewEdge->pNext;

				while (x > pNext->x)
					pNext = pNext->pNext;

				pNewEdge->pNext = pNext;
				pNewEdge->pPrev = pNext->pPrev;
				pNext->pPrev->pNext = pNewEdge;
				pNext->pPrev = pNewEdge;

				if (pNewEdge->bLeft)
					pNewEdge->pFace->pLeftEdge = pNewEdge;
				else
					pNewEdge->pFace->pRightEdge = pNewEdge;

				pNewEdge = pFollowing;
			}

			buildSpans();

			for (RdActiveEdge *pRemove = _apRemoveActiveEdges[_yCurScanLine]; pRemove; pRemove = pRemove->pNextRemove) {
				pRemove->pNext->pPrev = pRemove->pPrev;
				pRemove->pPrev->pNext = pRemove->pNext;
			}

			pNext = _pActiveEdgeHead->pNext;

			RdActiveEdge *pEdge = _pActiveEdgeHead->pNext;

			while (pEdge != _pActiveEdgeTail) {
				(_rd->_afRaster.*pEdge->pfStep)(pEdge);

				RdActiveEdge *pPrev = pEdge->pPrev;

				if (pEdge->x >= pPrev->x) {
					pEdge = pEdge->pNext;
				} else {
					RdActiveEdge *pAfter = pEdge->pNext;

					pAfter->pPrev = pPrev;
					pEdge->pPrev->pNext = pAfter;

					int x = pEdge->x;
					RdActiveEdge *pSearch = pEdge->pPrev->pPrev;

					while (x < pSearch->x)
						pSearch = pSearch->pPrev;

					RdActiveEdge *pInsertBefore = pSearch->pNext;

					pEdge->pPrev = pSearch;
					pEdge->pNext = pInsertBefore;
					pInsertBefore->pPrev = pEdge;
					pSearch->pNext = pEdge;
					pEdge = pAfter;
				}
			}

			pNext = _pActiveEdgeHead->pNext;
			_yCurScanLine++;
		} while (_yCurScanLine <= _yMaxEdge);
	}

	return flushSpans();
}

uint RenderDroid::ActiveModule::flushSpans() {
	for (uint i = 0; i < _numActiveFaces; i++) {
		RdActiveFace *pFace = &_aActiveFaces[i];

		if (pFace->pFirstSpan) {
			(_rd->_afRaster.*pFace->pfDrawSpans)(pFace);
			pFace->pFirstSpan = nullptr;
			pFace->pLastSpan = nullptr;
		}
	}

	_numActiveSpans = 0;

	return _numActiveFaces;
}

uint RenderDroid::ActiveModule::buildEdges() {
	RdProcFace *aProcFaces = _rd->_cache.getProcFaces();
	size_t numProcFaces = _rd->_cache.getNumProcFaces();

	for (size_t i = 0; i < numProcFaces; i++)
		addActiveFace(&aProcFaces[i]);

	RdActiveEdge *pEdge = _aActiveEdges;

	for (uint f = 0; f < _numActiveFaces; f++) {
		RdActiveFace *pFace = &_aActiveFaces[f];
		RdProcFace *pProcFace = pFace->pProcFace;
		uint numVertices = pProcFace->numVertices;

		for (uint v = 0; v < numVertices; v++) {
			int next = numVertices - 1 >= v + 1 ? v + 1 : 0;
			int result;

			if (_numActiveEdges >= 4096) {
				warning("RenderDroid::ActiveModule::buildEdges(): too many edges");
				return f;
			}

			if (pFace->pProcFace->bBackface)
				result = (_rd->_afRaster.*pFace->pfSetupEdge)(pEdge, pFace, next, v);
			else
				result = (_rd->_afRaster.*pFace->pfSetupEdge)(pEdge, pFace, v, next);

			if (!result)
				continue;

			int yStart = pEdge->yStart;
			uint yEnd = pEdge->numLines + yStart - 1;

			if (yStart < 0 || yStart >= 1024 || yEnd >= 1024) {
				warning("RenderDroid::ActiveModule::buildEdges(): edge lines %d..%u out of range", yStart, yEnd);
				continue;
			}

			pEdge->pFace = pFace;

			RdActiveEdge *pList = _apNewActiveEdges[yStart];

			if (pList && pEdge->x > pList->x) {
				int x = pEdge->x;

				for (RdActiveEdge *pSearch = pList->pNext; pSearch; pSearch = pSearch->pNext) {
					if (pSearch->x >= x)
						break;

					pList = pSearch;
				}

				pEdge->pNext = pList->pNext;
				pList->pNext = pEdge;
			} else {
				pEdge->pNext = pList;
				_apNewActiveEdges[yStart] = pEdge;
			}

			pEdge->pNextRemove = _apRemoveActiveEdges[yEnd];
			_apRemoveActiveEdges[yEnd] = pEdge;

			if ((uint)yStart < _yMinEdge)
				_yMinEdge = yStart;

			if (yEnd > _yMaxEdge)
				_yMaxEdge = yEnd;

			pEdge++;
			_numActiveEdges++;
		}
	}

	return _numActiveFaces;
}

void RenderDroid::ActiveModule::addSpan(RdActiveFace *pFace, uint alignedEndX) {
	int width = (int)(alignedEndX - pFace->curX) >> 16;

	if (width <= 0)
		return;

	if (_numActiveSpans >= 0x800)
		flushSpans();

	RdActiveSpan *pSpan = &_aActiveSpans[_numActiveSpans];
	RdActiveEdge *pLeft = pFace->pLeftEdge;
	RdActiveEdge *pRight = pFace->pRightEdge;

	_numActiveSpans++;

	pSpan->x = pFace->curX >> 16;
	pSpan->width = width;
	pSpan->y = _yCurScanLine;
	pSpan->pNext = nullptr;

	uint edgeWidth = (alignEdgeX(pRight->x) - alignEdgeX(pLeft->x)) >> 16;
	int offset = pFace->curX - pLeft->x;
	float fraction = (float)((double)offset * (1.0 / 65536.0));

	if (edgeWidth >= 2048) {
		warning("RenderDroid::ActiveModule::addSpan(): edge width %u out of range", edgeWidth);
		edgeWidth = 0;
	}

	pSpan->oneOverZ = (float)(pFace->dOneOverZdX * fraction + pLeft->oneOverZ);

	if (pFace->pProcFace->geometryMode == 4) {
		int textureMode = pFace->pProcFace->textureMode;

		if (textureMode) {
			if (textureMode == 1) {
				pSpan->uOverZ = (float)(pFace->dUOverZdX * fraction + pLeft->uOverZ);
				pSpan->vOverZ = (float)(pFace->dVOverZdX * fraction + pLeft->vOverZ);
			}
		} else {
			int64 oneOverN = _rd->_raster.getOneOverNFixed(edgeWidth);
			int du = (int)((uint64)(oneOverN * (int64)(pRight->u - pLeft->u)) >> 16);
			int dv;

			pSpan->du = du >> pFace->mipLevel;
			pSpan->u = (int)(((uint64)((int64)du * offset) >> 16) + pLeft->u) >> pFace->mipLevel;
			dv = (int)((uint64)(oneOverN * (int64)(pRight->v - pLeft->v)) >> 16);
			pSpan->dv = dv >> pFace->mipLevel;
			pSpan->v = (int)(((uint64)((int64)dv * offset) >> 16) + pLeft->v) >> pFace->mipLevel;
		}
	} else {
		pSpan->u = pFace->colorIndex;
	}

	if (pFace->pProcFace->lightingMode == 3) {
		uint dIntensity = (uint)((uint64)((int64)_rd->_raster.getOneOverNFixed(edgeWidth) * (int64)(pRight->intensity - pLeft->intensity)) >> 16);

		pSpan->dIntensity = dIntensity;

		if (dIntensity > 0xFFFFFAE2)
			pSpan->dIntensity = 0;

		pSpan->intensity = (int)(((uint64)((int64)pSpan->dIntensity * offset) >> 16) + pLeft->intensity);
	}

	if (pFace->pFirstSpan)
		pFace->pLastSpan->pNext = pSpan;
	else
		pFace->pFirstSpan = pSpan;

	pFace->pLastSpan = pSpan;
}

int RenderDroid::ActiveModule::buildSpans() {
	if (!_numActiveEdges)
		return 0;

	RdActiveEdge *pEdge = _pActiveEdgeHead->pNext;
	RdActiveFace *pTop = nullptr;

	while (pEdge != _pActiveEdgeTail) {
		RdActiveFace *pFace = pEdge->pFace;

		if (!pEdge->bLeft) {
			pFace->activeEdgeCount--;

			if (!pFace->activeEdgeCount) {
				if (pFace == pTop) {
					uint alignedX = alignEdgeX(pEdge->x);

					addSpan(pTop, alignedX);

					pTop = pTop->pNextFace;

					if (pTop) {
						pTop->curX = alignedX;
						pTop->pPrevFace = nullptr;
					}
				} else {
					if (pFace->pNextFace)
						pFace->pNextFace->pPrevFace = pFace->pPrevFace;

					if (pFace->pPrevFace)
						pFace->pPrevFace->pNextFace = pFace->pNextFace;
				}
			}
		} else {
			pFace->activeEdgeCount++;

			if (pFace->activeEdgeCount == 1) {
				if (pFace->pProcFace->geometryMode != 4 || pFace->pProcFace->textureMode != 1) {
					RdActiveEdge *pRightEdge = pFace->pRightEdge;
					RdActiveEdge *pLeftEdge = pFace->pLeftEdge;
					uint edgeWidth = (alignEdgeX(pRightEdge->x) - alignEdgeX(pLeftEdge->x)) >> 16;

					if (edgeWidth >= 2048) {
						warning("RenderDroid::ActiveModule::buildSpans(): edge width %u out of range", edgeWidth);
						edgeWidth = 0;
					}

					double step = ((double)pRightEdge->oneOverZ - pLeftEdge->oneOverZ) * _rd->_raster.getOneOverNFlex(edgeWidth);

					pFace->dOneOverZdX = (float)step;
					pFace->dOneOverZdXFixed = (int)floor((double)(float)(step * 65536.0));
				}

				if (!pTop) {
					pFace->pPrevFace = nullptr;
					pFace->pNextFace = nullptr;
					pTop = pFace;
					pFace->curX = alignEdgeX(pEdge->x);
				} else {
					uint userData = pFace->pProcFace->userData;
					uint topUserData = pTop->pProcFace->userData;
					bool inFront;

					if ((uint16)userData != (uint16)topUserData || (userData >> 16) == (topUserData >> 16)) {
						double topDepth = (double)(pEdge->x - pTop->pLeftEdge->x) * (1.0 / 65536.0) * pTop->dOneOverZdX + pTop->pLeftEdge->oneOverZ;

						inFront = pEdge->oneOverZ > topDepth || (pEdge->oneOverZ == topDepth && pFace->dOneOverZdX > (double)pTop->dOneOverZdX);
					} else {
						inFront = (userData >> 16) > (topUserData >> 16);
					}

					if (inFront) {
						uint alignedX = alignEdgeX(pEdge->x);

						addSpan(pTop, alignedX);
						pFace->pNextFace = pTop;
						pTop->pPrevFace = pFace;
						pFace->pPrevFace = nullptr;
						pTop = pFace;
						pFace->curX = alignedX;
					} else {
						RdActiveFace *pNextFace = pTop->pNextFace;
						RdActiveFace *pPrevFace = pTop;
						float edgeDepth = pEdge->oneOverZ;

						while (pNextFace) {
							uint nextUserData = pNextFace->pProcFace->userData;
							bool front;

							if ((uint16)userData == (uint16)nextUserData && (userData >> 16) != (nextUserData >> 16)) {
								front = (userData >> 16) > (nextUserData >> 16);
							} else {
								double nextDepth = (double)(pEdge->x - pNextFace->pLeftEdge->x) * (1.0 / 65536.0) * pNextFace->dOneOverZdX + pNextFace->pLeftEdge->oneOverZ;

								front = edgeDepth > nextDepth || (edgeDepth == nextDepth && pFace->dOneOverZdX > (double)pNextFace->dOneOverZdX);
							}

							if (front)
								break;

							pPrevFace = pNextFace;
							pNextFace = pNextFace->pNextFace;
						}

						if (pNextFace) {
							pFace->pPrevFace = pPrevFace;
							pPrevFace->pNextFace = pFace;
							pNextFace->pPrevFace = pFace;
							pFace->pNextFace = pNextFace;
						} else {
							pPrevFace->pNextFace = pFace;
							pFace->pPrevFace = pPrevFace;
							pFace->pNextFace = nullptr;
						}
					}
				}
			}
		}

		pEdge = pEdge->pNext;
	}

	return 0;
}

int RenderDroid::ActiveModule::addActiveFace(RdProcFace *pProcFace) {
	if (_numActiveFaces >= 0x400)
		return 0;

	int geometryMode = pProcFace->geometryMode;

	if (geometryMode >= _rd->_curGeometryMode)
		geometryMode = _rd->_curGeometryMode;

	int lightingMode = pProcFace->lightingMode;

	if (lightingMode >= _rd->_curLightingMode)
		lightingMode = _rd->_curLightingMode;

	int textureMode = pProcFace->textureMode;

	if (textureMode >= _rd->_curTextureMode)
		textureMode = _rd->_curTextureMode;

	if (!geometryMode)
		return 0;

	RdActiveFace *pFace = &_aActiveFaces[_numActiveFaces];

	pFace->pProcFace = pProcFace;
	pFace->pFirstSpan = nullptr;
	pFace->pLastSpan = nullptr;
	pFace->pLeftEdge = nullptr;
	pFace->pRightEdge = nullptr;
	pFace->activeEdgeCount = 0;

	RdMatCel *pCel = _rd->_cache.selectCel(pProcFace);

	if (!pCel && !(pProcFace->flags & 1)) {
		warning("RenderDroid::ActiveModule::addActiveFace(): face without a material cel");
		return 0;
	}

	int effectiveMode = geometryMode;

	if (pCel && effectiveMode == 4 && !(pCel->flags & 8))
		effectiveMode = 3;

	float ambientLight = 0.0f;

	if (_rd->_curRenderOptions & 2)
		ambientLight = pProcFace->ambientLight;

	if (effectiveMode == 2) {
		if ((uint)lightingMode <= 4) {
			if (pProcFace->flags & 1)
				pProcFace->pfUserSetup(pFace, pCel, 0.0f);
			else if (_rd->_curZBufferMethod == 1)
				_rd->_afRaster.setupNGonLW(pFace, pCel);
		}
	} else if (effectiveMode == 3 || effectiveMode == 4) {
		RdSetupFunc pfNone = nullptr;
		RdSetupLightFunc pfLight = nullptr;
		RdSetupFunc pfVertex = nullptr;

		if (effectiveMode == 3) {
			pfNone = &RenderDroid::AFRasterModule::setupNGonLS;
			pfLight = &RenderDroid::AFRasterModule::setupNGonFS;
			pfVertex = &RenderDroid::AFRasterModule::setupNGonGS;
		} else if (pCel->pTexture->flags & 1) {
			if (textureMode == 0) {
				pfNone = &RenderDroid::AFRasterModule::setupNGonMLAT;
				pfLight = &RenderDroid::AFRasterModule::setupNGonMFAT;
				pfVertex = &RenderDroid::AFRasterModule::setupNGonMGAT;
			} else if (textureMode > 0 && textureMode <= 2) {
				pfNone = &RenderDroid::AFRasterModule::setupNGonMLIT;
				pfLight = &RenderDroid::AFRasterModule::setupNGonMFIT;
				pfVertex = &RenderDroid::AFRasterModule::setupNGonMGIT;
			}
		} else {
			if (textureMode == 0) {
				pfNone = &RenderDroid::AFRasterModule::setupNGonLAT;
				pfLight = &RenderDroid::AFRasterModule::setupNGonFAT;
				pfVertex = &RenderDroid::AFRasterModule::setupNGonGAT;
			} else if (textureMode > 0 && textureMode <= 2) {
				pfNone = &RenderDroid::AFRasterModule::setupNGonLIT;
				pfLight = &RenderDroid::AFRasterModule::setupNGonFIT;
				pfVertex = &RenderDroid::AFRasterModule::setupNGonGIT;
			}
		}

		if (pfNone) {
			float light;

			switch (lightingMode) {
			case 0:
				if (pProcFace->flags & 1)
					pProcFace->pfUserSetup(pFace, pCel, 0.0f);
				else if (_rd->_curZBufferMethod == 1)
					(_rd->_afRaster.*pfNone)(pFace, pCel);
				break;
			case 1:
			case 2:
				light = _rd->_cache.calcFaceLight(pProcFace, lightingMode, ambientLight);
				pProcFace->diffuseLight = light;

				if (pProcFace->flags & 1)
					pProcFace->pfUserSetup(pFace, pCel, light);
				else if (_rd->_curZBufferMethod == 1)
					(_rd->_afRaster.*pfLight)(pFace, pCel, light);
				break;
			case 3:
				_rd->_cache.calcVertexLights(pProcFace, ambientLight);

				if (pProcFace->flags & 1)
					pProcFace->pfUserSetup(pFace, pCel, 0.0f);
				else if (_rd->_curZBufferMethod == 1)
					(_rd->_afRaster.*pfVertex)(pFace, pCel);
				break;
			default:
				break;
			}
		}
	}

	_drawnFaces++;
	_numActiveFaces++;

	return 1;
}

} // End of namespace Grim
