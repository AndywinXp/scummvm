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

int Cos::save(Common::WriteStream *stream) {
	int numSaved = 0;

	stream->writeSint32LE(_maxCostumes);

	for (int i = 0; i < _maxCostumes; i++) {
		if (!_costumes[i].free && (!_saveVerify || _saveVerify(_costumes[i].name)))
			numSaved++;
	}

	stream->writeSint32LE(numSaved);

	for (int i = 0; i < _maxCostumes; i++) {
		if (!_costumes[i].free && (!_saveVerify || _saveVerify(_costumes[i].name))) {
			stream->writeSint32LE(indexToHandle(i));
			stream->write(_costumes[i].name, 32);
		}
	}

	stream->writeSint32LE(_maxInstances);

	return 0;
}

int Cos::restore(Common::ReadStream *stream) {
	for (int i = 0; i < _maxCostumes; i++) {
		if (!_costumes[i].free)
			freeCostume(indexToHandle(i));
	}

	int newMaxCostumes = stream->readSint32LE();

	if (stream->err() || stream->eos() || newMaxCostumes < 0) {
		warning("Cos::restore(): invalid costume table size");
		return 0;
	}

	if (newMaxCostumes > _maxCostumes) {
		CosCostume *costumes = new CosCostume[newMaxCostumes]();

		for (int i = 0; i < _maxCostumes; i++)
			costumes[i] = _costumes[i];

		delete[] _costumes;
		_costumes = costumes;

		for (int i = _maxCostumes; i < newMaxCostumes; i++)
			_costumes[i].free = 1;

		_maxCostumes = newMaxCostumes;
	}

	int numSaved = stream->readSint32LE();

	for (int i = 0; i < numSaved; i++) {
		char name[32];
		int costume = stream->readSint32LE();
		stream->read(name, 32);

		if (stream->err() || stream->eos()) {
			warning("Cos::restore(): truncated costume list");
			return 0;
		}

		name[31] = 0;
		loadEntry(name, costume);

		int index = handleToIndex(costume);

		if (index < 0 || index >= _maxCostumes)
			warning("Cos::restore(): invalid costume handle %d", costume);
		else
			_costumes[index].free = 0;

		_numCostumes++;
	}

	int newMaxInstances = stream->readSint32LE();

	if (stream->err() || stream->eos() || newMaxInstances < 0) {
		warning("Cos::restore(): invalid instance table size");
		return 0;
	}

	if (newMaxInstances > _maxInstances) {
		delete[] _instances;
		_instances = new CosInstance[newMaxInstances]();
		_maxInstances = newMaxInstances;
	}

	for (int i = 0; i < _maxInstances; i++) {
		CosInstance *instance = &_instances[i];
		instance->costume = 0;
		instance->choreInstances = nullptr;
		instance->userData = 0;
		instance->instanceData = nullptr;
		instance->free = 0;
		instance->free = 1;
	}

	_maxInstances = newMaxInstances;

	return 0;
}

int Cos::saveInstance(int inst, Common::WriteStream *stream) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::saveInstance(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numComponents; i++) {
		CosComponentClass *classInfo = &_classes[costume->components[i].classIdx];
		classInfo->handler->saveLoad(inst, i, 1, instance->instanceData[i], stream, nullptr);
	}

	for (int i = 0; i < costume->numChores; i++) {
		CosChoreInstance *choreInst = &instance->choreInstances[i];
		stream->writeUint32LE(choreInst->state);
		stream->writeUint32LE(choreInst->choreIndex);
		stream->writeSint32LE(choreInst->time);
	}

	return 0;
}

int Cos::restoreInstance(int inst, Common::ReadStream *stream) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::restoreInstance(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numComponents; i++) {
		CosComponentClass *classInfo = &_classes[costume->components[i].classIdx];
		classInfo->handler->saveLoad(inst, i, 0, instance->instanceData[i], nullptr, stream);
	}

	for (int i = 0; i < costume->numChores; i++) {
		CosChoreInstance *choreInst = &instance->choreInstances[i];
		choreInst->state = stream->readUint32LE();
		choreInst->choreIndex = stream->readUint32LE();
		choreInst->time = stream->readSint32LE();

		if (choreInst->choreIndex >= (uint32)costume->numChores) {
			warning("Cos::restoreInstance(): chore instance %d refers to the invalid chore %u", i, choreInst->choreIndex);
			choreInst->choreIndex = i;
		}
	}

	return 0;
}

} // End of namespace Grim
