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

#include "engines/grim/chore/cos_util.h"
#include "engines/grim/std/std_sort.h"

namespace Grim {

CosUtil::CosUtil(Cos *pCos) : _pCos(pCos) {
}

CosComponent *CosUtil::compPtrFromHandle(CosCostume *pCostume, int compIndex) {
	if (!pCostume) {
		warning("CosUtil::compPtrFromHandle(): null costume");
		return nullptr;
	}

	if (compIndex < 0 || compIndex >= pCostume->numComponents)
		return nullptr;

	return &pCostume->aComponents[compIndex];
}

int CosUtil::adjustComponent(CosCostume *pCostume, int whichComponent) {
	if (!pCostume || whichComponent < 0 || whichComponent >= pCostume->numComponents) {
		warning("CosUtil::adjustComponent(): invalid costume or component %d", whichComponent);
		return 0;
	}

	CosComponent *pComp = &pCostume->aComponents[whichComponent];

	if (pComp->classIdx < 0 || pComp->classIdx >= _pCos->_numClasses) {
		warning("CosUtil::adjustComponent(): invalid tag index %d", pComp->classIdx);
		return 0;
	}

	CosComponentClass *pClass = &_pCos->_aClasses[pComp->classIdx];

	if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat) {
		warning("CosUtil::adjustComponent(): invalid tag type %d", pClass->type);
		return 0;
	}

	if (!pClass->pHandler || !pClass->pHandler->hasAdjuster()) {
		warning("CosUtil::adjustComponent(): tag %s has no adjuster", pClass->name);
		return 0;
	}

	return pClass->pHandler->adjust(&pComp->id, pComp->name, 32);
}

int CosUtil::componentHasAdjuster(CosCostume *pCostume, int whichComponent) {
	if (!pCostume || whichComponent < 0 || whichComponent >= pCostume->numComponents) {
		warning("CosUtil::componentHasAdjuster(): invalid costume or component %d", whichComponent);
		return 0;
	}

	CosComponent *pComp = &pCostume->aComponents[whichComponent];

	if (pComp->classIdx < 0 || pComp->classIdx >= _pCos->_numClasses) {
		warning("CosUtil::componentHasAdjuster(): invalid tag index %d", pComp->classIdx);
		return 0;
	}

	CosComponentClass *pClass = &_pCos->_aClasses[pComp->classIdx];

	if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat)
		return 0;

	if (pClass->pHandler && pClass->pHandler->hasAdjuster())
		return 1;

	return 0;
}

int CosUtil::addChore(CosCostume *pCostume) {
	if (!pCostume) {
		warning("CosUtil::addChore(): null costume");
		return -1;
	}

	CosChore *aChores = new CosChore[pCostume->numChores + 1]();

	for (int i = 0; i < pCostume->numChores; i++)
		aChores[i] = pCostume->aChores[i];

	delete[] pCostume->aChores;
	pCostume->aChores = aChores;
	pCostume->numChores++;
	_pCos->initChore(&aChores[pCostume->numChores - 1]);

	return pCostume->numChores - 1;
}

void CosUtil::deleteChore(CosCostume *pCostume, int choreNum) {
	if (!pCostume || choreNum < 0 || choreNum >= pCostume->numChores) {
		warning("CosUtil::deleteChore(): invalid costume or chore number %d", choreNum);
		return;
	}

	CosChore *pChore = &pCostume->aChores[choreNum];

	if (pChore->numTracks) {
		for (int t = 0; t < pChore->numTracks; t++) {
			if (pChore->aTracks[t].numKeys) {
				delete[] pChore->aTracks[t].aKeys;
				pChore->aTracks[t].aKeys = nullptr;
			}
		}

		delete[] pChore->aTracks;
		pChore->aTracks = nullptr;
	}

	int newNumChores = pCostume->numChores - 1;
	CosChore *aChores = nullptr;

	if (newNumChores > 0) {
		aChores = new CosChore[newNumChores]();

		for (int i = 0; i < choreNum; i++)
			aChores[i] = pCostume->aChores[i];

		for (int i = choreNum + 1; i < pCostume->numChores; i++)
			aChores[i - 1] = pCostume->aChores[i];
	}

	delete[] pCostume->aChores;
	pCostume->aChores = aChores;
	pCostume->numChores = newNumChores;
}

void CosUtil::nameChore(CosCostume *pCostume, int choreNum, const char *name) {
	if (!pCostume || !name || choreNum < 0 || choreNum >= pCostume->numChores) {
		warning("CosUtil::nameChore(): invalid costume, name or chore number %d", choreNum);
		return;
	}

	CosChore *pChore = &pCostume->aChores[choreNum];
	size_t length = strlen(name);

	if (length < 32) {
		Common::strlcpy(pChore->name, name, 32);

		for (size_t i = 0; i < length; i++) {
			if (pChore->name[i] == ' ')
				pChore->name[i] = '_';
		}
	}
}

