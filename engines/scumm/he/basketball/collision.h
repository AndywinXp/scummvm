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

#ifndef SCUMM_HE_BASKETBALL_COLLISION_H
#define SCUMM_HE_BASKETBALL_COLLISION_H

#ifdef ENABLE_HE

#include "common/algorithm.h"

#include "scumm/he/intern_he.h"
#include "scumm/he/logic_he.h"
#include "scumm/he/basketball/basketball.h"
#include "scumm/he/basketball/geotypes.h"

namespace Scumm {

#define RIM_CE                       (float).4
#define BACKBAORD_CE                 (float).3
#define FLOOR_CE                     (float).65

#define LEFT_BASKET                  0
#define RIGHT_BASKET                 1

#define RIM_WIDTH                    (WORLD_UNIT_MULTIPLIER / 11)
#define RIM_RADIUS                   ((3 * WORLD_UNIT_MULTIPLIER) / 4)

#define MAX_BALL_COLLISION_PASSES    10
#define MAX_PLAYER_COLLISION_PASSES  3

#define INIT_MAX_HEIGHT  10
#define INIT_MAX_OBJECTS 5

#define NUM_CHILDREN_NODES 4

#define WEST_SHIELD_ID    0
#define NORTH_SHIELD_ID   1
#define EAST_SHIELD_ID    2
#define SOUTH_SHIELD_ID   3
#define TOP_SHIELD_ID     4
#define ALL_SHIELD_ID     5
#define MAX_SHIELD_COUNT  5

#define BACKUP_WEST_SHIELD_ID  6
#define BACKUP_NORTH_SHIELD_ID 7
#define BACKUP_EAST_SHIELD_ID  8
#define BACKUP_SOUTH_SHIELD_ID 9

#define PLAYER_SHIELD_INCREMENT_VALUE 25


const float SMALL_TIME_INCREMENT = (float).05F;
const float BACK_OUT_TIME_LIMIT = (float)20.0F;
const int MAX_STEP_HEIGHT = 50;
const int PLAYER_CATCH_HEIGHT = 250;
const int SHIELD_DEPTH = 1000;
const int SHIELD_HEIGHT = 50000;
const int SHIELD_EXTENSION = 20000;
const int BUFFER_WIDTH = 800;
const int BOUNCE_SPEED_LIMIT = 20; // If the z-direction speed of the ball falls below this limit during a bounce, then the ball will begin to roll.
const int ROLL_SPEED_LIMIT = 20;   // If the xy-direction speed of the ball falls below this limit during a roll, then the ball will stop.
const int ROLL_SLOWDOWN_FREQUENCY = 10; // The ball slows down every x frames while rolling.

class CCollisionSphere;
class CCollisionBox;
class CCollisionCylinder;
class CCollisionObjectStack;
class CCollisionObjectVector;
class ICollisionObject;

enum EObjectShape {
	kNoObjectShape = 0,
	kSphere = 1,
	kBox = 2,
	kCylinder = 3
};

enum EObjectType {
	kNoObjectType = 0,
	kBackboard = 1,
	kRim = 2,
	kOtherType = 3,
	kFloor = 4,
	kBall = 5,
	kPlayer = 6
};

enum EChildID {
	kChild1 = 0,
	kChild2 = 1,
	kChild3 = 2,
	kChild4 = 3
};

// An object's velocity can either be carried out as straight or circular movement.
enum EMovementType {
	kStraight = 0,
	kCircular = 1 // 2D circular motion along the horizon plane.
};


class CCollisionObjectStack : public Common::Array<const ICollisionObject *> {
public:
	void clear();
};

class CCollisionObjectVector : public Common::Array<const ICollisionObject *> {
public:
	int GetMinPoint(EDimension dimension) const;
	int GetMaxPoint(EDimension dimension) const;
	bool Contains(const ICollisionObject &object) const;
};

class ICollisionObject {
public:
	ICollisionObject(EObjectShape shape = kNoObjectShape);
	~ICollisionObject();

	// Comparison operator
	bool operator==(const ICollisionObject &otherObject) const;

