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

#ifndef GRIM_WALKSYSTEM_H
#define GRIM_WALKSYSTEM_H

#include "common/scummsys.h"

namespace Grim {

class Puck;
class RenderDroid;
class StdConffile;
class StdMath;
class WalkPlane;
class WalkSystem;

struct WpVertex {
	int refCount;
	float x;
	float y;
	float z;
};

struct WpCollection {
	WalkPlane *pHead;
	char name[36];
};

struct WpCachePlane {
	WalkPlane *pPlane;
	int numVerts;
	int aVertIdx[10];
	int bActive;
};

struct WpCacheVert {
	int index;
	WpVertex v;
};

class WalkPlane {
public:
	WalkPlane(WalkSystem *pSystem, float (*pVerts)[3], int numPoints, const char *collection);
	~WalkPlane();

	void setName(const char *pName);
	void setType(int newType);
	int within(float *pPoint, uint flags);
	int closestPoint(float *pPoint);
	double intersection(float *pA, float *pB, float *pOut);
	void snapTo(float *pPoint);
	void project(float *pPoint);
	void draw(uint16 color);
	void jumpOut(Puck *pPuck);
	float *getConstraintVector(Puck *pPuck);
	int clipPolygon(const float *pPoly, int numPoly, float *pOut);
	double distToEdge(int edge, float *pPoint);
	int removeShortEdge();

	char name[32];
	int number;
	int type;
	int bActive;
	int bEnabled;
	float tolerance;
	int numVerts;
	WpVertex *apVerts[10];
	float normal[3];
	WalkPlane *pPrev;
	WalkPlane *pNext;
	int hitEdge;
	int quadAxis;
	int axisU;
	int axisV;

private:
	WalkSystem *_pSystem;
};

class WalkSystem {
public:
	WalkSystem(RenderDroid *pRd, StdMath *pStdMath, StdConffile *pStdConffile, bool isDemo);
	~WalkSystem();

	void buildCache();
	void destroyCache();
	void restoreCache();
	void killCollection(const char *collection);
	void setCollection(const char *name);
	WalkPlane *planeAt(float *pPoint, int flags);
	WalkPlane *planeNamed(const char *name);
	WalkPlane *planeNumbered(int index);
	WalkPlane *planeClosest(float *pPoint, int flags);
	void allActive();
	WpVertex *addVertex(WalkPlane *pPlane, float *pPos);
	void removeVertex(WpVertex *pVert);
	const char *parseCollection(const char *collection);
	int closestVertWithin(WalkPlane *pA, WalkPlane *pB, float *pTarget, float *pOut);
	WalkPlane *planeContaining(float *pA, float *pB);
	int getPath(float **ppPath);
	int buildPath(float *pFrom, float *pTo, int bTighten);
	int findPath(float *pFrom, float *pTo, WalkPlane *pExclude);
	int findExitPoints(WalkPlane *pPlane, float *pFrom, float *pOut);
	void tightenPath();
	void shrinkBoxes(float amount);

	WalkPlane *getHead() const { return _pWpHead; }
	Puck *getPuckHead() const { return _pPuckHead; }
	const char *getCurCollection() const { return _curCollection; }

private:
	friend class WalkPlane;
	friend class Puck;

	WalkPlane *newPlane(float (*pVerts)[3], int numPoints, const char *collection, const char *sectorName, int id, int bVisible, int type);
	WpVertex *addVertexDemo(float *pPos);

	RenderDroid *_pRd;
	StdMath *_pStdMath;
	StdConffile *_pStdConffile;

	WpVertex _aVertDb[2048];
	int _totVerts;
	int _numDemoVerts;
	WalkPlane *_pWpHead;
	char *_curCollection;
	WpCollection _aCollection[140];
	Puck *_pPuckHead;
	int _forwardCounter;
	bool _bDemo;

	float _aPath[30][3];
	int _numPathPoints;
	float _constraintVector[3];
	char _sectionName[48];

	WpCachePlane *_pCache;
	WpCacheVert *_pVertCache;
	int _numPlanes;
	int _numCacheVerts;
	int _bCacheBuilt;
};

} // End of namespace Grim

#endif
