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
#include "common/savefile.h"
#include "common/config-manager.h"

#include "engines/grim/remastered/lua_remastered.h"
#include "engines/grim/remastered/overlay.h"
#include "engines/grim/remastered/hotspot.h"
#include "engines/grim/grim.h"
// LEGACY: #include "engines/grim/font.h"
// LEGACY: #include "engines/grim/resource.h"
// LEGACY: #include "engines/grim/registry.h"
// LEGACY: #include "engines/grim/localize.h"
#include "engines/grim/lua/lauxlib.h"
#include "engines/grim/lua/luadebug.h"

namespace Grim {

void Lua_Remastered::WidescreenCorrectionFactor() {
	warning("Stub function: WidescreenCorrectionFactor, returns 1");
	lua_pushnumber(1);
}

void Lua_Remastered::GetFontDimensions() {
	// LEGACY: // Taken from Lua_v2 and modified
	// LEGACY: lua_Object fontObj = lua_getparam(1);
	// LEGACY: if (!lua_isuserdata(fontObj))
	// LEGACY: 	return;
	// LEGACY:
	// LEGACY: Font *font = getfont(fontObj);
	// LEGACY:
	// LEGACY: if (font) {
	// LEGACY: 	int32 h = font->getKernedHeight();
	// LEGACY: 	int32 w = font->getFontWidth();
	// LEGACY: 	lua_pushnumber(w);
	// LEGACY: 	lua_pushnumber(h);
	// LEGACY: } else {
	// LEGACY: 	warning("Lua_Remastered::GetFontDimensions for invalid font: returns 0,0");
	// LEGACY: 	lua_pushnumber(0.f);
	// LEGACY: 	lua_pushnumber(0.f);
	// LEGACY: }
	Lua_V1::GetFontDimensions();
}

void Lua_Remastered::GetTextObjectDimensions() {
	// LEGACY: lua_Object textObj = lua_getparam(1);
	// LEGACY: lua_Object coordsObj = lua_getparam(2);
	// LEGACY: if (lua_isnumber(coordsObj)) {
	// LEGACY: 	int val = lua_getnumber(coordsObj);
	// LEGACY: 	warning("Stub function: GetTextObjectDimensions(%d)", val);
	// LEGACY: }
	// LEGACY:
	// LEGACY: if (lua_isuserdata(textObj) && lua_tag(textObj) == MKTAG('T', 'X', 'E', 'T')) {
	// LEGACY: 	TextObject *textObject = gettextobject(textObj);
	// LEGACY: 	lua_pushnumber(textObject->getWidth()); // REMASTERED HACK
	// LEGACY: 	lua_pushnumber(textObject->getBitmapHeight()); // REMASTERED HACK
	// LEGACY: 	// If the line is rjustified getX does us no good
	// LEGACY: 	lua_pushnumber(textObject->getLineX(0));
	// LEGACY: 	lua_pushnumber(textObject->getY());
	// LEGACY: }
	Lua_V1::GetTextObjectDimensions();
}

void Lua_Remastered::OverlayDimensions() {
	// LEGACY: lua_Object overlayObj = lua_getparam(1);
	// LEGACY: if (!lua_isuserdata(overlayObj) || lua_tag(overlayObj) != Overlay::getStaticTag())
	// LEGACY: 	return;
	// LEGACY:
	// LEGACY: Overlay *overlay = Overlay::getPool().getObject(lua_getuserdata(overlayObj));
	// LEGACY: lua_pushnumber(overlay->getWidth());
	// LEGACY: lua_pushnumber(overlay->getHeight());
}

void Lua_Remastered::OverlayGetScreenSize() {
	warning("Stub function: OverlayGetScreenSize, returns 1920, 1080");
	lua_pushnumber(1920);
	lua_pushnumber(1080);
}

void Lua_Remastered::OverlayCreate() {
	warning("Stub function: OverlayCreate not done");
	// LEGACY: lua_Object param1 = lua_getparam(1);
	// LEGACY: lua_Object param2 = lua_getparam(2);
	// LEGACY: lua_Object param3 = lua_getparam(3);
	// LEGACY: lua_Object param4 = lua_getparam(4);
	// LEGACY: if (!lua_isstring(param1) || !lua_isnumber(param2) || !lua_isnumber(param3) || !lua_istable(param4)) {
	// LEGACY: 	return;
	// LEGACY: }
	// LEGACY: const char *overlayName = lua_getstring(param1);
	// LEGACY: float x = lua_getnumber(param2);
	// LEGACY: float y = lua_getnumber(param3);
	// LEGACY:
	// LEGACY: lua_pushobject(param4);
	// LEGACY: lua_pushstring("layer");
	// LEGACY: lua_Object table = lua_gettable();
	// LEGACY: float layer = lua_getnumber(table);
	// LEGACY:
	// LEGACY: Overlay *overlay = g_resourceloader->loadOverlay(overlayName);
	// LEGACY:
	// LEGACY: if (overlay) {
	// LEGACY: 	overlay->setPos(x, y);
	// LEGACY: 	overlay->setLayer(layer);
	// LEGACY: 	lua_pushusertag(overlay->getId(), overlay->getTag());
	// LEGACY: } else {
	// LEGACY: 	lua_pushnil();
	// LEGACY: }
}

void Lua_Remastered::OverlayDestroy() {
	// LEGACY: lua_Object actorObj = lua_getparam(1);
	// LEGACY: if (!lua_isuserdata(actorObj) || lua_tag(actorObj) != Overlay::getStaticTag())
	// LEGACY: 	return;
	// LEGACY:
	// LEGACY: Overlay *overlay = Overlay::getPool().getObject(lua_getuserdata(actorObj));
	// LEGACY: delete overlay;
}

void Lua_Remastered::OverlayFade() {
	// LEGACY: lua_Object overlayObj = lua_getparam(1);
	// LEGACY: lua_Object fadeObj = lua_getparam(2);
	// LEGACY: if (!lua_isuserdata(overlayObj) || lua_tag(overlayObj) != Overlay::getStaticTag())
	// LEGACY: 	return;
	// LEGACY: assert(lua_isnumber(fadeObj));
	// LEGACY:
	// LEGACY: int fadeType = lua_getnumber(fadeObj);
	// LEGACY: warning("Stub function: OverlayFade(%d)", fadeType);
	// LEGACY: Overlay *overlay = Overlay::getPool().getObject(lua_getuserdata(overlayObj));
	// LEGACY:
	// LEGACY: if (fadeType == 3) {
	// LEGACY: 	delete overlay;
	// LEGACY: }
}

void Lua_Remastered::OverlayMove() {
	// LEGACY: lua_Object overlayObj = lua_getparam(1);
	// LEGACY: lua_Object param2 = lua_getparam(2);
	// LEGACY: lua_Object param3 = lua_getparam(3);
	// LEGACY:
	// LEGACY: if (!lua_isuserdata(overlayObj) || lua_tag(overlayObj) != Overlay::getStaticTag())
	// LEGACY: 	return;
	// LEGACY:
	// LEGACY: Overlay *overlay = Overlay::getPool().getObject(lua_getuserdata(overlayObj));
	// LEGACY: float x = lua_getnumber(param2);
	// LEGACY: float y = lua_getnumber(param3);
	// LEGACY: overlay->setPos(x, y);
}

void Lua_Remastered::QueryActiveHotspots() {
	lua_Object param = lua_getparam(1);

	assert(lua_isnumber(param));

	warning("Stub function: QueryActiveHotspots(%f)", lua_getnumber(param));

	// LEGACY: // TODO: Handle coord scaling better
	// LEGACY: float scaleX = 1920.0f/1600;
	// LEGACY: float scaleY = 1080.0f/900;
	// LEGACY: Math::Vector2d pos(g_grim->_cursorX*scaleX, g_grim->_cursorY*scaleY);
	// LEGACY: lua_Object result = lua_createtable();
	// LEGACY: int count = 0;
	// LEGACY: for (Hotspot *h : Hotspot::getPool()) {
	// LEGACY: 	if (!h->_rect.containsPoint(pos)) {
	// LEGACY: 		continue;
	// LEGACY: 	}
	// LEGACY: 	lua_Object inner = lua_createtable();
	// LEGACY:
	// LEGACY: 	lua_pushobject(inner);
	// LEGACY: 	lua_pushstring("type");
	// LEGACY: 	lua_pushstring("normal");
	// LEGACY: 	lua_settable();
	// LEGACY:
	// LEGACY: 	lua_pushobject(inner);
	// LEGACY: 	lua_pushstring("cursor");
	// LEGACY: 	lua_pushnumber(0);
	// LEGACY: 	lua_settable();
	// LEGACY:
	// LEGACY: 	lua_pushobject(inner);
	// LEGACY: 	lua_pushstring("id");
	// LEGACY: 	lua_pushstring(h->_name.c_str());
	// LEGACY: 	lua_settable();
	// LEGACY:
	// LEGACY: 	lua_pushobject(inner);
	// LEGACY: 	lua_pushstring("obj");
	// LEGACY: 	lua_pushusertag(h->getId(), h->getTag());
	// LEGACY: 	lua_settable();
	// LEGACY:
	// LEGACY: 	lua_pushobject(result);
	// LEGACY: 	lua_pushnumber(count++);
	// LEGACY: 	lua_pushobject(inner);
	// LEGACY: 	lua_settable();
	// LEGACY: }
	// LEGACY:
	// LEGACY: lua_pushobject(result);
}

void Lua_Remastered::GetLanguage() {
	warning("Stub function: GetLanguage, just guesswork");
	// LEGACY: lua_pushnumber(g_grim->getLanguage());
}

void Lua_Remastered::SaveRegistryToDisk() {
	warning("Guesswork: SaveRegistryToDisk");
	// LEGACY: g_registry->save();
	ConfMan.flushToDisk(); // Since we can't consistently exit yet, we force a write for now
}

void Lua_Remastered::GetCursorPosition() {
	// LEGACY: lua_pushnumber(g_grim->_cursorX);
	// LEGACY: lua_pushnumber(g_grim->_cursorX);
}

void Lua_Remastered::SetCursor() {
	lua_Object param1 = lua_getparam(1);

	assert(lua_isnumber(param1));

	warning("Stub function: SetCursor(%f)", lua_getnumber(param1));
}

void Lua_Remastered::GetPlatform() {
	warning("Stub function: GetPlatform, returns 1 (windows)");
	lua_pushnumber(1);
}

void Lua_Remastered::GetRemappedKeyName() {
	lua_Object param1 = lua_getparam(1);

	const char *str = "";
	if (lua_isstring(param1)) {
		str = lua_getstring(param1);
	}
	warning("Stub function: GetRemappedKeyName(%s), returns TODO", str);

	lua_pushstring("TODO");
}

void Lua_Remastered::GetRemappedKeyHint() {
	lua_Object param1 = lua_getparam(1);
	warning("Stub function: GetRemappedKeyHint(%s), returns key_empty.png", lua_getstring(param1));
	lua_pushstring("key_empty.png");
}

void Lua_Remastered::ImSetCommentaryVol() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Stub function: ImSetCommentaryVol(%f)", lua_getnumber(param1));
}

