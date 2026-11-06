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

#include "engines/grim/chore/cos.h"

namespace Grim {

CosCostume *Cos::ptrFromHandle(int hCostume) {
	int index = handleToIndex(hCostume);

	if (index < 0 || index >= _maxCostumes)
		return nullptr;

	return &_aCostumes[index];
}

int Cos::findClass(int fourcc, const char *name, int type) {
	for (int i = 0; i < _numClasses; i++) {
		if (_aClasses[i].fourcc == fourcc && _aClasses[i].type == type && !scumm_stricmp(_aClasses[i].name, name))
			return i;
	}

	return -1;
}

int Cos::findComponent(CosCostume *pCostume, int fourcc, int depthIdx, const char *name) {
	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponent *pComp = &pCostume->aComponents[i];

		if (pComp->depthIdx == depthIdx && _aClasses[pComp->classIdx].fourcc == fourcc && !scumm_stricmp(pComp->name, name))
			return i;
	}

	return -1;
}

int Cos::findComponentByParent(CosCostume *pCostume, int fourcc, int depthIdx, const char *name, int parent) {
	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponent *pComp = &pCostume->aComponents[i];

		if (pComp->depthIdx == depthIdx && _aClasses[pComp->classIdx].fourcc == fourcc && pComp->parent == parent
				&& !scumm_stricmp(pComp->name, name))
			return i;
	}

	return -1;
}

int Cos::initComponent(CosCostume *pCostume, CosComponent *pComp, const char *name, int classIdx, void *id, int parent) {
	int result = 0;

	if (strlen(name) < 0x20) {
		Common::strlcpy(pComp->name, name, 32);
		pComp->id = id;
		pComp->classIdx = classIdx;
		pComp->parent = parent;
		pComp->depthIdx = 0;

		for (result = findComponent(pCostume, _aClasses[classIdx].fourcc, 0, name); result >= 0;
				result = findComponent(pCostume, _aClasses[pComp->classIdx].fourcc, pComp->depthIdx, name))
			pComp->depthIdx++;

		pComp->numNames = 0;
		pComp->aNameIds = nullptr;
		pComp->aNames = nullptr;
		pComp->apStateBitmaps = nullptr;
	}

	return result;
}

void Cos::setComponentNames(CosComponent *pComp, const CosComponentNames *pNames) {
	if (_aClasses[pComp->classIdx].type != kCosTagStateBased)
		return;

	if (!pNames || pNames->numNames == 0)
		return;

	if (pNames->numNames < 0) {
		warning("Cos::setComponentNames(): negative name count %d", pNames->numNames);
		return;
	}

	int numNames = pNames->numNames;
	pComp->numNames = numNames;

	if (pNames->aNameIds) {
		pComp->aNameIds = new int[numNames];
		memcpy(pComp->aNameIds, pNames->aNameIds, numNames * sizeof(int));
	}

	if (pNames->aNames) {
		pComp->aNames = new char *[numNames];

		for (int i = 0; i < numNames; i++) {
			size_t size = strlen(pNames->aNames[i]) + 1;
			pComp->aNames[i] = new char[size];
			Common::strlcpy(pComp->aNames[i], pNames->aNames[i], size);
		}
	}

	if (pNames->aStateBitmaps) {
		pComp->apStateBitmaps = new CosStateBitmap *[numNames];

		for (int i = 0; i < numNames; i++) {
			CosStateBitmap *pBitmap = new CosStateBitmap;
			const CosStateBitmapDesc *pDesc = &pNames->aStateBitmaps[i];
			pComp->apStateBitmaps[i] = pBitmap;

			pBitmap->field_0 = pDesc->field_0;
			pBitmap->field_4 = pDesc->field_4;
			pBitmap->field_8 = pDesc->field_8;
			pBitmap->field_C = pDesc->field_C;

			if (_pBitmapMunger) {
				if (pNames->aBitmapExtra)
					pBitmap->field_10 = _pBitmapMunger(pDesc, pNames->aBitmapExtra[i]);
				else
					pBitmap->field_10 = _pBitmapMunger(pDesc, 0);
			} else {
				pBitmap->field_10 = 0;
			}
		}
	}
}

