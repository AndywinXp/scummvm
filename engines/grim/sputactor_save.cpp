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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/chore/cos.h"
#include "engines/grim/puck.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputcostume_components.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/walkplane.h"

namespace Grim {

int SputActorModule::save(Common::WriteStream *pStream) {
	pStream->writeSint32LE(_maxActors);
	pStream->writeSint32LE(_numActors);

	for (int actorIndex = 0; actorIndex < _maxActors; actorIndex++) {
		SputActor *pActor = &_aActors[actorIndex];

		if (!pActor->bInUse)
			continue;

		pStream->writeSint32LE(actorIndex);
		pStream->writeSint32LE(pActor->pColorMap->valid ? _pRes->serialFromHandle(pActor->pColorMap->handle) : pActor->pColorMap->serial);
		pStream->writeSint32LE(pActor->flags);
		pStream->writeSint32LE(pActor->mode);

		for (int i = 0; i < 3; i++)
			pStream->writeFloatLE(pActor->actorPos[i]);

		for (int i = 0; i < 3; i++)
			pStream->writeFloatLE(pActor->prevPosition[i]);

		for (int i = 0; i < 3; i++)
			pStream->writeFloatLE(pActor->actorRot[i]);

		pStream->writeFloatLE(pActor->scale);
		pStream->writeSint32LE(pActor->matrixValid);

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 3; j++)
				pStream->writeFloatLE(pActor->orientMatrix[i][j]);
		}

		pStream->writeSint32LE(pActor->onScreen);
		pStream->writeSint32LE(pActor->drawnRect.x);
		pStream->writeSint32LE(pActor->drawnRect.y);
		pStream->writeSint32LE(pActor->drawnRect.width);
		pStream->writeSint32LE(pActor->drawnRect.height);
		pStream->write(pActor->curSetName, 32);
		pStream->writeSint32LE(pActor->pExtra != nullptr);

		int numCostumes = countCostumes(pActor);

		pStream->writeSint32LE(numCostumes);

		CostumeNode **apNodes = new CostumeNode *[numCostumes + 1];
		CostumeNode *pNode = pActor->pCostumeStack;

		for (int i = numCostumes - 1; i >= 0; i--) {
			apNodes[i] = pNode;
			pNode = pNode->pNext;
		}

		for (int i = 0; i < numCostumes; i++) {
			CostumeNode *pSaved = apNodes[i];

			pStream->writeSint32LE(pSaved->hCostumeInstance);

			for (int j = 0; j < 3; j++)
				pStream->writeSint32LE(pSaved->aHeadJoint[j]);

			pStream->writeFloatLE(pSaved->headTurnRate);
			pStream->writeFloatLE(pSaved->headYawLimit);
			pStream->writeFloatLE(pSaved->headPitchLimit);
			pStream->writeFloatLE(pSaved->headLookRef);
			pStream->writeSint32LE(pSaved->walkChore);
			pStream->writeSint32LE(pSaved->turnLeftChore);
			pStream->writeSint32LE(pSaved->turnRightChore);
			pStream->writeSint32LE(pSaved->restChore);

			for (int j = 0; j < 16; j++)
				pStream->writeSint32LE(pSaved->aChore[j]);

			pStream->writeSint32LE(pSaved->mumbleChore);

			if (!pSaved->rCostume.valid)
				warning("SputActorModule::save(): the costume reference is not valid");

			pStream->writeSint32LE(pSaved->rCostume.valid ? _pRes->serialFromHandle(pSaved->rCostume.handle) : pSaved->rCostume.serial);
			_pCos->saveInstance(pSaved->hCostumeInstance, pStream);
		}

		delete[] apNodes;

