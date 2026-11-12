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

#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/puck.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtime.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/walkplane.h"

namespace Grim {

int SputActorModule::canActorSeePoint(SputActor *pActor, float *pPoint, float *pDistance) {
	CostumeNode *pNode = pActor->pCostumeStack;

	if (!pNode)
		return 0;

	if (pNode->aHeadJoint[0] < 0)
		return 0;

	if (!pNode->pHeadThing)
		return 0;

	RdMatrix34 meshMatrix;

	_pRd->_model3.getMeshMatrix(pNode->pHeadThing, getOrientMatrix(pActor), pNode->aHeadJoint[0], meshMatrix);

	RdVector3 direction;

	direction[0] = pPoint[0] - meshMatrix[3][0];
	direction[1] = pPoint[1] - meshMatrix[3][1];
	direction[2] = pPoint[2] - meshMatrix[3][2];
	*pDistance = _pRd->normalize3Acc(direction);

	RdVector3 flat;

	flat[0] = direction[0];
	flat[1] = direction[1];
	flat[2] = 0.0f;
	_pRd->normalize3Acc(flat);

	float xTerm = (float)((double)getOrientMatrix(pActor)[1][0] * flat[0]);
	float xyTerm = (float)((double)getOrientMatrix(pActor)[1][1] * flat[1] + xTerm);
	double dot = (double)getOrientMatrix(pActor)[1][2] * flat[2] + xyTerm;

	return !(dot < pNode->headLookRef);
}

int SputActorModule::canActorSeeActor(SputActor *pActor, SputActor *pTarget, float *pDistance) {
	if (!pTarget->curSetName[0] || !pActor->curSetName[0] || scumm_stricmp(pTarget->curSetName, pActor->curSetName))
		return 0;

	return canActorSeePoint(pActor, getOrientMatrix(pTarget)[3], pDistance);
}

void SputActorModule::turn(SputActor *pActor, float dir) {
	Extra *pExtra = pActor->pExtra;
	float frameTime = _pSputTime->getFrameTimeFlex();
	float scaledTime = (float)((double)frameTime * pExtra->timeScale);
	float turnAmount = (float)(((double)pExtra->turnRate * scaledTime) * dir);

	if (!pExtra->puck)
		enablePuck(pActor, 1);

	pExtra = pActor->pExtra;

	double alignAbs = pExtra->alignOff;

	if (alignAbs < 0.0)
		alignAbs = -alignAbs;

	double doubleTurn = (double)turnAmount + turnAmount;

	if (doubleTurn < 0.0)
		doubleTurn = -doubleTurn;

	if (!(alignAbs > doubleTurn)) {
		if (!(dir > 0.0f))
			pExtra->turnDir = -1;
		else
			pExtra->turnDir = 1;
	}
}

void SputActorModule::forward(SputActor *pActor) {
	if (!pActor->pExtra->puck)
		enablePuck(pActor, 1);

	pActor->pExtra->bWalking = 1;
}

void SputActorModule::updateActors(float delta) {
	_numCollisionActors = 0;

	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom)
		return;

	updateSpeechOnly(delta);

	RoomActorLink *pLink = pRoom->pActorNode->pActors;

	if (!pLink)
		return;

	do {
		SputActor *pActor = &_aActors[pLink->actorIndex];

		if (pActor->pExtra)
			pActor->pExtra->collided = 0;

		if (pActor->pCostumeStack && pActor->pCostumeStack->pHeadThing) {
			if (_numCollisionActors < 256) {
				_apCollisionActors[_numCollisionActors] = pActor;
				_numCollisionActors++;
			} else {
				warning("SputActorModule::updateActors(): too many actors to collide");
			}
		}

		updateCostumesMain(pActor, delta);
		pLink = pLink->pNext;
	} while (pLink);

	updatePositions();
}

