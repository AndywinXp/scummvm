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

#include "engines/grim/emi/background.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lresourcehandle.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/grim.h"
#include "engines/grim/lua/luadebug.h"

namespace Grim {

void Lua_V2::Exit() {
	lua_Object codeObj = lua_getparam(1);

	if (lua_isnumber(codeObj))
		g_grim->requestShutdown(luaFtol(lua_getnumber(codeObj)));
	else
		g_grim->requestShutdown(0);
}

void Lua_V2::FunctionName() {
	const char *name = nullptr;
	const char *filename = nullptr;
	int32 line = 0;
	char buffer[256];
	lua_Object functionObj = lua_getparam(1);

	if (!lua_isfunction(functionObj))
		return;

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

void Lua_V2::GetControlState() {
	// TODO
	warning("Lua_V2::GetControlState(): not implemented");
}

void Lua_V2::PrintMessage() {
	lua_Object messageObj = lua_getparam(1);

	if (!lua_isnil(messageObj) && lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);

		message += "\n";

		if (gpConsoleServices)
			gpConsoleServices->logStatus("%s", message.c_str());
		else
			warning("%s", message.c_str());
	}
}

void Lua_V2::PrintError() {
	lua_Object messageObj = lua_getparam(1);

	if (lua_isnil(messageObj)) {
		if (gpConsoleServices)
			gpConsoleServices->logError("(nil)");
		else
			warning("(nil)");
	} else if (lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);

		message += "\n";

		if (message.contains('%'))
			warning("Lua_V2::PrintError(): the message contains a %% character");

		if (gpConsoleServices)
			gpConsoleServices->logError("%s", message.c_str());
		else
			warning("%s", message.c_str());
	}
}

void Lua_V2::PrintWarning() {
	lua_Object messageObj = lua_getparam(1);

	if (!lua_isnil(messageObj) && lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);

		message += "\n";

		if (gpConsoleServices)
			gpConsoleServices->logWarning("%s", message.c_str());
		else
			warning("%s", message.c_str());
	}
}

void Lua_V2::PrintDebug() {
	lua_Object messageObj = lua_getparam(1);

	if (!lua_isnil(messageObj) && lua_isstring(messageObj)) {
		LString message = lua_getstring(messageObj);

		message += "\n";

		if (gpConsoleServices)
			gpConsoleServices->logDebug("%s", message.c_str());
		else
			warning("%s", message.c_str());
	}
}

void Lua_V2::MakeCurrentSet() {
	lua_Object roomObj = lua_getparam(1);

	if (lua_isstring(roomObj)) {
		for (uint i = 0; i < _lockedSounds.size(); i++)
			Sounds::stopSound(_lockedSounds[i]);

		_lockedSounds.clear();
		Room_v2::loadRoom(lua_getstring(roomObj));
	} else {
		Room_v2::loadRoom(nullptr);
	}
}

void Lua_V2::LockSet() {
	lua_Object roomObj = lua_getparam(1);

	if (!lua_isstring(roomObj))
		return;

	LString roomName = lua_getstring(roomObj);
	LResourceHandle<Room_v2> roomHandle(roomName);

	if (roomHandle.lock() && roomHandle.typed) {
		roomHandle.typed->lockCount++;
		gpResourceManager->lockResource(roomHandle.typed);
		_lockedRooms.push_back(roomName);
	}

	roomHandle.unlock();
}

void Lua_V2::UnLockSet() {
	lua_Object roomObj = lua_getparam(1);

	if (!lua_isstring(roomObj))
		return;

	LString roomName = lua_getstring(roomObj);
	LResourceHandle<Room_v2> roomHandle(roomName);

	if (roomHandle.lock() && roomHandle.typed) {
		Room_v2 *room = roomHandle.typed;

		room->lockCount--;
		gpResourceManager->unlockResource(room);

		if (room->lockCount == 0 && room->purgeOnFree)
			gpResourceManager->purgeResource(room, true);

		for (uint i = 0; i < _lockedRooms.size(); i++) {
			if (_lockedRooms[i] == roomName) {
				_lockedRooms.remove_at(i);

				break;
			}
		}
	}

	roomHandle.unlock();
}

void Lua_V2::LockBackground() {
	lua_Object backgroundObj = lua_getparam(1);

	if (!lua_isstring(backgroundObj))
		return;

	LString backgroundName = "BG";

	backgroundName += lua_getstring(backgroundObj);

	LResourceHandle<Background> backgroundHandle(backgroundName);

	if (backgroundHandle.lock() && backgroundHandle.typed) {
		backgroundHandle.typed->lockCount++;
		gpResourceManager->lockResource(backgroundHandle.typed);
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

	LResourceHandle<Background> backgroundHandle(backgroundName);

	if (backgroundHandle.lock() && backgroundHandle.typed) {
		Background *background = backgroundHandle.typed;

		background->lockCount--;
		gpResourceManager->unlockResource(background);

		if (background->lockCount == 0 && background->purgeOnFree)
			gpResourceManager->purgeResource(background, true);

		for (uint i = 0; i < _lockedBackgrounds.size(); i++) {
			if (_lockedBackgrounds[i] == backgroundName) {
				_lockedBackgrounds.remove_at(i);

				break;
			}
		}
	}

	backgroundHandle.unlock();
}

void Lua_V2::MakeCurrentSetup() {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom)
		return;

	lua_Object setupObj = lua_getparam(1);

	if (lua_isnumber(setupObj)) {
		currentRoom->setSetup(luaFtol(lua_getnumber(setupObj)));
	} else if (lua_isstring(setupObj)) {
		// The disasm passes the address of the string as the setup index
		warning("Lua_V2::MakeCurrentSetup(): a setup name is not a valid setup index");
	}
}

void Lua_V2::GetCurrentSetup() {
	lua_Object roomObj = lua_getparam(1);

	if (!lua_isstring(roomObj))
		return;

	const char *currentRoomName = Room_v2::getCurrentRoomName();

	if (!currentRoomName)
		warning("Lua_V2::GetCurrentSetup(): there is no current room");

	if (currentRoomName && scumm_stricmp(lua_getstring(roomObj), currentRoomName) == 0) {
		Room_v2 *currentRoom = Room_v2::getCurrentRoom();

		if (currentRoom)
			lua_pushnumber((float)currentRoom->currentSetup);
		else
			lua_pushnil();

		return;
	}

	LString roomName = lua_getstring(roomObj);
	LResourceHandle<Room_v2> roomHandle(roomName);

	if (roomHandle.lock() && roomHandle.typed)
		lua_pushnumber((float)roomHandle.typed->currentSetup);

	roomHandle.unlock();
}

void Lua_V2::NextSetup() {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom)
		return;

	// The original could divide by zero here, let's avoid that...
	if (currentRoom->numSetups == 0) {
		warning("Lua_V2::NextSetup(): the room has no setups");

		return;
	}

	currentRoom->setSetup((currentRoom->currentSetup + 1) % currentRoom->numSetups);
}

void Lua_V2::PreviousSetup() {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom)
		return;

	// The original could divide by zero here, let's avoid that...
	if (currentRoom->numSetups == 0) {
		warning("Lua_V2::PreviousSetup(): the room has no setups");

		return;
	}

	currentRoom->setSetup((currentRoom->currentSetup - 1) % currentRoom->numSetups);
}

} // end of namespace Grim
