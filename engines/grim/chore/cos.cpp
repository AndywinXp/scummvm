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

Cos::Cos() : _hs(nullptr), _initialized(0), _started(0), _numCostumes(0), _maxCostumes(0), _costumes(nullptr), _nextCostumeId(0),
		_numClasses(0), _maxClasses(0), _classes(nullptr), _numInstances(0), _maxInstances(0), _instances(nullptr),
		_compMap(nullptr), _compMapSize(0), _lineNumber(0), _instanceHook(nullptr), _saveHook(nullptr), _saveVerify(nullptr),
		_compNotifier(nullptr), _bitmapMunger(nullptr) {
	memset(_tagMap, 0, sizeof(_tagMap));
	memset(_lineBuffer, 0, sizeof(_lineBuffer));
}

int Cos::initialize(SysStruct *hs) {
	_hs = hs;
	_initialized = 1;

	return 0;
}

int Cos::terminate() {
	_hs = nullptr;

	if (_initialized)
		_initialized = 0;

	return 0;
}

int Cos::startup() {
	if (_started)
		return 1;

	_numCostumes = 0;
	_maxCostumes = 32;
	_costumes = new CosCostume[32]();

	for (int i = 0; i < 32; i++)
		_costumes[i].free = 1;

	_numClasses = 0;
	_maxClasses = 32;
	_classes = new CosComponentClass[32]();

	_numInstances = 0;
	_maxInstances = 32;
	_instances = new CosInstance[32]();

	for (int i = 0; i < 32; i++)
		_instances[i].free = 1;

	_started = 1;

	return 0;
}

int Cos::shutdown() {
	_maxCostumes = 0;
	_numCostumes = 0;
	delete[] _costumes;
	_costumes = nullptr;

	_maxClasses = 0;
	_numClasses = 0;
	delete[] _classes;
	_classes = nullptr;

	_maxInstances = 0;
	_numInstances = 0;
	delete[] _instances;
	_instances = nullptr;

	if (_compMapSize) {
		delete[] _compMap;
		_compMap = nullptr;
		_compMapSize = 0;
	}

	_started = 0;

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
		if (_costumes[i].free) {
			_costumes[i].free = 0;
			_numCostumes++;
			return i;
		}
	}

	int oldMax = _maxCostumes;

	if (_numCostumes + 1 > _maxCostumes) {
		_maxCostumes += 8;

		CosCostume *costumes = new CosCostume[_maxCostumes]();

		for (int i = 0; i < oldMax; i++)
			costumes[i] = _costumes[i];

		delete[] _costumes;
		_costumes = costumes;

		for (int i = oldMax; i < _maxCostumes; i++)
			_costumes[i].free = 1;
	}

	int index = _numCostumes;

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::allocCostumeSlot(): costume counter %d is out of range, using slot %d", index, oldMax);
		index = oldMax;
	}

	_numCostumes++;
	_costumes[index].free = 0;

	return index;
}

int Cos::allocInstanceSlot() {
	for (int i = 0; i < _maxInstances; i++) {
		if (_instances[i].free) {
			_instances[i].free = 0;
			_numInstances++;
			return i;
		}
	}

	int oldMax = _maxInstances;

	if (_numInstances + 1 > _maxInstances) {
		_maxInstances += 8;

		CosInstance *instances = new CosInstance[_maxInstances]();

		for (int i = 0; i < oldMax; i++)
			instances[i] = _instances[i];

		delete[] _instances;
		_instances = instances;

		for (int i = oldMax; i < _maxInstances; i++)
			_instances[i].free = 1;
	}

	int index = _numInstances;

	if (index < 0 || index >= _maxInstances) {
		warning("Cos::allocInstanceSlot(): instance counter %d is out of range, using slot %d", index, oldMax);
		index = oldMax;
	}

	_numInstances++;
	_instances[index].free = 0;

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
		if (_classes[i].fourcc == fourcc)
			return nullptr;
	}

	if (strlen(name) >= 32)
		return nullptr;

	int index = allocTagSlot();

	if (index < 0)
		return nullptr;

	CosComponentClass *classInfo = &_classes[index];
	Common::strlcpy(classInfo->name, name, 32);
	classInfo->fourcc = fourcc;
	classInfo->param = param;

	return classInfo;
}