void SputActorModule::updateActorsDemo(float delta) {
	_numCollisionActors = 0;

	for (int i = 0; i < _numActors; i++) {
		SputActor *pActor = &_aActors[i];

		if (!pActor->bInUse)
			continue;

		if (!scumm_stricmp(pActor->curSetName, _pSputRoom->getCurrentRoomName()) || (pActor->pExtra && pActor->pExtra->talking))
			updateCostumesMainDemo(pActor, delta);
		else
			stopActorDemo(pActor);
	}
}

void SputActorModule::updateCostumesMainDemo(SputActor *pActor, float delta) {
	CostumeNode *pNode = pActor->pCostumeStack;

	if (pNode && pNode->rCostume.valid)
		_pCos->instRescue(pNode->hCostumeInstance, pNode->rCostume.serial);

	if (pActor->mode & 1)
		updatePuckMovement(pActor);

	if (pActor->pExtra && pActor->pExtra->puck)
		updatePuck(pActor);

	updateCostumes4(pActor);
	_pSputCostume->update(pActor->pCostumeStack, delta);

	if (pActor->mode & 2)
		updateCostumes2(pActor, delta);

	Extra *pExtra = pActor->pExtra;

	if (!pExtra || !pExtra->talking)
		return;

	// The demo has no lip sync data, only the timer and the voice end the line
	pExtra->talkTimerMs += (int)(int64)((double)delta * -1000.0);

	if (pExtra->voiceName[0] && !isVoicePlayingDemo(pActor))
		shutUpDemo(pActor);

	// Both checks can run, the count of talking actors is lowered twice then, as in the EXE
	if (!pExtra->voiceName[0] && pExtra->talkTimerMs < 0)
		shutUpDemo(pActor);
}

void SputActorModule::updateCostumesMain(SputActor *pActor, float delta) {
	if (pActor->pExtra)
		delta = (float)((double)pActor->pExtra->timeScale * delta);

	CostumeNode *pNode = pActor->pCostumeStack;

	pActor->prevPosition[0] = pActor->actorPos[0];
	pActor->prevPosition[1] = pActor->actorPos[1];
	pActor->prevPosition[2] = pActor->actorPos[2];

	if (pNode && pNode->rCostume.valid)
		_pCos->instRescue(pNode->hCostumeInstance, pNode->rCostume.serial);

	if (pActor->mode & 1)
		updatePuckMovement(pActor);

	if (pActor->pExtra && pActor->pExtra->puck)
		updatePuck(pActor);

	updateCostumes4(pActor);
	_pSputCostume->update(pActor->pCostumeStack, delta);

	if (pActor->mode & 2)
		updateCostumes2(pActor, delta);

	if (pActor->flags & 0x10)
		updateCostumes3(pActor);
}

