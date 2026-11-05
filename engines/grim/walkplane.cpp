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

#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/puck.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/walkplane.h"

namespace Grim {

WalkPlane::WalkPlane(WalkSystem *pSystem, float (*pVerts)[3], int numPoints, const char *collection) {
	_pSystem = pSystem;

	if (!collection)
		collection = "";

	bActive = 1;
	bEnabled = 1;
	pPrev = nullptr;
	pNext = nullptr;
	number = 0;
	type = 0;
	hitEdge = -1;
	quadAxis = 0;
	numVerts = numPoints;
	name[0] = 0;
	tolerance = 0.005f;

	if (numVerts > 10) {
		warning("WalkPlane::WalkPlane(): %d vertices requested, only 10 are supported", numVerts);
		numVerts = 10;
	}

	for (int i = 0; i < 10; i++)
		apVerts[i] = nullptr;

	normal[0] = 0.0f;
	normal[1] = 0.0f;
	normal[2] = 0.0f;

	for (int i = 0; i < numVerts; i++) {
		apVerts[i] = _pSystem->addVertex(this, pVerts[i]);

		if (!apVerts[i]) {
			for (int j = 0; j < i; j++)
				_pSystem->removeVertex(apVerts[j]);

			numVerts = 0;
			axisU = 2;
			axisV = 1;
			return;
		}
	}

	if (numVerts > 0) {
		WpVertex *pPrevVert = apVerts[numVerts - 1];

		for (int i = 0; i < numVerts; i++) {
			WpVertex *pVert = apVerts[i];

			normal[0] = (float)(((double)pPrevVert->y - pVert->y) * ((double)pPrevVert->z + pVert->z) + normal[0]);
			normal[1] = (float)(((double)pPrevVert->z - pVert->z) * ((double)pVert->x + pPrevVert->x) + normal[1]);
			normal[2] = (float)(((double)pPrevVert->x - pVert->x) * ((double)pPrevVert->y + pVert->y) + normal[2]);

			pPrevVert = pVert;
		}
	}

	_pSystem->_pRd->normalize3Acc(normal);

	float largest = normal[1];
	if (normal[1] < normal[0])
		largest = normal[0];

	if (largest <= normal[2])
		largest = normal[2];

	if (largest == normal[0]) {
		axisU = 2;
		axisV = 1;
	} else if (largest == normal[1]) {
		axisU = 0;
		axisV = 2;
	} else {
		axisU = 1;
		axisV = 0;
	}

	int index;
	for (index = 0; index < 140; index++) {
		WpCollection *pCollection = &_pSystem->_aCollection[index];

		if (!strcmp(pCollection->name, collection) || pCollection->name[0] == 0)
			break;
	}

	if (index == 140) {
		warning("WalkPlane::WalkPlane(): too many collections, plane not added to %s", collection);
		return;
	}

	WpCollection *pCollection = &_pSystem->_aCollection[index];

	if (pCollection->name[0] == 0) {
		if (strlen(collection) >= sizeof(pCollection->name))
			warning("WalkPlane::WalkPlane(): collection name %s is too long", collection);

		Common::strlcpy(pCollection->name, collection, sizeof(pCollection->name));
	}

	pNext = pCollection->pHead;

	if (pCollection->pHead)
		pCollection->pHead->pPrev = this;

	pCollection->pHead = this;
	_pSystem->_pWpHead = this;
}

WalkPlane::~WalkPlane() {
	if (this == _pSystem->_pWpHead)
		_pSystem->_pWpHead = pNext;

	for (int i = 0; i < 140; i++) {
		if (_pSystem->_aCollection[i].pHead == this) {
			_pSystem->_aCollection[i].pHead = pNext;
			break;
		}
	}

	for (int i = 0; i < numVerts; i++)
		_pSystem->removeVertex(apVerts[i]);

	if (pPrev)
		pPrev->pNext = pNext;

	if (pNext)
		pNext->pPrev = pPrev;
}

void WalkPlane::setName(const char *pName) {
	if (pName) {
		if (strlen(pName) >= sizeof(name))
			warning("WalkPlane::setName(): name %s is too long", pName);

		Common::strlcpy(name, pName, sizeof(name));
	}
}

void WalkPlane::setType(int newType) {
	type = newType;
	tolerance = 9999.0f;

	if ((newType & 0x1000) != 0)
		tolerance = 0.005f;

	if ((newType & 0x100) != 0) {
		if (numVerts == 4) {
			RdVector3 edgeA;
			RdVector3 edgeB;

			edgeA[0] = (float)((double)apVerts[1]->x - apVerts[0]->x);
			edgeA[1] = (float)((double)apVerts[1]->y - apVerts[0]->y);
			edgeA[2] = (float)((double)apVerts[1]->z - apVerts[0]->z);
			edgeB[0] = (float)((double)apVerts[3]->x - apVerts[2]->x);
			edgeB[1] = (float)((double)apVerts[3]->y - apVerts[2]->y);
			edgeB[2] = (float)((double)apVerts[3]->z - apVerts[2]->z);

			float lengthA = _pSystem->_pRd->normalize3Acc(edgeA);
			float lengthB = _pSystem->_pRd->normalize3Acc(edgeB);
			double dot = (double)edgeB[0] * edgeA[0] + (double)edgeB[1] * edgeA[1] + (double)edgeB[2] * edgeA[2];
			float dotAbs = (float)fabs(dot);

			edgeA[0] = (float)((double)apVerts[2]->x - apVerts[1]->x);
			edgeA[1] = (float)((double)apVerts[2]->y - apVerts[1]->y);
			edgeA[2] = (float)((double)apVerts[2]->z - apVerts[1]->z);
			edgeB[0] = (float)((double)apVerts[0]->x - apVerts[3]->x);
			edgeB[1] = (float)((double)apVerts[0]->y - apVerts[3]->y);
			edgeB[2] = (float)((double)apVerts[0]->z - apVerts[3]->z);

			float lengthC = _pSystem->_pRd->normalize3Acc(edgeA);
			float lengthD = _pSystem->_pRd->normalize3Acc(edgeB);
			dot = (double)edgeB[0] * edgeA[0] + (double)edgeB[1] * edgeA[1] + (double)edgeB[2] * edgeA[2];

			if (dotAbs <= fabs(dot)) {
				if (lengthC >= lengthD)
					quadAxis = 3;
				else
					quadAxis = 1;
			} else if (lengthA >= lengthB) {
				quadAxis = 2;
			} else {
				quadAxis = 0;
			}
		} else {
			type = 4096;
		}
	}
}

int WalkPlane::within(float *pPoint, uint flags) {
	if (flags) {
		if (!bEnabled)
			return 0;

		if (!bActive)
			return 0;
	}

	double distance = ((double)pPoint[0] - apVerts[1]->x) * normal[0] + ((double)pPoint[2] - apVerts[1]->z) * normal[2] + ((double)pPoint[1] - apVerts[1]->y) * normal[1];

	distance = fabs(distance);
	if (distance <= 0.00001f)
		distance = 0.0;

	if (distance > tolerance)
		return 0;

	if (numVerts > 0) {
		float *pPrevVert = &apVerts[numVerts - 1]->x;

		for (int i = 0; i < numVerts; i++) {
			float *pVert = &apVerts[i]->x;

			double side = -((double)pVert[axisU] - pPrevVert[axisU]) * ((double)pPoint[axisV] - pPrevVert[axisV]) + ((double)pVert[axisV] - pPrevVert[axisV]) * ((double)pPoint[axisU] - pPrevVert[axisU]) + 0.00001f;

			if (fabs(side) <= 0.00001f)
				side = 0.0;

			if (side < 0.0)
				return 0;

			pPrevVert = pVert;
		}
	}

	return 1;
}

int WalkPlane::closestPoint(float *pPoint) {
	project(pPoint);

	if (numVerts > 0) {
		float *pPrevVert = &apVerts[numVerts - 1]->x;

		for (int i = 0; i < numVerts; i++) {
			float *pVert = &apVerts[i]->x;

			float edgeY = (float)(-((double)pVert[1] - pPrevVert[1]));
			float edgeX = (float)((double)pVert[0] - pPrevVert[0]);
			double side = ((double)pPoint[0] - pPrevVert[0]) * edgeY + ((double)pPoint[1] - pPrevVert[1]) * edgeX;
			float sideStored = (float)side;

			if (side < 0.0) {
				double denominator = -((double)edgeY * edgeY) - (double)edgeX * edgeX;

				if (fabs(denominator) > 0.00001f) {
					double t = (double)sideStored / denominator;
					double newX = t * edgeY + pPoint[0];
					float newY = (float)(t * edgeX + pPoint[1]);

					pPoint[0] = (float)newX;
					pPoint[1] = newY;

					double deltaX = (double)pVert[0] - pPrevVert[0];
					float deltaY = (float)((double)pVert[1] - pPrevVert[1]);
					double param;

					if (fabs(deltaX) <= fabs((double)deltaY))
						param = ((double)newY - pPrevVert[1]) / deltaY;
					else
						param = (newX - pPrevVert[0]) / deltaX;

					if (i == numVerts - 1 && param > 1.0) {
						pPoint[0] = pVert[0];
						pPoint[1] = pVert[1];
						pPoint[2] = pVert[2];
					} else if (param < 0.0) {
						pPoint[0] = pPrevVert[0];
						pPoint[1] = pPrevVert[1];
						pPoint[2] = pPrevVert[2];
					} else {
						project(pPoint);
					}
				}
			}

			pPrevVert = pVert;
		}
	}

	return numVerts;
}

double WalkPlane::intersection(float *pA, float *pB, float *pOut) {
	float *pHitVert = nullptr;
	RdVector3 hitVec = { 0.0f, 0.0f, 0.0f };

	hitEdge = -1;

	if (within(pB, 1))
		return 0.0;

	pOut[0] = pB[0];
	pOut[1] = pB[1];
	pOut[2] = pB[2];

	if (numVerts > 0) {
		float *pPrevVert = &apVerts[numVerts - 1]->x;

		for (int i = 0; i < numVerts; i++) {
			float *pVert = &apVerts[i]->x;
			float edgeNormal[2];

			edgeNormal[0] = (float)(-((double)pVert[axisU] - pPrevVert[axisU]));
			edgeNormal[1] = (float)((double)pVert[axisV] - pPrevVert[axisV]);

			float side = (float)(((double)pOut[axisU] - pPrevVert[axisU]) * edgeNormal[1] + ((double)pOut[axisV] - pPrevVert[axisV]) * edgeNormal[0]);

			if (side < -0.00001f) {
				double deltaV = (double)pOut[axisV] - pA[axisV];
				double deltaU = (double)pOut[axisU] - pA[axisU];
				float deltaUStored = (float)deltaU;
				double denominator = -(deltaV * edgeNormal[0]) - deltaU * edgeNormal[1];

				if (fabs(denominator) > 0.00001f) {
					double t = (double)side / denominator;

					pHitVert = pPrevVert;
					pOut[axisV] = (float)(deltaV * t + pOut[axisV]);
					pOut[axisU] = (float)(deltaUStored * t + pOut[axisU]);

					hitVec[0] = (float)((double)pPrevVert[0] - pVert[0]);
					hitVec[1] = (float)((double)pPrevVert[1] - pVert[1]);
					hitVec[2] = (float)((double)pPrevVert[2] - pVert[2]);
					hitEdge = i;
				}
			}

			pPrevVert = pVert;
		}
	}

	project(pOut);

	if (!pHitVert) {
		snapTo(pOut);
		return 0.0;
	}

	RdVector3 toA;
	toA[0] = (float)((double)pA[0] - pB[0]);
	toA[1] = (float)((double)pA[1] - pB[1]);
	toA[2] = (float)((double)pA[2] - pB[2]);

	_pSystem->_pRd->normalize3Acc(hitVec);
	_pSystem->_pRd->normalize3Acc(toA);

	float value = _pSystem->_pRd->dot3(hitVec, toA);
	double angle = _pSystem->_pStdMath->arcSin3(value);

	if (angle <= 0.0)
		return -90.2f - angle;

	return 90.2f - angle;
}

void WalkPlane::snapTo(float *pPoint) {
	double distance = ((double)pPoint[0] - apVerts[0]->x) * normal[0] + ((double)pPoint[2] - apVerts[0]->z) * normal[2] + ((double)pPoint[1] - apVerts[0]->y) * normal[1];

	pPoint[0] = (float)(pPoint[0] - distance * normal[0]);
	pPoint[1] = (float)(pPoint[1] - distance * normal[1]);
	pPoint[2] = (float)(pPoint[2] - distance * normal[2]);
}

void WalkPlane::project(float *pPoint) {
	int axisW;

	if (axisU == 1 && axisV == 0) {
		axisW = 2;
	} else if (axisU == 0 && axisV == 2) {
		axisW = 1;
	} else if (axisU == 2 && axisV == 1) {
		axisW = 0;
	} else {
		warning("WalkPlane::project(): invalid projection axes %d, %d", axisU, axisV);
		return;
	}

	double planeDistance = (double)normal[0] * apVerts[0]->x + (double)normal[1] * apVerts[0]->y + (double)normal[2] * apVerts[0]->z;

	pPoint[axisW] = (float)((((double)normal[axisV] * pPoint[axisV] + (double)normal[axisU] * pPoint[axisU]) - planeDistance) / -(double)normal[axisW]);
}

void WalkPlane::draw(uint16 color) {
	if (bActive && numVerts > 0) {
		float *pPrevVert = &apVerts[numVerts - 1]->x;

		for (int i = 0; i < numVerts; i++) {
			float *pVert = &apVerts[i]->x;

			_pSystem->_pRd->_primit3.drawLine(pPrevVert, pVert, nullptr, nullptr, nullptr, nullptr, color, 0xFFFFFFFF);
			pPrevVert = pVert;
		}
	}
}

void WalkPlane::jumpOut(Puck *pPuck) {
	if (numVerts == 4) {
		RdVector3 pA;
		RdVector3 pB;
		RdVector3 pOut;

		pA[0] = pPuck->pos[0];
		pA[1] = pPuck->pos[1];
		pA[2] = pPuck->pos[2];

		pB[0] = (float)((double)pPuck->orient[1][0] * -10.0f + pA[0]);
		pB[1] = (float)((double)pPuck->orient[1][1] * -10.0f + pA[1]);
		pB[2] = (float)((double)pPuck->orient[1][2] * -10.0f + pA[2]);

		intersection(pA, pB, pOut);

		int edge = hitEdge;

		if (edge != -1) {
			float *pVert = &apVerts[edge]->x;
			int prevEdge = edge - 1;

			if (prevEdge <= -1)
				prevEdge = numVerts - 1;

			double deltaX = (double)apVerts[prevEdge]->x - pVert[0];
			double t;

			if (fabs(deltaX) > 0.00001f)
				t = ((double)pOut[0] - pVert[0]) / deltaX;
			else
				t = ((double)pOut[1] - pVert[1]) / ((double)apVerts[prevEdge]->y - pVert[1]);

			double inverseT = 1.0 - t;
			int nextEdge = (edge + 2) % numVerts;

			hitEdge = nextEdge;

			float *pTo = &apVerts[nextEdge]->x;
			int beforeNext = nextEdge - 1;

			if (beforeNext <= -1)
				beforeNext = numVerts - 1;

			float *pFrom = &apVerts[beforeNext]->x;

			pOut[0] = (float)(((double)pFrom[0] - pTo[0]) * inverseT + pTo[0]);
			pOut[1] = (float)(((double)pFrom[1] - pTo[1]) * inverseT + pTo[1]);
			pOut[2] = (float)(((double)pFrom[2] - pTo[2]) * inverseT + pTo[2]);

			pPuck->setPos(pOut);
		}
	}
}

float *WalkPlane::getConstraintVector(Puck *pPuck) {
	float *pConstraint = _pSystem->_constraintVector;
	RdVector3 pA;
	RdVector3 pB;

	pA[0] = pPuck->pos[0];
	pA[1] = pPuck->pos[1];
	pA[2] = pPuck->pos[2];

	float reach = 100.0f;
	if (pPuck->forwardDist > 0.0f)
		reach = -100.0f;

	pB[0] = (float)((double)reach * pPuck->orient[1][0] + pA[0]);
	pB[1] = (float)((double)reach * pPuck->orient[1][1] + pA[1]);
	pB[2] = (float)((double)reach * pPuck->orient[1][2] + pA[2]);

	intersection(pA, pB, pConstraint);

	if (hitEdge == -1)
		return nullptr;

	WpVertex *pHitVert = apVerts[hitEdge];

	double deltaX = (double)pHitVert->x - pConstraint[0];
	double deltaY = (double)pHitVert->y - pConstraint[1];
	double deltaZ = (double)pHitVert->z - pConstraint[2];
	float deltaXStored = (float)deltaX;
	float deltaZStored = (float)deltaZ;

	if (deltaX * deltaXStored + deltaY * deltaY + deltaZ * deltaZStored >= 0.0025) {
		int prevEdge = hitEdge - 1;

		if (prevEdge <= -1)
			prevEdge = numVerts - 1;

		WpVertex *pPrevVert = apVerts[prevEdge];

		double probeX = (double)pPrevVert->x - pConstraint[0];
		double probeY = (double)pPrevVert->y - pConstraint[1];
		double probeZ = (double)pPrevVert->z - pConstraint[2];

		if (probeX * probeX + probeY * probeY + probeZ * probeZ < 0.0025) {
			RdVector3 vec;

			vec[0] = (float)((double)pPrevVert->x - pHitVert->x);
			vec[1] = (float)((double)pPrevVert->y - apVerts[hitEdge]->y);
			vec[2] = (float)((double)pPrevVert->z - apVerts[hitEdge]->z);

			_pSystem->_pRd->normalize3Acc(vec);

			float value = _pSystem->_pRd->dot3(vec, pPuck->orient[1]);
			double angle = 90.0 - _pSystem->_pStdMath->arcSin3(value);

			if (angle < 1.0 || angle > 179.0) {
				int newEdge = hitEdge - 1;

				if (newEdge <= -1)
					newEdge = numVerts - 1;

				hitEdge = newEdge;
			}
		}
	} else {
		int prevEdge = hitEdge - 1;

		if (prevEdge <= -1)
			prevEdge = numVerts - 1;

		RdVector3 vec;

		vec[0] = (float)((double)apVerts[prevEdge]->x - pHitVert->x);
		vec[1] = (float)((double)apVerts[prevEdge]->y - apVerts[hitEdge]->y);
		vec[2] = (float)((double)apVerts[prevEdge]->z - apVerts[hitEdge]->z);

		_pSystem->_pRd->normalize3Acc(vec);

		float value = _pSystem->_pRd->dot3(vec, pPuck->orient[1]);
		double angle = 90.0 - _pSystem->_pStdMath->arcSin3(value);

		if (angle < 1.0 || angle > 179.0)
			hitEdge = (hitEdge + 1) % numVerts;
	}

	float *pEdgeVert = &apVerts[hitEdge]->x;
	int edgeBefore = hitEdge - 1;

	if (edgeBefore <= -1)
		edgeBefore = numVerts - 1;

	float *pEdgePrev = &apVerts[edgeBefore]->x;
	double edgeDeltaX = (double)pEdgePrev[0] - pEdgeVert[0];
	double t;

	if (fabs(edgeDeltaX) > 0.00001f)
		t = ((double)pConstraint[0] - pEdgeVert[0]) / edgeDeltaX;
	else
		t = ((double)pConstraint[1] - pEdgeVert[1]) / ((double)pEdgePrev[1] - pEdgeVert[1]);

	if (0.15f > t)
		t = 0.15f;

	if (0.85f < t)
		t = 0.85f;

	float *pNextVert = &apVerts[(hitEdge + 1) % numVerts]->x;
	float *pOtherVert = &apVerts[(numVerts + hitEdge - 2) % numVerts]->x;

	double targetX = ((double)pOtherVert[0] - pNextVert[0]) * t + pNextVert[0];
	double targetY = ((double)pOtherVert[1] - pNextVert[1]) * t + pNextVert[1];
	double targetZ = ((double)pOtherVert[2] - pNextVert[2]) * t + pNextVert[2];

	pConstraint[0] = (float)(targetX - pConstraint[0]);
	pConstraint[1] = (float)(targetY - pConstraint[1]);
	pConstraint[2] = (float)(targetZ - pConstraint[2]);

	_pSystem->_pRd->normalize3Acc(pConstraint);

	if (pPuck->forwardDist < 0.0f) {
		pConstraint[0] = (float)(pConstraint[0] * -1.0f);
		pConstraint[1] = (float)(pConstraint[1] * -1.0f);
		pConstraint[2] = (float)(pConstraint[2] * -1.0f);
	}

	return pConstraint;
}

int WalkPlane::clipPolygon(const float *pPoly, int numPoly, float *pOut) {
	float aBufferA[36] = { 0.0f };
	float aBufferB[36] = { 0.0f };
	float *pIn = aBufferA;
	float *pNew = aBufferB;
	int count = numPoly;

	if (numPoly > 12) {
		warning("WalkPlane::clipPolygon(): polygon has %d vertices, only 12 are supported", numPoly);
		return 0;
	}

	if (numPoly > 0)
		memcpy(aBufferA, pPoly, sizeof(float) * 3 * numPoly);

	if (numVerts > 0) {
		float *pPrevVert = &apVerts[numVerts - 1]->x;

		for (int i = 0; i < numVerts; i++) {
			float *pVert = &apVerts[i]->x;
			float edgeU = (float)(-((double)pVert[axisU] - pPrevVert[axisU]));
			float edgeV = (float)((double)pVert[axisV] - pPrevVert[axisV]);

			if (count <= 0)
				return 0;

			float *pPrevIn = &pIn[3 * (count - 1)];
			float prevDist = (float)(((double)pPrevIn[axisV] - pPrevVert[axisV]) * edgeU + ((double)pPrevIn[axisU] - pPrevVert[axisU]) * edgeV);
			int outCount = 0;

			for (int j = 0; j < count; j++) {
				float *pCurIn = &pIn[3 * j];
				double dist = ((double)pCurIn[axisV] - pPrevVert[axisV]) * edgeU + ((double)pCurIn[axisU] - pPrevVert[axisU]) * edgeV;

				bool bAddIntersection = (prevDist > 0.0f && dist < 0.0) || (prevDist < 0.0f && dist > 0.0);
				bool bAddVertex = dist > 0.0 && prevDist != 0.0f;

				if (bAddIntersection) {
					double deltaV = (double)pCurIn[axisV] - pPrevIn[axisV];
					double deltaU = (double)pCurIn[axisU] - pPrevIn[axisU];
					float deltaUStored = (float)deltaU;
					double denominator = -(deltaV * edgeU) - deltaU * edgeV;

					if (fabs(denominator) > 0.00001f) {
						if (outCount >= 12) {
							warning("WalkPlane::clipPolygon(): clipped polygon has more than 12 vertices");
							return 0;
						}

						double t = dist / denominator;
						float *pNewVert = &pNew[3 * outCount];

						pNewVert[axisV] = (float)(deltaV * t + pCurIn[axisV]);
						pNewVert[axisU] = (float)(deltaUStored * t + pCurIn[axisU]);
						outCount++;
					}
				}

				if (bAddVertex) {
					if (outCount >= 12) {
						warning("WalkPlane::clipPolygon(): clipped polygon has more than 12 vertices");
						return 0;
					}

					memcpy(&pNew[3 * outCount], pCurIn, sizeof(float) * 3);
					outCount++;
				}

				prevDist = (float)dist;
				pPrevIn = pCurIn;
			}

			if (outCount < 2)
				return 0;

			float *pSwap = pIn;
			pIn = pNew;
			pNew = pSwap;
			count = outCount;
			pPrevVert = pVert;
		}
	}

	for (int i = 0; i < count; i++) {
		memcpy(&pOut[3 * i], &pIn[3 * i], sizeof(float) * 3);
		project(&pOut[3 * i]);
	}

	return count;
}

double WalkPlane::distToEdge(int edge, float *pPoint) {
	if (edge < 0 || edge >= numVerts) {
		warning("WalkPlane::distToEdge(): edge %d out of range", edge);
		return 0.0;
	}

	float *pStart = &apVerts[edge]->x;
	float *pEnd = &apVerts[(edge + 1) % numVerts]->x;
	RdVector3 edgeVec;
	RdVector3 pointVec;
	RdVector3 edgeDir;
	RdVector3 pointDir;

	edgeVec[0] = (float)((double)pEnd[0] - pStart[0]);
	edgeVec[1] = (float)((double)pEnd[1] - pStart[1]);
	edgeVec[2] = (float)((double)pEnd[2] - pStart[2]);
	pointVec[0] = (float)((double)pPoint[0] - pStart[0]);
	pointVec[1] = (float)((double)pPoint[1] - pStart[1]);
	pointVec[2] = (float)((double)pPoint[2] - pStart[2]);

	_pSystem->_pRd->normalize3(edgeDir, edgeVec);
	float pointLength = _pSystem->_pRd->normalize3(pointDir, pointVec);

	float value = (float)((double)pointDir[0] * edgeDir[0] + (double)pointDir[1] * edgeDir[1] + (double)pointDir[2] * edgeDir[2]);
	float angle = (float)(90.0 - _pSystem->_pStdMath->arcSin3(value));
	float sinValue;
	float cosValue;

	_pSystem->_pStdMath->sinCos(angle, &sinValue, &cosValue);

	return (double)sinValue * pointLength;
}

int WalkPlane::removeShortEdge() {
	float minLength = 3.4028235e38f;
	int shortest = 0;

	if (numVerts < 4)
		return 0;

	float *pPrevVert = &apVerts[numVerts - 1]->x;

	for (int i = 0; i < numVerts; i++) {
		float *pVert = &apVerts[i]->x;
		double deltaY = (double)pPrevVert[1] - pVert[1];
		double deltaZ = (double)pPrevVert[2] - pVert[2];
		double deltaX = (double)pPrevVert[0] - pVert[0];
		float deltaYStored = (float)deltaY;
		float deltaXStored = (float)deltaX;
		double lengthSq = deltaY * deltaYStored + deltaZ * deltaZ + deltaX * deltaXStored;

		if (lengthSq < minLength) {
			minLength = (float)lengthSq;
			shortest = (numVerts + i - 1) % numVerts;
		}

		pPrevVert = pVert;
	}

	float *pEdgeStart = &apVerts[shortest]->x;
	float *pBefore = &apVerts[(numVerts + shortest - 1) % numVerts]->x;
	float *pAfter = &apVerts[(shortest + 1) % numVerts]->x;
	float *pFarVert = &apVerts[(shortest + 2) % numVerts]->x;
	float aPoint[3];

	aPoint[0] = pFarVert[0];
	aPoint[1] = pFarVert[1];
	aPoint[2] = pFarVert[2];

	float edgeU = (float)(-((double)pEdgeStart[axisU] - pBefore[axisU]));
	float edgeV = (float)((double)pEdgeStart[axisV] - pBefore[axisV]);
	float cross = (float)(((double)aPoint[axisU] - pBefore[axisU]) * edgeV + ((double)aPoint[axisV] - pBefore[axisV]) * edgeU);

	if (fabs((double)cross) <= 0.00001f)
		return 0;

	double deltaV = (double)aPoint[axisV] - pAfter[axisV];
	double deltaU = (double)aPoint[axisU] - pAfter[axisU];
	double denominator = -(deltaV * edgeU) - deltaU * edgeV;

	if (fabs(denominator) <= 0.00001f)
		return 0;

	double t = (double)cross / denominator;

	aPoint[axisV] = (float)(deltaV * t + aPoint[axisV]);
	aPoint[axisU] = (float)(deltaU * t + aPoint[axisU]);

	project(aPoint);

	WpVertex *pNewVert = _pSystem->addVertex(nullptr, aPoint);

	if (!pNewVert)
		return 0;

	WpVertex *apNew[10];

	apNew[0] = pNewVert;
	for (int i = 0; i < numVerts - 2; i++)
		apNew[1 + i] = apVerts[(shortest + 2 + i) % numVerts];

	_pSystem->removeVertex(apVerts[shortest]);
	_pSystem->removeVertex(apVerts[(shortest + 1) % numVerts]);

	numVerts--;

	for (int i = 0; i < numVerts; i++)
		apVerts[i] = apNew[i];

	return 1;
}

WalkSystem::WalkSystem(RenderDroid *pRd, StdMath *pStdMath, StdConffile *pStdConffile) {
	_pRd = pRd;
	_pStdMath = pStdMath;
	_pStdConffile = pStdConffile;

	memset(_aVertDb, 0, sizeof(_aVertDb));
	_totVerts = 0;
	_pWpHead = nullptr;
	_curCollection = nullptr;
	memset(_aCollection, 0, sizeof(_aCollection));
	_pPuckHead = nullptr;
	_forwardCounter = 0;

	memset(_aPath, 0, sizeof(_aPath));
	_numPathPoints = 0;
	memset(_constraintVector, 0, sizeof(_constraintVector));
	_sectionName[0] = 0;

	_pCache = nullptr;
	_pVertCache = nullptr;
	_numPlanes = 0;
	_numCacheVerts = 0;
	_bCacheBuilt = 0;
}

WalkSystem::~WalkSystem() {
	destroyCache();

	for (int i = 0; i < 140; i++) {
		while (_aCollection[i].pHead) {
			WalkPlane *pPlane = _aCollection[i].pHead;
			delete pPlane;
		}
	}
}

void WalkSystem::buildCache() {
	destroyCache();

	_numCacheVerts = 0;
	_numPlanes = 0;

	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		_numCacheVerts += pPlane->numVerts;
		_numPlanes++;
	}

