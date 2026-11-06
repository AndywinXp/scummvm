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

#include "common/util.h"

#include "engines/grim/primitivequeue.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/std/std_display.h"

namespace Grim {

SputPrimitiveObject::SputPrimitiveObject(int id, int shape) : pNext(nullptr), pPrev(nullptr), shape(shape), id(id),
		layer(2), x1(0), y1(0), x2(0), y2(0), color(0), pattern(-1), bFilled(0) {
}

PrimitiveQueue::PrimitiveQueue(RenderDroid *pRd) : _pRd(pRd), _pSputRender(nullptr), _pHead(nullptr), _pTail(nullptr), _nextId(1) {
}

void PrimitiveQueue::purge() {
	if (_pHead) {
		SputPrimitiveObject *pNext;

		do {
			pNext = _pHead->pNext;
			markDirty(_pHead);
			delete _pHead;
			_pHead = pNext;
		} while (pNext);
	}

	_pTail = nullptr;
}

int PrimitiveQueue::addLine(int x1, int y1, int x2, int y2, int color, int layer) {
	int id = _nextId++;
	SputPrimitiveObject *pObj = addObject(id, layer);

	pObj->x1 = x1;
	pObj->y1 = y1;
	pObj->x2 = x2;
	pObj->y2 = y2;
	pObj->color = color;

	return pObj->id;
}

int PrimitiveQueue::addRect(int x1, int y1, int x2, int y2, int color, int bFilled, int layer) {
	int id = _nextId++;
	SputPrimitiveObject *pObj = addObject(id, layer);

	pObj->shape = kSputPrimitiveRect;
	pObj->x1 = x1;
	pObj->y1 = y1;
	pObj->x2 = x2;
	pObj->y2 = y2;
	pObj->color = color;
	pObj->bFilled = bFilled;

	return pObj->id;
}

int PrimitiveQueue::addPolygon(int numPoints, int *pPoints, int color, int layer) {
	int id = _nextId++;

	for (int i = 0; i < numPoints - 1; i++) {
		SputPrimitiveObject *pObj = addObject(id, layer);

		pObj->shape = kSputPrimitivePolygon;
		pObj->x1 = pPoints[i * 2];
		pObj->y1 = pPoints[i * 2 + 1];
		pObj->x2 = pPoints[i * 2 + 2];
		pObj->y2 = pPoints[i * 2 + 3];
		pObj->color = color;
	}

	return id;
}

SputPrimitiveObject *PrimitiveQueue::addObject(int id, int layer) {
	if (_pTail) {
		SputPrimitiveObject *pObj = new SputPrimitiveObject(id, kSputPrimitiveLine);

		_pTail->pNext = pObj;
		_pTail->pNext->pPrev = _pTail;
		_pTail = _pTail->pNext;
	} else {
		SputPrimitiveObject *pObj = new SputPrimitiveObject(id, kSputPrimitiveLine);

		_pTail = pObj;
		_pHead = pObj;
	}

	if (layer != 2 && layer != 1)
		layer = 2;

	_pTail->layer = layer;

	return _pTail;
}

void PrimitiveQueue::deletePrimitive(int id) {
	SputPrimitiveObject *pObj = _pHead;

	while (pObj) {
		if (pObj->id == id) {
			SputPrimitiveObject *pPrev = pObj->pPrev;

			if (pPrev)
				pPrev->pNext = pObj->pNext;

			if (pObj->pNext)
				pObj->pNext->pPrev = pObj->pPrev;

			if (_pTail == pObj)
				_pTail = pObj->pPrev;

			if (pObj == _pHead)
				_pHead = _pHead->pNext;

			SputPrimitiveObject *pNext = pObj->pNext;

			markDirty(pObj);
			delete pObj;
			pObj = pNext;
		} else {
			pObj = pObj->pNext;
		}
	}
}

void PrimitiveQueue::changeColor(int id, int color) {
	for (SputPrimitiveObject *pObj = findPrimitive(id, nullptr); pObj; pObj = findPrimitive(id, pObj))
		pObj->color = color;
}

void PrimitiveQueue::changeLayer(int id, int layer) {
	for (SputPrimitiveObject *pObj = findPrimitive(id, nullptr); pObj; pObj = findPrimitive(id, pObj)) {
		markDirty(pObj);

		if (layer != 2 && layer != 1)
			layer = 2;

		pObj->layer = layer;
	}
}

void PrimitiveQueue::move(int id, int dx, int dy) {
	for (SputPrimitiveObject *pObj = findPrimitive(id, nullptr); pObj; pObj = findPrimitive(id, pObj)) {
		markDirty(pObj);

		pObj->x1 += dx;
		pObj->y2 += dy;
		pObj->y1 += dy;
		pObj->x2 += dx;
	}
}

void PrimitiveQueue::moveTo(int id, int x, int y) {
	int dx = 0;
	int dy = 0;
	SputPrimitiveObject *pObj = findPrimitive(id, nullptr);

	if (pObj) {
		markDirty(pObj);

		if (x != -1) {
			dx = x - pObj->x1;
			pObj->x1 = x;
		}

		if (y != -1) {
			dy = y - pObj->y1;
			pObj->y1 = y;
		}

		pObj->y2 += dy;
		pObj->x2 += dx;

		for (pObj = findPrimitive(id, pObj); pObj; pObj = findPrimitive(id, pObj)) {
			markDirty(pObj);

			pObj->x1 += dx;
			pObj->y1 += dy;
			pObj->x2 += dx;
			pObj->y2 += dy;
		}
	}
}

void PrimitiveQueue::moveEndpoint(int id, int x2, int y2) {
	SputPrimitiveObject *pObj = findPrimitive(id, nullptr);

	if (pObj) {
		markDirty(pObj);

		if (x2 != -1)
			pObj->x2 = x2;

		if (y2 != -1)
			pObj->y2 = y2;
	}
}

void PrimitiveQueue::changeDimensions(int id, int width, int height) {
	SputPrimitiveObject *pObj = findPrimitive(id, nullptr);

	if (pObj) {
		markDirty(pObj);

		if (width != -1)
			pObj->x2 = width + pObj->x1;

		if (height != -1)
			pObj->y2 = height + pObj->y1;
	}
}

void PrimitiveQueue::draw(RdCanvas *pCanvas, int layerMask) {
	for (SputPrimitiveObject *pObj = _pHead; pObj; pObj = pObj->pNext) {
		if (layerMask & pObj->layer)
			drawPrimitive(pObj, pCanvas);
	}
}

void PrimitiveQueue::drawPrimitive(SputPrimitiveObject *pObj, RdCanvas *pCanvas) {
	switch (pObj->shape) {
	case kSputPrimitiveLine:
	case kSputPrimitivePolygon:
		_pRd->_primit2.drawClippedLine(pCanvas, pObj->x1, pObj->y1, pObj->x2, pObj->y2, (uint16)pObj->color, (uint)pObj->pattern);
		break;
	case kSputPrimitiveRect:
		if (pObj->bFilled) {
			int left = MIN(pObj->x1, pObj->x2);
			int top = MIN(pObj->y1, pObj->y2);
			int right = MAX(pObj->x1, pObj->x2);
			int bottom = MAX(pObj->y1, pObj->y2);

			for (int y = top; y <= bottom; y++)
				_pRd->_primit2.drawClippedLine(pCanvas, left, y, right, y, (uint16)pObj->color, (uint)pObj->pattern);
		} else {
			_pRd->_primit2.drawRectangle(pCanvas, pObj->x1, pObj->y1, pObj->x2, pObj->y2, (uint16)pObj->color, (uint)pObj->pattern);
		}
		break;
	default:
		break;
	}
}

void PrimitiveQueue::markAllDirty(int layerMask) {
	for (SputPrimitiveObject *pObj = _pHead; pObj; pObj = pObj->pNext) {
		if (layerMask & pObj->layer)
			markDirty(pObj);
	}
}

void PrimitiveQueue::markDirty(SputPrimitiveObject *pObj) {
	StdRect rect;

	rect.x = MIN(pObj->x1, pObj->x2);
	rect.y = MIN(pObj->y1, pObj->y2);
	rect.width = MAX(pObj->x1, pObj->x2) - rect.x + 1;
	rect.height = MAX(pObj->y1, pObj->y2) - rect.y + 1;

	_pSputRender->markDirtyRect(&rect, 0);
}

void PrimitiveQueue::resetId() {
	_nextId = 1;
}

SputPrimitiveObject *PrimitiveQueue::findPrimitive(int id, SputPrimitiveObject *pPrev) {
	SputPrimitiveObject *pObj;

	if (pPrev)
		pObj = pPrev->pNext;
	else
		pObj = _pHead;

	for (; pObj; pObj = pObj->pNext) {
		if (pObj->id == id)
			break;
	}

	return pObj;
}

} // End of namespace Grim
