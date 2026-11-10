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

#ifndef GRIM_EMI_WALKPLANE_V2_H
#define GRIM_EMI_WALKPLANE_V2_H

#include "common/array.h"

#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/wpvertex.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class Puck_v2;
class WalkPlaneCollection;

class WalkPlane_v2 {
public:
	enum {
		kMaxVertices = 10,
		kMaxSortOrders = 32
	};

	WalkPlane_v2(const float *vertices, int vertexCount, WalkPlaneCollection *owner);
	~WalkPlane_v2();

	void *operator new(size_t size);
	void operator delete(void *block);

	int getSize();
	bool addNeighbor(int planeNumber);
	bool touches(WalkPlane_v2 *other, bool both);
	float *getGatePoint(float *out, WalkPlane_v2 *other, const float *from, const float *target);
	void serialize(tSaveRestoreAccess *access);
	void setType(int newType);
	bool within(const float *point, bool checkActive);
	void closestPoint(float *point);
	float intersection(const float *pointA, const float *pointB, float *out);
	void project(float *point);
	float *getConstraintVector(Puck_v2 *puck);
	void jumpOut(Puck_v2 *puck);

	static bool segmentIntersection(const float *startA, const float *endA, const float *startB, const float *endB, float *out);
	static bool closestSharedVertex(WalkPlane_v2 *planeA, WalkPlane_v2 *planeB, float x, float y, float z, float *out);

	static const float kRadiansToDegrees;

	LString name;
	int number;
	int type;
	bool active;
	bool enabled;
	float tolerance;
	int numSortOrders;
	int sortOrder[kMaxSortOrders];
	int numVerts;
	wpVertex *verts[kMaxVertices];
	int hitEdge;
	int quadAxis;
	WalkPlaneCollection *collection;
	float normal[3];
	float d;
	Common::Array<int> neighbors;
	int bfsState;
	WalkPlane_v2 *bfsParent;

private:
	static float _constraintVector[3];
};

} // End of namespace Grim

#endif
