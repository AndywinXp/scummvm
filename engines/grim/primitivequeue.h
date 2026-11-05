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

#ifndef GRIM_PRIMITIVEQUEUE_H
#define GRIM_PRIMITIVEQUEUE_H

#include "common/scummsys.h"

namespace Grim {

class RenderDroid;
struct RdCanvas;

enum SputPrimitiveShape {
	kSputPrimitiveLine = 0,
	kSputPrimitiveRect = 1,
	kSputPrimitivePolygon = 2
};

struct SputPrimitiveObject {
	SputPrimitiveObject(int id, int shape);

	SputPrimitiveObject *pNext;
	SputPrimitiveObject *pPrev;
	int shape;
	int id;
	int layer;
	int x1;
	int y1;
	int x2;
	int y2;
	int color;
	int pattern;
	int bFilled;
};

class PrimitiveQueue {
public:
	explicit PrimitiveQueue(RenderDroid *pRd);

	void purge();
	int addLine(int x1, int y1, int x2, int y2, int color, int layer);
	int addRect(int x1, int y1, int x2, int y2, int color, int bFilled, int layer);
	int addPolygon(int numPoints, int *pPoints, int color, int layer);
	SputPrimitiveObject *addObject(int id, int layer);
	void deletePrimitive(int id);
	void changeColor(int id, int color);
	void changeLayer(int id, int layer);
	void move(int id, int dx, int dy);
	void moveTo(int id, int x, int y);
	void moveEndpoint(int id, int x2, int y2);
	void changeDimensions(int id, int width, int height);
	void draw(RdCanvas *pCanvas, int layerMask);
	void drawPrimitive(SputPrimitiveObject *pObj, RdCanvas *pCanvas);
	void markAllDirty(int layerMask);
	void markDirty(SputPrimitiveObject *pObj);
	void resetId();
	SputPrimitiveObject *findPrimitive(int id, SputPrimitiveObject *pPrev);

	SputPrimitiveObject *getHead() const { return _pHead; }

private:
	friend class SputPrimitive;

	RenderDroid *_pRd;
	SputPrimitiveObject *_pHead;
	SputPrimitiveObject *_pTail;
	int _nextId;
};

} // End of namespace Grim

#endif
