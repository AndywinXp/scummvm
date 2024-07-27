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

#include "scumm/he/intern_he.h"
#include "scumm/he/basketball/basketball.h"
#include "scumm/he/basketball/collision.h"
#include "scumm/he/basketball/geotypes.h"

namespace Scumm {

float U32copysign(float x, float y) {
	return ((x < 0) == (y < 0)) ? x : -x;
}

CCollisionBasketball::CCollisionBasketball() {
}

CCollisionBasketball::~CCollisionBasketball() {
}

bool CCollisionBasketball::TestCatch(const ICollisionObject &targetObject, U32Distance3D *pDistance) {
	if (targetObject.m_objectType == kPlayer) {
		ICollisionObject *pObject = const_cast<ICollisionObject *>(&targetObject);
		CCollisionPlayer *pPlayer = static_cast<CCollisionPlayer *>(pObject);
		return (pPlayer->TestCatch(*(ICollisionObject *)this, pDistance));
	} else {
		return false;
	}
}

CCollisionBox::CCollisionBox() : ICollisionObject(kBox) {
}

CCollisionBox::~CCollisionBox() {
}

U32FltPoint3D CCollisionBox::FindNearestPoint(const U32FltPoint3D &testPoint) const {
	U32FltPoint3D boxPoint;

	for (int i = X_INDEX; i <= Z_INDEX; i++) {
		EDimension dimension = (EDimension)i;

		if (testPoint[dimension] < m_minPoint[dimension]) {
			boxPoint[dimension] = m_minPoint[dimension];
		} else if (testPoint[dimension] > m_maxPoint[dimension]) {
			boxPoint[dimension] = m_maxPoint[dimension];
		} else {
			boxPoint[dimension] = testPoint[dimension];
		}
	}

	return boxPoint;
}

U32BoundingBox CCollisionBox::GetBoundingBox(void) const {
	return *this;
}

U32BoundingBox CCollisionBox::GetBigBoundingBox(void) const {
	return *this;
}

CCollisionCylinder::CCollisionCylinder() : ICollisionObject(kCylinder),
										   m_height(0),
										   m_shieldRadius(0),
										   m_bPositionSaved(false) {}

CCollisionCylinder::~CCollisionCylinder() {}


float CCollisionCylinder::
	GetDimensionDistance(const CCollisionBox &targetObject,
						 EDimension dimension) const {
	if (m_center[dimension] < targetObject.m_minPoint[dimension]) {
		return (m_center[dimension] - targetObject.m_minPoint[dimension]);
	} else if (m_center[dimension] > targetObject.m_maxPoint[dimension]) {
		return (m_center[dimension] - targetObject.m_maxPoint[dimension]);
	} else {
		return 0;
	}
}

float CCollisionCylinder::
	GetDimensionDistance(const CCollisionSphere &targetObject, EDimension dimension) const {
	float centerDistance = m_center[dimension] - targetObject.m_center[dimension];

	if (dimension == Z_INDEX) {
		if (centerDistance < -(targetObject.m_radius)) {
			return (centerDistance + targetObject.m_radius);
		} else if (centerDistance > (targetObject.m_radius)) {
			return (centerDistance - targetObject.m_radius);
		} else {
			return 0;
		}
	} else {
		return centerDistance;
	}
}

float CCollisionCylinder::
	GetDimensionDistance(const CCollisionCylinder &targetObject, EDimension dimension) const {
	float centerDistance = m_center[dimension] - targetObject.m_center[dimension];

	if (dimension == Z_INDEX) {
		if (centerDistance < -(targetObject.m_height / 2)) {
			return (centerDistance + (targetObject.m_height / 2));
		} else if (centerDistance > (targetObject.m_height / 2)) {
			return (centerDistance - (targetObject.m_height / 2));
		} else {
			return 0;
		}
	} else {
		return centerDistance;
	}
}

float CCollisionCylinder::
	GetObjectDistance(const CCollisionSphere &targetObject) const {
	U32Distance3D distance;

	distance.x = GetDimensionDistance(targetObject, X_INDEX);
	distance.y = GetDimensionDistance(targetObject, Y_INDEX);
	distance.z = GetDimensionDistance(targetObject, Z_INDEX);

	float xyDistance = distance.XYMagnitude() - m_radius - targetObject.m_radius;
	if (xyDistance < 0)
		xyDistance = 0;

	float zDistance = fabs(distance.z) - (m_height / 2) - targetObject.m_radius;
	if (zDistance < 0)
		zDistance = 0;

	float totalDistance = sqrt((xyDistance * xyDistance) + (zDistance * zDistance));
	return (totalDistance);
}

float CCollisionCylinder::
	GetObjectDistance(const CCollisionBox &targetObject) const {
	U32Distance3D distance;

	distance.x = GetDimensionDistance(targetObject, X_INDEX);
	distance.y = GetDimensionDistance(targetObject, Y_INDEX);
	distance.z = GetDimensionDistance(targetObject, Z_INDEX);

	float xyDistance = distance.XYMagnitude() - m_radius;
	if (xyDistance < 0)
		xyDistance = 0;

	float zDistance = fabs(distance.z) - (m_height / 2);
	if (zDistance < 0)
		zDistance = 0;

	float totalDistance = sqrt((xyDistance * xyDistance) + (zDistance * zDistance));
	return (totalDistance);
}

float CCollisionCylinder::
	GetObjectDistance(const CCollisionCylinder &targetObject) const {
	U32Distance3D distance;

	distance.x = GetDimensionDistance(targetObject, X_INDEX);
	distance.y = GetDimensionDistance(targetObject, Y_INDEX);
	distance.z = GetDimensionDistance(targetObject, Z_INDEX);

	float xyDistance = distance.XYMagnitude() - m_radius - targetObject.m_radius;
	if (xyDistance < 0)
		xyDistance = 0;

	float zDistance = fabs(distance.z) - (m_height / 2) - (targetObject.m_height / 2);
	if (zDistance < 0)
		zDistance = 0;

	float totalDistance = sqrt((xyDistance * xyDistance) + (zDistance * zDistance));
	return (totalDistance);
}

bool CCollisionCylinder::TestObjectIntersection(const CCollisionSphere &targetObject,
												U32Distance3D *pDistance) const {
	// Get the distance between the ball and the cylinder
	pDistance->x = GetDimensionDistance(targetObject, X_INDEX);
	pDistance->y = GetDimensionDistance(targetObject, Y_INDEX);
	pDistance->z = GetDimensionDistance(targetObject, Z_INDEX);
	;

	if (pDistance->XYMagnitude() < (m_radius + targetObject.m_radius)) {
		return (fabs(pDistance->z) < (m_height / 2));
	} else {
		return false;
	}
}

bool CCollisionCylinder::TestObjectIntersection(const CCollisionBox &targetObject,
												U32Distance3D *pDistance) const {
	// Get the distance between the ball and the cylinder
	pDistance->x = GetDimensionDistance(targetObject, X_INDEX);
	pDistance->y = GetDimensionDistance(targetObject, Y_INDEX);
	pDistance->z = GetDimensionDistance(targetObject, Z_INDEX);
	;

	if (pDistance->XYMagnitude() < m_radius) {
		return (fabs(pDistance->z) < (m_height / 2));
	} else {
		return false;
	}
}

bool CCollisionCylinder::TestObjectIntersection(const CCollisionCylinder &targetObject,
												U32Distance3D *pDistance) const {
	// Get the distance between the ball and the cylinder
	pDistance->x = GetDimensionDistance(targetObject, X_INDEX);
	pDistance->y = GetDimensionDistance(targetObject, Y_INDEX);
	pDistance->z = GetDimensionDistance(targetObject, Z_INDEX);

	if (pDistance->XYMagnitude() < (m_radius + targetObject.m_radius)) {
		return (fabs(pDistance->z) < (m_height / 2));
	} else {
		return false;
	}
}

float CCollisionCylinder::
	GetPenetrationTime(const CCollisionBox &targetObject,
					   const U32Distance3D &distance,
					   EDimension dimension) const {
	float collisionDepth;

	if (dimension == Z_INDEX) {
		if (distance[dimension] > 0) {
			collisionDepth = (m_height / 2) - distance[dimension];
		} else if (distance[dimension] < 0) {
			collisionDepth = -(m_height / 2) - distance[dimension];
		} else {
			collisionDepth = 0;
		}
	} else {
		if (distance[dimension] > 0) {
			collisionDepth = m_radius - distance[dimension];
		} else if (distance[dimension] < 0) {
			collisionDepth = -m_radius - distance[dimension];
		} else {
			return 0;
		}
	}

	float tFinal = (m_velocity[dimension] == 0) ? 0 : (collisionDepth / -m_velocity[dimension]);

	return tFinal;
}

float CCollisionCylinder::
	GetPenetrationTime(const CCollisionCylinder &targetObject,
					   const U32Distance3D &distance,
					   EDimension dimension) const {
	float collisionDepth;

	if (dimension == Z_INDEX) {
		if (distance[dimension] > 0) {
			collisionDepth = (m_height / 2) - distance[dimension];
		} else if (distance[dimension] < 0) {
			collisionDepth = (m_height / 2) + distance[dimension];
		} else {
			collisionDepth = 0;
		}
	} else {
		if (distance[dimension] > 0) {
			collisionDepth = m_radius + targetObject.m_radius - distance[dimension];
		} else if (distance[dimension] < 0) {
			collisionDepth = -m_radius - targetObject.m_radius - distance[dimension];
		} else {
			collisionDepth = 0;
		}
	}

	float tFinal = (m_velocity[dimension] == 0) ? 0 : (collisionDepth / -m_velocity[dimension]);

	return tFinal;
}


bool CCollisionCylinder::ValidateCollision(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) {
	// This is kind of a hack.
	// I'm having a problem where somtimes when a player is running into another
	// player and jumps, they pop to the top of the player they were running into
	// and stand on their head.  This fixes that problem.
	float zCollisionTime = GetPenetrationTime(targetObject, *pDistance, Z_INDEX);
	if (((zCollisionTime > 1) || (zCollisionTime == 0)) &&
		(m_velocity.XYMagnitude() == 0) &&
		(m_velocity.z != 0)) {
		ForceOutOfObject(targetObject, pDistance);
		return m_ignore;
	} else {
		// If a player raises their shields and another player is standing inside
		// the shields, that player should be pushed out.
		if ((m_velocity.Magnitude() == 0) &&
			(m_shieldRadius == 0) &&
			(targetObject.m_shieldRadius != 0)) {
			return true;
		} else {
			// See if we're hitting the top or bottom of this.
			if (((pDistance->z > 0) && (m_velocity.z <= 0)) ||
				((pDistance->z < 0) && (m_velocity.z >= 0))) {
				return true;
			} else {
				// Create a vector from the center of the target cylinder to the center of
				// this cylinder.
				U32FltVector2D centerVector = targetObject.m_center - m_center;

				float aMag = m_velocity.Magnitude();
				float bMag = centerVector.Magnitude();
				float aDotb = centerVector * m_velocity;

				if (aMag == 0) {
					// If this object isn't moving, this can't be a valid collision.
					return false;
				}

				if (bMag == 0) {
					// bMag is 0, it is possible that this sphere penetrated too far into the target
					// object.  If this is the case, we'll go ahead and validate the collision.
					return true;
				}

				double angleCosine = aDotb / (aMag * bMag);

				if (angleCosine > 0) {
					return true;
				} else {
					return false;
				}
			}
		}
	}
}

bool CCollisionCylinder::BackOutOfObject(const CCollisionBox &targetObject,
										 U32Distance3D *pDistance,
										 float *pTimeUsed) {
	// See if this is an object we can simply step up onto.
	if (((m_height / 2) - pDistance->z) <= MAX_STEP_HEIGHT) {
		m_center.z += ((m_height / 2) - pDistance->z);
		return true;
	} else if (m_movementType == kCircular) {
		// Since we are moving with a circular motion, try circling out of the object
		if (CircleOutOfObject(targetObject, pDistance, pTimeUsed)) {
			return true;
		} else {
			// If we aren't able to circle out of the object, back out in a straight line
			m_movementType = kStraight;
			return BackStraightOutOfObject(targetObject, pDistance, pTimeUsed);
		}
	} else {
		m_movementType = kStraight;
		return BackStraightOutOfObject(targetObject, pDistance, pTimeUsed);
	}
}

bool CCollisionCylinder::BackOutOfObject(const CCollisionCylinder &targetObject,
										 U32Distance3D *pDistance,
										 float *pTimeUsed) {
	if (m_velocity.Magnitude() == 0) {
		ForceOutOfObject(targetObject, pDistance);
		return true;
	} else if ((m_movementType == kCircular) &&
			   (&targetObject != m_pRevCenter)) {
		// Since we are moving with a circular motion, try circling out of the object
		if (CircleOutOfObject(targetObject, pDistance, pTimeUsed)) {
			return true;
		} else {
			// If we aren't able to circle out of the object, back out in a straight line
			m_movementType = kStraight;
			return BackStraightOutOfObject(targetObject, pDistance, pTimeUsed);
		}
	} else {
		m_movementType = kStraight;
		return BackStraightOutOfObject(targetObject, pDistance, pTimeUsed);
	}
}

bool CCollisionCylinder::
	BackStraightOutOfObject(const ICollisionObject &targetObject,
							U32Distance3D *pDistance,
							float *pTimeUsed) {
	if (m_velocity.Magnitude() == 0)
		return true;

	U32FltPoint3D startPosition = m_center;

	int loopCounter = 0;

	while (ICollisionObject::TestObjectIntersection(targetObject, pDistance)) {
		float collisionTimes[3];

		collisionTimes[X_INDEX] = ICollisionObject::GetPenetrationTime(targetObject, *pDistance, X_INDEX);
		collisionTimes[Y_INDEX] = ICollisionObject::GetPenetrationTime(targetObject, *pDistance, Y_INDEX);
		collisionTimes[Z_INDEX] = ICollisionObject::GetPenetrationTime(targetObject, *pDistance, Z_INDEX);

		Common::sort(collisionTimes, collisionTimes + Z_INDEX + 1);

		float collisionTime = SMALL_TIME_INCREMENT;
		if (collisionTimes[2] > 0)
			collisionTime = collisionTimes[2];
		if (collisionTimes[1] > 0)
			collisionTime = collisionTimes[1];
		if (collisionTimes[0] > 0)
			collisionTime = collisionTimes[0];

		*pTimeUsed += collisionTime;

		// If we take too long to back out, something is wrong.
		// Restore the object to an ok state.
		if ((*pTimeUsed > BACK_OUT_TIME_LIMIT) && (*pTimeUsed != collisionTime)) {
			debug("It took too long for one object to back out of another.  Ignore and U32 will attempt to correct.");
			m_center = startPosition;
			Restore();
			return false;
		}

		m_center.x -= (collisionTime * m_velocity.x);
		m_center.y -= (collisionTime * m_velocity.y);
		m_center.z -= (collisionTime * m_velocity.z);

		// Make doubly sure we don't loop forever.
		if (++loopCounter > 500)
			return false;
	}

	return true;
}

bool CCollisionCylinder::
	GetCornerIntersection(const CCollisionBox &targetObject,
						  const U32Distance3D &distance,
						  U32FltPoint2D *pIntersection) {
	float centerDistance = (m_center - m_revCenterPt).XYMagnitude();
	U32BoundingBox testBox = targetObject;
	U32FltPoint2D boxCorner;

	// Get the corner that we think we've collided with.
	if (distance.x < 0) {
		boxCorner.x = targetObject.m_minPoint.x;
		testBox.m_maxPoint.x += centerDistance;
	} else if (distance.x > 0) {
		boxCorner.x = targetObject.m_maxPoint.x;
		testBox.m_minPoint.x -= centerDistance;
	} else {
		return false;
	}

	if (distance.y < 0) {
		boxCorner.y = targetObject.m_minPoint.y;
		testBox.m_maxPoint.y += centerDistance;
	} else if (distance.y > 0) {
		boxCorner.y = targetObject.m_maxPoint.y;
		testBox.m_maxPoint.y -= centerDistance;
	} else {
		return false;
	}

	U32FltPoint2D point1;
	U32FltPoint2D point2;

	int pointCount = GetEquidistantPoint(boxCorner, m_radius,
										 m_revCenterPt, centerDistance,
										 &point1,
										 &point2);

	switch (pointCount) {
	case 0:
		return false;
		break;

	case 1:
		*pIntersection = point1;
		break;

	case 2:
		if (!testBox.IsPointWithin(point1)) {
			*pIntersection = point1;
		} else if (!testBox.IsPointWithin(point2)) {
			*pIntersection = point2;
		} else {
			return false;
		}
		break;
	}

	return true;
}

bool CCollisionCylinder::CircleOutOfObject(const CCollisionBox &targetObject,
										   U32Distance3D *pDistance,
										   float *pTimeUsed) {
	bool validXIntercept = true;
	bool validYIntercept = true;

	// If this object isn't moving, push it out of the collision.
	if (m_velocity.XYMagnitude() == 0) {
		ForceOutOfObject(targetObject, pDistance);
		TestObjectIntersection(targetObject, pDistance);
		return false;
	}

	// Keep track of where we started.
	U32FltPoint3D oldCenter = m_center;
	U32FltVector3D oldVelocity = m_velocity;

	// Get the distance between the center of this cylinder and the point it is
	// revolving around
	float centerDistance = (m_center - m_revCenterPt).XYMagnitude();

	// Find out in which direction we are going to revolve around the target point.
	ERevDirection revDirection = (GetRevDirection() == kClockwise) ? kCounterClockwise : kClockwise;

	// Find where this cylinder would touch the box in the x dimension.
	U32FltPoint2D xIntercept;
	if (pDistance->y < 0) {
		xIntercept.y = targetObject.m_minPoint.y - m_radius;
		float yDist = xIntercept.y - m_revCenterPt.y;

		float xInterceptSquared = centerDistance * centerDistance - yDist * yDist;

		if (xInterceptSquared < 0) {
			validXIntercept = GetCornerIntersection(targetObject,
													*pDistance,
													&xIntercept);
		} else {
			if (revDirection == kClockwise) {
				xIntercept.x = sqrt(xInterceptSquared) + m_revCenterPt.x;
			} else {
				xIntercept.x = -sqrt(xInterceptSquared) + m_revCenterPt.x;
			}
		}
	} else if (pDistance->y > 0) {
		xIntercept.y = targetObject.m_maxPoint.y + m_radius;
		float yDist = xIntercept.y - m_revCenterPt.y;

		float xInterceptSquared = centerDistance * centerDistance - yDist * yDist;

		if (xInterceptSquared < 0) {
			validXIntercept = GetCornerIntersection(targetObject,
													*pDistance,
													&xIntercept);
		} else {
			if (revDirection == kClockwise) {
				xIntercept.x = -sqrt(xInterceptSquared) + m_revCenterPt.x;
			} else {
				xIntercept.x = sqrt(xInterceptSquared) + m_revCenterPt.x;
			}
		}
	} else {
		validXIntercept = false;
	}

	// If we found a valid place to back out to, try going there
	if (validXIntercept) {
		m_center.x = xIntercept.x;
		m_center.y = xIntercept.y;

		ForceOutOfObject(targetObject, pDistance);
		TestObjectIntersection(targetObject, pDistance);
		return true;
	}

	// Find where this cylinder would touch the box in the y dimension.
	U32FltPoint2D yIntercept;
	if (pDistance->x < 0) {
		yIntercept.x = targetObject.m_minPoint.x - m_radius;
		float xDist = yIntercept.x - m_revCenterPt.x;

		float yInterceptSquared = centerDistance * centerDistance - xDist * xDist;

		if (yInterceptSquared < 0) {
			validYIntercept = GetCornerIntersection(targetObject,
													*pDistance,
													&yIntercept);
		} else {
			if (revDirection == kClockwise) {
				yIntercept.y = -sqrt(yInterceptSquared) + m_revCenterPt.y;
			} else {
				yIntercept.y = sqrt(yInterceptSquared) + m_revCenterPt.y;
			}
		}
	} else if (pDistance->x > 0) {
		yIntercept.x = targetObject.m_maxPoint.x + m_radius;
		float xDist = yIntercept.x - m_revCenterPt.x;

		float yInterceptSquared = centerDistance * centerDistance - xDist * xDist;

		if (yInterceptSquared < 0) {
			validYIntercept = GetCornerIntersection(targetObject,
													*pDistance,
													&yIntercept);
		} else {
			if (revDirection == kClockwise) {
				yIntercept.y = sqrt(yInterceptSquared) + m_revCenterPt.y;
			} else {
				yIntercept.y = -sqrt(yInterceptSquared) + m_revCenterPt.y;
			}
		}
	} else {
		validYIntercept = false;
	}

	// If we found a valid place to back out to, try going there
	if (validYIntercept) {
		m_center.x = yIntercept.x;
		m_center.y = yIntercept.y;

		ForceOutOfObject(targetObject, pDistance);
		TestObjectIntersection(targetObject, pDistance);
		return true;
	}

	// If we get here, we weren't able to circle out.
	m_center = oldCenter;
	TestObjectIntersection(targetObject, pDistance);
	return false;
}

bool CCollisionCylinder::CircleOutOfObject(const CCollisionCylinder &targetObject,
										   U32Distance3D *pDistance,
										   float *pTimeUsed) {
	// Get the distance between the revolution center and the target.
	U32Distance3D targetRevDist = (m_revCenterPt - targetObject.m_center);

	// Get the distance from the target cylinder that we want to be.
	float targetDistance = m_radius + targetObject.m_radius;

	// Get the distance from the revolution center that we want to be.
	U32FltVector2D sourceVector = m_center - m_revCenterPt;
	float revDistance = sourceVector.Magnitude();

	// Find out in which direction we are going to revolve around the target point.
	ERevDirection revDirection = (GetRevDirection() == kClockwise) ? kCounterClockwise : kClockwise;

	U32FltPoint2D point1;
	U32FltPoint2D point2;

	int points = GetEquidistantPoint(targetObject.m_center, targetDistance,
									 m_revCenterPt, revDistance,
									 &point1,
									 &point2);

	switch (points) {
	case 0:
		debug("Could not find point of intersection.");
		return false;
		break;

	case 1:
		m_center.x = point1.x;
		m_center.y = point1.y;
		break;

	case 2:

		U32FltVector2D vector1 = point1 - m_revCenterPt;
		U32FltVector2D vector2 = point2 - m_revCenterPt;

		ERevDirection direction1 = (sourceVector.GetRevDirection(vector1));
		ERevDirection direction2 = (sourceVector.GetRevDirection(vector2));

		if (direction1 == direction2) {
			debug("Both directions are the same.  That's weird.");

		} else if (direction1 == revDirection) {
			m_center.x = point1.x;
			m_center.y = point1.y;
		} else {
			m_center.x = point2.x;
			m_center.y = point2.y;
		}
		break;
	}

	TestObjectIntersection(targetObject, pDistance);
	ForceOutOfObject(targetObject, pDistance);

	return true;
}

bool CCollisionCylinder::NudgeObject(const CCollisionBox &targetObject,
									 U32Distance3D *pDistance,
									 float *pTimeUsed) {
	float tFinal = 0;

	// To nudge the cylinder precisely against the box, we need to calculate when the
	// square root of the sum of the squared distances between the sphere and the three
	// planes equals the radius of the sphere.  Here we will construct a quadratic
	// equation to solve for time.

	double a = 0;
	double b = 0;
	double c = -(m_radius * m_radius);

	// See if we are standing on the object
	if ((pDistance->z == (m_height / 2)) &&
		(pDistance->XYMagnitude() == 0)) {
		return true;
	}

	// Only do caluclations for the x and y dimensions.  We will handle z seperately.
	for (int i = X_INDEX; i <= Y_INDEX; ++i) {
		EDimension dim = (EDimension)i;

		// If the ball is already within the boundaries of the box in a certain dimension,
		// we don't want to include that dimension in the equation.
		if ((*pDistance)[dim] != 0) {
			a += (m_velocity[dim] * m_velocity[dim]);
			b += (2 * m_velocity[dim] * (*pDistance)[dim]);
			c += ((*pDistance)[dim] * (*pDistance)[dim]);
		}
	}

	if (((b * b) < (4 * a * c)) || (a == 0)) {
		tFinal = -GetPenetrationTime(targetObject, *pDistance, Z_INDEX);

		assert(tFinal >= 0);
		// assert(!"Tried to use sqrt on a negative number");
		// return false;
	} else {

		// Now we have two answer candidates.  We want the smallest of the two that is
		// greater than 0.
		double t1 = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
		double t2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

		double tXY = 0;
		if ((0 <= t1) && (t1 <= t2)) {
			tXY = t1;
		} else if ((0 <= t2) && (t2 <= t1)) {
			tXY = t2;
		}

		float tZ = -GetPenetrationTime(targetObject, *pDistance, Z_INDEX);
		tFinal = ((0 < tZ) && (tZ < tXY)) ? tZ : tXY;
	}

	// Update the position of the ball
	m_center.x += m_velocity.x * tFinal;
	m_center.y += m_velocity.y * tFinal;
	m_center.z += m_velocity.z * tFinal;
	*pTimeUsed -= tFinal;

	TestObjectIntersection(targetObject, pDistance);
	return true;
}

bool CCollisionCylinder::
	NudgeObject(const CCollisionCylinder &targetObject,
				U32Distance3D *pDistance,
				float *pTimeUsed) {
	float collisionTimeFinal;

	U32FltVector3D xyVelocity = m_velocity;
	xyVelocity.z = 0;

	// Find the distance between the two centers that would indicate an exact collision
	float intersectionDist = m_radius + targetObject.m_radius;

	// Set up a vector pointing from the center of this sphere to the center of
	// the target cylinder.
	U32FltVector3D centerVector;
	centerVector.x = targetObject.m_center.x - m_center.x;
	centerVector.y = targetObject.m_center.y - m_center.y;

	float centerDistance = centerVector.XYMagnitude();

	if (centerDistance > intersectionDist) {
		// Project the center vector onto the velocity vector.
		// This is the distance along the velocity vector from the
		// center of the sphere to the point that is parallel with the center
		// of the cylinder.
		float parallelDistance = centerVector.ProjectScalar(xyVelocity);

		// Find the distance between the center of the target object to the point that
		// that is distance2 units along the the velocity vector.
		// assert( (centerDistance * centerDistance) >= ((parallelDistance * parallelDistance) - EPSILON));
		parallelDistance = ((centerDistance * centerDistance) >=
							(parallelDistance * parallelDistance))
							   ? parallelDistance
							   : U32copysign(centerDistance, parallelDistance);

		// Make sure we don't try to sqrt by negative number.
		if (parallelDistance > centerDistance) {
			debug("Tried to sqrt by negative number.");
			centerDistance = parallelDistance;
		}

		float perpDistance = sqrt(centerDistance * centerDistance - parallelDistance * parallelDistance);

		// Now we need to find the point along the velocity vector
		// where the distance between that point and the center of the target cylinder
		// equals intersectionDist.  We calculate using distance 2, distance3 and
		// intersectionDist.
		// assert( (intersectionDist * intersectionDist) >= ((perpDistance * perpDistance) - EPSILON));
		perpDistance = ((intersectionDist * intersectionDist) >=
						(perpDistance * perpDistance))
						   ? perpDistance
						   : U32copysign(intersectionDist, perpDistance);

		// Make sure we don't try to sqrt by negative number.
		if (perpDistance > intersectionDist) {
			debug("Tried to sqrt by negative number.");
			intersectionDist = perpDistance;
		}

		float xyCollisionDist = parallelDistance - sqrt(intersectionDist * intersectionDist - perpDistance * perpDistance);
		collisionTimeFinal = (m_velocity.XYMagnitude() == 0) ? 0 : (xyCollisionDist / m_velocity.XYMagnitude());
	} else {
		collisionTimeFinal = -GetPenetrationTime(targetObject, *pDistance, Z_INDEX);
	}

	m_center.x += collisionTimeFinal * m_velocity.x;
	m_center.y += collisionTimeFinal * m_velocity.y;
	m_center.z += collisionTimeFinal * m_velocity.z;
	*pTimeUsed -= collisionTimeFinal;

	TestObjectIntersection(targetObject, pDistance);
	return true;
}

float CCollisionCylinder::GetPointOfCollision(const CCollisionBox &targetBox,
											  U32Distance3D distance,
											  EDimension dimension) const {
	if (dimension == Z_INDEX) {
		if ((distance[dimension] + (m_height / 2)) < 0) {
			return targetBox.m_minPoint[dimension];
		} else if ((distance[dimension] - (m_height / 2)) > 0) {
			return targetBox.m_maxPoint[dimension];
		} else {
			return m_center[dimension];
		}
	} else {
		if (distance[dimension] < 0) {
			return targetBox.m_minPoint[dimension];
		} else if (distance[dimension] > 0) {
			return targetBox.m_maxPoint[dimension];
		} else {
			return m_center[dimension];
		}
	}
}

ERevDirection CCollisionCylinder::GetRevDirection(void) const {
	if (m_movementType == kCircular)
		error("We can't get a revolution direction if we aren't moving in a circular fashion.");

	U32FltVector3D targetVector = m_revCenterPt - m_center;

	if ((m_velocity.XYMagnitude() != 0) &&
		(targetVector.XYMagnitude() != 0)) {
		U32FltVector3D vector1 = targetVector;
		U32FltVector3D vector2 = m_velocity;

		vector1.z = 0;
		vector2.z = 0;

		U32FltVector3D vector3 = vector1.Cross(vector2);

		// float theta = asin(sinTheta);
		return (vector3.z > 0) ? kClockwise : kCounterClockwise;
	} else {
		error("You tried to divide by zero.  Naughty naughty.");
		//return kNone;
	}
}

void CCollisionCylinder::ForceOutOfObject(const ICollisionObject &targetObject,
										  U32Distance3D *pDistance) {
	U32FltVector3D targetVector;
	U32FltPoint3D intersectionPoint;

	const CCollisionBox *pTargetBox = nullptr;
	const CCollisionCylinder *pTargetCylinder = nullptr;

	float targetHeight;
	float targetRadius;
	U32FltPoint3D targetCenter;

	if (m_ignore)
		return;

	switch (targetObject.m_objectShape) {
	case kBox:

		pTargetBox = static_cast<const CCollisionBox *>(&targetObject);

		for (int i = X_INDEX; i <= Y_INDEX; ++i) {
			EDimension dim = (EDimension)i;

			if (m_center[dim] < pTargetBox->m_minPoint[dim]) {
				intersectionPoint[dim] = pTargetBox->m_minPoint[dim];
			} else if (m_center[dim] > pTargetBox->m_maxPoint[dim]) {
				intersectionPoint[dim] = pTargetBox->m_maxPoint[dim];
			} else {
				intersectionPoint[dim] = m_center[dim];
			}
		}

		targetVector = (m_center - intersectionPoint);
		targetVector.z = 0;
		targetVector = targetVector.Normalize() * m_radius;

		m_center.x = intersectionPoint.x + targetVector.x;
		m_center.y = intersectionPoint.y + targetVector.y;
		ICollisionObject::TestObjectIntersection(targetObject, pDistance);

		break;

	case kCylinder:

		pTargetCylinder = static_cast<const CCollisionCylinder *>(&targetObject);
		targetHeight = pTargetCylinder->m_height;
		targetRadius = pTargetCylinder->m_radius;
		targetCenter = pTargetCylinder->m_center;

		// Handle the case where two players are at the exact same spot.
		if (!((pDistance->x != 0) || (pDistance->y != 0)))
			error("these two cylinders have the same center, so we don't know which direction to push this one out.");

		if (pDistance->XYMagnitude() == 0) {
			pDistance->x = 1;
		}

		if (pDistance->z < (targetHeight / 2)) {
			targetVector.x = pDistance->x;
			targetVector.y = pDistance->y;
			targetVector.z = 0;

			targetVector = targetVector.Normalize() * (m_radius + targetRadius);

			m_center.x = targetCenter.x + targetVector.x;
			m_center.y = targetCenter.y + targetVector.y;
			ICollisionObject::TestObjectIntersection(targetObject, pDistance);
		}
		break;

	default:

		return;
	}
}

U32FltPoint3D CCollisionCylinder::
	FindNearestPoint(const U32FltPoint3D &testPoint) const {
	U32FltPoint3D cylinderPoint;

	if (testPoint.z <= m_center.z - (m_height / 2)) {
		cylinderPoint.x = testPoint.x;
		cylinderPoint.y = testPoint.y;
		cylinderPoint.z = m_center.z - (m_height / 2);
	} else if (testPoint.z >= m_center.z + (m_height / 2)) {
		cylinderPoint.x = testPoint.x;
		cylinderPoint.y = testPoint.y;
		cylinderPoint.z = m_center.z + (m_height / 2);
	} else {
		U32FltVector3D centerVector = testPoint - m_center;
		centerVector = centerVector.Normalize() * m_radius;

		cylinderPoint.x = m_center.x + centerVector.x;
		cylinderPoint.y = m_center.y + centerVector.y;
		cylinderPoint.z = testPoint.z;
	}

	return cylinderPoint;
}

void CCollisionCylinder::HandleCollision(const CCollisionBox &targetBox,
										 float *pTimeUsed,
										 U32Distance3D *pDistance,
										 bool advanceObject) {
	// Handle collisions in the z direction.
	// If we're on something, make sure we aren't going up or down.
	if (((pDistance->z >= (m_height / 2)) && (m_velocity.z < 0)) ||
		((pDistance->z <= -(m_height / 2)) && (m_velocity.z > 0))) {
		m_velocity.z = 0;
	}

	// Handle collisions in the xy direction.
	if (m_movementType == kCircular) {
		m_velocity.x = 0;
		m_velocity.y = 0;
		// m_velocity.z = zVelocity;
	} else {
		U32FltPoint2D collisionPoint;
		collisionPoint.x = GetPointOfCollision(targetBox, *pDistance, X_INDEX);
		collisionPoint.y = GetPointOfCollision(targetBox, *pDistance, Y_INDEX);

		U32FltVector2D targetVector;
		targetVector.x = collisionPoint.x - m_center.x;
		targetVector.y = collisionPoint.y - m_center.y;

		// Project the velocity vector onto that vector.
		// This will give us the component of the velocity vector that collides directly
		// with the target object.  This part of the velocity vector will be lost in the
		// collision.
		m_velocity -= m_velocity.ProjectVector(targetVector);
		// m_velocity.z = zVelocity;
	}

	// Handle collisions in the z direction.
	// if (pDistance->z >= (m_height / 2))
	//{
	//	m_velocity.z = 0;
	//}

	if (advanceObject) {
		m_center.x += (m_velocity.x * *pTimeUsed);
		m_center.y += (m_velocity.y * *pTimeUsed);
		m_center.z += (m_velocity.z * *pTimeUsed);
		*pTimeUsed = 0;
	}

	return;
}

void CCollisionCylinder::HandleCollision(const CCollisionCylinder &targetCylinder,
										 float *pTimeUsed,
										 U32Distance3D *pDistance,
										 bool advanceObject) {
	// Handle collisions in the z direction.
	// If we're on something, make sure we aren't going up or down.
	if (((pDistance->z >= (m_height / 2)) && (m_velocity.z < 0)) ||
		((pDistance->z <= -(m_height / 2)) && (m_velocity.z > 0))) {
		m_velocity.z = 0;
	}

	// Handle collisions in the xy direction.
	if (m_movementType == kCircular) {
		// If we are moving in a circular motion, then we must have hit at least 2 objects
		// this frame, so we stop our movement altogether.
		m_velocity.x = 0;
		m_velocity.y = 0;

		*pTimeUsed = 1;
	} else {
		// Find the vector from the center of this object to the center of
		// the collision object.
		U32FltVector2D targetVector = targetCylinder.m_center - m_center;

		// Project the velocity vector onto that vector.
		// This will give us the component of the velocity vector that collides directly
		// with the target object.  This part of the velocity vector will be lost in the
		// collision.
		m_velocity -= m_velocity.ProjectVector(targetVector);

		// If we are moving forward, do so, then update the vector
		if (advanceObject) {
			float finalMagnitude = m_velocity.XYMagnitude();

			if (finalMagnitude != 0) {
				// Revolve this cylinder around the targer cylinder.
				m_movementType = kCircular;
				m_pRevCenter = dynamic_cast<const ICollisionObject *>(&targetCylinder);
				m_revCenterPt = targetCylinder.m_center;

				// Find out in which direction we are going to revolve around the target cylinder.
				ERevDirection revDirection = GetRevDirection();

				// Find the diameter of the circle we are moving around.
				double movementDiameter = 2 * M_PI * (targetCylinder.m_radius + m_radius);

				// Find how many radians around the circle we will go.
				double movementRadians = 2 * M_PI * ((finalMagnitude * BB_MAX(*pTimeUsed, (float)1)) / movementDiameter) * revDirection;

				m_center.x = (pDistance->x * cos(movementRadians) - pDistance->y * sin(movementRadians)) + targetCylinder.m_center.x;
				m_center.y = (pDistance->x * sin(movementRadians) + pDistance->y * cos(movementRadians)) + targetCylinder.m_center.y;
				TestObjectIntersection(targetCylinder, pDistance);

				// Adjust for rounding errors in pi by pushing this cylinder straight out
				// or in so that it is in contact in just one location with the target
				// cylinder.
				ForceOutOfObject(targetCylinder, pDistance);

				// Calculate the final vector.  The final vector should be tangent to the
				// target cylinder at the point of intersection.
				m_velocity.x = targetVector.y;
				m_velocity.y = targetVector.x;

				m_velocity.x = m_velocity.x / targetVector.Magnitude();
				m_velocity.y = m_velocity.y / targetVector.Magnitude();

				m_velocity.x *= finalMagnitude;
				m_velocity.y *= finalMagnitude;

				if (revDirection == kClockwise) {
					m_velocity.x *= -1;
				} else {
					m_velocity.y *= -1;
				}
			}

			// Update the z position.
			m_center.z += m_velocity.z * *pTimeUsed;

			*pTimeUsed = 0;
		} else {
			// Just update the vector
			float finalMagnitude = m_velocity.XYMagnitude();
			ERevDirection revDirection = GetRevDirection();

			m_velocity.x = targetVector.y;
			m_velocity.y = targetVector.x;

			m_velocity.x = m_velocity.x / targetVector.Magnitude();
			m_velocity.y = m_velocity.y / targetVector.Magnitude();

			m_velocity.x *= finalMagnitude;
			m_velocity.y *= finalMagnitude;

			if (revDirection == kClockwise) {
				m_velocity.x *= -1;
			} else {
				m_velocity.y *= -1;
			}
		}
	}
}

void CCollisionCylinder::HandleCollisions(CCollisionObjectVector *pCollisionVector,
										  float *pTimeUsed,
										  bool advanceObject) {
	if (pCollisionVector->size() > 1) {
		m_velocity.x = 0;
		m_velocity.y = 0;
		return;
	}

	ICollisionObject::HandleCollisions(pCollisionVector, pTimeUsed, advanceObject);
}

bool CCollisionCylinder::IsOnObject(const CCollisionBox &targetObject,
									const U32Distance3D &distance) const {
	return ((distance.z == (m_height / 2)) &&
			(distance.XYMagnitude() < m_radius));
}


bool CCollisionCylinder::IsOnObject(const CCollisionSphere &targetObject,
									const U32Distance3D &distance) const {
	return ((distance.z == (m_height / 2)) &&
			(distance.XYMagnitude() < (m_radius + targetObject.m_radius)));
}

bool CCollisionCylinder::IsOnObject(const CCollisionCylinder &targetObject,
									const U32Distance3D &distance) const {
	return ((distance.z == (m_height / 2)) &&
			(distance.XYMagnitude() < (m_radius + targetObject.m_radius)));
}

U32BoundingBox CCollisionCylinder::GetBoundingBox(void) const {
	U32BoundingBox outBox;

	outBox.m_minPoint.x = m_center.x - m_radius;
	outBox.m_minPoint.y = m_center.y - m_radius;
	outBox.m_minPoint.z = m_center.z - m_height;

	outBox.m_maxPoint.x = m_center.x + m_radius;
	outBox.m_maxPoint.y = m_center.y + m_radius;
	outBox.m_maxPoint.z = m_center.z + m_height;

	return outBox;
}

U32BoundingBox CCollisionCylinder::GetBigBoundingBox(void) const {
	U32BoundingBox outBox;

	float xyVelocity = m_velocity.XYMagnitude();

	outBox.m_minPoint.x = m_center.x - (m_radius + xyVelocity);
	outBox.m_minPoint.y = m_center.y - (m_radius + xyVelocity);
	outBox.m_minPoint.z = m_center.z - (m_height + m_velocity.z);

	outBox.m_maxPoint.x = m_center.x + (m_radius + xyVelocity);
	outBox.m_maxPoint.y = m_center.y + (m_radius + xyVelocity);
	outBox.m_maxPoint.z = m_center.z + (m_height + m_velocity.z);

	return outBox;
}

void CCollisionCylinder::Save(void) {
	m_bPositionSaved = true;
	m_safetyPoint = m_center;
	m_safetyVelocity = m_velocity;
}

void CCollisionCylinder::Restore(void) {
	if (m_bPositionSaved) {
		if (m_safetyVelocity.Magnitude() != 0) {
			debug("Restoring");
			m_center = m_safetyPoint;
			// m_velocity = m_safetyVelocity;
			m_velocity.x = 0;
			m_velocity.y = 0;
			m_velocity.z = 0;
		}
	} else {
		debug("No save point.");
	}
}

int CCollisionCylinder::GetEquidistantPoint(U32FltPoint2D inPoint1, float distance1, U32FltPoint2D inPoint2, float distance2, U32FltPoint2D *pOutPoint1, U32FltPoint2D *pOutPoint2) {
	double distDiff = (distance2 * distance2) - (distance1 * distance1);
	double xDiff = (inPoint1.x * inPoint1.x) - (inPoint2.x * inPoint2.x);
	double yDiff = (inPoint1.y * inPoint1.y) - (inPoint2.y * inPoint2.y);

	double k1 = 0;
	double k2 = 0;

	if (xDiff != 0) {
		k1 = (distDiff + xDiff + yDiff) / (2 * (inPoint1.x - inPoint2.x));
		k2 = (inPoint2.y - inPoint1.y) / (inPoint1.x - inPoint2.x);
	}

	double a = (k2 * k2) + 1;

	double b = (2 * k1 * k2) -
			   (2 * k2 * inPoint1.x) -
			   (2 * inPoint1.y);

	double c = (inPoint1.x * inPoint1.x) +
			   (inPoint1.y * inPoint1.y) +
			   (k1 * k1) -
			   (2 * k1 * inPoint1.x) -
			   (distance1 * distance1);

	if (((b * b) < (4 * a * c)) || (a == 0)) {
		return 0;
	} else {
		pOutPoint1->y = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
		pOutPoint2->y = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

		pOutPoint1->x = k1 + (k2 * pOutPoint1->y);
		pOutPoint2->x = k1 + (k2 * pOutPoint2->y);

		if (*pOutPoint1 == *pOutPoint2) {
			return 1;
		} else {
			return 2;
		}
	}
}

CCollisionNode::CCollisionNode() {
	for (int i = 0; i < NUM_CHILDREN_NODES; i++) {
		m_pChild[i] = nullptr;
	}

	m_quadrant.SetMinPoint(0, 0);
	m_quadrant.SetMaxPoint(0, 0);
}

CCollisionNode::CCollisionNode(const CCollisionObjectVector &initObjects) : m_objectList(initObjects) {
	for (int i = 0; i < NUM_CHILDREN_NODES; i++) {
		m_pChild[i] = nullptr;
	}

	m_quadrant.SetMinPoint(0, 0);
	m_quadrant.SetMaxPoint(0, 0);
}


CCollisionNode::~CCollisionNode() {
	if (!m_bIsExternal) {
		for (int i = 0; i < NUM_CHILDREN_NODES; i++) {
			assert(m_pChild[i]);
			delete m_pChild[i];
			m_pChild[i] = nullptr;
		}
	}
}

U32BoundingBox CCollisionNode::GetChildQuadrant(const U32BoundingBox &parentQuadrant,
												EChildID childID) {
	int minX, minY;
	int maxX, maxY;

	int centerX = ((parentQuadrant.GetMinPoint().x + parentQuadrant.GetMaxPoint().x) / 2);
	int centerY = ((parentQuadrant.GetMinPoint().y + parentQuadrant.GetMaxPoint().y) / 2);

	switch (childID) {

	case kChild1:
		minX = centerX;
		minY = parentQuadrant.GetMinPoint().y;
		maxX = parentQuadrant.GetMaxPoint().x;
		maxY = centerY;
		break;

	case kChild2:
		minX = centerX;
		minY = centerY;
		maxX = parentQuadrant.GetMaxPoint().x;
		maxY = parentQuadrant.GetMaxPoint().y;
		break;

	case kChild3:
		minX = parentQuadrant.GetMinPoint().x;
		minY = centerY;
		maxX = centerX;
		maxY = parentQuadrant.GetMaxPoint().y;
		break;

	case kChild4:
		minX = parentQuadrant.GetMinPoint().x;
		minY = parentQuadrant.GetMinPoint().y;
		maxX = centerX;
		maxY = centerY;
		break;

	default:
		assert(!"Invalid childID passed to GetChildQuadrant");
		minX = 0;
		minY = 0;
		maxX = 0;
		maxY = 0;
		break;
	}

	U32BoundingBox childQuadrant;
	childQuadrant.SetMinPoint(minX, minY);
	childQuadrant.SetMaxPoint(maxX, maxY);
	return childQuadrant;
}

void CCollisionNode::SearchTree(const U32BoundingBox &searchRange,
								CCollisionObjectVector *pTargetList) const {
	if (searchRange.Intersect(m_quadrant)) {
		if (m_bIsExternal) {

			// Search through and add points that are within bounds
			for (CCollisionObjectVector::size_type objectIndex = 0; objectIndex < m_objectList.size(); ++objectIndex) {
				if (!m_objectList[objectIndex]->m_ignore) {
					const ICollisionObject *currentObject = m_objectList[objectIndex];
					pTargetList->push_back(currentObject);
				}
			}
		} else {
			// search all of the children
			for (int childID = 0; childID < NUM_CHILDREN_NODES; childID++) {
				m_pChild[childID]->SearchTree(searchRange, pTargetList);
			}
		}
	}
}


ICollisionObject::ICollisionObject(EObjectShape shape) : m_objectShape(shape),
														 m_objectID(0),
														 m_objectType(kNoObjectType),
														 m_description(""),
														 m_collisionEfficiency(0),
														 m_friction(0),
														 m_soundNumber(0),
														 m_ignore(false) {
	m_velocity.x = 0;
	m_velocity.y = 0;
	m_velocity.z = 0;
}

ICollisionObject::~ICollisionObject() {
}

bool ICollisionObject::operator==(const ICollisionObject &otherObject) const {
	return ((otherObject.m_objectShape == m_objectShape) &&
			(otherObject.m_objectType == m_objectType) &&
			(otherObject.m_objectID == m_objectID));
}

float ICollisionObject::GetObjectDistance(const ICollisionObject &targetObject) const {
	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return 0;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return 0;
		break;
	case kSphere:
		return GetObjectDistance(dynamic_cast<const CCollisionSphere &>(targetObject));
		break;
	case kBox:
		return GetObjectDistance(dynamic_cast<const CCollisionBox &>(targetObject));
		break;
	case kCylinder:
		return GetObjectDistance(dynamic_cast<const CCollisionCylinder &>(targetObject));
		break;
	}
}

