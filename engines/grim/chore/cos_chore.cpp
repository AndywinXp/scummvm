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

int Cos::instStopChores(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instStopChores(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numChores; i++) {
		CosChoreInstance *choreInst = &instance->choreInstances[i];
		uint32 choreIndex = choreInst->choreIndex;
		choreInst->state = 0;
		choreInst->time = 0;

		if (choreIndex >= (uint32)costume->numChores) {
			warning("Cos::instStopChores(): chore instance %d refers to the invalid chore %u", i, choreIndex);
			continue;
		}

		CosChore *chore = &costume->chores[choreIndex];

		for (int t = 0; t < chore->numTracks; t++) {
			int compIndex = chore->tracks[t].compIndex;
			CosComponentClass *classInfo = &_classes[costume->components[compIndex].classIdx];
			classInfo->handler->stop(instance->instanceData[compIndex]);
		}
	}

	return 1;
}

int Cos::instStopChore(int inst, int choreIndex) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instStopChore(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	if (choreIndex < 0 || choreIndex >= costume->numChores)
		return 0;

	CosChoreInstance *choreInst = &instance->choreInstances[choreIndex];
	choreInst->state = 0;
	choreInst->time = 0;

	if (choreInst->choreIndex >= (uint32)costume->numChores) {
		warning("Cos::instStopChore(): chore instance %d refers to the invalid chore %u", choreIndex, choreInst->choreIndex);
		return 1;
	}

	CosChore *chore = &costume->chores[choreInst->choreIndex];

	for (int t = 0; t < chore->numTracks; t++) {
		int compIndex = chore->tracks[t].compIndex;
		CosComponentClass *classInfo = &_classes[costume->components[compIndex].classIdx];
		classInfo->handler->stop(instance->instanceData[compIndex]);
	}

	return 1;
}

int Cos::instPauseChores(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instPauseChores(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numChores; i++)
		instance->choreInstances[i].state &= ~kCosChoreStatePlaying;

	return 1;
}

int Cos::instPauseChore(int inst, int choreIndex) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instPauseChore(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];

	if (choreIndex < 0 || choreIndex >= _costumes[instance->costume].numChores)
		return 0;

	instance->choreInstances[choreIndex].state &= ~kCosChoreStatePlaying;

	return 1;
}

int Cos::instCompleteChore(int inst, int choreIndex) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instCompleteChore(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	if (choreIndex < 0 || choreIndex >= costume->numChores)
		return 0;

	instance->choreInstances[choreIndex].state = kCosChoreStatePlaying;
	updateChoreInstance(costume, instance, choreIndex, costume->chores[choreIndex].length - instance->choreInstances[choreIndex].time, inst);

	return 1;
}

int Cos::instCompleteChores(int inst) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instCompleteChores(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numChores; i++) {
		CosChoreInstance *choreInst = &instance->choreInstances[i];

		if (choreInst->state & kCosChoreStatePlaying)
			updateChoreInstance(costume, instance, i, costume->chores[i].length - choreInst->time, inst);
	}

	return 1;
}

int Cos::instPlayChore(int inst, int choreIndex, int loop, int advance) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instPlayChore(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];

	if (choreIndex < 0 || choreIndex >= _costumes[instance->costume].numChores)
		return 0;

	CosChoreInstance *choreInst = &instance->choreInstances[choreIndex];
	choreInst->state = kCosChoreStatePlaying;

	if (loop)
		choreInst->state = kCosChoreStatePlaying | kCosChoreStateLooping;

	if (!advance)
		choreInst->state |= kCosChoreStateNoAdvance;

	return 1;
}

int Cos::instUpdateChore(int inst, int choreIndex, int deltaMs) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instUpdateChore(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];

	if (choreIndex < 0 || choreIndex >= _costumes[instance->costume].numChores)
		return 0;

	if (instance->choreInstances[choreIndex].state & kCosChoreStatePlaying)
		updateChoreInstance(&_costumes[instance->costume], instance, choreIndex, deltaMs, inst);

	return 1;
}