int Cos::registerComponent(int hCostume, const char *name, void *id, int fourcc, void *pParentId, int parentFourcc, const CosComponentNames *pNames) {
	int index = handleToIndex(hCostume);

	if (!name)
		return 0;

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::registerComponent(): invalid costume handle %d", hCostume);
		return 0;
	}

	if (strlen(name) >= 0x20) {
		warning("Cos::registerComponent(): component name %s is too long", name);
		return 0;
	}

	CosCostume *pCostume = &_aCostumes[index];
	int parentCompIndex = 0;

	if (pParentId) {
		int parentClassIdx = 0;

		while (parentClassIdx < _numClasses && _aClasses[parentClassIdx].fourcc != parentFourcc)
			parentClassIdx++;

		if (parentClassIdx == _numClasses)
			return 0;

		while (parentCompIndex < pCostume->numComponents) {
			CosComponent *pParent = &pCostume->aComponents[parentCompIndex];

			if (_aClasses[pParent->classIdx].fourcc == parentFourcc && pParent->id == pParentId)
				break;

			parentCompIndex++;
		}

		if (parentCompIndex == pCostume->numComponents)
			return 0;
	}

	int classIdx = 0;

	while (classIdx < _numClasses && _aClasses[classIdx].fourcc != fourcc)
		classIdx++;

	if (classIdx == _numClasses)
		return 0;

	CosComponent *aComponents = new CosComponent[pCostume->numComponents + 1]();

	for (int i = 0; i < pCostume->numComponents; i++)
		aComponents[i] = pCostume->aComponents[i];

	delete[] pCostume->aComponents;
	pCostume->aComponents = aComponents;

	CosComponent *pComp = &aComponents[pCostume->numComponents];
	initComponent(pCostume, pComp, name, classIdx, id, pParentId ? parentCompIndex : -1);
	pCostume->numComponents++;
	setComponentNames(pComp, pNames);

	if (_pCompNotifier)
		_pCompNotifier(pCostume, pComp, pCostume->numComponents - 1, 1);

	return 1;
}

int Cos::removeComponent(int hCostume, int fourcc, void *id) {
	CosCostume *pCostume = ptrFromHandle(hCostume);

	if (!pCostume)
		return 0;

	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponent *pComp = &pCostume->aComponents[i];

		if (pComp->id == id && _aClasses[pComp->classIdx].fourcc == fourcc)
			return deleteComponent(hCostume, i);
	}

	return 0;
}

int Cos::deleteComponent(int hCostume, int compIndex) {
	CosCostume *pCostume = ptrFromHandle(hCostume);

	if (!pCostume)
		return 0;

	int numComponents = pCostume->numComponents;

	if (compIndex < 0 || compIndex >= numComponents)
		return 0;

	int *aDelete = new int[numComponents]();
	int *aRemap = new int[numComponents];

	for (int i = 0; i < numComponents; i++)
		aRemap[i] = i;

	aDelete[compIndex] = 1;
	aRemap[compIndex] = -1;

	for (int i = 0; i < numComponents; i++) {
		int parent = pCostume->aComponents[i].parent;
		int steps = 0;
		bool bAncestorDeleted = false;

		while (parent != -1) {
			if (parent < 0 || parent >= numComponents || steps++ > numComponents) {
				warning("Cos::deleteComponent(): invalid parent chain at component %d", i);
				break;
			}

			if (aDelete[parent]) {
				bAncestorDeleted = true;
				break;
			}

			parent = pCostume->aComponents[parent].parent;
		}

		if (bAncestorDeleted) {
			aDelete[i] = 1;
			aRemap[i] = -1;
		}
	}

	for (int i = 0; i < numComponents; i++) {
		if (!aDelete[i])
			continue;

		CosComponent *pComp = &pCostume->aComponents[i];
		int nextDepth = pComp->depthIdx + 2;
		int found = findComponent(pCostume, _aClasses[pComp->classIdx].fourcc, pComp->depthIdx + 1, pComp->name);

		while (found >= 0) {
			pCostume->aComponents[found].depthIdx--;
			found = findComponent(pCostume, _aClasses[pComp->classIdx].fourcc, nextDepth, pComp->name);
			nextDepth++;
		}
	}

	int numDeleted = 0;

	for (int i = 0; i < numComponents; i++) {
		if (aDelete[i])
			numDeleted++;

		if (!aDelete[i] && numDeleted) {
			aRemap[i] = i - numDeleted;
			pCostume->aComponents[i - numDeleted] = pCostume->aComponents[i];
		}
	}

	int newNumComponents = numComponents - numDeleted;
	CosComponent *aComponents = nullptr;

	if (newNumComponents) {
		aComponents = new CosComponent[newNumComponents]();

		for (int i = 0; i < newNumComponents; i++)
			aComponents[i] = pCostume->aComponents[i];
	}

	delete[] pCostume->aComponents;
	pCostume->aComponents = aComponents;
	pCostume->numComponents = newNumComponents;

	for (int i = 0; i < newNumComponents; i++) {
		CosComponent *pComp = &pCostume->aComponents[i];

		if (pComp->parent != -1) {
			if (pComp->parent < 0 || pComp->parent >= numComponents) {
				warning("Cos::deleteComponent(): component %d has an invalid parent", i);
				pComp->parent = -1;
			} else {
				pComp->parent = aRemap[pComp->parent];
			}
		}
	}

	for (int c = 0; c < pCostume->numChores; c++) {
		CosChore *pChore = &pCostume->aChores[c];

		for (int t = 0; t < pChore->numTracks; t++) {
			CosChoreTrack *pTrack = &pChore->aTracks[t];

			if (pTrack->compIndex < 0 || pTrack->compIndex >= numComponents) {
				warning("Cos::deleteComponent(): track %d of chore %d has an invalid component", t, c);
				pTrack->compIndex = -1;
			} else {
				pTrack->compIndex = aRemap[pTrack->compIndex];
			}
		}
	}

	for (int c = 0; c < pCostume->numChores; c++) {
		CosChore *pChore = &pCostume->aChores[c];
		int numTracks = pChore->numTracks;
		int numRemoved = 0;

		for (int t = 0; t < numTracks; t++) {
			CosChoreTrack *pTrack = &pChore->aTracks[t];

			if (pTrack->compIndex == -1) {
				numRemoved++;

				if (pTrack->numKeys) {
					delete[] pTrack->aKeys;
					pTrack->aKeys = nullptr;
				}
			} else if (numRemoved) {
				pChore->aTracks[t - numRemoved] = *pTrack;
			}
		}

		int newNumTracks = numTracks - numRemoved;
		CosChoreTrack *aTracks = nullptr;

		if (newNumTracks > 0) {
			aTracks = new CosChoreTrack[newNumTracks]();

			for (int t = 0; t < newNumTracks; t++)
				aTracks[t] = pChore->aTracks[t];
		}

		delete[] pChore->aTracks;
		pChore->aTracks = aTracks;
		pChore->numTracks = newNumTracks;
	}

	if (_pCompNotifier) {
		int numNotified = 0;

		for (int i = 0; i < numComponents; i++) {
			if (aDelete[i])
				_pCompNotifier(pCostume, nullptr, i - numNotified++, 0);
		}
	}

	delete[] aDelete;
	delete[] aRemap;

	return 1;
}