float ICollisionObject::GetObjectDistance(const CCollisionSphere &targetObject) const {
	return 0;
}

float ICollisionObject::GetObjectDistance(const CCollisionBox &targetObject) const {
	return 0;
}

float ICollisionObject::GetObjectDistance(const CCollisionCylinder &targetObject) const {
	return 0;
}

bool ICollisionObject::TestObjectIntersection(const ICollisionObject &targetObject, U32Distance3D *pDistance) const {
	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kSphere:
		return TestObjectIntersection(dynamic_cast<const CCollisionSphere &>(targetObject), pDistance);
		break;
	case kBox:
		return TestObjectIntersection(dynamic_cast<const CCollisionBox &>(targetObject), pDistance);
		break;
	case kCylinder:
		return TestObjectIntersection(dynamic_cast<const CCollisionCylinder &>(targetObject), pDistance);
		break;
	}
}

bool ICollisionObject::TestObjectIntersection(const CCollisionSphere &targetObject, U32Distance3D *pDistance) const {
	return false;
}

bool ICollisionObject::TestObjectIntersection(const CCollisionBox &targetObject, U32Distance3D *pDistance) const {
	return false;
}

bool ICollisionObject::TestObjectIntersection(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) const {
	return false;
}

bool ICollisionObject::ValidateCollision(const ICollisionObject &targetObject, U32Distance3D *pDistance) {
	if (m_ignore == true) {
		return true;
	}

	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kSphere:
		return ValidateCollision(dynamic_cast<const CCollisionSphere &>(targetObject), pDistance);
		break;
	case kBox:
		return ValidateCollision(dynamic_cast<const CCollisionBox &>(targetObject), pDistance);
		break;
	case kCylinder:
		return ValidateCollision(dynamic_cast<const CCollisionCylinder &>(targetObject), pDistance);
		break;
	}
}