void Lua_Remastered::ImGetCommentaryVol() {
	warning("Stub function: ImGetCommentaryVol() returns 0");
	lua_pushnumber(0);
}

void Lua_Remastered::SetLanguage() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Stub function: SetLanguage(%f)", lua_getnumber(param1));
	// LEGACY: g_grim->setLanguage(lua_getnumber(param1));
	// LEGACY: delete g_localizer;
	// LEGACY: g_localizer = new Localizer();
}

void Lua_Remastered::SetMouseSpeedScale() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Stub function: SetMouseSpeedScale(%f)", lua_getnumber(param1));
}

// Commentary
void Lua_Remastered::EnableCommentary() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Stub function: EnableCommentary(%f)", lua_getnumber(param1));
}

void Lua_Remastered::SetCommentary() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isstring(param1));
	warning("Stub function: SetCommentary(%s)", lua_getstring(param1));
	// LEGACY: g_grim->getCommentary()->setCurrentCommentary(lua_getstring(param1));
}

void Lua_Remastered::PlayCurrentCommentary() {
	warning("Stub function: PlayCurrentCommentary");
	// LEGACY: g_grim->getCommentary()->playCurrentCommentary();
}

void Lua_Remastered::HasHeardCommentary() {
	// LEGACY: lua_Object param1 = lua_getparam(1);
	// LEGACY: assert(lua_isstring(param1));
	// LEGACY: bool hasHeard = g_grim->getCommentary()->hasHeardCommentary(lua_getstring(param1));
	// LEGACY: warning("Remastered function: HasHeardCommentary(%s) returns %d", lua_getstring(param1), hasHeard);
	// LEGACY: if (hasHeard) {
	// LEGACY: 	lua_pushnumber(1);
	// LEGACY: } else {
	// LEGACY: 	lua_pushnil();
	// LEGACY: }
}

