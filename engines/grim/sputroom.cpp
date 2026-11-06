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

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/puck.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_conffile.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_lbm.h"
#include "engines/grim/walkplane.h"

namespace Grim {

SputRoom::SputRoom(RenderDroid *pRd, Res *pRes, SputResource *pSputResource, StdDisplay *pStdDisplay, StdBitmap *pStdBitmap, StdLbm *pStdLbm,
		StdConffile *pStdConffile, WalkSystem *pWalkSystem) : _pRd(pRd), _pRes(pRes), _pSputResource(pSputResource), _pStdDisplay(pStdDisplay),
		_pStdBitmap(pStdBitmap), _pStdLbm(pStdLbm), _pStdConffile(pStdConffile), _pWalkSystem(pWalkSystem), _pSputRender(nullptr),
		_pGrim(nullptr), _pSputActor(nullptr), _aActors(nullptr), _pHS(nullptr), _bInitialized(0), _bStarted(0), _pSoundList(nullptr), _pActorNodeList(nullptr),
		_pLoadingRoom(nullptr), _bUpdateLoop(0), _numSections(0) {
	memset(&_currentRoomRef, 0, sizeof(_currentRoomRef));
	memset(_currentRoomName, 0, sizeof(_currentRoomName));

	for (int i = 0; i < 32; i++) {
		memset(_aSections[i].name, 0, sizeof(_aSections[i].name));
		_aSections[i].handler = nullptr;
	}
}

StdVBuffer *SputRoom::getSetupBackground(Room *pRoom, int setupIndex) {
	RoomSetup *pSetup = &pRoom->pSetups[setupIndex];
	ResRef *pBitmapRef = &pSetup->bitmapRef;

	if (pBitmapRef->valid) {
		_pRes->touch(pBitmapRef->handle);
	} else {
		if (scumm_stricmp(pSetup->aBitmapName, "<none>")) {
			void *handle = _pSputResource->alloc(kSputTagBknd, pSetup->aBitmapName);

			if (handle)
				_pRes->setReference(pBitmapRef, handle);
		}

		if (!pBitmapRef->valid) {
			void *handle = _pSputResource->alloc(kSputTagBknd, "dfltroom.bm");

			_pRes->setReference(pBitmapRef, handle);

			if (!pBitmapRef->valid)
				return nullptr;

			Common::strlcpy(pSetup->aBitmapName, "<none>", 32);
			pSetup->aZBufferName[0] = 0;
		}
	}

	StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pBitmapRef->handle);

	if (pBitmap->unk_104 < 0 || (uint)pBitmap->unk_104 >= pBitmap->numFrames) {
		warning("SputRoom::getSetupBackground(): frame %d of %s is out of range", pBitmap->unk_104, pSetup->aBitmapName);
		return nullptr;
	}

	return pBitmap->apFrames[pBitmap->unk_104];
}

StdVBuffer *SputRoom::getSetupZBackground(Room *pRoom, int setupIndex) {
	RoomSetup *pSetup = &pRoom->pSetups[setupIndex];
	ResRef *pZBufferRef = &pSetup->zbufferRef;

	if (pZBufferRef->valid) {
		_pRes->touch(pZBufferRef->handle);
	} else if (pSetup->aZBufferName[0]) {
		if (strcmp(pSetup->aZBufferName, "<none>")) {
			void *handle = _pSputResource->alloc(kSputTagZbuf, pSetup->aZBufferName);

			if (handle)
				_pRes->setReference(pZBufferRef, handle);
		}
	}

	if (pZBufferRef->valid)
		return static_cast<StdVBuffer *>(pZBufferRef->handle);

	return nullptr;
}

void SputRoom::relocateSounds() {
	freeSoundTable();

	for (RoomSound *pSound = _pSoundList; pSound; pSound = pSound->pNext)
		setupSoundPosition(pSound);
}

void SputRoom::setSoundParameters(Room *pRoom, float maxDist, byte minVol, byte maxVol, float panFactor, float maxRadius) {
	pRoom->minVol = minVol;
	pRoom->maxDist = maxDist;
	pRoom->maxVol = maxVol;
	pRoom->panFactor = panFactor;
	pRoom->maxRadius = maxRadius;
}

