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
#include "common/textconsole.h"

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/graphics/cdrawlist.h"
#include "engines/grim/emi/graphics/dimregion.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

static const int kDimPolyFlags = 67604;
static const int kDimPolyVertices = 4;
static const int kDimDrawListIndex = 1;

DimRegion::DimRegion() {
	_prev = nullptr;
	_next = nullptr;
	_alpha = 0.0f;
	_id = 0;
}

void *DimRegion::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void DimRegion::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void DimRegion::undimAll(EmiEngine *engine) {
	while (engine->_dimRegionCount) {
		DimRegion *region = engine->_dimRegionHead;

		if (!region) {
			warning("DimRegion::undimAll(): the region count is not zero but the list is empty");
			engine->_dimRegionCount = 0;

			return;
		}

		engine->_dimRegionHead = region->_next;

		if (engine->_dimRegionHead)
			engine->_dimRegionHead->_prev = nullptr;
		else
			engine->_dimRegionTail = nullptr;

		region->_prev = nullptr;
		region->_next = nullptr;
		engine->_dimRegionCount--;

		delete region;
	}
}

int DimRegion::drawDimRegions(EmiEngine *engine, CDrawContext *drawContext) {
	drawContext->setOrtho(-1.0f, 1.0f, -1.0f, 1.0f);

	DimRegion *region = engine->_dimRegionHead;

	while (region) {
		double scaledAlpha = (double)region->_alpha * 255.0;
		byte alphaByte = 0;

		if (scaledAlpha > -9.0e18 && scaledAlpha < 9.0e18)
			alphaByte = (byte)(int64)scaledAlpha;
		else
			warning("DimRegion::drawDimRegions(): the alpha is out of range");

		DrawPoly *poly = drawContext->getDrawList(kDimDrawListIndex)->newPoly(kDimPolyVertices);

		poly->flags = kDimPolyFlags;

		float left = region->_rect.getX();
		float right = region->_rect.getX();

		right = (float)((double)region->_rect.getWidth() + (double)right);

		float top = region->_rect.getY();
		float bottom = region->_rect.getY();
		double height = region->_rect.getHeight();

		poly->verts[0]->x = left;
		poly->verts[0]->y = top;
		poly->verts[0]->z = 1.0f;
		poly->verts[1]->x = right;
		poly->verts[1]->y = top;
		poly->verts[1]->z = 1.0f;

		float bottomEdge = (float)(height + (double)bottom);

		poly->verts[2]->x = right;
		poly->verts[2]->y = bottomEdge;
		poly->verts[2]->z = 1.0f;
		poly->verts[3]->x = left;
		poly->verts[3]->y = bottomEdge;
		poly->verts[3]->z = 1.0f;

		for (int i = 0; i < kDimPolyVertices; i++)
			poly->verts[i]->color = (uint32)alphaByte << 24;

		drawContext->getDrawList(kDimDrawListIndex)->addPoly(poly);

		region = region->_next;
	}

	return drawContext->flushDrawLists();
}

int DimRegion::dimScreen(EmiEngine *engine, int reserved, float alpha) {
	FRect screen;

	screen._x = -1.0f;
	screen._y = 1.0f;
	screen._w = 2.0f;
	screen._h = -2.0f;

	return dimRegion(engine, &screen, alpha);
}

int DimRegion::dimRegion(EmiEngine *engine, FRect *region, float alpha) {
	if (!region) {
		warning("DimRegion::dimRegion(): missing rectangle");

		return -1;
	}

	DimRegion *newRegion = new DimRegion();

	newRegion->_rect._x = region->_x;
	newRegion->_rect._y = region->_y;
	newRegion->_rect._w = region->_w;
	newRegion->_rect._h = region->_h;
	newRegion->_alpha = alpha;
	newRegion->_id = engine->_nextDimRegionId;
	engine->_nextDimRegionId++;

	if (engine->_dimRegionTail)
		engine->_dimRegionTail->_next = newRegion;

	newRegion->_prev = engine->_dimRegionTail;
	newRegion->_next = nullptr;
	engine->_dimRegionTail = newRegion;

	if (!engine->_dimRegionHead)
		engine->_dimRegionHead = newRegion;

	engine->_dimRegionCount++;

	return newRegion->_id;
}

bool DimRegion::undimRegion(EmiEngine *engine, int regionId) {
	DimRegion *region = engine->_dimRegionHead;

	if (!region)
		return false;

	while (region->_id != regionId) {
		region = region->_next;

		if (!region)
			return false;
	}

	if (region == engine->_dimRegionHead) {
		engine->_dimRegionHead = region->_next;

		if (engine->_dimRegionHead)
			engine->_dimRegionHead->_prev = nullptr;
		else
			engine->_dimRegionTail = nullptr;

		region->_prev = nullptr;
		region->_next = nullptr;
		engine->_dimRegionCount--;
	} else if (region == engine->_dimRegionTail) {
		engine->_dimRegionTail = region->_prev;

		if (engine->_dimRegionTail)
			engine->_dimRegionTail->_next = nullptr;
		else
			engine->_dimRegionHead = nullptr;

		region->_prev = nullptr;
		region->_next = nullptr;
		engine->_dimRegionCount--;
	} else if (region->_next || region->_prev) {
		region->_next->_prev = region->_prev;
		region->_prev->_next = region->_next;
		region->_prev = nullptr;
		region->_next = nullptr;
		engine->_dimRegionCount--;
	}

	delete region;

	return true;
}

} // End of namespace Grim
