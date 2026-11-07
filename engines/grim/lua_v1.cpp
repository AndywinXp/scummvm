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
#include "common/system.h"
#include "common/config-manager.h"

#include "graphics/surface.h"
#include "graphics/renderer.h"

#include "math/matrix3.h"

#include "engines/grim/debug.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/registry.h"
#include "engines/grim/grim.h"
#include "engines/grim/savegame.h"
#include "engines/grim/resource.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/blasttext.h"
#include "engines/grim/font.h"
#include "engines/grim/gfx_base.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/localize.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/sputtime.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/stdcontrol.h"
#include "engines/grim/text.h"
#include "engines/grim/wuregistry.h"
#include "engines/grim/lua/lauxlib.h"
#include "engines/grim/lua/luadebug.h"

namespace Grim {

int luaFtol(float value) {
	double number = value;

	if (!(number > -9223372036854775809.0 && number < 9223372036854775808.0)) {
		warning("luaFtol(): value out of range");
		return 0;
	}

	int64 result = (int64)number;

	if (result < -2147483648LL || result > 2147483647LL)
		warning("luaFtol(): value out of range");

	return (int)(uint32)result;
}

byte clamp_color(int c) {
	if (c < 0)
		return 0;
	else if (c > 255)
		return 255;
	else
		return c;
}

Common::String getLuaMessage() {
	lua_Object strObj = lua_getparam(1);
	if (lua_isnil(strObj))
		return "(nil)";
	if (!lua_isstring(strObj))
		return {};
	Common::String msg(lua_getstring(strObj));
	// Our print functions will handle the newline
	if (msg[msg.size() - 1] == '\n')
		msg.deleteLastChar();
	return msg;
}

int32 luaA_passresults();

void Lua_V1::new_dofile() {
	const char *fname_str = luaL_check_string(1);
	if (dofile(fname_str) == 0)
		if (luaA_passresults() == 0)
			lua_pushuserdata(0);
}

// Debugging message functions

void Lua_V1::PrintDebug() {
	if (!Debug::isChannelEnabled(Debug::Scripts))
		return;
	Common::String msg("Debug: ");
	msg += getLuaMessage();
	debug("%s", msg.c_str());
}

void Lua_V1::PrintError() {
	if (!Debug::isChannelEnabled(Debug::Scripts))
		return;
	Common::String msg("Error: ");
	msg += getLuaMessage();
	warning("%s", msg.c_str());
}

void Lua_V1::PrintWarning() {
	if (!Debug::isChannelEnabled(Debug::Scripts))
		return;
	Common::String msg("Warning: ");
	msg += getLuaMessage();
	warning( "%s", msg.c_str());
}

void Lua_V1::FunctionName() {
	const char *name = nullptr;
	const char *filename = nullptr;
	int32 line = 0;
	char buf[256];
	lua_Object funcObj = lua_getparam(1);

	if (!lua_isfunction(funcObj))
		return;

	lua_funcinfo(funcObj, &filename, &line);
	char funcType = *lua_getobjname(funcObj, &name);

	const char *typeName = "unnamed function";
	const char *funcName = "<unknown>";

	if (funcType == 'g') {
		typeName = "function";
		funcName = name;
	} else if (funcType == 't') {
		typeName = "tag method";
		funcName = name;
	}

	Common::sprintf_s(buf, "%s '%s' (%s:%d)\n", typeName, funcName, filename, (int)line);
	lua_pushstring(buf);
}

void Lua_V1::CheckForFile() {
	lua_Object strObj = lua_getparam(1);

	if (!lua_isstring(strObj))
		return;

	Common::SeekableReadStream *pFile = g_grim->openFile(lua_getstring(strObj));

	if (pFile) {
		delete pFile;
		lua_pushnumber(1.0f);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::MakeColor() {
	lua_Object rObj = lua_getparam(1);
	lua_Object gObj = lua_getparam(2);
	lua_Object bObj = lua_getparam(3);
	lua_Object aObj = lua_getparam(4);
	uint r, g, b, a;

	if (!lua_isnumber(rObj))
		r = 0;
	else
		r = clamp_color((int)lua_getnumber(rObj));

	if (!lua_isnumber(gObj))
		g = 0;
	else
		g = clamp_color((int)lua_getnumber(gObj));

	if (!lua_isnumber(bObj))
		b = 0;
	else
		b = clamp_color((int)lua_getnumber(bObj));

	if (!lua_isnumber(aObj))
		a = 0;
	else
		a = clamp_color((int)lua_getnumber(aObj));

	lua_pushusertag((int32)(r | (g << 8) | (b << 16) | (a << 24)), MKTAG('R','L','O','C'));
}

void Lua_V1::GetColorComponents() {
	lua_Object colorObj = lua_getparam(1);
	Color c(getcolor(colorObj));
	lua_pushnumber(c.getRed());
	lua_pushnumber(c.getGreen());
	lua_pushnumber(c.getBlue());
}

void Lua_V1::ReadRegistryValue() {
	lua_Object keyObj = lua_getparam(1);

	if (!lua_isstring(keyObj)) {
		lua_pushnil();
		return;
	}
	const char *key = lua_getstring(keyObj);
	WuRegistry *pWuRegistry = g_grim->_pWuRegistry;

	uint type = pWuRegistry->queryType(key);
	if (type == WuRegistry::kValueBinary) {
		lua_pushnumber(pWuRegistry->getFloat(key, 0.0f));
	} else if (type == WuRegistry::kValueString) {
		char value[256];

		value[0] = '\0';
		pWuRegistry->getStr(key, value, 255, "");
		lua_pushstring(value);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::WriteRegistryValue() {
	lua_Object keyObj = lua_getparam(1);

	if (!lua_isstring(keyObj))
		return;

	const char *key = lua_getstring(keyObj);
	lua_Object valObj = lua_getparam(2);
	WuRegistry *pWuRegistry = g_grim->_pWuRegistry;

	if (lua_isstring(valObj))
		pWuRegistry->saveStr(key, lua_getstring(valObj));
	else if (lua_isnumber(valObj))
		pWuRegistry->saveFloat(key, lua_getnumber(valObj));
}

void Lua_V1::GetAngleBetweenVectors() {
	lua_Object vec1Obj = lua_getparam(1);
	lua_Object vec2Obj = lua_getparam(2);

	if (!lua_istable(vec1Obj) || !lua_istable(vec2Obj)) {
		lua_pushnil();
		return;
	}

	RdVector3 vec1;
	RdVector3 vec2;

	lua_pushobject(vec1Obj);
	lua_pushstring("x");
	vec1[0] = lua_getnumber(lua_gettable());
	lua_pushobject(vec1Obj);
	lua_pushstring("y");
	vec1[1] = lua_getnumber(lua_gettable());
	lua_pushobject(vec1Obj);
	lua_pushstring("z");
	vec1[2] = lua_getnumber(lua_gettable());
	lua_pushobject(vec2Obj);
	lua_pushstring("x");
	vec2[0] = lua_getnumber(lua_gettable());
	lua_pushobject(vec2Obj);
	lua_pushstring("y");
	vec2[1] = lua_getnumber(lua_gettable());
	lua_pushobject(vec2Obj);
	lua_pushstring("z");
	vec2[2] = lua_getnumber(lua_gettable());

	g_grim->_pRd->normalize3Acc(vec1);
	g_grim->_pRd->normalize3Acc(vec2);

	float dot = (float)(((double)vec2[0] * vec1[0] + (double)vec2[1] * vec1[1]) + (double)vec2[2] * vec1[2]);

	lua_pushnumber((float)(90.0f - (double)g_grim->_pStdMath->arcSin3(dot)));
}

void Lua_V1::Is3DHardwareEnabled() {
	// TODO: std3D_pCurDevice (3D device layer)
	lua_pushnil();
}

void Lua_V1::SetHardwareState() {
	// TODO: std3D_pCurDevice, std3D_curDeviceIndex (hardware 3D)
	int curDevice = g_grim->_curDeviceIndex;
	int cur3DDevice = g_grim->_std3DCurDeviceIndex;

	g_grim->chooseDefaultDisplay(&g_grim->_pendingDevice, &g_grim->_pendingMode, &g_grim->_pending3DDevice);

	if (g_grim->_pendingDevice != curDevice || g_grim->_pending3DDevice != cur3DDevice)
		g_grim->_bDisplayChangePending = 1;
}

void Lua_V1::SetVideoDevices() {
	lua_Object deviceObj = lua_getparam(1);
	lua_Object modeObj = lua_getparam(2);

	// TODO: std3D_pCurDevice, std3D_curDeviceIndex (hardware 3D)
	int curDevice = g_grim->_curDeviceIndex;
	int cur3DDevice = g_grim->_std3DCurDeviceIndex;

	g_grim->_pendingDevice = luaFtol(lua_getnumber(deviceObj));
	g_grim->_pendingMode = 0;

	int mode3D = luaFtol(lua_getnumber(modeObj));

	g_grim->_pending3DDevice = mode3D;

	if (g_grim->_pendingDevice >= 0 && g_grim->_pendingDevice < kNumVideoDevices &&
		mode3D >= -1 && mode3D < kNum3DDevices &&
		(g_grim->_pendingDevice != curDevice || mode3D != cur3DDevice))
		g_grim->_bDisplayChangePending = 1;
}

void Lua_V1::GetVideoDevices() {
	lua_pushnumber((float)g_grim->_curDeviceIndex);

	// TODO: std3D_pCurDevice, std3D_curDeviceIndex (hardware 3D)
	lua_pushnumber((float)g_grim->_std3DCurDeviceIndex);
}

void Lua_V1::EnumerateVideoDevices() {
	lua_Object result = lua_createtable();
	int firstDevice = g_grim->_bEnumerateVideo == 0 ? 1 : 0;

	for (int i = firstDevice; i < kNumVideoDevices; i++) {
		lua_pushobject(result);
		lua_pushnumber((float)i);
		lua_pushstring("DIBsection windowed display");
		lua_settable();
	}

	lua_pushobject(result);
}

void Lua_V1::Enumerate3DDevices() {
	lua_Object numObj = lua_getparam(1);
	if (!lua_isnumber(numObj))
		return;

	lua_Object result = lua_createtable();

	lua_pushobject(result);
	lua_pushnumber(-1.0);
	lua_pushstring("/engn003/Software");
	lua_settable();

	lua_pushobject(result);
}

/* RotateVector takes a vector and rotates it around
 * the point (0,0,0) by the requested number of degrees.
 * This function is used to calculate the locations for
 * getting on and off of the Bone Wagon and for going up
 * and down the slide with the chain at the end of the world.
 */
void Lua_V1::RotateVector() {
	lua_Object vecObj = lua_getparam(1);
	lua_Object rotObj = lua_getparam(2);

	if (!lua_istable(vecObj) || !lua_istable(rotObj)) {
		lua_pushnil();
		return;
	}

	RdVector3 vec;
	RdVector3 rot;

	lua_pushobject(vecObj);
	lua_pushstring("x");
	vec[0] = lua_getnumber(lua_gettable());
	lua_pushobject(vecObj);
	lua_pushstring("y");
	vec[1] = lua_getnumber(lua_gettable());
	lua_pushobject(vecObj);
	lua_pushstring("z");
	vec[2] = lua_getnumber(lua_gettable());

	lua_pushobject(rotObj);
	lua_pushstring("x");
	rot[0] = lua_getnumber(lua_gettable());
	lua_pushobject(rotObj);
	lua_pushstring("y");
	rot[1] = lua_getnumber(lua_gettable());
	lua_pushobject(rotObj);
	lua_pushstring("z");
	rot[2] = lua_getnumber(lua_gettable());

	g_grim->_pRd->rotate3Acc(vec, rot);

	lua_Object resObj = lua_createtable();
	lua_pushobject(resObj);
	lua_pushstring("x");
	lua_pushnumber(vec[0]);
	lua_settable();
	lua_pushobject(resObj);
	lua_pushstring("y");
	lua_pushnumber(vec[1]);
	lua_settable();
	lua_pushobject(resObj);
	lua_pushstring("z");
	lua_pushnumber(vec[2]);
	lua_settable();

	lua_pushobject(resObj);
}

void Lua_V1::WorldToScreen() {
	lua_Object worldX = lua_getparam(1);
	lua_Object worldY = lua_getparam(2);
	lua_Object worldZ = lua_getparam(3);
	if (!lua_isnumber(worldX) || !lua_isnumber(worldY) || !lua_isnumber(worldZ)) {
		return;
	}
	RdVector3 worldPos;
	int screen[2];

	worldPos[0] = lua_getnumber(worldX);
	worldPos[1] = lua_getnumber(worldY);
	worldPos[2] = lua_getnumber(worldZ);

	if (g_grim->_pRd->_primit3.getScreenCoord(worldPos, screen)) {
		lua_pushnumber((float)screen[0]);
		lua_pushnumber((float)screen[1]);
	} else {
		lua_pushnil();
		lua_pushnil();
	}
}

void Lua_V1::FileFindDispose() {
	g_grim->_listFiles.clear();
	g_grim->_listFilesIter = nullptr;
}

void Lua_V1::FileFindNext() {
	if (g_grim->_listFilesIter == g_grim->_listFiles.end()) {
		lua_pushnil();
		Lua_V1::FileFindDispose();
	} else {
		lua_pushstring(g_grim->_listFilesIter->c_str());
		g_grim->_listFilesIter++;
	}
}

void Lua_V1::FileFindFirst() {
	lua_Object extObj = lua_getparam(1);
	if (!lua_isstring(extObj)) {
		lua_pushnil();
		return;
	}

	Lua_V1::FileFindDispose();

	const char *extension = lua_getstring(extObj);
	Common::String searchString = extension;
	if (searchString == "*.gsv") {
		searchString = "grim##.gsv";
	}
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	g_grim->_listFiles = saveFileMan->listSavefiles(searchString);
	Common::sort(g_grim->_listFiles.begin(), g_grim->_listFiles.end());
	g_grim->_listFilesIter = g_grim->_listFiles.begin();

	if (g_grim->_listFilesIter == g_grim->_listFiles.end())
		lua_pushnil();
	else
		Lua_V1::FileFindNext();
}

void Lua_V1::PerSecond() {
	lua_Object rateObj = lua_getparam(1);
	float frameTime = g_grim->_pSputTime->getFrameTimeFlex();

	lua_pushnumber((float)((double)lua_getnumber(rateObj) * frameTime));
}

void Lua_V1::EnableControl() {
	lua_Object numObj = lua_getparam(1);

	if (!lua_isnumber(numObj)) {
		lua_pushnil();
		return;
	}
	g_grim->_pSputControl->enableControl((uint)(int)lua_getnumber(numObj));
}

void Lua_V1::DisableControl() {
	lua_Object numObj = lua_getparam(1);

	if (!lua_isnumber(numObj)) {
		lua_pushnil();
		return;
	}
	g_grim->_pSputControl->disableControl((uint)(int)lua_getnumber(numObj));
}

void Lua_V1::GetControlState() {
	lua_Object numObj = lua_getparam(1);

	if (!lua_isnumber(numObj))
		return;

	uint index = (uint)(int)lua_getnumber(numObj);

	if (index >= (uint)g_grim->_pSputControl->getNumSymbols())
		return;

	ControlSymbol *pSymbol = &g_grim->_pSputControl->getSymbols()[index];

	if (pSymbol->flags & 4) {
		int pressCount = 0;
		int doubleClick = 0;

		if (g_grim->_pStdControl->readKey(pSymbol->code, &pressCount, &doubleClick))
			lua_pushnumber(1.0f);
		else
			lua_pushnil();

		if (doubleClick)
			lua_pushnumber(1.0f);
		else
			lua_pushnil();
	} else {
		float axis = g_grim->_pStdControl->readAxis(pSymbol->code);

		if (pSymbol->flags & 8)
			axis = -axis;

		lua_pushnumber(axis);
	}
}

void Lua_V1::Exit() {
	lua_Object exitObj = lua_getparam(1);

	if (lua_isnumber(exitObj))
		g_grim->requestShutdown((int)lua_getnumber(exitObj));
	else
		g_grim->requestShutdown(0);
}

void Lua_V1::SetSpeechMode() {
	lua_Object modeObj = lua_getparam(1);

	if (!lua_isnumber(modeObj))
		return;

	int mode = (int)lua_getnumber(modeObj);

	if (mode >= 1 && mode <= 3)
		g_grim->_pSputActor->setSpeechMode(mode);
}

void Lua_V1::GetSpeechMode() {
	int mode = g_grim->_pSputActor->getSpeechMode();

	if (mode & 2) {
		if (mode & 1) {
			lua_pushnumber(3.0f);
			return;
		}
	} else if (mode & 1) {
		lua_pushnumber(1.0f);
		return;
	}

	lua_pushnumber(2.0f);
}

void Lua_V1::GetDiskFreeSpace() {
	lua_pushnumber(700.0f);
}

void Lua_V1::GetCurrentScript() {
	current_script();
}

void Lua_V1::GetSaveGameImage() {
	lua_Object nameObj = lua_getparam(1);

	if (lua_isstring(nameObj)) {
		StdVBuffer *pImage = g_grim->getSaveGameImage(lua_getstring(nameObj));

		if (pImage) {
			g_grim->_pSputRender->colorCorrect((uint16 *)pImage->pPixels, pImage->tex.width * pImage->tex.height);
			pushImage(pImage);
			return;
		}
	}

	lua_pushnil();
}

void Lua_V1::SubmitSaveGameData() {
	lua_Object tableObj = lua_getparam(1);
	int index = 0;

	if (!g_grim->_pSaveWriteStream) {
		warning("Lua_V1::SubmitSaveGameData(): no save in progress");
		return;
	}

	_totSaveBytes = 0;

	for (;;) {
		lua_pushobject(tableObj);
		lua_pushnumber((float)index);
		index++;

		lua_Object entryObj = lua_gettable();

		if (lua_isnil(entryObj))
			break;

		if (!lua_isstring(entryObj)) {
			warning("Lua_V1::SubmitSaveGameData(): the table must contain only strings");
			return;
		}

		_totSaveBytes += (int)strlen(lua_getstring(entryObj)) + 5;
	}

	if (!_totSaveBytes)
		return;

	uint32 totalBytes = TO_LE_32((uint32)_totSaveBytes);

	g_grim->zwrite(&totalBytes, 4);

	index = 0;

	for (;;) {
		lua_pushobject(tableObj);
		lua_pushnumber((float)index);
		index++;

		lua_Object entryObj = lua_gettable();

		if (lua_isnil(entryObj))
			break;

		const char *str = lua_getstring(entryObj);
		int length = (int)strlen(str) + 1;
		uint32 lengthLE = TO_LE_32((uint32)length);

		g_grim->zwrite(&lengthLE, 4);
		g_grim->zwrite(str, length);
	}
}

void Lua_V1::GetSaveGameData() {
	lua_Object param = lua_getparam(1);
	if (!lua_isstring(param))
		return;
	Common::SeekableReadStream *pFile = g_system->getSavefileManager()->openForLoading(lua_getstring(param));

	if (!pFile)
		return;

	lua_Object result = lua_createtable();
	char str[1024];
	int index = 0;
	int totalBytes = pFile->readSint32LE();

	while (totalBytes > 0) {
		int strSize = pFile->readSint32LE();

		if (pFile->err() || pFile->eos() || strSize < 1 || strSize > (int)sizeof(str)) {
			warning("Lua_V1::GetSaveGameData(): invalid string size %d", strSize);
			break;
		}

		if (pFile->read(str, strSize) != (uint32)strSize) {
			warning("Lua_V1::GetSaveGameData(): unexpected end of file");
			break;
		}

		str[strSize - 1] = '\0';

		lua_pushobject(result);
		lua_pushnumber((float)index);
		lua_pushstring(str);
		lua_settable();
		index++;
		totalBytes -= strSize + 4;
	}

	lua_pushobject(result);
	delete pFile;
}

void Lua_V1::Load() {
	lua_Object fileNameObj = lua_getparam(1);

	if (lua_isnil(fileNameObj))
		g_grim->loadLib(nullptr);
	else if (lua_isstring(fileNameObj))
		g_grim->loadLib(lua_getstring(fileNameObj));
}

void Lua_V1::Save() {
	lua_Object fileNameObj = lua_getparam(1);

	if (lua_isnil(fileNameObj))
		g_grim->saveLib(nullptr);
	else if (lua_isstring(fileNameObj))
		g_grim->saveLib(lua_getstring(fileNameObj));
}

void Lua_V1::Remove() {
	if (g_system->getSavefileManager()->removeSavefile(luaL_check_string(1)))
		lua_pushuserdata(0);
	else {
		lua_pushnil();
		lua_pushstring(g_system->getSavefileManager()->getErrorDesc().c_str());
	}
}

void Lua_V1::LockFont() {
	lua_Object param1 = lua_getparam(1);
	if (lua_isstring(param1)) {
		void *pFont = g_grim->_pSputResource->alloc(kSputTagFont, lua_getstring(param1));

		if (pFont) {
			ResNode *pNode = g_grim->_pRes->checkAllocatedRes(pFont);

			if (pNode) {
				lua_pushusertag(pNode->serial, kSputTagFont);
				return;
			}

			warning("Lua_V1::LockFont(): font resource not found");
		}
	}

	lua_pushnil();
}

void Lua_V1::JustLoaded() {
	if (g_grim->justLoaded())
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V1::EnableDebugKeys() {
	g_grim->setDebugKeys(1);

	if (_pDebugToggleFunc)
		_pDebugToggleFunc(1);
}

void Lua_V1::FlushControls() {
	g_grim->_pStdControl->flush();
}

void Lua_V1::LightMgrSetChange() {
	// TODO: LightManager (Win32 light editor) not ported
}

void Lua_V1::LightMgrStartup() {
	// TODO: LightManager (Win32 light editor) not ported
}

void Lua_V1::SetEmergencyFont() {
	lua_Object fontObj = lua_getparam(1);
	SputText *pSputText = g_grim->_pSputText;

	if (lua_isstring(fontObj)) {
		if (pSputText->_pEmergencyFont) {
			g_grim->_pText->freeFont(pSputText->_pEmergencyFont);
			pSputText->_pEmergencyFont = nullptr;
		}

		g_grim->_pBlastText->load(lua_getstring(fontObj), pSputText->_pEmergencyFont);
	} else if (lua_isnil(fontObj) && pSputText->_pEmergencyFont) {
		g_grim->_pText->freeFont(pSputText->_pEmergencyFont);
		pSputText->_pEmergencyFont = nullptr;
	}
}

void Lua_V1::NukeResources() {
	g_grim->_pSputRoom->setCurrentRoom(nullptr);
	g_grim->_pSputActor->nukeCostumeStacks();
	g_grim->_pRes->freeAll();
	g_grim->_pSputActor->clearShadowMaterial();
}

void Lua_V1::AttachToResources() {
	g_grim->_pSputResource->closeResources();
	g_grim->findResources();
}

void Lua_V1::DetachFromResources() {
	g_grim->_pSputSmush->stopMovie();
	g_imuse->soundsCloseVoiceFile();
	g_grim->_pSputResource->closeResources();
}

void Lua_V1::SpewStartup() {
	// nothing to implement
	// originally this opcode launch external library 'spew.dll'
}

void Lua_V1::GetMemoryUsage() {
	lua_pushnumber((float)g_grim->_pRes->totalAllocated());
}

void Lua_V1::GetFontDimensions() {
	warning("GetFontDimensions(): the original dereferences a Lua object as a font pointer and crashes");
}

struct luaL_reg grimMainOpcodes[] = {
	{ "EngineDisplay", LUA_OPCODE(Lua_V1, EngineDisplay) },
	{ "CheckForFile", LUA_OPCODE(Lua_V1, CheckForFile) },
	{ "Load", LUA_OPCODE(Lua_V1, Load) },
	{ "Save", LUA_OPCODE(Lua_V1, Save) },
	{ "remove", LUA_OPCODE(Lua_V1, Remove) },
	{ "SetActorColormap", LUA_OPCODE(Lua_V1, SetActorColormap) },
	{ "GetActorCostume", LUA_OPCODE(Lua_V1, GetActorCostume) },
	{ "SetActorCostume", LUA_OPCODE(Lua_V1, SetActorCostume) },
	{ "SetActorScale", LUA_OPCODE(Lua_V1, SetActorScale) },
	{ "GetActorTimeScale", LUA_OPCODE(Lua_V1, GetActorTimeScale) },
	{ "SetActorTimeScale", LUA_OPCODE(Lua_V1, SetActorTimeScale) },
	{ "GetActorNodeLocation", LUA_OPCODE(Lua_V1, GetActorNodeLocation) },
	{ "SetActorWalkChore", LUA_OPCODE(Lua_V1, SetActorWalkChore) },
	{ "SetActorTurnChores", LUA_OPCODE(Lua_V1, SetActorTurnChores) },
	{ "SetActorRestChore", LUA_OPCODE(Lua_V1, SetActorRestChore) },
	{ "SetActorMumblechore", LUA_OPCODE(Lua_V1, SetActorMumblechore) },
	{ "SetActorTalkChore", LUA_OPCODE(Lua_V1, SetActorTalkChore) },
	{ "SetActorWalkRate", LUA_OPCODE(Lua_V1, SetActorWalkRate) },
	{ "GetActorWalkRate", LUA_OPCODE(Lua_V1, GetActorWalkRate) },
	{ "SetActorTurnRate", LUA_OPCODE(Lua_V1, SetActorTurnRate) },
	{ "SetSelectedActor", LUA_OPCODE(Lua_V1, SetSelectedActor) },
	{ "LoadActor", LUA_OPCODE(Lua_V1, LoadActor) },
	{ "GetActorPos", LUA_OPCODE(Lua_V1, GetActorPos) },
	{ "GetActorRect", LUA_OPCODE(Lua_V1, GetActorRect) },
	{ "GetActorPuckVector", LUA_OPCODE(Lua_V1, GetActorPuckVector) },
	{ "GetActorYawToPoint", LUA_OPCODE(Lua_V1, GetActorYawToPoint) },
	{ "SetActorReflection", LUA_OPCODE(Lua_V1, SetActorReflection) },
	{ "PutActorAtInterest", LUA_OPCODE(Lua_V1, PutActorAtInterest) },
	{ "PutActorAt", LUA_OPCODE(Lua_V1, PutActorAt) },
	{ "PutActorInSet", LUA_OPCODE(Lua_V1, PutActorInSet) },
	{ "WalkActorVector", LUA_OPCODE(Lua_V1, WalkActorVector) },
	{ "WalkActorForward", LUA_OPCODE(Lua_V1, WalkActorForward) },
	{ "DriveActorTo", LUA_OPCODE(Lua_V1, DriveActorTo) },
	{ "WalkActorTo", LUA_OPCODE(Lua_V1, WalkActorTo) },
	{ "WalkActorToAvoiding", LUA_OPCODE(Lua_V1, WalkActorToAvoiding) },
	{ "ActorLookAt", LUA_OPCODE(Lua_V1, ActorLookAt) },
	{ "SetActorLookRate", LUA_OPCODE(Lua_V1, SetActorLookRate) },
	{ "GetActorLookRate", LUA_OPCODE(Lua_V1, GetActorLookRate) },
	{ "GetVisibleThings", LUA_OPCODE(Lua_V1, GetVisibleThings) },
	{ "GetCameraActor", LUA_OPCODE(Lua_V1, GetCameraActor) },
	{ "SetActorHead", LUA_OPCODE(Lua_V1, SetActorHead) },
	{ "SetActorVisibility", LUA_OPCODE(Lua_V1, SetActorVisibility) },
	{ "SetActorFollowBoxes", LUA_OPCODE(Lua_V1, SetActorFollowBoxes) },
	{ "ShutUpActor", LUA_OPCODE(Lua_V1, ShutUpActor) },
	{ "SetActorFrustrumCull", LUA_OPCODE(Lua_V1, SetActorFrustrumCull) },
	{ "IsActorInSector", LUA_OPCODE(Lua_V1, IsActorInSector) },
	{ "GetActorSector", LUA_OPCODE(Lua_V1, GetActorSector) },
	{ "IsPointInSector", LUA_OPCODE(Lua_V1, IsPointInSector) },
	{ "GetPointSector", LUA_OPCODE(Lua_V1, GetPointSector) },
	{ "TurnActor", LUA_OPCODE(Lua_V1, TurnActor) },
	{ "GetActorRot", LUA_OPCODE(Lua_V1, GetActorRot) },
	{ "SetActorRot", LUA_OPCODE(Lua_V1, SetActorRot) },
	{ "SetActorPitch", LUA_OPCODE(Lua_V1, SetActorPitch) },
	{ "SetActorRoll", LUA_OPCODE(Lua_V1, SetActorRoll) },
	{ "IsActorTurning", LUA_OPCODE(Lua_V1, IsActorTurning) },
	{ "PlayActorChore", LUA_OPCODE(Lua_V1, PlayActorChore) },
	{ "PlayActorChoreLooping", LUA_OPCODE(Lua_V1, PlayActorChoreLooping) },
	{ "StopActorChore", LUA_OPCODE(Lua_V1, StopActorChore) },
	{ "CompleteActorChore", LUA_OPCODE(Lua_V1, CompleteActorChore) },
	{ "IsActorMoving", LUA_OPCODE(Lua_V1, IsActorMoving) },
	{ "IsActorChoring", LUA_OPCODE(Lua_V1, IsActorChoring) },
	{ "IsActorResting", LUA_OPCODE(Lua_V1, IsActorResting) },
	{ "SetActorChoreLooping", LUA_OPCODE(Lua_V1, SetActorChoreLooping) },
	{ "GetActorChores", LUA_OPCODE(Lua_V1, GetActorChores) },
	{ "GetActorCostumeDepth", LUA_OPCODE(Lua_V1, GetActorCostumeDepth) },
	{ "WorldToScreen", LUA_OPCODE(Lua_V1, WorldToScreen) },
	{ "exit", LUA_OPCODE(Lua_V1, Exit) },
	{ "FunctionName", LUA_OPCODE(Lua_V1, FunctionName) },
	{ "EnableDebugKeys", LUA_OPCODE(Lua_V1, EnableDebugKeys) },
	{ "LockFont", LUA_OPCODE(Lua_V1, LockFont) },
	{ "EnableControl", LUA_OPCODE(Lua_V1, EnableControl) },
	{ "DisableControl", LUA_OPCODE(Lua_V1, DisableControl) },
	{ "GetControlState", LUA_OPCODE(Lua_V1, GetControlState) },
	{ "PrintError", LUA_OPCODE(Lua_V1, PrintError) },
	{ "PrintWarning", LUA_OPCODE(Lua_V1, PrintWarning) },
	{ "PrintDebug", LUA_OPCODE(Lua_V1, PrintDebug) },
	{ "MakeCurrentSet", LUA_OPCODE(Lua_V1, MakeCurrentSet) },
	{ "LockSet", LUA_OPCODE(Lua_V1, LockSet) },
	{ "UnLockSet", LUA_OPCODE(Lua_V1, UnLockSet) },
	{ "MakeCurrentSetup", LUA_OPCODE(Lua_V1, MakeCurrentSetup) },
	{ "GetCurrentSetup", LUA_OPCODE(Lua_V1, GetCurrentSetup) },
	{ "NextSetup", LUA_OPCODE(Lua_V1, NextSetup) },
	{ "PreviousSetup", LUA_OPCODE(Lua_V1, PreviousSetup) },
	{ "StartFullscreenMovie", LUA_OPCODE(Lua_V1, StartFullscreenMovie) },
	{ "IsFullscreenMoviePlaying", LUA_OPCODE(Lua_V1, IsFullscreenMoviePlaying) },
	{ "StartMovie", LUA_OPCODE(Lua_V1, StartMovie) },
	{ "StopMovie", LUA_OPCODE(Lua_V1, StopMovie) },
	{ "PauseMovie", LUA_OPCODE(Lua_V1, PauseMovie) },
	{ "IsMoviePlaying", LUA_OPCODE(Lua_V1, IsMoviePlaying) },
	{ "PlaySound", LUA_OPCODE(Lua_V1, PlaySound) },
	{ "PlaySoundAt", LUA_OPCODE(Lua_V1, PlaySoundAt) },
	{ "IsSoundPlaying", LUA_OPCODE(Lua_V1, IsSoundPlaying) },
	{ "SetSoundPosition", LUA_OPCODE(Lua_V1, SetSoundPosition) },
	{ "FileFindFirst", LUA_OPCODE(Lua_V1, FileFindFirst) },
	{ "FileFindNext", LUA_OPCODE(Lua_V1, FileFindNext) },
	{ "FileFindDispose", LUA_OPCODE(Lua_V1, FileFindDispose) },
	{ "InputDialog", LUA_OPCODE(Lua_V1, InputDialog) },
	{ "WriteRegistryValue", LUA_OPCODE(Lua_V1, WriteRegistryValue) },
	{ "ReadRegistryValue", LUA_OPCODE(Lua_V1, ReadRegistryValue) },
	{ "GetSectorOppositeEdge", LUA_OPCODE(Lua_V1, GetSectorOppositeEdge) },
	{ "MakeSectorActive", LUA_OPCODE(Lua_V1, MakeSectorActive) },
	{ "PreRender", LUA_OPCODE(Lua_V1, PreRender) },
	{ "SpewStartup", LUA_OPCODE(Lua_V1, SpewStartup) },
	{ "GetCurrentScript", LUA_OPCODE(Lua_V1, GetCurrentScript) },
	{ "PrintActorCostumes", LUA_OPCODE(Lua_V1, PrintActorCostumes) },
	{ "PushActorCostume", LUA_OPCODE(Lua_V1, PushActorCostume) },
	{ "PopActorCostume", LUA_OPCODE(Lua_V1, PopActorCostume) },
	{ "LoadCostume", LUA_OPCODE(Lua_V1, LoadCostume) },
	{ "RotateVector", LUA_OPCODE(Lua_V1, RotateVector) },
	{ "GetCameraPosition", LUA_OPCODE(Lua_V1, GetCameraPosition) },
	{ "SetCameraPosition", LUA_OPCODE(Lua_V1, SetCameraPosition) },
	{ "SetCameraInterest", LUA_OPCODE(Lua_V1, SetCameraInterest) },
	{ "GetCameraFOV", LUA_OPCODE(Lua_V1, GetCameraFOV) },
	{ "SetCameraFOV", LUA_OPCODE(Lua_V1, SetCameraFOV) },
	{ "GetCameraRoll", LUA_OPCODE(Lua_V1, GetCameraRoll) },
	{ "SetCameraRoll", LUA_OPCODE(Lua_V1, SetCameraRoll) },
	{ "GetCameraLookVector", LUA_OPCODE(Lua_V1, GetCameraLookVector) },
	{ "PointActorAt", LUA_OPCODE(Lua_V1, PointActorAt) },
	{ "TurnActorTo", LUA_OPCODE(Lua_V1, TurnActorTo) },
	{ "PerSecond", LUA_OPCODE(Lua_V1, PerSecond) },
	{ "GetAngleBetweenVectors", LUA_OPCODE(Lua_V1, GetAngleBetweenVectors) },
	{ "GetAngleBetweenActors", LUA_OPCODE(Lua_V1, GetAngleBetweenActors) },
	{ "SetAmbientLight", LUA_OPCODE(Lua_V1, SetAmbientLight) },
	{ "TurnLightOn", LUA_OPCODE(Lua_V1, TurnLightOn) },
	{ "SetLightPosition", LUA_OPCODE(Lua_V1, SetLightPosition) },
	{ "SetLightIntensity", LUA_OPCODE(Lua_V1, SetLightIntensity) },
	{ "LightMgrSetChange", LUA_OPCODE(Lua_V1, LightMgrSetChange) },
	{ "LightMgrStartup", LUA_OPCODE(Lua_V1, LightMgrStartup) },
	{ "ImStartSound", LUA_OPCODE(Lua_V1, ImStartSound) },
	{ "ImStopSound", LUA_OPCODE(Lua_V1, ImStopSound) },
	{ "ImStopAllSounds", LUA_OPCODE(Lua_V1, ImStopAllSounds) },
	{ "ImGetParam", LUA_OPCODE(Lua_V1, ImGetParam) },
	{ "ImSetParam", LUA_OPCODE(Lua_V1, ImSetParam) },
	{ "ImFadeParam", LUA_OPCODE(Lua_V1, ImFadeParam) },
	{ "ImGetSfxVol", LUA_OPCODE(Lua_V1, ImGetSfxVol) },
	{ "ImSetSfxVol", LUA_OPCODE(Lua_V1, ImSetSfxVol) },
	{ "ImGetVoiceVol", LUA_OPCODE(Lua_V1, ImGetVoiceVol) },
	{ "ImSetVoiceVol", LUA_OPCODE(Lua_V1, ImSetVoiceVol) },
	{ "ImGetMusicVol", LUA_OPCODE(Lua_V1, ImGetMusicVol) },
	{ "ImSetMusicVol", LUA_OPCODE(Lua_V1, ImSetMusicVol) },
	{ "ImSetState", LUA_OPCODE(Lua_V1, ImSetState) },
	{ "ImSetSequence", LUA_OPCODE(Lua_V1, ImSetSequence) },
	{ "ImPause", LUA_OPCODE(Lua_V1, ImPause) },
	{ "ImResume", LUA_OPCODE(Lua_V1, ImResume) },
	{ "ImSetVoiceEffect", LUA_OPCODE(Lua_V1, ImSetVoiceEffect) },
	{ "LoadBundle", LUA_OPCODE(Lua_V1, LoadBundle) },
	{ "SetGamma", LUA_OPCODE(Lua_V1, SetGamma) },
	{ "SetActorWalkDominate", LUA_OPCODE(Lua_V1, SetActorWalkDominate) },
	{ "SetActorConstrain", LUA_OPCODE(Lua_V1, SetActorConstrain) },
	{ "RenderModeUser", LUA_OPCODE(Lua_V1, RenderModeUser) },
	{ "ForceRefresh", LUA_OPCODE(Lua_V1, ForceRefresh) },
	{ "DimScreen", LUA_OPCODE(Lua_V1, DimScreen) },
	{ "DimRegion", LUA_OPCODE(Lua_V1, DimRegion) },
	{ "CleanBuffer", LUA_OPCODE(Lua_V1, CleanBuffer) },
	{ "Display", LUA_OPCODE(Lua_V1, Display) },
	{ "SetSpeechMode", LUA_OPCODE(Lua_V1, SetSpeechMode) },
	{ "GetSpeechMode", LUA_OPCODE(Lua_V1, GetSpeechMode) },
	{ "SetShadowColor", LUA_OPCODE(Lua_V1, SetShadowColor) },
	{ "ActivateActorShadow", LUA_OPCODE(Lua_V1, ActivateActorShadow) },
	{ "SetActorShadowPlane", LUA_OPCODE(Lua_V1, SetActorShadowPlane) },
	{ "SetActorShadowPoint", LUA_OPCODE(Lua_V1, SetActorShadowPoint) },
	{ "SetActiveShadow", LUA_OPCODE(Lua_V1, SetActiveShadow) },
	{ "KillActorShadows", LUA_OPCODE(Lua_V1, KillActorShadows) },
	{ "AddShadowPlane", LUA_OPCODE(Lua_V1, AddShadowPlane) },
	{ "SetActorShadowValid", LUA_OPCODE(Lua_V1, SetActorShadowValid) },
	{ "FreeObjectState", LUA_OPCODE(Lua_V1, FreeObjectState) },
	{ "NewObjectState", LUA_OPCODE(Lua_V1, NewObjectState) },
	{ "SetObjectType", LUA_OPCODE(Lua_V1, SetObjectType) },
	{ "SendObjectToBack", LUA_OPCODE(Lua_V1, SendObjectToBack) },
	{ "SendObjectToFront", LUA_OPCODE(Lua_V1, SendObjectToFront) },
	{ "ActorToClean", LUA_OPCODE(Lua_V1, ActorToClean) },
	{ "FlushControls", LUA_OPCODE(Lua_V1, FlushControls) },
	{ "SetActorCollisionMode", LUA_OPCODE(Lua_V1, SetActorCollisionMode) },
	{ "SetActorCollisionScale", LUA_OPCODE(Lua_V1, SetActorCollisionScale) },
	{ "SetActorClipActive", LUA_OPCODE(Lua_V1, SetActorClipActive) },
	{ "SetActorClipPlane", LUA_OPCODE(Lua_V1, SetActorClipPlane) },
	{ "FadeOutChore", LUA_OPCODE(Lua_V1, FadeOutChore) },
	{ "FadeInChore", LUA_OPCODE(Lua_V1, FadeInChore) },
	{ "IrisDown", LUA_OPCODE(Lua_V1, IrisDown) },
	{ "IrisUp", LUA_OPCODE(Lua_V1, IrisUp) },
	{ "TextFileGetLineCount", LUA_OPCODE(Lua_V1, TextFileGetLineCount) },
	{ "TextFileGetLine", LUA_OPCODE(Lua_V1, TextFileGetLine) },
	{ "ScreenShot", LUA_OPCODE(Lua_V1, ScreenShot) },
	{ "GetSaveGameImage", LUA_OPCODE(Lua_V1, GetSaveGameImage) },
	{ "GetImage", LUA_OPCODE(Lua_V1, GetImage) },
	{ "FreeImage", LUA_OPCODE(Lua_V1, FreeImage) },
	{ "BlastImage", LUA_OPCODE(Lua_V1, BlastImage) },
	{ "BlastRect", LUA_OPCODE(Lua_V1, BlastRect) },
	{ "SubmitSaveGameData", LUA_OPCODE(Lua_V1, SubmitSaveGameData) },
	{ "GetSaveGameData", LUA_OPCODE(Lua_V1, GetSaveGameData) },
	{ "SetTextSpeed", LUA_OPCODE(Lua_V1, SetTextSpeed) },
	{ "GetTextSpeed", LUA_OPCODE(Lua_V1, GetTextSpeed) },
	{ "DetachFromResources", LUA_OPCODE(Lua_V1, DetachFromResources) },
	{ "AttachToResources", LUA_OPCODE(Lua_V1, AttachToResources) },
	{ "ActorPuckOrient", LUA_OPCODE(Lua_V1, ActorPuckOrient) },
	{ "JustLoaded", LUA_OPCODE(Lua_V1, JustLoaded) },
	{ "ResetTextures", LUA_OPCODE(Lua_V1, ResetTextures) },
	{ "ShrinkBoxes", LUA_OPCODE(Lua_V1, ShrinkBoxes) },
	{ "UnShrinkBoxes", LUA_OPCODE(Lua_V1, UnShrinkBoxes) },
	{ "GetShrinkPos", LUA_OPCODE(Lua_V1, GetShrinkPos) },
	{ "NukeResources", LUA_OPCODE(Lua_V1, NukeResources) },
	{ "SetActorInvClipNode", LUA_OPCODE(Lua_V1, SetActorInvClipNode) },
	{ "GetDiskFreeSpace", LUA_OPCODE(Lua_V1, GetDiskFreeSpace) },
	{ "SaveIMuse", LUA_OPCODE(Lua_V1, SaveIMuse) },
	{ "RestoreIMuse", LUA_OPCODE(Lua_V1, RestoreIMuse) },
	{ "GetMemoryUsage", LUA_OPCODE(Lua_V1, GetMemoryUsage) },
	{ "dofile", LUA_OPCODE(Lua_V1, new_dofile) },
};

static struct luaL_reg grimTextOpcodes[] = {
	{ "IsMessageGoing", LUA_OPCODE(Lua_V1, IsMessageGoing) },
	{ "SetSayLineDefaults", LUA_OPCODE(Lua_V1, SetSayLineDefaults) },
	{ "SetActorTalkColor", LUA_OPCODE(Lua_V1, SetActorTalkColor) },
	{ "GetActorTalkColor", LUA_OPCODE(Lua_V1, GetActorTalkColor) },
	{ "SayLine", LUA_OPCODE(Lua_V1, SayLine) },
	{ "PrintLine", LUA_OPCODE(Lua_V1, PrintLine) },
	{ "MakeTextObject", LUA_OPCODE(Lua_V1, MakeTextObject) },
	{ "GetTextObjectDimensions", LUA_OPCODE(Lua_V1, GetTextObjectDimensions) },
	{ "GetFontDimensions", LUA_OPCODE(Lua_V1, GetFontDimensions) },
	{ "ChangeTextObject", LUA_OPCODE(Lua_V1, ChangeTextObject) },
	{ "KillTextObject", LUA_OPCODE(Lua_V1, KillTextObject) },
	{ "BlastText", LUA_OPCODE(Lua_V1, BlastText) },
	{ "ExpireText", LUA_OPCODE(Lua_V1, ExpireText) },
	{ "PurgeText", LUA_OPCODE(Lua_V1, PurgeText) },
	{ "MakeColor", LUA_OPCODE(Lua_V1, MakeColor) },
	{ "GetColorComponents", LUA_OPCODE(Lua_V1, GetColorComponents) },
	{ "SetTranslationMode", LUA_OPCODE(Lua_V1, SetTranslationMode) },
	{ "GetTranslationMode", LUA_OPCODE(Lua_V1, GetTranslationMode) },
	{ "GetTextCharPosition", LUA_OPCODE(Lua_V1, GetTextCharPosition) },
	{ "LocalizeString", LUA_OPCODE(Lua_V1, LocalizeString) },
	{ "SetEmergencyFont", LUA_OPCODE(Lua_V1, SetEmergencyFont) },
	{ "SetOffscreenTextPos", LUA_OPCODE(Lua_V1, SetOffscreenTextPos) }
};

struct luaL_reg grimPrimitivesOpcodes[] = {
	{ "DrawLine", LUA_OPCODE(Lua_V1, DrawLine) },
	{ "DrawPolygon", LUA_OPCODE(Lua_V1, DrawPolygon) },
	{ "DrawRectangle", LUA_OPCODE(Lua_V1, DrawRectangle) },
	{ "ChangePrimitive", LUA_OPCODE(Lua_V1, ChangePrimitive) },
	{ "KillPrimitive", LUA_OPCODE(Lua_V1, KillPrimitive) },
	{ "PurgePrimitiveQueue", LUA_OPCODE(Lua_V1, PurgePrimitiveQueue) }
};

struct luaL_reg grimHardwareOpcodes[] = {
	{ "Is3DHardwareEnabled", LUA_OPCODE(Lua_V1, Is3DHardwareEnabled) },
	{ "GetVideoDevices", LUA_OPCODE(Lua_V1, GetVideoDevices) },
	{ "SetVideoDevices", LUA_OPCODE(Lua_V1, SetVideoDevices) },
	{ "SetHardwareState", LUA_OPCODE(Lua_V1, SetHardwareState) },
	{ "Enumerate3DDevices", LUA_OPCODE(Lua_V1, Enumerate3DDevices) },
	{ "EnumerateVideoDevices", LUA_OPCODE(Lua_V1, EnumerateVideoDevices) }
};

Lua_V1::Lua_V1() : _pHS(nullptr), _bInitialized(0), _pDebugToggleFunc(nullptr), _totSaveBytes(0), _nextImageHandle(1) {
	for (int i = 0; i < kMaxImages; i++) {
		_imageHandles[i] = 0;
		_images[i] = nullptr;
	}
}

void Lua_V1::registerMainOpcodes() {
	luaL_openlib(grimMainOpcodes, ARRAYSIZE(grimMainOpcodes));
}

void Lua_V1::registerTextOpcodes() {
	luaL_openlib(grimTextOpcodes, ARRAYSIZE(grimTextOpcodes));
}

void Lua_V1::registerPrimitiveOpcodes() {
	luaL_openlib(grimPrimitivesOpcodes, ARRAYSIZE(grimPrimitivesOpcodes));
}

void Lua_V1::registerHardwareOpcodes() {
	luaL_openlib(grimHardwareOpcodes, ARRAYSIZE(grimHardwareOpcodes));
}

void Lua_V1::registerOpcodes() {
	registerMainOpcodes();
	registerTextOpcodes();
	registerPrimitiveOpcodes();
	registerHardwareOpcodes();

	LuaBase::registerOpcodes();
}

void Lua_V1::boot() {
	// The default value of these globals, defined in _controls.lua, is 256, 257, 258, 259.
	// These values clash with the numpad 0, 1, 2 and 3 keycodes, so we set them here.
	lua_pushnumber(KEYCODE_JOY1_HLEFT);
	lua_setglobal("JOYSTICK_X_LEFT");

	lua_pushnumber(KEYCODE_JOY1_HRIGHT);
	lua_setglobal("JOYSTICK_X_RIGHT");

	lua_pushnumber(KEYCODE_JOY1_HUP);
	lua_setglobal("JOYSTICK_Y_UP");

	lua_pushnumber(KEYCODE_JOY1_HDOWN);
	lua_setglobal("JOYSTICK_Y_DOWN");

	LuaBase::boot();
}

void Lua_V1::postRestoreHandle() {
	if (g_grim->getGameType() == GType_GRIM) {
		lua_beginblock();
		// Set the developerMode, since the save contains the value of
		// the installation it was made with.
		lua_pushobject(lua_getglobal("developerMode"));
		bool devMode = g_registry->getBool("good_times");
		pushbool(devMode);
		lua_setglobal("developerMode");
		lua_endblock();
	}

	// Starting a movie calls the function 'music_state.pause()', which saves the current sfx volume to a temp
	// variable and sets it to 0. When the movie finishes 'music_state.unpause()' will be called, which reads
	// the volume from the temp variable and sets it. But if we have modified the sfx volume in the options
	// and than load a savegame made during a movie, at the end of the movie the temp variable will have the
	// old value. So here we call 'music_state.pause()' again, so that it can set the right value to the
	// temp variable.
	lua_beginblock();
	lua_Object o = lua_getglobal("music_state");
	if (!lua_isnil(o)) {
		lua_pushobject(o);
		lua_pushstring("paused");
		if (!lua_isnil(lua_gettable())) {
			lua_pushobject(o);
			lua_pushstring("paused");
			pushbool(false);
			lua_settable();

			lua_pushobject(o);
			lua_pushstring("pause");
			lua_Object func = lua_gettable();
			lua_pushobject(o);
			lua_callfunction(func);
		}
	}
	lua_endblock();
}

int Lua_V1::initialize(SysStruct *pHS) {
	_pHS = pHS;
	_bInitialized = 1;

	return 0;
}

int Lua_V1::terminate() {
	_pHS = nullptr;

	if (_bInitialized)
		_bInitialized = 0;

	return 0;
}

int Lua_V1::startup() {
	registerMainOpcodes();
	recognizeOpcodes();
	makeSystemTable();
	registerTypeOverride();
	registerSectorConstants();
	registerConcatFallback();

	lua_savehook = saveIndirector;
	lua_restorehook = restoreIndirector;

	g_grim->_pSputControl->addCallback(this);

	return 0;
}

int Lua_V1::shutdown() {
	FileFindDispose();

	lua_unref(refSystemTable);
	lua_savehook = nullptr;
	lua_restorehook = nullptr;

	return 0;
}

int Lua_V1::findSystemFiles() {
	return dofile("_system.lua") == 0;
}

int Lua_V1::boot(int saveGameResume, const char *bootParam) {
	if (saveGameResume)
		lua_pushnumber(1);
	else
		lua_pushnil();

	if (bootParam)
		lua_pushstring(bootParam);
	else
		lua_pushnil();

	return lua_call("BOOT") == 0;
}

void Lua_V1::update() {
	int movieTime = g_grim->_pSputSmush->getMovieTime();
	int frameTime = g_grim->_pSputTime->getFrameTime();

	LuaBase::update(frameTime, movieTime);
}

void Lua_V1::makeSystemTable() {
	lua_Object systemTable = lua_createtable();
	lua_pushobject(systemTable);
	lua_setglobal("system");

	lua_pushobject(systemTable);
	refSystemTable = lua_ref(1);

	registerControls();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("camChangeHandler");
	lua_pushcfunction(LUA_OPCODE(LuaBase, dummyHandler));
	lua_settable();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("axisHandler");
	lua_pushcfunction(LUA_OPCODE(LuaBase, dummyHandler));
	lua_settable();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("buttonHandler");
	lua_pushcfunction(LUA_OPCODE(LuaBase, dummyHandler));
	lua_settable();
}

void Lua_V1::registerControls() {
	lua_Object controlsTable = lua_createtable();

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring("controls");
	lua_pushobject(controlsTable);
	lua_settable();

	ControlSymbol *aSymbols = g_grim->_pSputControl->getSymbols();
	int numSymbols = g_grim->_pSputControl->getNumSymbols();

	for (int i = 0; i < numSymbols; i++) {
		lua_pushobject(controlsTable);
		lua_pushstring(aSymbols[i].pName);
		lua_pushnumber((float)i);
		lua_settable();
	}
}

bool Lua_V1::getSystemHandler(const char *name, bool bAllowTable, lua_Object *pFunc, lua_Object *pSelf) {
	*pFunc = LUA_NOOBJECT;
	*pSelf = LUA_NOOBJECT;

	lua_pushobject(lua_getref(refSystemTable));
	lua_pushstring(name);
	lua_Object handler = lua_gettable();

	if (bAllowTable && lua_istable(handler)) {
		lua_pushobject(handler);
		lua_pushstring(name);
		lua_Object func = lua_gettable();

		if (!lua_isfunction(func))
			return false;

		*pFunc = func;
		*pSelf = handler;

		return true;
	}

	if (lua_isfunction(handler)) {
		*pFunc = handler;

		return true;
	}

	return false;
}

int Lua_V1::controlCallback(float frameTime) {
	StdControl *pStdControl = g_grim->_pStdControl;
	ControlSymbol *aSymbols = g_grim->_pSputControl->getSymbols();
	int numSymbols = g_grim->_pSputControl->getNumSymbols();
	lua_Object buttonFunc;
	lua_Object buttonSelf;
	lua_Object axisFunc;
	lua_Object axisSelf;

	lua_beginblock();

	bool bButtonHandler = getSystemHandler("buttonHandler", true, &buttonFunc, &buttonSelf);
	bool bAxisHandler = getSystemHandler("axisHandler", true, &axisFunc, &axisSelf);

	for (int i = 0; i < numSymbols; i++) {
		ControlSymbol *pSymbol = &aSymbols[i];

		if (!(pSymbol->flags & 1))
			continue;

		if (pSymbol->flags & 4) {
			int pressCount = 0;
			int doubleClick = 0;
			int value = pStdControl->readKey(pSymbol->code, &pressCount, &doubleClick);

			if (value != pSymbol->lastValue && bButtonHandler) {
				if (buttonSelf != LUA_NOOBJECT)
					lua_pushobject(buttonSelf);

				lua_pushnumber((float)i);

				if (value)
					lua_pushnumber(1);
				else
					lua_pushnil();

				lua_pushnumber((float)(uint)pressCount);
				lua_pushnumber((float)doubleClick);
				lua_callfunction(buttonFunc);
				pSymbol->lastValue = value;
			}
		} else {
			float value = pStdControl->readAxis(pSymbol->code);
			float lastValue;

			if (pSymbol->flags & 8)
				value = -value;

			memcpy(&lastValue, &pSymbol->lastValue, sizeof(lastValue));

			if (value != lastValue && bAxisHandler) {
				if (axisSelf != LUA_NOOBJECT)
					lua_pushobject(axisSelf);

				lua_pushnumber((float)i);
				lua_pushnumber(value);
				lua_callfunction(axisFunc);
				memcpy(&pSymbol->lastValue, &value, sizeof(value));
			}
		}
	}

	lua_endblock();

	return 0;
}

void Lua_V1::setDebugToggleCallback(void (*pFunc)(int)) {
	_pDebugToggleFunc = pFunc;
}

void Lua_V1::cameraChange(int oldSetup, int newSetup) {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	if (getSystemHandler("camChangeHandler", false, &func, &self)) {
		lua_pushnumber((float)oldSetup);
		lua_pushnumber((float)newSetup);
		lua_callfunction(func);
	}

	lua_endblock();
}

void Lua_V1::postCameraChange(int setup) {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	if (getSystemHandler("postCamChangeHandler", false, &func, &self)) {
		lua_pushnumber((float)setup);
		lua_callfunction(func);
	}

	lua_endblock();
}

void Lua_V1::collisionHandler(SputActor *pActor, SputActor *pOther, const float *pPos0, const float *pPos1, const float *pPos2) {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	if (getSystemHandler("collisionHandler", true, &func, &self)) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_pushusertag(g_grim->_pSputActor->getActorHandle(pActor), MKTAG('R','T','C','A'));
		lua_pushusertag(g_grim->_pSputActor->getActorHandle(pOther), MKTAG('R','T','C','A'));
		lua_pushnumber(pPos0[0]);
		lua_pushnumber(pPos0[1]);
		lua_pushnumber(pPos0[2]);
		lua_pushnumber(pPos1[0]);
		lua_pushnumber(pPos1[1]);
		lua_pushnumber(pPos1[2]);
		lua_pushnumber(pPos2[0]);
		lua_pushnumber(pPos2[1]);
		lua_pushnumber(pPos2[2]);
		lua_callfunction(func);
	}

	lua_endblock();
}

void Lua_V1::doCostumeOverrideCallback(SputActor *pActor, int bOverride) {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	int hActor = g_grim->_pSputActor->getActorHandle(pActor);

	if (getSystemHandler("costumeOverrideHandler", true, &func, &self)) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_pushusertag(hActor, MKTAG('R','T','C','A'));

		if (bOverride)
			lua_pushnumber(1);
		else
			lua_pushnil();

		lua_callfunction(func);
	}

	lua_endblock();
}

void Lua_V1::doCostumeMarkerCallback(SputActor *pActor, int markerType) {
	lua_Object func;
	lua_Object self;

	if (!markerType)
		return;

	int hActor = g_grim->_pSputActor->getActorHandle(pActor);

	lua_beginblock();

	if (getSystemHandler("costumeMarkerHandler", true, &func, &self)) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_pushusertag(hActor, MKTAG('R','T','C','A'));
		lua_pushnumber((float)(uint)markerType);
		lua_callfunction(func);
	}

	lua_endblock();
}

void Lua_V1::doUserPaintCallback() {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	if (getSystemHandler("userPaintHandler", true, &func, &self)) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_callfunction(func);
	}

	lua_endblock();
}

int Lua_V1::doExitCallback() {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	bool bFound = getSystemHandler("exitHandler", true, &func, &self);

	if (bFound) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_callfunction(func);
	}

	lua_endblock();

	return bFound;
}

int Lua_V1::doPauseCallback() {
	lua_Object func;
	lua_Object self;

	lua_beginblock();

	bool bFound = getSystemHandler("pauseHandler", true, &func, &self);

	if (bFound) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_callfunction(func);
	}

