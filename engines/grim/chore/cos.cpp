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

namespace Grim {

Cos::Cos() : _pHS(nullptr), _bInitialized(0), _bStarted(0), _numCostumes(0), _maxCostumes(0), _aCostumes(nullptr), _nextCostumeId(0),
		_numClasses(0), _maxClasses(0), _aClasses(nullptr), _numInstances(0), _maxInstances(0), _aInstances(nullptr),
		_aCompMap(nullptr), _compMapSize(0), _lineNumber(0), _pInstanceHook(nullptr), _pSaveHook(nullptr), _pSaveVerify(nullptr),
		_pCompNotifier(nullptr), _pBitmapMunger(nullptr) {
	memset(_aTagMap, 0, sizeof(_aTagMap));
	memset(_lineBuffer, 0, sizeof(_lineBuffer));
}

int Cos::initialize(SysStruct *pHS) {
	_pHS = pHS;
	_bInitialized = 1;

	return 0;
}

int Cos::terminate() {
	_pHS = nullptr;

	if (_bInitialized)
		_bInitialized = 0;

	return 0;
}

int Cos::startup() {
	if (_bStarted)
		return 1;

	_numCostumes = 0;
	_maxCostumes = 32;
	_aCostumes = new CosCostume[32]();

	for (int i = 0; i < 32; i++)
		_aCostumes[i].bFree = 1;

	_numClasses = 0;
	_maxClasses = 32;
	_aClasses = new CosComponentClass[32]();

	_numInstances = 0;
	_maxInstances = 32;
	_aInstances = new CosInstance[32]();

	for (int i = 0; i < 32; i++)
		_aInstances[i].bFree = 1;

	_bStarted = 1;

	return 0;
}

int Cos::shutdown() {
	_maxCostumes = 0;
	_numCostumes = 0;
	delete[] _aCostumes;
	_aCostumes = nullptr;

	_maxClasses = 0;
	_numClasses = 0;
	delete[] _aClasses;
	_aClasses = nullptr;

	_maxInstances = 0;
	_numInstances = 0;
	delete[] _aInstances;
	_aInstances = nullptr;

	if (_compMapSize) {
		delete[] _aCompMap;
		_aCompMap = nullptr;
		_compMapSize = 0;
	}

	_bStarted = 0;

	return 0;
}

int Cos::handleToIndex(int handle) {
	if (handle && handle <= _maxCostumes)
		return handle - 1;

	return -1;
}

int Cos::indexToHandle(int index) {
	if (index < 0 || index >= _maxCostumes)
		return 0;

	return index + 1;
}

int Cos::instHandleToIndex(int handle) {
	if (handle && handle <= _maxInstances)
		return handle - 1;

	return -1;
}

int Cos::instIndexToHandle(int index) {
	if (index < 0 || index >= _maxInstances)
		return 0;

	return index + 1;
}

int Cos::allocCostumeSlot() {
	for (int i = 0; i < _maxCostumes; i++) {
		if (_aCostumes[i].bFree) {
			_aCostumes[i].bFree = 0;
			_numCostumes++;
			return i;
		}
	}

	int oldMax = _maxCostumes;

	if (_numCostumes + 1 > _maxCostumes) {
		_maxCostumes += 8;

		CosCostume *aCostumes = new CosCostume[_maxCostumes]();

		for (int i = 0; i < oldMax; i++)
			aCostumes[i] = _aCostumes[i];

		delete[] _aCostumes;
		_aCostumes = aCostumes;

		for (int i = oldMax; i < _maxCostumes; i++)
			_aCostumes[i].bFree = 1;
	}

	int index = _numCostumes;

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::allocCostumeSlot(): costume counter %d is out of range, using slot %d", index, oldMax);
		index = oldMax;
	}

	_numCostumes++;
	_aCostumes[index].bFree = 0;

	return index;
}

int Cos::allocInstanceSlot() {
	for (int i = 0; i < _maxInstances; i++) {
		if (_aInstances[i].bFree) {
			_aInstances[i].bFree = 0;
			_numInstances++;
			return i;
		}
	}

	int oldMax = _maxInstances;

	if (_numInstances + 1 > _maxInstances) {
		_maxInstances += 8;

		CosInstance *aInstances = new CosInstance[_maxInstances]();

		for (int i = 0; i < oldMax; i++)
			aInstances[i] = _aInstances[i];

		delete[] _aInstances;
		_aInstances = aInstances;

		for (int i = oldMax; i < _maxInstances; i++)
			_aInstances[i].bFree = 1;
	}

	int index = _numInstances;

	if (index < 0 || index >= _maxInstances) {
		warning("Cos::allocInstanceSlot(): instance counter %d is out of range, using slot %d", index, oldMax);
		index = oldMax;
	}

	_numInstances++;
	_aInstances[index].bFree = 0;

	return index;
}

