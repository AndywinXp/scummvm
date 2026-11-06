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

#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/chore/cos.h"

namespace Grim {

int Cos::save(Common::WriteStream *pStream) {
	int numSaved = 0;

	pStream->writeSint32LE(_maxCostumes);

	for (int i = 0; i < _maxCostumes; i++) {
		if (!_aCostumes[i].bFree && (!_pSaveVerify || _pSaveVerify(_aCostumes[i].name)))
			numSaved++;
	}

	pStream->writeSint32LE(numSaved);

	for (int i = 0; i < _maxCostumes; i++) {
		if (!_aCostumes[i].bFree && (!_pSaveVerify || _pSaveVerify(_aCostumes[i].name))) {
			pStream->writeSint32LE(indexToHandle(i));
			pStream->write(_aCostumes[i].name, 32);
		}
	}

	pStream->writeSint32LE(_maxInstances);

	return 0;
}

int Cos::restore(Common::ReadStream *pStream) {
	for (int i = 0; i < _maxCostumes; i++) {
		if (!_aCostumes[i].bFree)
			freeCostume(indexToHandle(i));
	}

	int newMaxCostumes = pStream->readSint32LE();

	if (pStream->err() || pStream->eos() || newMaxCostumes < 0) {
		warning("Cos::restore(): invalid costume table size");
		return 0;
	}

	if (newMaxCostumes > _maxCostumes) {
		CosCostume *aCostumes = new CosCostume[newMaxCostumes]();

		for (int i = 0; i < _maxCostumes; i++)
			aCostumes[i] = _aCostumes[i];

		delete[] _aCostumes;
		_aCostumes = aCostumes;

		for (int i = _maxCostumes; i < newMaxCostumes; i++)
			_aCostumes[i].bFree = 1;

		_maxCostumes = newMaxCostumes;
	}

	int numSaved = pStream->readSint32LE();

	for (int i = 0; i < numSaved; i++) {
		char name[32];
		int hCostume = pStream->readSint32LE();
		pStream->read(name, 32);

		if (pStream->err() || pStream->eos()) {
			warning("Cos::restore(): truncated costume list");
			return 0;
		}

		name[31] = 0;
		loadEntry(name, hCostume);

		int index = handleToIndex(hCostume);

		if (index < 0 || index >= _maxCostumes)
			warning("Cos::restore(): invalid costume handle %d", hCostume);
		else
			_aCostumes[index].bFree = 0;

		_numCostumes++;
	}

	int newMaxInstances = pStream->readSint32LE();

	if (pStream->err() || pStream->eos() || newMaxInstances < 0) {
		warning("Cos::restore(): invalid instance table size");
		return 0;
	}

	if (newMaxInstances > _maxInstances) {
		delete[] _aInstances;
		_aInstances = new CosInstance[newMaxInstances]();
		_maxInstances = newMaxInstances;
	}

	for (int i = 0; i < _maxInstances; i++) {
		CosInstance *pInstance = &_aInstances[i];
		pInstance->costume = 0;
		pInstance->aChoreInstances = nullptr;
		pInstance->userData = 0;
		pInstance->aInstanceData = nullptr;
		pInstance->bFree = 0;
		pInstance->bFree = 1;
	}

	_maxInstances = newMaxInstances;

	return 0;
}

int Cos::saveInstance(int hInst, Common::WriteStream *pStream) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::saveInstance(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponentClass *pClass = &_aClasses[pCostume->aComponents[i].classIdx];
		pClass->pHandler->saveLoad(hInst, i, 1, pInstance->aInstanceData[i], pStream, nullptr);
	}

	for (int i = 0; i < pCostume->numChores; i++) {
		CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[i];
		pStream->writeUint32LE(pChoreInst->state);
		pStream->writeUint32LE(pChoreInst->choreIndex);
		pStream->writeSint32LE(pChoreInst->time);
	}

	return 0;
}

int Cos::restoreInstance(int hInst, Common::ReadStream *pStream) {
	int index = instHandleToIndex(hInst);

	if (index < 0 || index >= _maxInstances || _aInstances[index].bFree) {
		warning("Cos::restoreInstance(): invalid instance handle %d", hInst);
		return 0;
	}

	CosInstance *pInstance = &_aInstances[index];
	CosCostume *pCostume = &_aCostumes[pInstance->costume];

	for (int i = 0; i < pCostume->numComponents; i++) {
		CosComponentClass *pClass = &_aClasses[pCostume->aComponents[i].classIdx];
		pClass->pHandler->saveLoad(hInst, i, 0, pInstance->aInstanceData[i], nullptr, pStream);
	}

	for (int i = 0; i < pCostume->numChores; i++) {
		CosChoreInstance *pChoreInst = &pInstance->aChoreInstances[i];
		pChoreInst->state = pStream->readUint32LE();
		pChoreInst->choreIndex = pStream->readUint32LE();
		pChoreInst->time = pStream->readSint32LE();

		if (pChoreInst->choreIndex >= (uint32)pCostume->numChores) {
			warning("Cos::restoreInstance(): chore instance %d refers to the invalid chore %u", i, pChoreInst->choreIndex);
			pChoreInst->choreIndex = i;
		}
	}

	return 0;
}

} // End of namespace Grim
