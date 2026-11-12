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
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/lresourcehandle.h"

namespace Grim {

LResourceHandleBase::LResourceHandleBase() {
	_engine = nullptr;
	lockCount = 0;
	resource = nullptr;
}

LResourceHandleBase::LResourceHandleBase(const LResourceHandleBase &other) {
	_engine = other._engine;
	lockCount = 0;
	resource = other.resource;

	if (resource)
		resource->addHandle(*this);
}

LResourceHandleBase::~LResourceHandleBase() {
	if (resource)
		resource->removeHandle(*this);

	resource = nullptr;
}

void LResourceHandleBase::setResource(LResource *newResource) {
	if (resource)
		resource->removeHandle(*this);

	resource = newResource;

	if (newResource)
		newResource->addHandle(*this);
}

LResourceHandleBase &LResourceHandleBase::operator=(const LResourceHandleBase &other) {
	_engine = other._engine;
	setResource(other.resource);

	return *this;
}

void LResourceHandleBase::bindEngine(EmiEngine *engine) {
	_engine = engine;
}

LResourceManager *LResourceHandleBase::getManager() const {
	return _engine->getResourceManager();
}

bool LResourceHandleBase::lock() {
	if (!resource)
		return false;

	LResourceManager *manager = resource->getEngine()->getResourceManager();

	lockCount++;
	resource->lockCount++;
	manager->lockResource(resource);

	if (resource->loaded || resource->loadResourceItem(resource->name)) {
		resource->loaded = true;

		return true;
	}

	lockCount--;

	LResource *failedResource = resource;
	failedResource->lockCount--;
	manager->unlockResource(failedResource);

	if (failedResource->lockCount == 0 && failedResource->purgeOnFree)
		manager->purgeResource(failedResource, true);

	return false;
}

void LResourceHandleBase::unlock() {
	LResource *lockedResource = resource;

	if (lockedResource && lockedResource->loaded) {
		LResourceManager *manager = lockedResource->getEngine()->getResourceManager();

		lockCount--;
		lockedResource->lockCount--;
		manager->unlockResource(lockedResource);

		if (lockedResource->lockCount == 0 && lockedResource->purgeOnFree)
			manager->purgeResource(lockedResource, true);
	}
}

} // End of namespace Grim
