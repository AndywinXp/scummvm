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
#include "engines/grim/emi/primitivequeue_v2.h"

namespace Grim {

static const float kNoChange = -10000.0f;

PrimitiveQueue_v2::PrimitiveQueue_v2(EmiEngine *engine) {
	_engine = engine;
	_started = false;
	_drawDisabled = false;
	_nextId = 1;
	head2D = nullptr;
	tail2D = nullptr;
	head3D = nullptr;
	tail3D = nullptr;
}

bool PrimitiveQueue_v2::startup() {
	if (_started)
		return false;

	_started = true;

	return true;
}

void PrimitiveQueue_v2::shutdown() {
	clear();
	_started = false;
}

void PrimitiveQueue_v2::resetIds() {
	_nextId = 1;
}

void PrimitiveQueue_v2::drawQueue(CDrawContext *drawContext, int layers) {
	if (!_drawDisabled)
		draw(drawContext, layers);
}

void PrimitiveQueue_v2::clear() {
	// The disasm only frees the 2D list
	while (head2D) {
		Primitive *following = head2D->next;

		delete head2D;
		head2D = following;
	}

	tail2D = nullptr;
}

int PrimitiveQueue_v2::add3DLine(float startX, float startY, float startZ, float endX, float endY, float endZ, const Color &lineColor, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	Primitive *primitive = newNode3D(primitiveId, layer);

	primitive->x1 = startX;
	primitive->y1 = startY;
	primitive->z1 = startZ;
	primitive->x2 = endX;
	primitive->y2 = endY;
	primitive->z2 = endZ;
	primitive->color = lineColor;

	return primitive->id;
}

int PrimitiveQueue_v2::addLine(float startX, float startY, float endX, float endY, const Color &lineColor, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	Primitive *primitive = newNode2D(primitiveId, layer);

	primitive->x1 = startX;
	primitive->y1 = startY;
	primitive->z1 = 0.0f;
	primitive->z2 = 0.0f;
	primitive->x2 = endX;
	primitive->y2 = endY;
	primitive->color = lineColor;

	return primitive->id;
}

int PrimitiveQueue_v2::addRectangle(float startX, float startY, float endX, float endY, const Color &rectangleColor, bool isFilled, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	Primitive *primitive = newNode2D(primitiveId, layer);

	primitive->type = Primitive::kTypeRectangle;
	primitive->x1 = startX;
	primitive->y1 = startY;
	primitive->z1 = 0.0f;
	primitive->z2 = 0.0f;
	primitive->x2 = endX;
	primitive->y2 = endY;
	primitive->color = rectangleColor;
	primitive->filled = isFilled;

	return primitive->id;
}

int PrimitiveQueue_v2::addPolygon(int numPoints, const float *points, const Color &polygonColor, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	if (numPoints - 1 > 0 && !points) {
		warning("PrimitiveQueue_v2::addPolygon(): missing points");

		return primitiveId;
	}

	for (int i = 0; i < numPoints - 1; i++) {
		Primitive *primitive = newNode2D(primitiveId, layer);

		primitive->type = Primitive::kTypePolygon;
		primitive->x1 = points[i * 3];
		primitive->y1 = points[i * 3 + 1];
		primitive->z1 = points[i * 3 + 2];
		primitive->x2 = points[i * 3 + 3];
		primitive->y2 = points[i * 3 + 4];
		primitive->z2 = points[i * 3 + 5];
		primitive->color = polygonColor;
		primitive->polyCount = numPoints - 1;
	}

	return primitiveId;
}

Primitive *PrimitiveQueue_v2::newNode2D(int primitiveId, int layer) {
	Primitive *primitive = new Primitive(primitiveId, Primitive::kTypeLine);

	if (tail2D) {
		tail2D->next = primitive;
		tail2D->next->prev = tail2D;
		tail2D = tail2D->next;
	} else {
		tail2D = primitive;
		head2D = primitive;
	}

	int nodeLayer = layer;

	if (nodeLayer != Primitive::kLayerSecond && nodeLayer != Primitive::kLayerFirst)
		nodeLayer = Primitive::kLayerSecond;

	tail2D->layer = nodeLayer;

	return tail2D;
}

Primitive *PrimitiveQueue_v2::newNode3D(int primitiveId, int layer) {
	Primitive *primitive = new Primitive(primitiveId, Primitive::kTypeLine);

	if (tail3D) {
		tail3D->next = primitive;
		tail3D->next->prev = tail3D;
		tail3D = tail3D->next;
	} else {
		tail3D = primitive;
		head3D = primitive;
	}

	int nodeLayer = layer;

	if (nodeLayer != Primitive::kLayerSecond && nodeLayer != Primitive::kLayerFirst)
		nodeLayer = Primitive::kLayerSecond;

	tail3D->layer = nodeLayer;

	return tail3D;
}

void PrimitiveQueue_v2::kill(int primitiveId) {
	Primitive *current = head2D;

	while (current) {
		if (current->id == primitiveId) {
			if (current->prev)
				current->prev->next = current->next;

			if (current->next)
				current->next->prev = current->prev;

			if (tail2D == current)
				tail2D = current->prev;

			if (current == head2D)
				head2D = head2D->next;

			if (tail3D == current) {
				warning("PrimitiveQueue_v2::kill(): the removed primitive was the 3D tail");
				tail3D = current->prev;
			}

			Primitive *following = current->next;

			delete current;
			current = following;
		} else {
			// The disasm moves to the 3D list from its second element
			if (current == tail2D && head3D)
				current = head3D;

			current = current->next;
		}
	}
}

void PrimitiveQueue_v2::setColor(int primitiveId, const Color &newColor) {
	Primitive *current = findNext(primitiveId, nullptr);

	while (current) {
		current->color = newColor;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::setLayer(int primitiveId, int newLayer) {
	Primitive *current = findNext(primitiveId, nullptr);

	while (current) {
		if (newLayer != Primitive::kLayerSecond && newLayer != Primitive::kLayerFirst)
			newLayer = Primitive::kLayerSecond;

		current->layer = newLayer;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::moveBy(int primitiveId, float deltaX, float deltaY) {
	Primitive *current = findNext(primitiveId, nullptr);

	while (current) {
		current->x1 = deltaX + current->x1;
		current->y1 = deltaY + current->y1;
		current->x2 = deltaX + current->x2;
		current->y2 = deltaY + current->y2;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::moveTo(int primitiveId, float newX, float newY) {
	float movedX = 0.0f;
	float movedY = 0.0f;
	Primitive *first = findNext(primitiveId, nullptr);

	if (!first)
		return;

	if (newX > kNoChange) {
		double difference = (double)newX - (double)first->x1;

		first->x1 = newX;
		movedX = (float)difference;
	}

	if (newY > kNoChange) {
		double difference = (double)newY - (double)first->y1;

		first->y1 = newY;
		movedY = (float)difference;
	}

	first->x2 = movedX + first->x2;
	first->y2 = movedY + first->y2;

	Primitive *current = findNext(primitiveId, first);

	while (current) {
		current->x1 = movedX + current->x1;
		current->y1 = movedY + current->y1;
		current->x2 = movedX + current->x2;
		current->y2 = movedY + current->y2;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::setEnd(int primitiveId, float newX, float newY) {
	Primitive *primitive = findNext(primitiveId, nullptr);

	if (!primitive)
		return;

	if (newX > kNoChange)
		primitive->x2 = newX;

	if (newY > kNoChange)
		primitive->y2 = newY;
}

void PrimitiveQueue_v2::setEndRelative(int primitiveId, float deltaX, float deltaY) {
	Primitive *primitive = findNext(primitiveId, nullptr);

	if (!primitive)
		return;

	if (deltaX > kNoChange)
		primitive->x2 = deltaX + primitive->x1;

	if (deltaY > kNoChange)
		primitive->y2 = deltaY + primitive->y1;
}

void PrimitiveQueue_v2::draw(CDrawContext *drawContext, int layers) {
	Primitive *current3D = head3D;

	drawContext->clear(false, true);

	while (current3D) {
		if ((layers & current3D->layer) != 0)
			current3D->drawNode(drawContext);

		current3D = current3D->next;
	}

	Primitive *current2D = head2D;

	while (current2D) {
		if ((layers & current2D->layer) != 0)
			current2D->drawNode(drawContext);

		current2D = current2D->next;
	}
}

Primitive *PrimitiveQueue_v2::findNext(int primitiveId, Primitive *previous) {
	Primitive *result;

	if (previous)
		result = previous->next;
	else
		result = head2D;

	while (result) {
		if (result->id == primitiveId)
			break;

		if (result == tail2D)
			result = head3D;
		else
			result = result->next;
	}

	return result;
}

} // End of namespace Grim