void SputRoom::getSoundParameters(Room *pRoom, float *maxDist, byte *minVol, byte *maxVol, float *panFactor, float *maxRadius) {
	*maxDist = pRoom->maxDist;
	*minVol = pRoom->minVol;
	*maxVol = pRoom->maxVol;
	*panFactor = pRoom->panFactor;
	*maxRadius = pRoom->maxRadius;
}

int SputRoom::setSoundPosition(const char *soundName, float *pPos, int minVol, int maxVol, float maxDist, float radius) {
	int result = g_imuse->soundsIsSoundPlaying(soundName);

	if (result) {
		RoomSound *pSound = allocSoundTable(soundName, pPos, minVol, maxVol, maxDist, radius);

		return setupSoundPosition(pSound);
	}

	return result;
}

void SputRoom::setCurrentRoom(Room *pRoom) {
	if (!_currentRoomRef.valid || _currentRoomRef.handle != pRoom) {
		if (_currentRoomRef.valid) {
			unReference(static_cast<Room *>(_currentRoomRef.handle));
			_pRes->removeReference(&_currentRoomRef);
			_currentRoomName[0] = 0;
		}

		if (pRoom) {
			_pRes->setReference(&_currentRoomRef, pRoom);

			if (!_currentRoomRef.valid)
				error("SputRoom::setCurrentRoom(): Bad room Reference");

			loadResources(static_cast<Room *>(_currentRoomRef.handle));
			Common::strlcpy(_currentRoomName, pRoom->name, 32);
			setSoundParameters(pRoom, 8.0f, 20, 127, 1.5f, 0.8f);
		}
	}
}

Room *SputRoom::getCurrentRoom() {
	if (_currentRoomRef.valid)
		return static_cast<Room *>(_currentRoomRef.handle);

	return nullptr;
}

const char *SputRoom::getCurrentRoomName() {
	return _currentRoomName;
}

int SputRoom::save(Common::WriteStream *pStream) {
	ResNode *pNext;
	void *handle;
	int numRooms = 0;

	_pRes->allocatedIterFirst(&pNext, &handle, kSputTagSet);

	while (pNext) {
		if (_pRes->countDependents(handle) > 0)
			numRooms++;

		_pRes->allocatedIterNext(&pNext, &handle, kSputTagSet);
	}

	pStream->writeSint32LE(numRooms);

	if (pStream->err()) {
		warning("SputRoom::save(): failed to write the room count");
		return 1;
	}

	_pRes->allocatedIterFirst(&pNext, &handle, kSputTagSet);

	while (pNext) {
		if (_pRes->countDependents(handle)) {
			Room *pRoom = static_cast<Room *>(handle);

			pStream->writeSint32LE((int)(intptr)handle);
			pStream->writeSint32LE(pRoom->curSetup);
			pStream->writeSint32LE(pRoom->roomRef.valid);

			for (int i = 0; i < (int)pRoom->numSetups; i++) {
				RoomSetup *pSetup = &pRoom->pSetups[i];
				int numStates = 0;

				for (SputObjectState *pState = pSetup->ObjectStates; pState; pState = pState->pNext)
					numStates++;

				pStream->writeSint32LE(numStates);

				for (SputObjectState *pState = pSetup->ObjectStates; pState; pState = pState->pNext) {
					pStream->writeSint32LE(pState->type);
					pStream->write(pState->colorImage.aFileName, 32);
					pStream->write(pState->zImage.aFileName, 32);
					pStream->writeSint32LE(pState->colorImage.copyMode);

					if (!pState->colorImage.bitmapRef.valid)
						error("SputRoom::save(): Bad Bitmap Reference");

					StdBitmapStruct *pBitmap = static_cast<StdBitmapStruct *>(pState->colorImage.bitmapRef.handle);

					pStream->writeSint32LE(pBitmap->unk_104);
					pStream->writeSint32LE(pState->zImage.curFrame);
				}
			}
		}

		_pRes->allocatedIterNext(&pNext, &handle, kSputTagSet);
	}

	Room *pCurrentRoom = getCurrentRoom();

	pStream->writeSint32LE((int)(intptr)pCurrentRoom);

	if (pCurrentRoom) {
		pStream->writeByte(pCurrentRoom->minVol);
		pStream->writeByte(pCurrentRoom->maxVol);
		pStream->writeFloatLE(pCurrentRoom->maxDist);
		pStream->writeFloatLE(pCurrentRoom->panFactor);
		pStream->writeFloatLE(pCurrentRoom->maxRadius);

		for (WalkPlane *pPlane = _pWalkSystem->getHead(); pPlane; pPlane = pPlane->pNext) {
			pStream->writeSint32LE(pPlane->number);
			pStream->writeSint32LE(pPlane->bActive);
			pStream->writeSint32LE(pPlane->type);
			pStream->writeSint32LE(pPlane->bEnabled);
			pStream->writeFloatLE(pPlane->tolerance);
		}
	}

	if (pStream->err()) {
		warning("SputRoom::save(): failed to write the room state");
		return 1;
	}

	return 0;
}