		if (pActor->pCostumeStack) {
			int numModels = 0;

			for (ModelInst *pModelInst = pActor->pCostumeStack->pHeadList; pModelInst; pModelInst = pModelInst->pNext)
				numModels++;

			pStream->writeSint32LE(numModels);

			for (ModelInst *pModelInst = pActor->pCostumeStack->pHeadList; pModelInst; pModelInst = pModelInst->pNext) {
				pStream->writeSint32LE(pModelInst->bVisible);
				pStream->writeSint32LE(pModelInst->resolveParentOffset);
				pStream->writeSint32LE(pModelInst->parentMeshIdx);

				if (pModelInst->pThing->type == 1) {
					if (!pModelInst->modelRef.valid)
						warning("SputActorModule::save(): Bad model Reference");

					RdModel3 *pModel3 = static_cast<RdModel3 *>(pModelInst->modelRef.handle);

					for (uint i = 0; i < pModel3->numHNodes; i++)
						pStream->writeSint32LE(pModelInst->pThing->abMeshVisible[i]);
				}

				int numTracks = 0;
				RdPuppet *pPuppet = pModelInst->pThing->pPuppet;

				if (pPuppet) {
					for (int i = 0; i < 6; i++) {
						if (pPuppet->aTracks[i].pKeyframe)
							numTracks++;
					}

					pStream->writeSint32LE(numTracks);

					for (int i = 0; i < 6; i++) {
						RdPuppetTrack *pTrack = &pPuppet->aTracks[i];

						if (!pTrack->pKeyframe)
							continue;

						pStream->write(pTrack->pKeyframe->aName, 32);
						pStream->writeSint32LE(pTrack->priority);
						pStream->writeSint32LE(pTrack->priorityMasked);
						pStream->writeUint32LE(pTrack->status);
						pStream->writeFloatLE(pTrack->curFrame);
						pStream->writeFloatLE(pTrack->prevFrame);
						pStream->writeFloatLE(pTrack->fade);
						pStream->writeFloatLE(pTrack->fadeSpeed);
					}
				} else {
					pStream->writeSint32LE(numTracks);
				}
			}
		}

		Extra *pExtra = pActor->pExtra;

		if (pExtra) {
			for (int i = 0; i < 3; i++)
				pStream->writeFloatLE(pExtra->lookCarrot[i]);

			pStream->writeSint32LE(pExtra->bWalking);
			pStream->writeSint32LE(pExtra->turnDir);
			pStream->writeSint32LE(pExtra->resting);
			pStream->writeFloatLE(pExtra->walkRate);
			pStream->writeFloatLE(pExtra->turnRate);
			pStream->writeSint32LE(pExtra->field_20);
			pStream->writeFloatLE(pExtra->offsetYaw);
			pStream->writeFloatLE(pExtra->alignOff);

			for (int i = 0; i < 3; i++)
				pStream->writeFloatLE(pExtra->walkTarget[i]);

			for (int i = 0; i < 3; i++)
				pStream->writeFloatLE(pExtra->goal[i]);

			pStream->writeSint32LE(pExtra->numPathPoints);

			for (int i = 0; i < 30; i++) {
				for (int j = 0; j < 3; j++)
					pStream->writeFloatLE(pExtra->aPath[i][j]);
			}

			pStream->writeSint32LE(pExtra->pathIndex);
			pStream->writeSint32LE(pExtra->puck != nullptr);
			pStream->writeSint32LE(pExtra->inBoxes);
			pStream->writeSint32LE(pExtra->walkDominate);
			pStream->writeFloatLE(pExtra->reflectionAngle);
			pStream->writeSint32LE(pExtra->talking);
			pStream->writeSint32LE(pExtra->soundId);
			pStream->writeSint32LE(pExtra->talkTimerMs);
			pStream->write(pExtra->sayLineId, 12);
			pStream->write(pExtra->voiceName, 15);

			if (pExtra->pSayText)
				pStream->writeSint32LE(pExtra->pSayText->serial);
			else
				pStream->writeSint32LE(0);

			pStream->writeSint32LE(pExtra->phoneme);
			pStream->writeSint32LE(pExtra->activeShadow);
			pStream->writeSint32LE(pExtra->collisionMode);
			pStream->writeFloatLE(pExtra->collisionScale);
			pStream->writeSint32LE(pExtra->field_3F8);
			pStream->writeSint32LE(pExtra->clipPlaneActive);

			for (int i = 0; i < 3; i++)
				pStream->writeFloatLE(pExtra->clipPlaneNorm[i]);

			for (int i = 0; i < 3; i++)
				pStream->writeFloatLE(pExtra->clipPlanePt[i]);

			pStream->writeSint32LE(pExtra->talkColor);
			pStream->writeFloatLE(pExtra->timeScale);
			pStream->writeSint32LE(pExtra->subClipNode);

			if (!_pSputRoom->getCurrentRoom())
				resetShadows(pActor);

			for (int i = 0; i < 6; i++) {
				ActorShadow *pShadow = &pExtra->aShadows[i];
				int numClipPlanes = 0;

				pStream->writeSint32LE(pShadow->bActive);
				pStream->writeSint32LE(pShadow->side);

				for (int j = 0; j < 3; j++)
					pStream->writeFloatLE(pShadow->normal[j]);

				pStream->writeFloatLE(pShadow->planeD);

				for (int j = 0; j < 3; j++)
					pStream->writeFloatLE(pShadow->lightPos[j]);

				for (int j = 0; j < 12; j++) {
					if (pShadow->apClipPlane[j])
						numClipPlanes++;
				}

				pStream->writeSint32LE(numClipPlanes);

				for (int j = 0; j < 12; j++) {
					if (pShadow->apClipPlane[j])
						pStream->write(pShadow->apClipPlane[j]->name, 32);
				}
			}
		}
	}

	int selectedIndex = -1;

	if (_pSelectedActor)
		selectedIndex = (int)(_pSelectedActor - _aActors);

	pStream->writeSint32LE(selectedIndex);

	if (pStream->err()) {
		warning("SputActorModule::save(): failed to write the actors");
		return 1;
	}

	return 0;
}

