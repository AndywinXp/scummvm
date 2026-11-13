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

#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/graphics/background.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/resources/lresourcehandle.h"
#include "engines/grim/emi/lua_v2_opcodes/lua_v2.h"
#include "engines/grim/emi/scene/room_v2.h"
#include "engines/grim/emi/sound/sounds.h"
#include "engines/grim/lua/luadebug.h"

namespace Grim {

void Lua_V2::Exit() {
	lua_Object codeObj = lua_getparam(1);
	int code = 0;

	if (lua_isnumber(codeObj))
		code = luaFtol(lua_getnumber(codeObj));

	if (_engine->isDemo())
		logConsole(kConsoleDebug, "The scripts called exit with code %d. Shutting down.\n", code);

	_engine->requestShutdown(code);
}

void Lua_V2::FunctionName() {
	const char *name = nullptr;
	const char *filename = nullptr;
	int32 line = 0;
	char buffer[256];
	lua_Object functionObj = lua_getparam(1);

	if (!lua_isfunction(functionObj)) {
		if (_engine->isDemo())
			logConsole(kConsoleStatus, "FunctionName: the argument must be a function.\n");

		return;
	}

	lua_funcinfo(functionObj, &filename, &line);

	char functionType = *lua_getobjname(functionObj, &name);
	const char *typeName = "unnamed function";
	const char *functionName = "<unknown>";

	if (functionType == 'g') {
		typeName = "function";
		functionName = name;
	} else if (functionType == 't') {
		typeName = "tag method";
		functionName = name;
	}

	Common::sprintf_s(buffer, "%s '%s' (%s:%d)", typeName, functionName, filename, (int)line);
	lua_pushstring(buffer);
}

void Lua_V2::PrintMessage() {
	lua_Object messageObj = lua_getparam(1);

	if (!lua_isnil(messageObj) && lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);
		LConsoleServices *console = _engine->getConsoleServices();

		message += "\n";

		if (console)
			console->logStatus("%s", message.c_str());
		else
			warning("%s", message.c_str());
	} else if (_engine->isDemo()) {
		if (!lua_isnil(messageObj))
			logConsole(kConsoleDebug, "PrintMessage: takes only a single string argument.\n");
		else if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logStatus("(nil)");
	}
}

void Lua_V2::PrintError() {
	lua_Object messageObj = lua_getparam(1);
	LConsoleServices *console = _engine->getConsoleServices();

	if (lua_isnil(messageObj)) {
		if (console)
			console->logError("(nil)");
		else
			warning("(nil)");
	} else if (lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);

		message += "\n";

		if (message.contains('%'))
			warning("Lua_V2::PrintError(): the message contains a %% character");

		if (console)
			console->logError("%s", message.c_str());
		else
			warning("%s", message.c_str());
	} else if (_engine->isDemo()) {
		logConsole(kConsoleDebug, "PrintError: takes only a single string argument.\n");
	}
}

void Lua_V2::PrintWarning() {
	lua_Object messageObj = lua_getparam(1);

	if (!lua_isnil(messageObj) && lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);
		LConsoleServices *console = _engine->getConsoleServices();

		message += "\n";

		if (console)
			console->logWarning("%s", message.c_str());
		else
			warning("%s", message.c_str());
	} else if (_engine->isDemo()) {
		if (!lua_isnil(messageObj))
			logConsole(kConsoleDebug, "PrintWarning: takes only a single string argument.\n");
		else if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logWarning("(nil)");
	}
}

void Lua_V2::PrintDebug() {
	lua_Object messageObj = lua_getparam(1);

	if (!lua_isnil(messageObj) && lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);
		LConsoleServices *console = _engine->getConsoleServices();

		message += "\n";

		if (console)
			console->logDebug("%s", message.c_str());
		else
			warning("%s", message.c_str());
	} else if (_engine->isDemo()) {
		if (!lua_isnil(messageObj))
			logConsole(kConsoleDebug, "PrintDebug: takes only a single string argument.\n");
		else if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logDebug("(nil)");
	}
}

void Lua_V2::MakeCurrentSet() {
	lua_Object roomObj = lua_getparam(1);

	if (lua_isstring(roomObj)) {
		// The demo logs the set name and leaves the loaded sounds alone
		if (_engine->isDemo()) {
			logConsole(kConsoleStatus, "Making the current set %s.\n", lua_getstring(roomObj));
		} else {
			for (uint i = 0; i < _lockedSounds.size(); i++)
				_engine->getSounds()->stopSound(_lockedSounds[i]);

			_lockedSounds.clear();
		}

		Room_v2::loadRoom(_engine, lua_getstring(roomObj));
	} else {
		if (_engine->isDemo())
			logConsole(kConsoleWarning, "MakeCurrentSet: the argument must be a set name.\n");

		Room_v2::loadRoom(_engine, nullptr);
	}
}

void Lua_V2::LockSet() {
	lua_Object roomObj = lua_getparam(1);

	if (!lua_isstring(roomObj))
		return;

	LString roomName = lua_getstring(roomObj);
	LResourceHandle<Room_v2> roomHandle(_engine, roomName);

	if (roomHandle.lock() && roomHandle._typed) {
		roomHandle._typed->_lockCount++;
		_engine->getResourceManager()->lockResource(roomHandle._typed);

		// The demo keeps no list of locked names
		if (!_engine->isDemo())
			_lockedRooms.push_back(roomName);
	}

	roomHandle.unlock();
}