int SputRoom::restore(Common::ReadStream *pStream) {
	int numRooms = pStream->readSint32LE();

	if (pStream->err() || pStream->eos()) {
		warning("SputRoom::restore(): failed to read the room count");
		return 1;
	}

	for (int i = 0; i < numRooms; i++) {
		int serial = pStream->readSint32LE();

		if (pStream->err() || pStream->eos()) {
			warning("SputRoom::restore(): failed to read room %d of %d", i + 1, numRooms);
			return 1;
		}

		Room *pRoom = static_cast<Room *>(_pRes->handleFromSerialNumber(serial));

		if (!pRoom) {
			warning("SputRoom::restore(): room %d not found", serial);
			return 1;
		}

		pRoom->curSetup = pStream->readSint32LE();

		if (_currentRoomRef.valid)
			_pRes->removeReference(&_currentRoomRef);

		_pRes->setReference(&_currentRoomRef, pRoom);

		if (!_currentRoomRef.valid)
			error("SputRoom::restore(): Bad room Reference");

		loadResources(static_cast<Room *>(_currentRoomRef.handle));
		Common::strlcpy(_currentRoomName, pRoom->name, 32);

		int bRoomRef = pStream->readSint32LE();

		if (bRoomRef)
			_pRes->setReference(&pRoom->roomRef, pRoom);

		for (int setupIndex = 0; setupIndex < (int)pRoom->numSetups; setupIndex++) {
			RoomSetup *pSetup = &pRoom->pSetups[setupIndex];
			int numStates = pStream->readSint32LE();

			for (int j = 0; j < numStates; j++) {
				char bitmapName[32];
				char zbufferName[32];
				int objectType = pStream->readSint32LE();

				pStream->read(bitmapName, 32);
				pStream->read(zbufferName, 32);

				int copyMode = pStream->readSint32LE();
				int colorFrame = pStream->readSint32LE();
				int zFrame = pStream->readSint32LE();

				if (pStream->err() || pStream->eos()) {
					warning("SputRoom::restore(): failed to read object state %d of setup %d", j + 1, setupIndex);
					return 1;
				}

				if (bitmapName[31] || zbufferName[31]) {
					warning("SputRoom::restore(): object state name is not terminated");
					bitmapName[31] = 0;
					zbufferName[31] = 0;
				}

				addObjectState(setupIndex, objectType, bitmapName, zbufferName, copyMode);

				SputObjectState *pHead = pSetup->ObjectStates;

				if (!pHead) {
					warning("SputRoom::restore(): could not add object state %d of setup %d", j + 1, setupIndex);
					continue;
				}

				if (pHead->colorImage.bitmapRef.valid)
					static_cast<StdBitmapStruct *>(pHead->colorImage.bitmapRef.handle)->unk_104 = colorFrame;

				if (pHead->zImage.bitmapRef.valid)
					static_cast<StdBitmapStruct *>(pHead->zImage.bitmapRef.handle)->unk_104 = zFrame;
			}

			for (int k = 0; k < numStates; k++) {
				if (!pSetup->ObjectStates) {
					warning("SputRoom::restore(): setup %d has no object states to reorder", setupIndex);
					break;
				}

				pSetup->ObjectStates = pSetup->ObjectStates->moveToBack(pSetup->ObjectStates);
			}
		}
	}

	int currentSerial = pStream->readSint32LE();
	Room *pCurrentRoom = static_cast<Room *>(_pRes->handleFromSerialNumber(currentSerial));

	setCurrentRoom(pCurrentRoom);

	if (pCurrentRoom) {
		_pGrim->makeCurrentSetupLib(pCurrentRoom->curSetup);

		_pWalkSystem->setCollection(pCurrentRoom->name);

		pCurrentRoom->minVol = pStream->readByte();
		pCurrentRoom->maxVol = pStream->readByte();
		pCurrentRoom->maxDist = pStream->readFloatLE();
		pCurrentRoom->panFactor = pStream->readFloatLE();
		pCurrentRoom->maxRadius = pStream->readFloatLE();

		for (WalkPlane *pPlane = _pWalkSystem->getHead(); pPlane; pPlane = pPlane->pNext) {
			int number = pStream->readSint32LE();

			if (number == pPlane->number) {
				pPlane->bActive = pStream->readSint32LE();
				pPlane->setType(pStream->readSint32LE());
				pPlane->bEnabled = pStream->readSint32LE();

				float tolerance = pStream->readFloatLE();

				if (tolerance == 0.0f)
					pPlane->tolerance = 0.005f;
				else
					pPlane->tolerance = tolerance;
			}
		}
	}

	if (pStream->err() || pStream->eos()) {
		warning("SputRoom::restore(): failed to read the current room state");
		return 1;
	}

	return 0;
}

