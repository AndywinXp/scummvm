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

#ifndef GRIM_LEGACY_SAVEGAMES_LUA_H
#define GRIM_LEGACY_SAVEGAMES_LUA_H

#include "common/endian.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "common/stream.h"

#include "engines/grim/lua/lua.h"

namespace Grim {

// Userdata tags of the legacy engine (the tag of the pool object)
static const int32 kLegacyLuaTagActor = MKTAG('A', 'C', 'T', 'R');
static const int32 kLegacyLuaTagColor = MKTAG('C', 'O', 'L', 'R');
static const int32 kLegacyLuaTagFont = MKTAG('F', 'O', 'N', 'T');
static const int32 kLegacyLuaTagTtfFont = MKTAG('T', 'T', 'F', ' ');
static const int32 kLegacyLuaTagTextObject = MKTAG('T', 'E', 'X', 'T');
static const int32 kLegacyLuaTagObjectState = MKTAG('S', 'T', 'A', 'T');
static const int32 kLegacyLuaTagPrimitive = MKTAG('P', 'R', 'I', 'M');
static const int32 kLegacyLuaTagBitmap = MKTAG('V', 'B', 'U', 'F');
// lua_pushuserdata(): the "success" value of dofile, remove and the io functions
static const int32 kLegacyLuaTagNone = 0;
// The two tags made by the io library (lua_newtag() right after lua_open())
static const int32 kLegacyLuaTagIoFile = -8;
static const int32 kLegacyLuaTagIoClosedFile = -9;

// Userdata tags of the new engine; the font one is kSputTagFont
static const int32 kLuaTagActor = MKTAG('R', 'T', 'C', 'A');
static const int32 kLuaTagColor = MKTAG('R', 'L', 'O', 'C');
static const int32 kLuaTagFont = MKTAG('T', 'N', 'O', 'F');
static const int32 kLuaTagTextObject = MKTAG('T', 'X', 'E', 'T');
static const int32 kLuaTagObjectState = MKTAG('T', 'A', 'T', 'S');
static const int32 kLuaTagCacheRect = MKTAG('P', 'O', 'I', 'N');
static const int32 kLuaTagImage = MKTAG('F', 'U', 'B', 'V');

// Number of lists of C functions of the legacy engine (depth 0 = registered last)
static const int kNumLegacyLuaLists = 13;

// The textobject field refs, in the order of the creation
static const int kNumLegacyLuaTextRefs = 16;

// The Lua refs the C++ side holds, they are not part of the savegame
struct LegacyLuaRefs {
	int systemTable;
	int typeOverride;
	int oldConcatFallback;
	int textObject[kNumLegacyLuaTextRefs]; // x, y, font, width, height, fgcolor, bgcolor, fxcolor, hicolor, duration, center, ljustify, rjustify, volume, pan, background
};

// Decides the new tag and id of every userdata of the legacy Lua state
class LegacyLuaMapper {
public:
	virtual ~LegacyLuaMapper() {}

	// false = unknown, the userdata is kept as it is
	virtual bool mapUserdata(int32 oldTag, int32 oldId, int32 *newTag, int32 *newId) = 0;
};

// Old list depth and index in, function out (nullptr = missing, the hook reports it)
typedef lua_CFunction (*lua_LegacyFunctionHook)(int32 depth, int32 index);
// Old tag and id in, new tag and id out (false = leave the userdata as it is)
typedef bool (*lua_LegacyUserdataHook)(int32 oldTag, int32 oldId, int32 *newTag, int32 *newId);

// Reads the Lua state of a legacy savegame (the whole 'LUAS' payload), 0 = success
int lua_RestoreLegacy(Common::SeekableReadStream *stream, int minorVersion, lua_LegacyFunctionHook functionHook, lua_LegacyUserdataHook userdataHook);

// Patch the Lua state after loading a broken Grim save (defined in lua_grim_patch.cpp)
void lua_PatchGrimSave();

// Fix the na.lua code in saves made with the old patch for bug #312 (Rubacava sign)
void lua_PatchGrimNaSave();

// Name of an entry of the legacy lists, nullptr if depth or index do not exist
const char *legacyLuaFunctionName(int depth, int index);

// Finds the entry by name in the lists of the running Lua, nullptr (with a warning) if it is not there
lua_CFunction legacyLuaResolveFunction(int32 depth, int32 index);

// Replaces the Lua state with the one of a legacy savegame; the stream is the payload of 'LUAS'.
// On failure the Lua state is damaged and the load has to be aborted.
bool legacyRestoreLua(Common::SeekableReadStream *stream, int minorVersion, LegacyLuaMapper *mapper);

// The ref numbers the legacy engine used
void legacyLuaGetRefs(LegacyLuaRefs *refs);

// Checks the restored refs and that the engine uses the same numbers
bool legacyLuaCheckRefs(const LegacyLuaRefs &engineRefs);

// After a restore: compares the name of every global holding a C function with the entry found in the running lists
Common::String legacyLuaSelfCheck();

} // End of namespace Grim

#endif
