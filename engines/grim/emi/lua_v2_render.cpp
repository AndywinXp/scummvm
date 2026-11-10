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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/clightnode.h"
#include "engines/grim/emi/dimregion.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/frect.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/room_v2.h"

namespace Grim {

static const float kDefaultDimAlpha = 0.7f;
static const uint kStatsOverlayFlag = 0x20;

CLightNode *Lua_V2::getLightArg(int paramNumber, const char *opcodeName) {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom) {
		warning("%s: no current room!", opcodeName);

		return nullptr;
	}

	lua_Object nameObj = lua_getparam(paramNumber);

	if (!lua_isstring(nameObj)) {
		warning("%s: light name argument must be a string.", opcodeName);

		return nullptr;
	}

	CLightNode *light = currentRoom->findLight(lua_getstring(nameObj));

	if (!light)
		warning("%s: Invalid light name.", opcodeName);

	return light;
}

void Lua_V2::TurnLightOn() {
	CLightNode *light = getLightArg(1, "TurnLightOn");

	if (light) {
		lua_Object activeObj = lua_getparam(2);

		light->setActive(lua_isnil(activeObj) == 0);
	}
}

void Lua_V2::IsLightOn() {
	CLightNode *light = getLightArg(1, "IsLightOn");

	if (light && light->active)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::SetLightPosition() {
	CLightNode *light = getLightArg(1, "SetLightPosition");

	if (!light)
		return;

	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	CVector position;

	position.z = lua_getnumber(zObj);
	position.y = lua_getnumber(yObj);
	position.x = lua_getnumber(xObj);
	light->setLocalPosition(position);
}

void Lua_V2::GammaEnabled() {
	if (EmiEngine::hasGammaControl())
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::SetGamma() {
	lua_Object gammaObj = lua_getparam(1);

	if (lua_isnumber(gammaObj))
		EmiEngine::setGamma(lua_getnumber(gammaObj));
}

void Lua_V2::SetPixelCenter() {
	lua_Object centerObj = lua_getparam(1);

	if (!lua_isnumber(centerObj))
		return;

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom)
		return;

	if (!currentRoom->getSetupCamera(-1))
		return;

	CDrawContext *drawContext = CDrawContext::context;

	if (!drawContext)
		return;

	float value = lua_getnumber(centerObj);

	drawContext->setPixelCenter(value > 0.0f || value < 0.0f);
}

void Lua_V2::LoadBundle() {
	lua_Object islandObj = lua_getparam(1);

	if (!lua_isstring(islandObj) && !lua_isnil(islandObj))
		return;

	const char *islandName = lua_getstring(islandObj);

	if (!islandName) {
		warning("Lua_V2::LoadBundle(): the island name is nil");
		lua_pushnil();

		return;
	}

	if (EmiEngine::setIsland(islandName))
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::RenderModeUser() {
	lua_Object modeObj = lua_getparam(1);
	lua_Object soundObj = lua_getparam(2);
	int mode = EmiEngine::getMode();

	if (lua_isnil(modeObj) && mode == EmiEngine::kModeOverworld) {
		// TODO
		if (lua_isnil(soundObj)) {
			// TODO
		}

		EmiEngine::setMode(EmiEngine::getSavedMode());
	} else if (!lua_isnil(modeObj) && mode != EmiEngine::kModeOverworld) {
		EmiEngine::setSavedMode(mode);
		// TODO
		if (lua_isnil(soundObj)) {
			// TODO
		}

		EmiEngine::setMode(EmiEngine::kModeOverworld);
		toggleStatsOverlay(true);
	}
}

void Lua_V2::toggleStatsOverlay(bool hide) {
	if (hide || (_statsOverlayFlags & kStatsOverlayFlag) != 0)
		_statsOverlayFlags &= ~kStatsOverlayFlag;
	else
		_statsOverlayFlags |= kStatsOverlayFlag;
}

void Lua_V2::DimScreen() {
	lua_Object alphaObj = lua_getparam(1);
	int regionId;

	if (lua_isnumber(alphaObj))
		regionId = Grim::DimRegion::dimScreen(0, lua_getnumber(alphaObj));
	else
		regionId = Grim::DimRegion::dimScreen(0, kDefaultDimAlpha);

	lua_pushnumber((float)(uint)regionId);
}

void Lua_V2::DimRegion() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object widthObj = lua_getparam(3);
	lua_Object heightObj = lua_getparam(4);
	lua_Object alphaObj = lua_getparam(5);
	FRect region;

	region.x = lua_getnumber(xObj);
	region.y = lua_getnumber(yObj);
	region.w = lua_getnumber(widthObj);
	region.h = lua_getnumber(heightObj);

	float alpha = lua_getnumber(alphaObj);

	lua_pushnumber((float)(uint)Grim::DimRegion::dimRegion(&region, alpha));
}

void Lua_V2::UndimAll() {
	Grim::DimRegion::undimAll();
}

void Lua_V2::UndimRegion() {
	lua_Object regionObj = lua_getparam(1);

	if (lua_isnumber(regionObj)) {
		Grim::DimRegion::undimRegion(luaFtol(lua_getnumber(regionObj)));
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("UndimRegion() - argument is not a dimregion id");
		else
			warning("UndimRegion() - argument is not a dimregion id");

		lua_pushnil();
	}
}

void Lua_V2::SetSpeechMode() {
	lua_Object modeObj = lua_getparam(1);

	if (!lua_isnumber(modeObj))
		return;

	int mode = luaFtol(lua_getnumber(modeObj));

	if (mode >= 1 && mode <= 3)
		Actor::setSpeechMode(mode);
}

void Lua_V2::GetSpeechMode() {
	int mode = Actor::getSpeechMode();

	if ((mode & 2) != 0) {
		if ((mode & 1) != 0) {
			lua_pushnumber(3.0f);

			return;
		}
	} else if ((mode & 1) != 0) {
		lua_pushnumber(1.0f);

		return;
	}

	lua_pushnumber(2.0f);
}

void Lua_V2::GetCPUSpeed() {
	lua_pushnumber((float)EmiEngine::getCpuSpeed());
}

} // end of namespace Grim
