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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/actor/puck_v2.h"
#include "engines/grim/emi/scene/walkplane_v2.h"
#include "engines/grim/emi/scene/walkplanecollection.h"

namespace Grim {

enum {
	kTypeWalk = 0x1000,
	kTypeFunnel = 0x100,
	kTypeConstraint = 0x1200,
	kMaxRecursionDepth = 8
};

static const float kMinLength = 1e-20f;
static const float kFullTurnDegrees = 360.0f;
static const float kHalfTurnDegrees = 180.0f;
static const float kQuarterTurnDegrees = 90.0f;
static const float kMinConstraintAlignment = 0.5f;
static const float kFunnelAngleLimit = 3.0f;

Puck_v2::Puck_v2(EmiEngine *engine, float newReflectionAngle) : _engine(engine) {
	constraint[0] = 0.0f;
	constraint[1] = 0.0f;
	constraint[2] = 0.0f;
	lineA[0] = 0.0f;
	lineA[1] = 0.0f;
	lineA[2] = 0.0f;
	lineB[0] = 0.0f;
	lineB[1] = 0.0f;
	lineB[2] = 0.0f;
	planes = nullptr;
	reset();
	reflectionAngle = newReflectionAngle;
}

Puck_v2::~Puck_v2() {
}

void Puck_v2::reset() {
	plane = nullptr;
	constrained = false;
	constraint[0] = 0.0f;
	constraint[1] = 1.0f;
	constraint[2] = 0.0f;
}

void Puck_v2::validate(WalkPlaneCollection *collection) {
	planes = collection;

	if (!plane) {
		if (!collection) {
			warning("Puck_v2::validate(): missing collection");

			return;
		}

		plane = collection->planeClosest(&pos.x, kTypeWalk);
	}

	if (plane && !plane->within(&pos.x, true)) {
		plane->closestPoint(&pos.x);

		CVector newPosition = pos;

		setLocalPosition(newPosition);
	}
}

void Puck_v2::rotate(float angle) {
	if ((nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	eulerDeg.y = (float)((double)eulerDeg.y + angle);
	nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
	updateLocalState();
}

float Puck_v2::pointAt(const float *target, float maxAngle) {
	float angle;

	if (getYawToPoint(target) > maxAngle)
		angle = maxAngle;
	else
		angle = getYawToPoint(target);

	rotate(angle);

	return angle;
}

float Puck_v2::getYawToPoint(const float *target) {
	if ((nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	double yaw = eulerDeg.y;
	float storedYaw = eulerDeg.y;

	if (yaw - yaw != 0.0) {
		warning("Puck_v2::getYawToPoint(): invalid yaw");
		yaw = 0.0;
		storedYaw = 0.0f;
	}

	if (yaw < 0.0) {
		do {
			yaw += kFullTurnDegrees;
		} while (yaw < 0.0);

		storedYaw = (float)yaw;
	}

	if (!(yaw < kFullTurnDegrees)) {
		do {
			yaw -= kFullTurnDegrees;
		} while (yaw >= kFullTurnDegrees);

		storedYaw = (float)yaw;
	}

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	float deltaX = (float)((double)target[0] - world.pos.x);
	float deltaZ = (float)((double)target[2] - world.pos.z);
	float normalizedX = deltaX;
	float normalizedZ = deltaZ;
	double length = sqrt((double)deltaX * deltaX + (double)deltaZ * deltaZ);

	if (length >= kMinLength) {
		double inverse = 1.0 / length;

		normalizedX = (float)(deltaX * inverse);
		normalizedZ = (float)(inverse * deltaZ);
	}

	double angle = acos((double)normalizedZ) * WalkPlane_v2::kRadiansToDegrees;

	if (normalizedX < 0.0f)
		angle = kFullTurnDegrees - angle;

	angle -= storedYaw;

	if (angle > kHalfTurnDegrees)
		return (float)-(kFullTurnDegrees - angle);

	return (float)angle;
}

float *Puck_v2::closestPointOnConstraintLine(float *out, const float *point) {
	float pointX = point[0];
	float pointZ = point[2];
	double pointDeltaX = (double)pointX - lineA[0];
	double negativeLineAY = -(double)lineA[1];
	double pointDeltaZ = (double)pointZ - lineA[2];
	float lineDeltaX = (float)((double)lineB[0] - lineA[0]);
	float lineDeltaY = (float)((double)lineB[1] - lineA[1]);
	double lineDeltaZExact = (double)lineB[2] - lineA[2];
	float lineDeltaZ = (float)lineDeltaZExact;
	double dot = (lineDeltaZExact * pointDeltaZ + (double)lineDeltaY * negativeLineAY) + (double)lineDeltaX * pointDeltaX;
	double lengthSquared = ((double)lineDeltaZ * lineDeltaZ + (double)lineDeltaY * lineDeltaY) + (double)lineDeltaX * lineDeltaX;
	double ratio;

	if (lengthSquared == 0.0) {
		// The original could divide by zero here, let's avoid that...
		warning("Puck_v2::closestPointOnConstraintLine(): degenerate constraint line");
		ratio = 0.0;
	} else {
		ratio = dot / lengthSquared;
	}

	float scaledX = (float)(lineDeltaX * ratio);
	float scaledY = (float)(lineDeltaY * ratio);
	double scaledZ = ratio * lineDeltaZ;

	out[0] = (float)((double)scaledX + lineA[0]);
	out[1] = (float)((double)scaledY + lineA[1]);
	out[2] = (float)(scaledZ + lineA[2]);

	return out;
}

bool Puck_v2::forward(float distance, int allowLeft, int allowRight) {
	float target[3] = { 0.0f, 0.0f, 0.0f };
	float hit[3] = { 0.0f, 0.0f, 0.0f };

	// The disasm ignores allowLeft and allowRight

	_engine->_puckRecursionDepth++;

	if (constrained) {
		if (!constrain(nullptr))
			return false;
	}

	if (!plane) {
		if ((nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		float forwardX = world.rot.m[6];
		float forwardY = world.rot.m[7];
		float forwardZ = world.rot.m[8];
		float step[3];

		step[0] = (float)((double)forwardX * distance);
		step[1] = (float)((double)forwardY * distance);
		step[2] = (float)((double)forwardZ * distance);

		if ((nodeFlags & kWorldValid) == 0)
			updateWorldTransform();

		CVector newPosition;

		newPosition.y = (float)((double)step[1] + world.pos.y);
		newPosition.x = (float)((double)step[0] + world.pos.x);
		newPosition.z = (float)((double)step[2] + world.pos.z);
		setLocalPosition(newPosition);
		_engine->_puckRecursionDepth = 0;

		return true;
	}

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	float forwardX = world.rot.m[6];
	float forwardY = world.rot.m[7];
	float forwardZ = world.rot.m[8];

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	float ahead[3];
	float step[3];

	ahead[0] = (float)((double)forwardX + world.pos.x);
	ahead[1] = (float)((double)forwardY + world.pos.y);
	ahead[2] = (float)((double)forwardZ + world.pos.z);
	plane->project(ahead);

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	ahead[0] = (float)((double)ahead[0] - world.pos.x);
	ahead[1] = (float)((double)ahead[1] - world.pos.y);
	ahead[2] = (float)((double)ahead[2] - world.pos.z);

	double length = sqrt(((double)ahead[0] * ahead[0] + (double)ahead[2] * ahead[2]) + (double)ahead[1] * ahead[1]);

	if (length >= kMinLength) {
		double inverse = 1.0 / length;

		ahead[0] = (float)(inverse * ahead[0]);
		ahead[1] = (float)(ahead[1] * inverse);
		ahead[2] = (float)(ahead[2] * inverse);
	}

	step[0] = (float)((double)ahead[0] * distance);
	step[1] = (float)((double)ahead[1] * distance);
	step[2] = (float)((double)ahead[2] * distance);

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	target[0] = (float)((double)step[0] + world.pos.x);
	target[1] = (float)((double)step[1] + world.pos.y);
	target[2] = (float)((double)step[2] + world.pos.z);

	if (constrained) {
		float *closest = closestPointOnConstraintLine(step, target);

		target[0] = closest[0];
		target[1] = closest[1];
		target[2] = closest[2];
		plane->project(target);
	}

	if (plane->within(target, true)) {
		plane->project(target);

		CVector newPosition;

		newPosition.x = target[0];
		newPosition.y = target[1];
		newPosition.z = target[2];
		setLocalPosition(newPosition);
		_engine->_puckRecursionDepth = 0;

		return true;
	}

	if (_engine->_puckRecursionDepth > kMaxRecursionDepth) {
		_engine->_puckRecursionDepth = 0;

		return false;
	}

	WalkPlane_v2 *oldPlane = plane;
	float angle = oldPlane->intersection(&pos.x, target, hit);

	oldPlane->enabled = false;

	CVector hitLocal;

	hitLocal.x = hit[0];
	hitLocal.y = hit[1];
	hitLocal.z = hit[2];
	worldToLocal(&hitLocal);

	float remaining = (float)((double)distance - hitLocal.z);
	WalkPlane_v2 *newPlane = nullptr;

	if (planes)
		newPlane = planes->planeAt(hit, kTypeWalk);
	else
		warning("Puck_v2::forward(): missing collection");

	CVector hitPosition;

	hitPosition.x = hit[0];
	hitPosition.y = hit[1];
	hitPosition.z = hit[2];

	if (newPlane) {
		setLocalPosition(hitPosition);
		plane = newPlane;

		if ((newPlane->type & kTypeConstraint) == kTypeConstraint) {
			float *constraintVector = newPlane->getConstraintVector(this);

			if (constraintVector) {
				constrain(constraintVector);
				constrained = true;
			}

			oldPlane->enabled = true;

			return false;
		}

		constrained = false;

		bool result = forward(remaining, 1, 1);

		oldPlane->enabled = true;

		return result;
	}

	if ((nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	setLocalPosition(hitPosition);

	if ((plane->type & kTypeFunnel) == 0) {
		rotate(angle);
		oldPlane->enabled = true;
	} else {
		int axis = plane->quadAxis;

		if (plane->numVerts <= 0 || axis < 0 || axis >= plane->numVerts) {
			warning("Puck_v2::forward(): invalid funnel (axis %d of %d)", axis, plane->numVerts);

			return false;
		}

		if ((axis + 1) % plane->numVerts == plane->hitEdge)
			return false;

		CVector corner;

		corner.x = plane->verts[axis]->x;
		corner.y = plane->verts[axis]->y;
		corner.z = plane->verts[axis]->z;
		worldToLocal(&corner);

		if (!(corner.x > 0.0f)) {
			if (angle < kFunnelAngleLimit)
				angle = (float)((double)angle + kQuarterTurnDegrees + kQuarterTurnDegrees);
		} else if (angle > kFunnelAngleLimit) {
			angle = (float)(-(double)kQuarterTurnDegrees - ((double)kQuarterTurnDegrees - angle));
		}

		rotate(angle);
	}

	bool result = forward(remaining, 1, 1);

	oldPlane->enabled = true;

	return result;
}

bool Puck_v2::constrain(const float *vector) {
	if (vector) {
		constraint[0] = vector[0];
		constraint[1] = vector[1];
		constraint[2] = vector[2];
		constraint[1] = 0.0f;

		double length = sqrt(((double)constraint[0] * constraint[0] + (double)constraint[1] * constraint[1]) + (double)constraint[2] * constraint[2]);

		if (length >= kMinLength) {
			double inverse = 1.0 / length;

			constraint[0] = (float)(inverse * constraint[0]);
			constraint[1] = (float)(inverse * constraint[1]);
			constraint[2] = (float)(inverse * constraint[2]);
		}

		constrained = true;
	}

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	double dot = ((double)world.rot.m[8] * constraint[2] + (double)world.rot.m[7] * constraint[1]) + (double)world.rot.m[6] * constraint[0];
	float storedDot = (float)dot;

	if (dot < 0.0)
		dot = -dot;

	if (dot < kMinConstraintAlignment)
		return false;

	double angle = acos((double)constraint[2]) * WalkPlane_v2::kRadiansToDegrees;

	if (constraint[0] < 0.0f)
		angle = kFullTurnDegrees - angle;

	if (!(storedDot > 0.0f))
		angle += kHalfTurnDegrees;

	eulerDeg.y = (float)angle;
	eulerDeg.x = 0.0f;
	eulerDeg.z = 0.0f;
	nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
	updateLocalState();

	return true;
}

} // End of namespace Grim