void Lua_Remastered::SetResolutionScaling() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Stub function: SetResolutionScaling(%f)", lua_getnumber(param1));
}

void Lua_Remastered::IsConceptUnlocked() {
	// LEGACY: lua_Object param1 = lua_getparam(1);
	// LEGACY: assert(lua_isnumber(param1));
	// LEGACY: warning("Remastered function: IsConceptUnlocked(%f) returns %d", lua_getnumber(param1), g_grim->isConceptEnabled(lua_getnumber(param1)));
	// LEGACY: if (g_grim->isConceptEnabled(lua_getnumber(param1))) {
	// LEGACY: 	lua_pushnumber(1);
	// LEGACY: } else {
	// LEGACY: 	lua_pushnil();
	// LEGACY: }
}

void Lua_Remastered::UnlockConcept() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Remastered function: UnlockConcept(%f)", lua_getnumber(param1));
	// LEGACY: g_grim->enableConcept(lua_getnumber(param1));
}

void Lua_Remastered::UnlockCutscene() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Remastered function: UnlockCutscene(%f)", lua_getnumber(param1));
	// LEGACY: g_grim->enableCutscene(lua_getnumber(param1));
}

void Lua_Remastered::IsCutsceneUnlocked() {
	// LEGACY: lua_Object param1 = lua_getparam(1);
	// LEGACY: assert(lua_isnumber(param1));
	// LEGACY: warning("Remastered function: IsCutsceneUnlocked(%f) returns %d", lua_getnumber(param1), g_grim->isCutsceneEnabled(lua_getnumber(param1)));
	// LEGACY: if (g_grim->isCutsceneEnabled(lua_getnumber(param1))) {
	// LEGACY: 	lua_pushnumber(1);
	// LEGACY: } else {
	// LEGACY: 	lua_pushnil();
	// LEGACY: }
}

