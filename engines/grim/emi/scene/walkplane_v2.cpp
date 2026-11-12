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

#include "engines/grim/emi/nodes/cnode.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/actor/puck_v2.h"
#include "engines/grim/emi/scene/walkplane_v2.h"
#include "engines/grim/emi/scene/walkplanecollection.h"

namespace Grim {

enum {
	kTypeWalk = 0x1000,
	kTypeFunnel = 0x100
};

static const float kMinLength = 1e-20f;
static const float kEdgeEpsilon = 1e-5f;
static const float kWalkTolerance = 0.005f;
static const float kDefaultTolerance = 10000.0f;
static const float kMaxFloat = 3.4028235e38f;
static const float kJumpOutDistance = -10.0f;
static const float kConstraintDistance = -100.0f;

const float WalkPlane_v2::kRadiansToDegrees = (float)(180.0 / (double)3.14159274f);

WalkPlane_v2::WalkPlane_v2(EmiEngine *engine, const float *vertices, int vertexCount, WalkPlaneCollection *owner) {
	_engine = engine;
	_constraintVector[0] = 0.0f;
	_constraintVector[1] = 0.0f;
	_constraintVector[2] = 0.0f;
	_normal[0] = 0.0f;
	_normal[1] = 0.0f;
	_normal[2] = 0.0f;
	_number = 0;
	_type = 0;
	_active = true;
	_enabled = true;
	_collection = owner;
	_tolerance = kWalkTolerance;
	_hitEdge = 0;
	_quadAxis = 0;
	_d = 0.0f;
	_bfsState = 0;
	_bfsParent = nullptr;

	memset(_verts, 0, kMaxVertices * sizeof(wpVertex *));

	if (vertexCount > kMaxVertices) {
		warning("WalkPlane_v2::WalkPlane_v2(): too many vertices (%d)", vertexCount);
		vertexCount = kMaxVertices;
	}

	if (vertexCount > 0 && !owner) {
		warning("WalkPlane_v2::WalkPlane_v2(): missing collection");
		vertexCount = 0;
	}

	if (vertexCount < 0)
		vertexCount = 0;

	_numVerts = vertexCount;

	for (int i = 0; i < _numVerts; i++)
		_verts[i] = _collection->addVertex(vertices + 3 * i);

	_numSortOrders = 0;
	memset(_sortOrder, 0, kMaxSortOrders * sizeof(int));

	if (_numVerts < 2) {
		warning("WalkPlane_v2::WalkPlane_v2(): less than two vertices");

		return;
	}

	wpVertex *first = _verts[0];
	wpVertex *second = _verts[1];
	wpVertex *last = _verts[_numVerts - 1];

	double lastDeltaX = (double)last->x - first->x;
	float lastDeltaY = (float)((double)last->y - first->y);
	float lastDeltaZ = (float)((double)last->z - first->z);
	float secondDeltaX = (float)((double)second->x - first->x);
	float secondDeltaY = (float)((double)second->y - first->y);
	double secondDeltaZExact = (double)second->z - first->z;
	float secondDeltaZ = (float)secondDeltaZExact;

	float normalX = (float)(secondDeltaZExact * lastDeltaY - (double)secondDeltaY * lastDeltaZ);
	float normalY = (float)((double)lastDeltaZ * secondDeltaX - (double)secondDeltaZ * lastDeltaX);
	float normalZ = (float)((double)secondDeltaY * lastDeltaX - (double)lastDeltaY * secondDeltaX);

	_normal[0] = normalX;
	_normal[1] = normalY;
	_normal[2] = normalZ;

	double length = sqrt(((double)_normal[0] * _normal[0] + (double)_normal[1] * _normal[1]) + (double)_normal[2] * _normal[2]);

	if (length >= kMinLength) {
		double inverse = 1.0 / length;

		_normal[0] = (float)(inverse * _normal[0]);
		_normal[1] = (float)(inverse * _normal[1]);
		_normal[2] = (float)(inverse * _normal[2]);
	}

	_d = (float)-(((double)_normal[2] * first->z + (double)_normal[1] * first->y) + (double)_normal[0] * first->x);
}

WalkPlane_v2::~WalkPlane_v2() {
	if (_collection) {
		for (int i = 0; i < _numVerts; i++)
			_collection->removeVertex(_verts[i]);
	} else if (_numVerts > 0) {
		warning("WalkPlane_v2::~WalkPlane_v2(): missing collection");
	}

	_neighbors.clear();
}

void *WalkPlane_v2::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void WalkPlane_v2::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int WalkPlane_v2::getSize() {
	return (int)(_name.size() + sizeof(WalkPlane_v2) + _neighbors.size() * sizeof(int));
}

bool WalkPlane_v2::addNeighbor(int planeNumber) {
	for (uint i = 0; i < _neighbors.size(); i++) {
		if (_neighbors[i] == planeNumber)
			return false;
	}

	_neighbors.push_back(planeNumber);

	return true;
}

bool WalkPlane_v2::touches(WalkPlane_v2 *other, bool both) {
	for (int i = 0; i < _numVerts; i++) {
		if (other->within(&_verts[i]->x, false))
			return true;
	}

	if (!both)
		return false;

	for (int i = 0; i < other->_numVerts; i++) {
		if (within(&other->_verts[i]->x, false))
			return true;
	}

	return false;
}

float *WalkPlane_v2::getGatePoint(float *out, WalkPlane_v2 *other, const float *from, const float *target) {
	float result[3] = { 0.0f, 0.0f, 0.0f };

	closestSharedVertex(this, other, target[0], target[1], target[2], result);

	out[0] = result[0];
	out[1] = result[1];
	out[2] = result[2];

	return out;
}

void WalkPlane_v2::serialize(SaveRestoreAccess *access) {
	access(&_type, sizeof(_type));
	access(&_active, sizeof(_active));
	access(&_enabled, sizeof(_enabled));
	access(&_tolerance, sizeof(_tolerance));
	access(&_numSortOrders, sizeof(_numSortOrders));

	if (_numSortOrders < 0 || _numSortOrders > kMaxSortOrders) {
		warning("WalkPlane_v2::serialize(): invalid number of sort orders (%d)", _numSortOrders);
		_numSortOrders = 0;

		return;
	}

	for (int i = 0; i < _numSortOrders; i++)
		access(&_sortOrder[i], sizeof(int));
}

void WalkPlane_v2::setType(int newType) {
	_type = newType;
	_tolerance = kDefaultTolerance;

	if ((newType & kTypeWalk) != 0)
		_tolerance = kWalkTolerance;

	if ((newType & kTypeFunnel) == 0)
		return;

	if (_numVerts != 4) {
		LConsoleServices *console = _engine->getConsoleServices();

		if (console)
			console->logError("Funnel boxes must have 4 sides!\n");
		else
			warning("Funnel boxes must have 4 sides!");

		_type = kTypeWalk;

		return;
	}

	wpVertex *vertex0 = _verts[0];
	wpVertex *vertex1 = _verts[1];
	wpVertex *vertex2 = _verts[2];
	wpVertex *vertex3 = _verts[3];

	float edge0X = (float)((double)vertex1->x - vertex0->x);
	float edge0Y = (float)((double)vertex1->y - vertex0->y);
	float edge0Z = (float)((double)vertex1->z - vertex0->z);
	float dir0X = edge0X;
	float dir0Y = edge0Y;
	float dir0Z = edge0Z;

	float edge1X = (float)((double)vertex3->x - vertex2->x);
	float edge1Y = (float)((double)vertex3->y - vertex2->y);
	float edge1Z = (float)((double)vertex3->z - vertex2->z);
	float dir1X = edge1X;
	float dir1Y = edge1Y;
	float dir1Z = edge1Z;

	double length0 = sqrt(((double)edge0Z * edge0Z + (double)edge0Y * edge0Y) + (double)edge0X * edge0X);

	if (length0 >= kMinLength) {
		double inverse = 1.0 / length0;

		dir0X = (float)(inverse * edge0X);
		dir0Y = (float)(edge0Y * inverse);
		dir0Z = (float)(edge0Z * inverse);
	} else {
		length0 = 0.0;
	}

	double length1 = sqrt(((double)edge1Z * edge1Z + (double)edge1Y * edge1Y) + (double)edge1X * edge1X);

	if (length1 >= kMinLength) {
		double inverse = 1.0 / length1;

		dir1X = (float)(inverse * edge1X);
		dir1Y = (float)(edge1Y * inverse);
		dir1Z = (float)(edge1Z * inverse);
	} else {
		length1 = 0.0;
	}

	double dotFirst = ((double)dir0Z * dir1Z + (double)dir0Y * dir1Y) + (double)dir1X * dir0X;

	if (dotFirst < 0.0)
		dotFirst = -dotFirst;

	float edge2X = (float)((double)vertex2->x - vertex1->x);
	float edge2Y = (float)((double)vertex2->y - vertex1->y);
	float edge2Z = (float)((double)vertex2->z - vertex1->z);
	float dir2X = edge2X;
	float dir2Y = edge2Y;
	float dir2Z = edge2Z;

	float edge3X = (float)((double)vertex0->x - vertex3->x);
	float edge3Y = (float)((double)vertex0->y - vertex3->y);
	float edge3Z = (float)((double)vertex0->z - vertex3->z);
	double dir3X = edge3X;
	double dir3Y = edge3Y;
	double dir3Z = edge3Z;

	double length2 = sqrt(((double)edge2Z * edge2Z + (double)edge2Y * edge2Y) + (double)edge2X * edge2X);
	float length2Stored = (float)length2;

	if (length2 < kMinLength) {
		length2Stored = 0.0f;
	} else {
		double inverse = 1.0 / length2Stored;

		dir2X = (float)(inverse * edge2X);
		dir2Y = (float)(edge2Y * inverse);
		dir2Z = (float)(edge2Z * inverse);
	}

	double length3 = sqrt(((double)edge3Z * edge3Z + (double)edge3Y * edge3Y) + (double)edge3X * edge3X);
	float length3Stored = 0.0f;

	if (length3 >= kMinLength) {
		float length3Float = (float)length3;
		double inverse = 1.0 / length3Float;
		float inverseFloat = (float)inverse;

		dir3X = inverse * edge3X;
		dir3Y = edge3Y * (double)inverseFloat;
		dir3Z = edge3Z * (double)inverseFloat;
		length3Stored = length3Float;
	}

	double dotSecond = ((double)dir2Z * dir3Z + (double)dir2Y * dir3Y) + dir3X * dir2X;

	if (dotSecond < 0.0)
		dotSecond = -dotSecond;

	if (dotFirst > dotSecond) {
		if (length0 < length1)
			_quadAxis = 0;
		else
			_quadAxis = 2;
	} else {
		if (length2Stored < length3Stored)
			_quadAxis = 1;
		else
			_quadAxis = 3;
	}
}

bool WalkPlane_v2::within(const float *point, bool checkActive) {
	if (checkActive && (!_enabled || !_active))
		return false;

	if (_normal[1] == 0.0f) {
		warning("WalkPlane_v2::within(): the plane is vertical");

		return false;
	}

	double height = (double)point[1] - ((-(double)_d - (double)_normal[0] * point[0] - (double)_normal[2] * point[2]) / _normal[1]);

	if (height < 0.0)
		height = -height;

	if (height > _tolerance)
		return false;

	if (_numVerts <= 0)
		return true;

	const float *previous = &_verts[_numVerts - 1]->x;

	for (int i = 0; i < _numVerts; i++) {
		const float *current = &_verts[i]->x;
		double deltaX = (double)previous[0] - current[0];
		double deltaZ = (double)previous[2] - current[2];
		double offsetZ = (double)current[2] - point[2];
		double offsetX = (double)current[0] - point[0];
		double denominator = deltaZ * deltaZ + deltaX * deltaX;

		if (denominator == 0.0) {
			// The original could divide by zero here, let's avoid that...
			previous = current;

			continue;
		}

		double side = (deltaX * offsetZ - offsetX * deltaZ) / denominator;
		double absoluteSide = side;

		if (absoluteSide < 0.0)
			absoluteSide = -absoluteSide;

		if (absoluteSide > kEdgeEpsilon && side < 0.0)
			return false;

		previous = current;
	}

	return true;
}

void WalkPlane_v2::closestPoint(float *point) {
	project(point);

	if (_numVerts <= 0)
		return;

	const float *previous = &_verts[_numVerts - 1]->x;

	for (int i = 0; i < _numVerts; i++) {
		const float *current = &_verts[i]->x;
		float edgeZ = (float)-((double)current[2] - previous[2]);
		float edgeX = (float)((double)current[0] - previous[0]);
		double numerator = ((double)point[2] - previous[2]) * edgeX + ((double)point[0] - previous[0]) * edgeZ;
		float numeratorStored = (float)numerator;

		if (numerator < 0.0) {
			double denominator = -((double)edgeZ * edgeZ) - (double)edgeX * edgeX;
			float denominatorStored = (float)denominator;
			float absoluteDenominator = denominatorStored;

			if (absoluteDenominator < 0.0f)
				absoluteDenominator = -absoluteDenominator;

			if (absoluteDenominator > kEdgeEpsilon && denominatorStored != 0.0f) {
				double ratio = (double)numeratorStored / denominatorStored;
				float newX = (float)(ratio * edgeZ + point[0]);
				double newZ = ratio * edgeX + point[2];

				point[0] = newX;
				point[2] = (float)newZ;

				double deltaX = (double)current[0] - previous[0];
				float deltaXStored = (float)deltaX;
				float deltaZStored = (float)((double)current[2] - previous[2]);
				double absoluteDeltaX = deltaX;
				float absoluteDeltaZ = deltaZStored;
				double position;

				if (absoluteDeltaX < 0.0)
					absoluteDeltaX = -absoluteDeltaX;

				if (absoluteDeltaZ < 0.0f)
					absoluteDeltaZ = -absoluteDeltaZ;

				if (absoluteDeltaX <= absoluteDeltaZ)
					position = (newZ - previous[2]) / deltaZStored;
				else
					position = ((double)newX - previous[0]) / deltaXStored;

				if (i == _numVerts - 1 && position > 1.0) {
					point[0] = current[0];
					point[1] = current[1];
					point[2] = current[2];
				} else if (position >= 0.0) {
					project(point);
				} else {
					point[0] = previous[0];
					point[1] = previous[1];
					point[2] = previous[2];
				}
			}
		}

		previous = current;
	}
}

bool WalkPlane_v2::segmentIntersection(const float *startA, const float *endA, const float *startB, const float *endB, float *out) {
	double deltaBY = (double)endB[1] - startB[1];
	float deltaAX = (float)((double)endA[0] - startA[0]);
	float deltaAY = (float)((double)endA[1] - startA[1]);
	double deltaBX = (double)endB[0] - startB[0];
	float denominator = (float)(deltaAX * deltaBY - deltaBX * deltaAY);
	float offsetY = (float)((double)startA[1] - startB[1]);
	float offsetX = (float)((double)startA[0] - startB[0]);

	if (denominator == 0.0f) {
		// The original could divide by zero here, let's avoid that...
		return false;
	}

	float firstRatio = (float)((offsetY * deltaBX - offsetX * deltaBY) / denominator);

	if (firstRatio < 0.0f || firstRatio > 1.0f)
		return false;

	double secondRatio = ((double)offsetY * deltaAX - (double)offsetX * deltaAY) / denominator;

	if (secondRatio < 0.0 || secondRatio > 1.0)
		return false;

	*out = firstRatio;

	return true;
}

float WalkPlane_v2::intersection(const float *pointA, const float *pointB, float *out) {
	float segmentA[2];
	float segmentB[2];
	float edgeStart[2];
	float edgeEnd[2];
	float bestRatio = -1.0f;
	float angle = 0.0f;

	_hitEdge = -1;
	segmentA[0] = pointA[0];
	segmentA[1] = pointA[2];
	segmentB[0] = pointB[0];
	segmentB[1] = pointB[2];
	edgeStart[0] = 0.0f;
	edgeStart[1] = 0.0f;
	edgeEnd[0] = 0.0f;
	edgeEnd[1] = 0.0f;

	if (_numVerts > 0) {
		const float *previous = &_verts[_numVerts - 1]->x;

		for (int i = 0; i < _numVerts; i++) {
			const float *current = &_verts[i]->x;
			float ratio;

			edgeStart[0] = current[0];
			edgeStart[1] = current[2];
			edgeEnd[0] = previous[0];
			edgeEnd[1] = previous[2];

			if (segmentIntersection(segmentA, segmentB, edgeStart, edgeEnd, &ratio)) {
				if (ratio > bestRatio) {
					bestRatio = ratio;
					_hitEdge = i;

					float directionX = (float)((double)segmentB[0] - segmentA[0]);
					float directionZ = (float)((double)segmentB[1] - segmentA[1]);
					float normalizedDirectionX = directionX;
					double normalizedDirectionZ = directionZ;
					double length = sqrt((double)directionX * directionX + (double)directionZ * directionZ);

					if (length >= kMinLength) {
						double inverse = 1.0 / length;

						normalizedDirectionX = (float)(directionX * inverse);
						normalizedDirectionZ = inverse * directionZ;
					}

					float edgeX = (float)((double)edgeEnd[0] - edgeStart[0]);
					float edgeZ = (float)((double)edgeEnd[1] - edgeStart[1]);
					float normalizedEdgeX = edgeX;
					double normalizedEdgeZ = edgeZ;

					length = sqrt((double)edgeZ * edgeZ + (double)edgeX * edgeX);

					if (length >= kMinLength) {
						double inverse = 1.0 / length;

						normalizedEdgeX = (float)(edgeX * inverse);
						normalizedEdgeZ = inverse * edgeZ;
					}

					float dot = (float)(normalizedDirectionZ * normalizedEdgeZ + (double)normalizedEdgeX * normalizedDirectionX);

					if (dot > -1.0f) {
						if (dot >= 1.0f)
							dot = 1.0f;
					} else {
						dot = -1.0f;
					}

					angle = (float)(acos((double)dot) * kRadiansToDegrees);
				} else if (ratio == 0.0f) {
					angle = 0.0f;
				}
			}

			previous = current;
		}
	}

	if (bestRatio <= 0.0f) {
		out[0] = segmentA[0];
		out[1] = 0.0f;
		out[2] = segmentA[1];
		project(out);

		return angle;
	}

	out[1] = 0.0f;
	out[0] = (float)(((double)segmentB[0] - segmentA[0]) * bestRatio + segmentA[0]);
	out[2] = (float)(((double)segmentB[1] - segmentA[1]) * bestRatio + segmentA[1]);
	project(out);

	if (angle <= 90.0f)
		return (float)((double)angle + 0.1f);

	return (float)-(180.2f - (double)angle);
}

void WalkPlane_v2::project(float *point) {
	if (_normal[1] == 0.0f) {
		warning("WalkPlane_v2::project(): the plane is vertical");

		return;
	}

	point[1] = (float)((-(double)_d - (double)_normal[0] * point[0] - (double)_normal[2] * point[2]) / _normal[1]);
}

float *WalkPlane_v2::getConstraintVector(Puck_v2 *puck) {
	float position[3] = { 0.0f, 0.0f, 0.0f };
	float behind[3] = { 0.0f, 0.0f, 0.0f };

	if (!puck) {
		warning("WalkPlane_v2::getConstraintVector(): missing puck");

		return nullptr;
	}

	if ((puck->_nodeFlags & CNode::kPositionValid) == 0)
		puck->computeLocalPositionFromWorld();

	position[0] = puck->_pos._x;
	position[1] = puck->_pos._y;
	position[2] = puck->_pos._z;

	if ((puck->_nodeFlags & CNode::kWorldValid) == 0)
		puck->updateWorldTransform();

	float forwardY = puck->_world._rot._m[7];
	float forwardZ = puck->_world._rot._m[8];
	float offsetY = (float)((double)forwardY * kConstraintDistance);
	float offsetZ = (float)((double)forwardZ * kConstraintDistance);

	behind[0] = (float)((double)puck->_world._rot._m[6] * kConstraintDistance + position[0]);
	behind[1] = (float)((double)position[1] + offsetY);
	behind[2] = (float)((double)position[2] + offsetZ);

	intersection(behind, position, _constraintVector);

	int edge = _hitEdge;

	if (edge == -1)
		return nullptr;

	if (_numVerts <= 0 || edge >= _numVerts) {
		warning("WalkPlane_v2::getConstraintVector(): invalid edge %d", edge);

		return nullptr;
	}

	const float *current = &_verts[edge]->x;
	int previousIndex = edge - 1;

	if (previousIndex <= -1)
		previousIndex = _numVerts - 1;

	const float *previous = &_verts[previousIndex]->x;
	float deltaX = (float)((double)previous[0] - current[0]);
	float absoluteDeltaX = deltaX;
	double ratio;

	if (absoluteDeltaX < 0.0f)
		absoluteDeltaX = -absoluteDeltaX;

	if (absoluteDeltaX <= kEdgeEpsilon || deltaX == 0.0f) {
		double deltaZ = (double)previous[2] - current[2];

		if (deltaZ == 0.0) {
			// The original could divide by zero here, let's avoid that...
			warning("WalkPlane_v2::getConstraintVector(): degenerate edge");
			ratio = 0.0;
		} else {
			ratio = ((double)_constraintVector[2] - current[2]) / deltaZ;
		}
	} else {
		ratio = ((double)_constraintVector[0] - current[0]) / deltaX;
	}

	const float *lineStart = &_verts[(edge + 1) % _numVerts]->x;
	const float *lineEnd = &_verts[(_numVerts + edge - 2) % _numVerts]->x;
	double lineDeltaX = (double)lineEnd[0] - lineStart[0];
	float lineDeltaZ = (float)((double)lineEnd[2] - lineStart[2]);
	float pointX = (float)(lineDeltaX * ratio + lineStart[0]);
	double pointZ = ratio * lineDeltaZ + lineStart[2];

	puck->_lineA[1] = 0.0f;
	puck->_lineA[0] = pointX;
	puck->_lineA[2] = (float)pointZ;

	_constraintVector[1] = 0.0f;
	puck->_lineB[0] = _constraintVector[0];
	puck->_lineB[1] = _constraintVector[1];
	puck->_lineB[2] = _constraintVector[2];

	float resultX = (float)((double)pointX - _constraintVector[0]);
	float resultZ = (float)(pointZ - _constraintVector[2]);

	_constraintVector[1] = -_constraintVector[1];
	_constraintVector[0] = resultX;
	_constraintVector[2] = resultZ;

	return _constraintVector;
}

void WalkPlane_v2::jumpOut(Puck_v2 *puck) {
	float position[3] = { 0.0f, 0.0f, 0.0f };
	float ahead[3] = { 0.0f, 0.0f, 0.0f };
	float hit[3] = { 0.0f, 0.0f, 0.0f };

	if (_numVerts != 4)
		return;

	if (!puck) {
		warning("WalkPlane_v2::jumpOut(): missing puck");

		return;
	}

	if ((puck->_nodeFlags & CNode::kPositionValid) == 0)
		puck->computeLocalPositionFromWorld();

	position[0] = puck->_pos._x;
	position[1] = puck->_pos._y;
	position[2] = puck->_pos._z;

	if ((puck->_nodeFlags & CNode::kWorldValid) == 0)
		puck->updateWorldTransform();

	float forwardY = puck->_world._rot._m[7];
	float forwardZ = puck->_world._rot._m[8];
	float offsetY = (float)((double)forwardY * kJumpOutDistance);
	float offsetZ = (float)((double)forwardZ * kJumpOutDistance);

	ahead[0] = (float)((double)puck->_world._rot._m[6] * kJumpOutDistance + position[0]);
	ahead[1] = (float)((double)offsetY + position[1]);
	ahead[2] = (float)((double)offsetZ + position[2]);

	intersection(position, ahead, hit);

	int edge = _hitEdge;

	if (edge == -1)
		return;

	const float *current = &_verts[edge]->x;
	int previousIndex = edge - 1;

	if (previousIndex <= -1)
		previousIndex = _numVerts - 1;

	const float *previous = &_verts[previousIndex]->x;
	float deltaX = (float)((double)previous[0] - current[0]);
	float absoluteDeltaX = deltaX;
	double ratio;

	if (absoluteDeltaX < 0.0f)
		absoluteDeltaX = -absoluteDeltaX;

	if (absoluteDeltaX <= kEdgeEpsilon || deltaX == 0.0f) {
		double deltaZ = (double)previous[2] - current[2];

		if (deltaZ == 0.0) {
			// The original could divide by zero here, let's avoid that...
			warning("WalkPlane_v2::jumpOut(): degenerate edge");
			ratio = 0.0;
		} else {
			ratio = ((double)hit[2] - current[2]) / deltaZ;
		}
	} else {
		ratio = ((double)hit[0] - current[0]) / deltaX;
	}

	double remaining = 1.0 - ratio;

	_hitEdge = (edge + 2) % _numVerts;

	const float *newCurrent = &_verts[_hitEdge]->x;
	int newPreviousIndex = _hitEdge - 1;

	if (newPreviousIndex <= -1)
		newPreviousIndex = _numVerts - 1;

	const float *newPrevious = &_verts[newPreviousIndex]->x;
	CVector newPosition;

	newPosition._x = (float)(((double)newPrevious[0] - newCurrent[0]) * remaining + newCurrent[0]);
	newPosition._y = (float)(((double)newPrevious[1] - newCurrent[1]) * remaining + newCurrent[1]);
	newPosition._z = (float)(((double)newPrevious[2] - newCurrent[2]) * remaining + newCurrent[2]);

	puck->setLocalPosition(newPosition);
}

bool WalkPlane_v2::closestSharedVertex(WalkPlane_v2 *planeA, WalkPlane_v2 *planeB, float x, float y, float z, float *out) {
	float best = kMaxFloat;

	for (int i = 0; i < planeA->_numVerts; i++) {
		wpVertex *vertex = planeA->_verts[i];

		if (planeB->within(&vertex->x, false)) {
			double deltaZ = (double)vertex->z - z;
			double deltaY = (double)vertex->y - y;
			double deltaX = (double)vertex->x - x;
			double distance = (deltaX * deltaX + deltaY * deltaY) + deltaZ * deltaZ;

			if (distance < best) {
				best = (float)distance;
				out[0] = vertex->x;
				out[1] = vertex->y;
				out[2] = vertex->z;
			}
		}
	}

	for (int i = 0; i < planeB->_numVerts; i++) {
		wpVertex *vertex = planeB->_verts[i];

		if (planeA->within(&vertex->x, false)) {
			double deltaZ = (double)vertex->z - z;
			double deltaY = (double)vertex->y - y;
			double deltaX = (double)vertex->x - x;
			double distance = (deltaX * deltaX + deltaY * deltaY) + deltaZ * deltaZ;

			if (distance < best) {
				best = (float)distance;
				out[0] = vertex->x;
				out[1] = vertex->y;
				out[2] = vertex->z;
			}
		}
	}

	return best != kMaxFloat;
}

} // End of namespace Grim