bool ICollisionObject::ValidateCollision(const CCollisionSphere &targetObject, U32Distance3D *pDistance) {
	return true;
}

bool ICollisionObject::ValidateCollision(const CCollisionBox &targetObject, U32Distance3D *pDistance) {
	return true;
}

bool ICollisionObject::ValidateCollision(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) {
	return true;
}

bool ICollisionObject::BackOutOfObject(const ICollisionObject &targetObject,
									   U32Distance3D *pDistance,
									   float *pTimeUsed)

{
	// if (m_velocity.Magnitude() == 0) return true;  // Since sometimes we'll want to
	//  Force a stationary object out of
	//  another, this is being moved to
	//  the child implementations. BH
	if (m_ignore)
		return true;

	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kSphere:
		return BackOutOfObject(dynamic_cast<const CCollisionSphere &>(targetObject), pDistance, pTimeUsed);
		break;
	case kBox:
		return BackOutOfObject(dynamic_cast<const CCollisionBox &>(targetObject), pDistance, pTimeUsed);
		break;
	case kCylinder:
		return BackOutOfObject(dynamic_cast<const CCollisionCylinder &>(targetObject), pDistance, pTimeUsed);
		break;
	}
}

bool ICollisionObject::BackOutOfObject(const CCollisionSphere &targetObject, U32Distance3D *pDistance, float *pTimeUsed) {
	return true;
}