void SputActorModule::updateCostumes2(SputActor *pActor, float delta) {
	CostumeNode *pNode = pActor->pCostumeStack;

	if (!pActor->pExtra) {
		warning("SputActorModule::updateCostumes2(): the actor has no extra");
		return;
	}

	if (!pNode || pNode->aHeadJoint[0] < 0 || !pNode->pHeadThing)
		return;

	int bLookAhead = 0;
	RdVector3 up;

	up[0] = 0.0f;
	up[1] = 1.0f;
	up[2] = 0.0f;

	// The demo uses the joint matrices of the last frame
	if (!pActor->onScreen && !(_pGrim->getGameFlags() & ADGF_DEMO))
		_pRd->_puppet.buildJointMatrices(pNode->pHeadThing, getOrientMatrix(pActor));

	RdMatrix34 headMatrix;
	RdMatrix34 inverseMatrix;

	_pRd->_model3.getMeshMatrix(pNode->pHeadThing, getOrientMatrix(pActor), pNode->aHeadJoint[2], headMatrix);
	_pRd->invertOrtho34(inverseMatrix, headMatrix);

	Extra *pExtra = pActor->pExtra;
	RdVector3 target;

	if (pExtra->lookCarrot[0] == 0.0f && pExtra->lookCarrot[1] == 0.0f && pExtra->lookCarrot[2] == 0.0f) {
		bLookAhead = 1;
		_pRd->transformVector34(target, getOrientMatrix(pActor)[1], inverseMatrix);

		if (g_grim->useRemasterFixes())
			_pRd->normalize3Acc(target);
	} else {
		_pRd->transformPoint34(target, pExtra->lookCarrot, inverseMatrix);
		_pRd->normalize3Acc(target);
	}

	RdVector3 axis;

	_pRd->cross3(axis, up, target);
	_pRd->normalize3Acc(axis);

	float turnLimit = (float)((double)pNode->headTurnRate * delta);
	float dotValue = _pRd->dot2(up, target);
	double angle = 90.0 - _pStdMath->arcSin2(dotValue);
	double absAngle = angle;

	if (absAngle < 0.0)
		absAngle = -absAngle;

	if (absAngle < turnLimit) {
		if (bLookAhead)
			pActor->mode &= ~2;

		if (angle <= 0.0)
			angle = -angle;

		turnLimit = (float)angle;
	}

	RdMatrix34 turnMatrix;
	RdVector3 pitchYawRoll;

	_pRd->buildFromVectorAngle34(turnMatrix, axis, turnLimit);
	_pRd->transformPoint34Acc(up, turnMatrix);
	_pRd->extractAngles(up, pitchYawRoll);

	RdThing *pHeadThing = pNode->pHeadThing;

	if (pActor->mode & 2) {
		pitchYawRoll[1] = (float)(pitchYawRoll[1] * 0.3333333432674408);
		pitchYawRoll[0] = (float)(pitchYawRoll[0] * 0.3333333432674408);

		double pitchLimit = pNode->headPitchLimit * 0.3333333432674408;
		double yawLimit = pNode->headYawLimit * 0.3333333432674408;

		for (int i = 0; i < 3; i++) {
			float *pJointAngles = pHeadThing->aJointPyr[pNode->aHeadJoint[i]];

			pJointAngles[1] = (float)(pitchYawRoll[1] + pJointAngles[1]);
			pJointAngles[0] = (float)(pitchYawRoll[0] + pJointAngles[0]);
			pJointAngles[2] = 0.0f;

			double value = pJointAngles[1];

			if (value < -pitchLimit)
				value = -pitchLimit;
			else if (value > pitchLimit)
				value = pitchLimit;

			pJointAngles[1] = (float)value;

			value = pJointAngles[0];

			if (value < -yawLimit)
				value = -yawLimit;
			else if (value > yawLimit)
				value = yawLimit;

			pJointAngles[0] = (float)value;
		}

		pHeadThing->jointsFrame = 0;
	} else {
		for (int i = 0; i < 3; i++) {
			float *pJointAngles = pHeadThing->aJointPyr[pNode->aHeadJoint[i]];

			pJointAngles[1] = 0.0f;
			pJointAngles[0] = 0.0f;
			pJointAngles[2] = 0.0f;
		}

		pHeadThing->jointsFrame = 0;
	}
}

void SputActorModule::updateCostumes3(SputActor *pActor) {
	if (!pActor || !pActor->pExtra || !pActor->pExtra->puck)
		return;

	Puck *pPuck = pActor->pExtra->puck;

	_pRd->build34(pActor->orientMatrix, pActor->actorRot, pActor->actorPos);

	RdVector3 actorForward;
	RdVector3 puckForward;

	actorForward[0] = pActor->orientMatrix[2][0];
	actorForward[1] = pActor->orientMatrix[2][1];
	actorForward[2] = pActor->orientMatrix[2][2];
	puckForward[0] = pPuck->orient[2][0];
	puckForward[1] = pPuck->orient[2][1];
	puckForward[2] = pPuck->orient[2][2];

	float dotValue = _pRd->dot3(actorForward, puckForward);
	double angle = 90.0 - _pStdMath->arcSin3(dotValue);
	float angleF = (float)angle;
	double absAngle = angle;

	if (absAngle < 0.0)
		absAngle = -absAngle;

	if (!(absAngle > 2.0))
		return;

	float frameTime = _pSputTime->getFrameTimeFlex();
	double rate = (double)frameTime * pActor->pExtra->turnRate;

	if (angleF > 0.0f && rate < angleF)
		angleF = (float)rate;

	if (angleF < 0.0f) {
		double negRate = -rate;

		if (negRate > angleF)
			angleF = (float)negRate;
	}

	RdVector3 axis;

	axis[0] = (float)((double)puckForward[2] * actorForward[1] - (double)puckForward[1] * actorForward[2]);
	axis[1] = (float)((double)puckForward[0] * actorForward[2] - (double)puckForward[2] * actorForward[0]);
	axis[2] = (float)((double)puckForward[1] * actorForward[0] - (double)puckForward[0] * actorForward[1]);
	_pRd->normalize3Acc(axis);

	RdMatrix34 turnMatrix;

	_pRd->buildFromVectorAngle34(turnMatrix, axis, angleF);
	_pRd->postMultiply34(pActor->orientMatrix, turnMatrix);
	_pRd->extractAngles34(pActor->orientMatrix, pActor->actorRot);
	_pRd->build34(pActor->orientMatrix, pActor->actorRot, pActor->actorPos);
}

