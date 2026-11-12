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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/resources/lresource.h"
#include "engines/grim/emi/resources/lresourcehandle.h"

namespace Grim {

LResourceType::LResourceType() {
	id = 0;
}

LResourceType::LResourceType(const char *name) {
	id = 0;
	uint index = 0;

	switch (strlen(name)) {
	case 4:
		id += (uint)toupper((byte)name[0]) << 24;
		index = 1;
		// fall through
	case 3:
		id += (uint)toupper((byte)name[index++]) << 16;
		// fall through
	case 2:
		id += (uint)toupper((byte)name[index++]) << 8;
		// fall through
	case 1:
		id += (uint)toupper((byte)name[index]);
		break;
	default:
		break;
	}
}

bool LResourceType::operator==(const LResourceType &other) const {
	return id == other.id;
}

LResource::LResource() {
	_engine = nullptr;
	lockCount = 0;
	loaded = false;
	byte29 = 0;
	unique = false;
	purgeOnFree = false;
	notPurgable = false;
}

LResource::~LResource() {
	if (lockCount != 0)
		warning("LResource::~LResource(): resource is still locked");

	_engine->getResourceManager()->removeResource(this);

	Common::List<LResourceHandleBase *>::iterator it;

	for (it = handles.begin(); it != handles.end(); ++it) {
		warning("LResource::~LResource(): resource still has handles bound");
		(*it)->resource = nullptr;
	}

	handles.clear();
}

void *LResource::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void LResource::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void LResource::initializeRes() {
	lockCount = 0;
	name.clear();
	loaded = false;
	byte29 = 0;
	unique = false;
	purgeOnFree = false;
	notPurgable = false;
}

bool LResource::loadResourceItem(const LString &itemName) {
	return loadResourceItem(itemName.c_str());
}

bool LResource::loadResourceItem(const char *itemName) {
	bool result = resLoadItem(itemName);

	if (result) {
		loaded = true;
	} else {
		LConsoleServices *console = _engine->getConsoleServices();

		if (console)
			console->logError("LResource::LoadResourceItem - Failed to load %s\n", name.c_str());
		else
			warning("LResource::loadResourceItem(): failed to load %s", name.c_str());
	}

	_engine->getResourceManager()->resourceLoaded(this, result);

	return result;
}

uint LResource::resSizeOfItem() {
	return sizeof(LResource);
}

LResource *LResource::resCacheOut(LFile *file) {
	return nullptr;
}

bool LResource::resCacheIn(LFile *file) {
	return false;
}

bool LResource::resLoadItem(const char *itemName) {
	warning("LResource::resLoadItem(): not implemented by the resource");

	return false;
}

bool LResource::resIsLoaded() const {
	return loaded;
}

bool LResource::resIsUnique() const {
	return unique;
}

bool LResource::resIsPurgeOnFree() const {
	return purgeOnFree;
}

bool LResource::resIsNotPurgable() const {
	return notPurgable;
}

void LResource::resSetLoaded(bool value) {
	loaded = value;
}

void LResource::resSetUnique(bool value) {
	unique = value;
}

void LResource::resSetPurgeOnFree(bool value) {
	purgeOnFree = value;
}

void LResource::resSetNotPurgable(bool value) {
	notPurgable = value;
}

LString &LResource::resGetName() {
	return name;
}

const LResourceType &LResource::resGetType() const {
	return type;
}

uint LResource::handleCount() const {
	return handles.size();
}

LResource *LResource::resLock() {
	lockCount++;
	_engine->getResourceManager()->lockResource(this);

	return this;
}

void LResource::resUnlock() {
	lockCount--;
	_engine->getResourceManager()->unlockResource(this);

	if (lockCount == 0 && purgeOnFree)
		_engine->getResourceManager()->purgeResource(this, true);
}

int LResource::resLockCount() const {
	return lockCount;
}

LResourceHandleBase *LResource::firstHandle() {
	if (handles.empty())
		return nullptr;

	return handles.front();
}

void LResource::addHandle(LResourceHandleBase &handle) {
	Common::List<LResourceHandleBase *>::iterator it;

	for (it = handles.begin(); it != handles.end(); ++it) {
		if (*it == &handle)
			return;
	}

	handles.push_front(&handle);
}

void LResource::removeHandle(LResourceHandleBase &handle) {
	Common::List<LResourceHandleBase *>::iterator it;

	for (it = handles.begin(); it != handles.end(); ++it) {
		if (*it == &handle) {
			handles.erase(it);
			return;
		}
	}
}

void LResource::resSetName(const char *newName) {
	LString newNameString(newName);

	_engine->getResourceManager()->resourceNameChange(this, newNameString);

	name = newName;
}

void LResource::resSetType(const LResourceType &newType) {
	_engine->getResourceManager()->resourceTypeChange(this, newType);

	type = newType;
}

void LResource::resReplaceResource(LResource &other) {
	Common::List<LResourceHandleBase *>::iterator it = handles.begin();

	while (it != handles.end()) {
		LResourceHandleBase *handle = *it;
		++it;

		if (handle->lockCount != 0)
			warning("LResource::resReplaceResource(): handle is still locked");

		handle->setResource(&other);
	}
}

void LResource::bindEngine(EmiEngine *engine) {
	_engine = engine;
}

EmiEngine *LResource::getEngine() const {
	return _engine;
}

LResourceManager::~LResourceManager() {
}

bool LResourceManager::resourceLoaded(LResource *resource, bool success) {
	return true;
}

bool LResourceManager::notifyResource(LResource *resource, bool flag) {
	return true;
}

bool LResourceManager::addResource(LResource *resource) {
	return true;
}

bool LResourceManager::removeResource(LResource *resource) {
	return true;
}

LResource *LResourceManager::findResource(const LString &name) {
	return nullptr;
}

LResource *LResourceManager::findResource(const char *name) {
	return nullptr;
}

void LResourceManager::resourceNameChange(LResource *resource, const LString &newName) {
}

void LResourceManager::resourceTypeChange(LResource *resource, const LResourceType &newType) {
}

LResource *LResourceManager::createResource(const LString &name) {
	return nullptr;
}

LResource *LResourceManager::createResource(const char *name) {
	return nullptr;
}

int LResourceManager::purgeResource(LResource *resource, bool logPurge) {
	return 0;
}

void LResourceManager::lockResource(LResource *resource) {
}

void LResourceManager::unlockResource(LResource *resource) {
}

} // End of namespace Grim
