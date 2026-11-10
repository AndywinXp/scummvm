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

#ifndef GRIM_EMI_PRIMITIVE_H
#define GRIM_EMI_PRIMITIVE_H

#include "common/scummsys.h"

#include "engines/grim/emi/color.h"

namespace Grim {

class CDrawContext;

class Primitive {
public:
	enum {
		kTypeLine3D = 0,
		kTypeLine = 1,
		kTypeRectangle = 2,
		kTypePolygon = 3
	};

	enum {
		kLayerFirst = 1,
		kLayerSecond = 2
	};

	Primitive(int primitiveId, int primitiveType);

	void *operator new(size_t size);
	void operator delete(void *block);

	void drawNode(CDrawContext *drawContext);

	Primitive *next;
	Primitive *prev;
	int type;
	int id;
	int layer;
	float x1;
	float y1;
	float z1;
	float x2;
	float y2;
	float z2;
	int polyCount;
	Color color;
	int reserved64;
	bool filled;
};

} // End of namespace Grim

#endif
