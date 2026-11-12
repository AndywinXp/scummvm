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

#include "engines/grim/emi/costume/choreinstance.h"
#include "engines/grim/emi/costume/component.h"
#include "engines/grim/emi/costume/componentinstance.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

ChoreInstance::ChoreInstance(EmiEngine *engine, Actor *actor, uint instanceId) : LResourceHandle<ChoreSet>(engine) {
	id = instanceId;
	_engine->_choreInstances[id] = this;
	owner = actor;
	flags = 0;
	time = 0.0f;
	stopTime = 0.0f;

	resource = getManager()->findResource("");

	if (!resource) {
		resource = getManager()->createResource("");

		if (!resource)
			allocateResource();

		resource->resSetName("");
		getManager()->addResource(resource);
	}

	setResource(resource);
}

ChoreInstance::ChoreInstance(EmiEngine *engine, Actor *actor, const LString &newChoreName, const LString &choreSetName) : LResourceHandle<ChoreSet>(engine) {
	id = _engine->_nextChoreInstanceId++;
	_engine->_choreInstances[id] = this;
	flags = 0;
	owner = actor;
	time = 0.0f;
	stopTime = 0.0f;

	bind(newChoreName, choreSetName);
}

ChoreInstance::~ChoreInstance() {
	for (uint i = 0; i < instances.size(); i++) {
		delete instances[i];
		instances[i] = nullptr;
	}

	instances.clear();
	_engine->_choreInstances.erase(id);
}

void *ChoreInstance::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void ChoreInstance::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void ChoreInstance::bind(const LString &newChoreName, const LString &choreSetName) {
	choreName = newChoreName;

	{
		LResourceHandle<ChoreSet> choreSetHandle(_engine, choreSetName);

		setResource(choreSetHandle.resource);
	}

	Chore *chore = nullptr;

	if (lock() && typed)
		chore = typed->findChore(choreName);

	if (chore) {
		for (uint i = 0; i < chore->tracks.size(); i++) {
			ChoreTrack *track = chore->tracks[i];

			if (!track) {
				warning("ChoreInstance::bind(): missing track %u", i);
				instances.push_back(nullptr);
				continue;
			}

			LResourceHandle<Component> componentHandle(_engine, track->resourceName);

			if (componentHandle.lock() && componentHandle.typed)
				instances.push_back(componentHandle.typed->createInstance(owner));
			else
				instances.push_back(nullptr);

			componentHandle.unlock();
		}

		for (uint i = 0; i < chore->tracks.size(); i++) {
			ChoreTrack *track = chore->tracks[i];

			if (!track || track->parentId < 0)
				continue;

			int parentIndex = chore->findTrackIndex(track->parentId);

			if (parentIndex < 0)
				continue;

			if (i >= instances.size() || (uint)parentIndex >= instances.size() || !instances[i]) {
				warning("ChoreInstance::bind(): missing instance for track %u", i);
				continue;
			}

			instances[i]->setParent(instances[parentIndex]);
		}
	} else {
		flags = kFinished;
	}

	unlock();
}

bool ChoreInstance::isPlaying() {
	return (flags & kPlaying) != 0;
}

bool ChoreInstance::isFinished() {
	return (flags & kFinished) != 0;
}

bool ChoreInstance::isLooping() {
	return (flags & kLooping) != 0;
}

bool ChoreInstance::setPlaying() {
	flags |= kPlaying;

	return true;
}

bool ChoreInstance::fadeIn(float seconds) {
	if (isPlaying())
		return false;

	if (!(seconds > 0.0f))
		return false;

	for (uint i = 0; i < instances.size(); i++) {
		if (instances[i])
			instances[i]->fadeIn(seconds);
	}

	setPlaying();

	return true;
}