void SputActorModule::updatePuck(SputActor *pActor) {
	Extra *pExtra = pActor->pExtra;
	Puck *pPuck = pExtra->puck;

	if (pExtra->turnDir && !(pActor->mode & 1)) {
		float frameTime = _pSputTime->getFrameTimeFlex();
		float scaledTime = (float)((double)frameTime * pExtra->timeScale);
		float turnAmount = (float)(((double)pExtra->turnRate * scaledTime) * (double)pExtra->turnDir);

		pExtra->puck->rotate(turnAmount);
	}

	if (pActor->pExtra->bWalking) {
		float frameTime = _pSputTime->getFrameTimeFlex();
		Extra *pWalkExtra = pActor->pExtra;
		float distance = (float)(((double)frameTime * pWalkExtra->timeScale) * pWalkExtra->walkRate);
		int bWalking;

		if (pWalkExtra->walkDominate)
			bWalking = pWalkExtra->puck->forward(distance, 1, 1);
		else
			bWalking = pWalkExtra->puck->forward(distance, pWalkExtra->turnDir != -1, pWalkExtra->turnDir != 1);

		pActor->pExtra->bWalking = bWalking;
	}

	pActor->pExtra->aligned = alignToPuck(pActor, pPuck);

	Extra *pLookExtra = pActor->pExtra;

	if (!pLookExtra->turnDir && !pLookExtra->aligned) {
		if (!(pLookExtra->alignOff > 0.0f))
			pLookExtra->turnDir = 1;
		else
			pLookExtra->turnDir = -1;
	}

	// The demo clears the pitch and the roll of the actor
	if (_pGrim->getGameFlags() & ADGF_DEMO) {
		pActor->actorRot[0] = 0.0f;
		pActor->actorRot[2] = 0.0f;
	}

	pActor->actorPos[0] = pPuck->pos[0];
	pActor->actorPos[1] = pPuck->pos[1];
	pActor->actorPos[2] = pPuck->pos[2];
	pActor->matrixValid = 0;
}