int Cos::newStateBasedTag(int fourcc, const char *name, int unused, CosComponentHandler *handler, int param) {
	if (!handler) {
		warning("Cos::newStateBasedTag(): null handler for tag %s", name);
		return 0;
	}

	CosComponentClass *classInfo = newTag(fourcc, name, param);

	if (!classInfo)
		return 0;

	classInfo->type = kCosTagStateBased;
	classInfo->handler = handler;

	return 1;
}

int Cos::newIntTag(int fourcc, const char *name, int unused, CosComponentHandler *handler, int param) {
	if (!handler) {
		warning("Cos::newIntTag(): null handler for tag %s", name);
		return 0;
	}

	CosComponentClass *classInfo = newTag(fourcc, name, param);

	if (!classInfo)
		return 0;

	classInfo->type = kCosTagInt;
	classInfo->handler = handler;

	return 1;
}

int Cos::newFloatTag(int fourcc, const char *name, int unused, CosComponentHandler *handler, int param) {
	if (!handler) {
		warning("Cos::newFloatTag(): null handler for tag %s", name);
		return 0;
	}

	CosComponentClass *classInfo = newTag(fourcc, name, param);

	if (!classInfo)
		return 0;

	classInfo->type = kCosTagFloat;
	classInfo->handler = handler;

	return 1;
}

CosInstanceHook Cos::registerInstanceHook(CosInstanceHook hook) {
	CosInstanceHook oldHook = _instanceHook;
	_instanceHook = hook;

	return oldHook;
}

CosSaveHook Cos::registerSaveHook(CosSaveHook hook) {
	CosSaveHook oldHook = _saveHook;
	_saveHook = hook;

	return oldHook;
}

CosSaveVerify Cos::registerSaveVerify(CosSaveVerify verify) {
	_saveVerify = verify;

	return verify;
}

CosCompNotifier Cos::registerCompNotifier(CosCompNotifier notifier) {
	CosCompNotifier oldNotifier = _compNotifier;
	_compNotifier = notifier;

	return oldNotifier;
}

CosBitmapMunger Cos::registerBitmapMunger(CosBitmapMunger munger) {
	CosBitmapMunger oldMunger = _bitmapMunger;
	_bitmapMunger = munger;

	return oldMunger;
}

void Cos::initCostume(CosCostume *costume) {
	Common::strlcpy(costume->name, "<unnamed>", 32);
	costume->uniqueId = _nextCostumeId++;
	costume->numComponents = 0;
	costume->components = nullptr;
	costume->numChores = 0;
	costume->chores = nullptr;
}

int Cos::newCostume() {
	int index = allocCostumeSlot();

	if (index < 0)
		return 0;

	initCostume(&_costumes[index]);

	return indexToHandle(index);
}

int Cos::load(const char *path) {
	for (int i = 0; i < _maxCostumes; i++) {
		if (!_costumes[i].free && scumm_stricmp(path, _costumes[i].name) == 0)
			return indexToHandle(i);
	}

	int index = allocCostumeSlot();
	int costume = indexToHandle(index);

	if (!loadEntry(path, costume)) {
		freeCostumeData(indexToHandle(index));
		return 0;
	}

	return costume;
}

