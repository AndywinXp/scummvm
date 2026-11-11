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

#include "engines/grim/emi/inputrecorder.h"
#include "engines/grim/emi/joystick.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/sputcontrol_v2.h"

namespace Grim {

void Lua_V2::makeControlsTable(int ref) {
	lua_Object controlsTable = lua_createtable();

	lua_pushobject(lua_getref(ref));
	lua_pushstring("controls");
	lua_pushobject(controlsTable);
	lua_settable();

	for (int i = 0; i < SputControl_v2::kNumControls; i++) {
		lua_pushobject(controlsTable);
		lua_pushstring(SputControl_v2::controlTable[i].name);
		lua_pushnumber((float)i);
		lua_settable();
	}

	lua_pushobject(controlsTable);
	lua_pushstring("joy_selected");
	lua_pushnumber((float)(int)Joystick::joyId);
	lua_settable();
}

void Lua_V2::EnableControl() {
	lua_Object controlObj = lua_getparam(1);

	if (!lua_isnumber(controlObj)) {
		lua_pushnil();

		return;
	}

	SputControl_v2::enableControl((uint)luaFtol(lua_getnumber(controlObj)));
}

void Lua_V2::DisableControl() {
	lua_Object controlObj = lua_getparam(1);

	if (!lua_isnumber(controlObj)) {
		lua_pushnil();

		return;
	}

	SputControl_v2::disableControl((uint)luaFtol(lua_getnumber(controlObj)));
}

void Lua_V2::GetControlState() {
	lua_Object controlObj = lua_getparam(1);

	if (!lua_isnumber(controlObj))
		return;

	int index = luaFtol(lua_getnumber(controlObj));

	if ((uint)index >= (uint)SputControl_v2::kNumControls) {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetControlState: Invalid control %d.\n", index);
		else
			warning("GetControlState: Invalid control %d.", index);

		return;
	}

	ControlEntry *control = &SputControl_v2::controlTable[index];

	if (control->flags & ControlEntry::kAxis)
		lua_pushnumber(control->axisValue);
	else if (control->state)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

bool Lua_V2::buttonHandler(int vk, bool down) {
	Lua_V2 *lua = dynamic_cast<Lua_V2 *>(LuaBase::instance());

	if (!lua) {
		warning("Lua_V2::buttonHandler(): the system table is not ready");

		return false;
	}

	bool pushSelf = false;
	bool callHandler;
	bool handled = false;
	lua_Object function;

	lua_beginblock();
	lua_pushobject(lua_getref(lua->refSystemTable));
	lua_pushstring("buttonHandler");

	lua_Object handler = lua_gettable();

	if (lua_istable(handler)) {
		lua_pushobject(handler);
		lua_pushstring("buttonHandler");

		function = lua_gettable();

		if (lua_isfunction(function)) {
			pushSelf = true;
			callHandler = true;
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("Installed button handler table has no member func '%s'!\n", "buttonHandler");
			else
				warning("Installed button handler table has no member func '%s'!", "buttonHandler");

			callHandler = down;
		}
	} else if (lua_isfunction(handler)) {
		function = handler;
		callHandler = true;
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("Installed button handler is invalid!\n");
		else
			warning("Installed button handler is invalid!");

		function = LUA_NOOBJECT;
		callHandler = false;
	}

	uint index = 0;

	while (index < (uint)SputControl_v2::kNumControls && vk != (int)SputControl_v2::controlTable[index].code)
		index++;

	if (index < (uint)SputControl_v2::kNumControls) {
		ControlEntry *control = &SputControl_v2::controlTable[index];

		if (SputControl_v2::isModifierKey(vk) && control->state == (byte)down) {
			index = SputControl_v2::kNumControls;
		} else {
			control->state = down;

			if (InputRecorder::instance)
				InputRecorder::instance->recordKey(control, down);
			else
				warning("Lua_V2::buttonHandler(): there is no input recorder");
		}
	}

	if (callHandler && index < (uint)SputControl_v2::kNumControls) {
		if (pushSelf)
			lua_pushobject(handler);

		lua_pushnumber((float)index);

		if (down)
			lua_pushnumber(1.0f);
		else
			lua_pushnil();

		lua_callfunction(function);

		if (!SputControl_v2::isModifierKey(vk))
			handled = true;
	}

	lua_endblock();

	return handled;
}

void Lua_V2::axisHandler() {
	Lua_V2 *lua = dynamic_cast<Lua_V2 *>(LuaBase::instance());

	if (!lua) {
		warning("Lua_V2::axisHandler(): the system table is not ready");

		return;
	}

	bool pushSelf = false;
	bool callHandler = false;
	bool buttonHandled = false;
	lua_Object function = LUA_NOOBJECT;

	lua_beginblock();
	lua_pushobject(lua_getref(lua->refSystemTable));
	lua_pushstring("axisHandler");

	lua_Object handler = lua_gettable();

	if (lua_istable(handler)) {
		lua_pushobject(handler);
		lua_pushstring("axisHandler");

		function = lua_gettable();

		if (lua_isfunction(function)) {
			pushSelf = true;
			callHandler = true;
		} else {
			// The disasm leaves the flag uninitialized here
			if (gpConsoleServices)
				gpConsoleServices->logError("Installed axis handler table has no member func '%s'!\n", "axisHandler");
			else
				warning("Installed axis handler table has no member func '%s'!", "axisHandler");
		}
	} else if (lua_isfunction(handler)) {
		function = handler;
		callHandler = true;
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("Installed axis handler is invalid!\n");
		else
			warning("Installed axis handler is invalid!");
	}

	for (int i = 0; i < SputControl_v2::kNumControls; i++) {
		ControlEntry *control = &SputControl_v2::controlTable[i];

		if (!(control->flags & ControlEntry::kEnabled))
			continue;

		if (control->flags & ControlEntry::kAxis) {
			float value = Joystick::getAxis((int)control->code);

			if (value != control->axisValue && callHandler) {
				if (pushSelf)
					lua_pushobject(handler);

				lua_pushnumber((float)i);
				lua_pushnumber(value);
				lua_callfunction(function);

				control->axisValue = value;
			}
		} else if (control->flags & ControlEntry::kJoystickButton) {
			bool state = Joystick::getControlState((int)control->code);

			if ((byte)state != control->state && !buttonHandled)
				buttonHandled = buttonHandler((int)control->code, state);
		}
	}

	lua_endblock();
}

void Lua_V2::updateJoySelected() {
	lua_pushobject(lua_getglobal("system"));
	lua_pushstring("controls");

	lua_Object controlsTable = lua_gettable();

	lua_pushobject(controlsTable);
	lua_pushstring("joy_selected");
	lua_pushnumber((float)(int)Joystick::joyId);
	lua_settable();
}

} // End of namespace Grim