int SputRoom::initialize(SysStruct *pHS) {
	_pHS = pHS;
	memset(&_currentRoomRef, 0, sizeof(_currentRoomRef));
	_bInitialized = 1;

	return 0;
}

StdVBuffer *SputRoom::loadZBuffer(const char *fileName) {
	return loadZBufferEntry(fileName);
}

StdVBuffer *SputRoom::loadZBufferEntry(const char *fileName) {
	Common::String path(fileName);
	StdBitmapStruct *pBitmap = _pStdBitmap->loadLbm(path, 0, 0);

	if (!pBitmap)
		return nullptr;

	if (!pBitmap->apFrames || !pBitmap->numFrames) {
		warning("SputRoom::loadZBufferEntry(): bitmap %s has no frames", fileName);
		_pStdBitmap->unloadBitmap(pBitmap);
		return nullptr;
	}

	StdVBuffer *pFrame = pBitmap->apFrames[0];

	delete[] pBitmap->apFrames;
	pBitmap->apFrames = nullptr;
	pBitmap->numFrames = 0;
	_pStdBitmap->unloadBitmap(pBitmap);

	return pFrame;
}

void SputRoom::freeZBuffer(StdVBuffer *&pZBuffer) {
	freeZBufferEntry(pZBuffer);
}

void SputRoom::freeZBufferEntry(StdVBuffer *&pZBuffer) {
	_pStdDisplay->vBufferFree(pZBuffer);
}

Room *SputRoom::loadRoom(const char *fileName) {
	Room *pRoom = new Room;

	memset(pRoom, 0, sizeof(Room));
	newRoomEntry(pRoom);

	if (loadRoomEntry(fileName, pRoom)) {
		RoomActorNode *pActorNode = findActorNode(fileName);

		if (!pActorNode)
			pActorNode = allocActorNode(fileName);

		pRoom->pActorNode = pActorNode;

		for (RoomActorLink *pLink = pActorNode->pActors; pLink; pLink = pLink->pNext) {
			Extra *pExtra = _aActors[pLink->actorIndex].pExtra;

			if (pExtra) {
				Puck *pPuck = pExtra->puck;

				if (pPuck) {
					if (pExtra->inBoxes)
						pPuck->validate();
				}
			}
		}

		return pRoom;
	}

	freeRoom(pRoom);

	return nullptr;
}

void SputRoom::newRoomEntry(Room *pRoom) {
	memset(pRoom, 0, sizeof(Room));
	pRoom->numLights = 0;
	pRoom->numSetups = 0;
	pRoom->pSetups = nullptr;
	pRoom->curSetup = 0;
	pRoom->field_88 = 0;
	pRoom->numcolormaps = 1;
	pRoom->apColormaps = nullptr;
	pRoom->colormapName[0] = 0;
	Common::strlcpy(pRoom->colormapName, "<none>", 32);
	Common::strlcpy(pRoom->name, "", 32);
	pRoom->structSize = 6920;
}

int SputRoom::loadRoomEntry(const char *fileName, Room *pRoom) {
	_pLoadingRoom = pRoom;
	Common::strlcpy(pRoom->name, _pStdLbm->stripPath(fileName), 128);

	return loadSections(fileName) != 0;
}