	lua_endblock();

	return bFound;
}

int Lua_V1::doCharacterCallback(int character) {
	lua_Object func;
	lua_Object self;
	int result = 0;

	lua_beginblock();

	if (getSystemHandler("characterHandler", true, &func, &self)) {
		char str[2];

		str[0] = (char)(byte)character;
		str[1] = '\0';

		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_pushstring(str);
		lua_callfunction(func);
		result = !lua_isnil(lua_getresult(1));
	}

	lua_endblock();

	return result;
}

void Lua_V1::saveGameCallback() {
	lua_Object func;
	lua_Object self;

	_totSaveBytes = 0;

	lua_beginblock();

	if (getSystemHandler("saveGameCallback", true, &func, &self)) {
		if (self != LUA_NOOBJECT)
			lua_pushobject(self);

		lua_callfunction(func);
	}

	if (!_totSaveBytes)
		g_grim->zwrite(&_totSaveBytes, 4);

	lua_endblock();
}

int32 Lua_V1::saveIndirector(int32 tag, int32 hData, tSaveRestoreAccess *saveFunc) {
	if (tag == MKTAG('T', 'A', 'T', 'S')) {
		int setup;
		int index;

		if (!g_grim->_pSputRoom->getCurrentRoom() || !findObjectState(hData, &setup, &index))
			return -1;

		return index | (setup << 16);
	}

	return hData;
}

int32 Lua_V1::restoreIndirector(int32 tag, int32 hData, tSaveRestoreAccess *restoreFunc) {
	if (tag == MKTAG('T', 'A', 'T', 'S')) {
		Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();

		if (hData == -1 || !pRoom)
			return 0;

		uint setup = (uint)hData >> 16;
		uint index = (uint)hData & 0xFFFF;

		if (setup >= pRoom->numSetups) {
			warning("Lua_V1::restoreIndirector(): invalid setup %u for an object state", setup);
			return 0;
		}

		SputObjectState *pState = pRoom->pSetups[setup].ObjectStates;

		while (pState && index) {
			pState = pState->pNext;
			index--;
		}

		if (!pState) {
			warning("Lua_V1::restoreIndirector(): object state not found");
			return 0;
		}

		return getObjectStateHandle(pState);
	}

	if (tag == MKTAG('T', 'X', 'E', 'T')) {
		if (g_grim->_pSputText->findSerialNumber(hData))
			return hData;

		return 0;
	}

	return hData;
}

} // end of namespace Grim
