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
#include "engines/grim/emi/graphics/primitivequeue_v2.h"

namespace Grim {

static const float kNoChange = -10000.0f;

PrimitiveQueue_v2::PrimitiveQueue_v2(EmiEngine *engine) {
	_engine = engine;
	_started = false;
	_drawDisabled = false;
	_nextId = 1;
	_head2D = nullptr;
	_tail2D = nullptr;
	_head3D = nullptr;
	_tail3D = nullptr;
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
	while (_head2D) {
		Primitive *following = _head2D->_next;

		delete _head2D;
		_head2D = following;
	}

	_tail2D = nullptr;
}

int PrimitiveQueue_v2::add3DLine(float startX, float startY, float startZ, float endX, float endY, float endZ, const Color &lineColor, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	Primitive *primitive = newNode3D(primitiveId, layer);

	primitive->_x1 = startX;
	primitive->_y1 = startY;
	primitive->_z1 = startZ;
	primitive->_x2 = endX;
	primitive->_y2 = endY;
	primitive->_z2 = endZ;
	primitive->_color = lineColor;

	return primitive->_id;
}

int PrimitiveQueue_v2::addLine(float startX, float startY, float endX, float endY, const Color &lineColor, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	Primitive *primitive = newNode2D(primitiveId, layer);

	primitive->_x1 = startX;
	primitive->_y1 = startY;
	primitive->_z1 = 0.0f;
	primitive->_z2 = 0.0f;
	primitive->_x2 = endX;
	primitive->_y2 = endY;
	primitive->_color = lineColor;

	return primitive->_id;
}

int PrimitiveQueue_v2::addRectangle(float startX, float startY, float endX, float endY, const Color &rectangleColor, bool isFilled, int layer) {
	int primitiveId = _nextId;

	_nextId++;

	Primitive *primitive = newNode2D(primitiveId, layer);

	primitive->_type = Primitive::kTypeRectangle;
	primitive->_x1 = startX;
	primitive->_y1 = startY;
	primitive->_z1 = 0.0f;
	primitive->_z2 = 0.0f;
	primitive->_x2 = endX;
	primitive->_y2 = endY;
	primitive->_color = rectangleColor;
	primitive->_filled = isFilled;

	return primitive->_id;
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

		primitive->_type = Primitive::kTypePolygon;
		primitive->_x1 = points[i * 3];
		primitive->_y1 = points[i * 3 + 1];
		primitive->_z1 = points[i * 3 + 2];
		primitive->_x2 = points[i * 3 + 3];
		primitive->_y2 = points[i * 3 + 4];
		primitive->_z2 = points[i * 3 + 5];
		primitive->_color = polygonColor;
		primitive->_polyCount = numPoints - 1;
	}

	return primitiveId;
}

Primitive *PrimitiveQueue_v2::newNode2D(int primitiveId, int layer) {
	Primitive *primitive = new Primitive(primitiveId, Primitive::kTypeLine);

	if (_tail2D) {
		_tail2D->_next = primitive;
		_tail2D->_next->_prev = _tail2D;
		_tail2D = _tail2D->_next;
	} else {
		_tail2D = primitive;
		_head2D = primitive;
	}

	int nodeLayer = layer;

	if (nodeLayer != Primitive::kLayerSecond && nodeLayer != Primitive::kLayerFirst)
		nodeLayer = Primitive::kLayerSecond;

	_tail2D->_layer = nodeLayer;

	return _tail2D;
}

Primitive *PrimitiveQueue_v2::newNode3D(int primitiveId, int layer) {
	Primitive *primitive = new Primitive(primitiveId, Primitive::kTypeLine);

	if (_tail3D) {
		_tail3D->_next = primitive;
		_tail3D->_next->_prev = _tail3D;
		_tail3D = _tail3D->_next;
	} else {
		_tail3D = primitive;
		_head3D = primitive;
	}

	int nodeLayer = layer;

	if (nodeLayer != Primitive::kLayerSecond && nodeLayer != Primitive::kLayerFirst)
		nodeLayer = Primitive::kLayerSecond;

	_tail3D->_layer = nodeLayer;

	return _tail3D;
}