	// Get the distance between the outside of this object and the outside of a target object.
	virtual float GetObjectDistance(const ICollisionObject &targetObject) const;
	virtual float GetObjectDistance(const CCollisionSphere &targetObject) const;
	virtual float GetObjectDistance(const CCollisionBox &targetObject) const;
	virtual float GetObjectDistance(const CCollisionCylinder &targetObject) const;

	// TestObjectIntersection is used to determine if this object is intersecting a target object.
	virtual bool TestObjectIntersection(const ICollisionObject &targetObject, U32Distance3D *pDistance) const;
	virtual bool TestObjectIntersection(const CCollisionSphere &targetObject, U32Distance3D *pDistance) const;
	virtual bool TestObjectIntersection(const CCollisionBox &targetObject, U32Distance3D *pDistance) const;
	virtual bool TestObjectIntersection(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) const;

	// Just because this object is intersecting another, doesn't mean that it collided with
	// that object.  That object could have collided with us.  This function verifies, that
	// we collided with another object.
	virtual bool ValidateCollision(const ICollisionObject &targetObject, U32Distance3D *pDistance);
	virtual bool ValidateCollision(const CCollisionSphere &targetObject, U32Distance3D *pDistance);
	virtual bool ValidateCollision(const CCollisionBox &targetObject, U32Distance3D *pDistance);
	virtual bool ValidateCollision(const CCollisionCylinder &targetObject, U32Distance3D *pDistance);

