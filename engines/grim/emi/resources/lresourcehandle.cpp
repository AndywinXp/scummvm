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

#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/resources/lresource.h"
#include "engines/grim/emi/resources/lresourcehandle.h"

namespace Grim {

LResourceHandleBase::LResourceHandleBase() {
	_engine = nullptr;
	_lockCount = 0;
	_resource = nullptr;
}

LResourceHandleBase::LResourceHandleBase(const LResourceHandleBase &other) {
	_engine = other._engine;
	_lockCount = 0;
	_resource = other._resource;

	if (_resource)
		_resource->addHandle(*this);
}

LResourceHandleBase::~LResourceHandleBase() {
	if (_resource)
		_resource->removeHandle(*this);

	_resource = nullptr;
}

void LResourceHandleBase::setResource(LResource *newResource) {
	if (_resource)
		_resource->removeHandle(*this);

	_resource = newResource;

	if (newResource)
		newResource->addHandle(*this);
}

LResourceHandleBase &LResourceHandleBase::operator=(const LResourceHandleBase &other) {
	_engine = other._engine;
	setResource(other._resource);

	return *this;
}

void LResourceHandleBase::bindEngine(EmiEngine *engine) {
	_engine = engine;
}

LResourceManager *LResourceHandleBase::getManager() const {
	return _engine->getResourceManager();
}

bool LResourceHandleBase::lock() {
	if (!_resource)
		return false;

	LResourceManager *manager = _resource->getEngine()->getResourceManager();

	_lockCount++;
	_resource->_lockCount++;
	manager->lockResource(_resource);

	if (_resource->_loaded || _resource->loadResourceItem(_resource->_name)) {
		_resource->_loaded = true;

		return true;
	}

	_lockCount--;

	LResource *failedResource = _resource;
	failedResource->_lockCount--;
	manager->unlockResource(failedResource);

	if (failedResource->_lockCount == 0 && failedResource->_purgeOnFree)
		manager->purgeResource(failedResource, true);

	return false;
}

void LResourceHandleBase::unlock() {
	LResource *lockedResource = _resource;

	if (lockedResource && lockedResource->_loaded) {
		LResourceManager *manager = lockedResource->getEngine()->getResourceManager();

		_lockCount--;
		lockedResource->_lockCount--;
		manager->unlockResource(lockedResource);

		if (lockedResource->_lockCount == 0 && lockedResource->_purgeOnFree)
			manager->purgeResource(lockedResource, true);
	}
}

} // End of namespace Grim