	_pVertCache = new WpCacheVert[_numCacheVerts];

	int cursor = 0;
	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		for (int i = 0; i < pPlane->numVerts; i++) {
			WpVertex *pVert = pPlane->apVerts[i];

			_pVertCache[cursor].v.refCount = pVert->refCount;
			_pVertCache[cursor].v.x = pVert->x;
			_pVertCache[cursor].v.y = pVert->y;
			_pVertCache[cursor].v.z = pVert->z;
			_pVertCache[cursor].index = (int)(pVert - _aVertDb);
			cursor++;
		}
	}

	_pCache = new WpCachePlane[_numPlanes];

	int planeIndex = 0;
	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		_pCache[planeIndex].pPlane = pPlane;
		_pCache[planeIndex].numVerts = pPlane->numVerts;
		_pCache[planeIndex].bActive = pPlane->bActive;

		for (int i = 0; i < pPlane->numVerts; i++)
			_pCache[planeIndex].aVertIdx[i] = (int)(pPlane->apVerts[i] - _aVertDb);

		planeIndex++;
	}

	_bCacheBuilt = 1;
}

void WalkSystem::destroyCache() {
	_bCacheBuilt = 0;

	delete[] _pVertCache;
	delete[] _pCache;

	_pVertCache = nullptr;
	_pCache = nullptr;
	_numPlanes = 0;
	_numCacheVerts = 0;
}

