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

#include "common/textconsole.h"

#include "engines/grim/puck.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/walkplane.h"

namespace Grim {

Puck::Puck(WalkSystem *pSystem, float angle) {
	_pSystem = pSystem;

	pos[0] = 0.0f;
	pos[1] = 0.0f;
	pos[2] = 0.0f;
	_pSystem->_pRd->build34(orient, pos, pos);

	constraint[1] = 1.0f;
	reflectionAngle = angle;
	pPlane = nullptr;
	forwardDist = 0.0f;
	bConstrained = 0;
	constraint[0] = 0.0f;
	constraint[2] = 0.0f;
	pNext = nullptr;
	pPrev = nullptr;

	pNext = _pSystem->_pPuckHead;

	if (_pSystem->_pPuckHead)
		_pSystem->_pPuckHead->pPrev = this;

	_pSystem->_pPuckHead = this;
}

Puck::~Puck() {
	if (this == _pSystem->_pPuckHead)
		_pSystem->_pPuckHead = pNext;

	if (pPrev)
		pPrev->pNext = pNext;

	if (pNext)
		pNext->pPrev = pPrev;
}

float *Puck::rot() {
	_pSystem->_pRd->extractAngles34(orient, _rot);

	return _rot;
}

WalkPlane *Puck::validate() {
	pPlane = _pSystem->planeClosest(pos, 4096);

	if (pPlane) {
		if (!pPlane->within(pos, 1)) {
			pPlane->closestPoint(pos);
			setPos(pos);
		}

		orientToPlane(pPlane);
	}

	return pPlane;
}

void Puck::setPos(float *pPos) {
	pos[0] = pPos[0];
	pos[1] = pPos[1];
	pos[2] = pPos[2];

	orient[3][0] = pPos[0];
	orient[3][1] = pPos[1];
	orient[3][2] = pPos[2];
}

void Puck::xform(float *pVec) {
	_pSystem->_pRd->transformPoint34Acc(pVec, orient);
}

void Puck::invXform(float *pVec) {
	RdVector3 out;

	pVec[0] = (float)((double)pVec[0] - pos[0]);
	pVec[1] = (float)((double)pVec[1] - pos[1]);
	pVec[2] = (float)((double)pVec[2] - pos[2]);

	_pSystem->_pRd->transformVectorOrtho34(out, pVec, orient);

	pVec[0] = out[0];
	pVec[1] = out[1];
	pVec[2] = out[2];
}

void Puck::rotate(float angle) {
	RdMatrix34 out;

	_pSystem->_pRd->buildFromVectorAngle34(out, orient[2], angle);
	_pSystem->_pRd->transformVector34Acc(orient[1], out);

	orient[1][0] = (float)((double)orient[1][0] + orient[3][0]);
	orient[1][1] = (float)((double)orient[3][1] + orient[1][1]);
	orient[1][2] = (float)((double)orient[3][2] + orient[1][2]);

	if (pPlane)
		pPlane->snapTo(orient[1]);

	orient[1][0] = (float)((double)orient[1][0] - orient[3][0]);
	orient[1][1] = (float)((double)orient[1][1] - orient[3][1]);
	orient[1][2] = (float)((double)orient[1][2] - orient[3][2]);

	normalize();
}

float Puck::pointAt(float *pTarget, float maxAngle) {
	RdVector3 vec;

	vec[0] = pTarget[0];
	vec[1] = pTarget[1];
	vec[2] = pTarget[2];

	if (pPlane)
		pPlane->project(vec);

	invXform(vec);
	_pSystem->_pRd->normalize3Acc(vec);

	float angle = (float)(90.0 - _pSystem->_pStdMath->arcSin3(vec[1]));

	if (maxAngle < angle)
		angle = maxAngle;

	if (vec[0] > 0.0f)
		angle = -angle;

	rotate(angle);

	return angle;
}

double Puck::getYawToPoint(float *pTarget) {
	RdVector3 vec;

	vec[0] = pTarget[0];
	vec[1] = pTarget[1];
	vec[2] = pTarget[2];

	if (pPlane)
		pPlane->project(vec);

	invXform(vec);
	_pSystem->_pRd->normalize3Acc(vec);

	float angle = (float)(90.0 - _pSystem->_pStdMath->arcSin3(vec[1]));

	if (vec[0] > 0.0f)
		angle = -angle;

	return (double)rot()[1] + angle;
}

void Puck::constrain(int bEnable) {
	bConstrained = 0;

	if (pPlane && bEnable) {
		float *pConstraint = pPlane->getConstraintVector(this);

		if (pConstraint) {
			constraint[0] = pConstraint[0];
			constraint[1] = pConstraint[1];
			constraint[2] = pConstraint[2];

			orient[1][0] = pConstraint[0];
			orient[1][1] = pConstraint[1];
			orient[1][2] = pConstraint[2];

			normalize();
			bConstrained = 1;
		}
	}
}

int Puck::forward(float dist, int bAllowLeft, int bAllowRight) {
	int result = 0;
	float value = 0.0f;

	if (_pSystem->_forwardCounter == 0)
		forwardDist = dist;

	_pSystem->_forwardCounter++;

	if (bConstrained) {
		float dot = _pSystem->_pRd->dot3(orient[0], constraint);
		double angleDelta = (90.0 - _pSystem->_pStdMath->arcSin3(dot)) - 90.0;

		value = (float)angleDelta;

		if (fabs(angleDelta) > reflectionAngle)
			return 0;

		if (_pSystem->_pRd->dot3(orient[1], constraint) <= 0.0f)
			rotate(-value);
		else
			rotate(value);
	}

	RdVector3 pPoint;

	pPoint[0] = 0.0f;
	pPoint[1] = dist;
	pPoint[2] = 0.0f;

	xform(pPoint);

	if (!pPlane || pPlane->within(pPoint, 1)) {
		setPos(pPoint);
		_pSystem->_forwardCounter = 0;

		return 1;
	}

	if (_pSystem->_forwardCounter > 8) {
		_pSystem->_forwardCounter = 0;

		return 0;
	}

	WalkPlane *pOldPlane = pPlane;
	RdVector3 pOut;

	value = (float)pOldPlane->intersection(pos, pPoint, pOut);
	pOldPlane->bEnabled = 0;

	RdVector3 local;

	local[0] = pOut[0];
	local[1] = pOut[1];
	local[2] = pOut[2];

	invXform(local);

	dist = (float)((double)dist - local[1]);

	WalkPlane *pNewPlane = _pSystem->planeAt(pOut, 4096);

	if (pNewPlane) {
		setPos(pOut);
		pPlane = pNewPlane;
		orientToPlane(pNewPlane);

		if (dist >= 0.02f)
			dist = 0.02f;

		result = forward(dist, 1, 1);
		pOldPlane->bEnabled = 1;

		return result;
	}

	setPos(pOut);

	if ((pPlane->type & 0x100) != 0) {
		int quadAxis = pPlane->quadAxis;

		if ((quadAxis + 1) % pPlane->numVerts == pPlane->hitEdge)
			return 0;

		float *pVert = &pPlane->apVerts[quadAxis]->x;
		RdVector3 vertLocal;

		vertLocal[0] = pVert[0];
		vertLocal[1] = pVert[1];
		vertLocal[2] = pVert[2];

		invXform(vertLocal);

		if (vertLocal[0] <= 0.0f) {
			if (value < 3.0f)
				value = (float)(((double)value + 90.0f) + 90.2f);
		} else if (value > 3.0f) {
			value = (float)(-90.2f - (90.0f - (double)value));
		}

		rotate(value);
		result = forward(dist, 1, 1);
		pOldPlane->bEnabled = 1;

		return result;
	}

	if (fabs((double)value) >= reflectionAngle || bConstrained) {
		forwardDist = (float)((double)forwardDist - dist);
	} else if ((value > 0.0f && bAllowLeft) || (value < 0.0f && bAllowRight)) {
		rotate(value);
		pOldPlane->bEnabled = 1;
		result = forward(dist, 1, 1);
	}

	if (bConstrained) {
		rotate(value);
		bConstrained = 1;
		constraint[0] = orient[1][0];
		constraint[1] = orient[1][1];
		constraint[2] = orient[1][2];
	}

	pOldPlane->bEnabled = 1;

	return result;
}

void Puck::orientToPlane(WalkPlane *pTargetPlane) {
	double forwardX = orient[1][0];
	double forwardY = orient[1][1];
	double forwardZ = orient[1][2];

	orient[2][0] = pTargetPlane->normal[0];
	orient[2][1] = pTargetPlane->normal[1];
	orient[2][2] = pTargetPlane->normal[2];

	orient[0][0] = (float)(forwardY * orient[2][2] - forwardZ * orient[2][1]);
	orient[0][1] = (float)((double)orient[2][0] * forwardZ - forwardX * orient[2][2]);
	orient[0][2] = (float)(forwardX * orient[2][1] - forwardY * orient[2][0]);

	_pSystem->_pRd->normalize3Acc(orient[0]);

	orient[1][0] = (float)((double)orient[0][2] * orient[2][1] - (double)orient[0][1] * orient[2][2]);
	orient[1][1] = (float)((double)orient[2][2] * orient[0][0] - (double)orient[0][2] * orient[2][0]);
	orient[1][2] = (float)((double)orient[0][1] * orient[2][0] - (double)orient[2][1] * orient[0][0]);

	_pSystem->_pRd->normalize3Acc(orient[1]);
}

void Puck::normalize() {
	_pSystem->_pRd->normalize3Acc(orient[1]);

	orient[0][0] = (float)((double)orient[2][2] * orient[1][1] - (double)orient[1][2] * orient[2][1]);
	orient[0][1] = (float)((double)orient[1][2] * orient[2][0] - (double)orient[2][2] * orient[1][0]);
	orient[0][2] = (float)((double)orient[1][0] * orient[2][1] - (double)orient[1][1] * orient[2][0]);

	_pSystem->_pRd->normalize3Acc(orient[0]);
}

int Puck::draw(uint16 color) {
	RdVector3 p0;
	RdVector3 p1;

	p0[0] = 0.0f;
	p0[1] = 0.0f;
	p0[2] = 0.0f;
	p1[0] = 0.0f;
	p1[1] = 0.1f;
	p1[2] = 0.0f;
	xform(p0);
	xform(p1);
	_pSystem->_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, color, 0xFFFFFFFF);

	p0[0] = 0.05f;
	p0[1] = 0.05f;
	p0[2] = 0.0f;
	p1[0] = 0.0f;
	p1[1] = 0.1f;
	p1[2] = 0.0f;
	xform(p0);
	xform(p1);
	_pSystem->_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, color, 0xFFFFFFFF);

	p0[0] = -0.05f;
	p0[1] = 0.05f;
	p0[2] = 0.0f;
	p1[0] = 0.0f;
	p1[1] = 0.1f;
	p1[2] = 0.0f;
	xform(p0);
	xform(p1);
	_pSystem->_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, color, 0xFFFFFFFF);

	p0[0] = 0.0f;
	p0[1] = 0.0f;
	p0[2] = 0.05f;
	p1[0] = 0.0f;
	p1[1] = 0.0f;
	p1[2] = 0.0f;
	xform(p0);
	xform(p1);

	return _pSystem->_pRd->_primit3.drawLine(p0, p1, nullptr, nullptr, nullptr, nullptr, color, 0xFFFFFFFF);
}

} // End of namespace Grim
