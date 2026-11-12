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

#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/cnode.h"
#include "engines/grim/emi/cnoderef.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const float kMinLength = 1e-20f;
static const float kPi = 3.14159274f;
static const float kHalfTurnDegrees = 180.0f;

CNode::CNode() {
	bindRot.setIdentity();
	bindPos.x = 0.0f;
	bindPos.y = 0.0f;
	bindPos.z = 0.0f;
	skinRot.setIdentity();
	skinPos.x = 0.0f;
	skinPos.y = 0.0f;
	skinPos.z = 0.0f;
	scale.z = 0.0f;
	scale.y = 0.0f;
	scale.x = 0.0f;
	pos.z = 0.0f;
	pos.y = 0.0f;
	pos.x = 0.0f;
	eulerDeg.z = 0.0f;
	eulerDeg.y = 0.0f;
	eulerDeg.x = 0.0f;
	world.rot.setIdentity();
	world.pos.x = 0.0f;
	world.pos.y = 0.0f;
	world.pos.z = 0.0f;
	observerCount = 0;
	observerHead = nullptr;
	observerTail = nullptr;
	pos.x = 0.0f;
	pos.y = 0.0f;
	pos.z = 0.0f;
	quat.setEuler(0.0f, 0.0f, 0.0f);
	eulerDeg.x = 0.0f;
	eulerDeg.y = 0.0f;
	eulerDeg.z = 0.0f;
	nodeFlags = 15;
	childCount = 0;
	prevSibling = nullptr;
	nextSibling = nullptr;
	firstChild = nullptr;
	parent = nullptr;
	name[0] = 0;
	scale.x = 1.0f;
	scale.y = 1.0f;
	scale.z = 1.0f;
}

CNode::~CNode() {
	CNodeRef *observer = observerHead;

	while (observer) {
		CNodeRef *current = observer;

		observer = observer->next;
		current->onNodeDestroyed();
		current->node = nullptr;
	}

	detachFromParent();
}

void *CNode::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void CNode::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