bool ICollisionObject::BackOutOfObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed) {
	return true;
}

bool ICollisionObject::BackOutOfObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed) {
	return true;
}

bool ICollisionObject::NudgeObject(const ICollisionObject &targetObject,
								   U32Distance3D *pDistance,
								   float *pTimeUsed) {
	if (m_velocity.Magnitude() == 0)
		return true;
	if (m_ignore)
		return true;
	if (*pTimeUsed == 0)
		return true;

	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kSphere:
		return NudgeObject(dynamic_cast<const CCollisionSphere &>(targetObject), pDistance, pTimeUsed);
		break;
	case kBox:
		return NudgeObject(dynamic_cast<const CCollisionBox &>(targetObject), pDistance, pTimeUsed);
		break;
	case kCylinder:
		return NudgeObject(dynamic_cast<const CCollisionCylinder &>(targetObject), pDistance, pTimeUsed);
		break;
	}
}

bool ICollisionObject::NudgeObject(const CCollisionSphere &targetObject, U32Distance3D *pDistance, float *pTimeUsed) {
	return true;
}

bool ICollisionObject::NudgeObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed) {
	return true;
}

bool ICollisionObject::NudgeObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed) {
	return true;
}

bool ICollisionObject::IsCollisionHandled(const ICollisionObject &targetObject) const {
	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kSphere:
		return IsCollisionHandled(dynamic_cast<const CCollisionSphere &>(targetObject));
		break;
	case kBox:
		return IsCollisionHandled(dynamic_cast<const CCollisionBox &>(targetObject));
		break;
	case kCylinder:
		return IsCollisionHandled(dynamic_cast<const CCollisionCylinder &>(targetObject));
		break;
	}
}

bool ICollisionObject::IsCollisionHandled(const CCollisionSphere &targetObject) const {
	return true;
}

bool ICollisionObject::IsCollisionHandled(const CCollisionBox &targetObject) const {
	return true;
}

bool ICollisionObject::IsCollisionHandled(const CCollisionCylinder &targetObject) const {
	return true;
}

void ICollisionObject::HandleCollisions(CCollisionObjectVector *pCollisionVector,
										float *pTimeUsed,
										bool advanceObject) {
	if (m_velocity.Magnitude() == 0)
		return;
	if (m_ignore)
		return;

	for (CCollisionObjectVector::const_iterator objectIt = pCollisionVector->begin(); objectIt != pCollisionVector->end(); ++objectIt) {
		const ICollisionObject *pTargetObject = *objectIt;

		U32Distance3D distance;
		TestObjectIntersection(*pTargetObject, &distance);

		switch (pTargetObject->m_objectShape) {
		default:
			assert(!"Tried to interact with an object of undefined type");
			break;
		case kNoObjectShape:
			assert(!"Tried to interact with an object of undefined type");
			break;
		case kSphere:
			HandleCollision(*static_cast<const CCollisionSphere *>(pTargetObject), pTimeUsed, &distance, advanceObject);
			break;
		case kBox:
			HandleCollision(*static_cast<const CCollisionBox *>(pTargetObject), pTimeUsed, &distance, advanceObject);
			break;
		case kCylinder:
			HandleCollision(*static_cast<const CCollisionCylinder *>(pTargetObject), pTimeUsed, &distance, advanceObject);
			break;
		}
	}

	return;
}

void ICollisionObject::HandleCollision(const CCollisionSphere &targetObject,
									   float *pTimeUsed,
									   U32Distance3D *pDistance,
									   bool advanceObject) {
	return;
}

void ICollisionObject::HandleCollision(const CCollisionBox &targetObject,
									   float *pTimeUsed,
									   U32Distance3D *pDistance,
									   bool advanceObject) {
	return;
}

void ICollisionObject::HandleCollision(const CCollisionCylinder &targetObject,
									   float *pTimeUsed,
									   U32Distance3D *pDistance,
									   bool advanceObject) {
	return;
}

bool ICollisionObject::IsOnObject(const ICollisionObject &targetObject,
								  const U32Distance3D &distance) const {
	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return false;
		break;
	case kSphere:
		return IsOnObject(static_cast<const CCollisionSphere &>(targetObject), distance);
		break;
	case kBox:
		return IsOnObject(static_cast<const CCollisionBox &>(targetObject), distance);
		break;
	case kCylinder:
		return IsOnObject(static_cast<const CCollisionCylinder &>(targetObject), distance);
		break;
	}

	assert(!"Control should not reach this point");
	return false;
}

bool ICollisionObject::IsOnObject(const CCollisionSphere &targetObject,
								  const U32Distance3D &distance) const {
	return false;
}

bool ICollisionObject::IsOnObject(const CCollisionBox &targetObject,
								  const U32Distance3D &distance) const {
	return false;
}

bool ICollisionObject::IsOnObject(const CCollisionCylinder &targetObject,
								  const U32Distance3D &distance) const {
	return false;
}

U32FltPoint3D ICollisionObject::FindNearestPoint(const U32FltPoint3D &testPoint) const {
	return testPoint;
}

float ICollisionObject::GetPenetrationTime(const ICollisionObject &targetObject,
										   const U32Distance3D &distance,
										   EDimension dimension) const {
	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		return 0;
		break;

	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		return 0;
		break;

	case kSphere:
		return GetPenetrationTime(*static_cast<const CCollisionSphere *>(&targetObject), distance, dimension);
		break;

	case kBox:
		return GetPenetrationTime(*static_cast<const CCollisionBox *>(&targetObject), distance, dimension);
		break;

	case kCylinder:
		return GetPenetrationTime(*static_cast<const CCollisionCylinder *>(&targetObject), distance, dimension);
	}
}

float ICollisionObject::GetPenetrationTime(const CCollisionSphere &targetObject,
										   const U32Distance3D &distance,
										   EDimension dimension) const {
	return 0;
}

float ICollisionObject::GetPenetrationTime(const CCollisionBox &targetObject,
										   const U32Distance3D &distance,
										   EDimension dimension) const {
	return 0;
}

float ICollisionObject::GetPenetrationTime(const CCollisionCylinder &targetObject,
										   const U32Distance3D &distance,
										   EDimension dimension) const {
	return 0;
}

void ICollisionObject::DefineReflectionPlane(const ICollisionObject &targetObject, const U32Distance3D &distance, U32Plane *pCollisionPlane) const {
	switch (targetObject.m_objectShape) {
	default:
		assert(!"Tried to interact with an object of undefined type");
		break;
	case kNoObjectShape:
		assert(!"Tried to interact with an object of undefined type");
		break;
	case kSphere:
		DefineReflectionPlane(dynamic_cast<const CCollisionSphere &>(targetObject), distance, pCollisionPlane);
		break;
	case kBox:
		DefineReflectionPlane(dynamic_cast<const CCollisionBox &>(targetObject), distance, pCollisionPlane);
		break;
	case kCylinder:
		DefineReflectionPlane(dynamic_cast<const CCollisionCylinder &>(targetObject), distance, pCollisionPlane);
		break;
	}
}

void ICollisionObject::DefineReflectionPlane(const CCollisionSphere &targetObject, const U32Distance3D &distance, U32Plane *collisionPlane) const {
	memset(collisionPlane, 0, sizeof(*collisionPlane) / sizeof(char));
	return;
}

void ICollisionObject::DefineReflectionPlane(const CCollisionBox &targetObject, const U32Distance3D &distance, U32Plane *collisionPlane) const {
	memset(collisionPlane, 0, sizeof(*collisionPlane) / sizeof(char));
	return;
}

void ICollisionObject::DefineReflectionPlane(const CCollisionCylinder &targetObject, const U32Distance3D &distance, U32Plane *collisionPlane) const {
	memset(collisionPlane, 0, sizeof(*collisionPlane) / sizeof(char));
	return;
}

void ICollisionObject::Save(void){

};

void ICollisionObject::Restore(void){

};

extern CBBallCourt g_court;

CCollisionPlayer::CCollisionPlayer() : m_playerHasBall(false),
									   m_playerIsInGame(true),
									   m_blockHeight(0),
									   m_blockTime(0),
									   m_catchHeight(0) {
}

CCollisionPlayer::~CCollisionPlayer() {
}

void CCollisionPlayer::StartBlocking(int blockHeight, int blockTime) {
	assert(blockTime != 0);

	m_maxBlockHeight = blockHeight;
	m_blockTime = blockTime;

	int blockIncrement = m_maxBlockHeight / m_blockTime;

	m_blockHeight += blockIncrement;
	m_height += blockIncrement;
}

void CCollisionPlayer::HoldBlocking() {
	int blockIncrement = m_maxBlockHeight / m_blockTime;

	if (m_velocity.z > 0) {
		if ((m_blockHeight + blockIncrement) <= m_maxBlockHeight) {
			m_blockHeight += blockIncrement;
			m_height += blockIncrement;
		} else {
			blockIncrement = m_maxBlockHeight - m_blockHeight;
			m_blockHeight += blockIncrement;
			m_height += blockIncrement;

			assert(m_blockHeight == m_maxBlockHeight);
		}
	} else if (m_velocity.z < 0) {
		if ((m_blockHeight - blockIncrement) >= 0) {
			m_blockHeight -= blockIncrement;
			m_height -= blockIncrement;
		} else {
			blockIncrement = m_blockHeight;
			m_blockHeight -= blockIncrement;
			m_height -= blockIncrement;

			assert(m_blockHeight == 0);
		}
	}
}