int SputActorModule::restore(Common::ReadStream *pStream) {
	for (int i = 0; i < 4; i++)
		_apTalkingActors[i] = nullptr;

	_numActorsTalking = 0;

	for (int i = 0; i < _maxActors; i++)
		freeActor(&_aActors[i]);

	int savedMaxActors = pStream->readSint32LE();

	if (savedMaxActors > _maxActors) {
		SputActor *pNewActors = new SputActor[savedMaxActors]();

		memcpy(pNewActors, _aActors, _maxActors * sizeof(SputActor));
		delete[] _aActors;
		_aActors = pNewActors;
		_maxActors = savedMaxActors;
		publishActors();
	}

	_numActors = pStream->readSint32LE();

	for (int actorCount = 0; actorCount < _numActors; actorCount++) {
		int actorIndex = pStream->readSint32LE();

		if (pStream->err() || pStream->eos() || actorIndex < 0 || actorIndex >= _maxActors) {
			warning("SputActorModule::restore(): invalid actor index %d", actorIndex);
			return 1;
		}

		SputActor *pActor = &_aActors[actorIndex];

		pActor->bInUse = 1;

		if (!pActor->pColorMap)
			pActor->pColorMap = new ResRef();

		pActor->pColorMap->serial = pStream->readSint32LE();
		pActor->flags = pStream->readSint32LE();
		pActor->mode = pStream->readSint32LE();

		for (int i = 0; i < 3; i++)
			pActor->actorPos[i] = pStream->readFloatLE();

		for (int i = 0; i < 3; i++)
			pActor->prevPosition[i] = pStream->readFloatLE();

		for (int i = 0; i < 3; i++)
			pActor->actorRot[i] = pStream->readFloatLE();

		pActor->scale = pStream->readFloatLE();
		pActor->matrixValid = pStream->readSint32LE();
		pActor->matrixValid = 0;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 3; j++)
				pActor->orientMatrix[i][j] = pStream->readFloatLE();
		}

		pActor->onScreen = pStream->readSint32LE();
		pActor->drawnRect.x = pStream->readSint32LE();
		pActor->drawnRect.y = pStream->readSint32LE();
		pActor->drawnRect.width = pStream->readSint32LE();
		pActor->drawnRect.height = pStream->readSint32LE();
		pStream->read(pActor->curSetName, 32);
		pActor->curSetName[31] = 0;

		int bHasExtra = pStream->readSint32LE();

		_pRes->resolveToSerialNumber(pActor->pColorMap);

		if (pActor->curSetName[0]) {
			char roomName[64];

			Common::strlcpy(roomName, pActor->curSetName, sizeof(roomName));
			pActor->curSetName[0] = 0;
			_pSputRoom->addActor(roomName, pActor);
		}

		pActor->pCostumeStack = nullptr;

		int numCostumes = pStream->readSint32LE();

		if (pStream->err() || pStream->eos() || numCostumes < 0) {
			warning("SputActorModule::restore(): invalid costume count");
			return 1;
		}

		for (int costumeIndex = 0; costumeIndex < numCostumes; costumeIndex++) {
			int savedInstance = pStream->readSint32LE();
			int aHeadJoint[3];
			float headTurnRate;
			float headYawLimit;
			float headPitchLimit;
			float headLookRef;
			int walkChore;
			int turnLeftChore;
			int turnRightChore;
			int restChore;
			int aChore[16];
			int mumbleChore;
			ResRef costumeRef;

			(void)savedInstance;

			for (int i = 0; i < 3; i++)
				aHeadJoint[i] = pStream->readSint32LE();

			headTurnRate = pStream->readFloatLE();
			headYawLimit = pStream->readFloatLE();
			headPitchLimit = pStream->readFloatLE();
			headLookRef = pStream->readFloatLE();
			walkChore = pStream->readSint32LE();
			turnLeftChore = pStream->readSint32LE();
			turnRightChore = pStream->readSint32LE();
			restChore = pStream->readSint32LE();

			for (int i = 0; i < 16; i++)
				aChore[i] = pStream->readSint32LE();

			mumbleChore = pStream->readSint32LE();

			memset(&costumeRef, 0, sizeof(costumeRef));
			costumeRef.serial = pStream->readSint32LE();
			_pRes->resolveToSerialNumber(&costumeRef);

			if (!costumeRef.valid) {
				warning("SputActorModule::restore(): Bad costume Reference");
				return 1;
			}

			CosCostume *pCostume = _pCos->ptrFromHandle(costumeRef.serial);
			int bPushed;

			if (costumeIndex)
				bPushed = pushCostume(pActor, pCostume->name, 0);
			else
				bPushed = setCostume(pActor, pCostume->name, 0);

			if (!bPushed) {
				warning("SputActorModule::restore(): failed to load the costume %s", pCostume->name);
				_pRes->removeReference(&costumeRef);
				return 1;
			}

			_pCos->restoreInstance(pActor->pCostumeStack->hCostumeInstance, pStream);

			CostumeNode *pNode = pActor->pCostumeStack;

			pNode->aHeadJoint[0] = aHeadJoint[0];
			pNode->aHeadJoint[1] = aHeadJoint[1];
			pNode->aHeadJoint[2] = aHeadJoint[2];
			pNode->headTurnRate = headTurnRate;
			pNode->headYawLimit = headYawLimit;
			pNode->headPitchLimit = headPitchLimit;
			pNode->headLookRef = headLookRef;
			pNode->walkChore = walkChore;
			pNode->turnLeftChore = turnLeftChore;
			pNode->turnRightChore = turnRightChore;
			pNode->restChore = restChore;
			memcpy(pNode->aChore, aChore, sizeof(pNode->aChore));
			pNode->mumbleChore = mumbleChore;
			_pRes->removeReference(&costumeRef);
		}

		if (pActor->pCostumeStack) {
			ModelInst *pModelInst = pActor->pCostumeStack->pHeadList;
			int numModels = pStream->readSint32LE();

			if (numModels > 0) {
				while (pModelInst) {
					pModelInst->bVisible = pStream->readSint32LE();
					pModelInst->resolveParentOffset = pStream->readSint32LE();
					pModelInst->parentMeshIdx = pStream->readSint32LE();

					if (pModelInst->pThing->type == 1) {
						if (!pModelInst->modelRef.valid) {
							warning("SputActorModule::restore(): Bad model Reference");
							return 1;
						}

						RdModel3 *pModel3 = static_cast<RdModel3 *>(pModelInst->modelRef.handle);

						for (uint i = 0; i < pModel3->numHNodes; i++)
							pModelInst->pThing->abMeshVisible[i] = pStream->readSint32LE();
					}

					int numTracks = pStream->readSint32LE();

					for (int i = 0; i < numTracks; i++) {
						RdPuppet *pPuppet = pModelInst->pThing->pPuppet;
						char keyframeName[33];

						pStream->read(keyframeName, 32);
						keyframeName[32] = 0;

						if (!pPuppet) {
							warning("SputActorModule::restore(): the model has no puppet for the track %s", keyframeName);
							return 1;
						}

						RdKeyframe *pKeyframe = static_cast<RdKeyframe *>(_pSputResource->alloc(kSputTagKeyf, keyframeName));

						if (pKeyframe) {
							int priority = pStream->readSint32LE();
							int priorityMasked = pStream->readSint32LE();
							int trackNum = _pRd->_puppet.addTrack(pPuppet, pKeyframe, priority, priorityMasked);
							uint status = pStream->readUint32LE();
							float curFrame = pStream->readFloatLE();
							float prevFrame = pStream->readFloatLE();
							float fade = pStream->readFloatLE();
							float fadeSpeed = pStream->readFloatLE();

							if (trackNum >= 0 && trackNum < 6) {
								pPuppet->aTracks[trackNum].status = status;
								pPuppet->aTracks[trackNum].curFrame = curFrame;
								pPuppet->aTracks[trackNum].prevFrame = prevFrame;
								pPuppet->aTracks[trackNum].fade = fade;
								pPuppet->aTracks[trackNum].fadeSpeed = fadeSpeed;
							} else {
								warning("SputActorModule::restore(): the keyframe %s could not be added to the puppet", keyframeName);
							}
						}
					}

					pModelInst = pModelInst->pNext;
				}
			}
		}

		if (bHasExtra) {
			pActor->pExtra = nullptr;
			allocateActorExtra(pActor);

			Extra *pExtra = pActor->pExtra;

			for (int i = 0; i < 3; i++)
				pExtra->lookCarrot[i] = pStream->readFloatLE();

			pExtra->bWalking = pStream->readSint32LE();
			pExtra->turnDir = pStream->readSint32LE();
			pExtra->resting = pStream->readSint32LE();
			pExtra->walkRate = pStream->readFloatLE();
			pExtra->turnRate = pStream->readFloatLE();
			pExtra->field_20 = pStream->readSint32LE();
			pExtra->offsetYaw = pStream->readFloatLE();
			pExtra->alignOff = pStream->readFloatLE();

			for (int i = 0; i < 3; i++)
				pExtra->walkTarget[i] = pStream->readFloatLE();

			for (int i = 0; i < 3; i++)
				pExtra->goal[i] = pStream->readFloatLE();

			pExtra->numPathPoints = pStream->readSint32LE();

			for (int i = 0; i < 30; i++) {
				for (int j = 0; j < 3; j++)
					pExtra->aPath[i][j] = pStream->readFloatLE();
			}

			pExtra->pathIndex = pStream->readSint32LE();

			int bHasPuck = pStream->readSint32LE();

			pExtra->inBoxes = pStream->readSint32LE();
			pExtra->walkDominate = pStream->readSint32LE();
			pExtra->reflectionAngle = pStream->readFloatLE();
			pExtra->talking = pStream->readSint32LE();
			pExtra->soundId = pStream->readSint32LE();
			pExtra->talkTimerMs = pStream->readSint32LE();
			pStream->read(pExtra->sayLineId, 12);
			pStream->read(pExtra->voiceName, 15);
			pExtra->voiceName[15] = 0;

			int sayTextSerial = pStream->readSint32LE();

			pExtra->phoneme = pStream->readSint32LE();
			pExtra->activeShadow = pStream->readSint32LE();
			pExtra->collisionMode = pStream->readSint32LE();
			pExtra->collisionScale = pStream->readFloatLE();
			pExtra->field_3F8 = pStream->readSint32LE();
			pExtra->clipPlaneActive = pStream->readSint32LE();

			for (int i = 0; i < 3; i++)
				pExtra->clipPlaneNorm[i] = pStream->readFloatLE();

			for (int i = 0; i < 3; i++)
				pExtra->clipPlanePt[i] = pStream->readFloatLE();

			pExtra->talkColor = pStream->readSint32LE();
			pExtra->timeScale = pStream->readFloatLE();
			pExtra->subClipNode = pStream->readSint32LE();

			if (bHasPuck) {
				pExtra->puck = nullptr;
				enablePuck(pActor, 1);

				if (pActor->pExtra->inBoxes)
					pActor->pExtra->puck->validate();
			}

			if (pActor->pExtra->talking) {
				for (int i = 0; i < 4; i++) {
					if (!_apTalkingActors[i]) {
						_apTalkingActors[i] = pActor;
						_numActorsTalking++;
						break;
					}
				}
			}

			if (sayTextSerial) {
				pActor->pExtra->pSayText = _pSputText->findSerialNumber(sayTextSerial);

				if (pActor->pExtra->pSayText)
					pActor->pExtra->pSayText->pActor = pActor;
			}

			resetShadows(pActor);

			for (int i = 0; i < 6; i++) {
				ActorShadow *pShadow = &pActor->pExtra->aShadows[i];

				pShadow->bActive = pStream->readSint32LE();
				pShadow->side = pStream->readSint32LE();

				for (int j = 0; j < 3; j++)
					pShadow->normal[j] = pStream->readFloatLE();

				pShadow->planeD = pStream->readFloatLE();

				for (int j = 0; j < 3; j++)
					pShadow->lightPos[j] = pStream->readFloatLE();

				int numClipPlanes = pStream->readSint32LE();

				if (numClipPlanes < 0 || numClipPlanes >= 12) {
					warning("SputActorModule::restore(): invalid shadow clip plane count %d", numClipPlanes);
					return 1;
				}

				for (int j = 0; j < numClipPlanes; j++) {
					char planeName[33];

					planeName[0] = 0;
					pStream->read(planeName, 32);
					planeName[32] = 0;
					pShadow->apClipPlane[j] = _pWalkSystem->planeNamed(planeName);
				}
			}
		}

		if (pStream->err() || pStream->eos()) {
			warning("SputActorModule::restore(): failed to read the actor %d", actorIndex);
			return 1;
		}
	}

	_pSelectedActor = nullptr;

	int selectedIndex = pStream->readSint32LE();

	if (selectedIndex != -1) {
		if (selectedIndex >= 0 && selectedIndex < _maxActors)
			_pSelectedActor = &_aActors[selectedIndex];
		else
			warning("SputActorModule::restore(): invalid selected actor %d", selectedIndex);
	}

	return 0;
}

} // End of namespace Grim
