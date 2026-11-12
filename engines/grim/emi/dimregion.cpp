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

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/cdrawlist.h"
#include "engines/grim/emi/dimregion.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const int kDimPolyFlags = 67604;
static const int kDimPolyVertices = 4;
static const int kDimDrawListIndex = 1;

DimRegion::DimRegion() {
	prev = nullptr;
	next = nullptr;
	alpha = 0.0f;
	id = 0;
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

		engine->_dimRegionHead = region->next;

		if (engine->_dimRegionHead)
			engine->_dimRegionHead->prev = nullptr;
		else
			engine->_dimRegionTail = nullptr;

		region->prev = nullptr;
		region->next = nullptr;
		engine->_dimRegionCount--;

		delete region;
	}
}

int DimRegion::drawDimRegions(EmiEngine *engine, CDrawContext *drawContext) {
	drawContext->setOrtho(-1.0f, 1.0f, -1.0f, 1.0f);

	DimRegion *region = engine->_dimRegionHead;

	while (region) {
		double scaledAlpha = (double)region->alpha * 255.0;
		byte alphaByte = 0;

		if (scaledAlpha > -9.0e18 && scaledAlpha < 9.0e18)
			alphaByte = (byte)(int64)scaledAlpha;
		else
			warning("DimRegion::drawDimRegions(): the alpha is out of range");

		DrawPoly *poly = drawContext->getDrawList(kDimDrawListIndex)->newPoly(kDimPolyVertices);

		poly->flags = kDimPolyFlags;

		float left = region->rect.getX();
		float right = region->rect.getX();

		right = (float)((double)region->rect.getWidth() + (double)right);

		float top = region->rect.getY();
		float bottom = region->rect.getY();
		double height = region->rect.getHeight();

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

		region = region->next;
	}

	return drawContext->flushDrawLists();
}

int DimRegion::dimScreen(EmiEngine *engine, int reserved, float alpha) {
	FRect screen;

	screen.x = -1.0f;
	screen.y = 1.0f;
	screen.w = 2.0f;
	screen.h = -2.0f;

	return dimRegion(engine, &screen, alpha);
}

int DimRegion::dimRegion(EmiEngine *engine, FRect *region, float alpha) {
	if (!region) {
		warning("DimRegion::dimRegion(): missing rectangle");

		return -1;
	}

	DimRegion *newRegion = new DimRegion();

	newRegion->rect.x = region->x;
	newRegion->rect.y = region->y;
	newRegion->rect.w = region->w;
	newRegion->rect.h = region->h;
	newRegion->alpha = alpha;
	newRegion->id = engine->_nextDimRegionId;
	engine->_nextDimRegionId++;

	if (engine->_dimRegionTail)
		engine->_dimRegionTail->next = newRegion;

	newRegion->prev = engine->_dimRegionTail;
	newRegion->next = nullptr;
	engine->_dimRegionTail = newRegion;

	if (!engine->_dimRegionHead)
		engine->_dimRegionHead = newRegion;

	engine->_dimRegionCount++;

	return newRegion->id;
}

bool DimRegion::undimRegion(EmiEngine *engine, int regionId) {
	DimRegion *region = engine->_dimRegionHead;

	if (!region)
		return false;

	while (region->id != regionId) {
		region = region->next;

		if (!region)
			return false;
	}

	if (region == engine->_dimRegionHead) {
		engine->_dimRegionHead = region->next;

		if (engine->_dimRegionHead)
			engine->_dimRegionHead->prev = nullptr;
		else
			engine->_dimRegionTail = nullptr;

		region->prev = nullptr;
		region->next = nullptr;
		engine->_dimRegionCount--;
	} else if (region == engine->_dimRegionTail) {
		engine->_dimRegionTail = region->prev;

		if (engine->_dimRegionTail)
			engine->_dimRegionTail->next = nullptr;
		else
			engine->_dimRegionHead = nullptr;

		region->prev = nullptr;
		region->next = nullptr;
		engine->_dimRegionCount--;
	} else if (region->next || region->prev) {
		region->next->prev = region->prev;
		region->prev->next = region->next;
		region->prev = nullptr;
		region->next = nullptr;
		engine->_dimRegionCount--;
	}

	delete region;

	return true;
}

} // End of namespace Grim