	// BackOutOfObject moves this object backwards along its velocity vector to a point
	// where it is guaranteed not to be intersecting a target object.
	virtual bool BackOutOfObject(const ICollisionObject &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	virtual bool BackOutOfObject(const CCollisionSphere &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	virtual bool BackOutOfObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	virtual bool BackOutOfObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed);

	// NudgeObject moves this object forward along its velocity vector to a point
	// where it is guaranteed to be touching the target object at the exact point
	// of collision.
	virtual bool NudgeObject(const ICollisionObject &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	virtual bool NudgeObject(const CCollisionSphere &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	virtual bool NudgeObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	virtual bool NudgeObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed);

	// Some collisions between objects are recorded, but not handled physically.  This
	// function determines whether a collision gets handled or not.
	virtual bool IsCollisionHandled(const ICollisionObject &targetObject) const;
	virtual bool IsCollisionHandled(const CCollisionSphere &targetObject) const;
	virtual bool IsCollisionHandled(const CCollisionBox &targetObject) const;
	virtual bool IsCollisionHandled(const CCollisionCylinder &targetObject) const;

	// HandleCollisions alters this objects velocity and position as a result of
	// a collision with one or more target objects.
	virtual void HandleCollisions(CCollisionObjectVector *pCollisionVector, float *pTimeUsed, bool advanceObject);
	virtual void HandleCollision(const CCollisionSphere &targetObject, float *pTimeUsed, U32Distance3D *pDistance, bool advanceObject);
	virtual void HandleCollision(const CCollisionBox &targetObject, float *pTimeUsed, U32Distance3D *pDistance, bool advanceObject);
	virtual void HandleCollision(const CCollisionCylinder &targetObject, float *pTimeUsed, U32Distance3D *pDistance, bool advanceObject);

	// Determine if this object is resting, running, or rolling on another object.
	// An object doing this would not necessarily trigger an intersection.
	virtual bool IsOnObject(const ICollisionObject &targetObject, const U32Distance3D &distance) const;
	virtual bool IsOnObject(const CCollisionSphere &targetObject, const U32Distance3D &distance) const;
	virtual bool IsOnObject(const CCollisionBox &targetObject, const U32Distance3D &distance) const;
	virtual bool IsOnObject(const CCollisionCylinder &targetObject, const U32Distance3D &distance) const;

	// Return a 3-dimensional bounding box for this object.
	virtual U32BoundingBox GetBoundingBox() const = 0;
	virtual U32BoundingBox GetBigBoundingBox() const = 0;

	// Return the point on the surface of the object that is closest to the input point.
	virtual U32FltPoint3D FindNearestPoint(const U32FltPoint3D &testPoint) const;

	// Call this function when it is known that the object is at a legal location and is
	// not intersecting any other objects.
	virtual void Save();

	// Put the object at the last place that it was guaranteed to not be intersecting any
	// other objects, and negate it's velocity vector.
	virtual void Restore();

	Common::String m_description;
	EObjectShape m_objectShape;
	EObjectType m_objectType;
	int m_objectID; // An identifier that uniquely identifies this object.

	U32FltVector3D m_velocity;

	float m_collisionEfficiency; // A multiplier (range 0 to 1) that represents
								 // the percentage of velocity (perpindicular with
								 // the collision) that a target object retains
								 // when it collides with this object.

	float m_friction; // A multiplier (range 0 to 1) that represents
					  // the percentage of velocity (parallel with
					  // the collision) that a target object loses
					  // when it collides with this object.

	int m_soundNumber;

	bool m_ignore; // There could be times when you want to ignore
				   // an object for collision purposes.  This flag
				   // determines that.

	CCollisionObjectVector m_objectCollisionHistory; // Stack with pointers to all the
													 // objects that this object collided
													 // with this frame.
	CCollisionObjectVector m_objectRollingHistory;   // Stack with pointers to all the
													 // objects that this object rolled
													 // on this frame.

protected:
	// Get the amount of time it took this sphere to penetrate whatever object it is
	// currently intersecting in the given dimension.
	virtual float GetPenetrationTime(const ICollisionObject &targetObject, const U32Distance3D &distance, EDimension dimension) const;
	virtual float GetPenetrationTime(const CCollisionSphere &targetObject, const U32Distance3D &distance, EDimension dimension) const;
	virtual float GetPenetrationTime(const CCollisionBox &targetObject, const U32Distance3D &distance, EDimension dimension) const;
	virtual float GetPenetrationTime(const CCollisionCylinder &targetObject, const U32Distance3D &distance, EDimension dimension) const;

	// Sometimes when this object collides with another object, it may want to treat
	// the object at the point of collision like a plane.  This can help simplify the
	// collision.  DefineReflectionPlane defines a plane between this object and the
	// target object at the point of collision.
	virtual void DefineReflectionPlane(const ICollisionObject &targetObject, const U32Distance3D &distance, U32Plane *pCollisionPlane) const;
	virtual void DefineReflectionPlane(const CCollisionSphere &targetObject, const U32Distance3D &distance, U32Plane *pCollisionPlane) const;
	virtual void DefineReflectionPlane(const CCollisionBox &targetObject, const U32Distance3D &distance, U32Plane *pCollisionPlane) const;
	virtual void DefineReflectionPlane(const CCollisionCylinder &targetObject, const U32Distance3D &distance, U32Plane *pCollisionPlane) const;
};

class CCollisionObjectTree {
public:
	CCollisionObjectTree();
	CCollisionObjectTree(const CCollisionObjectVector &inputObjects);
	~CCollisionObjectTree();

	void Initialize(const CCollisionObjectVector &inputObjects);
	void SelectObjectsInBound(const U32BoundingBox &bound, CCollisionObjectVector *pTargetVector);

	bool CheckErrors();

private:
	CCollisionNode *BuildSelectionStructure(const CCollisionObjectVector &inputObjects, int currentLevel, const U32BoundingBox &nodeRange);

	int m_maxHeight;
	size_t m_maxObjectsInNode;
	CCollisionNode *m_pRoot;
	bool m_errorFlag;
};

class CCollisionNode {
public:
	CCollisionNode();
	CCollisionNode(const CCollisionObjectVector &initObjects);
	~CCollisionNode();

	friend class CCollisionObjectTree;

private:
	void SearchTree(const U32BoundingBox &searchRange, CCollisionObjectVector *pTargetList) const;
	static U32BoundingBox GetChildQuadrant(const U32BoundingBox &parentQuadrant, EChildID childID);

	// children nodes to this node
	CCollisionNode *m_pChild[NUM_CHILDREN_NODES];

	// A list of object pointers that are contained within this node.
	CCollisionObjectVector m_objectList;

	// The area that is handled by this node.
	U32BoundingBox m_quadrant;

	// Whether or not this is a leaf node.
	bool m_bIsExternal;
};

class CCollisionCylinder : public ICollisionObject, public U32Cylinder {
public:
	CCollisionCylinder();
	~CCollisionCylinder();

	float GetObjectDistance(const CCollisionSphere &targetObject) const override;
	float GetObjectDistance(const CCollisionBox &targetObject) const override;
	float GetObjectDistance(const CCollisionCylinder &targetObject) const override;

	bool TestObjectIntersection(const CCollisionSphere &targetObject, U32Distance3D *distance) const override;
	bool TestObjectIntersection(const CCollisionBox &targetObject, U32Distance3D *distance) const override;
	bool TestObjectIntersection(const CCollisionCylinder &targetObject, U32Distance3D *distance) const override;

	bool ValidateCollision(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) override;

	bool BackOutOfObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;
	bool BackOutOfObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;

	bool NudgeObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;
	bool NudgeObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;

	bool IsCollisionHandled(const CCollisionSphere &targetObject) const override { return false; };

	void HandleCollisions(CCollisionObjectVector *pCollisionVector, float *pTimeUsed, bool advanceObject) override;
	void HandleCollision(const CCollisionCylinder &targetCylinder, float *pTimeUsed, U32Distance3D *pDistance, bool advanceObject) override;
	void HandleCollision(const CCollisionBox &targetBox, float *pTimeUsed, U32Distance3D *pDistance, bool advanceObject) override;

	bool IsOnObject(const CCollisionSphere &targetObject, const U32Distance3D &distance) const override;
	bool IsOnObject(const CCollisionBox &targetObject, const U32Distance3D &distance) const override;
	bool IsOnObject(const CCollisionCylinder &targetObject, const U32Distance3D &distance) const override;

	U32FltPoint3D FindNearestPoint(const U32FltPoint3D &testPoint) const override;
	int GetEquidistantPoint(U32FltPoint2D inPoint1, float distance1, U32FltPoint2D inPoint2, float distance2, U32FltPoint2D *pOutPoint1, U32FltPoint2D *pOutPoint2);

	U32BoundingBox GetBoundingBox() const override;
	U32BoundingBox GetBigBoundingBox() const override;

	void Save() override;
	void Restore() override;

	float m_height;
	float m_shieldRadius; // Sometimes we may want ot temporarily increase a player's
						  // effective collision radius.  This value keeps track of
						  // how much their radius has been increased by.

	EMovementType m_movementType;

	const ICollisionObject *m_pRevCenter; // If an object is moving with circular motion,
										  // this is the object it is revolving around.
	U32FltPoint2D m_revCenterPt;          // This is the center of the m_pRevCenter.

protected:
	float GetDimensionDistance(const CCollisionBox &targetObject, EDimension dimension) const;
	float GetDimensionDistance(const CCollisionSphere &targetObject, EDimension dimension) const;
	float GetDimensionDistance(const CCollisionCylinder &targetObject, EDimension dimension) const;

private:
	bool m_bPositionSaved;
	U32FltPoint3D m_safetyPoint;
	U32FltVector3D m_safetyVelocity;

	bool BackStraightOutOfObject(const ICollisionObject &targetObject, U32Distance3D *pDistance, float *pTimeUsed);

	bool CircleOutOfObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed);
	bool CircleOutOfObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed);