int CosUtil::deleteKey(CosCostume *pCostume, int whichChore, int whichComponent, int time) {
	if (!pCostume || whichChore < 0 || whichChore >= pCostume->numChores
			|| whichComponent < 0 || whichComponent >= pCostume->numComponents) {
		warning("CosUtil::deleteKey(): invalid costume, chore %d or component %d", whichChore, whichComponent);
		return 0;
	}

	CosChore *pChore = &pCostume->aChores[whichChore];
	int trackIndex = 0;

	while (trackIndex < pChore->numTracks && pChore->aTracks[trackIndex].compIndex != whichComponent)
		trackIndex++;

	if (trackIndex == pChore->numTracks)
		return 0;

	CosChoreTrack *pTrack = &pChore->aTracks[trackIndex];
	int keyIndex = 0;

	while (keyIndex < pTrack->numKeys && pTrack->aKeys[keyIndex].time != time)
		keyIndex++;

	if (keyIndex >= pTrack->numKeys)
		return 0;

	int newNumKeys = pTrack->numKeys - 1;
	CosChoreKey *aKeys = nullptr;

	if (newNumKeys) {
		aKeys = new CosChoreKey[newNumKeys]();

		for (int i = 0; i < keyIndex; i++)
			aKeys[i] = pTrack->aKeys[i];

		for (int i = keyIndex + 1; i < pTrack->numKeys; i++)
			aKeys[i - 1] = pTrack->aKeys[i];
	}

	delete[] pTrack->aKeys;
	pTrack->aKeys = aKeys;
	pTrack->numKeys = newNumKeys;

	return 1;
}

CosChoreKey *CosUtil::queryValueAtTime(CosCostume *pCostume, int whichChore, int whichComponent, int time) {
	if (!pCostume || whichChore < 0 || whichChore >= pCostume->numChores
			|| whichComponent < 0 || whichComponent >= pCostume->numComponents) {
		warning("CosUtil::queryValueAtTime(): invalid costume, chore %d or component %d", whichChore, whichComponent);
		return nullptr;
	}

	CosChore *pChore = &pCostume->aChores[whichChore];
	int trackIndex = 0;

	while (trackIndex < pChore->numTracks && pChore->aTracks[trackIndex].compIndex != whichComponent)
		trackIndex++;

	if (trackIndex == pChore->numTracks)
		return nullptr;

	CosChoreTrack *pTrack = &pChore->aTracks[trackIndex];

	for (int k = 0; k < pTrack->numKeys; k++) {
		if ((uint32)pTrack->aKeys[k].time == (uint32)time)
			return &pTrack->aKeys[k];

		if ((uint32)pTrack->aKeys[k].time > (uint32)time) {
			if (k == 0)
				return nullptr;

			return &pTrack->aKeys[k - 1];
		}
	}

	if (pTrack->numKeys == 0)
		return nullptr;

	return &pTrack->aKeys[pTrack->numKeys - 1];
}

int CosUtil::addFSKey(CosCostume *pCostume, int choreNum, int compNum, int time, int value) {
	if (!pCostume || compNum < 0 || compNum >= pCostume->numComponents) {
		warning("CosUtil::addFSKey(): invalid costume or component %d", compNum);
		return 0;
	}

	int classIdx = pCostume->aComponents[compNum].classIdx;

	if (classIdx < 0 || classIdx >= _pCos->_numClasses) {
		warning("CosUtil::addFSKey(): invalid tag index %d", classIdx);
		return 0;
	}

	if (_pCos->_aClasses[classIdx].type != kCosTagStateBased)
		return 0;

	CosChoreKey *pKey = addKey(pCostume, choreNum, compNum, time);

	if (!pKey)
		return 0;

	pKey->value = value;

	return 1;
}

int CosUtil::addIntKey(CosCostume *pCostume, int choreNum, int compNum, int time, int value) {
	if (!pCostume || compNum < 0 || compNum >= pCostume->numComponents) {
		warning("CosUtil::addIntKey(): invalid costume or component %d", compNum);
		return 0;
	}

	int classIdx = pCostume->aComponents[compNum].classIdx;

	if (classIdx < 0 || classIdx >= _pCos->_numClasses) {
		warning("CosUtil::addIntKey(): invalid tag index %d", classIdx);
		return 0;
	}

	if (_pCos->_aClasses[classIdx].type != kCosTagInt)
		return 0;

	CosChoreKey *pKey = addKey(pCostume, choreNum, compNum, time);

	if (!pKey)
		return 0;

	pKey->value = value;

	return 1;
}