int Cos::loadEntry(const char *path, int costumeHandle) {
	int index = handleToIndex(costumeHandle);

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::loadEntry(): invalid costume handle %d", costumeHandle);
		return 0;
	}

	CosCostume *costume = &_costumes[index];
	initCostume(costume);

	const char *name = strrchr(path, '\\');

	if (name) {
		name++;

		if (strlen(name) > 0x20) {
			costume->free = 1;
			return 0;
		}

		Common::strlcpy(costume->name, name, 32);
	} else {
		if (strlen(path) > 0x20) {
			costume->free = 1;
			return 0;
		}

		Common::strlcpy(costume->name, path, 32);
	}

	Common::SeekableReadStream *file = _hs->openFile(path);

	if (!file) {
		costume->free = 1;
		return 0;
	}

	_lineNumber = 0;

	int major = 0;
	int minor = 0;
	bool failed = false;

	if (!readLine(file, _lineBuffer, 1024)) {
		failed = true;
	} else {
		sscanf(_lineBuffer, " costume v%d.%d", &major, &minor);

		if (!readLine(file, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section tags")
				|| !parseTags(file)
				|| !readLine(file, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section components")
				|| !parseComponents(file, costumeHandle)
				|| !readLine(file, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section chores")
				|| !parseChores(file, costume)
				|| !readLine(file, _lineBuffer, 1024)
				|| sscanf(_lineBuffer, " section keys")
				|| (costume->numChores && !parseKeys(file, costume)))
			failed = true;
	}

	if (!failed) {
		for (int c = 0; c < costume->numChores && !failed; c++) {
			CosChore *chore = &costume->chores[c];
			setChoreLength(costume, c, chore->length);

			for (int i = 0; i < chore->numTracks && !failed; i++) {
				for (int j = 0; j < chore->numTracks; j++) {
					if (i != j && chore->tracks[i].compIndex == chore->tracks[j].compIndex) {
						failed = true;
						break;
					}
				}
			}
		}
	}

	delete file;

	if (failed) {
		costume->free = 1;
		return 0;
	}

	return 1;
}

int Cos::freeCostume(int costume) {
	int index = handleToIndex(costume);

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::freeCostume(): invalid costume handle %d", costume);
		return _numCostumes;
	}

	for (int i = 0; i < _maxInstances; i++) {
		if (!_instances[i].free && _instances[i].costume == index)
			freeCosInstance(instIndexToHandle(i));
	}

	freeCostumeData(costume);
	_costumes[index].free = 1;

	return --_numCostumes;
}

void Cos::freeCostumeData(int costumeHandle) {
	CosCostume *costume = ptrFromHandle(costumeHandle);

	if (!costume) {
		warning("Cos::freeCostumeData(): invalid costume handle %d", costumeHandle);
		return;
	}

	for (int i = 0; i < costume->numComponents; i++) {
		CosComponent *comp = &costume->components[i];

		if (comp->classIdx < 0 || comp->classIdx >= _numClasses) {
			warning("Cos::freeCostumeData(): component %d has an invalid tag index", i);
			continue;
		}

		freeComponent(costumeHandle, &_classes[comp->classIdx], comp);
	}

	delete[] costume->components;
	costume->components = nullptr;
	costume->numComponents = 0;

	for (int c = 0; c < costume->numChores; c++) {
		CosChore *chore = &costume->chores[c];

		for (int t = 0; chore->tracks && t < chore->numTracks; t++) {
			delete[] chore->tracks[t].keys;
			chore->tracks[t].keys = nullptr;
		}

		delete[] chore->tracks;
		chore->tracks = nullptr;
		chore->numTracks = 0;
	}

	delete[] costume->chores;
	costume->chores = nullptr;
	costume->numChores = 0;

	costume->free = 1;
}

void Cos::freeComponent(int costume, CosComponentClass *classInfo, CosComponent *comp) {
	if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat)
		warning("Cos::freeComponent(): invalid tag type %d", classInfo->type);
	else if (classInfo->handler)
		classInfo->handler->freeComponent(costume, classInfo->fourcc, comp->id);

	delete[] comp->nameIds;
	comp->nameIds = nullptr;

	if (comp->names) {
		for (int i = 0; i < comp->numNames; i++)
			delete[] comp->names[i];

		delete[] comp->names;
		comp->names = nullptr;
	}

	if (comp->stateBitmaps) {
		for (int i = 0; i < comp->numNames; i++)
			delete comp->stateBitmaps[i];

		delete[] comp->stateBitmaps;
		comp->stateBitmaps = nullptr;
	}
}

const char *Cos::getCostumeName(int costume) {
	int index = handleToIndex(costume);

	if (index < 0 || index >= _maxCostumes) {
		warning("Cos::getCostumeName(): invalid costume handle %d", costume);
		return nullptr;
	}

	return _costumes[index].name;
}

int Cos::compareFunction(const void *a, const void *b) {
	uint32 timeA = (uint32)((const CosChoreKey *)a)->time;
	uint32 timeB = (uint32)((const CosChoreKey *)b)->time;

	if (timeB > timeA)
		return -1;

	return timeB < timeA ? 1 : 0;
}

} // End of namespace Grim