bool ChoreInstance::fadeOut(float seconds) {
	if (!isPlaying())
		return false;

	if (!(seconds > 0.0f))
		return false;

	ChoreSet *choreSet = nullptr;

	if (lock())
		choreSet = typed;

	Chore *chore = nullptr;

	if (choreSet)
		chore = choreSet->findChore(choreName);
	else
		warning("ChoreInstance::fadeOut(): no chore set");

	if (chore && !(chore->length > seconds)) {
		stop();
		unlock();

		return true;
	}

	unlock();

	for (uint i = 0; i < instances.size(); i++) {
		if (instances[i])
			instances[i]->fadeOut(seconds);
	}

	stopTime = (float)((double)seconds + (double)time);

	return true;
}

bool ChoreInstance::stop() {
	for (uint i = 0; i < instances.size(); i++) {
		if (instances[i])
			instances[i]->reset();
	}

	stopTime = -1.0f;
	flags = kFinished;

	return true;
}

bool ChoreInstance::pause() {
	flags &= ~kPlaying;

	return true;
}

bool ChoreInstance::complete() {
	flags = kPlaying;

	ChoreSet *choreSet = nullptr;

	if (lock())
		choreSet = typed;

	Chore *chore = nullptr;

	if (choreSet)
		chore = choreSet->findChore(choreName);
	else
		warning("ChoreInstance::complete(): no chore set");

	if (chore)
		update((float)((double)chore->length - (double)time), false);

	unlock();

	return true;
}

bool ChoreInstance::update(float deltaTime, bool replay) {
	bool stopRequested = false;

	if (!(deltaTime >= 0.0f))
		return false;

	if (!isPlaying())
		return true;

	if (!lock())
		return false;

	if (!typed)
		return false;

	Chore *chore = typed->findChore(choreName);

	if (!chore)
		return false;

	float fullDeltaTime = deltaTime;

	if (stopTime > 0.0f && (double)deltaTime + (double)time > (double)stopTime) {
		stopTime = 0.0f;
		stopRequested = true;
	}

	while (true) {
		for (uint i = 0; i < chore->tracks.size(); i++) {
			ChoreTrack *track = chore->tracks[i];

			if (!track) {
				warning("ChoreInstance::update(): missing track %u", i);
				continue;
			}

			if (track->keys.empty())
				continue;

			if (i >= instances.size()) {
				warning("ChoreInstance::update(): missing instance for track %u", i);
				continue;
			}

			ComponentInstance *instance = instances[i];

			if (!instance)
				continue;

			if (replay && !instance->isReplayable())
				continue;

			Common::List<ChoreKey *>::iterator current = track->keys.begin();

			while (current != track->keys.end() && !((*current)->time >= time))
				++current;

			Common::List<ChoreKey *>::iterator next = current;

			if (next != track->keys.end())
				++next;

			while (current != track->keys.end()) {
				ChoreKey *key = *current;
				float limitTime = (float)((double)deltaTime + (double)time);

				if ((double)key->time > (double)limitTime)
					break;

				if (next == track->keys.end()) {
					if (isLooping()) {
						ChoreKey *firstKey = *track->keys.begin();
						double loopEnd = (double)chore->length + (double)firstKey->time;

						if (loopEnd > (double)limitTime)
							instance->setKey(key->value, (float)((double)limitTime - (double)key->time), firstKey->value, (float)(loopEnd - (double)limitTime), true);
						else
							instance->setKey(key->value, 0.0f, firstKey->value, (float)(loopEnd - (double)key->time), false);
					} else {
						double fullTime = (double)deltaTime + (double)time;

						instance->setKey(key->value, (float)(fullTime - (double)key->time), key->value, (float)((double)key->time - fullTime), true);
					}
				} else {
					ChoreKey *nextKey = *next;

					if ((double)nextKey->time > (double)limitTime)
						instance->setKey(key->value, (float)((double)limitTime - (double)key->time), nextKey->value, (float)((double)nextKey->time - (double)limitTime), true);
					else
						instance->setKey(key->value, 0.0f, nextKey->value, (float)((double)nextKey->time - (double)key->time), false);
				}

				++current;
				++next;
			}
		}

		double newTime = (double)deltaTime + (double)time;

		time = (float)newTime;

		if (!(newTime >= (double)chore->length))
			break;

		if (!isLooping()) {
			flags = kFinished;
			break;
		}

		if (!(chore->length > 0.0f)) {
			warning("ChoreInstance::update(): looping chore with no length");
			break;
		}

		deltaTime = (float)((double)time - (double)chore->length);
		time = 0.0f;

		if (!(chore->length >= stopTime))
			stopTime = (float)((double)stopTime - (double)chore->length);
	}

	for (uint i = 0; i < chore->tracks.size(); i++) {
		if (i >= instances.size()) {
			warning("ChoreInstance::update(): missing instance for track %u", i);
			continue;
		}

		if (instances[i])
			instances[i]->update(fullDeltaTime);
	}

	unlock();

	if (stopRequested)
		stop();

	return true;
}