int CosUtil::addFloatKey(CosCostume *pCostume, int choreNum, int compNum, int time, float value) {
	if (!pCostume || compNum < 0 || compNum >= pCostume->numComponents) {
		warning("CosUtil::addFloatKey(): invalid costume or component %d", compNum);
		return 0;
	}

	int classIdx = pCostume->aComponents[compNum].classIdx;

	if (classIdx < 0 || classIdx >= _pCos->_numClasses) {
		warning("CosUtil::addFloatKey(): invalid tag index %d", classIdx);
		return 0;
	}

	if (_pCos->_aClasses[classIdx].type != kCosTagFloat)
		return 0;

	CosChoreKey *pKey = addKey(pCostume, choreNum, compNum, time);

	if (!pKey)
		return 0;

	memcpy(&pKey->value, &value, sizeof(pKey->value));

	return 1;
}

int CosUtil::countChores(CosCostume *pCostume) {
	if (!pCostume) {
		warning("CosUtil::countChores(): null costume");
		return 0;
	}

	return pCostume->numChores;
}

const char *CosUtil::choreName(CosCostume *pCostume, int choreNum) {
	if (!pCostume || choreNum < 0 || choreNum >= pCostume->numChores) {
		warning("CosUtil::choreName(): invalid costume or chore number %d", choreNum);
		return nullptr;
	}

	return pCostume->aChores[choreNum].name;
}

int CosUtil::countTags() {
	return _pCos->_numClasses;
}

const char *CosUtil::tagInfo(int whichTag, int *pFourcc) {
	if (whichTag < 0 || whichTag >= _pCos->_numClasses) {
		warning("CosUtil::tagInfo(): invalid tag number %d", whichTag);
		return nullptr;
	}

	if (pFourcc)
		*pFourcc = _pCos->_aClasses[whichTag].fourcc;

	return _pCos->_aClasses[whichTag].name;
}

CosChoreKey *CosUtil::addKey(CosCostume *pCostume, int choreNum, int compNum, int time) {
	if (!pCostume || choreNum < 0 || choreNum >= pCostume->numChores) {
		warning("CosUtil::addKey(): invalid costume or chore number %d", choreNum);
		return nullptr;
	}

	CosChore *pChore = &pCostume->aChores[choreNum];
	int trackIndex = 0;

	while (trackIndex < pChore->numTracks && pChore->aTracks[trackIndex].compIndex != compNum)
		trackIndex++;

	if (trackIndex == pChore->numTracks) {
		CosChoreTrack *aTracks = new CosChoreTrack[pChore->numTracks + 1]();

		for (int t = 0; t < pChore->numTracks; t++)
			aTracks[t] = pChore->aTracks[t];

		delete[] pChore->aTracks;
		pChore->aTracks = aTracks;
		pChore->numTracks++;

		aTracks[trackIndex].numKeys = 0;
		aTracks[trackIndex].aKeys = nullptr;
		aTracks[trackIndex].length = 0;
		aTracks[trackIndex].compIndex = compNum;
	}

	CosChoreTrack *pTrack = &pChore->aTracks[trackIndex];
	CosChoreKey searchKey;
	searchKey.time = time;
	searchKey.value = 0;

	CosChoreKey *pKey = nullptr;

	if (pTrack->numKeys)
		pKey = (CosChoreKey *)bsearch(&searchKey, pTrack->aKeys, pTrack->numKeys, sizeof(CosChoreKey), Cos::compareFunction);

	if (!pKey) {
		CosChoreKey *aKeys = new CosChoreKey[pTrack->numKeys + 1]();

		for (int k = 0; k < pTrack->numKeys; k++)
			aKeys[k] = pTrack->aKeys[k];

		delete[] pTrack->aKeys;
		pTrack->aKeys = aKeys;
		pTrack->numKeys++;
		pKey = &aKeys[pTrack->numKeys - 1];
	}

	pKey->time = time;

	if ((uint32)time > (uint32)pTrack->length)
		pTrack->length = time;

	uint32 minLength = time ? (uint32)time : 1;

	if (minLength > (uint32)pChore->length)
		pChore->length = (int)minLength;

	sort(pTrack->aKeys, pTrack->numKeys, sizeof(CosChoreKey), Cos::compareFunction);

	for (int i = 0; i < pChore->numTracks; i++) {
		for (int j = 0; j < pChore->numTracks; j++) {
			if (i != j && pChore->aTracks[i].compIndex == pChore->aTracks[j].compIndex)
				return nullptr;
		}
	}

	return (CosChoreKey *)bsearch(&searchKey, pTrack->aKeys, pTrack->numKeys, sizeof(CosChoreKey), Cos::compareFunction);
}

} // End of namespace Grim