void PrimitiveQueue_v2::kill(int primitiveId) {
	Primitive *current = _head2D;

	while (current) {
		if (current->_id == primitiveId) {
			if (current->_prev)
				current->_prev->_next = current->_next;

			if (current->_next)
				current->_next->_prev = current->_prev;

			if (_tail2D == current)
				_tail2D = current->_prev;

			if (current == _head2D)
				_head2D = _head2D->_next;

			if (_tail3D == current) {
				warning("PrimitiveQueue_v2::kill(): the removed primitive was the 3D tail");
				_tail3D = current->_prev;
			}

			Primitive *following = current->_next;

			delete current;
			current = following;
		} else {
			// The disasm moves to the 3D list from its second element
			if (current == _tail2D && _head3D)
				current = _head3D;

			current = current->_next;
		}
	}
}

void PrimitiveQueue_v2::setColor(int primitiveId, const Color &newColor) {
	Primitive *current = findNext(primitiveId, nullptr);

	while (current) {
		current->_color = newColor;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::setLayer(int primitiveId, int newLayer) {
	Primitive *current = findNext(primitiveId, nullptr);

	while (current) {
		if (newLayer != Primitive::kLayerSecond && newLayer != Primitive::kLayerFirst)
			newLayer = Primitive::kLayerSecond;

		current->_layer = newLayer;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::moveBy(int primitiveId, float deltaX, float deltaY) {
	Primitive *current = findNext(primitiveId, nullptr);

	while (current) {
		current->_x1 = deltaX + current->_x1;
		current->_y1 = deltaY + current->_y1;
		current->_x2 = deltaX + current->_x2;
		current->_y2 = deltaY + current->_y2;
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
		double difference = (double)newX - (double)first->_x1;

		first->_x1 = newX;
		movedX = (float)difference;
	}

	if (newY > kNoChange) {
		double difference = (double)newY - (double)first->_y1;

		first->_y1 = newY;
		movedY = (float)difference;
	}

	first->_x2 = movedX + first->_x2;
	first->_y2 = movedY + first->_y2;

	Primitive *current = findNext(primitiveId, first);

	while (current) {
		current->_x1 = movedX + current->_x1;
		current->_y1 = movedY + current->_y1;
		current->_x2 = movedX + current->_x2;
		current->_y2 = movedY + current->_y2;
		current = findNext(primitiveId, current);
	}
}

void PrimitiveQueue_v2::setEnd(int primitiveId, float newX, float newY) {
	Primitive *primitive = findNext(primitiveId, nullptr);

	if (!primitive)
		return;

	if (newX > kNoChange)
		primitive->_x2 = newX;

	if (newY > kNoChange)
		primitive->_y2 = newY;
}

void PrimitiveQueue_v2::setEndRelative(int primitiveId, float deltaX, float deltaY) {
	Primitive *primitive = findNext(primitiveId, nullptr);

	if (!primitive)
		return;

	if (deltaX > kNoChange)
		primitive->_x2 = deltaX + primitive->_x1;

	if (deltaY > kNoChange)
		primitive->_y2 = deltaY + primitive->_y1;
}

void PrimitiveQueue_v2::draw(CDrawContext *drawContext, int layers) {
	Primitive *current3D = _head3D;

	drawContext->clear(false, true);

	while (current3D) {
		if ((layers & current3D->_layer) != 0)
			current3D->drawNode(drawContext);

		current3D = current3D->_next;
	}

	Primitive *current2D = _head2D;

	while (current2D) {
		if ((layers & current2D->_layer) != 0)
			current2D->drawNode(drawContext);

		current2D = current2D->_next;
	}
}

Primitive *PrimitiveQueue_v2::findNext(int primitiveId, Primitive *previous) {
	Primitive *result;

	if (previous)
		result = previous->_next;
	else
		result = _head2D;

	while (result) {
		if (result->_id == primitiveId)
			break;

		if (result == _tail2D)
			result = _head3D;
		else
			result = result->_next;
	}

	return result;
}

} // End of namespace Grim