int Cos::addComponent(int hCostume, CosComponent *pComp, int fourcc, void *pParentId, int parentFourcc) {
	int classIdx = 0;

	while (classIdx < _numClasses && _aClasses[classIdx].fourcc != fourcc)
		classIdx++;

	if (classIdx == _numClasses)
		return 0;

	CosComponentClass *pClass = &_aClasses[classIdx];

	if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat) {
		warning("Cos::addComponent(): invalid tag type %d", pClass->type);
		return pComp != nullptr;
	}

	return pClass->pHandler->request(hCostume, fourcc, pParentId, parentFourcc, pComp) != 0;
}

int Cos::countComponents(CosCostume *pCostume) {
	if (!pCostume) {
		warning("Cos::countComponents(): null costume");
		return 0;
	}

	return pCostume->numComponents;
}

CosComponent *Cos::componentInfo(CosCostume *pCostume, int compNum, int *pFourcc, void **pId, int *pParent, int *pType) {
	if (!pCostume || compNum < 0 || compNum >= pCostume->numComponents) {
		warning("Cos::componentInfo(): invalid costume or component number %d", compNum);
		return nullptr;
	}

	CosComponent *pComp = &pCostume->aComponents[compNum];
	CosComponentClass *pClass = &_aClasses[pComp->classIdx];

	if (pFourcc)
		*pFourcc = pClass->fourcc;

	if (pId)
		*pId = pComp->id;

	if (pType)
		*pType = pClass->type;

	if (pParent)
		*pParent = pComp->parent;

	return pComp;
}

void Cos::initChore(CosChore *pChore) {
	pChore->length = 15;
	pChore->numTracks = 0;
	pChore->aTracks = nullptr;
	Common::strlcpy(pChore->name, "<unnamed>", 32);
}

void Cos::initChoreInstances(CosChore *aChores, CosChoreInstance *aInst, int count) {
	for (int i = 0; i < count; i++) {
		aInst[i].state = 0;
		aInst[i].choreIndex = i;
		aInst[i].time = 0;
	}
}

void Cos::setChoreLength(CosCostume *pCostume, int choreNum, int length) {
	if (!pCostume || choreNum < 0 || choreNum >= pCostume->numChores) {
		warning("Cos::setChoreLength(): invalid costume or chore number %d", choreNum);
		return;
	}

	CosChore *pChore = &pCostume->aChores[choreNum];

	if ((uint32)length <= (uint32)pChore->length) {
		for (int t = 0; t < pChore->numTracks; t++) {
			CosChoreTrack *pTrack = &pChore->aTracks[t];
			int k = 0;

			while (k < pTrack->numKeys && (uint32)pTrack->aKeys[k].time <= (uint32)pChore->length)
				k++;

			if (k < pTrack->numKeys) {
				CosChoreKey *aKeys = nullptr;

				if (k > 0) {
					aKeys = new CosChoreKey[k];

					for (int i = 0; i < k; i++)
						aKeys[i] = pTrack->aKeys[i];
				}

				delete[] pTrack->aKeys;
				pTrack->aKeys = aKeys;
				pTrack->numKeys = k;
			}
		}
	}

	pChore->length = length;
}

} // End of namespace Grim
