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

int Cos::createCosInstance(int hCostume, int userData) {
	int costumeIndex = handleToIndex(hCostume);

	if (costumeIndex < 0 || costumeIndex >= _maxCostumes)
		return 0;

	CosCostume *pCostume = &_aCostumes[costumeIndex];
	int slot = allocInstanceSlot();
	_aInstances[slot].costume = costumeIndex;

	if (pCostume->numChores)
		_aInstances[slot].aChoreInstances = new CosChoreInstance[pCostume->numChores]();
	else
		_aInstances[slot].aChoreInstances = nullptr;

	initChoreInstances(pCostume->aChores, _aInstances[slot].aChoreInstances, pCostume->numChores);

	if (pCostume->numComponents) {
		_aInstances[slot].aInstanceData = new void *[pCostume->numComponents]();

		for (int i = 0; i < pCostume->numComponents; i++)
			_aInstances[slot].aInstanceData[i] = pCostume->aComponents[i].id;
	} else {
		_aInstances[slot].aInstanceData = nullptr;
	}

	_aInstances[slot].userData = userData;
	int hInst = instIndexToHandle(slot);

	int *aTreeDepths = nullptr;

	if (pCostume->numComponents)
		aTreeDepths = new int[pCostume->numComponents];

	int maxDepth = 0;

	for (int i = 0; i < pCostume->numComponents; i++) {
		int parent = pCostume->aComponents[i].parent;
		int depth = 0;

		while (parent != -1) {
			if (parent < 0 || parent >= pCostume->numComponents || depth > pCostume->numComponents) {
				warning("Cos::createCosInstance(): invalid parent chain at component %d", i);
				break;
			}

			parent = pCostume->aComponents[parent].parent;
			depth++;
		}

		aTreeDepths[i] = depth;

		if (maxDepth <= depth)
			maxDepth = depth;
	}

	for (int depth = 0; depth <= maxDepth; depth++) {
		for (int i = 0; i < pCostume->numComponents; i++) {
			if (aTreeDepths[i] != depth)
				continue;

			CosComponentClass *pClass = &_aClasses[pCostume->aComponents[i].classIdx];
			void *pData = nullptr;

			if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat)
				warning("Cos::createCosInstance(): invalid tag type %d", pClass->type);
			else
				pData = pClass->pHandler->instance(hInst, i, 1);

			if (pData)
				_aInstances[slot].aInstanceData[i] = pData;
		}
	}

	delete[] aTreeDepths;

	if (_pInstanceHook) {
		if (!_pInstanceHook(hInst, 1)) {
			freeCosInstance(hInst);
			return 0;
		}
	} else {
		_aInstances[slot].userData = 0;
	}

	return hInst;
}

int Cos::freeCosInstance(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances) {
		warning("Cos::freeCosInstance(): invalid instance handle %d", hInst);
		return _numInstances;
	}

	if (_pInstanceHook)
		_pInstanceHook(hInst, 0);

	freeInstance(index);
	_aInstances[index].bFree = 1;

	return --_numInstances;
}

void Cos::freeInstance(int index) {
	CosInstance *pInstance = &_aInstances[index];
	int hInst = instIndexToHandle(index);

	if (pInstance->bFree)
		return;

	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponentClass *pClass = &_aClasses[pCostume->aComponents[i].classIdx];
		void *pData = nullptr;

		if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat)
			warning("Cos::freeInstance(): invalid tag type %d", pClass->type);
		else
			pData = pClass->pHandler->instance(hInst, i, 0);

		if (pData)
			pInstance->aInstanceData[i] = pData;
	}

	delete[] pInstance->aChoreInstances;
	delete[] pInstance->aInstanceData;

	pInstance->costume = 0;
	pInstance->aChoreInstances = nullptr;
	pInstance->userData = 0;
	pInstance->aInstanceData = nullptr;
	pInstance->bFree = 0;
	pInstance->costume = -1;
	pInstance->bFree = 1;
}

void Cos::instRescue(int hInst, int hCostume) {
	int costumeIndex = handleToIndex(hCostume);

	if (costumeIndex >= 0) {
		int instIndex = instHandleToIndex(hInst);

		if (instIndex >= 0 && instIndex < _maxInstances)
			_aInstances[instIndex].costume = costumeIndex;
	}
}

void *Cos::instGetInstanceData(int hInst, int compIndex, int *pFourcc) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || compIndex < 0)
		return nullptr;

	if (_aInstances[index].bFree) {
		warning("Cos::instGetInstanceData(): instance %d is not in use", hInst);
		return nullptr;
	}

	CosCostume *pCostume = &_aCostumes[_aInstances[index].costume];

	if (compIndex >= pCostume->numComponents)
		return nullptr;

	if (pFourcc)
		*pFourcc = _aClasses[pCostume->aComponents[compIndex].classIdx].fourcc;

	return _aInstances[index].aInstanceData[compIndex];
}

int Cos::instGetUserData(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances)
		return 0;

	return _aInstances[index].userData;
}

void Cos::instSetUserData(int hInst, int userData) {
	int index = instHandleToIndex(hInst);

	if (index >= 0 && index < _maxInstances)
		_aInstances[index].userData = userData;
}

int Cos::instCountComponents(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0)
		return 0;

	if (index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instCountComponents(): invalid instance handle %d", hInst);
		return 0;
	}

	return countComponents(&_aCostumes[_aInstances[index].costume]);
}

CosComponent *Cos::instComponentInfo(int hInst, int compNum, int *pFourcc, void **pId, int *pParent) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instComponentInfo(): invalid instance handle %d", hInst);
		return nullptr;
	}

	return componentInfo(&_aCostumes[_aInstances[index].costume], compNum, pFourcc, pId, pParent, nullptr);
}

int Cos::instCountChores(int hInst) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instCountChores(): invalid instance handle %d", hInst);
		return 0;
	}

	return _aCostumes[_aInstances[index].costume].numChores;
}

CosCostume *Cos::instGetCostume(int hInst, int *pHandle) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instGetCostume(): invalid instance handle %d", hInst);
		return nullptr;
	}

	int costumeIndex = _aInstances[index].costume;

	if (pHandle)
		*pHandle = indexToHandle(costumeIndex);

	return &_aCostumes[costumeIndex];
}

const char *Cos::instChoreName(int hInst, int choreIndex) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::instChoreName(): invalid instance handle %d", hInst);
		return nullptr;
	}

	CosCostume *pCostume = &_aCostumes[_aInstances[index].costume];

	if (choreIndex < 0 || choreIndex >= pCostume->numChores) {
		warning("Cos::instChoreName(): chore number %d is out of range", choreIndex);
		return nullptr;
	}

	return pCostume->aChores[choreIndex].name;
}

} // End of namespace Grim