void CNode::updateLocalState() {
	if ((nodeFlags & kWorldValid) != 0)
		updateChildrenLocalTransforms();

	if ((nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	if ((nodeFlags & (kEulerValid | kQuaternionValid)) == 0)
		updateLocalQuaternion();

	nodeFlags = (nodeFlags & ~(kWorldValid | kLocalSet)) | kLocalSet;
}

CQuaternion CNode::getWorldOrientation() {
	CQuaternion orientation;

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	world.rot.toQuaternion(&orientation);

	return orientation;
}

void CNode::setLocalPosition(const CVector &position) {
	pos = position;
	nodeFlags |= kPositionValid | kLocalSet;
	updateLocalState();
}

void CNode::setLocalTransform(const CVector &position, const CQuaternion &rotation) {
	pos = position;
	nodeFlags |= kPositionValid | kLocalSet;
	updateLocalState();

	quat.x = rotation.x;
	nodeFlags = (nodeFlags & ~(kEulerValid | kQuaternionValid | kLocalSet)) | kQuaternionValid | kLocalSet;
	quat.y = rotation.y;
	quat.z = rotation.z;
	quat.w = rotation.w;
	updateLocalState();
}

void CNode::localToWorld(CVector *point) {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;
	float newY = (float)((double)m[7] * point->z + (double)m[1] * point->x + (double)m[4] * point->y);
	float newZ = (float)((double)m[8] * point->z + (double)m[2] * point->x + (double)m[5] * point->y);
	double newX = (double)m[6] * point->z + (double)m[3] * point->y + (double)m[0] * point->x + world.pos.x;
	float storedY = (float)((double)newY + world.pos.y);
	float storedZ = (float)((double)newZ + world.pos.z);

	point->y = storedY;
	point->x = (float)newX;
	point->z = storedZ;
}

CVector *CNode::localToWorld(const CVector *point, CVector *out) {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;
	float newY = (float)((double)m[1] * point->x + (double)m[7] * point->z + (double)m[4] * point->y);
	float newZ = (float)((double)m[2] * point->x + (double)m[8] * point->z + (double)m[5] * point->y);
	double sumY = (double)newY + world.pos.y;
	float storedZ = (float)((double)newZ + world.pos.z);

	out->x = (float)((double)m[6] * point->z + (double)m[3] * point->y + (double)m[0] * point->x + world.pos.x);
	out->z = storedZ;
	out->y = (float)sumY;

	return out;
}

float *CNode::worldToLocal(const float *worldX, const float *worldY, const float *worldZ, float *localX, float *localY, float *localZ) {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;

	*localX = *worldX - world.pos.x;
	*localY = *worldY - world.pos.y;

	double deltaZ = (double)*worldZ - world.pos.z;

	*localZ = (float)deltaZ;

	float storedX = *localX;
	float storedY = *localY;

	*localX = (float)(deltaZ * m[2] + (double)*localY * m[1] + (double)storedX * m[0]);
	*localY = (float)(deltaZ * m[5] + (double)storedY * m[4] + (double)storedX * m[3]);
	*localZ = (float)(deltaZ * m[8] + (double)storedY * m[7] + (double)storedX * m[6]);

	return localX;
}

CVector *CNode::worldToLocal(CVector *point) {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;
	double deltaX = (double)point->x - world.pos.x;
	float deltaY = (float)((double)point->y - world.pos.y);
	double deltaZ = (double)point->z - world.pos.z;
	float deltaZStored = (float)deltaZ;
	float newY = (float)(deltaX * m[3] + (double)deltaZStored * m[5] + (double)deltaY * m[4]);
	double newX = deltaZ * m[2] + (double)deltaY * m[1] + deltaX * m[0];
	double partialZ = deltaX * m[6] + (double)deltaZStored * m[8];
	double partialY = (double)deltaY * m[7];

	point->y = newY;
	float newZ = (float)(partialZ + partialY);
	point->x = (float)newX;
	point->z = newZ;

	return point;
}

CVector *CNode::worldToLocal(const CVector *point, CVector *out) {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;
	double deltaX = (double)point->x - world.pos.x;
	float deltaY = (float)((double)point->y - world.pos.y);
	double deltaZ = (double)point->z - world.pos.z;
	float deltaZStored = (float)deltaZ;
	float newX = (float)(deltaZ * m[2] + (double)deltaY * m[1] + deltaX * m[0]);
	double newY = deltaX * m[3] + (double)deltaZStored * m[5] + (double)deltaY * m[4];
	float newZ = (float)(deltaX * m[6] + (double)deltaZStored * m[8] + (double)deltaY * m[7]);

	out->x = newX;
	out->z = newZ;
	out->y = (float)newY;

	return out;
}

CVector *CNode::localToWorldDirection(const CVector *direction, CVector *out) {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	const float *m = world.rot.m;
	double newY = (double)m[1] * direction->x + (double)m[7] * direction->z + (double)m[4] * direction->y;
	float newZ = (float)((double)m[2] * direction->x + (double)m[8] * direction->z + (double)m[5] * direction->y);

	out->x = (float)((double)m[6] * direction->z + (double)m[3] * direction->y + (double)m[0] * direction->x);
	out->z = newZ;
	out->y = (float)newY;

	return out;
}

void CNode::updateWorldTransform() {
	CMatrix rotation;

	if ((nodeFlags & kQuaternionValid) != 0) {
		rotation.setFromQuaternion(quat);
		world.rot = rotation;
		world.pos.x = pos.x;
		world.pos.y = pos.y;
		world.pos.z = pos.z;
	} else if ((nodeFlags & kEulerValid) != 0) {
		rotation = *rotation.setFromEuler(eulerDeg);
		world.rot = rotation;
		world.pos.x = pos.x;
		world.pos.y = pos.y;
		world.pos.z = pos.z;
	}

	if (parent) {
		if ((parent->nodeFlags & kWorldValid) == 0)
			parent->updateWorldTransform();

		const float *p = parent->world.rot.m;
		const float *w = world.rot.m;
		const CVector &wp = world.pos;
		const CVector &pp = parent->world.pos;
		float posY = (float)((double)p[1] * wp.x + (double)wp.z * p[7] + (double)wp.y * p[4]);
		float posZ = (float)((double)wp.z * p[8] + (double)wp.y * p[5] + (double)wp.x * p[2]);
		double posX = (double)wp.z * p[6] + (double)p[0] * wp.x + (double)wp.y * p[3] + pp.x;
		float worldY = (float)((double)posY + pp.y);
		float worldZ = (float)((double)posZ + pp.z);

		rotation.m[0] = (float)((double)p[0] * w[0] + (double)p[6] * w[2] + (double)p[3] * w[1]);
		rotation.m[1] = (float)((double)p[1] * w[0] + (double)p[7] * w[2] + (double)p[4] * w[1]);
		rotation.m[2] = (float)((double)p[8] * w[2] + (double)p[5] * w[1] + (double)w[0] * p[2]);
		rotation.m[3] = (float)((double)p[6] * w[5] + (double)w[4] * p[3] + (double)w[3] * p[0]);
		rotation.m[4] = (float)((double)w[3] * p[1] + (double)w[5] * p[7] + (double)w[4] * p[4]);
		rotation.m[5] = (float)((double)w[5] * p[8] + (double)w[4] * p[5] + (double)w[3] * p[2]);
		rotation.m[6] = (float)((double)w[7] * p[3] + (double)p[0] * w[6] + (double)p[6] * w[8]);
		rotation.m[7] = (float)((double)p[7] * w[8] + (double)p[4] * w[7] + (double)w[6] * p[1]);
		rotation.m[8] = (float)((double)w[8] * p[8] + (double)w[7] * p[5] + (double)w[6] * p[2]);

		world.rot = rotation;
		world.pos.x = (float)posX;
		world.pos.y = worldY;
		world.pos.z = worldZ;
		nodeFlags |= kWorldValid;
	} else {
		nodeFlags |= kWorldValid;
	}
}

void CNode::computeLocalMatrixFromWorld() {
	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	if (parent) {
		const float *p = parent->world.rot.m;
		const float *w = world.rot.m;
		CMatrix local;

		local.m[0] = (float)((double)p[2] * w[2] + (double)p[1] * w[1] + (double)p[0] * w[0]);
		local.m[1] = (float)((double)p[5] * w[2] + (double)p[4] * w[1] + (double)p[3] * w[0]);
		local.m[2] = (float)((double)p[8] * w[2] + (double)p[7] * w[1] + (double)p[6] * w[0]);
		local.m[3] = (float)((double)p[0] * w[3] + (double)p[2] * w[5] + (double)p[1] * w[4]);
		local.m[4] = (float)((double)p[3] * w[3] + (double)p[5] * w[5] + (double)p[4] * w[4]);
		local.m[5] = (float)((double)p[6] * w[3] + (double)p[8] * w[5] + (double)p[7] * w[4]);
		local.m[6] = (float)((double)p[0] * w[6] + (double)p[2] * w[8] + (double)p[1] * w[7]);
		local.m[7] = (float)((double)p[3] * w[6] + (double)p[5] * w[8] + (double)p[4] * w[7]);
		local.m[8] = (float)((double)p[6] * w[6] + (double)p[8] * w[8] + (double)p[7] * w[7]);
		local.toEuler(&eulerDeg);
	} else {
		world.rot.toEuler(&eulerDeg);
	}

	nodeFlags |= kEulerValid;
}

void CNode::updateLocalQuaternion() {
	if ((nodeFlags & kEulerValid) != 0) {
		quat = CQuaternion(eulerDeg);
		nodeFlags |= kQuaternionValid;
	} else {
		if ((nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		if (parent) {
			const float *p = parent->world.rot.m;
			const float *w = world.rot.m;
			CMatrix local;

			local.m[0] = (float)((double)p[0] * w[0] + (double)p[2] * w[2] + (double)p[1] * w[1]);
			local.m[1] = (float)((double)p[3] * w[0] + (double)p[5] * w[2] + (double)p[4] * w[1]);
			local.m[2] = (float)((double)p[6] * w[0] + (double)p[8] * w[2] + (double)p[7] * w[1]);
			local.m[3] = (float)((double)p[0] * w[3] + (double)p[2] * w[5] + (double)p[1] * w[4]);
			local.m[4] = (float)((double)p[3] * w[3] + (double)p[5] * w[5] + (double)p[4] * w[4]);
			local.m[5] = (float)((double)p[6] * w[3] + (double)p[8] * w[5] + (double)p[7] * w[4]);
			local.m[6] = (float)((double)p[2] * w[8] + (double)p[1] * w[7] + (double)p[0] * w[6]);
			local.m[7] = (float)((double)p[5] * w[8] + (double)p[4] * w[7] + (double)p[3] * w[6]);
			local.m[8] = (float)((double)p[8] * w[8] + (double)p[7] * w[7] + (double)p[6] * w[6]);
			local.toQuaternion(&quat);
		} else {
			world.rot.toQuaternion(&quat);
		}

		nodeFlags |= kQuaternionValid;
	}
}

void CNode::computeLocalPositionFromWorld() {
	pos = world.pos;

	if (parent) {
		if ((parent->nodeFlags & kWorldValid) == 0)
			parent->updateWorldTransform();

		const float *p = parent->world.rot.m;
		double deltaX = (double)pos.x - parent->world.pos.x;
		float deltaY = (float)((double)pos.y - parent->world.pos.y);
		double deltaZ = (double)pos.z - parent->world.pos.z;
		float deltaZStored = (float)deltaZ;
		double newX = deltaZ * p[2] + (double)deltaY * p[1] + deltaX * p[0];
		float newY = (float)((double)deltaZStored * p[5] + deltaX * p[3] + (double)deltaY * p[4]);
		double partialZ = (double)deltaZStored * p[8] + deltaX * p[6];
		double partialY = (double)deltaY * p[7];

		pos.y = newY;
		float newZ = (float)(partialZ + partialY);
		pos.x = (float)newX;
		pos.z = newZ;
	}

	nodeFlags |= kPositionValid;
}

void CNode::updateChildrenLocalTransforms() {
	for (CNode *child = firstChild; child; child = child->nextSibling)
		child->updateLocalState();
}

void CNode::setParent(CNode *newParent) {
	if (!newParent) {
		warning("CNode::setParent(): null parent");

		return;
	}

	if (parent)
		detachFromParent();

	parent = newParent;
	prevSibling = nullptr;
	nextSibling = newParent->firstChild;

	if (nextSibling)
		nextSibling->prevSibling = this;

	parent->childCount++;
	parent->firstChild = this;
	updateLocalState();
}

void CNode::detachFromParent() {
	if (!parent)
		return;

	parent->childCount--;

	if (!prevSibling)
		parent->firstChild = nextSibling;

	if (prevSibling)
		prevSibling->nextSibling = nextSibling;

	if (nextSibling)
		nextSibling->prevSibling = prevSibling;

	nextSibling = nullptr;
	prevSibling = nullptr;
	parent = nullptr;
	updateLocalState();
}

CNode *CNode::findNode(const char *nodeName) {
	CNode *node = this;

	if (!scumm_strnicmp(name, nodeName, kNameSize))
		return node;

	while (true) {
		if (node->firstChild) {
			CNode *result = node->firstChild->findNode(nodeName);

			if (result)
				return result;
		}

		node = node->nextSibling;

		if (!node)
			return nullptr;

		if (!scumm_strnicmp(node->name, nodeName, kNameSize))
			return node;
	}
}

CNode *CNode::findNode(const LString &nodeName) {
	return findNode(nodeName.c_str());
}

void CNode::lookAt(const CVector &target) {
	CVector localTarget = target;

	if (parent) {
		if ((parent->nodeFlags & kWorldValid) == 0)
			parent->updateWorldTransform();

		const float *p = parent->world.rot.m;
		double deltaX = (double)localTarget.x - parent->world.pos.x;
		float deltaY = (float)((double)localTarget.y - parent->world.pos.y);
		double deltaZ = (double)localTarget.z - parent->world.pos.z;
		float deltaZStored = (float)deltaZ;
		float newY = (float)((double)deltaZStored * p[5] + (double)deltaY * p[4] + deltaX * p[3]);
		float newZ = (float)((double)deltaZStored * p[8] + (double)deltaY * p[7] + deltaX * p[6]);

		localTarget.y = newY;
		localTarget.x = (float)(deltaZ * p[2] + (double)deltaY * p[1] + deltaX * p[0]);
		localTarget.z = newZ;
	}

	if ((nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	CVector direction;

	direction.x = localTarget.x - pos.x;
	direction.y = localTarget.y - pos.y;
	direction.z = localTarget.z - pos.z;

	CVector unit = direction;
	double length = sqrt((double)direction.z * direction.z + (double)direction.y * direction.y + (double)direction.x * direction.x);

	if (length >= (double)kMinLength) {
		double inverse = 1.0 / length;

		unit.x = (float)(direction.x * inverse);
		unit.y = (float)(inverse * direction.y);
		unit.z = (float)(inverse * direction.z);
	}

	float factor = (float)((double)kHalfTurnDegrees / kPi);

	eulerDeg.z = 0.0f;

	float yaw = (float)(atan2(unit.x, unit.z) * factor);

	eulerDeg.x = (float)(-asin(unit.y) * factor);
	eulerDeg.y = yaw;
	nodeFlags = (nodeFlags & ~(kEulerValid | kQuaternionValid | kLocalSet)) | kEulerValid | kLocalSet;
	updateLocalState();
}

} // End of namespace Grim