void SputRoom::unReference(Room *pRoom) {
	if (!pRoom->roomRef.valid) {
		if (pRoom->apColormaps) {
			for (int i = 0; i < pRoom->numcolormaps; i++)
				_pRes->removeReference(&pRoom->apColormaps[i]);
		}

		for (int i = 0; i < (int)pRoom->numSetups; i++) {
			_pRes->removeReference(&pRoom->pSetups[i].bitmapRef);
			_pRes->removeReference(&pRoom->pSetups[i].zbufferRef);

			while (pRoom->pSetups[i].ObjectStates) {
				SputObjectState *pObjectState = pRoom->pSetups[i].ObjectStates;

				pRoom->pSetups[i].ObjectStates = pObjectState->pNext;
				pObjectState->unLoadResources();
				delete pObjectState;
			}
		}
	}
}

void SputRoom::freeRoom(Room *&pRoom) {
	freeRoomEntry(pRoom);
	delete pRoom;
	pRoom = nullptr;
}

void SputRoom::freeRoomEntry(Room *pRoom) {
	unReference(pRoom);

	if (pRoom->apColormaps) {
		delete[] pRoom->apColormaps;
		pRoom->apColormaps = nullptr;
	}

	freeRoomSetups(pRoom);
	_pWalkSystem->killCollection(pRoom->name);

	RoomActorNode *pActorNode = pRoom->pActorNode;

	if (pActorNode) {
		for (RoomActorLink *pLink = pActorNode->pActors; pLink; pLink = pActorNode->pActors) {
			_pSputActor->resetShadows(&_aActors[pLink->actorIndex]);
			removeActor(pRoom->name, &_aActors[pLink->actorIndex]);

			if (pActorNode->pActors == pLink) {
				warning("SputRoom::freeRoomEntry(): could not remove the actor %d from room %s", pLink->actorIndex, pRoom->name);
				break;
			}
		}

		freeActorNode(pActorNode);
		pRoom->pActorNode = nullptr;
	}
}

void SputRoom::freeRoomSetups(Room *pRoom) {
	if (pRoom->numSetups) {
		for (uint i = 0; i < pRoom->numSetups; i++)
			freeSetupEntry(pRoom, &pRoom->pSetups[i]);

		if (pRoom->numSetups && pRoom->pSetups)
			delete[] pRoom->pSetups;
	}

	pRoom->pSetups = nullptr;
	pRoom->numSetups = 0;
}

void SputRoom::freeSetupEntry(Room *pRoom, RoomSetup *pSetup) {
	if (pSetup->camera.prdCamera)
		_pRd->_camera.destroy(pSetup->camera.prdCamera);

	if (pSetup->camera.pThing)
		_pRd->_thing.destroy(pSetup->camera.pThing);

	pSetup->ObjectStates = nullptr;
}

void SputRoom::freeSoundTable() {
	RoomSound *pSound = _pSoundList;
	RoomSound *pPrev = nullptr;

	while (pSound) {
		if (g_imuse->soundsIsSoundPlaying(pSound->soundName)) {
			pPrev = pSound;
			pSound = pSound->pNext;
		} else {
			RoomSound *pFree = pSound;

			pSound = pSound->pNext;

			if (pFree == _pSoundList) {
				_pSoundList = pSound;
				pPrev = nullptr;
			} else {
				pPrev->pNext = pSound;
			}

			delete pFree;
		}
	}
}

RoomSound *SputRoom::allocSoundTable(const char *soundName, float *pPos, int minVol, int maxVol, float maxDist, float radius) {
	freeSoundTable();

	RoomSound *pSound = new RoomSound;

	pSound->soundName = soundName;
	pSound->pNext = _pSoundList;
	pSound->pos[0] = pPos[0];
	pSound->pos[1] = pPos[1];
	pSound->pos[2] = pPos[2];
	pSound->minVol = minVol;
	pSound->maxVol = maxVol;
	pSound->maxDist = maxDist;
	pSound->radius = radius;
	_pSoundList = pSound;

	return pSound;
}