void WalkSystem::restoreCache() {
	if (_bCacheBuilt) {
		for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
			for (int i = 0; i < pPlane->numVerts; i++)
				removeVertex(pPlane->apVerts[i]);
		}

		for (int i = 0; i < _numCacheVerts; i++) {
			WpVertex *pVert = &_aVertDb[_pVertCache[i].index];

			pVert->refCount = _pVertCache[i].v.refCount;
			pVert->x = _pVertCache[i].v.x;
			pVert->y = _pVertCache[i].v.y;
			pVert->z = _pVertCache[i].v.z;
		}

		for (int i = 0; i < _numPlanes; i++) {
			WalkPlane *pPlane = _pCache[i].pPlane;

			pPlane->numVerts = _pCache[i].numVerts;

			for (int j = 0; j < pPlane->numVerts; j++)
				pPlane->apVerts[j] = &_aVertDb[_pCache[i].aVertIdx[j]];

			pPlane->bActive = _pCache[i].bActive;
		}

		_totVerts = 0;

		for (int i = 0; i < 1024; i++) {
			if (_aVertDb[i].refCount > 0)
				_totVerts++;
		}
	}
}

void WalkSystem::killCollection(const char *collection) {
	WalkPlane *pOldHead = _pWpHead;

	setCollection(collection);

	if (pOldHead == _pWpHead)
		pOldHead = nullptr;

	while (_pWpHead) {
		WalkPlane *pPlane = _pWpHead;
		delete pPlane;
	}

	_pWpHead = pOldHead;
}

