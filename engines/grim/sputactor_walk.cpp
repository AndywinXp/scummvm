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
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/walkplane.h"

namespace Grim {

static const int kMaxWalkPoints = 64;

static int clampPoints(int numOut, int count) {
	if (numOut + count > kMaxWalkPoints) {
		warning("SputActorModule::actorWalkToImpl(): the path is too long");
		return kMaxWalkPoints - numOut;
	}

	return count;
}

void SputActorModule::actorWalkTo(SputActor *pActor, float *pPos, float *pGoal, SputActor *pTarget) {
	pActor->mode &= ~1;

	Puck *pPuck = pActor->pExtra->puck;

	if (!pPuck || !_pWalkSystem->getHead())
		return;

	WalkPlane *pPlane = _pWalkSystem->planeClosest(pPos, 4096);

	if (pPlane)
		pPlane->closestPoint(pPos);

	if (_pWalkSystem->buildPath(pPuck->pos, pPos, 1)) {
		float *pBuiltPath;
		RdVector3 aBuilt[30];
		RdVector3 aWalkPath[kMaxWalkPoints];
		int numBuilt = _pWalkSystem->getPath(&pBuiltPath);

		if (numBuilt > 30) {
			warning("SputActorModule::actorWalkTo(): the built path has too many points");
			numBuilt = 30;
		}

		if (numBuilt > 0)
			memcpy(aBuilt, pBuiltPath, numBuilt * sizeof(RdVector3));

		int numPath = actorWalkToImpl(pActor, pTarget, aBuilt[0], numBuilt, aWalkPath[0]);

		if (numPath > 30) {
			warning("SputActorModule::actorWalkTo(): the walk path has too many points");
			numPath = 30;
		}

		pActor->pExtra->numPathPoints = numPath;
		pActor->pExtra->pathIndex = 1;

		if (numPath > 0)
			memcpy(pActor->pExtra->aPath, aWalkPath, numPath * sizeof(RdVector3));

		float *pFirstTarget = pActor->pExtra->aPath[pActor->pExtra->pathIndex];

		pActor->pExtra->walkTarget[0] = pFirstTarget[0];
		pActor->pExtra->walkTarget[1] = pFirstTarget[1];
		pActor->pExtra->walkTarget[2] = pFirstTarget[2];

		if (numPath > 1)
			pActor->mode |= 1;
	}

	pActor->pExtra->goal[0] = pGoal[0];
	pActor->pExtra->goal[1] = pGoal[1];
	pActor->pExtra->goal[2] = pGoal[2];
}

int SputActorModule::actorWalkToImpl(SputActor *pActor, SputActor *pTarget, float *pPath, int numPoints, float *pOutPath) {
	int numOut = 0;

	if (!pTarget) {
		if (numPoints > 0) {
			numPoints = clampPoints(0, numPoints);
			memcpy(pOutPath, pPath, numPoints * sizeof(RdVector3));
			numOut = numPoints;
		}

		return numOut;
	}

	if (numPoints <= 0 || !pActor->pCostumeStack || !pTarget->pCostumeStack) {
		warning("SputActorModule::actorWalkToImpl(): no path or no costume to walk around");
		return 0;
	}

	RdModel3 *pModel = pActor->pCostumeStack->pHeadThing->data.pModel3;
	float myRadius = (float)((double)pActor->pExtra->collisionScale * pModel->radius);
	RdModel3 *pTargetModel = pTarget->pCostumeStack->pHeadThing->data.pModel3;
	RdVector3 targetCenterWorld;
	RdVector3 targetCenterOffset;

	_pRd->transformPoint34(targetCenterWorld, pTargetModel->geomCenter, getOrientMatrix(pTarget));
	_pRd->transformVector34(targetCenterOffset, pTargetModel->geomCenter, getOrientMatrix(pTarget));

	float targetScale = pTarget->pExtra->collisionScale;
	RdBound3 targetBox;

	targetBox.min[0] = (float)((double)pTargetModel->bound.min[0] * targetScale - myRadius);
	targetBox.min[1] = (float)((double)pTargetModel->bound.min[1] * targetScale - myRadius);
	targetBox.min[2] = (float)((double)pTargetModel->bound.min[2] * targetScale - myRadius);
	targetBox.max[0] = (float)((double)pTargetModel->bound.max[0] * targetScale + myRadius);
	targetBox.max[1] = (float)((double)pTargetModel->bound.max[1] * targetScale + myRadius);
	targetBox.max[2] = (float)((double)pTargetModel->bound.max[2] * targetScale + myRadius);

	float *pLastInput = &pPath[3 * numPoints - 3];

	getOuttaTheBox(pLastInput, targetBox, getOrientMatrix(pTarget), pTarget->actorPos);
	getOuttaTheBox(pPath, targetBox, getOrientMatrix(pTarget), pTarget->actorPos);

	RdVector3 lastPoint;

	lastPoint[0] = pLastInput[0];
	lastPoint[1] = pLastInput[1];
	lastPoint[2] = pLastInput[2];

	if (!_pWalkSystem->planeAt(lastPoint, -1))
		return 0;

	int numSegments = numPoints - 1;
	int idx = 0;
	RdVector3 segFrom;
	RdVector3 segTo;
	bool bCollided = false;

	while (idx < numSegments) {
		float *pCurrent = &pPath[3 * idx];
		float *pNext = &pPath[3 * idx + 3];

		numOut++;
		segFrom[0] = (float)((double)targetCenterOffset[0] + pCurrent[0]);
		segFrom[1] = (float)((double)pCurrent[1] + targetCenterOffset[1]);
		segFrom[2] = (float)((double)targetCenterOffset[2] + pCurrent[2]);
		segTo[0] = (float)((double)pNext[0] + targetCenterOffset[0]);
		segTo[1] = (float)((double)pNext[1] + targetCenterOffset[1]);
		segTo[2] = (float)((double)pNext[2] + targetCenterOffset[2]);
		pOutPath[3 * (numOut - 1)] = pCurrent[0];
		pOutPath[3 * (numOut - 1) + 1] = pCurrent[1];
		pOutPath[3 * (numOut - 1) + 2] = pCurrent[2];

		if (testCollision(segFrom, segTo, targetCenterWorld, targetBox, getOrientMatrix(pTarget))) {
			bCollided = true;
			break;
		}

		idx++;
	}

	if (!bCollided) {
		memcpy(&pOutPath[3 * numOut], &pPath[3 * idx], sizeof(RdVector3));

		return numOut + 1;
	}

	RdVector3 snapped;

	_pRd->transformPoint34(snapped, _collisionImpact, getOrientMatrix(pTarget));

	WalkPlane *pPlane = _pWalkSystem->planeClosest(snapped, -1);

	if (!pPlane) {
		warning("SputActorModule::actorWalkToImpl(): no walk plane near the collision point");
		return numOut;
	}

	pPlane->snapTo(snapped);

	ObjectCorners corners;

	memset(&corners, 0, sizeof(corners));

	float margin = (float)((double)myRadius - -0.019999999552965164);

	findObjectCorners(pTargetModel, margin, getOrientMatrix(pTarget), snapped[2], &corners);

	for (int i = 0; i < 4; i++) {
		corners.aValid[i] = 0;
		segTo[0] = (float)((double)targetCenterOffset[0] + corners.aCorner[i][0]);
		segTo[1] = (float)((double)corners.aCorner[i][1] + targetCenterOffset[1]);
		segTo[2] = (float)((double)targetCenterOffset[2] + corners.aCorner[i][2]);

		if (!testCollision(segFrom, segTo, targetCenterWorld, targetBox, getOrientMatrix(pTarget)) && _pWalkSystem->buildPath(&pPath[3 * idx], corners.aCorner[i], 0))
			corners.aValid[i] = 1;
	}

	int closest = findClosestCandidate(segFrom, lastPoint, &corners);

	if (closest == -1)
		return numOut;

	float *pBuilt;

	_pWalkSystem->buildPath(&pPath[3 * idx], corners.aCorner[closest], 0);

	int numBuilt = _pWalkSystem->getPath(&pBuilt);

	if (numBuilt > 0) {
		numBuilt = clampPoints(numOut, numBuilt);
		memcpy(&pOutPath[3 * numOut], pBuilt, numBuilt * sizeof(RdVector3));
		numOut += numBuilt;
	}

	corners.aValid[closest] = 0;

	float *pLastOut = &pOutPath[3 * (numOut - 1)];

	if (checkCollisions(pLastOut, lastPoint, targetCenterWorld, targetBox, getOrientMatrix(pTarget), targetCenterOffset)) {
		numBuilt = _pWalkSystem->getPath(&pBuilt);

		if (numBuilt > 0) {
			numBuilt = clampPoints(numOut, numBuilt);
			memcpy(&pOutPath[3 * numOut], pBuilt, numBuilt * sizeof(RdVector3));
			numOut += numBuilt;
		}

		return numOut;
	}

	int ccwCorner = (closest + 3) % 4;
	int cwCorner = (closest + 5) % 4;
	int bCcwValid = _pWalkSystem->buildPath(pLastOut, corners.aCorner[ccwCorner], 0);
	int bCwValid = _pWalkSystem->buildPath(pLastOut, corners.aCorner[cwCorner], 0);

	if (bCwValid && bCcwValid) {
		float *pCcw = corners.aCorner[ccwCorner];
		float *pCw = corners.aCorner[cwCorner];

		double ccwDx = (double)pLastOut[0] - pCcw[0];
		float ccwDxF = (float)ccwDx;
		float ccwDy = (float)((double)pLastOut[1] - pCcw[1]);
		float ccwDz = (float)((double)pLastOut[2] - pCcw[2]);
		double ccwEndX = (double)pCcw[0] - lastPoint[0];
		float ccwEndXF = (float)ccwEndX;
		float ccwEndY = (float)((double)pCcw[1] - lastPoint[1]);
		float ccwEndZ = (float)((double)pCcw[2] - lastPoint[2]);
		double cwDx = (double)pLastOut[0] - pCw[0];
		float cwDxF = (float)cwDx;
		float cwDy = (float)((double)pLastOut[1] - pCw[1]);
		float cwDz = (float)((double)pLastOut[2] - pCw[2]);
		double cwEndX = (double)pCw[0] - lastPoint[0];
		float cwEndXF = (float)cwEndX;
		float cwEndY = (float)((double)pCw[1] - lastPoint[1]);
		double cwEndZ = (double)pCw[2] - lastPoint[2];

		double ccwCost = ccwDx * ccwDxF + (double)ccwDy * ccwDy + (double)ccwDz * ccwDz + ccwEndX * ccwEndXF + (double)ccwEndY * ccwEndY + (double)ccwEndZ * ccwEndZ;
		double cwCost = cwDx * cwDxF + (double)cwDy * cwDy + (double)cwDz * cwDz + cwEndX * cwEndXF + (double)cwEndY * cwEndY + cwEndZ * cwEndZ;

		if (cwCost <= ccwCost)
			bCcwValid = 0;
		else
			bCwValid = 0;
	}

	int cornerIdx;
	int step;

	if (bCwValid) {
		step = 1;
		cornerIdx = cwCorner;
	} else if (bCcwValid) {
		step = -1;
		cornerIdx = ccwCorner;
	} else {
		warning("SputActorModule::actorWalkToImpl(): no corner to walk around the target");
		return numOut;
	}

	if (cornerIdx == closest)
		return numOut;

	for (;;) {
		_pWalkSystem->buildPath(pLastOut, corners.aCorner[cornerIdx], 0);
		numBuilt = _pWalkSystem->getPath(&pBuilt);

		for (int i = 0; i < numBuilt - 1; i++) {
			segFrom[0] = (float)((double)targetCenterOffset[0] + pBuilt[3 * i]);
			segFrom[1] = (float)((double)pBuilt[3 * i + 1] + targetCenterOffset[1]);
			segFrom[2] = (float)((double)pBuilt[3 * i + 2] + targetCenterOffset[2]);
			segTo[0] = (float)((double)pBuilt[3 * i + 3] + targetCenterOffset[0]);
			segTo[1] = (float)((double)pBuilt[3 * i + 4] + targetCenterOffset[1]);
			segTo[2] = (float)((double)pBuilt[3 * i + 5] + targetCenterOffset[2]);

			if (testCollision(segFrom, segTo, targetCenterWorld, targetBox, getOrientMatrix(pTarget)))
				return 0;
		}

		if (numBuilt > 0) {
			numBuilt = clampPoints(numOut, numBuilt);
			memcpy(&pOutPath[3 * numOut], pBuilt, numBuilt * sizeof(RdVector3));
			numOut += numBuilt;
		}

		pLastOut = &pOutPath[3 * (numOut - 1)];

		if (checkCollisions(pLastOut, lastPoint, targetCenterWorld, targetBox, getOrientMatrix(pTarget), targetCenterOffset))
			break;

		cornerIdx = (step + cornerIdx + 4) % 4;

		if (cornerIdx == closest)
			return numOut;
	}

	numBuilt = _pWalkSystem->getPath(&pBuilt);

	for (int i = 0; i < numBuilt - 1; i++) {
		segFrom[0] = (float)((double)targetCenterOffset[0] + pBuilt[3 * i]);
		segFrom[1] = (float)((double)pBuilt[3 * i + 1] + targetCenterOffset[1]);
		segFrom[2] = (float)((double)pBuilt[3 * i + 2] + targetCenterOffset[2]);
		segTo[0] = (float)((double)pBuilt[3 * i + 3] + targetCenterOffset[0]);
		segTo[1] = (float)((double)pBuilt[3 * i + 4] + targetCenterOffset[1]);
		segTo[2] = (float)((double)pBuilt[3 * i + 5] + targetCenterOffset[2]);

		if (testCollision(segFrom, segTo, targetCenterWorld, targetBox, getOrientMatrix(pTarget)))
			return 0;
	}

	if (numBuilt > 0) {
		numBuilt = clampPoints(numOut, numBuilt);
		memcpy(&pOutPath[3 * numOut], pBuilt, numBuilt * sizeof(RdVector3));
		numOut += numBuilt;
	}

	return numOut;
}

} // End of namespace Grim