int SputRoom::setupSoundPosition(RoomSound *pSound) {
	Room *pRoom = getCurrentRoom();

	if (!pRoom)
		return 0;

	RoomSetup *pSetup = &pRoom->pSetups[pRoom->curSetup];
	RdVector3 vec;

	vec[0] = (float)((double)pSound->pos[0] - pSetup->camera.position[0]);
	vec[1] = (float)((double)pSound->pos[1] - pSetup->camera.position[1]);
	vec[2] = (float)((double)pSound->pos[2] - pSetup->camera.position[2]);

	double distance = (double)_pRd->normalize3QuickAcc(vec) - pSound->radius;

	if (distance >= 0.0) {
		uint minVol = (uint)pSound->minVol;
		uint maxVol = (uint)pSound->maxVol;
		double value = (double)(uint)(maxVol - minVol) / pSound->maxDist * ((double)pSound->maxDist - distance) + (double)minVol;
		uint volume = 0;

		if (value > -9.2e18 && value < 9.2e18)
			volume = (uint)(int64)value;
		else
			warning("SputRoom::setupSoundPosition(): volume out of range");

		if (volume < minVol)
			volume = minVol;
		else if (volume > maxVol)
			volume = maxVol;

		g_imuse->soundsSetParam(pSound->soundName, IMUSE_P_VOLUME, (int)volume);
	}

	double pan = ((double)pSetup->camera.posMatrix[0][0] * vec[0] + (double)pSetup->camera.posMatrix[0][1] * vec[1] +
			(double)pSetup->camera.posMatrix[0][2] * vec[2]) * pRoom->panFactor;

	if (!(pan >= -1.0))
		pan = -1.0;
	else if (pan > 1.0)
		pan = 1.0;

	uint panValue = (uint)(int64)((pan + 1.0) * 64.0);

	if (panValue > 127)
		panValue = 127;

	g_imuse->soundsSetParam(pSound->soundName, IMUSE_P_PAN, (int)panValue);

	return 1;
}

int SputRoom::loadResources(Room *pRoom) {
	pRoom->structSize = 6920;

	g_imuse->soundsFloodMusicBuffer();

	if (pRoom->numcolormaps && !pRoom->apColormaps) {
		warning("SputRoom::loadResources(): room %s has no colormap references", pRoom->name);
		return 0;
	}

	for (uint i = 0; i < (uint)pRoom->numcolormaps; i++) {
		ResRef *pColormapRef = &pRoom->apColormaps[i];

		if (pColormapRef->valid) {
			_pRes->touch(pColormapRef->handle);
		} else {
			if (scumm_stricmp(pRoom->colormapName, "<none>")) {
				void *handle = _pSputResource->alloc(kSputTagCmap, pRoom->colormapName);

				_pRes->setReference(pColormapRef, handle);
			}

			if (!pColormapRef->valid) {
				void *handle = _pSputResource->alloc(kSputTagCmap, "dfltcmp.cmp");

				_pRes->setReference(pColormapRef, handle);

				if (!pColormapRef->valid)
					return 0;
			}
		}
	}

	for (uint i = 0; i < pRoom->numSetups; i++) {
		RoomSetup *pSetup = &pRoom->pSetups[i];

		if (!_bUpdateLoop) {
			ResRef *pBitmapRef = &pSetup->bitmapRef;

			if (pBitmapRef->valid) {
				_pRes->touch(pBitmapRef->handle);
			} else {
				if (scumm_stricmp(pSetup->aBitmapName, "<none>")) {
					void *handle = _pSputResource->alloc(kSputTagBknd, pSetup->aBitmapName);

					_pRes->setReference(pBitmapRef, handle);
				}

				if (!pBitmapRef->valid) {
					void *handle = _pSputResource->alloc(kSputTagBknd, "dfltroom.bm");

					_pRes->setReference(pBitmapRef, handle);

					if (!pBitmapRef->valid)
						return 0;

					Common::strlcpy(pSetup->aBitmapName, "<none>", 32);
					pSetup->aZBufferName[0] = 0;
				}
			}

			if (pSetup->aZBufferName[0]) {
				ResRef *pZBufferRef = &pSetup->zbufferRef;

				if (!pZBufferRef->valid) {
					void *handle = _pSputResource->alloc(kSputTagZbuf, pSetup->aZBufferName);

					if (handle)
						_pRes->setReference(pZBufferRef, handle);
				}

				if (pZBufferRef->valid)
					_pRes->touch(pZBufferRef->handle);
			}
		}

		if (!pSetup->camera.prdCamera) {
			pSetup->camera.prdCamera = _pRd->_camera.create(pSetup->camera.fov, 1, pSetup->camera.nearClip, pSetup->camera.farClip, 1.0f);

			if (!pSetup->camera.prdCamera)
				return 0;

			pSetup->camera.pThing = _pRd->_thing.create(0);
			_pRd->_thing.setCamera(pSetup->camera.pThing, pSetup->camera.prdCamera);
			_pRd->lookAt(pSetup->camera.posMatrix, pSetup->camera.position, pSetup->camera.interest, pSetup->camera.roll);

			for (uint j = 0; j < pRoom->numLights; j++) {
				RoomLight *pLight = &pRoom->aLights[j];

				_pRd->_light.setAngles(&pLight->light, pLight->light.angleX, pLight->light.angleY);
				_pRd->_camera.addLight(pSetup->camera.prdCamera, &pLight->light, &pLight->position);
			}
		}
	}

	return 1;
}

