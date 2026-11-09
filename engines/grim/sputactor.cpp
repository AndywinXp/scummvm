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

#include "engines/grim/chore/cos.h"
#include "engines/grim/grim.h"
#include "engines/grim/puck.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/std/std_sort.h"
#include "engines/grim/walkplane.h"

namespace Grim {

static const LipSyncEntry g_lipSyncAnimTable[46] = {
	{ 95, 0 }, { 593, 1 }, { 97, 1 }, { 230, 1 }, { 652, 8 }, { 596, 1 }, { 601, 1 }, { 98, 6 },
	{ 679, 2 }, { 100, 2 }, { 240, 5 }, { 603, 8 }, { 616, 8 }, { 602, 9 }, { 605, 9 }, { 101, 1 },
	{ 102, 4 }, { 103, 8 }, { 609, 8 }, { 104, 8 }, { 618, 8 }, { 105, 3 }, { 676, 2 }, { 107, 2 },
	{ 108, 5 }, { 619, 5 }, { 109, 6 }, { 110, 8 }, { 331, 8 }, { 111, 7 }, { 112, 6 }, { 114, 2 },
	{ 635, 2 }, { 633, 2 }, { 115, 2 }, { 643, 2 }, { 116, 2 }, { 638, 2 }, { 952, 5 }, { 650, 9 },
	{ 117, 9 }, { 118, 4 }, { 119, 9 }, { 106, 8 }, { 122, 2 }, { 658, 2 }
};

SputActorModule::SputActorModule(Res *pRes, RenderDroid *pRd, StdMath *pStdMath, StdDisplay *pStdDisplay, WalkSystem *pWalkSystem,
		SputResource *pSputResource, SputRoom *pSputRoom, SputRender *pSputRender, SputCostume *pSputCostume, SputText *pSputText,
		SputTime *pSputTime) :
		_pRes(pRes), _pRd(pRd), _pStdMath(pStdMath), _pStdDisplay(pStdDisplay), _pWalkSystem(pWalkSystem), _pSputResource(pSputResource),
		_pSputRoom(pSputRoom), _pSputRender(pSputRender), _pSputCostume(pSputCostume), _pSputText(pSputText), _pSputTime(pSputTime),
		_pGrim(nullptr), _pCos(pSputCostume->getCos()),
		_pHS(nullptr), _bInitialized(0), _bStarted(0), _aActors(nullptr), _maxActors(1024), _numActors(0), _pSelectedActor(nullptr),
		_numActorsTalking(0), _numCollisionActors(0), _speechMode(2), _textRate(6), _walkSystemFadeTime(130), _numLipSyncAnims(46),
		_pShadowMaterial(nullptr) {
	memset(_apTalkingActors, 0, sizeof(_apTalkingActors));
	memset(_apCollisionActors, 0, sizeof(_apCollisionActors));
	memset(_collisionImpact, 0, sizeof(_collisionImpact));
	memset(_collisionOut, 0, sizeof(_collisionOut));
	memset(_collisionIn, 0, sizeof(_collisionIn));
	memcpy(_aLipSyncAnims, g_lipSyncAnimTable, sizeof(_aLipSyncAnims));

	// The demo has no shadow material, its shadows are one solid cel
	memset(&_demoShadowMaterial, 0, sizeof(_demoShadowMaterial));
	memset(&_demoShadowCel, 0, sizeof(_demoShadowCel));
	Common::strlcpy(_demoShadowMaterial.aName, "Default shadow material", 32);
	_demoShadowMaterial.numCels = 1;
	_demoShadowMaterial.apCels[0] = &_demoShadowCel;
}

void SputActorModule::publishActors() {
	_pSputRoom->setActors(_aActors);
	_pSputRender->setActors(_aActors);
	_pSputCostume->setActors(_aActors);
}

void SputActorModule::nukeCostumeStacks() {
	for (int i = 0; i < _maxActors; i++) {
		if (!_aActors[i].bInUse)
			continue;

		if (_aActors[i].pCostumeStack)
			_pSputCostume->printCostumeInfo(_aActors[i].pCostumeStack);

		_pSputCostume->freeCostumeNodes(_aActors[i].pCostumeStack, 1);
		_aActors[i].pCostumeStack = nullptr;
	}
}

int SputActorModule::initialize(SysStruct *pHS) {
	_pHS = pHS;

	for (int i = 0; i < 4; i++)
		_apTalkingActors[i] = nullptr;

	_bInitialized = 1;

	return 0;
}

int SputActorModule::terminate() {
	_pHS = nullptr;

	if (_bInitialized)
		_bInitialized = 0;

	return 0;
}

int SputActorModule::startup() {
	if (_bStarted)
		return 1;

	_aActors = new SputActor[_maxActors]();
	publishActors();

	Grim::sort(_aLipSyncAnims, _numLipSyncAnims, sizeof(LipSyncEntry), compareAnimEntry);

	_bStarted = 1;

	return 0;
}

void SputActorModule::shutdown() {
	for (int i = 0; i < _maxActors; i++)
		freeActor(&_aActors[i]);

	delete[] _aActors;
	_aActors = nullptr;
	publishActors();

	_bStarted = 0;
}

int SputActorModule::getNumActorsTalking() {
	return _numActorsTalking;
}

int SputActorModule::setCostume(SputActor *pActor, const char *costumeName, int hInstance) {
	CostumeNode *pNode;

	if (!costumeName || hInstance) {
		pNode = _pSputCostume->setCostume(pActor->pCostumeStack, 0, hInstance);
	} else {
		int hCostume = (int)(intptr)_pSputResource->alloc(kSputTagCost, costumeName);

		if (!hCostume)
			return 0;

		pNode = _pSputCostume->setCostume(pActor->pCostumeStack, hCostume, 0);
	}

	pActor->pCostumeStack = pNode;
	_pCos->instSetUserData(pActor->pCostumeStack->hCostumeInstance, (int)(pActor - _aActors));
	setFrustrumCull(pActor, ((uint)~pActor->flags >> 2) & 1);

	return 1;
}

void SputActorModule::setActorHead(SputActor *pActor, uint joint1, uint joint2, uint joint3, float maxAngle, float yawLimit, float pitchLimit) {
	CostumeNode *pNode = pActor->pCostumeStack;

	if (!pNode || !pNode->pHeadThing)
		return;

	uint numHNodes = pNode->pHeadThing->data.pModel3->numHNodes;

	if (joint1 > numHNodes || joint2 > numHNodes || joint3 > numHNodes)
		return;

	pNode->aHeadJoint[0] = joint1;
	pNode->aHeadJoint[1] = joint2;
	pNode->headYawLimit = yawLimit;
	pNode->aHeadJoint[2] = joint3;
	pNode->headPitchLimit = pitchLimit;

	if (maxAngle < 0.0f)
		maxAngle = -maxAngle;

	float halfAngle = (float)((double)maxAngle * 0.5);
	float sinValue;

	_pStdMath->sinCos(halfAngle, &sinValue, &pNode->headLookRef);
}

int SputActorModule::setSelectedActor(int handle) {
	int result = -1;

	if (_pSelectedActor)
		result = getActorHandle(_pSelectedActor);

	if (handle >= 0 && handle < _maxActors)
		_pSelectedActor = &_aActors[handle];

	return result;
}

SputActor *SputActorModule::actorDeref(int handle) {
	if (handle < 0 || handle >= _maxActors) {
		warning("SputActorModule::actorDeref(): invalid actor handle %d", handle);
		return nullptr;
	}

	return &_aActors[handle];
}

int SputActorModule::getActorHandle(SputActor *pActor) {
	return (int)(pActor - _aActors);
}

int SputActorModule::allocateActor() {
	SputActor *pActor = nullptr;

	for (int i = 0; i < _maxActors; i++) {
		if (!_aActors[i].bInUse) {
			pActor = &_aActors[i];
			break;
		}
	}

	if (!pActor) {
		int aTalkingIndex[4];
		int selectedIndex = -1;

		for (int i = 0; i < 4; i++) {
			aTalkingIndex[i] = -1;

			if (_apTalkingActors[i])
				aTalkingIndex[i] = (int)(_apTalkingActors[i] - _aActors);
		}

		if (_pSelectedActor)
			selectedIndex = (int)(_pSelectedActor - _aActors);

		SputActor *pNewActors = new SputActor[_maxActors + 32]();

		memcpy(pNewActors, _aActors, _maxActors * sizeof(SputActor));
		delete[] _aActors;
		_aActors = pNewActors;
		_maxActors += 32;
		pActor = &_aActors[_maxActors - 32];

		for (int i = 0; i < 4; i++) {
			if (_apTalkingActors[i])
				_apTalkingActors[i] = &_aActors[aTalkingIndex[i]];
		}

		if (_pSelectedActor)
			_pSelectedActor = &_aActors[selectedIndex];

		for (int i = 0; i < _maxActors - 32; i++) {
			if (_aActors[i].bInUse && _aActors[i].pExtra && _aActors[i].pExtra->pSayText)
				_aActors[i].pExtra->pSayText->pActor = &_aActors[i];
		}

		publishActors();
	}

	_numActors++;

	memset(pActor, 0, sizeof(SputActor));
	pActor->matrixValid = 1;
	_pRd->identity34(pActor->orientMatrix);
	pActor->scale = 1.0f;
	pActor->pCostumeStack = nullptr;
	pActor->pColorMap = new ResRef();
	pActor->bInUse = 1;

	return (int)(pActor - _aActors);
}

void SputActorModule::allocateActorExtra(SputActor *pActor) {
	if (!pActor || pActor->pExtra) {
		warning("SputActorModule::allocateActorExtra(): no actor or the actor already has an extra");
		return;
	}

	pActor->pExtra = new Extra();
	pActor->pExtra->walkRate = 0.40000001f;
	pActor->pExtra->turnRate = 135.0f;
	pActor->pExtra->walkDominate = 0;
	pActor->pExtra->reflectionAngle = 60.0f;
	pActor->pExtra->collisionScale = 1.0f;
	pActor->pExtra->timeScale = 1.0f;
	pActor->pExtra->talkColor = 0xFFFFFF;
	pActor->pExtra->subClipNode = -1;
	pActor->pExtra->clipPlaneNorm[0] = 0.0f;
	pActor->pExtra->clipPlaneNorm[1] = 0.0f;
	pActor->pExtra->clipPlaneNorm[2] = 1.0f;
}

void SputActorModule::enableWalkSystem(SputActor *pActor, int bEnable) {
	if (!pActor->pExtra)
		allocateActorExtra(pActor);

	if (bEnable) {
		pActor->pExtra->inBoxes = 1;
		enablePuck(pActor, 1);
		pActor->pExtra->puck->validate();
	} else {
		pActor->pExtra->inBoxes = 0;

		if (pActor->pExtra->puck)
			pActor->pExtra->puck->pPlane = nullptr;
	}
}

void SputActorModule::enablePuck(SputActor *pActor, int bEnable) {
	Extra *pExtra = pActor->pExtra;

	if (bEnable || !pExtra->puck) {
		if (!pExtra->puck) {
			Puck *pPuck = new Puck(_pWalkSystem, pExtra->reflectionAngle);

			pPuck->setPos(pActor->actorPos);

			float *pForward = getOrientMatrix(pActor)[1];
			RdVector3 demoForward;
			RdVector3 target;

			// The demo reads the three values after the matrix (walking, turn direction, resting) as floats
			if (_pGrim->getGameFlags() & ADGF_DEMO) {
				memcpy(&demoForward[0], &pExtra->bWalking, sizeof(float));
				memcpy(&demoForward[1], &pExtra->turnDir, sizeof(float));
				memcpy(&demoForward[2], &pExtra->resting, sizeof(float));
				pForward = demoForward;
			}

			target[0] = pForward[0] + pActor->actorPos[0];
			target[1] = pActor->actorPos[1] + pForward[1];
			target[2] = pActor->actorPos[2] + pForward[2];
			pPuck->pointAt(target, 3.4028235e38f);
			pExtra->puck = pPuck;
		}
	} else {
		delete pExtra->puck;
		pExtra->puck = nullptr;
	}
}

void SputActorModule::setActorPosition(SputActor *pActor, float *pPos) {
	Puck *pPuck = nullptr;

	if (pActor->pExtra)
		pPuck = pActor->pExtra->puck;

	pActor->mode &= ~1;

	if (pPuck) {
		pPuck->setPos(pPos);

		if (pActor->pExtra->inBoxes)
			pPuck->validate();

		pActor->actorPos[0] = pPuck->pos[0];
		pActor->actorPos[1] = pPuck->pos[1];
		pActor->actorPos[2] = pPuck->pos[2];
	} else {
		pActor->actorPos[0] = pPos[0];
		pActor->actorPos[1] = pPos[1];
		pActor->actorPos[2] = pPos[2];
	}

	pActor->matrixValid = 0;
}

void SputActorModule::setActorRotation(SputActor *pActor, float *pRot, int bFlipTurn) {
	Extra *pExtra = pActor->pExtra;

	pActor->mode &= ~1;

	if (pExtra && pExtra->puck) {
		Puck *pPuck = pExtra->puck;
		RdMatrix34 matrix;

		_pRd->buildRotate34(matrix, pRot);
		matrix[1][0] = matrix[1][0] + pPuck->pos[0];
		matrix[1][1] = pPuck->pos[1] + matrix[1][1];
		matrix[1][2] = pPuck->pos[2] + matrix[1][2];
		pPuck->invXform(matrix[1]);
		matrix[1][2] = 0.0f;
		_pRd->normalize3Acc(matrix[1]);

		float angle = (float)(90.0 - _pStdMath->arcSin3(matrix[1][1]));

		if (!(matrix[1][0] > 0.0f))
			pPuck->rotate(angle);
		else
			pPuck->rotate(-angle);

		if (pActor->pExtra->turnRate > 0.0f) {
			if (bFlipTurn)
				pActor->pExtra->turnRate = pActor->pExtra->turnRate * -1.0f;
		}

		pActor->pExtra->alignOff = angle;

		// The demo leaves the rotation of the actor alone here
		if (!(_pGrim->getGameFlags() & ADGF_DEMO)) {
			pActor->actorRot[2] = pRot[2];
			pActor->actorRot[0] = pRot[0];
			pActor->matrixValid = 0;
		}
	} else {
		pActor->actorRot[0] = pRot[0];
		pActor->actorRot[1] = pRot[1];
		pActor->actorRot[2] = pRot[2];
		pActor->matrixValid = 0;
	}
}

void SputActorModule::setReflection(SputActor *pActor, float angle) {
	Puck *pPuck = pActor->pExtra->puck;

	if (pPuck)
		pPuck->reflectionAngle = angle;
}

void SputActorModule::setWalkChore(SputActor *pActor, int choreIndex, CostumeNode *pNode) {
	CostumeNode *pCurrent = pActor->pCostumeStack;

	if (!pCurrent)
		return;

	bool bDemo = (_pGrim->getGameFlags() & ADGF_DEMO) != 0;

	// The demo has no costume parameter, the chore stays in the node where it was found
	CostumeNode *pTarget = bDemo ? nullptr : pNode;

	if (!pTarget)
		pTarget = pActor->pCostumeStack;

	RdThing *pHeadThing = pCurrent->pHeadThing;

	do {
		if (pCurrent->pHeadThing != pHeadThing)
			break;

		if (pCurrent->walkChore >= 0)
			break;

		pCurrent = pCurrent->pNext;
	} while (pCurrent);

	if (pCurrent && pCurrent->pHeadThing == pHeadThing) {
		if (choreIndex != pCurrent->walkChore) {
			int status = _pCos->instChoreStatus(pCurrent->hCostumeInstance, pCurrent->walkChore);

			if (status) {
				fadeOutChore(pCurrent->hCostumeInstance, pCurrent->walkChore, _walkSystemFadeTime);

				if (choreIndex != -1) {
					_pCos->instPlayChore(pCurrent->hCostumeInstance, choreIndex, status & 2, ((uint)~status >> 2) & 1);
					_pCos->instUpdateChore(pCurrent->hCostumeInstance, choreIndex, 100);
					_pSputCostume->fadeKeysIn(pCurrent->hCostumeInstance, choreIndex, _walkSystemFadeTime);
				}
			}

			if (bDemo) {
				pCurrent->walkChore = choreIndex;
			} else {
				pCurrent->walkChore = -1;
				pTarget->walkChore = choreIndex;
			}
		}
	} else {
		pTarget->walkChore = choreIndex;
	}
}

void SputActorModule::setTurnChores(SputActor *pActor, int leftChore, int rightChore, CostumeNode *pNode) {
	CostumeNode *pCurrent = pActor->pCostumeStack;

	if (!pCurrent)
		return;

	bool bDemo = (_pGrim->getGameFlags() & ADGF_DEMO) != 0;

	// The demo has no costume parameter, the chores stay in the node where they were found
	CostumeNode *pTarget = bDemo ? nullptr : pNode;

	if (!pTarget)
		pTarget = pActor->pCostumeStack;

	RdThing *pHeadThing = pCurrent->pHeadThing;

	do {
		if (pCurrent->pHeadThing != pHeadThing)
			break;

		if (pCurrent->turnLeftChore >= 0)
			break;

		pCurrent = pCurrent->pNext;
	} while (pCurrent);

	if (pCurrent && pCurrent->pHeadThing == pHeadThing) {
		if (leftChore != pCurrent->turnLeftChore) {
			int status = _pCos->instChoreStatus(pCurrent->hCostumeInstance, pCurrent->turnLeftChore);

			if (status) {
				_pCos->instStopChore(pCurrent->hCostumeInstance, pCurrent->turnLeftChore);

				if (leftChore != -1)
					_pCos->instPlayChore(pCurrent->hCostumeInstance, leftChore, status & 2, ((uint)~status >> 2) & 1);
			}

			if (bDemo) {
				pCurrent->turnLeftChore = leftChore;
			} else {
				pCurrent->turnLeftChore = -1;
				pTarget->turnLeftChore = leftChore;
			}
		}

		if (rightChore != pCurrent->turnRightChore) {
			int status = _pCos->instChoreStatus(pCurrent->hCostumeInstance, pCurrent->turnRightChore);

			if (status) {
				_pCos->instStopChore(pCurrent->hCostumeInstance, pCurrent->turnRightChore);

				if (leftChore != -1)
					_pCos->instPlayChore(pCurrent->hCostumeInstance, rightChore, status & 2, ((uint)~status >> 2) & 1);
			}

			if (bDemo) {
				pCurrent->turnRightChore = rightChore;
			} else {
				pCurrent->turnRightChore = -1;
				pTarget->turnRightChore = rightChore;
			}
		}
	} else {
		pTarget->turnLeftChore = leftChore;
		pTarget->turnRightChore = rightChore;
	}
}

void SputActorModule::setRestChore(SputActor *pActor, int choreIndex, CostumeNode *pNode) {
	CostumeNode *pCurrent = pActor->pCostumeStack;

	if (!pCurrent)
		return;

	bool bDemo = (_pGrim->getGameFlags() & ADGF_DEMO) != 0;

	// The demo has no costume parameter, the chore stays in the node where it was found
	CostumeNode *pTarget = bDemo ? nullptr : pNode;

	if (!pTarget)
		pTarget = pActor->pCostumeStack;

	RdThing *pHeadThing = pCurrent->pHeadThing;

	do {
		if (pCurrent->pHeadThing != pHeadThing)
			break;

		// The demo looks for the node by the left turn chore
		if (bDemo ? pCurrent->turnLeftChore >= 0 : pCurrent->restChore >= 0)
			break;

		pCurrent = pCurrent->pNext;
	} while (pCurrent);

	if (pCurrent && pCurrent->pHeadThing == pHeadThing) {
		if (choreIndex != pCurrent->restChore) {
			int status = _pCos->instChoreStatus(pCurrent->hCostumeInstance, pCurrent->restChore);

			if (status) {
				_pCos->instStopChore(pCurrent->hCostumeInstance, pCurrent->restChore);

				if (choreIndex != -1)
					_pCos->instPlayChore(pCurrent->hCostumeInstance, choreIndex, status & 2, ((uint)~status >> 2) & 1);
			}

			if (bDemo) {
				pCurrent->restChore = choreIndex;
			} else {
				pCurrent->restChore = -1;
				pTarget->restChore = choreIndex;
			}
		}
	} else {
		pTarget->restChore = choreIndex;
	}
}

int SputActorModule::pushCostume(SputActor *pActor, const char *costumeName, int hInstance) {
	CostumeNode *pNode;

	if (!costumeName || hInstance) {
		pNode = _pSputCostume->pushCostume(pActor->pCostumeStack, 0, hInstance);
	} else {
		int hCostume = (int)(intptr)_pSputResource->alloc(kSputTagCost, costumeName);

		if (!hCostume)
			return 0;

		pNode = _pSputCostume->pushCostume(pActor->pCostumeStack, hCostume, 0);
	}

	pActor->pCostumeStack = pNode;
	_pCos->instSetUserData(pActor->pCostumeStack->hCostumeInstance, (int)(pActor - _aActors));
	setFrustrumCull(pActor, ((uint)~pActor->flags >> 2) & 1);

	return 1;
}

void SputActorModule::popCostume(SputActor *pActor, int bFreeInstance) {
	pActor->pCostumeStack = _pSputCostume->popCostume(pActor->pCostumeStack, bFreeInstance);
}

void SputActorModule::fadeInChore(int hInst, int choreIndex, int fadeMs) {
	_pCos->instPlayChore(hInst, choreIndex, 1, 0);
	_pCos->instUpdateChore(hInst, choreIndex, 70);
	_pSputCostume->fadeKeysIn(hInst, choreIndex, fadeMs);
}

void SputActorModule::fadeOutChore(int hInst, int choreIndex, int fadeMs) {
	if (choreIndex == -1) {
		int numChores = _pCos->instCountChores(hInst);

		for (int i = 0; i < numChores; i++)
			fadeOutChore(hInst, i, fadeMs);
	} else {
		_pSputCostume->fadeKeysOut(hInst, choreIndex, fadeMs);
		_pCos->instStopChore(hInst, choreIndex);
	}
}

void SputActorModule::setSpeechMode(int mode) {
	if (mode != _speechMode && (mode & 3) != 0)
		_speechMode = mode;
}

int SputActorModule::getSpeechMode() {
	return _speechMode;
}

void SputActorModule::setWalkDominate(SputActor *pActor, int bDominate) {
	pActor->pExtra->walkDominate = bDominate;
}

int SputActorModule::countCostumes(SputActor *pActor) {
	int count = 0;

	for (CostumeNode *pNode = pActor->pCostumeStack; pNode; pNode = pNode->pNext)
		count++;

	return count;
}

void SputActorModule::setClipPlane(SputActor *pActor, float *pNormal, float *pPoint) {
	if (!pActor->pExtra)
		allocateActorExtra(pActor);

	pActor->pExtra->clipPlaneNorm[0] = pNormal[0];
	pActor->pExtra->clipPlaneNorm[1] = pNormal[1];
	pActor->pExtra->clipPlaneNorm[2] = pNormal[2];
	pActor->pExtra->clipPlanePt[0] = pPoint[0];
	pActor->pExtra->clipPlanePt[1] = pPoint[1];
	pActor->pExtra->clipPlanePt[2] = pPoint[2];
}

void SputActorModule::setClipPlaneActive(SputActor *pActor, int bActive) {
	if (!pActor->pExtra)
		allocateActorExtra(pActor);

	pActor->pExtra->clipPlaneActive = bActive;
}

int SputActorModule::getActorTextRate() {
	return _textRate;
}

void SputActorModule::setActorTextRate(int rate) {
	if (rate >= 1) {
		if (rate > 10)
			rate = 10;

		_textRate = rate;
	} else {
		_textRate = 1;
	}
}

void SputActorModule::setFrustrumCull(SputActor *pActor, int bCull) {
	if (!pActor)
		return;

	if (bCull)
		pActor->flags |= 4;
	else
		pActor->flags &= ~4;

	if (pActor->pCostumeStack) {
		for (ModelInst *pModelInst = pActor->pCostumeStack->pHeadList; pModelInst; pModelInst = pModelInst->pNext)
			pModelInst->pThing->frustrumCull = bCull;
	}
}

void SputActorModule::freeActor(SputActor *pActor) {
	if (pActor->curSetName[0])
		_pSputRoom->removeActor(pActor->curSetName, pActor);

	resetShadows(pActor);
	_pSputCostume->freeCostumeNodes(pActor->pCostumeStack, 0);
	pActor->pCostumeStack = nullptr;

	if (pActor->pExtra) {
		delete pActor->pExtra->puck;
		pActor->pExtra->puck = nullptr;
		delete pActor->pExtra;
		pActor->pExtra = nullptr;
	}

	if (pActor->pColorMap) {
		_pRes->removeReference(pActor->pColorMap);
		delete pActor->pColorMap;
		pActor->pColorMap = nullptr;
	}

	pActor->bInUse = 0;
	_numActors--;
}

void SputActorModule::setWalkSystemFadeTime(int fadeMs) {
	_walkSystemFadeTime = fadeMs;
}

CostumeNode *SputActorModule::allocCostumeNode() {
	CostumeNode *pNode = new CostumeNode();

	initCostumeNode(pNode);

	return pNode;
}

void SputActorModule::initCostumeNode(CostumeNode *pNode) {
	pNode->hCostumeInstance = 0;
	pNode->aHeadJoint[0] = -1;
	pNode->aHeadJoint[1] = -1;
	pNode->aHeadJoint[2] = -1;
	pNode->headYawLimit = 30.0f;
	pNode->headPitchLimit = 90.0f;
	pNode->headLookRef = -0.35f;
	pNode->headTurnRate = 360.0f;
	pNode->walkChore = -1;
	pNode->turnLeftChore = -1;
	pNode->turnRightChore = -1;
	pNode->restChore = -1;
	pNode->mumbleChore = -1;
	memset(pNode->aChore, 0xFF, sizeof(pNode->aChore));
	memset(&pNode->rCostume, 0, sizeof(pNode->rCostume));
	pNode->pHeadList = nullptr;
	pNode->pNext = nullptr;
}

} // End of namespace Grim