void Lua_Remastered::GetGameRenderMode() {
	warning("Stub function: GetGameRenderMode() - not all modes possible yet");
	// LEGACY: lua_pushnumber(g_grim->getMode());
}

void Lua_Remastered::SetGameRenderMode() {
	lua_Object param1 = lua_getparam(1);
	if (lua_isnil(param1)) {
		warning("SetGameRenderMode(nil) - Should it be possible to call this with nil?");
		return;
	}
	assert(lua_isnumber(param1));
	warning("Stub function: SetGameRenderMode(%f)", lua_getnumber(param1));
	// LEGACY: g_grim->setMode((GrimEngine::EngineMode)(int)lua_getnumber(param1));
}

void Lua_Remastered::AddHotspot() {
	lua_Object param1 = lua_getparam(1);
	lua_Object param2 = lua_getparam(2);
	lua_Object param3 = lua_getparam(3);
	lua_Object param4 = lua_getparam(4);
	lua_Object param5 = lua_getparam(5);
	lua_Object param6 = lua_getparam(6);
	lua_Object param7 = lua_getparam(7);
	lua_Object param8 = lua_getparam(8);
	lua_Object param9 = lua_getparam(9);
	lua_Object param10 = lua_getparam(10);
	lua_Object param11 = lua_getparam(11);

	assert(lua_isstring(param1));
	assert(lua_isnumber(param2));
	assert(lua_isnumber(param3));
	assert(lua_isnumber(param4));
	assert(lua_isnumber(param5));
	assert(lua_isnumber(param6));
	assert(lua_isnumber(param7));
	//assert(lua_isnumber(param8));
	assert(lua_isnumber(param11));

	const char *p9str = "nil";
	const char *p10str = "nil";
	if (lua_isstring(param9)) {
		p9str = lua_getstring(param9);
	} else if (!lua_isnil(param9)) {
		assert(lua_isnil(param9));
	}

	if (lua_isstring(param10)) {
		p10str = lua_getstring(param10);
	} else if (!lua_isnil(param10)) {
		assert(lua_isnil(param10));
	}
	warning("Stub function: AddHotspot(%s, %f, %f, %f, %f, %f, %f, %f, %s, %s, %f)", lua_getstring(param1), lua_getnumber(param2), lua_getnumber(param3), lua_getnumber(param4), lua_getnumber(param5), lua_getnumber(param6), lua_getnumber(param7), lua_getnumber(param8), p9str, p10str, lua_getnumber(param11));

	// LEGACY: Hotspot *hotspot = new Hotspot(lua_getstring(param1), lua_getnumber(param2), lua_getnumber(param3), lua_getnumber(param4), lua_getnumber(param5));
	// LEGACY:
	// LEGACY: lua_pushusertag(hotspot->getId(), hotspot->getTag());
	// LEGACY:
}