void WalkSystem::setCollection(const char *name) {
	int index;

	for (index = 0; index < 140; index++) {
		if (!strcmp(_aCollection[index].name, name))
			break;
	}

	if (index >= 140) {
		_pWpHead = nullptr;
		_curCollection = nullptr;
	} else {
		_pWpHead = _aCollection[index].pHead;
		_curCollection = _aCollection[index].name;
	}
}

WalkPlane *WalkSystem::planeAt(float *pPoint, int flags) {
	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		if ((flags & pPlane->type) != 0 && pPlane->within(pPoint, 1))
			return pPlane;
	}

	return nullptr;
}

WalkPlane *WalkSystem::planeNamed(const char *name) {
	WalkPlane *pPlane;

	for (pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		if (!strcmp(pPlane->name, name))
			break;
	}

	return pPlane;
}

WalkPlane *WalkSystem::planeNumbered(int index) {
	WalkPlane *pPlane;

	for (pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		if (pPlane->number == index)
			break;
	}

	return pPlane;
}

WalkPlane *WalkSystem::planeClosest(float *pPoint, int flags) {
	WalkPlane *pClosest = nullptr;
	float minDistance = 3.4028235e38f;

	WalkPlane *pFound = planeAt(pPoint, flags);
	if (pFound)
		return pFound;

	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		if ((flags & pPlane->type) != 0 && pPlane->bEnabled && pPlane->bActive) {
			float aPoint[3];

			aPoint[0] = pPoint[0];
			aPoint[1] = pPoint[1];
			aPoint[2] = pPoint[2];

			pPlane->closestPoint(aPoint);

			double deltaZ = (double)aPoint[2] - pPoint[2];
			double deltaY = (double)aPoint[1] - pPoint[1];
			double deltaX = (double)aPoint[0] - pPoint[0];
			float deltaYStored = (float)deltaY;
			float deltaXStored = (float)deltaX;
			double distance = deltaY * deltaYStored + deltaZ * deltaZ + deltaX * deltaXStored;

			if (distance < minDistance) {
				minDistance = (float)distance;
				pClosest = pPlane;
			}
		}
	}

	return pClosest;
}