int SputRoom::terminate() {
	_pHS = nullptr;

	if (_bInitialized)
		_bInitialized = 0;

	return 0;
}

int SputRoom::startup() {
	if (_bStarted)
		return 1;

	return readSections();
}

int SputRoom::shutdown() {
	RoomActorNode *pNode = _pActorNodeList;

	while (pNode) {
		RoomActorLink *pLink = pNode->pActors;

		while (pLink) {
			RoomActorLink *pNextLink = pLink->pNext;

			delete pLink;
			pLink = pNextLink;
		}

		RoomActorNode *pNextNode = pNode->pNext;

		delete pNode;
		pNode = pNextNode;
	}

	_pActorNodeList = nullptr;

	if (_bStarted) {
		while (_pSoundList) {
			RoomSound *pNextSound = _pSoundList->pNext;

			delete _pSoundList;
			_pSoundList = pNextSound;
		}
	}

	_bStarted = 0;

	return 0;
}

RoomLight *SputRoom::getLight(Room *pRoom, int index) {
	Room *pCurrentRoom = pRoom;

	if (!pRoom)
		pCurrentRoom = getCurrentRoom();

	if (!pCurrentRoom) {
		warning("SputRoom::getLight(): no current room");
		return nullptr;
	}

	if (index < 0 || index >= (int)pCurrentRoom->numLights)
		return nullptr;

	return &pCurrentRoom->aLights[index];
}

int SputRoom::findLightByName(Room *pRoom, const char *name) {
	Room *pCurrentRoom = pRoom;

	if (!pRoom)
		pCurrentRoom = getCurrentRoom();

	if (!pCurrentRoom) {
		warning("SputRoom::findLightByName(): no current room");
		return -1;
	}

	int index = 0;

	if ((int)pCurrentRoom->numLights > 0) {
		do {
			if (!scumm_stricmp(name, pCurrentRoom->aLights[index].aName))
				break;

			index++;
		} while (index < (int)pCurrentRoom->numLights);
	}

	if (index >= (int)pCurrentRoom->numLights)
		return -1;

	return index;
}

SputObjectState *SputRoom::addObjectState(int setupIndex, int objectType, const char *bitmapName, const char *zbufferName, int copyMode) {
	Room *pRoom = getCurrentRoom();

	if (!pRoom || setupIndex < 0 || (uint)setupIndex >= pRoom->numSetups) {
		warning("SputRoom::addObjectState(): no room or invalid setup %d", setupIndex);
		return nullptr;
	}

	RoomSetup *pSetup = &pRoom->pSetups[setupIndex];

	pSetup->ObjectStates = new SputObjectState(pSetup->ObjectStates, bitmapName, zbufferName, objectType, _pRes, _pSputResource,
			_pStdDisplay, _pStdBitmap, _pSputRender);
	pSetup->ObjectStates->loadResources();
	pSetup->ObjectStates->setupIndex = setupIndex;

	if (copyMode <= -1)
		pSetup->ObjectStates->setCopyMode(objectType == 3);
	else
		pSetup->ObjectStates->setCopyMode(copyMode);

	return pSetup->ObjectStates;
}

