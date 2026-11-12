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
	_id = 0;
}

LResourceType::LResourceType(const char *name) {
	_id = 0;
	uint index = 0;

	switch (strlen(name)) {
	case 4:
		_id += (uint)toupper((byte)name[0]) << 24;
		index = 1;
		// fall through
	case 3:
		_id += (uint)toupper((byte)name[index++]) << 16;
		// fall through
	case 2:
		_id += (uint)toupper((byte)name[index++]) << 8;
		// fall through
	case 1:
		_id += (uint)toupper((byte)name[index]);
		break;
	default:
		break;
	}
}

bool LResourceType::operator==(const LResourceType &other) const {
	return _id == other._id;
}

LResource::LResource() {
	_engine = nullptr;
	_lockCount = 0;
	_loaded = false;
	_byte29 = 0;
	_unique = false;
	_purgeOnFree = false;
	_notPurgable = false;
}

LResource::~LResource() {
	if (_lockCount != 0)
		warning("LResource::~LResource(): resource is still locked");

	_engine->getResourceManager()->removeResource(this);

	Common::List<LResourceHandleBase *>::iterator it;

	for (it = _handles.begin(); it != _handles.end(); ++it) {
		warning("LResource::~LResource(): resource still has handles bound");
		(*it)->_resource = nullptr;
	}

	_handles.clear();
}

void *LResource::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void LResource::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void LResource::initializeRes() {
	_lockCount = 0;
	_name.clear();
	_loaded = false;
	_byte29 = 0;
	_unique = false;
	_purgeOnFree = false;
	_notPurgable = false;
}

bool LResource::loadResourceItem(const LString &itemName) {
	return loadResourceItem(itemName.c_str());
}

bool LResource::loadResourceItem(const char *itemName) {
	bool result = resLoadItem(itemName);

	if (result) {
		_loaded = true;
	} else {
		LConsoleServices *console = _engine->getConsoleServices();

		if (console)
			console->logError("LResource::LoadResourceItem - Failed to load %s\n", _name.c_str());
		else
			warning("LResource::loadResourceItem(): failed to load %s", _name.c_str());
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
	return _loaded;
}

bool LResource::resIsUnique() const {
	return _unique;
}

bool LResource::resIsPurgeOnFree() const {
	return _purgeOnFree;
}

bool LResource::resIsNotPurgable() const {
	return _notPurgable;
}

void LResource::resSetLoaded(bool value) {
	_loaded = value;
}

void LResource::resSetUnique(bool value) {
	_unique = value;
}

void LResource::resSetPurgeOnFree(bool value) {
	_purgeOnFree = value;
}

void LResource::resSetNotPurgable(bool value) {
	_notPurgable = value;
}

LString &LResource::resGetName() {
	return _name;
}

const LResourceType &LResource::resGetType() const {
	return _type;
}

uint LResource::handleCount() const {
	return _handles.size();
}

LResource *LResource::resLock() {
	_lockCount++;
	_engine->getResourceManager()->lockResource(this);

	return this;
}

void LResource::resUnlock() {
	_lockCount--;
	_engine->getResourceManager()->unlockResource(this);

	if (_lockCount == 0 && _purgeOnFree)
		_engine->getResourceManager()->purgeResource(this, true);
}

int LResource::resLockCount() const {
	return _lockCount;
}

LResourceHandleBase *LResource::firstHandle() {
	if (_handles.empty())
		return nullptr;

	return _handles.front();
}

void LResource::addHandle(LResourceHandleBase &handle) {
	Common::List<LResourceHandleBase *>::iterator it;

	for (it = _handles.begin(); it != _handles.end(); ++it) {
		if (*it == &handle)
			return;
	}

	_handles.push_front(&handle);
}

void LResource::removeHandle(LResourceHandleBase &handle) {
	Common::List<LResourceHandleBase *>::iterator it;

	for (it = _handles.begin(); it != _handles.end(); ++it) {
		if (*it == &handle) {
			_handles.erase(it);
			return;
		}
	}
}

void LResource::resSetName(const char *newName) {
	LString newNameString(newName);

	_engine->getResourceManager()->resourceNameChange(this, newNameString);

	_name = newName;
}

void LResource::resSetType(const LResourceType &newType) {
	_engine->getResourceManager()->resourceTypeChange(this, newType);

	_type = newType;
}

void LResource::resReplaceResource(LResource &other) {
	Common::List<LResourceHandleBase *>::iterator it = _handles.begin();

	while (it != _handles.end()) {
		LResourceHandleBase *handle = *it;
		++it;

		if (handle->_lockCount != 0)
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