	ERevDirection GetRevDirection() const;

	float GetPenetrationTime(const CCollisionBox &targetObject, const U32Distance3D &distance, EDimension dimension) const override;
	float GetPenetrationTime(const CCollisionCylinder &targetObject, const U32Distance3D &distance, EDimension dimension) const override;

	float GetPointOfCollision(const CCollisionBox &targetBox, U32Distance3D distance, EDimension dimension) const;

	void ForceOutOfObject(const ICollisionObject &targetObject, U32Distance3D *pDistance);

	bool GetCornerIntersection(const CCollisionBox &targetObject, const U32Distance3D &distance, U32FltPoint2D *pIntersection);

};

class CCollisionBasketball : public CCollisionSphere {
public:
	CCollisionBasketball();
	~CCollisionBasketball();

	bool TestCatch(const ICollisionObject &targetObject, U32Distance3D *distance);
};

class CCollisionBox : public ICollisionObject, public U32BoundingBox {

public:
	CCollisionBox();
	~CCollisionBox();

	U32FltPoint3D FindNearestPoint(const U32FltPoint3D &testPoint) const override;
	U32BoundingBox GetBoundingBox() const override;
	U32BoundingBox GetBigBoundingBox() const override;

};

class CCollisionPlayer : public CCollisionCylinder {
public:
	CCollisionPlayer();
	~CCollisionPlayer();