void Lua_V2::UnLockSet() {
	lua_Object roomObj = lua_getparam(1);

	if (!lua_isstring(roomObj))
		return;

	LString roomName = lua_getstring(roomObj);
	LResourceHandle<Room_v2> roomHandle(_engine, roomName);

	if (roomHandle.lock() && roomHandle._typed) {
		Room_v2 *room = roomHandle._typed;

		room->_lockCount--;
		_engine->getResourceManager()->unlockResource(room);

		if (room->_lockCount == 0 && room->_purgeOnFree)
			_engine->getResourceManager()->purgeResource(room, true);

		for (uint i = 0; !_engine->isDemo() && i < _lockedRooms.size(); i++) {
			if (_lockedRooms[i] == roomName) {
				_lockedRooms.remove_at(i);

				break;
			}
		}
	} else if (_engine->isDemo()) {
		logConsole(kConsoleWarning, "UnLockSet: the set %s does not exist.\n", roomName.c_str());
	}

	roomHandle.unlock();
}

void Lua_V2::LockBackground() {
	lua_Object backgroundObj = lua_getparam(1);

	if (!lua_isstring(backgroundObj))
		return;

	LString backgroundName = "BG";

	backgroundName += lua_getstring(backgroundObj);

	LResourceHandle<Background> backgroundHandle(_engine, backgroundName);

	if (backgroundHandle.lock() && backgroundHandle._typed) {
		backgroundHandle._typed->_lockCount++;
		_engine->getResourceManager()->lockResource(backgroundHandle._typed);

		if (!_engine->isDemo())
			_lockedBackgrounds.push_back(backgroundName);
	}

	backgroundHandle.unlock();
}

void Lua_V2::UnLockBackground() {
	lua_Object backgroundObj = lua_getparam(1);

	if (!lua_isstring(backgroundObj))
		return;

	LString backgroundName = "BG";

	backgroundName += lua_getstring(backgroundObj);

	LResourceHandle<Background> backgroundHandle(_engine, backgroundName);

	if (backgroundHandle.lock() && backgroundHandle._typed) {
		Background *background = backgroundHandle._typed;

		background->_lockCount--;
		_engine->getResourceManager()->unlockResource(background);

		if (background->_lockCount == 0 && background->_purgeOnFree)
			_engine->getResourceManager()->purgeResource(background, true);

		for (uint i = 0; !_engine->isDemo() && i < _lockedBackgrounds.size(); i++) {
			if (_lockedBackgrounds[i] == backgroundName) {
				_lockedBackgrounds.remove_at(i);

				break;
			}
		}
	} else if (_engine->isDemo()) {
		logConsole(kConsoleWarning, "UnLockBackground: the background %s does not exist.\n", backgroundName.c_str());
	}

	backgroundHandle.unlock();
}

void Lua_V2::MakeCurrentSetup() {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom(_engine);

	if (!currentRoom) {
		if (_engine->isDemo())
			logConsole(kConsoleStatus, "MakeCurrentSetup: there is no current set.\n");

		return;
	}

	lua_Object setupObj = lua_getparam(1);

	if (lua_isnumber(setupObj)) {
		currentRoom->setSetup(luaFtol(lua_getnumber(setupObj)));
	} else if (_engine->isDemo()) {
		logConsole(kConsoleStatus, "MakeCurrentSetup: the setup parameter is not valid.\n");
	} else if (lua_isstring(setupObj)) {
		// The disasm passes the address of the string as the setup index
		warning("Lua_V2::MakeCurrentSetup(): a setup name is not a valid setup index");
	}
}

void Lua_V2::GetCurrentSetup() {
	lua_Object roomObj = lua_getparam(1);

	if (!lua_isstring(roomObj)) {
		if (_engine->isDemo())
			logConsole(kConsoleDebug, "GetCurrentSetup: the argument must be a set name.\n");

		return;
	}

	const char *currentRoomName = Room_v2::getCurrentRoomName(_engine);

	if (!currentRoomName)
		warning("Lua_V2::GetCurrentSetup(): there is no current room");

	if (currentRoomName && scumm_stricmp(lua_getstring(roomObj), currentRoomName) == 0) {
		Room_v2 *currentRoom = Room_v2::getCurrentRoom(_engine);

		if (currentRoom)
			lua_pushnumber((float)currentRoom->_currentSetup);
		else
			lua_pushnil();

		return;
	}

	LString roomName = lua_getstring(roomObj);
	LResourceHandle<Room_v2> roomHandle(_engine, roomName);

	if (roomHandle.lock() && roomHandle._typed)
		lua_pushnumber((float)roomHandle._typed->_currentSetup);

	roomHandle.unlock();
}

void Lua_V2::NextSetup() {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom(_engine);

	if (!currentRoom) {
		if (_engine->isDemo())
			logConsole(kConsoleStatus, "NextSetup: there is no current set.\n");

		return;
	}

	// The original could divide by zero here, let's avoid that...
	if (currentRoom->_numSetups == 0) {
		warning("Lua_V2::NextSetup(): the room has no setups");

		return;
	}

	currentRoom->setSetup((currentRoom->_currentSetup + 1) % currentRoom->_numSetups);
}

void Lua_V2::PreviousSetup() {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom(_engine);

	if (!currentRoom) {
		if (_engine->isDemo())
			logConsole(kConsoleStatus, "PreviousSetup: there is no current set.\n");

		return;
	}

	// The original could divide by zero here, let's avoid that...
	if (currentRoom->_numSetups == 0) {
		warning("Lua_V2::PreviousSetup(): the room has no setups");

		return;
	}

	currentRoom->setSetup((currentRoom->_currentSetup - 1) % currentRoom->_numSetups);
}

} // end of namespace Grim