void CCollisionPlayer::EndBlocking() {
	m_height -= m_blockHeight;
	m_blockHeight = 0;
}

bool CCollisionPlayer::TestCatch(const ICollisionObject &targetObject, U32Distance3D *pDistance) {
	if (&targetObject == (ICollisionObject *)&g_court.m_virtualBall) {
		int oldHeight = m_height;
		m_height += m_catchHeight;

		bool retVal = ICollisionObject::TestObjectIntersection(targetObject, pDistance);

		m_height = oldHeight;

		return retVal;
	} else {
		return false;
	}
}

CCollisionShieldVector::CCollisionShieldVector() : m_shieldUpCount(0) {
	CCollisionBox westShield;
	CCollisionBox northShield;
	CCollisionBox eastShield;
	CCollisionBox southShield;
	CCollisionBox topShield;

	CCollisionBox westShield2;
	CCollisionBox northShield2;
	CCollisionBox eastShield2;
	CCollisionBox southShield2;

	westShield.m_minPoint.x = 0 - SHIELD_DEPTH;
	westShield.m_maxPoint.x = 0;
	westShield.m_minPoint.y = 0 - SHIELD_EXTENSION;
	westShield.m_maxPoint.y = MAX_WORLD_Y + SHIELD_EXTENSION;
	westShield.m_minPoint.z = 0;
	westShield.m_maxPoint.z = SHIELD_HEIGHT;
	westShield.m_description = "West Shield";
	westShield.m_collisionEfficiency = .5F;
	westShield.m_friction = .3F;
	westShield.m_objectID = WEST_SHIELD_ID;
	westShield.m_ignore = true;

	westShield2.m_minPoint.x = 0 - SHIELD_DEPTH - BUFFER_WIDTH;
	westShield2.m_maxPoint.x = 0 - BUFFER_WIDTH;
	westShield2.m_minPoint.y = 0 - SHIELD_EXTENSION;
	westShield2.m_maxPoint.y = MAX_WORLD_Y + SHIELD_EXTENSION;
	westShield2.m_minPoint.z = 0;
	westShield2.m_maxPoint.z = SHIELD_HEIGHT;
	westShield2.m_description = "West Shield 2";
	westShield2.m_collisionEfficiency = .5F;
	westShield2.m_friction = .3F;
	westShield2.m_objectID = BACKUP_WEST_SHIELD_ID;
	westShield2.m_ignore = true;

	northShield.m_minPoint.x = 0 - SHIELD_EXTENSION;
	northShield.m_maxPoint.x = MAX_WORLD_X + SHIELD_EXTENSION;
	northShield.m_minPoint.y = MAX_WORLD_Y;
	northShield.m_maxPoint.y = MAX_WORLD_Y + SHIELD_DEPTH;
	northShield.m_minPoint.z = 0;
	northShield.m_maxPoint.z = SHIELD_HEIGHT;
	northShield.m_description = "North Shield";
	northShield.m_collisionEfficiency = .5F;
	northShield.m_friction = .3F;
	northShield.m_objectID = NORTH_SHIELD_ID;
	northShield.m_ignore = true;

	northShield2.m_minPoint.x = 0 - SHIELD_EXTENSION;
	northShield2.m_maxPoint.x = MAX_WORLD_X + SHIELD_EXTENSION;
	northShield2.m_minPoint.y = MAX_WORLD_Y + BUFFER_WIDTH;
	northShield2.m_maxPoint.y = MAX_WORLD_Y + SHIELD_DEPTH + BUFFER_WIDTH;
	northShield2.m_minPoint.z = 0;
	northShield2.m_maxPoint.z = SHIELD_HEIGHT;
	northShield2.m_description = "North Shield 2";
	northShield2.m_collisionEfficiency = .5F;
	northShield2.m_friction = .3F;
	northShield2.m_objectID = BACKUP_NORTH_SHIELD_ID;
	northShield2.m_ignore = true;

	eastShield.m_minPoint.x = MAX_WORLD_X;
	eastShield.m_maxPoint.x = MAX_WORLD_X + SHIELD_DEPTH;
	eastShield.m_minPoint.y = 0 - SHIELD_EXTENSION;
	eastShield.m_maxPoint.y = MAX_WORLD_Y + SHIELD_EXTENSION;
	eastShield.m_minPoint.z = 0;
	eastShield.m_maxPoint.z = SHIELD_HEIGHT;
	eastShield.m_description = "East Shield";
	eastShield.m_collisionEfficiency = .5F;
	eastShield.m_friction = .3F;
	eastShield.m_objectID = EAST_SHIELD_ID;
	eastShield.m_ignore = true;

	eastShield2.m_minPoint.x = MAX_WORLD_X + BUFFER_WIDTH;
	eastShield2.m_maxPoint.x = MAX_WORLD_X + SHIELD_DEPTH + BUFFER_WIDTH;
	eastShield2.m_minPoint.y = 0 - SHIELD_EXTENSION;
	eastShield2.m_maxPoint.y = MAX_WORLD_Y + SHIELD_EXTENSION;
	eastShield2.m_minPoint.z = 0;
	eastShield2.m_maxPoint.z = SHIELD_HEIGHT;
	eastShield2.m_description = "East Shield 2";
	eastShield2.m_collisionEfficiency = .5F;
	eastShield2.m_friction = .3F;
	eastShield2.m_objectID = BACKUP_EAST_SHIELD_ID;
	eastShield2.m_ignore = true;

	southShield.m_minPoint.x = 0 - SHIELD_EXTENSION;
	southShield.m_maxPoint.x = MAX_WORLD_X + SHIELD_EXTENSION;
	southShield.m_minPoint.y = 0 - SHIELD_DEPTH;
	southShield.m_maxPoint.y = 0;
	southShield.m_minPoint.z = 0;
	southShield.m_maxPoint.z = SHIELD_HEIGHT;
	southShield.m_description = "South Shield";
	southShield.m_collisionEfficiency = .5F;
	southShield.m_friction = .3F;
	southShield.m_objectID = SOUTH_SHIELD_ID;
	southShield.m_ignore = true;

	southShield2.m_minPoint.x = 0 - SHIELD_EXTENSION;
	southShield2.m_maxPoint.x = MAX_WORLD_X + SHIELD_EXTENSION;
	southShield2.m_minPoint.y = 0 - SHIELD_DEPTH - BUFFER_WIDTH;
	southShield2.m_maxPoint.y = 0 - BUFFER_WIDTH;
	southShield2.m_minPoint.z = 0;
	southShield2.m_maxPoint.z = SHIELD_HEIGHT;
	southShield2.m_description = "South Shield 2";
	southShield2.m_collisionEfficiency = .5F;
	southShield2.m_friction = .3F;
	southShield2.m_objectID = BACKUP_SOUTH_SHIELD_ID;
	southShield2.m_ignore = true;

	topShield.m_minPoint.x = 0 - SHIELD_EXTENSION;
	topShield.m_maxPoint.x = MAX_WORLD_X + SHIELD_EXTENSION;
	topShield.m_minPoint.y = 0 - SHIELD_EXTENSION;
	topShield.m_maxPoint.y = MAX_WORLD_Y + SHIELD_EXTENSION;
	topShield.m_minPoint.z = SHIELD_HEIGHT;
	topShield.m_maxPoint.z = SHIELD_HEIGHT + SHIELD_DEPTH;
	topShield.m_description = "Top Shield";
	topShield.m_collisionEfficiency = .5F;
	topShield.m_friction = .3F;
	topShield.m_objectID = TOP_SHIELD_ID;
	topShield.m_ignore = true;

	push_back(westShield);
	push_back(northShield);
	push_back(eastShield);
	push_back(southShield);
	push_back(topShield);

	push_back(westShield2);
	push_back(northShield2);
	push_back(eastShield2);
	push_back(southShield2);
}

CCollisionShieldVector::~CCollisionShieldVector() {
	CCollisionShieldVector::iterator shieldIt;

	for (shieldIt = begin();
		 shieldIt != end();
		 ++shieldIt) {
		shieldIt->m_ignore = true;
	}
}

int LogicHEBasketball::U32_User_Raise_Shields(int shieldID) {
	assert(shieldID < MAX_SHIELD_COUNT || shieldID == ALL_SHIELD_ID);

	CCollisionShieldVector::iterator shieldIt;

	for (shieldIt = _vm->_basketball->g_shields.begin(); shieldIt != _vm->_basketball->g_shields.end(); ++shieldIt) {
		// Make sure we don't mess with the backup shields.
		if (shieldIt->m_objectID < MAX_SHIELD_COUNT) {
			if (((shieldIt->m_objectID == shieldID) || (shieldID == ALL_SHIELD_ID)) &&
				(shieldIt->m_ignore == true)) {
				shieldIt->m_ignore = false;
				++_vm->_basketball->g_shields.m_shieldUpCount;
			}
		}
	}

	if (shieldID == ALL_SHIELD_ID) {
		assert(_vm->_basketball->g_shields.m_shieldUpCount == MAX_SHIELD_COUNT);
	}

	return 1;
}

int LogicHEBasketball::U32_User_Lower_Shields(int shieldID) {
	assert(shieldID < MAX_SHIELD_COUNT || shieldID == ALL_SHIELD_ID);

	CCollisionShieldVector::iterator shieldIt;

	for (shieldIt = _vm->_basketball->g_shields.begin(); shieldIt != _vm->_basketball->g_shields.end(); ++shieldIt) {
		// Make sure we don't mess with the backup shields.
		if (shieldIt->m_objectID < MAX_SHIELD_COUNT) {
			if (((shieldIt->m_objectID == shieldID) || (shieldID == ALL_SHIELD_ID)) &&
				(shieldIt->m_ignore == false)) {
				shieldIt->m_ignore = true;
				--_vm->_basketball->g_shields.m_shieldUpCount;
			}
		}
	}

	if (shieldID == ALL_SHIELD_ID) {
		assert(_vm->_basketball->g_shields.m_shieldUpCount == 0);
	}

	return 1;
}

int LogicHEBasketball::U32_User_Are_Shields_Clear() {
	int bShieldsAreClear = (_vm->_basketball->g_shields.m_shieldUpCount != MAX_SHIELD_COUNT);
	writeScummVar(_vm1->VAR_U32_USER_VAR_A, bShieldsAreClear);
	return 1;
}

int LogicHEBasketball::U32_User_Shield_Player(int playerID, int shieldRadius) {
	CCollisionPlayer *pPlayer = _vm->_basketball->g_court.GetPlayerPtr(playerID);

	if (pPlayer->m_shieldRadius < shieldRadius) {
		pPlayer->m_shieldRadius += PLAYER_SHIELD_INCREMENT_VALUE;
		pPlayer->m_radius += PLAYER_SHIELD_INCREMENT_VALUE;
	}

	return 1;
}


int LogicHEBasketball::U32_User_Clear_Player_Shield(int playerID) {
	CCollisionPlayer *pPlayer = _vm->_basketball->g_court.GetPlayerPtr(playerID);

	pPlayer->m_radius -= pPlayer->m_shieldRadius;
	pPlayer->m_shieldRadius = 0;

	return 1;
}

CCollisionSphere::CCollisionSphere() : ICollisionObject(kSphere),
									   m_rollingCount(0),
									   m_bPositionSaved(false) {
}

CCollisionSphere::~CCollisionSphere() {
}

float CCollisionSphere::
	GetDimensionDistance(const CCollisionBox &targetObject,
						 EDimension dimension) const {
	if (m_center[dimension] < targetObject.m_minPoint[dimension]) {
		return (m_center[dimension] - targetObject.m_minPoint[dimension]);
	} else if (m_center[dimension] > targetObject.m_maxPoint[dimension]) {
		return (m_center[dimension] - targetObject.m_maxPoint[dimension]);
	} else {
		return 0;
	}
}

float CCollisionSphere::
	GetDimensionDistance(const CCollisionCylinder &targetObject,
						 EDimension dimension) const {
	float centerDistance = m_center[dimension] - targetObject.m_center[dimension];

	if (dimension == Z_INDEX) {
		if (centerDistance < -(targetObject.m_height / 2)) {
			return (centerDistance + (targetObject.m_height / 2));
		} else if (centerDistance > (targetObject.m_height / 2)) {
			return (centerDistance - (targetObject.m_height / 2));
		} else {
			return 0;
		}
	} else {
		return centerDistance;
	}
}

float CCollisionSphere::
	GetObjectDistance(const CCollisionBox &targetObject) const {
	U32Distance3D distance;

	distance.x = GetDimensionDistance(targetObject, X_INDEX);
	distance.y = GetDimensionDistance(targetObject, Y_INDEX);
	distance.z = GetDimensionDistance(targetObject, Z_INDEX);

	float totalDistance = distance.Magnitude() - m_radius;
	if (totalDistance < 0)
		totalDistance = 0;

	return (totalDistance);
}

float CCollisionSphere::
	GetObjectDistance(const CCollisionCylinder &targetObject) const {
	U32Distance3D distance;

	distance.x = GetDimensionDistance(targetObject, X_INDEX);
	distance.y = GetDimensionDistance(targetObject, Y_INDEX);
	distance.z = GetDimensionDistance(targetObject, Z_INDEX);

	float xyDistance = distance.XYMagnitude() - m_radius - targetObject.m_radius;
	if (xyDistance < 0)
		xyDistance = 0;

	float zDistance = fabs(distance.z) - m_radius - (targetObject.m_height / 2);
	if (zDistance < 0)
		zDistance = 0;

	float totalDistance = sqrt((xyDistance * xyDistance) + (zDistance * zDistance));
	return (totalDistance);
}

bool CCollisionSphere::
	TestObjectIntersection(const CCollisionBox &targetObject,
						   U32Distance3D *pDistance) const {
	// Get the distance between the ball and the bounding box
	pDistance->x = GetDimensionDistance(targetObject, X_INDEX);
	pDistance->y = GetDimensionDistance(targetObject, Y_INDEX);
	pDistance->z = GetDimensionDistance(targetObject, Z_INDEX);

	// Determine if the ball intersects the bounding box.
	return (pDistance->Magnitude() < m_radius);
}

bool CCollisionSphere::
	TestObjectIntersection(const CCollisionCylinder &targetObject,
						   U32Distance3D *pDistance) const {
	// Get the distance between the ball and the cylinder
	pDistance->x = GetDimensionDistance(targetObject, X_INDEX);
	pDistance->y = GetDimensionDistance(targetObject, Y_INDEX);
	pDistance->z = GetDimensionDistance(targetObject, Z_INDEX);
	;

	if (pDistance->XYMagnitude() < (m_radius + targetObject.m_radius)) {
		return (fabs(pDistance->z) < m_radius);
	} else {
		return false;
	}
}