int Cos::allocTagSlot() {
	if (_numClasses + 1 > _maxClasses) {
		warning("Cos::allocTagSlot(): too many component tags");
		return -1;
	}

	return _numClasses++;
}

CosComponentClass *Cos::newTag(int fourcc, const char *name, int param) {
	for (int i = 0; i < _numClasses; i++) {
		if (_aClasses[i].fourcc == fourcc)
			return nullptr;
	}

	if (strlen(name) >= 32)
		return nullptr;

	int index = allocTagSlot();

	if (index < 0)
		return nullptr;

	CosComponentClass *pClass = &_aClasses[index];
	Common::strlcpy(pClass->name, name, 32);
	pClass->fourcc = fourcc;
	pClass->param = param;

	return pClass;
}

int Cos::newStateBasedTag(int fourcc, const char *name, int unused, CosComponentHandler *pHandler, int param) {
	if (!pHandler) {
		warning("Cos::newStateBasedTag(): null handler for tag %s", name);
		return 0;
	}

	CosComponentClass *pClass = newTag(fourcc, name, param);

	if (!pClass)
		return 0;

	pClass->type = kCosTagStateBased;
	pClass->pHandler = pHandler;

	return 1;
}

int Cos::newIntTag(int fourcc, const char *name, int unused, CosComponentHandler *pHandler, int param) {
	if (!pHandler) {
		warning("Cos::newIntTag(): null handler for tag %s", name);
		return 0;
	}

	CosComponentClass *pClass = newTag(fourcc, name, param);

	if (!pClass)
		return 0;

	pClass->type = kCosTagInt;
	pClass->pHandler = pHandler;

	return 1;
}

int Cos::newFloatTag(int fourcc, const char *name, int unused, CosComponentHandler *pHandler, int param) {
	if (!pHandler) {
		warning("Cos::newFloatTag(): null handler for tag %s", name);
		return 0;
	}

	CosComponentClass *pClass = newTag(fourcc, name, param);

	if (!pClass)
		return 0;

	pClass->type = kCosTagFloat;
	pClass->pHandler = pHandler;

	return 1;
}

CosInstanceHook Cos::registerInstanceHook(CosInstanceHook pHook) {
	CosInstanceHook pOldHook = _pInstanceHook;
	_pInstanceHook = pHook;

	return pOldHook;
}

CosSaveHook Cos::registerSaveHook(CosSaveHook pHook) {
	CosSaveHook pOldHook = _pSaveHook;
	_pSaveHook = pHook;

	return pOldHook;
}

CosSaveVerify Cos::registerSaveVerify(CosSaveVerify pVerify) {
	_pSaveVerify = pVerify;

	return pVerify;
}

CosCompNotifier Cos::registerCompNotifier(CosCompNotifier pNotifier) {
	CosCompNotifier pOldNotifier = _pCompNotifier;
	_pCompNotifier = pNotifier;

	return pOldNotifier;
}

CosBitmapMunger Cos::registerBitmapMunger(CosBitmapMunger pMunger) {
	CosBitmapMunger pOldMunger = _pBitmapMunger;
	_pBitmapMunger = pMunger;

	return pOldMunger;
}

void Cos::initCostume(CosCostume *pCostume) {
	Common::strlcpy(pCostume->name, "<unnamed>", 32);
	pCostume->uniqueId = _nextCostumeId++;
	pCostume->numComponents = 0;
	pCostume->aComponents = nullptr;
	pCostume->numChores = 0;
	pCostume->aChores = nullptr;
}

int Cos::newCostume() {
	int index = allocCostumeSlot();

	if (index < 0)
		return 0;

	initCostume(&_aCostumes[index]);

	return indexToHandle(index);
}

int Cos::load(const char *path) {
	for (int i = 0; i < _maxCostumes; i++) {
		if (!_aCostumes[i].bFree && scumm_stricmp(path, _aCostumes[i].name) == 0)
			return indexToHandle(i);
	}

	int index = allocCostumeSlot();
	int hCostume = indexToHandle(index);

	if (!loadEntry(path, hCostume)) {
		freeCostumeData(indexToHandle(index));
		return 0;
	}

	return hCostume;
}