void SputRoom::removeObjectState(SputObjectState *pObj) {
	if (pObj) {
		Room *pRoom = getCurrentRoom();

		if (!pRoom) {
			warning("SputRoom::removeObjectState(): no current room");
			return;
		}

		RoomSetup *pSetup = &pRoom->pSetups[pObj->setupIndex];
		SputObjectState *pHead = pSetup->ObjectStates;
		SputObjectState *pNext;

		if (pHead == pObj)
			pNext = pObj->pNext;
		else
			pNext = pHead;

		delete pObj;
		pSetup->ObjectStates = pNext;
	}
}

void SputRoom::removeActor(const char *roomName, SputActor *pActor) {
	RoomActorNode *pActorNode = findActorNode(roomName);

	if (pActorNode) {
		pActor->onScreen = 0;

		RoomActorLink *pLink = pActorNode->pActors;

		while (pLink) {
			if (&_aActors[pLink->actorIndex] == pActor)
				break;

			pLink = pLink->pNext;
		}

		if (pLink) {
			Extra *pExtra = pActor->pExtra;

			if (pExtra) {
				Puck *pPuck = pExtra->puck;

				if (pPuck)
					pPuck->pPlane = nullptr;
			}

			if (pLink->pNext)
				pLink->pNext->pPrev = pLink->pPrev;

			if (pLink->pPrev)
				pLink->pPrev->pNext = pLink->pNext;

			if (pActorNode->pActors == pLink)
				pActorNode->pActors = pLink->pNext;

			delete pLink;
			pActor->curSetName[0] = 0;
		}
	}
}

void SputRoom::addActor(const char *roomName, SputActor *pActor) {
	if (!pActor || !_aActors) {
		warning("SputRoom::addActor(): no actor or no actor array");
		return;
	}

	if (roomName && !strcmp(roomName, pActor->curSetName))
		return;

	removeActor(pActor->curSetName, pActor);

	if (roomName) {
		RoomActorNode *pActorNode = findActorNode(roomName);

		if (!pActorNode)
			pActorNode = allocActorNode(roomName);

		RoomActorLink *pLink = pActorNode->pActors;

		while (pLink) {
			if (&_aActors[pLink->actorIndex] == pActor)
				return;

			pLink = pLink->pNext;
		}

		pLink = new RoomActorLink;
		pLink->actorIndex = 0;
		pLink->pPrev = nullptr;
		pLink->pNext = nullptr;
		pLink->actorIndex = (int)(pActor - _aActors);
		pLink->pNext = pActorNode->pActors;

		if (pActorNode->pActors)
			pActorNode->pActors->pPrev = pLink;

		pActorNode->pActors = pLink;

		if (strlen(roomName) >= 32)
			warning("SputRoom::addActor(): room name %s is too long", roomName);

		Common::strlcpy(pActor->curSetName, roomName, 32);
	} else {
		pActor->onScreen = 0;
	}
}

RoomActorLink *SputRoom::getRoomActors(const char *roomName) {
	RoomActorNode *pActorNode = findActorNode(roomName);

	if (pActorNode)
		return pActorNode->pActors;

	return nullptr;
}

RoomActorNode *SputRoom::findActorNode(const char *roomName) {
	RoomActorNode *pNode = _pActorNodeList;

	if (!roomName || !pNode)
		return nullptr;

	while (scumm_stricmp(pNode->name, roomName)) {
		pNode = pNode->pNext;

		if (!pNode)
			return nullptr;
	}

	return pNode;
}

RoomActorNode *SputRoom::allocActorNode(const char *roomName) {
	RoomActorNode *pNode = new RoomActorNode;

	memset(pNode, 0, sizeof(RoomActorNode));

	if (strlen(roomName) >= 128)
		warning("SputRoom::allocActorNode(): room name %s is too long", roomName);

	Common::strlcpy(pNode->name, roomName, 128);
	pNode->pNext = _pActorNodeList;
	_pActorNodeList = pNode;

	return pNode;
}

void SputRoom::freeActorNode(RoomActorNode *pNode) {
	RoomActorNode *pCurrent = _pActorNodeList;
	RoomActorNode *pPrev = nullptr;

	if (pCurrent) {
		do {
			if (pCurrent == pNode)
				break;

			pPrev = pCurrent;
			pCurrent = pCurrent->pNext;
		} while (pCurrent);

		if (pCurrent) {
			if (pPrev)
				pPrev->pNext = pCurrent->pNext;
			else
				_pActorNodeList = pCurrent->pNext;

			delete pCurrent;
		}
	}
}

} // End of namespace Grim