bool CCollisionSphere::
	ValidateCollision(const CCollisionBox &targetObject, U32Distance3D *pDistance) {
	U32FltPoint3D targetPoint = targetObject.FindNearestPoint(m_center);
	U32FltVector3D centerVector = targetPoint - m_center;
	float aMag = m_velocity.Magnitude();
	float bMag = centerVector.Magnitude();
	float aDotb = m_velocity * centerVector;

	if (aMag == 0) {
		// If this object isn't moving, this can't be a valid collision.
		return false;
	}

	if (bMag == 0) {
		// bMag is 0, it is possible that this sphere penetrated too far into the target
		// object.  If this is the case, we'll go ahead and validate the collision.
		return true;
	}

	double angleCosine = aDotb / (aMag * bMag);

	if (angleCosine > 1) {
		angleCosine = 1;
	} else if (angleCosine < -1) {
		angleCosine = -1;
	}

	float sourceMovementAngle = acos(angleCosine);

	if (sourceMovementAngle < (M_PI / 2)) {
		return true;
	} else {
		return false;
	}
}

bool CCollisionSphere::
	ValidateCollision(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) {
	// Create a vector from the center of the target cylinder to the center of
	// this sphere.  If the sphere is not hitting above or below the cylinder,
	// negate any z component of the center vector.
	U32FltVector3D centerVector = targetObject.m_center - m_center;

	if (m_center.z > (targetObject.m_center.z +
					  targetObject.m_height / 2)) {
		centerVector.z = targetObject.m_center.z + (targetObject.m_height / 2) - m_center.z;
	} else if (m_center.z < (targetObject.m_center.z -
							 targetObject.m_height / 2)) {
		centerVector.z = targetObject.m_center.z - (targetObject.m_height / 2) - m_center.z;
	} else if (m_velocity.XYMagnitude() != 0) {
		centerVector.z = 0;
	}

	float aMag = m_velocity.Magnitude();
	float bMag = centerVector.Magnitude();
	float aDotb = m_velocity * centerVector;

	if (aMag == 0) {
		// If this object isn't moving, this can't be a valid collision.
		return false;
	}

	if (bMag == 0) {
		// bMag is 0, it is possible that this sphere penetrated too far into the target
		// object.  If this is the case, we'll go ahead and validate the collision.
		return true;
	}

	// Calculate the angle between this object's velocity and the vector from the target
	// objects center to our center.  If the angle between the two is greater than
	// 90 degrees, then the target object is not colliding with us.
	double angleCosine = aDotb / (aMag * bMag);

	if (angleCosine > 0)
	// if (sourceMovementAngle < (M_PI / 2))
	{
		return true;
	} else {
		return false;
	}
}

float CCollisionSphere::
	GetPenetrationTime(const CCollisionBox &targetObject,
					   const U32Distance3D &distance,
					   EDimension dimension) const {
	float collisionDepth1;
	float collisionDepth2;

	float boxWidth = targetObject.m_maxPoint[dimension] - targetObject.m_minPoint[dimension];

	if (distance[dimension] > 0) {
		collisionDepth1 = (m_radius - distance[dimension]);
		collisionDepth2 = (-m_radius + distance[dimension] - boxWidth);
	} else if (distance[dimension] < 0) {
		collisionDepth1 = (-m_radius - distance[dimension]);
		collisionDepth2 = (m_radius - distance[dimension] + boxWidth);
	} else {
		return 0;
	}

	float time1 = (m_velocity[dimension] == 0) ? 0 : collisionDepth1 / -m_velocity[dimension];
	float time2 = (m_velocity[dimension] == 0) ? 0 : collisionDepth2 / -m_velocity[dimension];
	float tFinal = BB_MIN_GREATER_THAN_ZERO(time1, time2);

	return (tFinal);
}

float CCollisionSphere::
	GetPenetrationTime(const CCollisionCylinder &targetObject,
					   const U32Distance3D &distance,
					   EDimension dimension) const {
	float collisionDepth;

	if (dimension == Z_INDEX) {
		if (distance[dimension] > 0) {
			collisionDepth = (m_radius - distance[dimension]);
		} else if (distance[dimension] < 0) {
			collisionDepth = (-m_radius - distance[dimension]);
		} else {
			collisionDepth = 0;
		}
	} else {
		if (distance[dimension] > 0) {
			collisionDepth = (m_radius + targetObject.m_radius - distance[dimension]);
		} else if (distance[dimension] < 0) {
			collisionDepth = (-m_radius - targetObject.m_radius - distance[dimension]);
		} else {
			collisionDepth = 0;
		}
	}

	float tFinal = (m_velocity[dimension] == 0) ? 0 : (collisionDepth / -m_velocity[dimension]);
	return (tFinal);
}

bool CCollisionSphere::
	BackStraightOutOfObject(const ICollisionObject &targetObject,
							U32Distance3D *pDistance,
							float *pTimeUsed) {
	if (m_velocity.Magnitude() == 0)
		return true;

	U32FltPoint3D startPosition = m_center;

	int loopCounter = 0;

	while (ICollisionObject::TestObjectIntersection(targetObject, pDistance)) {
		float collisionTimes[3];

		collisionTimes[X_INDEX] = ICollisionObject::GetPenetrationTime(targetObject, *pDistance, X_INDEX);
		collisionTimes[Y_INDEX] = ICollisionObject::GetPenetrationTime(targetObject, *pDistance, Y_INDEX);
		collisionTimes[Z_INDEX] = ICollisionObject::GetPenetrationTime(targetObject, *pDistance, Z_INDEX);

		Common::sort(collisionTimes, collisionTimes + Z_INDEX + 1);

		float collisionTime = SMALL_TIME_INCREMENT;
		if (collisionTimes[2] > 0)
			collisionTime = collisionTimes[2];
		if (collisionTimes[1] > 0)
			collisionTime = collisionTimes[1];
		if (collisionTimes[0] > 0)
			collisionTime = collisionTimes[0];

		*pTimeUsed += collisionTime;

		// If we take too long to back out, something is wrong.
		// Restore the object to an ok state.
		if ((*pTimeUsed > BACK_OUT_TIME_LIMIT) && (*pTimeUsed != collisionTime)) {
			assert(!"It took too long for one object to back out of another.  Ignore and U32 will attempt to correct.");
			m_center = startPosition;
			Restore();
			return false;
		}

		m_center.x -= (collisionTime * m_velocity.x);
		m_center.y -= (collisionTime * m_velocity.y);
		m_center.z -= (collisionTime * m_velocity.z);

		// Make doubly sure we don't loop forever.
		if (++loopCounter > 500)
			return false;
	}

	return true;
}

bool CCollisionSphere::
	BackOutOfObject(const CCollisionBox &targetObject,
					U32Distance3D *pDistance,
					float *pTimeUsed) {
	return BackStraightOutOfObject(targetObject, pDistance, pTimeUsed);
}

bool CCollisionSphere::
	BackOutOfObject(const CCollisionCylinder &targetObject,
					U32Distance3D *pDistance,
					float *pTimeUsed) {
	return BackStraightOutOfObject(targetObject, pDistance, pTimeUsed);
}

bool CCollisionSphere::
	NudgeObject(const CCollisionBox &targetObject,
				U32Distance3D *pDistance,
				float *pTimeUsed) {
	// To nudge the sphere precisely against the box, we need to calculate when the
	// square root of the sum of the squared distances between the sphere and the three
	// planes equals the radius of the sphere.  Here we will construct a quadratic
	// equation to solve for time.

	double a = 0;
	double b = 0;
	double c = -(m_radius * m_radius);

	for (int i = X_INDEX; i <= Z_INDEX; ++i) {
		EDimension dim = (EDimension)i;

		// If the ball is already within the boundaries of the box in a certain dimension,
		// we don't want to include that dimension in the equation.
		if ((*pDistance)[dim] != 0) {
			a += (m_velocity[dim] * m_velocity[dim]);
			b += (2 * m_velocity[dim] * (*pDistance)[dim]);
			c += ((*pDistance)[dim] * (*pDistance)[dim]);
		}
	}

	if (((b * b) < (4 * a * c)) || (a == 0)) {
		assert(!"Tried to use sqrt on a negative number");
		return false;
	}

	// Now we have two answer candidates.  We want the smallest of the two that is
	// greater than 0.
	double t1 = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
	double t2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

	double tFinal = 0;
	if ((0 <= t1) && (t1 <= t2)) {
		tFinal = t1;
	} else if ((0 <= t2) && (t2 <= t1)) {
		tFinal = t2;
	}

	// Update the position of the ball
	m_center.x += m_velocity.x * tFinal;
	m_center.y += m_velocity.y * tFinal;
	m_center.z += m_velocity.z * tFinal;
	*pTimeUsed -= tFinal;

	TestObjectIntersection(targetObject, pDistance);
	return true;
}

bool CCollisionSphere::
	NudgeObject(const CCollisionCylinder &targetObject,
				U32Distance3D *pDistance,
				float *pTimeUsed) {
	float collisionTimeFinal;

	U32FltVector3D xyVelocity = m_velocity;
	xyVelocity.z = 0;

	// Find the distance between the two centers that would indicate an exact collision
	float intersectionDist = m_radius + targetObject.m_radius;

	// Set up a vector pointing from the center of this sphere to the center of
	// the target cylinder.
	U32FltVector3D centerVector;
	centerVector.x = targetObject.m_center.x - m_center.x;
	centerVector.y = targetObject.m_center.y - m_center.y;

	float centerDistance = centerVector.XYMagnitude();

	if (centerDistance > intersectionDist) {
		// Project the center vector onto the velocity vector.
		// This is the distance along the velocity vector from the
		// center of the sphere to the point that is parallel with the center
		// of the cylinder.
		float parallelDistance = centerVector.ProjectScalar(xyVelocity);

		// Find the distance between the center of the target object to the point that
		// that is distance2 units along the the velocity vector.
		// assert( (centerDistance * centerDistance) >= ((parallelDistance * parallelDistance) - EPSILON));
		parallelDistance = ((centerDistance * centerDistance) >=
							(parallelDistance * parallelDistance))
							   ? parallelDistance
							   : U32copysign(centerDistance, parallelDistance);

		// Make sure we don't try to sqrt by negative number.
		if (parallelDistance > centerDistance) {
			debug("Tried to sqrt by negative number.");
			centerDistance = parallelDistance;
		}

		float perpDistance = sqrt(centerDistance * centerDistance - parallelDistance * parallelDistance);

		// Now we need to find the point along the velocity vector
		// where the distance between that point and the center of the target cylinder
		// equals intersectionDist.  We calculate using distance 2, distance3 and
		// intersectionDist.
		// assert( (intersectionDist * intersectionDist) >= ((perpDistance * perpDistance) - EPSILON));
		perpDistance = ((intersectionDist * intersectionDist) >=
						(perpDistance * perpDistance))
						   ? perpDistance
						   : U32copysign(intersectionDist, perpDistance);

		// Make sure we don't try to sqrt by negative number.
		if (perpDistance > intersectionDist) {
			debug("Tried to sqrt by negative number.");
			intersectionDist = perpDistance;
		}

		float xyCollisionDist = parallelDistance - sqrt(intersectionDist * intersectionDist - perpDistance * perpDistance);
		collisionTimeFinal = (m_velocity.XYMagnitude() == 0) ? 0 : (xyCollisionDist / m_velocity.XYMagnitude());
	} else {
		collisionTimeFinal = -GetPenetrationTime(targetObject, *pDistance, Z_INDEX);
	}

	m_center.x += collisionTimeFinal * m_velocity.x;
	m_center.y += collisionTimeFinal * m_velocity.y;
	m_center.z += collisionTimeFinal * m_velocity.z;
	*pTimeUsed -= collisionTimeFinal;

	TestObjectIntersection(targetObject, pDistance);

	// We may still have a little ways to go in the z direction
	if (fabs(pDistance->z) >= (m_radius + COLLISION_EPSILON)) {
		collisionTimeFinal = -GetPenetrationTime(targetObject, *pDistance, Z_INDEX);

		m_center.x += collisionTimeFinal * m_velocity.x;
		m_center.y += collisionTimeFinal * m_velocity.y;
		m_center.z += collisionTimeFinal * m_velocity.z;
		*pTimeUsed -= collisionTimeFinal;

		TestObjectIntersection(targetObject, pDistance);
	}

	return true;
}

void CCollisionSphere::
	DefineReflectionPlane(const CCollisionBox &targetObject,
						  const U32Distance3D &distance,
						  U32Plane *pCollisionPlane) const {
	// Find the point of collision
	pCollisionPlane->m_point = targetObject.FindNearestPoint(m_center);

	// Find the normal of the collision plane.
	pCollisionPlane->m_normal.x = (distance.Magnitude() == 0) ? 0 : distance.x / distance.Magnitude();
	pCollisionPlane->m_normal.y = (distance.Magnitude() == 0) ? 0 : distance.y / distance.Magnitude();
	pCollisionPlane->m_normal.z = (distance.Magnitude() == 0) ? 0 : distance.z / distance.Magnitude();

	pCollisionPlane->m_friction = targetObject.m_friction;
	pCollisionPlane->m_collisionEfficiency = targetObject.m_collisionEfficiency;
}

void CCollisionSphere::
	DefineReflectionPlane(const CCollisionCylinder &targetObject,
						  const U32Distance3D &distance,
						  U32Plane *pCollisionPlane) const {
	// Find the point of collision
	pCollisionPlane->m_point = targetObject.FindNearestPoint(m_center);

	// Find the normal of the collision plane.
	pCollisionPlane->m_normal.x = (distance.Magnitude() == 0) ? 0 : distance.x / distance.Magnitude();
	pCollisionPlane->m_normal.y = (distance.Magnitude() == 0) ? 0 : distance.y / distance.Magnitude();
	pCollisionPlane->m_normal.z = (distance.Magnitude() == 0) ? 0 : distance.z / distance.Magnitude();

	pCollisionPlane->m_friction = targetObject.m_friction;
	pCollisionPlane->m_collisionEfficiency = targetObject.m_collisionEfficiency;
}

void CCollisionSphere::
	ReboundOffPlane(const U32Plane &collisionPlane, bool isOnObject) {
	U32FltVector3D normalVector;
	U32FltVector3D reflectionVector;
	U32FltVector3D parallelVector;
	U32FltVector3D perpendicularVector;

	// Calculate the reflection vector
	normalVector = m_velocity.ProjectVector(collisionPlane.m_normal) * -1;
	reflectionVector = (normalVector * 2) + m_velocity;

	// Break down the components of the refelction vector that are perpendicular
	// and parallel to the collision plane.  This way we can account for drag and
	// inelastic collisions.
	perpendicularVector = normalVector;
	perpendicularVector *= collisionPlane.m_collisionEfficiency * m_collisionEfficiency;

	parallelVector = reflectionVector - normalVector;

	if ((!isOnObject) || ((m_rollingCount % ROLL_SLOWDOWN_FREQUENCY) == 0)) {
		parallelVector -= ((parallelVector * collisionPlane.m_friction) + (parallelVector * m_friction));
	}

	// Now put the reflection vector back together again
	reflectionVector = parallelVector + perpendicularVector;

	m_velocity = reflectionVector;
}