void Lua_Remastered::RemoveHotspot() {
	// LEGACY: lua_Object hotspotObj = lua_getparam(1);
	// LEGACY: Hotspot *hotspot = Hotspot::getPool().getObject(lua_getuserdata(hotspotObj));
	// LEGACY: delete hotspot;
}

void Lua_Remastered::GlobalSaveResolved() {
	warning("Stub function: GlobalSaveResolved() returns 1");
	lua_pushnumber(1);
}

void Lua_Remastered::FindSaveGames() {
	warning("Stub function: FindSaveGames()");

	// LEGACY: Common::SaveFileManager *saveMan = g_grim->getSaveFileManager();
	// LEGACY: Common::StringArray saveFiles = saveMan->listSavefiles("grim_r???.sav");
	// LEGACY:
	// LEGACY: if (saveFiles.empty()) {
	// LEGACY: 	lua_pushnil();
	// LEGACY: 	return;
	// LEGACY: }
	// LEGACY:
	// LEGACY: lua_Object result = lua_createtable();
	// LEGACY:
	// LEGACY: Common::StringArray::iterator it = saveFiles.begin();
	// LEGACY: for (int i = 0; it != saveFiles.end(); ++it) {
	// LEGACY: 	const char *filename  = it->c_str();
	// LEGACY: 	warning("Savefile: %s", filename);
	// LEGACY: 	SaveGame *savedState = SaveGame::openForLoading(filename);
	// LEGACY:
	// LEGACY: 	if (!savedState || !savedState->isCompatible()) {
	// LEGACY: 		if (!savedState) {
	// LEGACY: 			error("Savegame %s is invalid", filename);
	// LEGACY: 		} else {
	// LEGACY: 			error("Savegame %s is incompatible with this ScummVM build. Save version: %d.%d; current version: %d.%d",
	// LEGACY: 				filename, savedState->saveMajorVersion(), savedState->saveMinorVersion(),
	// LEGACY: 				SaveGame::SAVEGAME_MAJOR_VERSION, SaveGame::SAVEGAME_MINOR_VERSION);
	// LEGACY: 		}
	// LEGACY: 		return;
	// LEGACY: 	}
	// LEGACY: 	int slot = atoi(it->c_str() + 6);
	// LEGACY: 	Common::String str1;
	// LEGACY: 	Common::String str2;
	// LEGACY: 	int x;
	// LEGACY: 	/*int32 dataSize = */savedState->beginSection('META');
	// LEGACY: 	char str[200] = {};
	// LEGACY: 	int32 strSize = 0;
	// LEGACY:
	// LEGACY: 	strSize = savedState->readLESint32();
	// LEGACY: 	savedState->read(str, strSize);
	// LEGACY: 	str1 = str;
	// LEGACY: 	x = savedState->readLESint32();
	// LEGACY: 	strSize = savedState->readLESint32();
	// LEGACY: 	savedState->read(str, strSize);
	// LEGACY: 	str2 = str;
	// LEGACY: 	savedState->endSection();
	// LEGACY: 	delete savedState;
	// LEGACY:
	// LEGACY: 	lua_pushobject(result);
	// LEGACY: 	lua_pushnumber(i++);
	// LEGACY:
	// LEGACY: 	str2 = g_localizer->localize(str2.c_str());
	// LEGACY:
	// LEGACY: 	lua_Object keyVal = lua_createtable();
	// LEGACY: 	// The key-value-mapping:
	// LEGACY: 	{
	// LEGACY:
	// LEGACY: 		lua_pushobject(keyVal);
	// LEGACY: 		lua_pushstring("slot");
	// LEGACY: 		lua_pushnumber(slot);
	// LEGACY: 		lua_settable();
	// LEGACY:
	// LEGACY: 		lua_pushobject(keyVal);
	// LEGACY: 		lua_pushstring("title");
	// LEGACY: 		lua_pushstring(str2.c_str());
	// LEGACY: 		lua_settable();
	// LEGACY:
	// LEGACY: 		lua_pushobject(keyVal);
	// LEGACY: 		lua_pushstring("timeDateString");
	// LEGACY: 		lua_pushstring("Unknown");
	// LEGACY: 		lua_settable();
	// LEGACY:
	// LEGACY: 		lua_pushobject(keyVal);
	// LEGACY: 		lua_pushstring("mural_info");
	// LEGACY: 		lua_pushstring(str1.c_str());
	// LEGACY: 		lua_settable();
	// LEGACY:
	// LEGACY: 		lua_pushobject(keyVal);
	// LEGACY: 		lua_pushstring("setIndex");
	// LEGACY: 		lua_pushnumber(x);
	// LEGACY: 		lua_settable();
	// LEGACY: 	}
	// LEGACY: 	lua_pushobject(keyVal);
	// LEGACY:
	// LEGACY: 	lua_settable();
	// LEGACY: }
	// LEGACY:
	// LEGACY: lua_pushobject(result);
}


