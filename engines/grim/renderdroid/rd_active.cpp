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

typedef void (RenderDroid::AFRasterModule::*RdSetupFunc)(RdActiveFace *face, RdMatCel *cel);
typedef void (RenderDroid::AFRasterModule::*RdSetupLightFunc)(RdActiveFace *face, RdMatCel *cel, float light);

RenderDroid::ActiveModule::ActiveModule(RenderDroid *rd) : Module(rd), _drawnFaces(0), _numActiveEdges(0), _numActiveFaces(0), _numActiveSpans(0), _yCurScanLine(0), _yMaxEdge(0), _yMinEdge(0) {
	_activeEdges = new RdActiveEdge[4096]();
	_activeFaces = new RdActiveFace[1024]();
	_activeSpans = new RdActiveSpan[2048]();
	_activeEdgeHead = new RdActiveEdge();
	_activeEdgeTail = new RdActiveEdge();
	memset(_newActiveEdges, 0, sizeof(_newActiveEdges));
	memset(_removeActiveEdges, 0, sizeof(_removeActiveEdges));
}

RenderDroid::ActiveModule::~ActiveModule() {
	delete[] _activeEdges;
	delete[] _activeFaces;
	delete[] _activeSpans;
	delete _activeEdgeHead;
	delete _activeEdgeTail;
}

int RenderDroid::ActiveModule::startup() {
	return 1;
}

int RenderDroid::ActiveModule::advanceFrame() {
	RdCanvas *canvas = _rd->_camera.getCurrent()->canvas;
	int yStart = canvas->yStart;
	int numLines = canvas->yEnd - yStart + 1;

	if (yStart < 0 || numLines < 0 || yStart + numLines > 1024) {
		warning("RenderDroid::ActiveModule::advanceFrame(): canvas lines %d..%d out of range", yStart, canvas->yEnd);
		yStart = 0;
		numLines = 0;
	}

	_activeEdgeHead->x = -65536;
	_activeEdgeHead->next = _activeEdgeTail;
	_activeEdgeTail->prev = _activeEdgeHead;
	memset(&_newActiveEdges[yStart], 0, sizeof(RdActiveEdge *) * numLines);
	memset(&_removeActiveEdges[yStart], 0, sizeof(RdActiveEdge *) * numLines);
	_yMinEdge = 0x7FFFFFFF;
	_yMaxEdge = 0;
	_activeEdgeHead->prev = nullptr;
	_activeEdgeTail->x = 0x7FFFFFFF;
	_activeEdgeTail->next = nullptr;
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
		RdActiveEdge *next = _activeEdgeHead->next;

		do {
			RdActiveEdge *newEdge = _newActiveEdges[_yCurScanLine];

			while (newEdge) {
				int x = newEdge->x;
				RdActiveEdge *following = newEdge->next;

				while (x > next->x)
					next = next->next;

				newEdge->next = next;
				newEdge->prev = next->prev;
				next->prev->next = newEdge;
				next->prev = newEdge;

				if (newEdge->left)
					newEdge->face->leftEdge = newEdge;
				else
					newEdge->face->rightEdge = newEdge;

				newEdge = following;
			}

			buildSpans();

			for (RdActiveEdge *remove = _removeActiveEdges[_yCurScanLine]; remove; remove = remove->nextRemove) {
				remove->next->prev = remove->prev;
				remove->prev->next = remove->next;
			}

			next = _activeEdgeHead->next;

			RdActiveEdge *edge = _activeEdgeHead->next;

			while (edge != _activeEdgeTail) {
				(_rd->_afRaster.*edge->step)(edge);

				RdActiveEdge *prev = edge->prev;

				if (edge->x >= prev->x) {
					edge = edge->next;
				} else {
					RdActiveEdge *after = edge->next;

					after->prev = prev;
					edge->prev->next = after;

					int x = edge->x;
					RdActiveEdge *search = edge->prev->prev;

					while (x < search->x)
						search = search->prev;

					RdActiveEdge *insertBefore = search->next;

					edge->prev = search;
					edge->next = insertBefore;
					insertBefore->prev = edge;
					search->next = edge;
					edge = after;
				}
			}

			next = _activeEdgeHead->next;
			_yCurScanLine++;
		} while (_yCurScanLine <= _yMaxEdge);
	}

	return flushSpans();
}

uint RenderDroid::ActiveModule::flushSpans() {
	for (uint i = 0; i < _numActiveFaces; i++) {
		RdActiveFace *face = &_activeFaces[i];

		if (face->firstSpan) {
			(_rd->_afRaster.*face->drawSpans)(face);
			face->firstSpan = nullptr;
			face->lastSpan = nullptr;
		}
	}

	_numActiveSpans = 0;

	return _numActiveFaces;
}