void CCollisionSphere::IncreaseVelocity(float minSpeed) {
	if (m_velocity.Magnitude() < minSpeed) {
		// Make sure we're moving along the XY plane
		if (m_velocity.XYMagnitude() == 0) {
			// Randomly add some velocity
			int random = (g_scumm->_rnd.getRandomNumber(0x7FFF) * 4) / 0x7FFF;
			switch (random) {
			case 0:
				m_velocity.x = minSpeed;
				break;

			case 1:
				m_velocity.x = -minSpeed;
				break;

			case 2:
				m_velocity.y = minSpeed;
				break;

			case 3:
			case 4:
				m_velocity.y = -minSpeed;
				break;

			default:
				debug("Brady doesn't know how to use the rand() function");
			}
		} else {
			// Increase the current velocity
			float oldVelocity = m_velocity.Magnitude();
			m_velocity.x = (m_velocity.x / oldVelocity) * minSpeed;
			m_velocity.y = (m_velocity.y / oldVelocity) * minSpeed;
			m_velocity.z = (m_velocity.z / oldVelocity) * minSpeed;
		}
	}
}

void CCollisionSphere::
	HandleCollisions(CCollisionObjectVector *pCollisionVector,
					 float *pTimeUsed,
					 bool advanceObject) {
	Common::Array<U32Plane> planeVector;
	Common::Array<bool> rollingRecord;
	U32Plane collisionPlane;
	U32Distance3D distance;
	bool isRollingOnObject;

	for (CCollisionObjectVector::const_iterator objectIt = pCollisionVector->begin();
		 objectIt != pCollisionVector->end();
		 ++objectIt) {
		const ICollisionObject *pCurrentObject = *objectIt;

		ICollisionObject::TestObjectIntersection(*pCurrentObject, &distance);

		// See if we are rolling on this object.
		isRollingOnObject = ICollisionObject::IsOnObject(*pCurrentObject, distance);
		if (isRollingOnObject) {
			// See if rolling has slowed to the point that we are stopped.
			// Never stop on the backboard or rim.
			if ((pCurrentObject->m_objectType == kBackboard) ||
				(pCurrentObject->m_objectType == kRim)) {
				IncreaseVelocity(ROLL_SPEED_LIMIT);
			} else {
				if (m_velocity.XYMagnitude() < ROLL_SPEED_LIMIT) {
					m_velocity.x = 0;
					m_velocity.y = 0;
				}
			}

			m_velocity.z = 0;
		} else {
			// See if rolling has slowed to the point that we are stopped.
			// Never stop on the backboard or rim.
			if ((pCurrentObject->m_objectType == kBackboard) ||
				(pCurrentObject->m_objectType == kRim)) {
				IncreaseVelocity(ROLL_SPEED_LIMIT);
			}
		}

		ICollisionObject::DefineReflectionPlane(*pCurrentObject, distance, &collisionPlane);
		planeVector.push_back(collisionPlane);
		rollingRecord.push_back(isRollingOnObject);
	}

	pCollisionVector->clear();

	if (!planeVector.empty()) {
		collisionPlane = planeVector[0];
		isRollingOnObject = rollingRecord[0];

		Common::Array<U32Plane>::const_iterator planeIt;
		Common::Array<bool>::const_iterator recordIt;

		for (planeIt = planeVector.begin(), recordIt = rollingRecord.begin();
			 planeIt != planeVector.end();
			 ++planeIt, ++recordIt) {

			collisionPlane.Average(*planeIt);
			isRollingOnObject = isRollingOnObject || *recordIt;
		}

		ReboundOffPlane(collisionPlane, isRollingOnObject);
	}

	if (advanceObject) {
		// Move the ball for the amount of time remaining
		m_center.x += (m_velocity.x * *pTimeUsed);
		m_center.y += (m_velocity.y * *pTimeUsed);
		m_center.z += (m_velocity.z * *pTimeUsed);

		*pTimeUsed = 0;
	}
}

bool CCollisionSphere::IsOnObject(const CCollisionBox &targetObject,
								  const U32Distance3D &distance) const {
	float distancePercentage = fabs(distance.z - m_radius) / m_radius;

	// See if bouncing has slowed to the point that we are rolling.
	return ((distancePercentage < .1) &&
			(distance.XYMagnitude() == 0) &&
			(fabs(m_velocity.z) <= BOUNCE_SPEED_LIMIT));
}

bool CCollisionSphere::IsOnObject(const CCollisionCylinder &targetObject,
								  const U32Distance3D &distance) const {
	float distancePercentage = fabs(distance.z - m_radius) / m_radius;

	// See if bouncing has slowed to the point that we are rolling.
	return ((distancePercentage < .1) &&
			(distance.XYMagnitude() <= targetObject.m_radius) &&
			(fabs(m_velocity.z) <= BOUNCE_SPEED_LIMIT));
}

U32BoundingBox CCollisionSphere::GetBoundingBox(void) const {
	U32BoundingBox outBox;

	outBox.m_minPoint.x = m_center.x - m_radius;
	outBox.m_minPoint.y = m_center.y - m_radius;
	outBox.m_minPoint.z = m_center.z - m_radius;

	outBox.m_maxPoint.x = m_center.x + m_radius;
	outBox.m_maxPoint.y = m_center.y + m_radius;
	outBox.m_maxPoint.z = m_center.z + m_radius;

	return outBox;
}

U32BoundingBox CCollisionSphere::GetBigBoundingBox(void) const {
	U32BoundingBox outBox;

	float velocity = m_velocity.Magnitude();

	outBox.m_minPoint.x = m_center.x - (m_radius + velocity);
	outBox.m_minPoint.y = m_center.y - (m_radius + velocity);
	outBox.m_minPoint.z = m_center.z - (m_radius + velocity);

	outBox.m_maxPoint.x = m_center.x + (m_radius + velocity);
	outBox.m_maxPoint.y = m_center.y + (m_radius + velocity);
	outBox.m_maxPoint.z = m_center.z + (m_radius + velocity);

	return outBox;
}

void CCollisionSphere::Save(void) {
	m_bPositionSaved = true;
	m_safetyPoint = m_center;
	m_safetyVelocity = m_velocity;
}

void CCollisionSphere::Restore(void) {
	if (m_bPositionSaved) {
		if (m_safetyVelocity.Magnitude() != 0) {
			debug("Restoring");
			m_center = m_safetyPoint;
			// m_velocity = m_safetyVelocity;
			m_velocity.x = 0;
			m_velocity.y = 0;
			m_velocity.z = 0;
		}
	} else {
		debug("No save point.");
	}
}

void CCollisionObjectStack::clear() {
	while (!empty()) {
		pop_back();
	}
}

int CCollisionObjectVector::GetMinPoint(EDimension dimension) const {
	int min = 1000000;
	for (size_type i = 0; i < size(); i++) {
		int current = ((*this)[i])->GetBoundingBox().m_minPoint[dimension];
		if (current < min) {
			min = current;
		}
	}
	return min;
}

int CCollisionObjectVector::GetMaxPoint(EDimension dimension) const {
	int max = 0;
	for (uint i = 0; i < size(); i++) {
		int current = ((*this)[i])->GetBoundingBox().m_minPoint[dimension];
		if (current > max) {
			max = current;
		}
	}
	return max;
}

bool CCollisionObjectVector::Contains(const ICollisionObject &object) const {
	for (CCollisionObjectVector::const_iterator objectIt = begin();
		 objectIt != end();
		 ++objectIt) {
		if (object == **objectIt) {
			return true;
		}
	}

	return false;
}

CCollisionObjectTree::CCollisionObjectTree() : m_maxHeight(INIT_MAX_HEIGHT),
											   m_maxObjectsInNode(INIT_MAX_OBJECTS),
											   m_pRoot(nullptr),
											   m_errorFlag(false) {}

CCollisionObjectTree::CCollisionObjectTree(const CCollisionObjectVector &inputObjects) : m_maxHeight(INIT_MAX_HEIGHT),
																						 m_maxObjectsInNode(INIT_MAX_OBJECTS),
																						 m_pRoot(nullptr),
																						 m_errorFlag(false) {
	Initialize(inputObjects);
}

CCollisionObjectTree::~CCollisionObjectTree() {
	if (m_pRoot) {
		delete m_pRoot;
		m_pRoot = nullptr;
	}
}

void CCollisionObjectTree::Initialize(const CCollisionObjectVector &inputObjects) {
	CCollisionObjectTree::~CCollisionObjectTree();

	U32BoundingBox buildRange;
	buildRange.SetMinPoint(inputObjects.GetMinPoint(X_INDEX), inputObjects.GetMinPoint(Y_INDEX));
	buildRange.SetMaxPoint(inputObjects.GetMaxPoint(X_INDEX), inputObjects.GetMaxPoint(Y_INDEX));
	m_pRoot = BuildSelectionStructure(inputObjects, 0, buildRange);
}

void CCollisionObjectTree::SelectObjectsInBound(const U32BoundingBox &bound, CCollisionObjectVector *pTargetVector) {
	m_pRoot->SearchTree(bound, pTargetVector);

	if (pTargetVector->size() <= 0) {
		warning("Something went really wrong with a collision, ignore and U32 will attempt to correct.");
		m_errorFlag = true;
		return;
	}

	// Here we make sure that we don't have any duplicate objects in our target stack.
	Common::sort(pTargetVector->begin(), pTargetVector->end());

	//CCollisionObjectVector::iterator newEnd = unique(pTargetVector->begin(), pTargetVector->end());
	//pTargetVector->erase(newEnd, pTargetVector->end());
}

bool CCollisionObjectTree::CheckErrors(void) {
	if (m_errorFlag) {
		m_errorFlag = false;
		return true;
	} else {
		return false;
	}
}

CCollisionNode *CCollisionObjectTree::
	BuildSelectionStructure(const CCollisionObjectVector &inputObjects,
							int currentLevel,
							const U32BoundingBox &nodeRange) {
	// create a new node, containing all of the input points.
	CCollisionNode *pNewNode = new CCollisionNode(inputObjects);
	pNewNode->m_quadrant = nodeRange;

	// see if we are at the final level, or if we have reached our target occupancy.
	if ((currentLevel == m_maxHeight) ||
		(inputObjects.size() <= m_maxObjectsInNode)) {
		pNewNode->m_bIsExternal = true;
	} else {
		pNewNode->m_bIsExternal = false;

		// otherwise, break the inputPoints into 4 new point lists.
		CCollisionObjectVector newList[NUM_CHILDREN_NODES];
		U32BoundingBox newRange[NUM_CHILDREN_NODES];
		int childID;

		for (childID = 0; childID < NUM_CHILDREN_NODES; childID++) {
			newRange[childID] = CCollisionNode::GetChildQuadrant(nodeRange, (EChildID)childID);
		}

		// go through each point in the list
		for (size_t i = 0; i < inputObjects.size(); i++) {
			const ICollisionObject *pCurrentObject = inputObjects[i];

			// Figure out which child each point belongs to.
			for (childID = 0; childID < NUM_CHILDREN_NODES; ++childID) {
				if (newRange[childID].Intersect(pCurrentObject->GetBoundingBox())) {
					newList[childID].push_back(pCurrentObject);
				}
			}
		}

		// Make recursive calls.
		for (childID = 0; childID < NUM_CHILDREN_NODES; ++childID) {
			pNewNode->m_pChild[childID] = BuildSelectionStructure(newList[childID],
																  currentLevel + 1,
																  newRange[childID]);
		}
	}

	return pNewNode;
}

int LogicHEBasketball::U32_User_Init_Ball(U32FltPoint3D &ballLocation,
					   U32FltVector3D &bellVelocity,
					   int radius,
					   int ballID) {
	// Initialize basketball
	g_court.m_basketball.m_description = "Basketball";
	g_court.m_basketball.m_objectType = kBall;
	g_court.m_basketball.m_objectID = ballID;
	g_court.m_basketball.m_center = ballLocation;
	g_court.m_basketball.m_velocity = bellVelocity;
	g_court.m_basketball.m_radius = radius;
	g_court.m_basketball.m_collisionEfficiency = 1.0;
	g_court.m_basketball.m_friction = 0;
	g_court.m_basketball.m_ignore = false;

	g_court.m_basketball.Save();

	return 1;
}

int LogicHEBasketball::U32_User_Init_Virtual_Ball(U32FltPoint3D &ballLocation,
							   U32FltVector3D &bellVelocity,
							   int radius,
							   int ballID) {
	// Initialize basketball
	g_court.m_virtualBall.m_description = "Virtual Basketball";
	g_court.m_virtualBall.m_objectType = kBall;
	g_court.m_virtualBall.m_objectID = ballID;
	g_court.m_virtualBall.m_center = ballLocation;
	g_court.m_virtualBall.m_velocity = bellVelocity;
	g_court.m_virtualBall.m_radius = radius;
	g_court.m_virtualBall.m_collisionEfficiency = 1.0;
	g_court.m_virtualBall.m_friction = 0;
	g_court.m_virtualBall.m_ignore = false;

	g_court.m_virtualBall.Save();

	return 1;
}

int LogicHEBasketball::U32_User_Deinit_Ball(void) {
	// Deinitialize basketball
	g_court.m_basketball.m_ignore = true;

	return 1;
}

int LogicHEBasketball::U32_User_Deinit_Virtual_Ball(void) {
	// Deinitialize virtual basketball
	g_court.m_virtualBall.m_ignore = true;

	return 1;
}

int LogicHEBasketball::U32_User_Init_Player(int playerID,
						 U32FltPoint3D &playerLocation,
						 int height,
						 int radius,
						 bool bPlayerIsInGame) {
	if (!((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER)))
		error("Passed in invalid player ID");

	// Cycle through all of the player slots until an empty one is found.
	Common::Array<CCollisionPlayer> *pPlayerList = _vm->_basketball->g_court.GET_PLAYER_LIST_PTR(playerID);
	if (pPlayerList->size() < MAX_PLAYERS_ON_TEAM) {
		CCollisionPlayer newPlayer;
		newPlayer.m_objectType = kPlayer;
		newPlayer.m_objectID = playerID;
		newPlayer.m_height = height;
		newPlayer.m_catchHeight = PLAYER_CATCH_HEIGHT;
		newPlayer.m_radius = radius;
		newPlayer.m_center = playerLocation;
		newPlayer.m_center.z = playerLocation.z + (height / 2);
		newPlayer.m_collisionEfficiency = .5;
		newPlayer.m_friction = .5;
		newPlayer.m_playerIsInGame = bPlayerIsInGame;
		newPlayer.Save();
		pPlayerList->push_back(newPlayer);
		return 1;
	} else {
		error("There were no empty player slots.  You can't initialize a new player until you deinit one.");
		return 0;
	}
}

int LogicHEBasketball::U32_User_Deinit_Player(int playerID) {
	if (!((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER)))
		error("Passed in invalid player ID");

	int index = _vm->_basketball->g_court.GetPlayerIndex(playerID);
	Common::Array<CCollisionPlayer> *pPlayerList = _vm->_basketball->g_court.GET_PLAYER_LIST_PTR(playerID);
	pPlayerList->remove_at(index);

	return 1;
}

int LogicHEBasketball::U32_User_Player_Off(int playerID) {
	if (!((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER)))
		error("Passed in invalid player ID");

	_vm->_basketball->g_court.GetPlayerPtr(playerID)->m_ignore = true;

	return 1;
}

int LogicHEBasketball::U32_User_Player_On(int playerID) {
	if (!((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER)))
		error("Passed in invalid player ID");

	_vm->_basketball->g_court.GetPlayerPtr(playerID)->m_ignore = false;

	return 1;
}

