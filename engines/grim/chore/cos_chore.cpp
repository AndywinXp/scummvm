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

namespace Grim {

int Cos::instStopChores(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instStopChores(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numChores; i++) {
		CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[i];
		uint32 choreIndex = pChoreInst->choreIndex;
		pChoreInst->state = 0;
		pChoreInst->time = 0;

		if (choreIndex >= (uint32)pCostume->numChores) {
			warning("Cos::instStopChores(): chore instance %d refers to the invalid chore %u", i, choreIndex);
			continue;
		}

		CosChore *pChore = &pCostume->aChores[choreIndex];

		for (int t = 0; t < pChore->numTracks; t++) {
			int compIndex = pChore->aTracks[t].compIndex;
			CosComponentClass *pClass = &_aClasses[pCostume->aComponents[compIndex].classIdx];
			pClass->pHandler->stop(pInstance->aInstanceData[compIndex]);
		}
	}

	return 1;
}

int Cos::instStopChore(int hInst, int choreIndex) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instStopChore(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	if (choreIndex < 0 || choreIndex >= pCostume->numChores)
		return 0;

	CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[choreIndex];
	pChoreInst->state = 0;
	pChoreInst->time = 0;

	if (pChoreInst->choreIndex >= (uint32)pCostume->numChores) {
		warning("Cos::instStopChore(): chore instance %d refers to the invalid chore %u", choreIndex, pChoreInst->choreIndex);
		return 1;
	}

	CosChore *pChore = &pCostume->aChores[pChoreInst->choreIndex];

	for (int t = 0; t < pChore->numTracks; t++) {
		int compIndex = pChore->aTracks[t].compIndex;
		CosComponentClass *pClass = &_aClasses[pCostume->aComponents[compIndex].classIdx];
		pClass->pHandler->stop(pInstance->aInstanceData[compIndex]);
	}

	return 1;
}

int Cos::instPauseChores(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instPauseChores(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numChores; i++)
		pInstance->aChoreInstances[i].state &= ~kCosChoreStatePlaying;

	return 1;
}

int Cos::instPauseChore(int hInst, int choreIndex) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instPauseChore(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];

	if (choreIndex < 0 || choreIndex >= _aCostumes[pInstance->costume].numChores)
		return 0;

	pInstance->aChoreInstances[choreIndex].state &= ~kCosChoreStatePlaying;

	return 1;
}

int Cos::instCompleteChore(int hInst, int choreIndex) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instCompleteChore(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	if (choreIndex < 0 || choreIndex >= pCostume->numChores)
		return 0;

	pInstance->aChoreInstances[choreIndex].state = kCosChoreStatePlaying;
	updateChoreInstance(pCostume, pInstance, choreIndex, pCostume->aChores[choreIndex].length - pInstance->aChoreInstances[choreIndex].time, hInst);

	return 1;
}

int Cos::instCompleteChores(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instCompleteChores(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numChores; i++) {
		CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[i];

		if (pChoreInst->state & kCosChoreStatePlaying)
			updateChoreInstance(pCostume, pInstance, i, pCostume->aChores[i].length - pChoreInst->time, hInst);
	}

	return 1;
}

int Cos::instPlayChore(int hInst, int choreIndex, int bLoop, int bAdvance) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instPlayChore(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];

	if (choreIndex < 0 || choreIndex >= _aCostumes[pInstance->costume].numChores)
		return 0;

	CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[choreIndex];
	pChoreInst->state = kCosChoreStatePlaying;

	if (bLoop)
		pChoreInst->state = kCosChoreStatePlaying | kCosChoreStateLooping;

	if (!bAdvance)
		pChoreInst->state |= kCosChoreStateNoAdvance;

	return 1;
}

int Cos::instUpdateChore(int hInst, int choreIndex, int deltaMs) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instUpdateChore(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];

	if (choreIndex < 0 || choreIndex >= _aCostumes[pInstance->costume].numChores)
		return 0;

	if (pInstance->aChoreInstances[choreIndex].state & kCosChoreStatePlaying)
		updateChoreInstance(&_aCostumes[pInstance->costume], pInstance, choreIndex, deltaMs, hInst);

	return 1;
}

