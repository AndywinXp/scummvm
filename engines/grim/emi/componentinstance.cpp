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

#include "engines/grim/emi/componentinstance.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

ComponentInstance::ComponentInstance(const LString &componentName) {
	{
		LResourceHandle<Component> componentHandle(componentName);

		handle.setResource(componentHandle.resource);
	}

	handle.lock();
}

ComponentInstance::~ComponentInstance() {
	handle.unlock();
}

void *ComponentInstance::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void ComponentInstance::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

void ComponentInstance::update(float deltaTime) {
}

bool ComponentInstance::isReplayable() {
	return true;
}

void ComponentInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
}

bool ComponentInstance::setParent(ComponentInstance *parent) {
	return true;
}

void ComponentInstance::fadeIn(float time) {
}

void ComponentInstance::fadeOut(float time) {
}

void ComponentInstance::reset() {
}

} // End of namespace Grim