static void TrackCollisionObject(const ICollisionObject &sourceObject, const ICollisionObject &targetObject, CCollisionObjectVector *pObjectVector) {
	float currentDist = sourceObject.GetObjectDistance(targetObject);

	// As an object moves backwards along its velocity vector and new collisions
	// are detected, older collisions may become invalid.  Here, we go through prior
	// collisions, and see which ones are invalidated by this new collision.
	for (CCollisionObjectVector::const_iterator objectIt = pObjectVector->begin();
		 objectIt != pObjectVector->end();
		 ++objectIt) {
		float pastDist = sourceObject.GetObjectDistance(**objectIt);

		// If the distance between the source object and the current target object
		// is less than or equal to the distance between the source object and the last
		// target object, then the current target object is stored along side the last
		// target object.  Otherwise, the current object replaces the last object.
		if ((fabs(pastDist - currentDist) < COLLISION_EPSILON) ||
			(!sourceObject.IsCollisionHandled(targetObject)) ||
			(!sourceObject.IsCollisionHandled(**objectIt))) {
			break;
		}
	}

	// Make sure that we aren't keeping track of the same object twice.
	if (!pObjectVector->Contains(targetObject)) {
		pObjectVector->push_back(&targetObject);
	}
}

int LogicHEBasketball::U32_User_Detect_Ball_Collision(U32FltPoint3D &ballLocation, U32FltVector3D &ballVector, int recordCollision, int ballID) {
	bool bBallIsClear = false; // Flag that indicates if the ball collided with
							   // any objects on its current vector.
	bool bErrorOccurred = false;

	U32Distance3D distance;                 // The distance between the ball and a collision object candidate.
	CCollisionObjectVector targetList;      // All potential collision candidates.
	CCollisionObjectVector collisionVector; // All objects that have been collided with.
	CCollisionObjectVector rollingVector;   // All objects that have been rolled on.

	int bCollisionOccurred = 0;
	int bRollingHappened = 0;

	// Determine which ball we're dealing with.
	CCollisionBasketball *pSourceBall = _vm->_basketball->g_court.GetBallPtr(ballID);

	// Update the position and vector of the basketball.
	pSourceBall->m_center = ballLocation;
	pSourceBall->m_velocity = ballVector;

	// Clear the ball's collision stack
	pSourceBall->m_objectCollisionHistory.clear();
	pSourceBall->m_objectRollingHistory.clear();

	// Find out who our potential collision mates are.
	_vm->_basketball->FillBallTargetList((CCollisionSphere *)pSourceBall, &targetList);

	// See if there was an error while traversing the object tree.
	if (g_court.m_objectTree.CheckErrors()) {
		// Put the ball in the lastk known safe position.
		pSourceBall->Restore();
	}

	for (int i = 0; (i < MAX_BALL_COLLISION_PASSES) && (!bBallIsClear); i++) {
		float totalTime = 0; // The amount of time it takes to back out of all objects
							 // we have intersected while on the current vector.
		bBallIsClear = 1;

		// Go through all of the collision candidates.
		for (size_t j = 0; j < targetList.size(); ++j) {
			const ICollisionObject *pTargetObject = targetList[j];
			assert(pTargetObject);

			// See if we intersect the current object
			bool bIntersectionResult = pSourceBall->ICollisionObject::TestObjectIntersection(*pTargetObject, &distance);
			if (bIntersectionResult) {
				// If we are intersecting a moving object, make sure that we actually
				// ran into them, and they didn't just run into us.
				if (pSourceBall->ICollisionObject::ValidateCollision(*pTargetObject, &distance)) {
					// If we are intersecting the object, back out of it.
					if (pSourceBall->ICollisionObject::BackOutOfObject(*pTargetObject, &distance, &totalTime)) {
						// Move in to the exact point of collision.
						if (pSourceBall->ICollisionObject::NudgeObject(*pTargetObject, &distance, &totalTime)) {
							// assert2(totalTime <= 1.1, "The ball has used up all of its time for this frame")

							// Keep track of this object so we can respond to the collision later.
							TrackCollisionObject(*(ICollisionObject *)pSourceBall, *pTargetObject, &pSourceBall->m_objectCollisionHistory);

							if (pSourceBall->IsCollisionHandled(*pTargetObject)) {
								TrackCollisionObject(*(ICollisionObject *)pSourceBall, *pTargetObject, &collisionVector);
								bBallIsClear = false;
							}

							bCollisionOccurred = 1;
						} else {
							bErrorOccurred = true;
						}
					} else {
						bErrorOccurred = true;
					}
				}

			} else {

				// See if we are passing over a player.
				if (pSourceBall->TestCatch(*pTargetObject, &distance)) {
					TrackCollisionObject(*pSourceBall, *pTargetObject, &pSourceBall->m_objectCollisionHistory);
					bCollisionOccurred = true;
				}
			}

			// See if we are rolling on the current object
			if (pSourceBall->ICollisionObject::IsOnObject(*pTargetObject, distance)) {
				bRollingHappened = 1;

				if (!bIntersectionResult) {
					//  This is not really a collision, but the ball is rolling, so we want to slow it down
					TrackCollisionObject(*(ICollisionObject *)pSourceBall, *pTargetObject, &rollingVector);
					TrackCollisionObject(*(ICollisionObject *)pSourceBall, *pTargetObject, &pSourceBall->m_objectRollingHistory);
				}
			}
		}

		// Adjust the ball's velocity and position due to any collisions
		pSourceBall->HandleCollisions(&rollingVector, &totalTime, false);
		pSourceBall->HandleCollisions(&collisionVector, &totalTime, true);
	}

	// Keep track of how long we've been rolling.
	if (bRollingHappened) {
		++pSourceBall->m_rollingCount;
	} else {
		pSourceBall->m_rollingCount = 0;
	}

	// If we didn't hit anything this frame, then we know we are probably in an ok
	// place.  Go ahead and save the position.
	if (!bErrorOccurred) {
		pSourceBall->Save();
	}

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, _vm->_basketball->U32_Float_To_Int(pSourceBall->m_center.x));
	writeScummVar(_vm1->VAR_U32_USER_VAR_B, _vm->_basketball->U32_Float_To_Int(pSourceBall->m_center.y));
	writeScummVar(_vm1->VAR_U32_USER_VAR_C, _vm->_basketball->U32_Float_To_Int(pSourceBall->m_center.z));
	writeScummVar(_vm1->VAR_U32_USER_VAR_D, _vm->_basketball->U32_Float_To_Int(pSourceBall->m_velocity.x));
	writeScummVar(_vm1->VAR_U32_USER_VAR_E, _vm->_basketball->U32_Float_To_Int(pSourceBall->m_velocity.y));
	writeScummVar(_vm1->VAR_U32_USER_VAR_F, _vm->_basketball->U32_Float_To_Int(pSourceBall->m_velocity.z));
	writeScummVar(_vm1->VAR_U32_USER_VAR_G, bCollisionOccurred);
	writeScummVar(_vm1->VAR_U32_USER_VAR_H, bRollingHappened != 0 ? 0 : 1);

	return 1;
}

int LogicHEBasketball::U32_User_Detect_Player_Collision(int playerID, U32FltPoint3D &playerLocation, U32FltVector3D &playerVector, bool bPlayerHasBall) {
	U32Distance3D distance;                 // The distance between the ball and a collision object candidate.
	CCollisionObjectVector collisionVector; // All objects that have been collided with.

	int bPlayerIsOnObject = 0;
	int bCollisionOccurred = 0;

	bool bPlayerIsClear = false;
	bool bErrorOccurred = false;

	float totalTime = 0; // The amount of time it takes to back out of all objects
						 // we have intersected this frame.

	if (!((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER)))
		error("Passed in invalid player ID");

	// Get a pointer to the player that is being tested.
	CCollisionPlayer *pSourcePlayer = _vm->_basketball->g_court.GetPlayerPtr(playerID);

	// Update the player's status
	pSourcePlayer->m_playerHasBall = bPlayerHasBall;

	// In SCUMM code, the center of a player in the z dimension is at their feet.
	// In U32 code, it is in the middle of the cylinder.  Make the translation here.
	playerLocation.z += (pSourcePlayer->m_height / 2);

	// Update the player's position and velocity.
	pSourcePlayer->m_center = playerLocation;
	pSourcePlayer->m_velocity = playerVector;
	pSourcePlayer->m_movementType = kStraight;

	// Clear the player's collision stack
	pSourcePlayer->m_objectCollisionHistory.clear();
	pSourcePlayer->m_objectRollingHistory.clear();

	// Find out who our potential collision mates are.
	CCollisionObjectVector targetList;
	_vm->_basketball->FillPlayerTargetList(pSourcePlayer, &targetList);

	// See if there was an error while traversing the object tree.
	if (g_court.m_objectTree.CheckErrors()) {
		// Put the player in the last known safe position.
		pSourcePlayer->Restore();
	}

	for (int i = 0; (i < MAX_PLAYER_COLLISION_PASSES) && (!bPlayerIsClear); i++) {
		bPlayerIsClear = 1;

		// Check all of the collision candidates.
		for (size_t j = 0; j < targetList.size(); ++j) {
			const ICollisionObject *pTargetObject = targetList[j];
			assert(pTargetObject);

			// See if we intersect the current object
			bool bIntersectionResult = pSourcePlayer->ICollisionObject::TestObjectIntersection(*pTargetObject, &distance);
			if (bIntersectionResult) {
				// If we are intersecting a moving object, make sure that we actually
				// ran into them, and they didn't just run into us.
				if (pSourcePlayer->ICollisionObject::ValidateCollision(*pTargetObject, &distance)) {
					// If we are intersecting an object, back out to the exact point of collision.
					if (pSourcePlayer->ICollisionObject::BackOutOfObject(*pTargetObject, &distance, &totalTime)) {
						// Move in to the exact point of collision.
						if (pSourcePlayer->ICollisionObject::NudgeObject(*pTargetObject, &distance, &totalTime)) {
							// assert2(totalTime <= 1.1, "A player has used up all of its time for this frame");

							TrackCollisionObject(*pSourcePlayer, *pTargetObject, &pSourcePlayer->m_objectCollisionHistory);
							bCollisionOccurred = true;

							if (pSourcePlayer->ICollisionObject::IsCollisionHandled(*pTargetObject)) {
								TrackCollisionObject(*pSourcePlayer, *pTargetObject, &collisionVector);
								bPlayerIsClear = false;
							}
						} else {
							bErrorOccurred = true;
						}
					} else {
						bErrorOccurred = true;
					}
				}

			} else {

				// See if the virtual ball is passing over us.
				if (pSourcePlayer->TestCatch(*pTargetObject, &distance)) {
					TrackCollisionObject(*pSourcePlayer, *pTargetObject, &pSourcePlayer->m_objectCollisionHistory);
					bCollisionOccurred = true;
				}
			}

			// See if we are standing on the current object
			if (pSourcePlayer->ICollisionObject::IsOnObject(*pTargetObject, distance)) {
				bPlayerIsOnObject = true;
			}
		}

		pSourcePlayer->HandleCollisions(&collisionVector, &totalTime, true);
	}

	// If there were no errors this frame, then we know we are probably in an ok
	// place.  Go ahead and save the position.
	if (!bErrorOccurred) {
		pSourcePlayer->Save();
	}

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, _vm->_basketball->U32_Float_To_Int(pSourcePlayer->m_center.x));
	writeScummVar(_vm1->VAR_U32_USER_VAR_B, _vm->_basketball->U32_Float_To_Int(pSourcePlayer->m_center.y));
	writeScummVar(_vm1->VAR_U32_USER_VAR_C, _vm->_basketball->U32_Float_To_Int(pSourcePlayer->m_center.z - (pSourcePlayer->m_height / 2)));
	writeScummVar(_vm1->VAR_U32_USER_VAR_D, _vm->_basketball->U32_Float_To_Int(pSourcePlayer->m_velocity.x));
	writeScummVar(_vm1->VAR_U32_USER_VAR_E, _vm->_basketball->U32_Float_To_Int(pSourcePlayer->m_velocity.y));
	writeScummVar(_vm1->VAR_U32_USER_VAR_F, _vm->_basketball->U32_Float_To_Int(pSourcePlayer->m_velocity.z));
	writeScummVar(_vm1->VAR_U32_USER_VAR_G, bCollisionOccurred);
	writeScummVar(_vm1->VAR_U32_USER_VAR_H, bPlayerIsOnObject != 0 ? 0 : 1);

	return 1;
}

int LogicHEBasketball::U32_User_Get_Last_Ball_Collision(int ballID) {
	EObjectType eLastObjectType = kNoObjectType;
	int objectID = 0;

	// Determine which ball we're dealing with.
	CCollisionSphere *pSourceBall;
	if (ballID == g_court.m_basketball.m_objectID) {
		pSourceBall = (CCollisionSphere *)&g_court.m_basketball;
	} else if (ballID == g_court.m_virtualBall.m_objectID) {
		pSourceBall = (CCollisionSphere *)&g_court.m_virtualBall;
	} else {
		debug("Invalid ball ID passed to U32_User_Get_Last_Ball_Collision.");
		pSourceBall = (CCollisionSphere *)&g_court.m_basketball;
	}

	if (!pSourceBall->m_objectCollisionHistory.empty()) {
		eLastObjectType = pSourceBall->m_objectCollisionHistory.back()->m_objectType;
		objectID = pSourceBall->m_objectCollisionHistory.back()->m_objectID;
		pSourceBall->m_objectCollisionHistory.pop_back();
	} else if (!pSourceBall->m_objectRollingHistory.empty()) {
		eLastObjectType = pSourceBall->m_objectRollingHistory.back()->m_objectType;
		objectID = pSourceBall->m_objectRollingHistory.back()->m_objectID;
		pSourceBall->m_objectRollingHistory.pop_back();
	}

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, eLastObjectType);
	writeScummVar(_vm1->VAR_U32_USER_VAR_B, objectID);

	return 1;
}

int LogicHEBasketball::U32_User_Get_Last_Player_Collision(int playerID) {
	EObjectType eLastObjectType = kNoObjectType;
	int objectID = 0;
	bool bPlayerIsOnObject = false;

	CCollisionPlayer *pPlayer = _vm->_basketball->g_court.GetPlayerPtr(playerID);

	if (!pPlayer->m_objectCollisionHistory.empty()) {
		const ICollisionObject *pTargetObject = pPlayer->m_objectCollisionHistory.back();

		eLastObjectType = pTargetObject->m_objectType;
		objectID = pTargetObject->m_objectID;

		// See if we are standing on the current object
		U32Distance3D distance;
		pPlayer->ICollisionObject::TestObjectIntersection(*pTargetObject, &distance);
		if (pPlayer->ICollisionObject::IsOnObject(*pTargetObject, distance)) {
			bPlayerIsOnObject = true;
		}

		pPlayer->m_objectCollisionHistory.pop_back();
	}

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, eLastObjectType);
	writeScummVar(_vm1->VAR_U32_USER_VAR_B, objectID);
	writeScummVar(_vm1->VAR_U32_USER_VAR_C, bPlayerIsOnObject);

	return 1;
}

} // End of namespace Scumm
