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

#include "common/algorithm.h"
#include "common/array.h"
#include "common/textconsole.h"

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/rnode.h"
#include "engines/grim/emi/rprimitiveinstance.h"

namespace Grim {

static const float kInitialGlobalAlpha = 255.0f;
static const int kSaveBaseSize = 17;
static const int kSaveAlphaEntrySize = 8;
static const int kNumBoxCorners = 8;
static const int kNumBoxEdges = 12;
static const int kBoxEdges[kNumBoxEdges][2] = {
	{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
	{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
	{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
};

RPrimitiveInstance::RPrimitiveInstance(RPrimitive *primitive) {
	prim = primitive;
	visible = true;
	stateRestored = false;
	globalAlphaMode = kAlphaOff;
	globalAlpha = kInitialGlobalAlpha;
	matAlphaMode = kAlphaOff;
}

RPrimitiveInstance::~RPrimitiveInstance() {
}

int RPrimitiveInstance::getSaveSize() {
	return kSaveAlphaEntrySize * (int)matAlpha.size() + kSaveBaseSize;
}

bool RPrimitiveInstance::saveState(tSaveRestoreAccess *access) {
	byte visibleByte = visible ? 1 : 0;

	access(&visibleByte, sizeof(visibleByte));
	access(&globalAlphaMode, sizeof(globalAlphaMode));
	access(&globalAlpha, sizeof(globalAlpha));
	access(&matAlphaMode, sizeof(matAlphaMode));

	uint32 numEntries = matAlpha.size();

	access(&numEntries, sizeof(numEntries));

	Common::Array<int> keys;

	for (Common::HashMap<int, float>::const_iterator it = matAlpha.begin(); it != matAlpha.end(); ++it)
		keys.push_back(it->_key);

	Common::sort(keys.begin(), keys.end());

	for (uint i = 0; i < keys.size(); i++) {
		int key = keys[i];
		float value = matAlpha[key];

		access(&key, sizeof(key));
		access(&value, sizeof(value));
	}

	return true;
}

bool RPrimitiveInstance::restoreState(tSaveRestoreAccess *access) {
	byte visibleByte = 0;

	access(&visibleByte, sizeof(visibleByte));
	visible = visibleByte != 0;
	access(&globalAlphaMode, sizeof(globalAlphaMode));
	access(&globalAlpha, sizeof(globalAlpha));
	access(&matAlphaMode, sizeof(matAlphaMode));

	int numEntries = 0;

	access(&numEntries, sizeof(numEntries));

	for (int i = 0; i < numEntries; i++) {
		int key = 0;
		float value = 0.0f;

		access(&key, sizeof(key));
		access(&value, sizeof(value));
		matAlpha[key] = value;
	}

	stateRestored = true;

	return true;
}

bool RPrimitiveInstance::isVisible(RNode *node, Camera *camera) {
	CDrawContext *ctx = CDrawContext::context;

	if (!ctx) {
		warning("RPrimitiveInstance::isVisible(): no draw context");
		return false;
	}

	if ((node->nodeFlags & CNode::kWorldValid) == 0)
		node->updateWorldTransform();

	const float *w = node->world.rot.m;
	const float *v = camera->view.rot.m;
	const CVector &nodePos = node->world.pos;
	const CVector &viewPos = camera->view.pos;
	const CVector &center = prim->center;

	double sphereX = (double)w[6] * center.z + (double)w[3] * center.y + (double)w[0] * center.x;
	float sphereY = (float)((double)w[7] * center.z + (double)w[1] * center.x + (double)w[4] * center.y);
	float sphereZ = (float)((double)w[8] * center.z + (double)w[2] * center.x + (double)w[5] * center.y);
	float worldX = (float)(sphereX + nodePos.x);
	float worldY = (float)((double)sphereY + nodePos.y);
	float worldZ = (float)((double)sphereZ + nodePos.z);
	double viewX = (double)worldZ * v[6] + (double)worldY * v[3] + (double)worldX * v[0];
	float viewY = (float)((double)worldZ * v[7] + (double)worldY * v[4] + (double)worldX * v[1]);
	float viewZ = (float)((double)worldZ * v[8] + (double)worldY * v[5] + (double)worldX * v[2]);
	float sphere[4];

	sphere[0] = (float)(viewX + viewPos.x);
	sphere[1] = (float)((double)viewY + viewPos.y);
	sphere[2] = (float)((double)viewZ + viewPos.z);
	sphere[3] = prim->radius;

	int result = ctx->clipVolume.testSphere(sphere);

	if (result == CClipVolume::kOutside)
		return false;

	if (result == CClipVolume::kInside)
		return true;

	if ((node->nodeFlags & CNode::kWorldValid) == 0)
		node->updateWorldTransform();

	double nodeViewX = (double)nodePos.x * v[0] + (double)v[6] * nodePos.z + (double)nodePos.y * v[3];
	float nodeViewY = (float)((double)nodePos.y * v[4] + (double)v[7] * nodePos.z + (double)nodePos.x * v[1]);
	float nodeViewZ = (float)((double)nodePos.x * v[2] + (double)v[8] * nodePos.z + (double)nodePos.y * v[5]);
	float transform[12];

	transform[0] = (float)((double)v[6] * w[2] + (double)v[0] * w[0] + (double)v[3] * w[1]);
	transform[1] = (float)((double)v[1] * w[0] + (double)v[7] * w[2] + (double)v[4] * w[1]);
	transform[2] = (float)((double)v[8] * w[2] + (double)v[5] * w[1] + (double)v[2] * w[0]);
	transform[3] = (float)((double)w[4] * v[3] + (double)v[0] * w[3] + (double)v[6] * w[5]);
	transform[4] = (float)((double)w[5] * v[7] + (double)w[4] * v[4] + (double)v[1] * w[3]);
	transform[5] = (float)((double)v[2] * w[3] + (double)w[5] * v[8] + (double)w[4] * v[5]);
	transform[6] = (float)((double)v[6] * w[8] + (double)w[6] * v[0] + (double)v[3] * w[7]);
	transform[7] = (float)((double)v[7] * w[8] + (double)v[4] * w[7] + (double)v[1] * w[6]);
	transform[8] = (float)((double)w[6] * v[2] + (double)w[8] * v[8] + (double)w[7] * v[5]);
	transform[9] = (float)(nodeViewX + viewPos.x);
	transform[10] = (float)((double)nodeViewY + viewPos.y);
	transform[11] = (float)((double)nodeViewZ + viewPos.z);

	return ctx->clipVolume.testBox(&prim->box, transform) != CClipVolume::kOutside;
}

void RPrimitiveInstance::drawBounds(RNode *node, Camera *camera) {
	if ((node->nodeFlags & CNode::kWorldValid) == 0)
		node->updateWorldTransform();

	const float *v = camera->view.rot.m;
	const CVector &nodePos = node->world.pos;
	const CVector &viewPos = camera->view.pos;

	float nodeViewY = (float)((double)v[4] * nodePos.y + (double)v[1] * nodePos.x + (double)v[7] * nodePos.z);
	float nodeViewZ = (float)((double)v[5] * nodePos.y + (double)v[2] * nodePos.x + (double)v[8] * nodePos.z);
	double nodeViewX = (double)v[3] * nodePos.y + (double)v[6] * nodePos.z + (double)v[0] * nodePos.x;
	float originX = (float)(nodeViewX + viewPos.x);
	float originY = (float)((double)nodeViewY + viewPos.y);
	float originZ = (float)((double)nodeViewZ + viewPos.z);

	CMatrix combined;

	node->world.rot.multiply(&combined, &camera->view.rot);

	const CVector &boxMin = prim->box.min;
	const CVector &boxMax = prim->box.max;
	float corners[kNumBoxCorners][3] = {
		{ boxMin.x, boxMin.y, boxMin.z },
		{ boxMax.x, boxMin.y, boxMin.z },
		{ boxMax.x, boxMin.y, boxMax.z },
		{ boxMin.x, boxMin.y, boxMax.z },
		{ boxMin.x, boxMax.y, boxMin.z },
		{ boxMax.x, boxMax.y, boxMin.z },
		{ boxMax.x, boxMax.y, boxMax.z },
		{ boxMin.x, boxMax.y, boxMax.z }
	};
	const float *a = combined.m;

	for (int i = 0; i < kNumBoxCorners; i++) {
		float x = corners[i][0];
		float y = corners[i][1];
		float z = corners[i][2];
		float transformedX = (float)((double)a[6] * z + (double)a[3] * y + (double)a[0] * x);
		float transformedY = (float)((double)a[1] * x + (double)a[7] * z + (double)a[4] * y);
		float transformedZ = (float)((double)a[8] * z + (double)a[5] * y + (double)a[2] * x);

		corners[i][0] = (float)((double)originX + transformedX);
		corners[i][1] = (float)((double)originY + transformedY);
		corners[i][2] = (float)((double)originZ + transformedZ);
	}

	byte red;
	byte green;
	byte blue;

	Color::kWhite.getRGB(&red, &green, &blue);

	DrawVertex line[2];

	for (int i = 0; i < 2; i++)
		line[i].color = (line[i].color & 0xFF000000) | ((uint32)red << 16) | ((uint32)green << 8) | blue;

	for (int i = 0; i < kNumBoxEdges; i++) {
		line[0].x = corners[kBoxEdges[i][0]][0];
		line[0].y = corners[kBoxEdges[i][0]][1];
		line[0].z = corners[kBoxEdges[i][0]][2];
		line[1].x = corners[kBoxEdges[i][1]][0];
		line[1].y = corners[kBoxEdges[i][1]][1];
		line[1].z = corners[kBoxEdges[i][1]][2];

		Camera::drawLineLoop(line, 2);
	}
}

} // End of namespace Grim