void WalkSystem::allActive() {
	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext)
		pPlane->bEnabled = 1;
}

WpVertex *WalkSystem::addVertex(WalkPlane *pPlane, float *pPos) {
	if (_totVerts >= 1022)
		warning("WalkSystem::addVertex(): vertex database is almost full (%d vertices)", _totVerts);

	for (int i = 0; i < 1024; i++) {
		if (_aVertDb[i].refCount != 0) {
			double deltaY = (double)pPos[1] - _aVertDb[i].y;
			double deltaX = (double)pPos[0] - _aVertDb[i].x;
			double deltaZ = (double)pPos[2] - _aVertDb[i].z;
			float deltaXStored = (float)deltaX;
			float deltaZStored = (float)deltaZ;

			if (deltaX * deltaXStored + deltaY * deltaY + deltaZ * deltaZStored < 0.00000001) {
				_aVertDb[i].refCount++;
				return &_aVertDb[i];
			}
		}
	}

	WpVertex *pVert = nullptr;

	for (int i = 0; i < 1024; i++) {
		if (_aVertDb[i].refCount <= 0) {
			pVert = &_aVertDb[i];
			break;
		}
	}

	if (!pVert) {
		warning("WalkSystem::addVertex(): vertex database is full");
		return nullptr;
	}

	pVert->refCount = 1;
	_totVerts++;
	pVert->x = pPos[0];
	pVert->y = pPos[1];
	pVert->z = pPos[2];

	return pVert;
}

