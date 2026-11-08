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

#include "math/matrix3.h"

#include "engines/grim/debug.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/actor.h"
#include "engines/grim/grim.h"
#include "engines/grim/set.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputobjectstate.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/walkplane.h"
#include "engines/grim/lua/lauxlib.h"

namespace Grim {

// Sector functions
/* Find the sector (of any type) which contains
 * the requested coordinate (x,y,z).
 */
void Lua_V1::GetPointSector() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object zObj = lua_getparam(3);
	lua_Object typeObj = lua_getparam(4);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj)) {
		lua_pushnil();
		return;
	}

	int sectorType = 4096;

	if (!lua_isnil(typeObj))
		sectorType = luaFtol(lua_getnumber(typeObj));

	float point[3];
	point[0] = lua_getnumber(xObj);
	point[1] = lua_getnumber(yObj);
	point[2] = lua_getnumber(zObj);

	WalkPlane *pPlane = g_grim->_pWalkSystem->planeAt(point, sectorType);

	if (pPlane) {
		lua_pushnumber((float)pPlane->number);
		lua_pushstring(pPlane->name);
		lua_pushnumber((float)pPlane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::GetActorSector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object typeObj = lua_getparam(2);

	if (!isSputActor(actorObj) || !lua_isnumber(typeObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	WalkPlane *pPlane = g_grim->_pWalkSystem->planeAt(pActor->actorPos, luaFtol(lua_getnumber(typeObj)));

	if (pPlane) {
		lua_pushnumber((float)pPlane->number);
		lua_pushstring(pPlane->name);
		lua_pushnumber((float)pPlane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::IsActorInSector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nameObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = nullptr;
	const char *name = nullptr;
	WalkPlane *pPlane = nullptr;

	if (lua_isstring(nameObj)) {
		pActor = getSputActor(actorObj);
		name = lua_getstring(nameObj);

		if (pActor) {
			pPlane = g_grim->_pWalkSystem->getHead();

			while (pPlane && (!strstr(pPlane->name, name) || !pPlane->within(pActor->actorPos, 1)))
				pPlane = pPlane->pNext;
		}
	}

	if (pPlane) {
		lua_pushnumber((float)pPlane->number);
		lua_pushstring(pPlane->name);
		lua_pushnumber((float)pPlane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::IsPointInSector() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object zObj = lua_getparam(3);
	lua_Object nameObj = lua_getparam(4);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj) || !lua_isstring(nameObj)) {
		lua_pushnil();
		return;
	}

	float point[3];
	point[0] = lua_getnumber(xObj);
	point[1] = lua_getnumber(yObj);
	point[2] = lua_getnumber(zObj);
	const char *name = lua_getstring(nameObj);
	WalkPlane *pPlane = g_grim->_pWalkSystem->getHead();

	while (pPlane && (!strstr(pPlane->name, name) || !pPlane->within(point, 1)))
		pPlane = pPlane->pNext;

	if (pPlane) {
		lua_pushnumber((float)pPlane->number);
		lua_pushstring(pPlane->name);
		lua_pushnumber((float)pPlane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::GetSectorOppositeEdge() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);
	lua_Object nameObj = lua_getparam(2);
	WalkPlane *pPlane = nullptr;

	if (lua_isstring(nameObj))
		pPlane = g_grim->_pWalkSystem->planeNamed(lua_getstring(nameObj));

	if (!pPlane) {
		lua_pushnil();
		return;
	}

	if (pActor && hasActorExtra(pActor, "GetSectorOppositeEdge"))
		g_grim->_pSputActor->leaveSector(pActor, pPlane);
}

void Lua_V1::MakeSectorActive() {
	lua_Object sectorObj = lua_getparam(1);

	if (!lua_isnumber(sectorObj) && !lua_isstring(sectorObj))
		return;

	lua_Object visibleObj = lua_getparam(2);
	WalkPlane *pPlane;

	if (lua_isstring(sectorObj))
		pPlane = g_grim->_pWalkSystem->planeNamed(lua_getstring(sectorObj));
	else
		pPlane = g_grim->_pWalkSystem->planeNumbered(luaFtol(lua_getnumber(sectorObj)));

	if (pPlane)
		pPlane->bActive = lua_isnil(visibleObj) ? 0 : 1;
}

// Set functions
void Lua_V1::LockSet() {
	lua_Object nameObj = lua_getparam(1);
	if (!lua_isstring(nameObj))
		return;

	Room *pRoom = static_cast<Room *>(g_grim->_pSputResource->alloc(kSputTagSet, lua_getstring(nameObj)));

	if (pRoom && !pRoom->roomRef.valid)
		g_grim->_pRes->setReference(&pRoom->roomRef, pRoom);
}

void Lua_V1::UnLockSet() {
	lua_Object nameObj = lua_getparam(1);
	if (!lua_isstring(nameObj))
		return;

	Room *pRoom = static_cast<Room *>(g_grim->_pSputResource->alloc(kSputTagSet, lua_getstring(nameObj)));

	if (pRoom && pRoom->roomRef.valid)
		g_grim->_pRes->removeReference(&pRoom->roomRef);
}

void Lua_V1::LockSetDemo() {
	warning("LockSet: Set handles no longer allowed, use filename!");
}

void Lua_V1::UnlockSetDemo() {
	warning("UnlockSet: Set handles no longer allowed.");
}

void Lua_V1::MakeCurrentSet() {
	lua_Object nameObj = lua_getparam(1);

	if (lua_isstring(nameObj))
		g_grim->makeCurrentSet(lua_getstring(nameObj));
	else
		g_grim->makeCurrentSet(nullptr);
}

void Lua_V1::MakeCurrentSetup() {
	lua_Object setupObj = lua_getparam(1);
	if (!lua_isnumber(setupObj))
		return;

	g_grim->makeCurrentSetupLib((uint)luaFtol(lua_getnumber(setupObj)));
}

void Lua_V1::GetCurrentSetup() {
	lua_Object nameObj = lua_getparam(1);
	if (!lua_isstring(nameObj))
		return;

	const char *name = lua_getstring(nameObj);

	if (scumm_stricmp(name, g_grim->_pSputRoom->getCurrentRoomName()) != 0) {
		if (g_grim->_pRes->loaded(kSputTagSet, name)) {
			Room *pRoom = static_cast<Room *>(g_grim->_pSputResource->alloc(kSputTagSet, name));

			if (pRoom)
				lua_pushnumber((float)pRoom->curSetup);
		}
	} else if (g_grim->_pSputRoom->getCurrentRoom()) {
		lua_pushnumber((float)g_grim->_pSputRoom->getCurrentRoom()->curSetup);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::PreviousSetup() {
	g_grim->previousSetup();
}

void Lua_V1::NextSetup() {
	g_grim->nextSetup();
}

void Lua_V1::GetCameraLookVector() {
	lua_Object setupObj = lua_getparam(1);
	Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();

	if (!pRoom) {
		lua_pushnil();
		return;
	}

	int setupIndex;

	if (lua_isnumber(setupObj))
		setupIndex = luaFtol(lua_getnumber(setupObj));
	else
		setupIndex = pRoom->curSetup;

	if (setupIndex < 0 || (uint)setupIndex >= pRoom->numSetups) {
		warning("GetCameraLookVector(): invalid setup %d", setupIndex);
		lua_pushnil();
		return;
	}

	RoomCamera *pCamera = &pRoom->pSetups[setupIndex].camera;
	RdVector3 lookVector;

	lookVector[0] = pCamera->position[0];
	lookVector[1] = pCamera->position[1];
	lookVector[2] = pCamera->position[2];
	lookVector[0] = (float)((double)lookVector[0] - pCamera->interest[0]);
	lookVector[1] = (float)((double)lookVector[1] - pCamera->interest[1]);
	lookVector[2] = (float)((double)lookVector[2] - pCamera->interest[2]);
	g_grim->_pRd->normalize3Acc(lookVector);

	lua_Object result = lua_createtable();
	lua_pushobject(result);
	lua_pushstring("x");
	lua_pushnumber(lookVector[0]);
	lua_settable();
	lua_pushobject(result);
	lua_pushstring("y");
	lua_pushnumber(lookVector[1]);
	lua_settable();
	lua_pushobject(result);
	lua_pushstring("z");
	lua_pushnumber(lookVector[2]);
	lua_settable();
	lua_pushobject(result);
}

/* This function makes the walkplane sectors smaller by the
 * given size. This is used when manny is holding some big
 * thing, like his scythe, that is likely to clip with the
 * things around him. The sectors are still connected, but they
 * have a margin which prevents manny to go too close to the
 * sectors edges.
 */
void Lua_V1::ShrinkBoxes() {
	lua_Object sizeObj = lua_getparam(1);

	if (lua_isnil(sizeObj))
		return;

	UnShrinkBoxes();
	g_grim->_pWalkSystem->buildCache();
	g_grim->_pWalkSystem->shrinkBoxes(lua_getnumber(sizeObj));
}

void Lua_V1::UnShrinkBoxes() {
	g_grim->_pWalkSystem->restoreCache();
	g_grim->_pWalkSystem->destroyCache();
}

/* Given a position and a size this function calculates and pushes
 * the nearest point to that which will be valid if the boxes are
 * shrunk by the amount specified.
 */
void Lua_V1::GetShrinkPos() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object zObj = lua_getparam(3);
	lua_Object rObj = lua_getparam(4);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj) || !lua_isnumber(rObj))
		return;

	RdVector3 pos;
	float radius;

	pos[0] = lua_getnumber(xObj);
	pos[1] = lua_getnumber(yObj);
	pos[2] = lua_getnumber(zObj);
	radius = lua_getnumber(rObj);

	g_grim->_pWalkSystem->buildCache();
	g_grim->_pWalkSystem->shrinkBoxes(radius);

	WalkPlane *pPlane = g_grim->_pWalkSystem->planeClosest(pos, 4096);

	if (pPlane)
		pPlane->closestPoint(pos);

	UnShrinkBoxes();

	if (pPlane) {
		lua_pushnumber(pos[0]);
		lua_pushnumber(pos[1]);
		lua_pushnumber(pos[2]);
	} else {
		lua_pushnil();
	}
}

int Lua_V1::getObjectStateHandle(SputObjectState *pState) {
	static int nextHandle = 1;

	if (!pState->luaHandle)
		pState->luaHandle = nextHandle++;

	return pState->luaHandle;
}

SputObjectState *Lua_V1::findObjectState(int handle, int *pSetup, int *pIndex) {
	Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();

	if (!pRoom || !handle)
		return nullptr;

	for (uint setup = 0; setup < pRoom->numSetups; setup++) {
		int index = 0;

		for (SputObjectState *pState = pRoom->pSetups[setup].ObjectStates; pState; pState = pState->pNext) {
			if (pState->luaHandle == handle) {
				if (pSetup)
					*pSetup = (int)setup;

				if (pIndex)
					*pIndex = index;

				return pState;
			}

			index++;
		}
	}

	return nullptr;
}

void Lua_V1::NewObjectState() {
	lua_Object setupObj = lua_getparam(1);
	lua_Object typeObj = lua_getparam(2);
	lua_Object bitmapObj = lua_getparam(3);
	lua_Object zBitmapObj = lua_getparam(4);
	lua_Object copyModeObj = lua_getparam(5);
	int copyMode = -1;

	if (!lua_isnil(copyModeObj))
		copyMode = luaFtol(lua_getnumber(copyModeObj));

	const char *zBitmap = lua_getstring(zBitmapObj);
	const char *bitmap = lua_getstring(bitmapObj);
	int type = luaFtol(lua_getnumber(typeObj));
	int setup = luaFtol(lua_getnumber(setupObj));

	SputObjectState *pState = g_grim->_pSputRoom->addObjectState(setup, type, bitmap, zBitmap, copyMode);

	if (pState)
		lua_pushusertag(getObjectStateHandle(pState), MKTAG('T','A','T','S'));
	else
		lua_pushnil();
}

void Lua_V1::FreeObjectState() {
	lua_Object stateObj = lua_getparam(1);

	if (!lua_isuserdata(stateObj) || lua_tag(stateObj) != MKTAG('T','A','T','S'))
		return;

	SputObjectState *pState = findObjectState(lua_getuserdata(stateObj), nullptr, nullptr);

	if (pState)
		g_grim->_pSputRoom->removeObjectState(pState);
}

void Lua_V1::SendObjectToBack() {
	lua_Object stateObj = lua_getparam(1);

	if (!lua_isuserdata(stateObj) || lua_tag(stateObj) != MKTAG('T','A','T','S'))
		return;

	int setup;
	SputObjectState *pState = findObjectState(lua_getuserdata(stateObj), &setup, nullptr);

	if (pState) {
		RoomSetup *pSetup = &g_grim->_pSputRoom->getCurrentRoom()->pSetups[setup];

		pSetup->ObjectStates = pState->moveToBack(pSetup->ObjectStates);
	}
}

void Lua_V1::SendObjectToFront() {
	lua_Object stateObj = lua_getparam(1);

	if (!lua_isuserdata(stateObj) || lua_tag(stateObj) != MKTAG('T','A','T','S'))
		return;

	int setup;
	SputObjectState *pState = findObjectState(lua_getuserdata(stateObj), &setup, nullptr);

	if (pState) {
		RoomSetup *pSetup = &g_grim->_pSputRoom->getCurrentRoom()->pSetups[setup];

		pSetup->ObjectStates = pState->moveToFront(pSetup->ObjectStates);
	}
}

void Lua_V1::SetObjectType() {
	lua_Object stateObj = lua_getparam(1);
	lua_Object typeObj = lua_getparam(2);

	if (!lua_isuserdata(stateObj) || lua_tag(stateObj) != MKTAG('T','A','T','S'))
		return;

	SputObjectState *pState = findObjectState(lua_getuserdata(stateObj), nullptr, nullptr);

	if (pState)
		pState->type = luaFtol(lua_getnumber(typeObj));
}

void Lua_V1::SetAmbientLight() {
	lua_Object ambientObj = lua_getparam(1);

	if (!lua_isnumber(ambientObj))
		return;

	Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();

	if (pRoom)
		g_grim->_pRd->_camera.setAmbientLight(pRoom->pSetups[pRoom->curSetup].camera.prdCamera, lua_getnumber(ambientObj));
}

void Lua_V1::SetLightIntensity() {
	lua_Object lightObj = lua_getparam(1);
	lua_Object intensityObj = lua_getparam(2);
	Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();
	int lightIndex = -1;

	if (lua_isnumber(lightObj)) {
		lightIndex = luaFtol(lua_getnumber(lightObj));
	} else if (lua_isstring(lightObj)) {
		lightIndex = g_grim->_pSputRoom->findLightByName(nullptr, lua_getstring(lightObj));
	} else {
		return;
	}

	RoomLight *pLight = g_grim->_pSputRoom->getLight(nullptr, lightIndex);

	if (!pLight || !lua_isnumber(intensityObj))
		return;

	float intensity = lua_getnumber(intensityObj);
	pLight->light.intensity = intensity;

	for (uint s = 0; s < pRoom->numSetups; s++) {
		RdCamera *pCamera = pRoom->pSetups[s].camera.prdCamera;

		if (!pCamera || !pCamera->apLights[lightIndex]) {
			warning("SetLightIntensity(): setup %u has no camera light %d", s, lightIndex);
			continue;
		}

		pCamera->apLights[lightIndex]->intensity = intensity;
	}
}

void Lua_V1::SetLightPosition() {
	lua_Object lightObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	int lightIndex = -1;

	if (lua_isnumber(lightObj)) {
		lightIndex = luaFtol(lua_getnumber(lightObj));
	} else if (lua_isstring(lightObj)) {
		lightIndex = g_grim->_pSputRoom->findLightByName(nullptr, lua_getstring(lightObj));
	} else {
		return;
	}

	Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();

	if (!pRoom)
		return;

	RoomLight *pLight = g_grim->_pSputRoom->getLight(nullptr, lightIndex);

	if (!pLight || !lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);

	pLight->position[0] = x;
	pLight->position[1] = y;
	pLight->position[2] = z;

	for (uint s = 0; s < pRoom->numSetups; s++) {
		RdCamera *pCamera = pRoom->pSetups[s].camera.prdCamera;

		if (!pCamera) {
			warning("SetLightPosition(): setup %u has no camera", s);
			continue;
		}

		pCamera->aLightPos[lightIndex][0] = x;
		pCamera->aLightPos[lightIndex][1] = y;
		pCamera->aLightPos[lightIndex][2] = z;
	}
}

void Lua_V1::TurnLightOn() {
	lua_Object lightObj = lua_getparam(1);
	lua_Object onObj = lua_getparam(2);
	int lightIndex = -1;

	if (lua_isnumber(lightObj)) {
		lightIndex = luaFtol(lua_getnumber(lightObj));
	} else if (lua_isstring(lightObj)) {
		lightIndex = g_grim->_pSputRoom->findLightByName(nullptr, lua_getstring(lightObj));
	} else {
		return;
	}

	RoomLight *pLight = g_grim->_pSputRoom->getLight(nullptr, lightIndex);

	if (pLight)
		pLight->light.bActive = !lua_isnil(onObj);
}

void Lua_V1::GetCameraPosition() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("GetCameraPosition(): no camera");
		lua_pushnil();
		return;
	}

	lua_pushnumber(pCamera->position[0]);
	lua_pushnumber(pCamera->position[1]);
	lua_pushnumber(pCamera->position[2]);
}

void Lua_V1::SetCameraPosition() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("SetCameraPosition(): no camera");
		lua_pushnil();
		return;
	}

	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(1);
	lua_Object zObj = lua_getparam(1);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		lua_pushnil();

	pCamera->position[0] = lua_getnumber(xObj);
	pCamera->position[1] = lua_getnumber(yObj);
	pCamera->position[2] = lua_getnumber(zObj);
	g_grim->_pRd->lookAt(pCamera->posMatrix, pCamera->position, pCamera->interest, pCamera->roll);
	g_grim->_pRd->_camera.update(pCamera->posMatrix);
}

void Lua_V1::SetCameraInterest() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("SetCameraInterest(): no camera");
		lua_pushnil();
		return;
	}

	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(1);
	lua_Object zObj = lua_getparam(1);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		lua_pushnil();

	pCamera->interest[0] = lua_getnumber(xObj);
	pCamera->interest[1] = lua_getnumber(yObj);
	pCamera->interest[2] = lua_getnumber(zObj);
	g_grim->_pRd->lookAt(pCamera->posMatrix, pCamera->position, pCamera->interest, pCamera->roll);
	g_grim->_pRd->_camera.update(pCamera->posMatrix);
}

