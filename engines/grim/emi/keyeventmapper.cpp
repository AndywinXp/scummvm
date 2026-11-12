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
#include "engines/grim/emi/keyeventmapper.h"
#include "engines/grim/emi/lua_v2.h"

namespace Grim {

KeyEventMapper::KeyEventMapper(EmiEngine *engine) : EventHandler(engine, 500) {
	typeMask = 3;
}

KeyEventMapper::~KeyEventMapper() {
}

void KeyEventMapper::bindKey(int vk, const KeyBinding *binding) {
	BindingEntry entry;

	entry.vk = vk;
	entry.binding = binding;
	_bindings.push_back(entry);
}

bool KeyEventMapper::handleEvent(const Event *event) {
	int vk = event->vk;

	if (event->type == Event::kKeyDown) {
		for (uint i = 0; i < _bindings.size(); i++) {
			if (_bindings[i].vk != vk || !(_bindings[i].binding->flags & KeyBinding::kOnKeyDown))
				continue;

			if ((_bindings[i].binding->flags & KeyBinding::kIgnoreRepeat) && (event->modifiers & Event::kModifierRepeat))
				return false;

			return _engine->getLua()->buttonHandler(vk, true);
		}

		return false;
	}

	for (uint i = 0; i < _bindings.size(); i++) {
		if (_bindings[i].vk != vk || !(_bindings[i].binding->flags & KeyBinding::kOnKeyUp))
			continue;

		return _engine->getLua()->buttonHandler(vk, false);
	}

	return false;
}

} // End of namespace Grim
