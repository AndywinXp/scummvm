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

int Cos::createCosInstance(int costumeHandle, int userData) {
	int costumeIndex = handleToIndex(costumeHandle);

	if (costumeIndex < 0 || costumeIndex >= _maxCostumes)
		return 0;

	CosCostume *costume = &_costumes[costumeIndex];
	int slot = allocInstanceSlot();
	_instances[slot].costume = costumeIndex;

	if (costume->numChores)
		_instances[slot].choreInstances = new CosChoreInstance[costume->numChores]();
	else
		_instances[slot].choreInstances = nullptr;

	initChoreInstances(costume->chores, _instances[slot].choreInstances, costume->numChores);

	if (costume->numComponents) {
		_instances[slot].instanceData = new void *[costume->numComponents]();

		for (int i = 0; i < costume->numComponents; i++)
			_instances[slot].instanceData[i] = costume->components[i].id;
	} else {
		_instances[slot].instanceData = nullptr;
	}

	_instances[slot].userData = userData;
	int inst = instIndexToHandle(slot);

	int *treeDepths = nullptr;

	if (costume->numComponents)
		treeDepths = new int[costume->numComponents];

	int maxDepth = 0;

	for (int i = 0; i < costume->numComponents; i++) {
		int parent = costume->components[i].parent;
		int depth = 0;

		while (parent != -1) {
			if (parent < 0 || parent >= costume->numComponents || depth > costume->numComponents) {
				warning("Cos::createCosInstance(): invalid parent chain at component %d", i);
				break;
			}

			parent = costume->components[parent].parent;
			depth++;
		}

		treeDepths[i] = depth;

		if (maxDepth <= depth)
			maxDepth = depth;
	}

	for (int depth = 0; depth <= maxDepth; depth++) {
		for (int i = 0; i < costume->numComponents; i++) {
			if (treeDepths[i] != depth)
				continue;

			CosComponentClass *classInfo = &_classes[costume->components[i].classIdx];
			void *data = nullptr;

			if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat)
				warning("Cos::createCosInstance(): invalid tag type %d", classInfo->type);
			else
				data = classInfo->handler->instance(inst, i, 1);

			if (data)
				_instances[slot].instanceData[i] = data;
		}
	}

	delete[] treeDepths;

	if (_instanceHook) {
		if (!_instanceHook(inst, 1)) {
			freeCosInstance(inst);
			return 0;
		}
	} else {
		_instances[slot].userData = 0;
	}

	return inst;
}

int Cos::freeCosInstance(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances) {
		warning("Cos::freeCosInstance(): invalid instance handle %d", inst);
		return _numInstances;
	}

	if (_instanceHook)
		_instanceHook(inst, 0);

	freeInstance(index);
	_instances[index].free = 1;

	return --_numInstances;
}

void Cos::freeInstance(int index) {
	CosInstance *instance = &_instances[index];
	int inst = instIndexToHandle(index);

	if (instance->free)
		return;

	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numComponents; i++) {
		CosComponentClass *classInfo = &_classes[costume->components[i].classIdx];
		void *data = nullptr;

		if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat)
			warning("Cos::freeInstance(): invalid tag type %d", classInfo->type);
		else
			data = classInfo->handler->instance(inst, i, 0);

		if (data)
			instance->instanceData[i] = data;
	}

	delete[] instance->choreInstances;
	delete[] instance->instanceData;

	instance->costume = 0;
	instance->choreInstances = nullptr;
	instance->userData = 0;
	instance->instanceData = nullptr;
	instance->free = 0;
	instance->costume = -1;
	instance->free = 1;
}

void Cos::instRescue(int inst, int costume) {
	int costumeIndex = handleToIndex(costume);

	if (costumeIndex >= 0) {
		int instIndex = instHandleToIndex(inst);

		if (instIndex >= 0 && instIndex < _maxInstances)
			_instances[instIndex].costume = costumeIndex;
	}
}

void *Cos::instGetInstanceData(int inst, int compIndex, int *fourcc) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || compIndex < 0)
		return nullptr;

	if (_instances[index].free) {
		warning("Cos::instGetInstanceData(): instance %d is not in use", inst);
		return nullptr;
	}

	CosCostume *costume = &_costumes[_instances[index].costume];

	if (compIndex >= costume->numComponents)
		return nullptr;

	if (fourcc)
		*fourcc = _classes[costume->components[compIndex].classIdx].fourcc;

	return _instances[index].instanceData[compIndex];
}

int Cos::instGetUserData(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances)
		return 0;

	return _instances[index].userData;
}

void Cos::instSetUserData(int inst, int userData) {
	int index = instHandleToIndex(inst);

	if (index >= 0 && index < _maxInstances)
		_instances[index].userData = userData;
}

int Cos::instCountComponents(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0)
		return 0;

	if (index >= _maxInstances || _instances[index].free) {
		warning("Cos::instCountComponents(): invalid instance handle %d", inst);
		return 0;
	}

	return countComponents(&_costumes[_instances[index].costume]);
}

CosComponent *Cos::instComponentInfo(int inst, int compNum, int *fourcc, void **id, int *parent) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instComponentInfo(): invalid instance handle %d", inst);
		return nullptr;
	}

	return componentInfo(&_costumes[_instances[index].costume], compNum, fourcc, id, parent, nullptr);
}

int Cos::instCountChores(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instCountChores(): invalid instance handle %d", inst);
		return 0;
	}

	return _costumes[_instances[index].costume].numChores;
}

CosCostume *Cos::instGetCostume(int inst, int *handle) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instGetCostume(): invalid instance handle %d", inst);
		return nullptr;
	}

	int costumeIndex = _instances[index].costume;

	if (handle)
		*handle = indexToHandle(costumeIndex);

	return &_costumes[costumeIndex];
}

const char *Cos::instChoreName(int inst, int choreIndex) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instChoreName(): invalid instance handle %d", inst);
		return nullptr;
	}

	CosCostume *costume = &_costumes[_instances[index].costume];

	if (choreIndex < 0 || choreIndex >= costume->numChores) {
		warning("Cos::instChoreName(): chore number %d is out of range", choreIndex);
		return nullptr;
	}

	return costume->chores[choreIndex].name;
}

} // End of namespace Grim
