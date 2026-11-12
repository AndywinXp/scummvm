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

#include "common/textconsole.h"

#include "engines/grim/emi/scene/camera.h"
#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/graphics/color.h"
#include "engines/grim/emi/nodes/rnode.h"
#include "engines/grim/emi/nodes/rprimitiveinstance.h"

namespace Grim {

static const float kAxisLength = 0.04f;
static const int kNumDebugAxes = 3;

static void setLineColor(DrawVertex *line, const Color *color) {
	byte red;
	byte green;
	byte blue;

	color->getRGB(&red, &green, &blue);

	for (int i = 0; i < 2; i++)
		line[i].color = (line[i].color & 0xFF000000) | ((uint32)red << 16) | ((uint32)green << 8) | blue;
}

RNode::RNode() {
	_lastFrame = 0;
	_drawFlags = 0;
}

RNode::~RNode() {
	for (uint i = 0; i < _instances.size(); i++) {
		if (_instances[i])
			delete _instances[i];
	}

	_instances.clear();
}

int RNode::findPrimitiveInstance(RPrimitiveInstance *instance) {
	for (uint i = 0; i < _instances.size(); i++) {
		if (_instances[i] == instance)
			return i;
	}

	return -1;
}

void RNode::removePrimitiveInstance(int index) {
	if (index >= 0 && (uint)index < _instances.size())
		_instances.remove_at(index);
}

void RNode::draw(Camera *camera) {
	CDrawContext *ctx = camera->getDrawContext();

	if (!ctx) {
		warning("RNode::draw(): no draw context");
		return;
	}

	if (_lastFrame == ctx->_frameCounter || _instances.empty())
		return;

	_lastFrame = ctx->_frameCounter;

	if (_drawFlags & kDebugEnabled)
		drawDebug(camera);

	for (uint i = 0; i < _instances.size(); i++) {
		RPrimitiveInstance *instance = _instances[i];

		if (instance && instance->isVisible(this, camera)) {
			if (_drawFlags & kDebugEnabled)
				instance->drawBounds(this, camera);

			if ((_drawFlags & kNoOutput) == 0)
				instance->output(this, camera);
		}
	}
}

void RNode::drawDebug(Camera *camera) {
	DrawVertex parentLine[2];
	DrawVertex axisLine[2];

	const float *v = camera->_view._rot._m;
	const CVector &viewPos = camera->_view._pos;

	_drawFlags &= ~kDebugEnabled;

	if ((_drawFlags & (kDebugParentLine | kDebugEnabled)) != 0 && _parent) {
		if ((_nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		parentLine[0].x = _world._pos._x;
		parentLine[0].y = _world._pos._y;
		parentLine[0].z = _world._pos._z;

		if ((_parent->_nodeFlags & kWorldValid) == 0)
			_parent->updateWorldTransform();

		parentLine[1].x = _parent->_world._pos._x;
		parentLine[1].y = _parent->_world._pos._y;
		parentLine[1].z = _parent->_world._pos._z;

		setLineColor(parentLine, &Color::kCyan);

		for (int i = 0; i < 2; i++) {
			float x = parentLine[i].x;
			float y = parentLine[i].y;
			float z = parentLine[i].z;
			float viewX = (float)((double)z * v[6] + (double)y * v[3] + (double)x * v[0]);
			float viewY = (float)((double)z * v[7] + (double)y * v[4] + (double)x * v[1]);
			float viewZ = (float)((double)z * v[8] + (double)y * v[5] + (double)x * v[2]);

			parentLine[i].x = (float)((double)viewX + viewPos._x);
			parentLine[i].y = (float)((double)viewY + viewPos._y);
			parentLine[i].z = (float)((double)viewZ + viewPos._z);
		}

		camera->drawLineLoop(parentLine, 2);
	}

	if ((_drawFlags & (kDebugEnabled | kDebugAxes)) != 0) {
		if ((_nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		double nodeViewX = (double)v[3] * _world._pos._y + (double)v[6] * _world._pos._z + (double)v[0] * _world._pos._x;
		float nodeViewY = (float)((double)v[4] * _world._pos._y + (double)v[1] * _world._pos._x + (double)v[7] * _world._pos._z);
		float nodeViewZ = (float)((double)v[5] * _world._pos._y + (double)v[2] * _world._pos._x + (double)v[8] * _world._pos._z);
		float originX = (float)(nodeViewX + viewPos._x);
		float originY = (float)((double)nodeViewY + viewPos._y);
		float originZ = (float)((double)nodeViewZ + viewPos._z);

		CMatrix combined;

		_world._rot.multiply(&combined, &camera->_view._rot);

		const float *a = combined._m;
		const Color *axisColors[kNumDebugAxes] = { &Color::kRed, &Color::kGreen, &Color::kBlue };

		for (int axis = 0; axis < kNumDebugAxes; axis++) {
			axisLine[0].x = originX;
			axisLine[0].y = originY;
			axisLine[0].z = originZ;

			double endX = (double)a[axis * 3] * kAxisLength;
			double endY = (double)a[axis * 3 + 1] * kAxisLength;
			double endZ = (double)a[axis * 3 + 2] * kAxisLength;

			if (axis == 0) {
				axisLine[1].x = (float)((double)originX + (float)endX);
				axisLine[1].y = (float)((double)originY + (float)endY);
				axisLine[1].z = (float)(endZ + originZ);
			} else {
				axisLine[1].x = (float)(originX + endX);
				axisLine[1].y = (float)((double)(float)endY + originY);
				axisLine[1].z = (float)((double)(float)endZ + originZ);
			}

			setLineColor(axisLine, axisColors[axis]);
			camera->drawLineLoop(axisLine, 2);
		}
	}

	_drawFlags |= kDebugEnabled;
}

} // End of namespace Grim
