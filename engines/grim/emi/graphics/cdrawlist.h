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

#ifndef GRIM_EMI_CDRAWLIST_H
#define GRIM_EMI_CDRAWLIST_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Grim {

class CDrawContext;
class CTexture;

struct DrawVertex {
	DrawVertex();

	float x;
	float y;
	float z;
	uint32 color;
	float u;
	float v;
	float reserved24;
	float reserved28;
	int outcode;
	uint32 flags;
};

struct DrawPoly {
	enum {
		kMaxVerts = 16
	};

	byte inUse;
	uint32 flags;
	int reserved8;
	int numVerts;
	DrawVertex *verts[kMaxVerts];
	CTexture *tex;
	float zMin;
	byte reserved88;
};

class CDrawList {
public:
	enum {
		kSortThreshold = 16
	};

	CDrawList(CDrawContext *owner, int listCapacity);
	~CDrawList();

	void reset();
	void sort();
	DrawPoly *newPoly(int numVerts);
	void finishImmediatePoly(DrawPoly *poly);
	void recyclePoly(DrawPoly *poly);
	bool addPoly(DrawPoly *poly);
	DrawVertex *allocVertex();

	static bool drawPolyLess(const DrawPoly *first, const DrawPoly *second);

	DrawPoly **_polys;
	DrawPoly *_pool;
	int _capacity;
	int _count;
	bool _polyPending;
	int _deferAll;
	CDrawContext *_ctx;

private:
	static void sortRange(DrawPoly **first, DrawPoly **last);
};

} // End of namespace Grim

#endif