void Lua_V1::GetCameraFOV() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("GetCameraFOV(): no camera");
		lua_pushnil();
		return;
	}

	lua_pushnumber(pCamera->fov);
}

void Lua_V1::SetCameraFOV() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("SetCameraFOV(): no camera");
		lua_pushnil();
		return;
	}

	lua_Object fovObj = lua_getparam(1);

	if (!lua_isnumber(fovObj))
		lua_pushnil();

	pCamera->fov = lua_getnumber(fovObj);
	pCamera->prdCamera->fov = pCamera->fov;
}

void Lua_V1::GetCameraRoll() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("GetCameraRoll(): no camera");
		lua_pushnil();
		return;
	}

	lua_pushnumber(pCamera->roll);
}

void Lua_V1::SetCameraRoll() {
	RoomCamera *pCamera = g_grim->_pSputRender->getCamera();

	if (!pCamera) {
		warning("SetCameraRoll(): no camera");
		lua_pushnil();
		return;
	}

	lua_Object rollObj = lua_getparam(1);

	if (!lua_isnumber(rollObj))
		lua_pushnil();

	pCamera->roll = lua_getnumber(rollObj);
	g_grim->_pRd->lookAt(pCamera->posMatrix, pCamera->position, pCamera->interest, pCamera->roll);
	g_grim->_pRd->_camera.update(pCamera->posMatrix);
}

} // end of namespace Grim
