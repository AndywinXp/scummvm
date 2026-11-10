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

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/rnode.h"
#include "engines/grim/emi/rprimitiveinstance.h"

namespace Grim {

static const float kAxisLength = 0.04f;
static const int kNumDebugAxes = 3;

static void setLineColor(DrawVertex *line, Color *color) {
	byte red;
	byte green;
	byte blue;

	color->getRGB(&red, &green, &blue);

	for (int i = 0; i < 2; i++)
		line[i].color = (line[i].color & 0xFF000000) | ((uint32)red << 16) | ((uint32)green << 8) | blue;
}

RNode::RNode() {
	lastFrame = 0;
	drawFlags = 0;
}

RNode::~RNode() {
	for (uint i = 0; i < instances.size(); i++) {
		if (instances[i])
			delete instances[i];
	}

	instances.clear();
}

int RNode::findPrimitiveInstance(RPrimitiveInstance *instance) {
	for (uint i = 0; i < instances.size(); i++) {
		if (instances[i] == instance)
			return i;
	}

	return -1;
}

void RNode::removePrimitiveInstance(int index) {
	if (index >= 0 && (uint)index < instances.size())
		instances.remove_at(index);
}

void RNode::draw(Camera *camera) {
	CDrawContext *ctx = CDrawContext::context;

	if (!ctx) {
		warning("RNode::draw(): no draw context");
		return;
	}

	if (lastFrame == ctx->frameCounter || instances.empty())
		return;

	lastFrame = ctx->frameCounter;

	if (drawFlags & kDebugEnabled)
		drawDebug(camera);

	for (uint i = 0; i < instances.size(); i++) {
		RPrimitiveInstance *instance = instances[i];

		if (instance && instance->isVisible(this, camera)) {
			if (drawFlags & kDebugEnabled)
				instance->drawBounds(this, camera);

			if ((drawFlags & kNoOutput) == 0)
				instance->output(this, camera);
		}
	}
}

void RNode::drawDebug(Camera *camera) {
	static DrawVertex parentLine[2];
	static DrawVertex axisLine[2];

	const float *v = camera->view.rot.m;
	const CVector &viewPos = camera->view.pos;

	drawFlags &= ~kDebugEnabled;

	if ((drawFlags & (kDebugParentLine | kDebugEnabled)) != 0 && parent) {
		if ((nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		parentLine[0].x = world.pos.x;
		parentLine[0].y = world.pos.y;
		parentLine[0].z = world.pos.z;

		if ((parent->nodeFlags & kWorldValid) == 0)
			parent->updateWorldTransform();

		parentLine[1].x = parent->world.pos.x;
		parentLine[1].y = parent->world.pos.y;
		parentLine[1].z = parent->world.pos.z;

		setLineColor(parentLine, &Color::kCyan);

		for (int i = 0; i < 2; i++) {
			float x = parentLine[i].x;
			float y = parentLine[i].y;
			float z = parentLine[i].z;
			float viewX = (float)((double)z * v[6] + (double)y * v[3] + (double)x * v[0]);
			float viewY = (float)((double)z * v[7] + (double)y * v[4] + (double)x * v[1]);
			float viewZ = (float)((double)z * v[8] + (double)y * v[5] + (double)x * v[2]);

			parentLine[i].x = (float)((double)viewX + viewPos.x);
			parentLine[i].y = (float)((double)viewY + viewPos.y);
			parentLine[i].z = (float)((double)viewZ + viewPos.z);
		}

		Camera::drawLineLoop(parentLine, 2);
	}

	if ((drawFlags & (kDebugEnabled | kDebugAxes)) != 0) {
		if ((nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		double nodeViewX = (double)v[3] * world.pos.y + (double)v[6] * world.pos.z + (double)v[0] * world.pos.x;
		float nodeViewY = (float)((double)v[4] * world.pos.y + (double)v[1] * world.pos.x + (double)v[7] * world.pos.z);
		float nodeViewZ = (float)((double)v[5] * world.pos.y + (double)v[2] * world.pos.x + (double)v[8] * world.pos.z);
		float originX = (float)(nodeViewX + viewPos.x);
		float originY = (float)((double)nodeViewY + viewPos.y);
		float originZ = (float)((double)nodeViewZ + viewPos.z);

		CMatrix combined;

		world.rot.multiply(&combined, &camera->view.rot);

		const float *a = combined.m;
		Color *axisColors[kNumDebugAxes] = { &Color::kRed, &Color::kGreen, &Color::kBlue };

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
			Camera::drawLineLoop(axisLine, 2);
		}
	}

	drawFlags |= kDebugEnabled;
}

} // End of namespace Grim