void WalkSystem::removeVertex(WpVertex *pVert) {
	if (pVert->refCount > 0) {
		pVert->refCount--;

		if (pVert->refCount == 0)
			_totVerts--;
	}
}

const char *WalkSystem::parseCollection(const char *collection) {
	int numVerts = 0;
	int id = 0;
	int type = 0;
	int bVisible = 0;
	int numVertsRead = 0;
	int vertsParsed = 0;
	float height = 12345.0f;
	char sectorName[48];
	char word[48];
	char line[256];
	float aVerts[10][3];
	float aOverflow[3];
	int value;

	memset(aVerts, 0, sizeof(aVerts));
	sectorName[0] = 0;
	_sectionName[0] = 0;

	while (_pStdConffile->readLine()) {
		Common::strlcpy(line, _pStdConffile->getLine(), sizeof(line));

		if (sscanf(line, " section: %47s", _sectionName) == 1) {
			if (numVerts > 2)
				newPlane(aVerts, numVerts, collection, sectorName, id, bVisible, type);

			return _sectionName;
		}

		if (!strstr(line, " numsectors ")) {
			if (sscanf(line, " sector %47s", word) == 1) {
				if (numVerts > 2) {
					WalkPlane *pPlane = newPlane(aVerts, numVerts, collection, sectorName, id, bVisible, type);

					if (height != 12345.0f) {
						if (type == 4096)
							height = 0.005f;

						if (height == 0.0f)
							pPlane->tolerance = 0.005f;
						else
							pPlane->tolerance = height;
					}
				}

				sectorName[0] = 0;
				height = 12345.0f;
				Common::strlcpy(sectorName, word, sizeof(sectorName));
				id = 0;
				type = 0;
				bVisible = 0;
				numVertsRead = 0;
				vertsParsed = 0;
			} else if (sscanf(line, " id %i", &value) == 1) {
				id = value;
			} else if (sscanf(line, " type %47s", word) == 1) {
				if (!strcmp(word, "walk"))
					type = 4096;
				else if (!strcmp(word, "funnel"))
					type = 4352;
				else if (!strcmp(word, "camera"))
					type = 0x2000;
				else if (!strcmp(word, "special"))
					type = 0x4000;
				else if (!strcmp(word, "chernobyl"))
					type = 0x8000;
			} else if (sscanf(line, " default visibility %47s", word) == 1) {
				bVisible = strcmp(word, "invisible") != 0;
			} else if (sscanf(line, " height %f", &height) != 1) {
				if (sscanf(line, " numvertices %d", &value) == 1) {
					numVertsRead = value;
				} else if (sscanf(line, " numgates %47s", word) != 1) {
					float *pVert = aOverflow;
					if (vertsParsed < 10)
						pVert = aVerts[vertsParsed];

					if (sscanf(line, " vertices: %f %f %f", &pVert[0], &pVert[1], &pVert[2]) == 3 || (vertsParsed < numVertsRead && sscanf(line, " %f %f %f", &pVert[0], &pVert[1], &pVert[2]) == 3)) {
						if (vertsParsed >= 10)
							warning("WalkSystem::parseCollection(): sector %s has more than 10 vertices", sectorName);

						vertsParsed++;
					}
				}
			}
		}

		numVerts = numVertsRead;
	}

	if (numVerts > 2)
		newPlane(aVerts, numVerts, collection, sectorName, id, bVisible, type);

	return nullptr;
}

WalkPlane *WalkSystem::newPlane(float (*pVerts)[3], int numPoints, const char *collection, const char *sectorName, int id, int bVisible, int type) {
	WalkPlane *pPlane = new WalkPlane(this, pVerts, numPoints, collection);

	pPlane->setName(sectorName);
	pPlane->bActive = bVisible;
	pPlane->number = id;
	pPlane->setType(type);

	return pPlane;
}

