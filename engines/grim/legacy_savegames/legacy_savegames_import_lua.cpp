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

#include "common/debug.h"
#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/legacy_savegames/legacy_savegames_import.h"
#include "engines/grim/lua/lua.h"
#include "engines/grim/lua_v1_opcodes/lua_v1.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

bool LegacyImporter::mapUserdata(int32 oldTag, int32 oldId, int32 *newTag, int32 *newId) {
	if (oldTag == kLegacyLuaTagActor) {
		*newTag = kLuaTagActor;

		if (_ctx.actorIndices.contains(oldId))
			*newId = _ctx.actorIndices.getVal(oldId);
		else
			*newId = _ctx.getDeadId(_ctx.deadActors, oldId);

		return true;
	}

	if (oldTag == kLegacyLuaTagColor) {
		// The legacy payload is r << 16 | g << 8 | b, MakeColor() makes r | g << 8 | b << 16 | a << 24 with a = 0
		uint32 color = (uint32)oldId;

		*newTag = kLuaTagColor;
		*newId = (int32)(((color >> 16) & 0xFF) | (color & 0xFF00) | ((color & 0xFF) << 16));

		return true;
	}

	if (oldTag == kLegacyLuaTagFont) {
		*newTag = kSputTagFont;

		if (_ctx.fontSerials.contains(oldId))
			*newId = _ctx.fontSerials.getVal(oldId);
		else
			*newId = _ctx.getDeadId(_ctx.deadFonts, oldId);

		return true;
	}

	if (oldTag == kLegacyLuaTagTtfFont) {
		*newTag = kSputTagFont;
		*newId = _ctx.getDeadId(_ctx.deadTtfFonts, oldId);

		return true;
	}

	if (oldTag == kLegacyLuaTagTextObject) {
		*newTag = kLuaTagTextObject;
		*newId = _ctx.getDeadId(_ctx.deadTextObjects, oldId);

		return true;
	}

	if (oldTag == kLegacyLuaTagObjectState) {
		*newTag = kLuaTagObjectState;

		if (_ctx.objectStates.contains(oldId))
			*newId = _ctx.objectStates.getVal(oldId).luaHandle;
		else
			*newId = _ctx.getDeadId(_ctx.deadObjectStates, oldId);

		return true;
	}

	if (oldTag == kLegacyLuaTagBitmap) {
		*newTag = kLuaTagImage;
		*newId = _ctx.getDeadId(_ctx.deadBitmaps, oldId);

		return true;
	}

	// The new engine keeps the primitives as numbers, nothing knows this tag, so the userdata stays as it is
	if (oldTag == kLegacyLuaTagPrimitive || oldTag == kLegacyLuaTagNone || oldTag == kLegacyLuaTagIoFile || oldTag == kLegacyLuaTagIoClosedFile) {
		*newTag = oldTag;
		*newId = oldId;

		return true;
	}

	return false;
}

bool LegacyImporter::importLua() {
	if (!legacyRestoreLua(_luaStream, _data.minorVersion, this))
		return false;

	LegacyLuaRefs engineRefs;

	getEngineRefs(&engineRefs);

	if (!legacyLuaCheckRefs(engineRefs))
		warning("LegacyImporter::importLua(): the Lua refs of the savegame and of the engine differ");

	debug(1, "LegacyImporter::importLua(): %s", legacyLuaSelfCheck().c_str());

	return true;
}

void LegacyImporter::applyDeveloperMode() {
	WuRegistry *wuRegistry = _grim->_wuRegistry;
	uint type = wuRegistry->queryType("good_times");

	lua_beginblock();

	// The same value as developerMode = ReadRegistryValue("good_times") in the scripts
	if (type == WuRegistry::kValueBinary) {
		lua_pushnumber(wuRegistry->getFloat("good_times", 0.0f));
	} else if (type == WuRegistry::kValueString) {
		char value[256];

		value[0] = '\0';
		wuRegistry->getStr("good_times", value, sizeof(value) - 1, "");
		lua_pushstring(value);
	} else {
		lua_pushnil();
	}

	lua_setglobal("developerMode");
	lua_endblock();
}

void LegacyImporter::repeatMusicPause() {
	lua_beginblock();

	lua_Object musicState = lua_getglobal("music_state");

	if (!lua_isnil(musicState)) {
		lua_pushobject(musicState);
		lua_pushstring("paused");

		if (!lua_isnil(lua_gettable())) {
			// A movie was playing: pause() takes the sound volume of the options again
			lua_pushobject(musicState);
			lua_pushstring("paused");
			lua_pushnil();
			lua_settable();

			lua_pushobject(musicState);
			lua_pushstring("pause");

			lua_Object pauseFunc = lua_gettable();

			if (lua_isfunction(pauseFunc)) {
				lua_pushobject(musicState);
				lua_callfunction(pauseFunc);
			}
		}
	}

	lua_endblock();
}

} // End of namespace Grim
