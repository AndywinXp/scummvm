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

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/graphics/cdrawlist.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/graphics/primitive.h"

namespace Grim {

static const int kPrimitiveDrawFlags = 0x302;
static const int kLineVertices = 2;
static const int kRectangleVertices = 4;

Primitive::Primitive(int primitiveId, int primitiveType) : _color(0u) {
	_x1 = 0.0f;
	_y1 = 0.0f;
	_z1 = 0.0f;
	_x2 = 0.0f;
	_y2 = 0.0f;
	_z2 = 0.0f;
	_id = primitiveId;
	_next = nullptr;
	_prev = nullptr;
	_type = primitiveType;
	_reserved64 = -1;
	_layer = kLayerSecond;
	_polyCount = 0;
	_filled = false;
}

void *Primitive::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void Primitive::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void Primitive::drawNode(CDrawContext *drawContext) {
	drawContext->setColor(&_color);

	switch (_type) {
	case kTypeLine3D:
	case kTypeLine:
	case kTypePolygon: {
		DrawVertex vertices[kLineVertices];

		vertices[0].x = _x1;
		vertices[0].y = _y1;
		vertices[0].z = _z1;
		vertices[1].x = _x2;
		vertices[1].y = _y2;
		vertices[1].z = _z1;

		// The disasm uses the first depth for both ends
		if (_type == kTypeLine3D)
			drawContext->drawPrim(kPrimitiveDrawFlags, vertices, kLineVertices, false);
		else
			drawContext->drawPrim2D(kPrimitiveDrawFlags, vertices, kLineVertices, false);

		break;
	}
	case kTypeRectangle: {
		float minX = !(_x2 > _x1) ? _x2 : _x1;
		float minY = !(_y2 > _y1) ? _y2 : _y1;
		float maxX = (_x2 >= _x1) ? _x2 : _x1;
		float maxY = (_y2 >= _y1) ? _y2 : _y1;
		DrawVertex vertices[kRectangleVertices];

		vertices[0].x = minX;
		vertices[0].y = minY;
		vertices[1].x = maxX;
		vertices[1].y = minY;
		vertices[2].x = maxX;
		vertices[2].y = maxY;
		vertices[3].x = minX;
		vertices[3].y = maxY;

		drawContext->drawPrim2D(kPrimitiveDrawFlags, vertices, kRectangleVertices, _filled);

		break;
	}
	default:
		break;
	}
}

} // End of namespace Grim