int SputActorModule::alignToPuck(SputActor *pActor, Puck *pPuck) {
	float *pForward = getOrientMatrix(pActor)[1];
	RdVector3 vec;

	vec[0] = pForward[0];
	vec[1] = pForward[1];
	vec[2] = pForward[2];

	if (pActor->pExtra->offsetYaw != 0.0f) {
		RdMatrix34 turnMatrix;

		_pRd->buildFromVectorAngle34(turnMatrix, getOrientMatrix(pActor)[2], pActor->pExtra->offsetYaw);
		_pRd->transformVector34Acc(vec, turnMatrix);
	}

	vec[0] = vec[0] + pPuck->pos[0];
	vec[1] = pPuck->pos[1] + vec[1];
	vec[2] = pPuck->pos[2] + vec[2];
	pPuck->invXform(vec);
	vec[2] = 0.0f;
	_pRd->normalize3Acc(vec);

	double angle = 90.0 - _pStdMath->arcSin3(vec[1]);
	float angleF = (float)angle;

	if (angle < 0.5) {
		pActor->pExtra->alignOff = 0.0f;
		return 1;
	}

	pActor->pExtra->alignOff = angleF;

	Extra *pExtra = pActor->pExtra;
	double limit;

	if (pExtra->turnRate <= 0.0f) {
		limit = 3.4028234663852886e38;
		pExtra->turnRate = pExtra->turnRate * -1.0f;
	} else {
		float frameTime = _pSputTime->getFrameTimeFlex();

		limit = (double)frameTime * pActor->pExtra->timeScale * pActor->pExtra->turnRate;
	}

	if (angleF > limit)
		angleF = (float)((angleF - limit) * 0.10000000149011612 + limit);

	float rotX = pActor->actorRot[0];
	float rotY = pActor->actorRot[1];
	float rotZ = pActor->actorRot[2];

	if (!(vec[0] > 0.0f))
		rotY = (float)((double)rotY - angleF);
	else
		rotY = (float)((double)rotY + angleF);

	pActor->actorRot[0] = rotX;
	pActor->actorRot[1] = rotY;
	pActor->actorRot[2] = rotZ;
	pActor->matrixValid = 0;

	return 0;
}

void SputActorModule::updateCostumes4(SputActor *pActor) {
	if (!pActor->pExtra)
		return;

	CostumeNode *pNode = pActor->pCostumeStack;

	if (!pNode)
		return;

	RdThing *pHeadThing = pNode->pHeadThing;

	do {
		if (pNode->pHeadThing != pHeadThing)
			break;

		if (pNode->walkChore >= 0)
			break;

		pNode = pNode->pNext;
	} while (pNode);

	if (pNode && pNode->pHeadThing == pHeadThing) {
		int status = _pCos->instChoreStatus(pNode->hCostumeInstance, pNode->walkChore);

		if (pActor->pExtra->bWalking) {
			if (!status) {
				pActor->pExtra->resting = 0;
				fadeInChore(pNode->hCostumeInstance, pNode->walkChore, _walkSystemFadeTime);
			}
		} else if ((status & 1) && (!(pActor->mode & 1) || (_pGrim->getGameFlags() & ADGF_DEMO))) {
			fadeOutChore(pNode->hCostumeInstance, pNode->walkChore, _walkSystemFadeTime);
		}
	}

	if (pActor->pExtra->bWalking)
		pActor->pExtra->turnDir = 0;

	pNode = pActor->pCostumeStack;
	pHeadThing = pNode->pHeadThing;

	do {
		if (pNode->pHeadThing != pHeadThing)
			break;

		if (pNode->turnLeftChore >= 0)
			break;

		pNode = pNode->pNext;
	} while (pNode);

	if (pNode) {
		if (pNode->pHeadThing == pHeadThing && pNode->turnLeftChore >= 0) {
			int status = _pCos->instChoreStatus(pNode->hCostumeInstance, pNode->turnLeftChore);

			if (pActor->pExtra->turnDir <= 0) {
				if (status & 1)
					fadeOutChore(pNode->hCostumeInstance, pNode->turnLeftChore, _walkSystemFadeTime);
			} else if (!status) {
				pActor->pExtra->resting = 0;
				fadeInChore(pNode->hCostumeInstance, pNode->turnLeftChore, _walkSystemFadeTime);
			}
		}

		if (pNode->pHeadThing == pHeadThing && pNode->turnRightChore >= 0) {
			int status = _pCos->instChoreStatus(pNode->hCostumeInstance, pNode->turnRightChore);

			if (pActor->pExtra->turnDir >= 0) {
				if (status & 1)
					fadeOutChore(pNode->hCostumeInstance, pNode->turnRightChore, _walkSystemFadeTime);
			} else if (!status) {
				pActor->pExtra->resting = 0;
				fadeInChore(pNode->hCostumeInstance, pNode->turnRightChore, _walkSystemFadeTime);
			}
		}
	}

	if (!pActor->pExtra->bWalking && !pActor->pExtra->turnDir)
		pActor->pExtra->resting = 1;

	pNode = pActor->pCostumeStack;
	pHeadThing = pNode->pHeadThing;

	do {
		if (pNode->pHeadThing != pHeadThing)
			break;

		if (pNode->restChore >= 0)
			break;

		pNode = pNode->pNext;
	} while (pNode);

	if (pNode && pNode->pHeadThing == pHeadThing && pNode->restChore >= 0) {
		int status = _pCos->instChoreStatus(pNode->hCostumeInstance, pNode->restChore);

		if (pActor->pExtra->resting) {
			if (!status)
				fadeInChore(pNode->hCostumeInstance, pNode->restChore, _walkSystemFadeTime);
		} else if (status) {
			fadeOutChore(pNode->hCostumeInstance, pNode->restChore, _walkSystemFadeTime);
		}
	}

	pActor->pExtra->bWalking = 0;
	pActor->pExtra->turnDir = 0;
}