uint RenderDroid::ActiveModule::buildEdges() {
	RdProcFace *procFaces = _rd->_cache.getProcFaces();
	size_t numProcFaces = _rd->_cache.getNumProcFaces();

	for (size_t i = 0; i < numProcFaces; i++)
		addActiveFace(&procFaces[i]);

	RdActiveEdge *edge = _activeEdges;

	for (uint f = 0; f < _numActiveFaces; f++) {
		RdActiveFace *face = &_activeFaces[f];
		RdProcFace *procFace = face->procFace;
		uint numVertices = procFace->numVertices;

		for (uint v = 0; v < numVertices; v++) {
			int next = numVertices - 1 >= v + 1 ? v + 1 : 0;
			int result;

			if (_numActiveEdges >= 4096) {
				warning("RenderDroid::ActiveModule::buildEdges(): too many edges");
				return f;
			}

			if (face->procFace->backface)
				result = (_rd->_afRaster.*face->setupEdge)(edge, face, next, v);
			else
				result = (_rd->_afRaster.*face->setupEdge)(edge, face, v, next);

			if (!result)
				continue;

			int yStart = edge->yStart;
			uint yEnd = edge->numLines + yStart - 1;

			if (yStart < 0 || yStart >= 1024 || yEnd >= 1024) {
				warning("RenderDroid::ActiveModule::buildEdges(): edge lines %d..%u out of range", yStart, yEnd);
				continue;
			}

			edge->face = face;

			RdActiveEdge *list = _newActiveEdges[yStart];

			if (list && edge->x > list->x) {
				int x = edge->x;

				for (RdActiveEdge *search = list->next; search; search = search->next) {
					if (search->x >= x)
						break;

					list = search;
				}

				edge->next = list->next;
				list->next = edge;
			} else {
				edge->next = list;
				_newActiveEdges[yStart] = edge;
			}

			edge->nextRemove = _removeActiveEdges[yEnd];
			_removeActiveEdges[yEnd] = edge;

			if ((uint)yStart < _yMinEdge)
				_yMinEdge = yStart;

			if (yEnd > _yMaxEdge)
				_yMaxEdge = yEnd;

			edge++;
			_numActiveEdges++;
		}
	}

	return _numActiveFaces;
}

void RenderDroid::ActiveModule::addSpan(RdActiveFace *face, uint alignedEndX) {
	int width = (int)(alignedEndX - face->curX) >> 16;

	if (width <= 0)
		return;

	if (_numActiveSpans >= 0x800)
		flushSpans();

	RdActiveSpan *span = &_activeSpans[_numActiveSpans];
	RdActiveEdge *left = face->leftEdge;
	RdActiveEdge *right = face->rightEdge;

	_numActiveSpans++;

	span->x = face->curX >> 16;
	span->width = width;
	span->y = _yCurScanLine;
	span->next = nullptr;

	uint edgeWidth = (alignEdgeX(right->x) - alignEdgeX(left->x)) >> 16;
	int offset = face->curX - left->x;
	float fraction = (float)((double)offset * (1.0 / 65536.0));

	if (edgeWidth >= 2048) {
		warning("RenderDroid::ActiveModule::addSpan(): edge width %u out of range", edgeWidth);
		edgeWidth = 0;
	}

	span->oneOverZ = (float)(face->dOneOverZdX * fraction + left->oneOverZ);

	if (face->procFace->geometryMode == 4) {
		int textureMode = face->procFace->textureMode;

		if (textureMode) {
			if (textureMode == 1) {
				span->uOverZ = (float)(face->dUOverZdX * fraction + left->uOverZ);
				span->vOverZ = (float)(face->dVOverZdX * fraction + left->vOverZ);
			}
		} else {
			int64 oneOverN = _rd->_raster.getOneOverNFixed(edgeWidth);
			int du = (int)((uint64)(oneOverN * (int64)(right->u - left->u)) >> 16);
			int dv;

			span->du = du >> face->mipLevel;
			span->u = (int)(((uint64)((int64)du * offset) >> 16) + left->u) >> face->mipLevel;
			dv = (int)((uint64)(oneOverN * (int64)(right->v - left->v)) >> 16);
			span->dv = dv >> face->mipLevel;
			span->v = (int)(((uint64)((int64)dv * offset) >> 16) + left->v) >> face->mipLevel;
		}
	} else {
		span->u = face->colorIndex;
	}

	if (face->procFace->lightingMode == 3) {
		uint dIntensity = (uint)((uint64)((int64)_rd->_raster.getOneOverNFixed(edgeWidth) * (int64)(right->intensity - left->intensity)) >> 16);

		span->dIntensity = dIntensity;

		if (dIntensity > 0xFFFFFAE2)
			span->dIntensity = 0;

		span->intensity = (int)(((uint64)((int64)span->dIntensity * offset) >> 16) + left->intensity);
	}

	if (face->firstSpan)
		face->lastSpan->next = span;
	else
		face->firstSpan = span;

	face->lastSpan = span;
}

