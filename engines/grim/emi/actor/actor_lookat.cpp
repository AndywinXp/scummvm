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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cplane.h"
#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/lconsoleservices.h"

namespace Grim {

static const float kLookAtMinLength = 1e-20f;
static const float kLookAtDotLimit = 0.9999f;

static void setJointQuaternion(CNode *joint, const CQuaternion &rotation) {
	joint->quat = rotation;
	joint->nodeFlags = (joint->nodeFlags & ~(uint)CNode::kEulerValid) | CNode::kQuaternionValid | CNode::kLocalSet;

	if (joint->nodeFlags & CNode::kWorldValid)
		joint->updateChildrenLocalTransforms();

	if ((joint->nodeFlags & CNode::kPositionValid) == 0)
		joint->computeLocalPositionFromWorld();

	if ((joint->nodeFlags & (CNode::kEulerValid | CNode::kQuaternionValid)) == 0)
		joint->updateLocalQuaternion();

	joint->nodeFlags = (joint->nodeFlags & ~(uint)CNode::kWorldValid) | CNode::kLocalSet;
}

bool Actor::setActorHead(LString jointName, float *params) {
	if (!extra)
		allocateActorExtra();

	extra->lookAtActive = false;
	extra->lookAtHasTarget = false;
	extra->lookJointName = "";
	extra->lookOffset.x = 0.0f;
	extra->lookOffset.y = 0.0f;
	extra->lookOffset.z = 0.0f;

	if (!jointName.empty()) {
		CNode *joint = findNode(jointName);

		if (!joint)
			return false;

		if ((joint->nodeFlags & CNode::kQuaternionValid) == 0)
			joint->updateLocalQuaternion();

		extra->lookGoalQuat = joint->quat;
		extra->lookCurQuat = extra->lookGoalQuat;
		extra->lookAtActive = true;
		extra->lookJointName = jointName;

		if ((nodeFlags & CNode::kWorldValid) == 0)
			updateWorldTransform();

		float forwardX = world.rot.m[6];
		float forwardY = world.rot.m[7];
		float forwardZ = world.rot.m[8];

		if ((joint->nodeFlags & CNode::kWorldValid) == 0)
			joint->updateWorldTransform();

		extra->lookDirection.x = (float)((double)forwardX + joint->world.pos.x);
		extra->lookDirection.y = (float)((double)forwardY + joint->world.pos.y);
		extra->lookDirection.z = (float)((double)forwardZ + joint->world.pos.z);
		joint->worldToLocal(&extra->lookDirection);

		CVector &direction = extra->lookDirection;
		double length = sqrt(((double)direction.x * direction.x + (double)direction.y * direction.y) + (double)direction.z * direction.z);

		if (length >= (double)kLookAtMinLength) {
			double inverse = 1.0 / length;

			direction.x = (float)(inverse * direction.x);
			direction.y = (float)(inverse * direction.y);
			direction.z = (float)(inverse * direction.z);
		}

		if (!params) {
			warning("Actor::setActorHead(): no offset given");

			return true;
		}

		extra->lookOffset.x = params[0];
		extra->lookOffset.y = params[1];
		extra->lookOffset.z = params[2];
	}

	return true;
}

bool Actor::setHeadLimits(float horizontalLimit, float upLimit, float downLimit) {
	double halfAngle = (double)horizontalLimit * 0.5;
	float halfCos = (float)cos(halfAngle);
	float halfSin = (float)sin(halfAngle);

	if (!extra)
		allocateActorExtra();

	extra->lookLimits[0].x = halfCos;
	extra->lookLimits[0].y = 0.0f;
	extra->lookLimits[0].z = halfSin;
	extra->lookLimits[1].x = -halfCos;
	extra->lookLimits[1].y = 0.0f;
	extra->lookLimits[1].z = halfSin;
	extra->lookLimits[2].x = 0.0f;
	extra->lookLimits[2].y = -(float)cos((double)upLimit);
	extra->lookLimits[2].z = (float)sin((double)upLimit);
	extra->lookLimits[3].x = 0.0f;
	extra->lookLimits[3].y = (float)cos((double)downLimit);
	extra->lookLimits[3].z = (float)sin((double)downLimit);

	return true;
}

bool Actor::canActorSeePoint(const float *point, float *out) {
	if (!extra)
		return false;

	if (!point || !out) {
		warning("Actor::canActorSeePoint(): missing point or output");

		return false;
	}

	if ((nodeFlags & CNode::kWorldValid) == 0)
		updateWorldTransform();

	float forwardX = world.rot.m[6];
	float forwardZ = world.rot.m[8];
	double forwardLength = sqrt((double)world.rot.m[8] * world.rot.m[8] + (double)world.rot.m[6] * world.rot.m[6]);

	if (forwardLength >= (double)kLookAtMinLength) {
		double inverse = 1.0 / forwardLength;

		forwardX = (float)((double)world.rot.m[6] * inverse);
		forwardZ = (float)((double)world.rot.m[8] * inverse);
	}

	if ((nodeFlags & CNode::kWorldValid) == 0)
		updateWorldTransform();

	float deltaX = (float)((double)point[0] - world.pos.x);
	double deltaY = (double)point[1] - world.pos.y;
	float deltaZ = (float)((double)point[2] - world.pos.z);
	double distance = sqrt(((double)deltaZ * deltaZ + deltaY * deltaY) + (double)deltaX * deltaX);
	float directionX = deltaX;
	float directionZ = deltaZ;

	if (distance >= (double)kLookAtMinLength) {
		double inverse = 1.0 / distance;

		directionX = (float)((double)deltaX * inverse);
		directionZ = (float)((double)deltaZ * inverse);
		*out = (float)distance;
	} else {
		*out = 0.0f;
	}

	double horizontal = sqrt((double)directionZ * directionZ + (double)directionX * directionX);
	float horizontalStored = (float)horizontal;
	double normalizedX = directionX;
	float normalizedZ = directionZ;

	if (horizontal >= (double)kLookAtMinLength) {
		double inverse = 1.0 / (double)horizontalStored;
		float inverseStored = (float)inverse;

		normalizedX = inverse * (double)directionX;
		normalizedZ = (float)((double)directionZ * inverseStored);
	}

	double dot = ((double)forwardX * normalizedX + (double)normalizedZ * (double)forwardZ) + 0.0 * 0.0;

	return dot >= (double)extra->lookCosFov;
}

bool Actor::canActorSeeActor(Actor *other, float *out) {
	if (!other) {
		warning("Actor::canActorSeeActor(): no actor");

		return false;
	}

	if (!other->room.resource)
		return false;

	if (!room.resource)
		return false;

	if (room.resource->name != other->room.resource->name)
		return false;

	if ((other->nodeFlags & CNode::kWorldValid) == 0)
		other->updateWorldTransform();

	return canActorSeePoint(&other->world.pos.x, out);
}

void Actor::setLookAtJoints(float deltaTime) {
	CQuaternion slerped;
	CQuaternion target;

	if (!extra)
		return;

	if (extra->lookTargetActorId != 0) {
		Actor *targetActor = actorDeref(extra->lookTargetActorId);

		if (targetActor) {
			if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
				targetActor->updateWorldTransform();

			extra->lookTarget.x = targetActor->world.pos.x;
			extra->lookTarget.y = targetActor->world.pos.y;
			extra->lookTarget.z = targetActor->world.pos.z;
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("SetLookAtJoints trying to look at deleted actor!!");
			else
				warning("SetLookAtJoints trying to look at deleted actor!!");

			extra->lookTargetActorId = 0;
			extra->lookAtHasTarget = false;
		}
	}

	CNode *joint = findNode(extra->lookJointName);

	if (!joint)
		return;

	if ((joint->nodeFlags & CNode::kQuaternionValid) == 0)
		joint->updateLocalQuaternion();

	CQuaternion jointQuat = joint->quat;
	CVector direction = extra->lookDirection;

	if (extra->lookAtHasTarget) {
		CVector aim;
		aim.x = (float)((double)extra->lookOffset.x + extra->lookTarget.x);
		aim.y = (float)((double)extra->lookOffset.y + extra->lookTarget.y);
		aim.z = (float)((double)extra->lookOffset.z + extra->lookTarget.z);

		CVector toTarget = aim;
		joint->worldToLocal(&toTarget);

		CPlane plane;
		plane.n.x = 0.0f;
		plane.n.y = 0.0f;
		plane.n.z = 0.0f;
		plane.d = 0.0f;

		CVector worldLimit;

		for (int i = 0; i < Extra_v2::kNumLookLimits; i++) {
			localToWorldDirection(&extra->lookLimits[i], &worldLimit);

			if ((joint->nodeFlags & CNode::kWorldValid) == 0)
				joint->updateWorldTransform();

			plane.setFromNormalAndPoint(worldLimit, joint->world.pos);

			if ((joint->nodeFlags & CNode::kWorldValid) == 0)
				joint->updateWorldTransform();

			plane.transformToLocal(&joint->world);

			if (plane.classifyPoint(toTarget) == CPlane::kPointBehind)
				plane.projectPoint(&toTarget);
		}

		double targetLength = sqrt(((double)toTarget.x * toTarget.x + (double)toTarget.y * toTarget.y) + (double)toTarget.z * toTarget.z);

		if (targetLength >= (double)kLookAtMinLength) {
			double inverse = 1.0 / targetLength;

			toTarget.x = (float)(inverse * toTarget.x);
			toTarget.y = (float)(inverse * toTarget.y);
			toTarget.z = (float)(inverse * toTarget.z);
		}

		float cosine = (float)(((double)toTarget.z * direction.z + (double)toTarget.y * direction.y) + (double)direction.x * toTarget.x);
		CVector cross;
		cross.x = (float)((double)toTarget.z * direction.y - (double)toTarget.y * direction.z);
		cross.y = (float)((double)direction.z * toTarget.x - (double)toTarget.z * direction.x);
		cross.z = (float)((double)toTarget.y * direction.x - (double)direction.y * toTarget.x);

		CVector axis = cross;
		double crossLength = sqrt(((double)cross.z * cross.z + (double)cross.y * cross.y) + (double)cross.x * cross.x);

		if (crossLength >= (double)kLookAtMinLength) {
			double inverse = 1.0 / crossLength;

			axis.x = (float)((double)cross.x * inverse);
			axis.y = (float)((double)axis.y * inverse);
			axis.z = (float)((double)axis.z * inverse);
		}

		float angle = (float)acos((double)cosine);
		CQuaternion rotation(axis, angle);
		const CQuaternion &goal = extra->lookGoalQuat;
		float resultW = (float)((((double)rotation.w * goal.w - (double)goal.x * rotation.x) - (double)goal.y * rotation.y) - (double)goal.z * rotation.z);
		float resultZ = (float)((((double)goal.x * rotation.y + (double)rotation.w * goal.z) + (double)rotation.z * goal.w) - (double)goal.y * rotation.x);
		float resultY = (float)((((double)goal.z * rotation.x + (double)rotation.w * goal.y) + (double)rotation.y * goal.w) - (double)goal.x * rotation.z);
		float resultX = (float)((((double)rotation.w * goal.x + (double)goal.y * rotation.z) + (double)rotation.x * goal.w) - (double)goal.z * rotation.y);

		target = CQuaternion(resultX, resultY, resultZ, resultW);
	} else {
		target = jointQuat;
	}

	float step = (float)((double)deltaTime * extra->lookRate);
	float dot = extra->lookCurQuat.dot(target);

	// The disasm clamps the dot product to the open interval
	if (dot > kLookAtDotLimit)
		dot = kLookAtDotLimit;
	else if (!(dot >= -kLookAtDotLimit))
		dot = -kLookAtDotLimit;

	double span = acos((double)dot);
	span = span + span;

	if (span >= (double)step) {
		float fraction = (float)((double)step / span);

		extra->lookCurQuat.slerp(target, fraction, &slerped);
		slerped.normalize();
		setJointQuaternion(joint, slerped);
		extra->lookCurQuat = slerped;
	} else {
		setJointQuaternion(joint, target);
		extra->lookCurQuat = target;
	}
}

} // End of namespace Grim