	void StartBlocking(int blockHeight, int blockTime);
	void HoldBlocking();
	void EndBlocking();

	bool m_playerHasBall;
	bool m_playerIsInGame;

	int m_catchHeight; // The extra height that a player gets for detecting the ball.

	bool TestCatch(const ICollisionObject &targetObject, U32Distance3D *pDistance);

private:
	int m_blockHeight;    // The extra height that a player gets when blocking.
	int m_maxBlockHeight; // The max extra height that a player gets when blocking.
	int m_blockTime;      // The time it takes from the start of a block, to the apex of the block.
};

class CCollisionShieldVector : public Common::Array<CCollisionBox> {
public:
	CCollisionShieldVector();
	~CCollisionShieldVector();

	int m_shieldUpCount;
};

class CCollisionSphere : public ICollisionObject, public U32Sphere {
public:
	CCollisionSphere();
	~CCollisionSphere();

	float GetObjectDistance(const CCollisionBox &targetObject) const override;
	float GetObjectDistance(const CCollisionCylinder &targetObject) const override;

	bool TestObjectIntersection(const CCollisionBox &targetObject, U32Distance3D *pDistance) const override;
	bool TestObjectIntersection(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) const override;

	bool ValidateCollision(const CCollisionBox &targetObject, U32Distance3D *pDistance) override;
	bool ValidateCollision(const CCollisionCylinder &targetObject, U32Distance3D *pDistance) override;

	bool BackOutOfObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;
	bool BackOutOfObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;

	bool NudgeObject(const CCollisionBox &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;
	bool NudgeObject(const CCollisionCylinder &targetObject, U32Distance3D *pDistance, float *pTimeUsed) override;

	void HandleCollisions(CCollisionObjectVector *pCollisionVector, float *pTimeUsed, bool advanceObject) override;

	bool IsOnObject(const CCollisionBox &targetObject, const U32Distance3D &distance) const override;
	bool IsOnObject(const CCollisionCylinder &targetObject, const U32Distance3D &distance) const override;

	U32BoundingBox GetBoundingBox() const override;
	U32BoundingBox GetBigBoundingBox() const override;

	void Save() override;
	void Restore() override;

	int m_rollingCount;

	// using ICollisionObject::m_objectCollisionHistory;
	// using ICollisionObject::m_objectRollingHistory;

protected:
	float GetDimensionDistance(const CCollisionBox &targetObject, EDimension dimension) const;
	float GetDimensionDistance(const CCollisionCylinder &targetObject, EDimension dimension) const;

private:
	bool m_bPositionSaved;
	U32FltPoint3D m_safetyPoint;
	U32FltVector3D m_safetyVelocity;

	bool BackStraightOutOfObject(const ICollisionObject &targetObject, U32Distance3D *pDistance, float *pTimeUsed);

	void DefineReflectionPlane(const CCollisionBox &targetObject, const U32Distance3D &distance, U32Plane *collisionPlane) const override;
	void DefineReflectionPlane(const CCollisionCylinder &targetObject, const U32Distance3D &distance, U32Plane *collisionPlane) const override;

	void ReboundOffPlane(const U32Plane &collisionPlane, bool isOnPlane);

	float GetPenetrationTime(const CCollisionBox &targetObject, const U32Distance3D &distance, EDimension dimension) const override;
	float GetPenetrationTime(const CCollisionCylinder &targetObject, const U32Distance3D &distance, EDimension dimension) const override;

	void IncreaseVelocity(float minSpeed);
};

} // End of namespace Scumm

#endif // ENABLE_HE

#endif // SCUMM_HE_BASKETBALL_COLLISION_H