int RenderDroid::ActiveModule::buildSpans() {
	if (!_numActiveEdges)
		return 0;

	RdActiveEdge *edge = _activeEdgeHead->next;
	RdActiveFace *top = nullptr;

	while (edge != _activeEdgeTail) {
		RdActiveFace *face = edge->face;

		if (!edge->left) {
			face->activeEdgeCount--;

			if (!face->activeEdgeCount) {
				if (face == top) {
					uint alignedX = alignEdgeX(edge->x);

					addSpan(top, alignedX);

					top = top->nextFace;

					if (top) {
						top->curX = alignedX;
						top->prevFace = nullptr;
					}
				} else {
					if (face->nextFace)
						face->nextFace->prevFace = face->prevFace;

					if (face->prevFace)
						face->prevFace->nextFace = face->nextFace;
				}
			}
		} else {
			face->activeEdgeCount++;

			if (face->activeEdgeCount == 1) {
				if (face->procFace->geometryMode != 4 || face->procFace->textureMode != 1) {
					RdActiveEdge *rightEdge = face->rightEdge;
					RdActiveEdge *leftEdge = face->leftEdge;
					uint edgeWidth = (alignEdgeX(rightEdge->x) - alignEdgeX(leftEdge->x)) >> 16;

					if (edgeWidth >= 2048) {
						warning("RenderDroid::ActiveModule::buildSpans(): edge width %u out of range", edgeWidth);
						edgeWidth = 0;
					}

					double step = ((double)rightEdge->oneOverZ - leftEdge->oneOverZ) * _rd->_raster.getOneOverNFlex(edgeWidth);

					face->dOneOverZdX = (float)step;
					face->dOneOverZdXFixed = (int)floor((double)(float)(step * 65536.0));
				}

				if (!top) {
					face->prevFace = nullptr;
					face->nextFace = nullptr;
					top = face;
					face->curX = alignEdgeX(edge->x);
				} else {
					uint userData = face->procFace->userData;
					uint topUserData = top->procFace->userData;
					bool inFront;

					if ((uint16)userData != (uint16)topUserData || (userData >> 16) == (topUserData >> 16)) {
						double topDepth = (double)(edge->x - top->leftEdge->x) * (1.0 / 65536.0) * top->dOneOverZdX + top->leftEdge->oneOverZ;

						inFront = edge->oneOverZ > topDepth || (edge->oneOverZ == topDepth && face->dOneOverZdX > (double)top->dOneOverZdX);
					} else {
						inFront = (userData >> 16) > (topUserData >> 16);
					}

					if (inFront) {
						uint alignedX = alignEdgeX(edge->x);

						addSpan(top, alignedX);
						face->nextFace = top;
						top->prevFace = face;
						face->prevFace = nullptr;
						top = face;
						face->curX = alignedX;
					} else {
						RdActiveFace *nextFace = top->nextFace;
						RdActiveFace *prevFace = top;
						float edgeDepth = edge->oneOverZ;

						while (nextFace) {
							uint nextUserData = nextFace->procFace->userData;
							bool front;

							if ((uint16)userData == (uint16)nextUserData && (userData >> 16) != (nextUserData >> 16)) {
								front = (userData >> 16) > (nextUserData >> 16);
							} else {
								double nextDepth = (double)(edge->x - nextFace->leftEdge->x) * (1.0 / 65536.0) * nextFace->dOneOverZdX + nextFace->leftEdge->oneOverZ;

								front = edgeDepth > nextDepth || (edgeDepth == nextDepth && face->dOneOverZdX > (double)nextFace->dOneOverZdX);
							}

							if (front)
								break;

							prevFace = nextFace;
							nextFace = nextFace->nextFace;
						}

						if (nextFace) {
							face->prevFace = prevFace;
							prevFace->nextFace = face;
							nextFace->prevFace = face;
							face->nextFace = nextFace;
						} else {
							prevFace->nextFace = face;
							face->prevFace = prevFace;
							face->nextFace = nullptr;
						}
					}
				}
			}
		}

		edge = edge->next;
	}

	return 0;
}