void Lua_Remastered::Load() {
	// LEGACY: lua_Object fileName = lua_getparam(1);
	// LEGACY: //lua_Object param2 = lua_getparam(2);
	// LEGACY:
	// LEGACY: if (lua_isnil(fileName)) {
	// LEGACY: 	g_grim->loadGame("");
	// LEGACY: } else if (lua_isnumber(fileName)) {
	// LEGACY: 	int slot = lua_getnumber(fileName);
	// LEGACY: 	Common::String saveGameFilename = Common::String::format("grim_r%03d.sav", slot);
	// LEGACY: 	g_grim->loadGame(saveGameFilename.c_str());
	// LEGACY: } else if (lua_isstring(fileName)) { // Check for number before this
	// LEGACY: 	g_grim->loadGame(lua_getstring(fileName));
	// LEGACY: } else {
	// LEGACY: 	warning("Load() fileName is wrong");
	// LEGACY: 	return;
	// LEGACY: }
}

void Lua_Remastered::Save() {
	lua_Object param1 = lua_getparam(1);
	lua_Object param2 = lua_getparam(2);
	lua_Object param3 = lua_getparam(3);
	lua_Object param4 = lua_getparam(4);
	assert(lua_isnumber(param1));
	assert(lua_isstring(param2));
	assert(lua_isnumber(param3));
	assert(lua_isstring(param4));

	int slot = lua_getnumber(param1);
	const char *p2Str = lua_getstring(param2);
	int p3Num = lua_getnumber(param3);
	const char *p4Str = lua_getstring(param4);

	warning("REMASTERED save: %d, %s, %d, %s", slot, p2Str, p3Num, p4Str);
	// LEGACY: Common::String saveGameFilename = Common::String::format("grim_r%03d.sav", slot);
	// LEGACY: g_grim->setSaveMetaData(p2Str, p3Num, p4Str);
	// LEGACY: g_grim->saveGame(saveGameFilename.c_str());
}

void Lua_Remastered::ShowCursor() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isnumber(param1));
	warning("Stub function: ShowCursor(%f)", lua_getnumber(param1));
}

void Lua_Remastered::ReadRegistryIntValue() {
	lua_Object param1 = lua_getparam(1);
	assert(lua_isstring(param1));
	warning("Stub function: ReadRegistryIntValue(%s) returns nil", lua_getstring(param1));
	lua_pushnil();
}

// Stub function for builtin functions not yet implemented
static void stubWarning(const char *funcName) {
	warning("Stub function: %s", funcName);
}
/*
static void stubError(const char *funcName) {
	error("Stub function: %s", funcName);
}*/

#define STUB_FUNC(name) void name() { stubWarning(#name); }
#define STUB_FUNC2(name) void name() { stubError(#name); }

