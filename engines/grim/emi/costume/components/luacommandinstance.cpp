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

#include "engines/grim/emi/costume/components/luacommandinstance.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

LuaCommandInstance::LuaCommandInstance(EmiEngine *engine, const LString &componentName, Actor *actor) : ComponentInstance(engine, componentName) {
	if (componentName.size() >= Component::kNamePrefixSize)
		command = componentName.c_str() + Component::kNamePrefixSize;
	else
		warning("LuaCommandInstance::LuaCommandInstance(): invalid component name %s", componentName.c_str());
}

bool LuaCommandInstance::isReplayable() {
	return false;
}

void LuaCommandInstance::setKey(float value, float elapsed, float nextValue, float remaining, bool partial) {
	LString commandCopy = command;

	lua_dostring(commandCopy.c_str());
}

} // End of namespace Grim