bool ChoreInstance::setLooping(bool looping) {
	if (looping)
		flags |= kLooping;
	else
		flags &= ~kLooping;

	return true;
}

bool ChoreInstance::save(tSaveRestoreAccess *access) {
	LString choreSetName;

	if (resource)
		choreSetName = resource->name;
	else
		warning("ChoreInstance::save(): no chore set");

	int length = (int)choreSetName.size() + 1;

	access(&length, sizeof(length));
	access((void *)choreSetName.c_str(), length);

	length = (int)choreName.size() + 1;

	access(&length, sizeof(length));
	access((void *)choreName.c_str(), length);
	access(&time, sizeof(time));
	access(&stopTime, sizeof(stopTime));
	access(&flags, sizeof(flags));

	return true;
}

bool ChoreInstance::restore(tSaveRestoreAccess *access) {
	char buffer[kRestoreBufferSize + 1];
	int length;
	LString choreSetName;
	LString restoredChoreName;
	float savedTime;
	uint savedFlags;

	access(&length, sizeof(length));

	if (length < 1 || length > kRestoreBufferSize) {
		warning("ChoreInstance::restore(): invalid chore set name length %d", length);

		return false;
	}

	access(buffer, length);
	buffer[length] = 0;
	choreSetName = buffer;

	access(&length, sizeof(length));

	if (length < 1 || length > kRestoreBufferSize) {
		warning("ChoreInstance::restore(): invalid chore name length %d", length);

		return false;
	}

	access(buffer, length);
	buffer[length] = 0;
	restoredChoreName = buffer;

	bind(restoredChoreName, choreSetName);

	access(&savedTime, sizeof(savedTime));
	access(&stopTime, sizeof(stopTime));
	access(&savedFlags, sizeof(savedFlags));

	flags = kPlaying;

	bool result = update(savedTime, true);

	flags = savedFlags;

	return result;
}

uint ChoreInstance::newInstanceWithId(EmiEngine *engine, Actor *actor, uint instanceId) {
	ChoreInstance *instance = new ChoreInstance(engine, actor, instanceId);

	return instance->id;
}

uint ChoreInstance::newInstance(EmiEngine *engine, Actor *actor, const LString &newChoreName, const LString &choreSetName) {
	if (newChoreName.empty() || choreSetName.empty())
		return 0;

	ChoreInstance *instance = new ChoreInstance(engine, actor, newChoreName, choreSetName);

	return instance->id;
}

ChoreInstance *ChoreInstance::getInstance(EmiEngine *engine, uint instanceId) {
	InstanceMap::iterator it = engine->_choreInstances.find(instanceId);

	if (it == engine->_choreInstances.end())
		return nullptr;

	return it->_value;
}

bool ChoreInstance::isInstanceValid(EmiEngine *engine, uint instanceId) {
	if (instanceId == 0)
		return false;

	InstanceMap::iterator it = engine->_choreInstances.find(instanceId);

	if (it == engine->_choreInstances.end())
		return false;

	if (it->_value)
		return true;

	engine->_choreInstances.erase(instanceId);

	return false;
}

bool ChoreInstance::startup(EmiEngine *engine) {
	if (engine->_choreInstancesStarted)
		return false;

	engine->_choreInstancesStarted = true;

	return true;
}

void ChoreInstance::shutdown(EmiEngine *engine) {
	engine->_choreInstancesStarted = false;
}

} // End of namespace Grim
