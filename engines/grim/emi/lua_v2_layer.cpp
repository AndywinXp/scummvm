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

#include "common/endian.h"
#include "common/textconsole.h"

#include "engines/grim/emi/layer.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/setup.h"

namespace Grim {

void Lua_V2::NewLayer() {
	lua_Object nameObj = lua_getparam(1);
	lua_Object sortOrderObj = lua_getparam(2);
	// The disasm reads the first parameter again where the frame should be
	lua_Object frameObj = lua_getparam(1);
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom) {
		lua_pushnil();

		return;
	}

	if (!currentRoom->setups || currentRoom->currentSetup < 0 || currentRoom->currentSetup >= currentRoom->numSetups || !lua_isstring(nameObj)) {
		warning("Lua_V2::NewLayer(): invalid setup or layer name");
		lua_pushnil();

		return;
	}

	Setup *setup = &currentRoom->setups[currentRoom->currentSetup];
	int frame = luaFtol(lua_getnumber(frameObj));
	int sortOrder = luaFtol(lua_getnumber(sortOrderObj));
	const char *layerName = lua_getstring(nameObj);
	int layerId = setup->addLayer(layerName, sortOrder, frame, -1);

	lua_pushusertag(layerId, MKTAG('R', 'Y', 'A', 'L'));
}

void Lua_V2::FreeLayer() {
	lua_Object layerObj = lua_getparam(1);

	if (lua_isuserdata(layerObj) && lua_tag(layerObj) == MKTAG('R', 'Y', 'A', 'L')) {
		int layerId = lua_getuserdata(layerObj);
		Room_v2 *currentRoom = Room_v2::getCurrentRoom();

		if (currentRoom) {
			if (!currentRoom->setups || currentRoom->currentSetup < 0 || currentRoom->currentSetup >= currentRoom->numSetups) {
				warning("Lua_V2::FreeLayer(): invalid setup");

				return;
			}

			currentRoom->setups[currentRoom->currentSetup].removeLayerById(layerId);
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("FreeLayer first parm must be a layer");
		else
			warning("FreeLayer first parm must be a layer");
	}
}

void Lua_V2::SetLayerSortOrder() {
	lua_Object layerObj = lua_getparam(1);

	if (lua_isuserdata(layerObj) && lua_tag(layerObj) == MKTAG('R', 'Y', 'A', 'L')) {
		Layer *layer = Room_v2::findLayerById(lua_getuserdata(layerObj));

		if (layer) {
			lua_Object sortOrderObj = lua_getparam(2);

			layer->sortOrder = luaFtol(lua_getnumber(sortOrderObj));
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("SetLayerSortOrder layer not found.\n");
			else
				warning("SetLayerSortOrder layer not found.");
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetLayerSortOrder first parm must be a layer");
		else
			warning("SetLayerSortOrder first parm must be a layer");
	}
}

void Lua_V2::GetLayerSortOrder() {
	lua_Object layerObj = lua_getparam(1);

	if (lua_isuserdata(layerObj) && lua_tag(layerObj) == MKTAG('R', 'Y', 'A', 'L')) {
		Layer *layer = Room_v2::findLayerById(lua_getuserdata(layerObj));

		if (layer) {
			lua_pushnumber((float)layer->sortOrder);
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("GetLayerSortOrder: layer not found.\n");
			else
				warning("GetLayerSortOrder: layer not found.");
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetLayerSortOrder first parm must be a layer");
		else
			warning("GetLayerSortOrder first parm must be a layer");
	}
}

void Lua_V2::GetLayerFrame() {
	lua_Object layerObj = lua_getparam(1);

	if (lua_isuserdata(layerObj) && lua_tag(layerObj) == MKTAG('R', 'Y', 'A', 'L')) {
		Layer *layer = Room_v2::findLayerById(lua_getuserdata(layerObj));

		if (layer) {
			lua_pushnumber((float)layer->frame);
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("GetLayerCurFrame: layer not found.\n");
			else
				warning("GetLayerCurFrame: layer not found.");
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetLayerCurFrame first parm must be a layer");
		else
			warning("GetLayerCurFrame first parm must be a layer");
	}
}

void Lua_V2::SetLayerFrame() {
	lua_Object layerObj = lua_getparam(1);

	if (lua_isuserdata(layerObj) && lua_tag(layerObj) == MKTAG('R', 'Y', 'A', 'L')) {
		Layer *layer = Room_v2::findLayerById(lua_getuserdata(layerObj));

		if (layer) {
			lua_Object frameObj = lua_getparam(2);

			layer->frame = luaFtol(lua_getnumber(frameObj));
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("SetLayerCurFrame: layer not found.\n");
			else
				warning("SetLayerCurFrame: layer not found.");
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetLayerCurFrame first parm must be a layer");
		else
			warning("SetLayerCurFrame first parm must be a layer");
	}
}

void Lua_V2::AdvanceLayerFrame() {
	lua_Object layerObj = lua_getparam(1);

	if (lua_isuserdata(layerObj) && lua_tag(layerObj) == MKTAG('R', 'Y', 'A', 'L')) {
		Layer *layer = Room_v2::findLayerById(lua_getuserdata(layerObj));

		if (layer) {
			lua_Object frameObj = lua_getparam(2);

			layer->frame += luaFtol(lua_getnumber(frameObj));
		} else {
			if (gpConsoleServices)
				gpConsoleServices->logError("AdvanceLayerCurFrame: layer not found.\n");
			else
				warning("AdvanceLayerCurFrame: layer not found.");
		}
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("AdvanceLayerCurFrame: first parm must be a layer");
		else
			warning("AdvanceLayerCurFrame: first parm must be a layer");
	}
}

} // end of namespace Grim