int Cos::instUpdateChores(int inst, int deltaMs) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instUpdateChores(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];
	CosCostume *costume = &_costumes[instance->costume];

	for (int i = 0; i < costume->numChores; i++) {
		uint32 state = instance->choreInstances[i].state;

		if ((state & kCosChoreStatePlaying) && (state & kCosChoreStateNoAdvance))
			updateChoreInstance(costume, instance, i, deltaMs, inst);
	}

	return 1;
}

int Cos::instSetChoreLoop(int inst, int choreIndex, int loop) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instSetChoreLoop(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];

	if (choreIndex < 0 || choreIndex >= _costumes[instance->costume].numChores)
		return 0;

	CosChoreInstance *choreInst = &instance->choreInstances[choreIndex];

	if (loop)
		choreInst->state |= kCosChoreStateLooping;
	else
		choreInst->state &= ~kCosChoreStateLooping;

	return 1;
}

int Cos::instSetChoreAdv(int inst, int choreIndex, int advance) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instSetChoreAdv(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];

	if (choreIndex < 0 || choreIndex >= _costumes[instance->costume].numChores)
		return 0;

	CosChoreInstance *choreInst = &instance->choreInstances[choreIndex];

	if (advance)
		choreInst->state &= ~kCosChoreStateNoAdvance;
	else
		choreInst->state |= kCosChoreStateNoAdvance;

	return 1;
}

int Cos::instChoreStatus(int inst, int choreIndex) {
	int index = instHandleToIndex(inst);

	if (index < 0 || index >= _maxInstances || _instances[index].free) {
		warning("Cos::instChoreStatus(): invalid instance handle %d", inst);
		return 0;
	}

	CosInstance *instance = &_instances[index];

	if (choreIndex < 0 || choreIndex >= _costumes[instance->costume].numChores)
		return 0;

	return instance->choreInstances[choreIndex].state;
}

int Cos::updateChoreInstance(CosCostume *costume, CosInstance *instance, int choreIndex, int deltaMs, int inst) {
	CosChore *chore = &costume->chores[choreIndex];
	CosChoreInstance *choreInst = &instance->choreInstances[choreIndex];
	int result = 0;

	if (deltaMs == 0)
		return result;

	while (true) {
		for (int t = 0; t < chore->numTracks; t++) {
			CosChoreTrack *track = &chore->tracks[t];
			int k = 0;

			if (track->numKeys == 0)
				continue;

			while (k < track->numKeys && track->keys[k].time < choreInst->time)
				k++;

			while (k < track->numKeys) {
				CosChoreKey *key = &track->keys[k];
				int endTime = (int)((uint32)choreInst->time + (uint32)deltaMs);

				if (key->time >= endTime)
					break;

				int compIndex = track->compIndex;
				changeComponent(2, &_classes[costume->components[compIndex].classIdx], instance->instanceData[compIndex], key,
					(int)((uint32)endTime - (uint32)key->time), key, 0, inst);
				k++;
			}
		}

		int newTime = (int)((uint32)deltaMs + (uint32)choreInst->time);
		choreInst->time = newTime;
		result = newTime;

		if (newTime < chore->length)
			break;

		if ((choreInst->state & kCosChoreStateLooping) == 0) {
			choreInst->state = 0;
			choreInst->time = 0;
			return result;
		}

		if (chore->length <= 0) {
			warning("Cos::updateChoreInstance(): looping chore %d has no length", choreIndex);
			choreInst->state = 0;
			choreInst->time = 0;
			return result;
		}

		deltaMs = (int)((uint32)newTime - (uint32)chore->length);
		choreInst->time = 0;
	}

	return result;
}

void Cos::changeComponent(int op, CosComponentClass *classInfo, void *instData, CosChoreKey *key, int timeOffset, CosChoreKey *nextKey, int arg7, int inst) {
	if (classInfo->type < kCosTagStateBased || classInfo->type > kCosTagFloat) {
		warning("Cos::changeComponent(): invalid tag type %d", classInfo->type);
		return;
	}

	classInfo->handler->change(classInfo->fourcc, instData, op, key->value, timeOffset, nextKey->value, arg7, inst);
}

} // End of namespace Grim