void SputActorModule::updatePuckMovement(SputActor *pActor) {
	Extra *pExtra = pActor->pExtra;
	double deltaY = pActor->actorPos[1] - pExtra->walkTarget[1];
	double deltaX = pActor->actorPos[0] - pExtra->walkTarget[0];
	double deltaZ = pActor->actorPos[2] - pExtra->walkTarget[2];
	float deltaXF = (float)deltaX;
	float deltaYF = (float)deltaY;
	float distance = (float)sqrt(deltaX * deltaXF + deltaY * deltaYF + deltaZ * deltaZ);
	float walkRate = pExtra->walkRate;

	if (walkRate < 0.0f)
		walkRate = -walkRate;

	pExtra = pActor->pExtra;

	float frameTime = _pSputTime->getFrameTimeFlex();
	float step = (float)((double)frameTime * pExtra->timeScale * walkRate * 1.25);

	if (distance < step) {
		double currentDistance;

		do {
			if (pExtra->pathIndex >= pExtra->numPathPoints)
				break;

			pExtra->pathIndex++;
			setActorPosition(pActor, pActor->pExtra->walkTarget);

			if (pActor->pExtra->pathIndex < 0 || pActor->pExtra->pathIndex >= 30) {
				warning("SputActorModule::updatePuckMovement(): path index %d out of range", pActor->pExtra->pathIndex);
				break;
			}

			float *pNextPoint = pActor->pExtra->aPath[pActor->pExtra->pathIndex];

			pActor->pExtra->walkTarget[0] = pNextPoint[0];
			pActor->pExtra->walkTarget[1] = pNextPoint[1];
			pActor->pExtra->walkTarget[2] = pNextPoint[2];

			pExtra = pActor->pExtra;
			deltaY = pActor->actorPos[1] - pExtra->walkTarget[1];
			deltaX = pActor->actorPos[0] - pExtra->walkTarget[0];
			deltaZ = pActor->actorPos[2] - pExtra->walkTarget[2];
			deltaXF = (float)deltaX;
			deltaYF = (float)deltaY;
			currentDistance = sqrt(deltaX * deltaXF + deltaY * deltaYF + deltaZ * deltaZ);
		} while (currentDistance < step);

		pExtra = pActor->pExtra;

		if (pExtra->pathIndex == pExtra->numPathPoints) {
			pActor->mode &= ~1;
			return;
		}
	}

	pActor->mode |= 1;
	pExtra->puck->pointAt(pExtra->walkTarget, 3.4028235e38f);
	forward(pActor);
}

void SputActorModule::leaveSector(SputActor *pActor, WalkPlane *pSector) {
	if (!pSector || !pActor)
		return;

	Puck *pPuck = pActor->pExtra->puck;

	if (pPuck) {
		pSector->jumpOut(pPuck);
		setActorPosition(pActor, pPuck->pos);
	}
}

void SputActorModule::setActorConstrain(SputActor *pActor, int bConstrain) {
	if (!(pActor->mode & 1) || !bConstrain) {
		Puck *pPuck = pActor->pExtra->puck;

		if (pPuck)
			pPuck->constrain(bConstrain);
	}
}

} // End of namespace Grim