STUB_FUNC(Lua_Remastered::PreloadCursors)
STUB_FUNC(Lua_Remastered::GetFindSaveGameStatus)
STUB_FUNC(Lua_Remastered::InitiateFindSaveGames)
STUB_FUNC(Lua_Remastered::AreAchievementsInstalled)
STUB_FUNC(Lua_Remastered::UnlockAchievement)
STUB_FUNC(Lua_Remastered::SetAdvancedLighting)
STUB_FUNC(Lua_Remastered::IsPlayingCommentary)
STUB_FUNC(Lua_Remastered::ClearCommentary)
STUB_FUNC(Lua_Remastered::LoadRemappedKeys)
STUB_FUNC(Lua_Remastered::StopCommentaryImmediately)
STUB_FUNC(Lua_Remastered::DestroyAllUIButtonsImmediately)
STUB_FUNC(Lua_Remastered::UpdateUIButtons)
STUB_FUNC(Lua_Remastered::OverlayClearCache)
STUB_FUNC(Lua_Remastered::LinkHotspot)
STUB_FUNC(Lua_Remastered::UpdateHotspot)
STUB_FUNC(Lua_Remastered::HideMouseCursor)
STUB_FUNC(Lua_Remastered::UpdateMouseCursor)
STUB_FUNC(Lua_Remastered::SetActorHKHackMode)
STUB_FUNC(Lua_Remastered::CacheCurrentWalkVector)
STUB_FUNC(Lua_Remastered::SetKeyMappingMode)
STUB_FUNC(Lua_Remastered::ResetKeyMappingToDefault)
STUB_FUNC(Lua_Remastered::SaveRemappedKeys)
STUB_FUNC(Lua_Remastered::New)
STUB_FUNC(Lua_Remastered::RemoveBorders)
STUB_FUNC(Lua_Remastered::GetSaveStatus)
STUB_FUNC(Lua_Remastered::StartCheckOfCrossSaveStatus)
STUB_FUNC(Lua_Remastered::GetCrossSaveStatus)
STUB_FUNC(Lua_Remastered::GetFloorWalkPos)
STUB_FUNC(Lua_Remastered::CursorMovieEnabled)

