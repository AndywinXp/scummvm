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

#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/walkplane.h"

namespace Grim {

void SputActorModule::getOuttaTheBox(float *pPoint, const RdBound3 &box, RdMatrix34 pMatrix, float *pOrigin) {
	RdVector3 local;

	local[0] = (float)((double)pPoint[0] - pOrigin[0]);
	local[1] = (float)((double)pPoint[1] - pOrigin[1]);
	local[2] = (float)((double)pPoint[2] - pOrigin[2]);

	RdVector3 out;

	_pRd->transformVectorOrtho34(out, local, pMatrix);

	if (!(out[2] > box.min[2] && out[1] > box.min[1] && out[1] < box.max[1] && out[0] > box.min[0] && out[0] < box.max[0] && out[2] < box.max[2]))
		return;

	double best = 3.4028234663852886e38;

	if (box.max[1] > out[1]) {
		double distance = (double)box.max[1] - out[1];

		if (distance < 0.0)
			distance = -distance;

		if (distance < best) {
			best = (float)distance;
			local[0] = out[0];
			local[2] = out[2];
			local[1] = (float)((double)box.max[1] - -0.009999999776482582);
		}
	}

	if (box.min[1] < out[1]) {
		double distance = (double)box.min[1] - out[1];

		if (distance < 0.0)
			distance = -distance;

		if (distance < best) {
			best = (float)distance;
			local[0] = out[0];
			local[2] = out[2];
			local[1] = (float)((double)box.min[1] - 0.009999999776482582);
		}
	}

	if (box.max[0] > out[0]) {
		double distance = (double)box.max[0] - out[0];

		if (distance < 0.0)
			distance = -distance;

		if (distance < best) {
			best = (float)distance;
			local[1] = out[1];
			local[2] = out[2];
			local[0] = (float)((double)box.max[0] - -0.009999999776482582);
		}
	}

	if (box.min[0] < out[0]) {
		double distance = (double)box.min[0] - out[0];

		if (distance < 0.0)
			distance = -distance;

		if (distance < best) {
			local[1] = out[1];
			local[2] = out[2];
			local[0] = (float)((double)box.min[0] - 0.009999999776482582);
		}
	}

	_pRd->transformPoint34(pPoint, local, pMatrix);
}

int SputActorModule::checkCollisions(float *pFrom, float *pTo, float *pCenter, const RdBound3 &box, RdMatrix34 pMatrix, float *pOffset) {
	RdVector3 offsetFrom;
	RdVector3 offsetTo;

	offsetFrom[0] = (float)((double)pOffset[0] + pFrom[0]);
	offsetFrom[1] = (float)((double)pFrom[1] + pOffset[1]);
	offsetFrom[2] = (float)((double)pFrom[2] + pOffset[2]);
	offsetTo[0] = (float)((double)pOffset[0] + pTo[0]);
	offsetTo[1] = (float)((double)pTo[1] + pOffset[1]);
	offsetTo[2] = (float)((double)pTo[2] + pOffset[2]);

	if (testCollision(offsetFrom, offsetTo, pCenter, box, pMatrix))
		return 0;

	return _pWalkSystem->buildPath(pFrom, pTo, 0);
}

void SputActorModule::findObjectCorners(RdModel3 *pModel, float margin, RdMatrix34 mat, float z, ObjectCorners *pOut) {
	if (mat[2][2] < 0.999)
		return;

	pOut->aCorner[0][0] = pModel->bound.min[0] - margin;
	pOut->aCorner[0][1] = pModel->bound.min[1] - margin;
	pOut->aCorner[0][2] = pModel->bound.min[2];
	pOut->aCorner[1][0] = margin + pModel->bound.max[0];
	pOut->aCorner[1][1] = pModel->bound.min[1] - margin;
	pOut->aCorner[1][2] = pModel->bound.min[2];
	pOut->aCorner[2][0] = margin + pModel->bound.max[0];
	pOut->aCorner[2][1] = margin + pModel->bound.max[1];
	pOut->aCorner[2][2] = pModel->bound.min[2];
	pOut->aCorner[3][0] = pModel->bound.min[0] - margin;
	pOut->aCorner[3][1] = margin + pModel->bound.max[1];
	pOut->aCorner[3][2] = pModel->bound.min[2];

	for (int i = 0; i < 4; i++) {
		_pRd->transformPoint34Acc(pOut->aCorner[i], mat);
		pOut->aCorner[i][2] = z;
		pOut->aValid[i] = 1;
	}
}

int SputActorModule::testCollision(float *pFrom, float *pTo, float *pCenter, const RdBound3 &box, RdMatrix34 pMatrix) {
	RdVector3 local;
	RdVector3 start;
	RdVector3 end;

	local[0] = (float)((double)pFrom[0] - pCenter[0]);
	local[1] = (float)((double)pFrom[1] - pCenter[1]);
	local[2] = (float)((double)pFrom[2] - pCenter[2]);
	_pRd->transformVectorOrtho34(start, local, pMatrix);

	local[0] = (float)((double)pTo[0] - pCenter[0]);
	local[1] = (float)((double)pTo[1] - pCenter[1]);
	local[2] = (float)((double)pTo[2] - pCenter[2]);
	_pRd->transformVectorOrtho34(end, local, pMatrix);

	float dir[3];

	dir[0] = (float)((double)end[0] - start[0]);
	dir[1] = (float)((double)end[1] - start[1]);
	dir[2] = (float)((double)end[2] - start[2]);

	float best = 1.0f;

	if (start[1] > box.max[1] && end[1] < box.max[1]) {
		double t = ((double)box.max[1] - start[1]) / dir[1];
		float tF = (float)t;

		if (t < 1.0) {
			double xD = (double)tF * dir[0] + start[0];
			float x = (float)xD;
			float y = (float)((double)tF * dir[1] + start[1]);
			float z = (float)((double)tF * dir[2] + start[2]);

			if (xD > box.min[0] && x < box.max[0]) {
				_tmpPos0[0] = x;
				_tmpPos0[1] = y;
				_tmpPos0[2] = z;
				best = tF;
			}
		}
	}

	if (start[1] < box.min[1] && end[1] > box.min[1]) {
		double t = ((double)box.min[1] - start[1]) / dir[1];
		float tF = (float)t;

		if (t < best) {
			double xD = (double)tF * dir[0] + start[0];
			float x = (float)xD;
			float y = (float)((double)tF * dir[1] + start[1]);
			float z = (float)((double)tF * dir[2] + start[2]);

			if (xD > box.min[0] && x < box.max[0]) {
				_tmpPos0[0] = x;
				_tmpPos0[1] = y;
				_tmpPos0[2] = z;
				best = tF;
			}
		}
	}

	if (start[0] > box.max[0] && end[0] < box.max[0]) {
		double t = ((double)box.max[0] - start[0]) / dir[0];
		float tF = (float)t;

		if (t < best) {
			double yD = (double)tF * dir[1] + start[1];
			float y = (float)yD;
			float x = (float)((double)tF * dir[0] + start[0]);
			float z = (float)((double)tF * dir[2] + start[2]);

			if (yD > box.min[1] && y < box.max[1]) {
				_tmpPos0[0] = x;
				_tmpPos0[1] = y;
				_tmpPos0[2] = z;
				best = tF;
			}
		}
	}

	if (start[0] < box.min[0] && end[0] > box.min[0]) {
		double t = ((double)box.min[0] - start[0]) / dir[0];
		float tF = (float)t;

		if (t < best) {
			double yD = (double)tF * dir[1] + start[1];
			float y = (float)yD;
			float x = (float)((double)tF * dir[0] + start[0]);
			float z = (float)((double)tF * dir[2] + start[2]);

			if (yD > box.min[1] && y < box.max[1]) {
				_tmpPos0[0] = x;
				_tmpPos0[1] = y;
				_tmpPos0[2] = z;
				best = tF;
			}
		}
	}

	return best != 1.0f;
}

int SputActorModule::findClosestCandidate(float *pFrom, float *pTo, ObjectCorners *pCorners) {
	int closest = -1;
	float best = 3.4028235e38f;

	for (int i = 0; i < 4; i++) {
		if (!pCorners->aValid[i])
			continue;

		float *pCorner = pCorners->aCorner[i];
		double dx1 = (double)pFrom[0] - pCorner[0];
		double dy1 = (double)pFrom[1] - pCorner[1];
		double dy2 = (double)pCorner[1] - pTo[1];
		double dz2 = (double)pCorner[2] - pTo[2];
		double dz1 = (double)pFrom[2] - pCorner[2];
		double dx2 = (double)pCorner[0] - pTo[0];
		float dx1F = (float)dx1;
		float dy1F = (float)dy1;
		float dz1F = (float)dz1;
		float dy2F = (float)dy2;
		float dz2F = (float)dz2;

		double distance = ((dx1 * dx1F + dy1 * dy1F) + dz1 * dz1F) + ((dy2 * dy2F + dz2 * dz2F) + dx2 * dx2);

		if (distance < best) {
			best = (float)distance;
			closest = i;
		}
	}

	return closest;
}

int SputActorModule::sweepIntoBox(float *pFrom, float *pTo, const RdBound3 &box, float *pOutPos) {
	int face = 0;
	double dx = (double)pFrom[0] - pTo[0];
	float dxF = (float)dx;
	double dy = (double)pFrom[1] - pTo[1];
	float dyF = (float)dy;
	double dz = (double)pFrom[2] - pTo[2];
	float toX = (float)(dx * -10.0 + pTo[0]);
	float toY = (float)(dy * -10.0 + pTo[1]);
	float toZ = (float)(dz * -10.0 + pTo[2]);

	pTo[0] = toX;
	pTo[1] = toY;
	pTo[2] = toZ;

	double best = 3.4028234663852886e38;
	RdVector3 normal = { 0.0f, 0.0f, 0.0f };

	if ((double)box.max[0] - 0.002 < toX) {
		double denominator = dxF < 0.0f ? -dxF : dxF;
		double t = ((double)box.max[0] - pFrom[0]) / denominator;
		float tF = (float)t;
		double tAbs = t < 0.0 ? -t : t;

		if (tAbs < 3.4028234663852886e38) {
			best = -tF;
			face = 1;
			normal[0] = 1.0f;
			normal[1] = 0.0f;
			normal[2] = 0.0f;
		}
	}

	if ((double)box.min[0] - -0.002 > toX) {
		double denominator = dxF < 0.0f ? -dxF : dxF;
		double t = ((double)box.min[0] - pFrom[0]) / denominator;
		float tF = (float)t;
		double tAbs = t < 0.0 ? -t : t;
		double bestAbs = best < 0.0 ? -best : best;

		if (tAbs < bestAbs) {
			best = tF;
			face = 2;
			normal[0] = -1.0f;
			normal[1] = 0.0f;
			normal[2] = 0.0f;
		}
	}

	if ((double)box.max[1] - 0.002 < toY) {
		double denominator = dyF < 0.0f ? -dyF : dyF;
		double t = ((double)box.max[1] - pFrom[1]) / denominator;
		float tF = (float)t;
		double tAbs = t < 0.0 ? -t : t;
		double bestAbs = best < 0.0 ? -best : best;

		if (tAbs < bestAbs) {
			best = -tF;
			face = 3;
			normal[0] = 0.0f;
			normal[1] = 1.0f;
			normal[2] = 0.0f;
		}
	}

	if ((double)box.min[1] - -0.002 > toY) {
		double denominator = dyF < 0.0f ? -dyF : dyF;
		double t = ((double)box.min[1] - pFrom[1]) / denominator;
		float tF = (float)t;
		double tAbs = t < 0.0 ? -t : t;
		double bestAbs = best < 0.0 ? -best : best;

		if (tAbs < bestAbs) {
			best = tF;
			face = 4;
			normal[0] = 0.0f;
			normal[1] = -1.0f;
			normal[2] = 0.0f;
		}
	}

	if ((double)box.max[2] - 0.002 < toZ) {
		double denominator = dz < 0.0 ? -dz : dz;
		double t = ((double)box.max[2] - pFrom[2]) / denominator;
		float tF = (float)t;
		double tAbs = t < 0.0 ? -t : t;
		double bestAbs = best < 0.0 ? -best : best;

		if (tAbs < bestAbs) {
			best = -tF;
			face = 5;
			normal[0] = 0.0f;
			normal[1] = 0.0f;
			normal[2] = 1.0f;
		}
	}

	if ((double)box.min[2] - -0.002 > toZ) {
		double denominator = dz < 0.0 ? -dz : dz;
		double t = ((double)box.min[2] - pFrom[2]) / denominator;
		float tF = (float)t;
		double tAbs = t < 0.0 ? -t : t;
		double bestAbs = best < 0.0 ? -best : best;

		if (tAbs < bestAbs) {
			best = tF;
			face = 6;
			normal[0] = 0.0f;
			normal[1] = 0.0f;
			normal[2] = -1.0f;
		}
	}

	if (!face)
		return 0;

	pOutPos[0] = (float)(best * dxF + pFrom[0]);
	pOutPos[1] = (float)(best * dyF + pFrom[1]);
	pOutPos[2] = (float)(best * dz + pFrom[2]);
	_tmpPos0[0] = pOutPos[0];
	_tmpPos0[1] = pOutPos[1];
	_tmpPos0[2] = pOutPos[2];

	double bestAbs = best < 0.0 ? -best : best;
	RdVector3 sweep;

	sweep[0] = (float)(dxF * bestAbs);
	sweep[1] = (float)(dyF * bestAbs);
	sweep[2] = (float)(dz * bestAbs);

	float length = _pRd->normalize3Acc(sweep);
	float dotValue = _pRd->dot3(sweep, normal);
	double angle = 90.0 - _pStdMath->arcSin3(dotValue);
	float angleF = (float)angle;

	if (angle < 90.0)
		return 0;

	RdVector3 axis;

	axis[0] = (float)((double)normal[2] * sweep[1] - (double)normal[1] * sweep[2]);
	axis[1] = (float)((double)normal[0] * sweep[2] - (double)normal[2] * sweep[0]);
	axis[2] = (float)((double)normal[1] * sweep[0] - (double)normal[0] * sweep[1]);
	_pRd->normalize3Acc(axis);

	float turnAngle = (float)((double)angleF - 90.0);
	RdMatrix34 turnMatrix;

	_pRd->buildFromVectorAngle34(turnMatrix, axis, turnAngle);
	_pRd->transformVector34Acc(sweep, turnMatrix);

	pOutPos[0] = (float)((double)length * sweep[0] + pOutPos[0]);
	pOutPos[1] = (float)((double)length * sweep[1] + pOutPos[1]);
	pOutPos[2] = (float)((double)length * sweep[2] + pOutPos[2]);

	return 1;
}

void SputActorModule::updatePositions() {
	for (int i = 0; i < _numCollisionActors; i++) {
		SputActor *pActor = _apCollisionActors[i];

		if (memcmp(pActor->actorPos, pActor->prevPosition, sizeof(RdVector3)))
			updatePosition(pActor);
	}
}

void SputActorModule::updatePosition(SputActor *pActor) {
	RdModel3 *pModel3 = pActor->pCostumeStack->pHeadThing->data.pModel3;
	RdVector3 myCenterOffset;

	_pRd->transformVector34(myCenterOffset, pModel3->geomCenter, getOrientMatrix(pActor));

	RdVector3 myCenterWorld;

	myCenterWorld[0] = (float)((double)myCenterOffset[0] + pActor->actorPos[0]);
	myCenterWorld[1] = (float)((double)pActor->actorPos[1] + myCenterOffset[1]);
	myCenterWorld[2] = (float)((double)pActor->actorPos[2] + myCenterOffset[2]);

	float myRadius = (float)((double)pModel3->radius * pActor->pExtra->collisionScale);

	for (int i = 0; i < _numCollisionActors; i++) {
		SputActor *pOther = _apCollisionActors[i];

		if (pOther == pActor)
			continue;

		if (!pOther->pExtra || !pOther->pExtra->collisionMode)
			continue;

		RdModel3 *pOtherModel3 = pOther->pCostumeStack->pHeadThing->data.pModel3;
		RdVector3 otherCenterWorld;

		_pRd->transformPoint34(otherCenterWorld, pOtherModel3->geomCenter, getOrientMatrix(pOther));

		double deltaX = (double)myCenterWorld[0] - otherCenterWorld[0];
		double deltaY = (double)myCenterWorld[1] - otherCenterWorld[1];
		double deltaZ = (double)myCenterWorld[2] - otherCenterWorld[2];
		float deltaXF = (float)deltaX;
		float deltaYF = (float)deltaY;
		float deltaZF = (float)deltaZ;
		double distance = sqrt((deltaX * deltaXF + deltaY * deltaY) + deltaZ * deltaZF);
		double otherRadius = (double)pOtherModel3->radius * pOther->pExtra->collisionScale;
		float otherRadiusF = (float)otherRadius;
		float distanceF = (float)distance;

		if (!(distance < otherRadius + myRadius))
			continue;

		if (pOther->pExtra->collisionMode == 2) {
			pActor->pExtra->field_3F8 = 1;
			pOther->pExtra->field_3F8 = 1;

			RdVector3 direction;

			direction[0] = (float)((double)myCenterWorld[0] - otherCenterWorld[0]);
			direction[1] = (float)((double)myCenterWorld[1] - otherCenterWorld[1]);
			direction[2] = (float)((double)myCenterWorld[2] - otherCenterWorld[2]);
			_tmpPos2[0] = pActor->actorPos[0];
			_tmpPos2[1] = pActor->actorPos[1];
			_tmpPos2[2] = pActor->actorPos[2];
			_pRd->normalize3Acc(direction);

			float ratio = (float)(((double)distanceF - otherRadiusF) / myRadius);
			float pushOut = (float)((1.0 - ratio) * myRadius);
			RdVector3 newPos;

			newPos[0] = (float)(((double)direction[0] * pushOut + myCenterWorld[0]) - myCenterOffset[0]);
			newPos[1] = (float)(((double)direction[1] * pushOut + myCenterWorld[1]) - myCenterOffset[1]);
			newPos[2] = (float)(((double)direction[2] * pushOut + myCenterWorld[2]) - myCenterOffset[2]);

			int bWalking = (pActor->mode & 1) != 0;

			setActorPosition(pActor, newPos);
			_tmpPos1[0] = pActor->actorPos[0];
			_tmpPos1[1] = pActor->actorPos[1];
			_tmpPos1[2] = pActor->actorPos[2];
			_tmpPos0[0] = pActor->actorPos[0];
			_tmpPos0[1] = pActor->actorPos[1];
			_tmpPos0[2] = pActor->actorPos[2];

			if (bWalking)
				pActor->mode |= 1;
		} else {
			RdVector3 delta;

			delta[0] = deltaXF;
			delta[1] = deltaYF;
			delta[2] = deltaZF;

			RdVector3 localPrev;

			_pRd->transformVectorOrtho34(localPrev, delta, getOrientMatrix(pOther));
			localPrev[0] = (float)((double)localPrev[0] + pOtherModel3->geomCenter[0]);
			localPrev[1] = (float)((double)localPrev[1] + pOtherModel3->geomCenter[1]);
			localPrev[2] = (float)((double)localPrev[2] + pOtherModel3->geomCenter[2]);

			RdBound3 otherBox;
			float otherScale = pOther->pExtra->collisionScale;

			otherBox.min[0] = (float)((double)pOtherModel3->bound.min[0] * otherScale - myRadius);
			otherBox.min[1] = (float)((double)pOtherModel3->bound.min[1] * otherScale - myRadius);
			otherBox.min[2] = (float)((double)pOtherModel3->bound.min[2] * otherScale - myRadius);
			otherBox.max[0] = (float)((double)pOtherModel3->bound.max[0] * otherScale + myRadius);
			otherBox.max[1] = (float)((double)pOtherModel3->bound.max[1] * otherScale + myRadius);
			otherBox.max[2] = (float)((double)pOtherModel3->bound.max[2] * otherScale + myRadius);

			if (!(localPrev[2] > otherBox.min[2] && localPrev[1] > otherBox.min[1] && localPrev[1] < otherBox.max[1] && localPrev[0] > otherBox.min[0] &&
					localPrev[0] < otherBox.max[0] && localPrev[2] < otherBox.max[2]))
				continue;

			pActor->pExtra->field_3F8 = 1;
			_tmpPos2[0] = pActor->actorPos[0];
			_tmpPos2[1] = pActor->actorPos[1];
			_tmpPos2[2] = pActor->actorPos[2];

			RdVector3 prevDelta;

			prevDelta[0] = (float)(((double)myCenterOffset[0] + pActor->prevPosition[0]) - otherCenterWorld[0]);
			prevDelta[1] = (float)((double)pActor->prevPosition[1] + myCenterOffset[1]);
			prevDelta[2] = (float)((double)pActor->prevPosition[2] + myCenterOffset[2]);
			prevDelta[1] = (float)((double)prevDelta[1] - otherCenterWorld[1]);
			prevDelta[2] = (float)((double)prevDelta[2] - otherCenterWorld[2]);

			RdVector3 localCur;

			_pRd->transformVectorOrtho34(localCur, prevDelta, getOrientMatrix(pOther));
			localCur[0] = (float)((double)localCur[0] + pOtherModel3->geomCenter[0]);
			localCur[1] = (float)((double)localCur[1] + pOtherModel3->geomCenter[1]);
			localCur[2] = (float)((double)localCur[2] + pOtherModel3->geomCenter[2]);

			RdVector3 hitPos;

			if (!sweepIntoBox(localPrev, localCur, otherBox, hitPos)) {
				getOuttaTheBox(localPrev, otherBox, getOrientMatrix(pOther), pOther->actorPos);
				hitPos[0] = localPrev[0];
				hitPos[1] = localPrev[1];
				hitPos[2] = localPrev[2];
			}

			_pRd->transformPoint34Acc(hitPos, getOrientMatrix(pOther));
			hitPos[0] = (float)((double)hitPos[0] - myCenterOffset[0]);
			hitPos[1] = (float)((double)hitPos[1] - myCenterOffset[1]);
			hitPos[2] = (float)((double)hitPos[2] - myCenterOffset[2]);
			pActor->prevPosition[0] = pActor->actorPos[0];
			pActor->prevPosition[1] = pActor->actorPos[1];
			pActor->prevPosition[2] = pActor->actorPos[2];

			_pRd->transformPoint34Acc(_tmpPos0, getOrientMatrix(pOther));
			_tmpPos0[0] = (float)((double)_tmpPos0[0] - myCenterOffset[0]);
			_tmpPos0[1] = (float)((double)_tmpPos0[1] - myCenterOffset[1]);
			_tmpPos0[2] = (float)((double)_tmpPos0[2] - myCenterOffset[2]);

			int bWalking = (pActor->mode & 1) != 0;

			setActorPosition(pActor, hitPos);
			_tmpPos1[0] = pActor->actorPos[0];
			_tmpPos1[1] = pActor->actorPos[1];
			_tmpPos1[2] = pActor->actorPos[2];

			if (bWalking) {
				pActor->mode |= 1;

				if (pActor->pExtra->numPathPoints > 0 && pActor->pExtra->numPathPoints <= 30) {
					RdVector3 goalPoint;
					float *pLastPathPoint = pActor->pExtra->aPath[pActor->pExtra->numPathPoints - 1];

					goalPoint[0] = pLastPathPoint[0];
					goalPoint[1] = pLastPathPoint[1];
					goalPoint[2] = pLastPathPoint[2];
					actorWalkTo(pActor, goalPoint, pActor->pExtra->goal, pOther);
				} else {
					warning("SputActorModule::updatePosition(): the actor has no path to rebuild");
				}
			}
		}

		// TODO: sputLua_CollisionHandler(pActor, pOther, _tmpPos2, _tmpPos0, _tmpPos1)
	}
}

} // End of namespace Grim
