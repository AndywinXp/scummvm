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
#include "engines/grim/emi/costume/components/globalalphainstance.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/nodes/rprimitiveinstance.h"

namespace Grim {

GlobalAlphaInstance::GlobalAlphaInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	_owner = actor;

	if (componentName.size() >= Component::kNamePrefixSize)
		_filter = componentName.c_str() + Component::kNamePrefixSize;
	else
		warning("GlobalAlphaInstance::GlobalAlphaInstance(): invalid component name %s", componentName.c_str());
}

GlobalAlphaInstance::~GlobalAlphaInstance() {
	setKey(-1.0f, 0.0f, 0.0f, 0.0f, true);
}

void GlobalAlphaInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	if (!_owner)
		return;

	for (uint i = 0; i < _owner->_instances.size(); i++) {
		RPrimitiveInstance *instance = _owner->_instances[i];

		if (!instance)
			continue;

		if (!instance->_prim) {
			warning("GlobalAlphaInstance::setKey(): instance %u has no primitive", i);
			continue;
		}

		if (instance->_prim->_name != _filter)
			continue;

		switch ((int)value) {
		case -1:
			instance->_globalAlphaMode = RPrimitiveInstance::kAlphaOff;
			break;
		case 2:
			instance->_globalAlphaMode = RPrimitiveInstance::kAlphaSet;
			break;
		case 3:
			instance->_globalAlphaMode = RPrimitiveInstance::kAlphaModulate;
			break;
		default:
			if (nextValue >= 0.0f && nextValue <= 255.0f) {
				double span = (double)elapsed + (double)remaining;
				double fraction = 0.0;

				// The original could divide by zero here, let's avoid that...
				if (span != 0.0)
					fraction = (double)elapsed / span;

				instance->_globalAlpha = (float)(fraction * ((double)nextValue - (double)value) + (double)value);
			} else {
				instance->_globalAlpha = value;
			}

			break;
		}

		return;
	}

	if (_engine->isDemo())
		warning("GlobalAlphaInstance::setKey(): no primitive named %s was found on this actor", _filter.c_str());
}

} // End of namespace Grim