struct luaL_reg remasteredMainOpcodes[] = {
	{ "GetPlatform", LUA_OPCODE(Lua_Remastered, GetPlatform) },
	{ "GetLanguage", LUA_OPCODE(Lua_Remastered, GetLanguage) },
	{ "PreloadCursors", LUA_OPCODE(Lua_Remastered, PreloadCursors) },
	{ "AreAchievementsInstalled", LUA_OPCODE(Lua_Remastered, AreAchievementsInstalled) },
	{ "UnlockAchievement", LUA_OPCODE(Lua_Remastered, UnlockAchievement) },
	{ "ImGetCommentaryVol", LUA_OPCODE(Lua_Remastered, ImGetCommentaryVol) },
	{ "ImSetCommentaryVol", LUA_OPCODE(Lua_Remastered, ImSetCommentaryVol) },
	{ "SetMouseSpeedScale", LUA_OPCODE(Lua_Remastered, SetMouseSpeedScale) },
	{ "SetResolutionScaling", LUA_OPCODE(Lua_Remastered, SetResolutionScaling) },
	{ "SetAdvancedLighting", LUA_OPCODE(Lua_Remastered, SetAdvancedLighting) },
	{ "SetLanguage", LUA_OPCODE(Lua_Remastered, SetLanguage) },
	{ "PlayCurrentCommentary", LUA_OPCODE(Lua_Remastered, PlayCurrentCommentary) },
	{ "IsPlayingCommentary", LUA_OPCODE(Lua_Remastered, IsPlayingCommentary) },
	{ "EnableCommentary", LUA_OPCODE(Lua_Remastered, EnableCommentary) },
	{ "ClearCommentary", LUA_OPCODE(Lua_Remastered, ClearCommentary) },
	{ "HasHeardCommentary", LUA_OPCODE(Lua_Remastered, HasHeardCommentary) },
	{ "SetCommentary", LUA_OPCODE(Lua_Remastered, SetCommentary) },
	{ "LoadRemappedKeys", LUA_OPCODE(Lua_Remastered, LoadRemappedKeys) },
	{ "GlobalSaveResolved", LUA_OPCODE(Lua_Remastered, GlobalSaveResolved) },
	{ "StopCommentaryImmediately", LUA_OPCODE(Lua_Remastered, StopCommentaryImmediately) },
	{ "ReadRegistryIntValue", LUA_OPCODE(Lua_Remastered, ReadRegistryIntValue) },
	{ "DestroyAllUIButtonsImmediately", LUA_OPCODE(Lua_Remastered, DestroyAllUIButtonsImmediately) },
	{ "UpdateUIButtons", LUA_OPCODE(Lua_Remastered, UpdateUIButtons) },
	{ "GetGameRenderMode", LUA_OPCODE(Lua_Remastered, GetGameRenderMode) },
	{ "SetGameRenderMode", LUA_OPCODE(Lua_Remastered, SetGameRenderMode) },
	{ "WidescreenCorrectionFactor", LUA_OPCODE(Lua_Remastered, WidescreenCorrectionFactor) },
	{ "OverlayCreate", LUA_OPCODE(Lua_Remastered, OverlayCreate) },
	{ "OverlayClearCache", LUA_OPCODE(Lua_Remastered, OverlayClearCache) },
	{ "OverlayDimensions", LUA_OPCODE(Lua_Remastered, OverlayDimensions) },
	{ "OverlayDestroy", LUA_OPCODE(Lua_Remastered, OverlayDestroy) },
	{ "OverlayFade", LUA_OPCODE(Lua_Remastered, OverlayFade) },
	{ "OverlayGetScreenSize", LUA_OPCODE(Lua_Remastered, OverlayGetScreenSize) },
	{ "OverlayMove", LUA_OPCODE(Lua_Remastered, OverlayMove) },
	{ "AddHotspot", LUA_OPCODE(Lua_Remastered, AddHotspot) },
	{ "LinkHotspot", LUA_OPCODE(Lua_Remastered, LinkHotspot) },
	{ "RemoveHotspot", LUA_OPCODE(Lua_Remastered, RemoveHotspot) },
	{ "UpdateHotspot", LUA_OPCODE(Lua_Remastered, UpdateHotspot) },
	{ "QueryActiveHotspots", LUA_OPCODE(Lua_Remastered, QueryActiveHotspots) },
	{ "HideMouseCursor", LUA_OPCODE(Lua_Remastered, HideMouseCursor) },
	{ "ShowCursor", LUA_OPCODE(Lua_Remastered, ShowCursor) },
	{ "UpdateMouseCursor", LUA_OPCODE(Lua_Remastered, UpdateMouseCursor) },
	{ "GetCursorPosition", LUA_OPCODE(Lua_Remastered, GetCursorPosition) },
	{ "SetCursor", LUA_OPCODE(Lua_Remastered, SetCursor) },
	{ "UnlockCutscene", LUA_OPCODE(Lua_Remastered, UnlockCutscene) },
	{ "IsCutsceneUnlocked", LUA_OPCODE(Lua_Remastered, IsCutsceneUnlocked) },
	{ "SetActorHKHackMode", LUA_OPCODE(Lua_Remastered, SetActorHKHackMode) },
	{ "CacheCurrentWalkVector", LUA_OPCODE(Lua_Remastered, CacheCurrentWalkVector) },
	{ "UnlockConcept", LUA_OPCODE(Lua_Remastered, UnlockConcept) },
	{ "IsConceptUnlocked", LUA_OPCODE(Lua_Remastered, IsConceptUnlocked) },
	{ "GetRemappedKeyHint", LUA_OPCODE(Lua_Remastered, GetRemappedKeyHint) },
	{ "SetKeyMappingMode", LUA_OPCODE(Lua_Remastered, SetKeyMappingMode) },
	{ "ResetKeyMappingToDefault", LUA_OPCODE(Lua_Remastered, ResetKeyMappingToDefault) },
	{ "SaveRemappedKeys", LUA_OPCODE(Lua_Remastered, SaveRemappedKeys) },
	{ "SaveRegistryToDisk", LUA_OPCODE(Lua_Remastered, SaveRegistryToDisk) },
	{ "InitiateFindSaveGames", LUA_OPCODE(Lua_Remastered, InitiateFindSaveGames) },
	{ "GetFindSaveGameStatus", LUA_OPCODE(Lua_Remastered, GetFindSaveGameStatus) },
	{ "FindSaveGames", LUA_OPCODE(Lua_Remastered, FindSaveGames) },
	{ "GetRemappedKeyName", LUA_OPCODE(Lua_Remastered, GetRemappedKeyName) },
	{ "New", LUA_OPCODE(Lua_Remastered, New) },
	{ "RemoveBorders", LUA_OPCODE(Lua_Remastered, RemoveBorders) },
	{ "GetSaveStatus", LUA_OPCODE(Lua_Remastered, GetSaveStatus) },
	{ "StartCheckOfCrossSaveStatus", LUA_OPCODE(Lua_Remastered, StartCheckOfCrossSaveStatus) },
	{ "GetCrossSaveStatus", LUA_OPCODE(Lua_Remastered, GetCrossSaveStatus) },
	{ "GetFloorWalkPos", LUA_OPCODE(Lua_Remastered, GetFloorWalkPos) },
	{ "CursorMovieEnabled", LUA_OPCODE(Lua_Remastered, CursorMovieEnabled) },
};

void Lua_Remastered::registerOpcodes() {
	// LEGACY: Lua_V1::registerOpcodes();

	// Register main opcodes functions
	luaL_openlib(remasteredMainOpcodes, ARRAYSIZE(remasteredMainOpcodes));
}

} // end of namespace Grim