int RenderDroid::ActiveModule::addActiveFace(RdProcFace *procFace) {
	if (_numActiveFaces >= 0x400)
		return 0;

	int geometryMode = procFace->geometryMode;

	if (geometryMode >= _rd->_curGeometryMode)
		geometryMode = _rd->_curGeometryMode;

	int lightingMode = procFace->lightingMode;

	if (lightingMode >= _rd->_curLightingMode)
		lightingMode = _rd->_curLightingMode;

	int textureMode = procFace->textureMode;

	if (textureMode >= _rd->_curTextureMode)
		textureMode = _rd->_curTextureMode;

	if (!geometryMode)
		return 0;

	RdActiveFace *face = &_activeFaces[_numActiveFaces];

	face->procFace = procFace;
	face->firstSpan = nullptr;
	face->lastSpan = nullptr;
	face->leftEdge = nullptr;
	face->rightEdge = nullptr;
	face->activeEdgeCount = 0;

	RdMatCel *cel = _rd->_cache.selectCel(procFace);

	if (!cel && !(procFace->flags & 1)) {
		warning("RenderDroid::ActiveModule::addActiveFace(): face without a material cel");
		return 0;
	}

	int effectiveMode = geometryMode;

	if (cel && effectiveMode == 4 && !(cel->flags & 8))
		effectiveMode = 3;

	float ambientLight = 0.0f;

	if (_rd->_curRenderOptions & 2)
		ambientLight = procFace->ambientLight;

	if (effectiveMode == 2) {
		if ((uint)lightingMode <= 4) {
			if (procFace->flags & 1)
				procFace->userSetup(face, cel, 0.0f);
			else if (_rd->_curZBufferMethod == 1)
				_rd->_afRaster.setupNGonLW(face, cel);
		}
	} else if (effectiveMode == 3 || effectiveMode == 4) {
		RdSetupFunc none = nullptr;
		RdSetupLightFunc lightFunction = nullptr;
		RdSetupFunc vertex = nullptr;

		if (effectiveMode == 3) {
			none = &RenderDroid::AFRasterModule::setupNGonLS;
			lightFunction = &RenderDroid::AFRasterModule::setupNGonFS;
			vertex = &RenderDroid::AFRasterModule::setupNGonGS;
		} else if (cel->texture->flags & 1) {
			if (textureMode == 0) {
				none = &RenderDroid::AFRasterModule::setupNGonMLAT;
				lightFunction = &RenderDroid::AFRasterModule::setupNGonMFAT;
				vertex = &RenderDroid::AFRasterModule::setupNGonMGAT;
			} else if (textureMode > 0 && textureMode <= 2) {
				none = &RenderDroid::AFRasterModule::setupNGonMLIT;
				lightFunction = &RenderDroid::AFRasterModule::setupNGonMFIT;
				vertex = &RenderDroid::AFRasterModule::setupNGonMGIT;
			}
		} else {
			if (textureMode == 0) {
				none = &RenderDroid::AFRasterModule::setupNGonLAT;
				lightFunction = &RenderDroid::AFRasterModule::setupNGonFAT;
				vertex = &RenderDroid::AFRasterModule::setupNGonGAT;
			} else if (textureMode > 0 && textureMode <= 2) {
				none = &RenderDroid::AFRasterModule::setupNGonLIT;
				lightFunction = &RenderDroid::AFRasterModule::setupNGonFIT;
				vertex = &RenderDroid::AFRasterModule::setupNGonGIT;
			}
		}

		if (none) {
			float light;

			switch (lightingMode) {
			case 0:
				if (procFace->flags & 1)
					procFace->userSetup(face, cel, 0.0f);
				else if (_rd->_curZBufferMethod == 1)
					(_rd->_afRaster.*none)(face, cel);
				break;
			case 1:
			case 2:
				light = _rd->_cache.calcFaceLight(procFace, lightingMode, ambientLight);
				procFace->diffuseLight = light;

				if (procFace->flags & 1)
					procFace->userSetup(face, cel, light);
				else if (_rd->_curZBufferMethod == 1)
					(_rd->_afRaster.*lightFunction)(face, cel, light);
				break;
			case 3:
				_rd->_cache.calcVertexLights(procFace, ambientLight);

				if (procFace->flags & 1)
					procFace->userSetup(face, cel, 0.0f);
				else if (_rd->_curZBufferMethod == 1)
					(_rd->_afRaster.*vertex)(face, cel);
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
