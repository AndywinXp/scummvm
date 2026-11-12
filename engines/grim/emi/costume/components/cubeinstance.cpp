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

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/costume/components/cubecomponent.h"
#include "engines/grim/emi/costume/components/cubeinstance.h"

namespace Grim {

CubeInstance::CubeInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	_owner = actor;
	_boxInstance = nullptr;

	CubeComponent *component = nullptr;

	if (_handle.lock())
		component = dynamic_cast<CubeComponent *>(_handle._typed);

	if (!component) {
		warning("CubeInstance::CubeInstance(): cannot get the cube component %s", componentName.c_str());
	} else if (!_owner) {
		warning("CubeInstance::CubeInstance(): no owner");
	} else {
		_boxInstance = static_cast<RBoxInstance *>(component->RBox::createInstance());
		_owner->_instances.push_back(_boxInstance);
	}

	_handle.unlock();

	if (_boxInstance)
		_boxInstance->_visible = true;
}

CubeInstance::~CubeInstance() {
	if (_owner && _boxInstance) {
		int index = _owner->findPrimitiveInstance(_boxInstance);

		_owner->removePrimitiveInstance(index);
		delete _boxInstance;
		_boxInstance = nullptr;
	}
}

void CubeInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	if (_boxInstance)
		_boxInstance->_visible = (value < 0.0f || value > 0.0f);
}

} // End of namespace Grim
