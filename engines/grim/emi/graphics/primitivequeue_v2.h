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

#ifndef GRIM_EMI_PRIMITIVEQUEUE_V2_H
#define GRIM_EMI_PRIMITIVEQUEUE_V2_H

#include "common/scummsys.h"

#include "engines/grim/emi/graphics/color.h"
#include "engines/grim/emi/graphics/primitive.h"

namespace Grim {

class CDrawContext;
class EmiEngine;

class PrimitiveQueue_v2 {
public:
	explicit PrimitiveQueue_v2(EmiEngine *engine);

	void clear();
	int add3DLine(float startX, float startY, float startZ, float endX, float endY, float endZ, const Color &lineColor, int layer);
	int addLine(float startX, float startY, float endX, float endY, const Color &lineColor, int layer);
	int addRectangle(float startX, float startY, float endX, float endY, const Color &rectangleColor, bool isFilled, int layer);
	int addPolygon(int numPoints, const float *points, const Color &polygonColor, int layer);
	Primitive *newNode2D(int primitiveId, int layer);
	Primitive *newNode3D(int primitiveId, int layer);
	void kill(int primitiveId);
	void setColor(int primitiveId, const Color &newColor);
	void setLayer(int primitiveId, int newLayer);
	void moveBy(int primitiveId, float deltaX, float deltaY);
	void moveTo(int primitiveId, float newX, float newY);
	void setEnd(int primitiveId, float newX, float newY);
	void setEndRelative(int primitiveId, float deltaX, float deltaY);
	void draw(CDrawContext *drawContext, int layers);
	Primitive *findNext(int primitiveId, Primitive *previous);

	bool startup();
	void shutdown();
	void resetIds();
	void drawQueue(CDrawContext *drawContext, int layers);

	Primitive *head2D;
	Primitive *tail2D;
	Primitive *head3D;
	Primitive *tail3D;

private:
	EmiEngine *_engine;
	bool _started;
	bool _drawDisabled;
	int _nextId;
};

} // End of namespace Grim

#endif
