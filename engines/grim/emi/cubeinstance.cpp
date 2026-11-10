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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cubecomponent.h"
#include "engines/grim/emi/cubeinstance.h"

namespace Grim {

CubeInstance::CubeInstance(const LString &componentName, Actor *actor) : ComponentInstance(componentName) {
	owner = actor;
	boxInstance = nullptr;

	CubeComponent *component = nullptr;

	if (handle.lock())
		component = dynamic_cast<CubeComponent *>(handle.typed);

	if (!component) {
		warning("CubeInstance::CubeInstance(): cannot get the cube component %s", componentName.c_str());
	} else if (!owner) {
		warning("CubeInstance::CubeInstance(): no owner");
	} else {
		boxInstance = static_cast<RBoxInstance *>(component->RBox::createInstance());
		owner->instances.push_back(boxInstance);
	}

	handle.unlock();

	if (boxInstance)
		boxInstance->visible = true;
}

CubeInstance::~CubeInstance() {
	if (owner && boxInstance) {
		int index = owner->findPrimitiveInstance(boxInstance);

		owner->removePrimitiveInstance(index);
		delete boxInstance;
		boxInstance = nullptr;
	}
}

void CubeInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	// The disasm treats NaN as zero
	if (boxInstance)
		boxInstance->visible = (value < 0.0f || value > 0.0f);
}

} // End of namespace Grim