int Cos::loadEntry(const char *path, int hCostume) {
	int index = handleToIndex(hCostume);

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::loadEntry(): invalid costume handle %d", hCostume);
		return 0;
	}

	CosCostume *pCostume = &_aCostumes[index];
	initCostume(pCostume);

	const char *pName = strrchr(path, '\\');

	if (pName) {
		pName++;

		if (strlen(pName) > 0x20) {
			pCostume->bFree = 1;
			return 0;
		}

		Common::strlcpy(pCostume->name, pName, 32);
	} else {
		if (strlen(path) > 0x20) {
			pCostume->bFree = 1;
			return 0;
		}

		Common::strlcpy(pCostume->name, path, 32);
	}

	Common::SeekableReadStream *pFile = _pHS->openFile(path);

	if (!pFile) {
		pCostume->bFree = 1;
		return 0;
	}

	_lineNumber = 0;

	int major = 0;
	int minor = 0;
	bool bFailed = false;

	if (!readLine(pFile, _lineBuffer, 1024)) {
		bFailed = true;
	} else {
		sscanf(_lineBuffer, " costume v%d.%d", &major, &minor);

		if (!readLine(pFile, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section tags")
				|| !parseTags(pFile)
				|| !readLine(pFile, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section components")
				|| !parseComponents(pFile, hCostume)
				|| !readLine(pFile, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section chores")
				|| !parseChores(pFile, pCostume)
				|| !readLine(pFile, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section keys")
				|| (pCostume->numChores && !parseKeys(pFile, pCostume)))
			bFailed = true;
	}

	if (!bFailed) {
		for (int c = 0; c < pCostume->numChores && !bFailed; c++) {
			CosChore *pChore = &pCostume->aChores[c];
			setChoreLength(pCostume, c, pChore->length);

			for (int i = 0; i < pChore->numTracks && !bFailed; i++) {
				for (int j = 0; j < pChore->numTracks; j++) {
					if (i != j && pChore->aTracks[i].compIndex == pChore->aTracks[j].compIndex) {
						bFailed = true;
						break;
					}
				}
			}
		}
	}

	delete pFile;

	if (bFailed) {
		pCostume->bFree = 1;
		return 0;
	}

	return 1;
}

int Cos::freeCostume(int hCostume) {
	int index = handleToIndex(hCostume);

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::freeCostume(): invalid costume handle %d", hCostume);
		return _numCostumes;
	}

	for (int i = 0; i < _maxInstances; i++) {
		if (!_aInstances[i].bFree && _aInstances[i].costume == index)
			freeCosInstance(instIndexToHandle(i));
	}

	freeCostumeData(hCostume);
	_aCostumes[index].bFree = 1;

	return --_numCostumes;
}

void Cos::freeCostumeData(int hCostume) {
	CosCostume *pCostume = ptrFromHandle(hCostume);

	if (!pCostume) {
		warning("Cos::freeCostumeData(): invalid costume handle %d", hCostume);
		return;
	}

	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponent *pComp = &pCostume->aComponents[i];

		if (pComp->classIdx < 0 || pComp->classIdx >= _numClasses) {
			warning("Cos::freeCostumeData(): component %d has an invalid tag index", i);
			continue;
		}

		freeComponent(hCostume, &_aClasses[pComp->classIdx], pComp);
	}

	delete[] pCostume->aComponents;
	pCostume->aComponents = nullptr;
	pCostume->numComponents = 0;

	for (int c = 0; c < pCostume->numChores; c++) {
		CosChore *pChore = &pCostume->aChores[c];

		for (int t = 0; pChore->aTracks && t < pChore->numTracks; t++) {
			delete[] pChore->aTracks[t].aKeys;
			pChore->aTracks[t].aKeys = nullptr;
		}

		delete[] pChore->aTracks;
		pChore->aTracks = nullptr;
		pChore->numTracks = 0;
	}

	delete[] pCostume->aChores;
	pCostume->aChores = nullptr;
	pCostume->numChores = 0;

	pCostume->bFree = 1;
}

void Cos::freeComponent(int hCostume, CosComponentClass *pClass, CosComponent *pComp) {
	if (pClass->type < kCosTagStateBased || pClass->type > kCosTagFloat)
		warning("Cos::freeComponent(): invalid tag type %d", pClass->type);
	else if (pClass->pHandler)
		pClass->pHandler->freeComponent(hCostume, pClass->fourcc, pComp->id);

	delete[] pComp->aNameIds;
	pComp->aNameIds = nullptr;

	if (pComp->aNames) {
		for (int i = 0; i < pComp->numNames; i++)
			delete[] pComp->aNames[i];

		delete[] pComp->aNames;
		pComp->aNames = nullptr;
	}

	if (pComp->apStateBitmaps) {
		for (int i = 0; i < pComp->numNames; i++)
			delete pComp->apStateBitmaps[i];

		delete[] pComp->apStateBitmaps;
		pComp->apStateBitmaps = nullptr;
	}
}

const char *Cos::getCostumeName(int hCostume) {
	int index = handleToIndex(hCostume);

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::getCostumeName(): invalid costume handle %d", hCostume);
		return nullptr;
	}

	return _aCostumes[index].name;
}

int Cos::compareFunction(const void *pA, const void *pB) {
	uint32 timeA = (uint32)((const CosChoreKey *)pA)->time;
	uint32 timeB = (uint32)((const CosChoreKey *)pB)->time;

	if (timeB > timeA)
		return -1;

	return timeB < timeA ? 1 : 0;
}

} // End of namespace Grim