int WalkSystem::closestVertWithin(WalkPlane *pA, WalkPlane *pB, float *pTarget, float *pOut) {
	float minDistance = 3.4028235e38f;

	for (int i = 0; i < pA->numVerts; i++) {
		WpVertex *pVert = pA->apVerts[i];

		if (pB->within(&pVert->x, 0)) {
			double deltaZ = (double)pVert->z - pTarget[2];
			double deltaY = (double)pVert->y - pTarget[1];
			double deltaX = (double)pVert->x - pTarget[0];
			float deltaYStored = (float)deltaY;
			float deltaXStored = (float)deltaX;
			double distance = deltaY * deltaYStored + deltaZ * deltaZ + deltaX * deltaXStored;

			if (distance < minDistance) {
				minDistance = (float)distance;
				pOut[0] = pVert->x;
				pOut[1] = pVert->y;
				pOut[2] = pVert->z;
			}
		}
	}

	for (int i = 0; i < pB->numVerts; i++) {
		WpVertex *pVert = pB->apVerts[i];

		if (pA->within(&pVert->x, 0)) {
			double deltaZ = (double)pVert->z - pTarget[2];
			double deltaY = (double)pVert->y - pTarget[1];
			double deltaX = (double)pVert->x - pTarget[0];
			float deltaYStored = (float)deltaY;
			float deltaXStored = (float)deltaX;
			double distance = deltaY * deltaYStored + deltaZ * deltaZ + deltaX * deltaXStored;

			if (distance < minDistance) {
				minDistance = (float)distance;
				pOut[0] = pVert->x;
				pOut[1] = pVert->y;
				pOut[2] = pVert->z;
			}
		}
	}

	return minDistance != 3.4028235e38f;
}

WalkPlane *WalkSystem::planeContaining(float *pA, float *pB) {
	WalkPlane *pPlane = planeAt(pA, 4096);

	while (pPlane) {
		if (pPlane->within(pB, 1))
			break;

		pPlane->bEnabled = 0;
		pPlane = planeAt(pA, 4096);
	}

	allActive();

	return pPlane;
}

int WalkSystem::getPath(float **ppPath) {
	*ppPath = &_aPath[0][0];

	return _numPathPoints;
}

int WalkSystem::buildPath(float *pFrom, float *pTo, int bTighten) {
	int result = 0;

	allActive();
	_numPathPoints = 0;

	if (!planeAt(pFrom, 4096))
		return 0;

	if (!planeAt(pTo, 4096))
		return 0;

	if (findPath(pFrom, pTo, nullptr)) {
		if (_numPathPoints >= 30) {
			warning("WalkSystem::buildPath(): path has too many points");
		} else {
			_aPath[_numPathPoints][0] = pTo[0];
			_aPath[_numPathPoints][1] = pTo[1];
			_aPath[_numPathPoints][2] = pTo[2];
			_numPathPoints++;

			if (bTighten)
				tightenPath();
		}

		result = 1;
	}

	allActive();

	return result;
}

int WalkSystem::findPath(float *pFrom, float *pTo, WalkPlane *pExclude) {
	if (_numPathPoints + 1 >= 30) {
		if (pExclude)
			pExclude->bEnabled = 0;

		return 1;
	}

	WalkPlane *pPlane = pExclude;
	if (!pPlane)
		pPlane = planeAt(pFrom, 4096);

	if (!pPlane)
		return 0;

	_aPath[_numPathPoints][0] = pFrom[0];
	_aPath[_numPathPoints][1] = pFrom[1];
	_aPath[_numPathPoints][2] = pFrom[2];
	_numPathPoints++;

	if (pPlane->within(pTo, 1))
		return 1;

	int savedPathPoints = _numPathPoints;
	float aHit[3];

	pPlane->intersection(pFrom, pTo, aHit);
	pPlane->bEnabled = 0;

	double deltaZ = (double)aHit[2] - pFrom[2];
	double deltaY = (double)aHit[1] - pFrom[1];
	double deltaX = (double)aHit[0] - pFrom[0];
	float deltaYStored = (float)deltaY;
	float deltaZStored = (float)deltaZ;
	float deltaXStored = (float)deltaX;
	WalkPlane *pNextPlane = nullptr;

	if (deltaY * deltaYStored + deltaZ * deltaZStored + deltaX * deltaXStored > 0.0001f)
		pNextPlane = planeAt(aHit, 4096);

	if (pNextPlane) {
		if (findPath(aHit, pTo, pNextPlane))
			return 1;

		pNextPlane->bEnabled = 0;
	}

	float aExits[120];
	int numExits = findExitPoints(pPlane, pTo, aExits);

	for (int i = 0; i < numExits; i++) {
		WalkPlane *pExit = planeAt(&aExits[4 * i], 4096);

		if (pExit) {
			_numPathPoints = savedPathPoints;

			if (closestVertWithin(pPlane, pExit, pTo, aHit)) {
				if (findPath(aHit, pTo, pExit))
					return 1;
			}
		}
	}

	pPlane->bEnabled = 1;

	if (pNextPlane)
		pNextPlane->bEnabled = 1;

	return 0;
}

int WalkSystem::findExitPoints(WalkPlane *pPlane, float *pFrom, float *pOut) {
	float aExits[30][4];
	int count = 0;

	for (WalkPlane *pOther = _pWpHead; pOther; pOther = pOther->pNext) {
		if (pOther != pPlane && pOther->bEnabled && (pOther->type & 0x1000) != 0) {
			for (int i = 0; i < pOther->numVerts; i++) {
				WpVertex *pVert = pOther->apVerts[i];

				if (pPlane->within(&pVert->x, 0)) {
					int j;

					for (j = 0; j < count; j++) {
						if (!memcmp(aExits[j], &pVert->x, 12))
							break;
					}

					if (j == count) {
						if (count >= 30) {
							warning("WalkSystem::findExitPoints(): more than 30 exit points");
							continue;
						}

						aExits[count][0] = pVert->x;
						aExits[count][1] = pVert->y;
						aExits[count][2] = pVert->z;

						double deltaX = (double)pFrom[0] - aExits[count][0];
						double deltaY = (double)pFrom[1] - aExits[count][1];
						double deltaZ = (double)pFrom[2] - aExits[count][2];
						float deltaXStored = (float)deltaX;
						float deltaZStored = (float)deltaZ;

						aExits[count][3] = (float)(deltaX * deltaXStored + deltaY * deltaY + deltaZ * deltaZStored);
						count++;
					}
				}
			}
		}
	}

	if (count > 0) {
		memcpy(&pOut[0], aExits[0], sizeof(float) * 4);

		for (int i = 1; i < count; i++) {
			float distance = aExits[i][3];
			int j = i - 1;

			while (j > -1) {
				if (pOut[4 * j + 3] <= (double)distance)
					break;

				memcpy(&pOut[4 * (j + 1)], &pOut[4 * j], sizeof(float) * 4);
				j--;
			}

			memcpy(&pOut[4 * (j + 1)], aExits[i], sizeof(float) * 4);
		}
	}

	return count;
}

