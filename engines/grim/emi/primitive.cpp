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

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/cdrawlist.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/primitive.h"

namespace Grim {

static const int kPrimitiveDrawFlags = 0x302;
static const int kLineVertices = 2;
static const int kRectangleVertices = 4;

Primitive::Primitive(int primitiveId, int primitiveType) : color(0u) {
	x1 = 0.0f;
	y1 = 0.0f;
	z1 = 0.0f;
	x2 = 0.0f;
	y2 = 0.0f;
	z2 = 0.0f;
	id = primitiveId;
	next = nullptr;
	prev = nullptr;
	type = primitiveType;
	reserved64 = -1;
	layer = kLayerSecond;
	polyCount = 0;
	filled = false;
}

void *Primitive::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void Primitive::operator delete(void *block) {
	gpMemoryServices->free(block);
}

void Primitive::drawNode(CDrawContext *drawContext) {
	drawContext->setColor(&color);

	switch (type) {
	case kTypeLine3D:
	case kTypeLine:
	case kTypePolygon: {
		DrawVertex vertices[kLineVertices];

		vertices[0].x = x1;
		vertices[0].y = y1;
		vertices[0].z = z1;
		vertices[1].x = x2;
		vertices[1].y = y2;
		vertices[1].z = z1;

		// The disasm uses the first depth for both ends
		if (type == kTypeLine3D)
			drawContext->drawPrim(kPrimitiveDrawFlags, vertices, kLineVertices, false);
		else
			drawContext->drawPrim2D(kPrimitiveDrawFlags, vertices, kLineVertices, false);

		break;
	}
	case kTypeRectangle: {
		float minX = !(x2 > x1) ? x2 : x1;
		float minY = !(y2 > y1) ? y2 : y1;
		float maxX = (x2 >= x1) ? x2 : x1;
		float maxY = (y2 >= y1) ? y2 : y1;
		DrawVertex vertices[kRectangleVertices];

		vertices[0].x = minX;
		vertices[0].y = minY;
		vertices[1].x = maxX;
		vertices[1].y = minY;
		vertices[2].x = maxX;
		vertices[2].y = maxY;
		vertices[3].x = minX;
		vertices[3].y = maxY;

		drawContext->drawPrim2D(kPrimitiveDrawFlags, vertices, kRectangleVertices, filled);

		break;
	}
	default:
		break;
	}
}

} // End of namespace Grim
