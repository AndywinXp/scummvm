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

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/cdrawlist.h"
#include "engines/grim/emi/ctexture.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

Common::Array<DrawVertex *> CDrawList::_vertexPool;
int CDrawList::_listCount = 0;

DrawVertex::DrawVertex() {
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	flags = 0;
	reserved24 = -1.0f;
	color = 0xFF000000;
	reserved28 = 0.0f;
	u = 0.0f;
	v = 0.0f;
}

CDrawList::CDrawList(int listCapacity) {
	polyPending = false;
	deferAll = 0;
	ctx = nullptr;
	polys = (DrawPoly **)gpMemoryServices->mallocImpl(listCapacity * sizeof(DrawPoly *));
	pool = (DrawPoly *)gpMemoryServices->mallocImpl((listCapacity + 1) * sizeof(DrawPoly));
	capacity = listCapacity;

	for (int i = 0; i < capacity; i++)
		polys[i] = &pool[i];

	reset();
	_listCount++;
}

CDrawList::~CDrawList() {
	for (int i = 0; i < count; i++)
		recyclePoly(polys[i]);

	gpMemoryServices->freeImpl(polys);
	polys = nullptr;
	gpMemoryServices->freeImpl(pool);
	pool = nullptr;

	_listCount--;

	if (_listCount == 0) {
		for (uint i = 0; i < _vertexPool.size(); i++) {
			gpMemoryServices->freeImpl(_vertexPool[i]);
			_vertexPool[i] = nullptr;
		}

		_vertexPool.clear();
	}
}

void CDrawList::reset() {
	count = 0;
	deferAll = 0;
	polyPending = false;
}

bool CDrawList::drawPolyLess(const DrawPoly *first, const DrawPoly *second) {
	uint32 firstTransparent = first->flags & 0x400;
	uint32 secondTransparent = second->flags & 0x400;

	if (firstTransparent < secondTransparent)
		return true;

	if (firstTransparent > secondTransparent)
		return false;

	if (firstTransparent)
		return (double)first->zMin > (double)second->zMin;

	return (uintptr)first->tex < (uintptr)second->tex;
}

void CDrawList::sortRange(DrawPoly **first, DrawPoly **last) {
	while (last - first > kSortThreshold) {
		DrawPoly *low = *first;
		DrawPoly *middle = first[(last - first) / 2];
		DrawPoly *high = last[-1];
		DrawPoly *pivot;

		if (drawPolyLess(low, middle)) {
			if (drawPolyLess(middle, high))
				pivot = middle;
			else if (drawPolyLess(low, high))
				pivot = high;
			else
				pivot = low;
		} else if (drawPolyLess(low, high)) {
			pivot = low;
		} else if (drawPolyLess(middle, high)) {
			pivot = high;
		} else {
			pivot = middle;
		}

		DrawPoly **left = first;
		DrawPoly **right = last;

		while (true) {
			while (drawPolyLess(*left, pivot))
				left++;

			right--;

			while (drawPolyLess(pivot, *right))
				right--;

			if (right <= left)
				break;

			DrawPoly *swapped = *left;
			*left = *right;
			*right = swapped;
			left++;
		}

		if (last - left > left - first) {
			sortRange(first, left);
			first = left;
		} else {
			sortRange(left, last);
			last = left;
		}
	}
}

void CDrawList::sort() {
	DrawPoly **first = polys;
	DrawPoly **last = polys + count;
	DrawPoly **limit = last;

	if (count > kSortThreshold) {
		sortRange(first, last);
		limit = first + kSortThreshold;
	}

	if (first == last)
		return;

	for (DrawPoly **current = first + 1; current != limit; ++current) {
		DrawPoly *value = *current;

		if (drawPolyLess(value, *first)) {
			for (DrawPoly **shifted = current; shifted != first; --shifted)
				*shifted = shifted[-1];

			*first = value;
		} else {
			DrawPoly **position = current;

			while (drawPolyLess(value, position[-1])) {
				*position = position[-1];
				position--;

				if (position == first) {
					warning("CDrawList::sort(): inconsistent polygon order");
					break;
				}
			}

			*position = value;
		}
	}

	for (DrawPoly **current = limit; current != last; ++current) {
		DrawPoly *value = *current;
		DrawPoly **position = current;

		while (drawPolyLess(value, position[-1])) {
			*position = position[-1];
			position--;

			if (position == first) {
				warning("CDrawList::sort(): inconsistent polygon order");
				break;
			}
		}

		*position = value;
	}
}

DrawPoly *CDrawList::newPoly(int numVerts) {
	DrawPoly *poly = polys[count];

	poly->flags = 4;
	poly->inUse = 0;
	poly->reserved88 = 0;
	poly->numVerts = 0;
	poly->tex = nullptr;
	poly->reserved8 = 0;

	if (numVerts > DrawPoly::kMaxVerts) {
		warning("CDrawList::newPoly(): too many vertices");
		numVerts = DrawPoly::kMaxVerts;
	}

	for (int i = 0; i < numVerts; i++) {
		poly->verts[poly->numVerts] = allocVertex();
		poly->numVerts++;
	}

	polyPending = true;

	return poly;
}

void CDrawList::finishImmediatePoly(DrawPoly *poly) {
	polyPending = false;
	recyclePoly(poly);
}

void CDrawList::recyclePoly(DrawPoly *poly) {
	poly->inUse = 0;

	while (poly->numVerts != 0) {
		DrawVertex *vertex = poly->verts[poly->numVerts - 1];

		if (!(vertex->flags & 1))
			_vertexPool.push_back(vertex);

		poly->numVerts--;
	}
}

bool CDrawList::addPoly(DrawPoly *poly) {
	CTexture *texture = poly->tex;
	int numVerts = poly->numVerts;

	if (texture && (texture->flags & 0x480))
		poly->flags |= 0x10400;

	if (poly->flags & 0x800)
		poly->flags |= 0x10400;

	uint32 transparent = poly->flags & 0x400;

	if (transparent) {
		if (numVerts <= 0) {
			warning("CDrawList::addPoly(): transparent polygon without vertices");
			poly->zMin = 0.0f;
		} else {
			poly->zMin = poly->verts[0]->z;

			for (int i = 1; i < numVerts; i++) {
				if ((double)poly->verts[i]->z < (double)poly->zMin)
					poly->zMin = poly->verts[i]->z;
			}
		}
	}

	if (deferAll || transparent) {
		polyPending = false;
		poly->inUse = 1;

		if (count >= capacity - 1) {
			if (gpConsoleServices)
				gpConsoleServices->logStatus("Draw List overflow!\n");
			else
				warning("CDrawList::addPoly(): draw list overflow");

			ctx->flushDrawLists();
		} else {
			count++;
		}

		return true;
	}

	polyPending = true;
	ctx->outputDrawPoly(poly);
	finishImmediatePoly(poly);

	return true;
}

DrawVertex *CDrawList::allocVertex() {
	DrawVertex *vertex;

	if (!_vertexPool.empty()) {
		vertex = _vertexPool.back();
		_vertexPool.pop_back();
	} else {
		vertex = (DrawVertex *)gpMemoryServices->mallocImpl(sizeof(DrawVertex));

		if (!vertex)
			return nullptr;

		new (vertex) DrawVertex();
	}

	vertex->flags &= 0xFFFFFFFC;
	vertex->outcode = 0;

	return vertex;
}

} // End of namespace Grim