int Cos::instUpdateChores(int hInst, int deltaMs) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instUpdateChores(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numChores; i++) {
		uint32 state = pInstance->aChoreInstances[i].state;

		if ((state & kCosChoreStatePlaying) && (state & kCosChoreStateNoAdvance))
			updateChoreInstance(pCostume, pInstance, i, deltaMs, hInst);
	}

	return 1;
}

int Cos::instSetChoreLoop(int hInst, int choreIndex, int bLoop) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instSetChoreLoop(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];

	if (choreIndex < 0 || choreIndex >= _aCostumes[pInstance->costume].numChores)
		return 0;

	CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[choreIndex];

	if (bLoop)
		pChoreInst->state |= kCosChoreStateLooping;
	else
		pChoreInst->state &= ~kCosChoreStateLooping;

	return 1;
}

int Cos::instSetChoreAdv(int hInst, int choreIndex, int bAdvance) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instSetChoreAdv(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];

	if (choreIndex < 0 || choreIndex >= _aCostumes[pInstance->costume].numChores)
		return 0;

	CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[choreIndex];

	if (bAdvance)
		pChoreInst->state &= ~kCosChoreStateNoAdvance;
	else
		pChoreInst->state |= kCosChoreStateNoAdvance;

	return 1;
}

int Cos::instChoreStatus(int hInst, int choreIndex) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instChoreStatus(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];

	if (choreIndex < 0 || choreIndex >= _aCostumes[pInstance->costume].numChores)
		return 0;

	return pInstance->aChoreInstances[choreIndex].state;
}

int Cos::updateChoreInstance(CosCostume *pCostume, CosInstance *pInstance, int choreIndex, int deltaMs, int hInst) {
	CosChore *pChore = &pCostume->aChores[choreIndex];
	CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[choreIndex];
	int result = 0;

	if (deltaMs == 0)
		return result;

	while (true) {
		for (int t = 0; t < pChore->numTracks; t++) {
			CosChoreTrack *pTrack = &pChore->aTracks[t];
			int k = 0;

			if (pTrack->numKeys == 0)
				continue;

			while (k < pTrack->numKeys && pTrack->aKeys[k].time < pChoreInst->time)
				k++;

			while (k < pTrack->numKeys) {
				CosChoreKey *pKey = &pTrack->aKeys[k];
				int endTime = (int)((uint32)pChoreInst->time + (uint32)deltaMs);

				if (pKey->time >= endTime)
					break;

				int compIndex = pTrack->compIndex;
				changeComponent(2, &_aClasses[pCostume->aComponents[compIndex].classIdx], pInstance->aInstanceData[compIndex], pKey,
					(int)((uint32)endTime - (uint32)pKey->time), pKey, 0, hInst);
				k++;
			}
		}

		int newTime = (int)((uint32)deltaMs + (uint32)pChoreInst->time);
		pChoreInst->time = newTime;
		result = newTime;

		if (newTime < pChore->length)
			break;

		if ((pChoreInst->state & kCosChoreStateLooping) == 0) {
			pChoreInst->state = 0;
			pChoreInst->time = 0;
			return result;
		}

		if (pChore->length <= 0) {
			warning("Cos::updateChoreInstance(): looping chore %d has no length", choreIndex);
			pChoreInst->state = 0;
			pChoreInst->time = 0;
			return result;
		}

		deltaMs = (int)((uint32)newTime - (uint32)pChore->length);
		pChoreInst->time = 0;
	}

	return result;
}

void Cos::changeComponent(int op, CosComponentClass *pClass, void *pInstData, CosChoreKey *pKey, int timeOffset, CosChoreKey *pNextKey, int arg7, int hInst) {
	if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat) {
		warning("Cos::changeComponent(): invalid tag type %d", pClass->type);
		return;
	}

	pClass->pHandler->change(pClass->fourcc, pInstData, op, pKey->value, timeOffset, pNextKey->value, arg7, hInst);
}

} // End of namespace Grim