void WalkSystem::tightenPath() {
	int bChanged = 0;

	if (_numPathPoints - 3 > 0) {
		int last = 2;
		int i = 0;

		do {
			float *pFirst = _aPath[last - 2];
			float *pMid = _aPath[last - 1];
			float *pLast = _aPath[last];

			WalkPlane *pPlaneA = planeContaining(pFirst, pMid);
			WalkPlane *pPlaneB = planeContaining(pMid, pLast);

			if (!pPlaneA || !pPlaneB)
				return;

			float aPoint[3];

			pPlaneA->intersection(pFirst, pLast, aPoint);

			WalkPlane *pPlaneC = planeContaining(aPoint, pFirst);

			if (planeContaining(aPoint, pLast)) {
				if (pPlaneC) {
					double deltaY = (double)aPoint[1] - pMid[1];
					double deltaX = (double)aPoint[0] - pMid[0];
					double deltaZ = (double)aPoint[2] - pMid[2];
					float deltaXStored = (float)deltaX;
					float deltaZStored = (float)deltaZ;

					if (deltaY * deltaY + deltaX * deltaXStored + deltaZ * deltaZStored > 0.0025f) {
						pMid[0] = aPoint[0];
						pMid[1] = aPoint[1];
						pMid[2] = aPoint[2];
						bChanged = 1;
					}
				}
			}

			last++;
			i++;
		} while (i < _numPathPoints - 3);
	}

	if (_numPathPoints - 2 > 0) {
		int j = 1;

		do {
			float *pPrevPt = _aPath[j - 1];
			float *pPt = _aPath[j];
			double deltaY = (double)pPrevPt[1] - pPt[1];
			double deltaX = (double)pPrevPt[0] - pPt[0];
			double deltaZ = (double)pPrevPt[2] - pPt[2];
			float deltaXStored = (float)deltaX;
			float deltaZStored = (float)deltaZ;

			if (deltaY * deltaY + deltaX * deltaXStored + deltaZ * deltaZStored < 0.0001f) {
				if (planeContaining(pPrevPt, pPt)) {
					bChanged = 1;
					_numPathPoints--;

					for (int k = j; k < _numPathPoints; k++) {
						_aPath[k][0] = _aPath[k + 1][0];
						_aPath[k][1] = _aPath[k + 1][1];
						_aPath[k][2] = _aPath[k + 1][2];
					}
				}
			}

			j++;
		} while (j - 1 < _numPathPoints - 2);
	}

	if (bChanged)
		tightenPath();
}

void WalkSystem::shrinkBoxes(float amount) {
	int vertexUseCount[1024];
	float vertexOffsetSum[3072];
	float aOffsets[30];

	if (amount < 0.0f)
		return;

	memset(vertexOffsetSum, 0, sizeof(vertexOffsetSum));
	memset(vertexUseCount, 0, sizeof(vertexUseCount));

	for (WalkPlane *pPlane = _pWpHead; pPlane; pPlane = pPlane->pNext) {
		if ((pPlane->type & 0x1000) == 0 || !pPlane->bActive)
			continue;

		bool bRestart;

		do {
			bRestart = false;
			memset(aOffsets, 0, sizeof(aOffsets));

			float *pVert = &pPlane->apVerts[0]->x;
			float *pPrevVert = &pPlane->apVerts[pPlane->numVerts - 1]->x;
			int corner = 0;
			bool bValid = true;

			while (bValid && corner < pPlane->numVerts) {
				float *pNextVert = &pPlane->apVerts[(corner + 1) % pPlane->numVerts]->x;
				RdVector3 toPrev;
				RdVector3 toNext;
				RdVector3 bisector;
				RdVector3 pPoint;

				toPrev[0] = (float)((double)pPrevVert[0] - pVert[0]);
				toPrev[1] = (float)((double)pPrevVert[1] - pVert[1]);
				toPrev[2] = (float)((double)pPrevVert[2] - pVert[2]);
				_pRd->normalize3Acc(toPrev);

				toNext[0] = (float)((double)pNextVert[0] - pVert[0]);
				toNext[1] = (float)((double)pNextVert[1] - pVert[1]);
				toNext[2] = (float)((double)pNextVert[2] - pVert[2]);
				_pRd->normalize3Acc(toNext);

				bisector[0] = (float)((double)toNext[0] + toPrev[0]);
				bisector[1] = (float)((double)toNext[1] + toPrev[1]);
				bisector[2] = (float)((double)toNext[2] + toPrev[2]);
				_pRd->normalize3Acc(bisector);

				float value = (float)((double)toNext[0] * bisector[0] + (double)toNext[1] * bisector[1] + (double)toNext[2] * bisector[2]);
				float angle = (float)(90.0 - _pStdMath->arcSin3(value));
				float sinValue;
				float cosValue;

				_pStdMath->sinCos(angle, &sinValue, &cosValue);

				double offsetLength = (double)amount / sinValue;
				float offsetLengthStored = (float)offsetLength;

				pPoint[0] = (float)(offsetLength * bisector[0] + pVert[0]);
				pPoint[1] = (float)((double)offsetLengthStored * bisector[1] + pVert[1]);
				pPoint[2] = (float)((double)offsetLengthStored * bisector[2] + pVert[2]);

				if (pPlane->within(pPoint, 1)) {
					double limit = (double)amount - 0.001;

					for (int edge = 0; edge < pPlane->numVerts; edge++) {
						if (pPlane->distToEdge(edge, pPoint) < limit) {
							bValid = false;
							break;
						}
					}
				} else {
					bValid = false;
				}

				if (bValid) {
					aOffsets[3 * corner] = (float)((double)offsetLengthStored * bisector[0]);
					aOffsets[3 * corner + 1] = (float)((double)offsetLengthStored * bisector[1]);
					aOffsets[3 * corner + 2] = (float)((double)offsetLengthStored * bisector[2]);

					corner++;
					pPrevVert = pVert;
					pVert = pNextVert;
				}
			}

			if (bValid) {
				for (int i = 0; i < pPlane->numVerts; i++) {
					int index = (int)(pPlane->apVerts[i] - _aVertDb);

					vertexOffsetSum[3 * index] = (float)(aOffsets[3 * i] + vertexOffsetSum[3 * index]);
					vertexOffsetSum[3 * index + 1] = (float)(vertexOffsetSum[3 * index + 1] + aOffsets[3 * i + 1]);
					vertexOffsetSum[3 * index + 2] = (float)(aOffsets[3 * i + 2] + vertexOffsetSum[3 * index + 2]);
					vertexUseCount[index]++;
				}
			} else if (pPlane->removeShortEdge()) {
				bRestart = true;
			} else {
				pPlane->bActive = 0;
			}
		} while (bRestart);
	}

	for (int i = 0; i < 1024; i++) {
		if (vertexUseCount[i] != 0) {
			double inverse = 1.0 / (double)vertexUseCount[i];

			_aVertDb[i].x = (float)(vertexOffsetSum[3 * i] * inverse + _aVertDb[i].x);
			_aVertDb[i].y = (float)(vertexOffsetSum[3 * i + 1] * inverse + _aVertDb[i].y);
			_aVertDb[i].z = (float)(vertexOffsetSum[3 * i + 2] * inverse + _aVertDb[i].z);
		}
	}
}

} // End of namespace Grim
