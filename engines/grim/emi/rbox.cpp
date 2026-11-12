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

#include <math.h>

#include "common/scummsys.h"
#include "common/textconsole.h"

#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/rbox.h"
#include "engines/grim/emi/rnode.h"

namespace Grim {

struct BoxFace {
	int corners[4];
	float normal[3];
};

static const int kNumBoxCorners = 8;
static const int kNumBoxFaces = 6;
static const int kFacePolyFlags = 0x304;
static const BoxFace kBoxFaces[kNumBoxFaces] = {
	{ { 2, 3, 7, 6 }, { 0.0f, 1.0f, 0.0f } },
	{ { 4, 5, 1, 0 }, { 0.0f, -1.0f, 0.0f } },
	{ { 0, 1, 3, 2 }, { -1.0f, 0.0f, 0.0f } },
	{ { 6, 7, 5, 4 }, { 1.0f, 0.0f, 0.0f } },
	{ { 2, 6, 4, 0 }, { 0.0f, 0.0f, -1.0f } },
	{ { 1, 5, 7, 3 }, { 0.0f, 0.0f, 1.0f } }
};

RBox::RBox() {
	sizeX = 0.0f;
	sizeY = 0.0f;
	sizeZ = 0.0f;
}

RBox::~RBox() {
}

void RBox::setSize(float width, float height, float depth) {
	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;
	float halfDepth = depth * 0.5f;

	sizeX = width;
	sizeY = height;
	sizeZ = depth;

	box.min.x = -halfWidth;
	box.min.y = -halfHeight;
	box.min.z = -halfDepth;
	box.max.x = halfWidth;
	box.max.y = halfHeight;
	box.max.z = halfDepth;

	float sumY = (float)((double)box.max.y + box.min.y);
	float sumZ = (float)((double)box.max.z + box.min.z);

	center.x = (float)(((double)box.min.x + box.max.x) * 0.5);
	center.y = (float)((double)sumY * 0.5);
	center.z = (float)((double)sumZ * 0.5);

	double extentX = (double)box.max.x - box.min.x;
	float extentY = (float)((double)box.max.y - box.min.y);
	float extentZ = (float)((double)box.max.z - box.min.z);
	double halfExtentX = extentX * 0.5;
	double halfExtentY = (double)extentY * 0.5;
	double halfExtentZ = (double)extentZ * 0.5;

	radius = (float)sqrt(halfExtentZ * halfExtentZ + halfExtentY * halfExtentY + halfExtentX * halfExtentX);
}

RPrimitiveInstance *RBox::createInstance() {
	return new RBoxInstance(this);
}

RBoxInstance::RBoxInstance(RBox *boxPrimitive) : RPrimitiveInstance(boxPrimitive) {
}

RBoxInstance::~RBoxInstance() {
}

void RBoxInstance::output(RNode *node, Camera *camera) {
	if (!visible)
		return;

	CDrawContext *ctx = camera->getDrawContext();

	if (!ctx) {
		warning("RBoxInstance::output(): no draw context");
		return;
	}

	const Color *faceColors[kNumBoxFaces] = {
		&Color::kWhite, &Color::kGreen, &Color::kBlue, &Color::kRed, &Color::kYellow, &Color::kCyan
	};

	if ((node->nodeFlags & CNode::kWorldValid) == 0)
		node->updateWorldTransform();

	const float *v = camera->view.rot.m;
	const CVector &nodePos = node->world.pos;
	const CVector &viewPos = camera->view.pos;

	double nodeViewX = (double)v[6] * nodePos.z + (double)v[3] * nodePos.y + (double)v[0] * nodePos.x;
	float nodeViewY = (float)((double)v[7] * nodePos.z + (double)v[1] * nodePos.x + (double)v[4] * nodePos.y);
	float nodeViewZ = (float)((double)v[8] * nodePos.z + (double)v[2] * nodePos.x + (double)v[5] * nodePos.y);
	float originX = (float)(nodeViewX + viewPos.x);
	float originY = (float)((double)nodeViewY + viewPos.y);
	float originZ = (float)((double)nodeViewZ + viewPos.z);

	CMatrix combined;

	node->world.rot.multiply(&combined, &camera->view.rot);

	const CVector &boxMin = prim->box.min;
	const CVector &boxMax = prim->box.max;
	float corners[kNumBoxCorners][3] = {
		{ boxMin.x, boxMin.y, boxMin.z },
		{ boxMin.x, boxMin.y, boxMax.z },
		{ boxMin.x, boxMax.y, boxMin.z },
		{ boxMin.x, boxMax.y, boxMax.z },
		{ boxMax.x, boxMin.y, boxMin.z },
		{ boxMax.x, boxMin.y, boxMax.z },
		{ boxMax.x, boxMax.y, boxMin.z },
		{ boxMax.x, boxMax.y, boxMax.z }
	};
	float transformed[kNumBoxCorners][3];
	const float *a = combined.m;

	for (int i = 0; i < kNumBoxCorners; i++) {
		float x = corners[i][0];
		float y = corners[i][1];
		float z = corners[i][2];
		double pendingX = (double)a[6] * z + (double)a[0] * x + (double)a[3] * y;
		float rotatedY = (float)((double)a[7] * z + (double)a[1] * x + (double)a[4] * y);
		float rotatedZ = (float)((double)a[8] * z + (double)a[2] * x + (double)a[5] * y);

		transformed[i][0] = (float)(pendingX + originX);
		transformed[i][1] = (float)((double)rotatedY + originY);
		transformed[i][2] = (float)((double)rotatedZ + originZ);
	}

	int clipResult = ctx->clipVolume.testPoints(&transformed[0][0], kNumBoxCorners);

	if (clipResult == CClipVolume::kOutside)
		return;

	CDrawList *drawList = ctx->getDrawList(0);
	CVector cameraLocal;

	if ((camera->nodeFlags & CNode::kWorldValid) == 0)
		camera->updateWorldTransform();

	node->worldToLocal(&camera->world.pos, &cameraLocal);

	for (int face = 0; face < kNumBoxFaces; face++) {
		const BoxFace &boxFace = kBoxFaces[face];
		const float *anchor = corners[boxFace.corners[0]];
		double deltaX = (double)cameraLocal.x - anchor[0];
		double deltaY = (double)cameraLocal.y - anchor[1];
		double deltaZ = (double)cameraLocal.z - anchor[2];
		double facing = deltaZ * boxFace.normal[2] + deltaX * boxFace.normal[0] + deltaY * boxFace.normal[1];

		if (facing < 0.0)
			continue;

		DrawPoly *poly = drawList->newPoly(4);
		byte red;
		byte green;
		byte blue;

		poly->flags = kFacePolyFlags;
		faceColors[face]->getRGB(&red, &green, &blue);

		for (int i = 0; i < 4; i++) {
			DrawVertex *vertex = poly->verts[i];
			const float *point = transformed[boxFace.corners[i]];

			vertex->x = point[0];
			vertex->y = point[1];
			vertex->z = point[2];
			vertex->color = (vertex->color & 0xFF000000) | ((uint32)red << 16) | ((uint32)green << 8) | blue;
		}

		if (clipResult == CClipVolume::kPartial)
			ctx->clipVolume.computeVertexOutcodes(4, poly->verts);

		poly->reserved8 = 0;
		drawList->addPoly(poly);
	}
}

} // End of namespace Grim
