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

#include "engines/grim/grim.h"
#include "engines/grim/lua_v1_opcodes/lua_v1.h"
#include "engines/grim/chore/cos.h"
#include "engines/grim/puck.h"
#include "engines/grim/res.h"
#include "engines/grim/actor/sputactor.h"
#include "engines/grim/costume/sputcostume.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/render/sputrender.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputtime.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/walkplane.h"
#include "engines/grim/lua/lauxlib.h"

namespace Grim {

bool Lua_V1::isSputActor(lua_Object obj) {
	return lua_isuserdata(obj) && lua_tag(obj) == MKTAG('R','T','C','A');
}

SputActor *Lua_V1::getSputActor(lua_Object obj) {
	return g_grim->_pSputActor->actorDeref(lua_getuserdata(obj));
}

void Lua_V1::pushSputActor(int handle) {
	lua_pushusertag(handle, MKTAG('R','T','C','A'));
}

bool Lua_V1::hasActorExtra(SputActor *pActor, const char *opcode) {
	if (pActor->pExtra)
		return true;

	warning("%s(): the actor has no extra data", opcode);
	return false;
}

bool Lua_V1::findCostumeInStack(lua_Object obj, CostumeNode *pStack, CostumeNode **ppFound) {
	*ppFound = pStack;

	if (lua_isnil(obj))
		return true;

	if (lua_isnumber(obj)) {
		int depth = luaFtol(lua_getnumber(obj));

		if (*ppFound) {
			while (depth > 0) {
				depth--;
				*ppFound = (*ppFound)->pNext;

				if (!*ppFound)
					return false;
			}
		}

		return *ppFound != nullptr;
	}

	if (lua_isstring(obj)) {
		const char *costumeName = lua_getstring(obj);
		bool bDemo = (g_grim->getGameFlags() & ADGF_DEMO) != 0;

		if (*ppFound) {
			do {
				CosCostume *pCostume = g_grim->_pSputCostume->getCos()->instGetCostume((*ppFound)->hCostumeInstance, nullptr);

				if (pCostume && !(bDemo ? strcmp(pCostume->name, costumeName) : scumm_stricmp(pCostume->name, costumeName)))
					break;

				*ppFound = (*ppFound)->pNext;
			} while (*ppFound);
		}

		return *ppFound != nullptr;
	}

	return false;
}

void Lua_V1::LoadActor() {
	int handle = g_grim->_pSputActor->allocateActor();

	if (handle < 0)
		lua_pushnil();
	else
		lua_pushusertag(handle, MKTAG('R','T','C','A'));
}

void Lua_V1::GetActorTimeScale() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	lua_pushnumber(pActor->pExtra->timeScale);
}

void Lua_V1::SetSelectedActor() {
	lua_Object actorObj = lua_getparam(1);
	if (!lua_isuserdata(actorObj) || lua_tag(actorObj) != MKTAG('R','T','C','A'))
		return;

	int handle = g_grim->_pSputActor->setSelectedActor(lua_getuserdata(actorObj));

	if (handle < 0)
		lua_pushnil();
	else
		lua_pushusertag(handle, MKTAG('R','T','C','A'));
}

void Lua_V1::GetCameraActor() {
	pushSputActor(g_grim->_pSputRender->getCamActorIndex());
}

void Lua_V1::SetActorTalkColor() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object colorObj = lua_getparam(2);

	if (lua_isuserdata(actorObj) && lua_tag(actorObj) == MKTAG('R','T','C','A')) {
		SputActor *actor = g_grim->_pSputActor->actorDeref(lua_getuserdata(actorObj));

		if (!actor)
			return;

		if (!actor->pExtra)
			g_grim->_pSputActor->allocateActorExtra(actor);

		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('R','L','O','C'))
			actor->pExtra->talkColor = lua_getuserdata(colorObj);
	}
}

void Lua_V1::GetActorTalkColor() {
	lua_Object actorObj = lua_getparam(1);
	if (lua_isuserdata(actorObj) && lua_tag(actorObj) == MKTAG('R','T','C','A')) {
		SputActor *actor = g_grim->_pSputActor->actorDeref(lua_getuserdata(actorObj));

		if (actor && actor->pExtra) {
			lua_pushusertag(actor->pExtra->talkColor, MKTAG('R','L','O','C'));
			return;
		}
	}
	lua_pushnil();
}

void Lua_V1::SetActorRestChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);
	CostumeNode *pNode;
	int chore;

	if (!isSputActor(actorObj) || (!lua_isnumber(choreObj) && !lua_isnil(choreObj)))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	// The demo has no costume parameter
	if (g_grim->getGameFlags() & ADGF_DEMO)
		pNode = nullptr;
	else if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (lua_isnil(choreObj))
		chore = -1;
	else
		chore = luaFtol(lua_getnumber(choreObj));

	g_grim->_pSputActor->setRestChore(pActor, chore, pNode);
}

void Lua_V1::SetActorWalkChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);
	CostumeNode *pNode;
	int chore;

	bool bDemo = (g_grim->getGameFlags() & ADGF_DEMO) != 0;

	// The demo does not accept nil and has no costume parameter
	if (!isSputActor(actorObj) || (!lua_isnumber(choreObj) && (bDemo || !lua_isnil(choreObj))))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (bDemo)
		pNode = nullptr;
	else if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (lua_isnumber(choreObj))
		chore = luaFtol(lua_getnumber(choreObj));
	else
		chore = -1;

	g_grim->_pSputActor->setWalkChore(pActor, chore, pNode);
}

void Lua_V1::SetActorTurnChores() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object leftChoreObj = lua_getparam(2);
	lua_Object rightChoreObj = lua_getparam(3);
	lua_Object costumeObj = lua_getparam(4);
	CostumeNode *pNode;

	if (!isSputActor(actorObj) || !lua_isnumber(leftChoreObj) || !lua_isnumber(rightChoreObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	// The demo has no costume parameter
	if (g_grim->getGameFlags() & ADGF_DEMO)
		pNode = nullptr;
	else if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	int rightChore = luaFtol(lua_getnumber(rightChoreObj));
	int leftChore = luaFtol(lua_getnumber(leftChoreObj));

	g_grim->_pSputActor->setTurnChores(pActor, leftChore, rightChore, pNode);
}

void Lua_V1::SetActorTalkChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object indexObj = lua_getparam(2);
	lua_Object choreObj = lua_getparam(3);
	lua_Object costumeObj = lua_getparam(4);
	CostumeNode *pNode;
	int chore;

	if (!isSputActor(actorObj) || !lua_isnumber(indexObj) || (!lua_isnumber(choreObj) && !lua_isnil(choreObj)))
		return;

	int index = luaFtol(lua_getnumber(indexObj));

	if (index < 1 || index > 16)
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	// The demo has no costume parameter
	if (g_grim->getGameFlags() & ADGF_DEMO)
		pNode = pActor->pCostumeStack;
	else if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (!pNode)
		return;

	if (lua_isnumber(choreObj))
		chore = luaFtol(lua_getnumber(choreObj));
	else
		chore = -1;

	pNode->aChore[index - 1] = chore;
}

void Lua_V1::SetActorMumblechore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);
	CostumeNode *pNode;

	if (!isSputActor(actorObj) || !lua_isnumber(choreObj))
		return;

	int chore = luaFtol(lua_getnumber(choreObj));
	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	// The demo has no costume parameter
	if (g_grim->getGameFlags() & ADGF_DEMO)
		pNode = pActor->pCostumeStack;
	else if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (!pNode)
		return;

	pNode->mumbleChore = chore;
}

void Lua_V1::SetActorVisibility() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object visibleObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (lua_isnil(visibleObj))
		pActor->flags |= 2;
	else
		pActor->flags &= ~2;
}

void Lua_V1::SetActorScale() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object scaleObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (lua_isnumber(scaleObj))
		pActor->scale = lua_getnumber(scaleObj);
	else
		pActor->scale = 1.0f;
}

void Lua_V1::GetActorScale() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj)) {
		if (g_grim->getGameFlags() & ADGF_DEMO)
			warning("GetActorScale: Invalid actor handle!");

		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	lua_pushnumber(pActor->scale);
}

void Lua_V1::SetActorTimeScale() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object scaleObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	if (lua_isnumber(scaleObj))
		pActor->pExtra->timeScale = lua_getnumber(scaleObj);
	else
		pActor->pExtra->timeScale = 1.0f;

	if (!(pActor->pExtra->timeScale >= 0.0f))
		pActor->pExtra->timeScale = -pActor->pExtra->timeScale;
}

void Lua_V1::SetActorCollisionMode() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object modeObj  = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	pActor->pExtra->collisionMode = luaFtol(lua_getnumber(modeObj));
}

void Lua_V1::SetActorCollisionScale() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object scaleObj  = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	pActor->pExtra->collisionScale = lua_getnumber(scaleObj);
}

void Lua_V1::PutActorAt() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);

	if (!isSputActor(actorObj) || !lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	RdVector3 pos;
	pos[0] = lua_getnumber(xObj);
	pos[1] = lua_getnumber(yObj);
	pos[2] = lua_getnumber(zObj);
	g_grim->_pSputActor->setActorPosition(pActor, pos);
}

void Lua_V1::GetActorPos() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	lua_pushnumber(pActor->actorPos[0]);
	lua_pushnumber(pActor->actorPos[1]);
	lua_pushnumber(pActor->actorPos[2]);
}

void Lua_V1::SetActorRot() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object pitchObj = lua_getparam(2);
	lua_Object yawObj = lua_getparam(3);
	lua_Object rollObj = lua_getparam(4);
	lua_Object flipObj = lua_getparam(5);

	if (!isSputActor(actorObj) || !lua_isnumber(pitchObj) || !lua_isnumber(yawObj) || !lua_isnumber(rollObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	RdVector3 rot;
	rot[0] = lua_getnumber(pitchObj);
	rot[1] = lua_getnumber(yawObj);
	rot[2] = lua_getnumber(rollObj);
	g_grim->_pSputActor->setActorRotation(pActor, rot, lua_isnil(flipObj));
}

void Lua_V1::GetActorRot() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	lua_pushnumber(pActor->actorRot[0]);
	lua_pushnumber(pActor->actorRot[1]);
	lua_pushnumber(pActor->actorRot[2]);
}

void Lua_V1::IsActorTurning() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	if (pActor->pExtra->alignOff != 0.0f)
		lua_pushnumber(1.0f);

	lua_pushnil();
}

void Lua_V1::GetAngleBetweenActors() {
	lua_Object actor1Obj = lua_getparam(1);
	lua_Object actor2Obj = lua_getparam(2);

	if (!isSputActor(actor1Obj) || !isSputActor(actor2Obj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor1 = getSputActor(actor1Obj);
	SputActor *pActor2 = getSputActor(actor2Obj);

	if (!pActor1 || !pActor2 || !hasActorExtra(pActor1, "GetAngleBetweenActors") || !pActor1->pExtra->puck) {
		lua_pushnil();
		return;
	}

	float *pLook = pActor1->pExtra->puck->orient[1];
	RdVector3 lookVec;
	RdVector3 targetVec;

	lookVec[0] = pLook[0];
	lookVec[1] = pLook[1];
	lookVec[2] = pLook[2];
	targetVec[0] = pActor2->actorPos[0];
	targetVec[1] = pActor2->actorPos[1];
	targetVec[2] = lookVec[2];
	targetVec[0] = (float)((double)targetVec[0] - pActor1->actorPos[0]);
	targetVec[1] = (float)((double)targetVec[1] - pActor1->actorPos[1]);
	targetVec[2] = (float)((double)lookVec[2] - pActor1->actorPos[2]);

	g_grim->_pRd->normalize3Acc(lookVec);
	g_grim->_pRd->normalize3Acc(targetVec);

	float dot = (float)(((double)targetVec[0] * lookVec[0] + (double)targetVec[1] * lookVec[1]) + (double)targetVec[2] * lookVec[2]);

	lua_pushnumber((float)(90.0f - (double)g_grim->_pStdMath->arcSin3(dot)));
}

void Lua_V1::GetActorYawToPoint() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object pointObj = lua_getparam(2);
	lua_Object xObj, yObj, zObj;

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	if (lua_istable(pointObj)) {
		lua_pushobject(pointObj);
		lua_pushstring("x");
		xObj = lua_gettable();
		lua_pushobject(pointObj);
		lua_pushstring("y");
		yObj = lua_gettable();
		lua_pushobject(pointObj);
		lua_pushstring("z");
		zObj = lua_gettable();
	} else {
		xObj = pointObj;
		yObj = lua_getparam(3);
		zObj = lua_getparam(4);
	}

	RdVector3 target;

	target[0] = lua_getnumber(xObj);
	target[1] = lua_getnumber(yObj);
	target[2] = lua_getnumber(zObj);

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	if (!pActor->pExtra->puck)
		g_grim->_pSputActor->enablePuck(pActor, 1);

	lua_pushnumber((float)pActor->pExtra->puck->getYawToPoint(target));
}

/* Changes the set that an actor is associated with,
 * by changing the set to "nil" an actor is disabled
 * but should still not be destroyed.
 */
void Lua_V1::PutActorInSet() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object setObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!lua_isstring(setObj) && !lua_isnil(setObj))
		return;

	// The demo only stores the name of the set
	if (g_grim->getGameFlags() & ADGF_DEMO) {
		if (lua_isstring(setObj))
			Common::strlcpy(pActor->curSetName, lua_getstring(setObj), sizeof(pActor->curSetName));
		else
			pActor->curSetName[0] = 0;

		return;
	}

	g_grim->_pSputRoom->addActor(lua_getstring(setObj), pActor);

	if (pActor->pExtra && lua_isstring(setObj) && pActor->pExtra->puck && pActor->pExtra->inBoxes) {
		if (g_grim->_pRes->loaded(kSputTagSet, lua_getstring(setObj)))
			pActor->pExtra->puck->validate();
	}
}

void Lua_V1::SetActorWalkRate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rateObj = lua_getparam(2);

	if (!isSputActor(actorObj) || !lua_isnumber(rateObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	pActor->pExtra->walkRate = lua_getnumber(rateObj);
}

void Lua_V1::GetActorWalkRate() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	lua_pushnumber(pActor->pExtra->walkRate);
}

void Lua_V1::SetActorTurnRate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rateObj = lua_getparam(2);

	if (!isSputActor(actorObj) || !lua_isnumber(rateObj))
		return;

	float rate = lua_getnumber(rateObj);
	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	// The demo stores the rate as it is
	if (!(pActor->pExtra->turnRate >= 0.0f) && !(g_grim->getGameFlags() & ADGF_DEMO))
		rate = rate * -1.0f;

	pActor->pExtra->turnRate = rate;
}

void Lua_V1::WalkActorForward() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "WalkActorForward"))
		return;

	pActor->mode &= ~1;
	g_grim->_pSputActor->forward(pActor);
}

void Lua_V1::SetActorReflection() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object angleObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	g_grim->_pSputActor->setReflection(pActor, lua_getnumber(angleObj));
}

void Lua_V1::GetActorPuckVector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object addObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	int bAddToPos = !lua_isnil(addObj);
	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	Puck *pPuck = pActor->pExtra->puck;

	if (!pPuck) {
		lua_pushnil();
		return;
	}

	float x = pPuck->orient[1][0];
	float y = pPuck->orient[1][1];
	float z = pPuck->orient[1][2];

	if (bAddToPos) {
		x = x + pPuck->pos[0];
		y = pPuck->pos[1] + y;
		z = pPuck->pos[2] + z;
	}

	lua_pushnumber(x);
	lua_pushnumber(y);
	lua_pushnumber(z);
}

void Lua_V1::ActorPuckOrient() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object orientObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (lua_isnil(orientObj) || !luaFtol(lua_getnumber(orientObj)))
		pActor->flags &= ~0x10;
	else
		pActor->flags |= 0x10;
}

void Lua_V1::WalkActorTo() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object goalObj = lua_getparam(5);
	RdVector3 pos;
	RdVector3 goal;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (lua_isnumber(xObj)) {
		pos[0] = lua_getnumber(xObj);
		pos[1] = lua_getnumber(yObj);
		pos[2] = lua_getnumber(zObj);
	} else if (isSputActor(xObj)) {
		SputActor *pDestActor = getSputActor(xObj);

		if (!pDestActor)
			return;

		pos[0] = pDestActor->actorPos[0];
		pos[1] = pDestActor->actorPos[1];
		pos[2] = pDestActor->actorPos[2];
	} else {
		return;
	}

	goal[0] = lua_getnumber(goalObj);
	goal[1] = lua_getnumber(goalObj);
	goal[2] = lua_getnumber(goalObj);

	if (!hasActorExtra(pActor, "WalkActorTo"))
		return;

	g_grim->_pSputActor->actorWalkTo(pActor, pos, goal, nullptr);
}

void Lua_V1::WalkActorToAvoiding() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object avoidObj = lua_getparam(2);
	lua_Object xObj = lua_getparam(3);
	lua_Object yObj = lua_getparam(4);
	lua_Object zObj = lua_getparam(5);
	lua_Object goalObj = lua_getparam(6);
	RdVector3 pos;
	RdVector3 goal;

	if (!isSputActor(actorObj) || !isSputActor(avoidObj))
		return;

	SputActor *pActor = getSputActor(actorObj);
	SputActor *pAvoidActor = getSputActor(avoidObj);

	if (!pActor || !pAvoidActor)
		return;

	if (lua_isnumber(xObj)) {
		pos[0] = lua_getnumber(xObj);
		pos[1] = lua_getnumber(yObj);
		pos[2] = lua_getnumber(zObj);
	} else if (isSputActor(xObj)) {
		SputActor *pDestActor = getSputActor(xObj);

		if (!pDestActor)
			return;

		pos[0] = pDestActor->actorPos[0];
		pos[1] = pDestActor->actorPos[1];
		pos[2] = pDestActor->actorPos[2];
	} else {
		return;
	}

	goal[0] = lua_getnumber(goalObj);
	goal[1] = lua_getnumber(goalObj);
	goal[2] = lua_getnumber(goalObj);

	if (!hasActorExtra(pActor, "WalkActorToAvoiding"))
		return;

	g_grim->_pSputActor->actorWalkTo(pActor, pos, goal, pAvoidActor);
}

/* This draw an actor to an offscreen buffer.
 * The actors' buffers blit to the global buffer which then gets blitted to screen.
 * This double layer system is necessary for set at, where Glottis and the Albinizod
 * are drawn to the clean buffer but at different times.
 *
 * If you change the clean system check with the sets dd, at and with the trail of flowers
 * in sh or zw.
 */
void Lua_V1::ActorToClean() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	g_grim->_pSputActor->drawToClean(pActor);
}

void Lua_V1::IsActorMoving() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	pushbool((pActor->mode & 1) != 0);
}

void Lua_V1::IsActorResting() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	pushbool(pActor->pExtra->resting != 0);
}

/* Get the location of one of the actor's nodes, this is
 * used by Glottis to watch where Manny is located in
 * order to hand him the work order.  This function is
 * also important for when Velasco hands Manny the logbook
 * in Rubacava. It is also used for calculating many positions
 * passed to ActorLookAt.
 */
void Lua_V1::GetActorNodeLocation() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nodeObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	if (!lua_isnumber(nodeObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	CostumeNode *pNode = pActor->pCostumeStack;

	if (!pNode || !pNode->pHeadThing)
		return;

	bool bDemo = (g_grim->getGameFlags() & ADGF_DEMO) != 0;

	// The demo uses the joint matrices of the last frame
	if (!pActor->onScreen && !bDemo)
		g_grim->_pRd->_puppet.buildJointMatrices(pNode->pHeadThing, g_grim->_pSputActor->getOrientMatrix(pActor));

	uint nodeNum = (uint)luaFtol(lua_getnumber(nodeObj));
	RdMatrix34 meshMatrix;

	if (!g_grim->_pRd->_model3.getMeshMatrix(pNode->pHeadThing, g_grim->_pSputActor->getOrientMatrix(pActor), nodeNum, meshMatrix)) {
		warning("GetActorNodeLocation(): invalid node %u", nodeNum);
		return;
	}

	// The demo returns the position of the mesh matrix as it is
	if (bDemo) {
		lua_pushnumber(meshMatrix[3][0]);
		lua_pushnumber(meshMatrix[3][1]);
		lua_pushnumber(meshMatrix[3][2]);
		return;
	}

	RdVector3 location;

	location[0] = pNode->pHeadThing->data.pModel3->aHNodes[nodeNum].pivot[0];
	location[1] = pNode->pHeadThing->data.pModel3->aHNodes[nodeNum].pivot[1];
	location[2] = pNode->pHeadThing->data.pModel3->aHNodes[nodeNum].pivot[2];
	g_grim->_pRd->transformVector34Acc(location, meshMatrix);

	location[0] = (float)((double)location[0] + meshMatrix[3][0]);
	location[1] = (float)((double)location[1] + meshMatrix[3][1]);
	location[2] = (float)((double)location[2] + meshMatrix[3][2]);

	lua_pushnumber(location[0]);
	lua_pushnumber(location[1]);
	lua_pushnumber(location[2]);
}

void Lua_V1::SetActorWalkDominate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object modeObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	int bDominate = lua_isnil(modeObj);
	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra) {
		warning("SetActorWalkDominate(): the actor has no extra data");
		return;
	}

	g_grim->_pSputActor->setWalkDominate(pActor, bDominate);
}

void Lua_V1::SetActorColormap() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nameObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pColorMap) {
		warning("SetActorColormap(): the actor has no colormap reference");
		return;
	}

	if (lua_isstring(nameObj)) {
		void *hColormap = g_grim->_pSputResource->alloc(kSputTagCmap, lua_getstring(nameObj));

		if (!pActor->pColorMap->valid || pActor->pColorMap->serial != (int)(intptr)hColormap) {
			g_grim->_pRes->removeReference(pActor->pColorMap);
			g_grim->_pRes->setReference(pActor->pColorMap, hColormap);
		}
	} else if (lua_isnil(nameObj)) {
		g_grim->_pRes->removeReference(pActor->pColorMap);
	}
}

void Lua_V1::TurnActor() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object dirObj = lua_getparam(2);

	if (!isSputActor(actorObj) || !lua_isnumber(dirObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "TurnActor"))
		return;

	pActor->mode &= ~1;
	g_grim->_pSputActor->turn(pActor, lua_getnumber(dirObj));
}

void Lua_V1::PushActorCostume() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nameObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (lua_isstring(nameObj))
		g_grim->_pSputActor->pushCostume(pActor, lua_getstring(nameObj), 0);
}

void Lua_V1::SetActorCostume() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object costumeObj = lua_getparam(2);

	int result = 0;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (lua_isstring(costumeObj)) {
		result = g_grim->_pSputActor->setCostume(pActor, lua_getstring(costumeObj), 0);
	} else if (lua_isnil(costumeObj)) {
		while (pActor->pCostumeStack)
			g_grim->_pSputActor->popCostume(pActor, 1);

		result = 1;
	}

	if (result)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V1::GetActorCostume() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object costumeObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	CostumeNode *pNode = pActor->pCostumeStack;

	if (!lua_isnil(costumeObj)) {
		if (!lua_isnumber(costumeObj))
			return;

		int depth = luaFtol(lua_getnumber(costumeObj));

		while (pNode && depth > 0) {
			depth--;
			pNode = pNode->pNext;
		}
	}

	if (pNode) {
		CosCostume *pCostume = g_grim->_pSputCostume->getCos()->instGetCostume(pNode->hCostumeInstance, nullptr);

		if (pCostume) {
			lua_pushstring(pCostume->name);
			return;
		}
	}

	lua_pushnil();
}

void Lua_V1::PopActorCostume() {
	lua_Object actorObj = lua_getparam(1);
	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	CosCostume *pCostume = nullptr;

	if (pActor->pCostumeStack)
		pCostume = g_grim->_pSputCostume->getCos()->instGetCostume(pActor->pCostumeStack->hCostumeInstance, nullptr);

	if (pCostume)
		lua_pushstring(pCostume->name);
	else
		lua_pushnil();

	g_grim->_pSputActor->popCostume(pActor, 1);
}

void Lua_V1::GetActorCostumeDepth() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	lua_pushnumber(g_grim->_pSputActor->countCostumes(pActor));
}

void Lua_V1::PrintActorCostumes() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	g_grim->_pSputCostume->printCostumeInfo(pActor->pCostumeStack);
}

void Lua_V1::LoadCostume() {
	lua_Object nameObj = lua_getparam(1);

	if (lua_isstring(nameObj))
		g_grim->_pSputResource->alloc(kSputTagCost, lua_getstring(nameObj));
	else
		lua_pushnil();
}

void Lua_V1::PlayActorChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);

	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (pNode && lua_isnumber(choreObj) && g_grim->_pSputCostume->getCos()->instPlayChore(pNode->hCostumeInstance, luaFtol(lua_getnumber(choreObj)), 0, 0))
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V1::CompleteActorChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);

	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (pNode && lua_isnumber(choreObj) && g_grim->_pSputCostume->getCos()->instCompleteChore(pNode->hCostumeInstance, luaFtol(lua_getnumber(choreObj))))
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V1::PlayActorChoreLooping() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);

	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (pNode && lua_isnumber(choreObj) && g_grim->_pSputCostume->getCos()->instPlayChore(pNode->hCostumeInstance, luaFtol(lua_getnumber(choreObj)), 1, 0))
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V1::SetActorChoreLooping() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object loopObj = lua_getparam(3);
	lua_Object costumeObj = lua_getparam(4);
	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode) || !pNode)
		return;

	Cos *pCos = g_grim->_pSputCostume->getCos();

	if (lua_isnumber(choreObj)) {
		pCos->instSetChoreLoop(pNode->hCostumeInstance, luaFtol(lua_getnumber(choreObj)), !lua_isnil(loopObj));
	} else if (lua_isnil(choreObj)) {
		int numChores = pCos->instCountChores(pNode->hCostumeInstance);

		for (int i = 0; i < numChores; i++)
			pCos->instSetChoreLoop(pNode->hCostumeInstance, i, !lua_isnil(loopObj));
	}
}

void Lua_V1::StopActorChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object costumeObj = lua_getparam(3);

	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode) || !pNode)
		return;

	if (lua_isnumber(choreObj))
		g_grim->_pSputCostume->getCos()->instStopChore(pNode->hCostumeInstance, luaFtol(lua_getnumber(choreObj)));
	else if (lua_isnil(choreObj))
		g_grim->_pSputCostume->getCos()->instStopChores(pNode->hCostumeInstance);
}

void Lua_V1::FadeOutChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object costumeObj = lua_getparam(2);
	lua_Object choreObj = lua_getparam(3);
	lua_Object timeObj = lua_getparam(4);

	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	int choreIndex = luaFtol(lua_getnumber(choreObj));
	int timeMs = luaFtol(lua_getnumber(timeObj));

	if ((lua_isstring(costumeObj) || lua_isnumber(costumeObj)) && findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		g_grim->_pSputCostume->fadeKeysOut(pNode->hCostumeInstance, choreIndex, timeMs);
}

void Lua_V1::FadeInChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object costumeObj = lua_getparam(2);
	lua_Object choreObj = lua_getparam(3);
	lua_Object timeObj = lua_getparam(4);

	CostumeNode *pNode;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	int choreIndex = luaFtol(lua_getnumber(choreObj));
	int timeMs = luaFtol(lua_getnumber(timeObj));

	if ((lua_isstring(costumeObj) || lua_isnumber(costumeObj)) && findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode)) {
		g_grim->_pSputCostume->getCos()->instPlayChore(pNode->hCostumeInstance, choreIndex, 0, 0);
		g_grim->_pSputCostume->getCos()->instUpdateChore(pNode->hCostumeInstance, choreIndex, 100);
		g_grim->_pSputCostume->fadeKeysIn(pNode->hCostumeInstance, choreIndex, timeMs);
	}
}

void Lua_V1::IsActorChoring() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object ignoreLoopingObj = lua_getparam(3);
	lua_Object costumeObj = lua_getparam(4);
	CostumeNode *pNode;
	int foundChore = -1;

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	bool ignoreLooping = !lua_isnil(ignoreLoopingObj);

	if (!pNode) {
		lua_pushnil();
		return;
	}

	Cos *pCos = g_grim->_pSputCostume->getCos();

	if (lua_isnumber(choreObj)) {
		int status = pCos->instChoreStatus(pNode->hCostumeInstance, luaFtol(lua_getnumber(choreObj)));

		if (!(status & kCosChoreStatePlaying)) {
			lua_pushnil();
		} else if (!ignoreLooping || !(status & kCosChoreStateLooping)) {
			lua_pushobject(choreObj);

			if (status & kCosChoreStateLooping)
				lua_pushnumber(1.0f);
			else
				lua_pushnil();
		}

		return;
	}

	if (lua_isnil(choreObj)) {
		int numChores = pCos->instCountChores(pNode->hCostumeInstance);

		for (int i = 0; i < numChores; i++) {
			int status = pCos->instChoreStatus(pNode->hCostumeInstance, i);

			if ((status & kCosChoreStatePlaying) && (!ignoreLooping || !(status & kCosChoreStateLooping))) {
				foundChore = i;

				if (status & kCosChoreStateLooping) {
					lua_pushnumber((float)i);
					lua_pushnumber(1.0f);
					return;
				}
			}
		}

		if (foundChore >= 0)
			lua_pushnumber((float)foundChore);
		else
			lua_pushnil();

		return;
	}

	lua_pushnil();
}

void Lua_V1::GetActorChores() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object costumeObj = lua_getparam(2);
	CostumeNode *pNode;

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor) {
		lua_pushnil();
		return;
	}

	if (!findCostumeInStack(costumeObj, pActor->pCostumeStack, &pNode))
		return;

	if (!pNode) {
		lua_pushnil();
		return;
	}

	Cos *pCos = g_grim->_pSputCostume->getCos();
	lua_Object choreTable = lua_createtable();
	int numChores = pCos->instCountChores(pNode->hCostumeInstance);
	int playingCount = 0;

	for (int i = 0; i < numChores; i++) {
		if (pCos->instChoreStatus(pNode->hCostumeInstance, i) & kCosChoreStatePlaying) {
			lua_pushobject(choreTable);
			lua_pushnumber((float)i);
			lua_pushnumber((float)i);
			lua_settable();
			playingCount++;
		}
	}

	lua_pushobject(choreTable);
	lua_pushstring("count");
	lua_pushnumber((float)playingCount);
	lua_settable();

	lua_pushobject(choreTable);
}

void Lua_V1::ActorLookAt() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object rateObj = lua_getparam(5);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	if (!pActor->pCostumeStack)
		return;

	Extra *pExtra = pActor->pExtra;

	if (lua_isnumber(rateObj))
		pActor->pCostumeStack->headTurnRate = lua_getnumber(rateObj);

	if (lua_isnil(xObj)) {
		if (pExtra->lookCarrot[0] != 0.0f || pExtra->lookCarrot[1] != 0.0f || pExtra->lookCarrot[2] != 0.0f) {
			pExtra->lookCarrot[0] = 0.0f;
			pExtra->lookCarrot[1] = 0.0f;
			pExtra->lookCarrot[2] = 0.0f;

			if (lua_isnumber(yObj))
				pActor->pCostumeStack->headTurnRate = lua_getnumber(yObj);

			pActor->mode |= 2;
		}
	} else if (lua_isnumber(xObj)) {
		pExtra->lookCarrot[0] = lua_getnumber(xObj);

		if (lua_isnumber(yObj))
			pExtra->lookCarrot[1] = lua_getnumber(yObj);
		else
			pExtra->lookCarrot[1] = 0.0f;

		if (lua_isnumber(zObj))
			pExtra->lookCarrot[2] = lua_getnumber(zObj);
		else
			pExtra->lookCarrot[2] = 0.0f;

		pActor->mode |= 2;
	} else if (isSputActor(xObj)) {
		SputActor *pLookedActor = getSputActor(xObj);

		if (!pLookedActor)
			return;

		pExtra->lookCarrot[0] = pLookedActor->actorPos[0];
		pExtra->lookCarrot[1] = pLookedActor->actorPos[1];
		pExtra->lookCarrot[2] = pLookedActor->actorPos[2];

		if (lua_isnumber(yObj))
			pActor->pCostumeStack->headTurnRate = lua_getnumber(yObj);

		pActor->mode |= 2;
	}
}

/* Turn the actor to a point specified in the 3D space,
 * this should not have the actor look toward the point
 * but should rotate the entire actor toward it.
 * It does not however make the actor slowly turn the next
 * times update() is called on it. It does make it turn
 * slightly, by the amount given by its turn rate, and its
 * effect is immediate.
 */
void Lua_V1::TurnActorTo() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	RdVector3 target;

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "TurnActorTo")) {
		lua_pushnil();
		return;
	}

	if (isSputActor(xObj)) {
		SputActor *pTargetActor = getSputActor(xObj);

		if (!pTargetActor) {
			lua_pushnil();
			return;
		}

		target[0] = pTargetActor->actorPos[0];
		target[1] = pTargetActor->actorPos[1];
		target[2] = pTargetActor->actorPos[2];
	} else {
		target[0] = lua_getnumber(xObj);
		target[1] = lua_getnumber(yObj);
		target[2] = lua_getnumber(zObj);
	}

	if (!pActor->pExtra->puck)
		g_grim->_pSputActor->enablePuck(pActor, 1);

	float turnRate = pActor->pExtra->turnRate;

	if (!(turnRate >= 0.0f))
		turnRate = -turnRate;

	float maxAngle = (float)((double)g_grim->_pSputTime->getFrameTimeFlex() * turnRate);
	double turned = pActor->pExtra->puck->pointAt(target, maxAngle);

	if (turned < 0.0)
		turned = -turned;

	if (turned >= maxAngle)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V1::PointActorAt() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	RdVector3 target;

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "PointActorAt"))
		return;

	if (isSputActor(xObj)) {
		SputActor *pTargetActor = getSputActor(xObj);

		if (!pTargetActor)
			return;

		target[0] = pTargetActor->actorPos[0];
		target[1] = pTargetActor->actorPos[1];
		target[2] = pTargetActor->actorPos[2];
	} else {
		target[0] = lua_getnumber(xObj);
		target[1] = lua_getnumber(yObj);
		target[2] = lua_getnumber(zObj);
	}

	if (!pActor->pExtra->puck)
		g_grim->_pSputActor->enablePuck(pActor, 1);

	pActor->pExtra->puck->pointAt(target, 3.4028235e38f);

	if (pActor->pExtra->turnRate > 0.0f)
		pActor->pExtra->turnRate = pActor->pExtra->turnRate * -1.0f;
}

void Lua_V1::WalkActorVector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object refObj = lua_getparam(2);
	lua_Object xObj = lua_getparam(3);
	lua_Object yObj = lua_getparam(4);
	lua_Object zObj = lua_getparam(5);
	lua_Object adjustObj = lua_getparam(6);

	if (!isSputActor(actorObj) || !isSputActor(refObj))
		return;

	SputActor *pActor = getSputActor(actorObj);
	SputActor *pRefActor = getSputActor(refObj);

	if (!pActor || !pRefActor || !hasActorExtra(pActor, "WalkActorVector"))
		return;

	if (!pActor->pExtra->puck) {
		warning("WalkActorVector(): the actor has no puck");
		return;
	}

	RdVector3 target;
	RdMatrix34 refMatrix;

	target[0] = lua_getnumber(xObj);
	target[1] = lua_getnumber(yObj);
	target[2] = lua_getnumber(zObj);
	memcpy(refMatrix, g_grim->_pSputActor->getOrientMatrix(pRefActor), sizeof(RdMatrix34));

	if (lua_isnumber(adjustObj)) {
		RdVector3 adjust;
		RdVector3 savedPos;

		adjust[0] = 0.0f;
		adjust[1] = lua_getnumber(adjustObj);
		adjust[2] = 0.0f;
		savedPos[0] = refMatrix[3][0];
		savedPos[1] = refMatrix[3][1];
		savedPos[2] = refMatrix[3][2];
		refMatrix[3][0] = 0.0f;
		refMatrix[3][1] = 0.0f;
		refMatrix[3][2] = 0.0f;
		g_grim->_pRd->preRotate34(refMatrix, adjust);
		refMatrix[3][0] = savedPos[0];
		refMatrix[3][1] = savedPos[1];
		refMatrix[3][2] = savedPos[2];
	}

	g_grim->_pRd->transformVector34Acc(target, refMatrix);

	for (int i = 0; i < 3; i++)
		target[i] = (float)((double)pActor->actorPos[i] + target[i]);

	RdVector3 localTarget;

	localTarget[0] = target[0];
	localTarget[1] = target[1];
	localTarget[2] = target[2];
	pActor->pExtra->puck->invXform(localTarget);
	g_grim->_pRd->normalize3Acc(localTarget);

	float turnAngle = (float)(90.0f - (double)g_grim->_pStdMath->arcSin3(localTarget[1]));
	float turnRate = pActor->pExtra->turnRate;

	if (!(turnRate >= 0.0f))
		turnRate = -turnRate;

	double turnStep = (double)g_grim->_pSputTime->getFrameTimeFlex() * turnRate;

	pActor->pExtra->puck->pointAt(target, (float)(turnStep + turnStep));

	if (!(turnAngle >= 90.0f)) {
		pActor->mode &= ~1;
		g_grim->_pSputActor->forward(pActor);
	}
}

/* Set the pitch of the actor to the requested value,
 * this will rotate an actor toward/away from the ground.
 * This is used when Glottis runs over the signpost in
 * the Petrified Forest
 */
void Lua_V1::SetActorPitch() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object pitchObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	pActor->actorRot[0] = lua_getnumber(pitchObj);
	pActor->matrixValid = 0;
}

void Lua_V1::SetActorRoll() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rollObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	pActor->actorRot[2] = lua_getnumber(rollObj);
	pActor->matrixValid = 0;
}

void Lua_V1::SetActorLookRate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rateObj = lua_getparam(2);

	if (!isSputActor(actorObj) || !lua_isnumber(rateObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (pActor->pCostumeStack)
		pActor->pCostumeStack->headTurnRate = lua_getnumber(rateObj);
}

void Lua_V1::GetActorLookRate() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (pActor && pActor->pCostumeStack)
		lua_pushnumber(pActor->pCostumeStack->headTurnRate);
	else
		lua_pushnil();
}

void Lua_V1::SetActorHead() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object joint1Obj = lua_getparam(2);
	lua_Object joint2Obj = lua_getparam(3);
	lua_Object joint3Obj = lua_getparam(4);
	lua_Object maxRollObj = lua_getparam(5);
	lua_Object maxPitchObj = lua_getparam(6);
	lua_Object maxYawObj = lua_getparam(7);

	if (!isSputActor(actorObj))
		return;
	if (!lua_isnumber(joint1Obj) || !lua_isnumber(joint2Obj) || !lua_isnumber(joint3Obj) ||
			!lua_isnumber(maxRollObj) || !lua_isnumber(maxPitchObj) || !lua_isnumber(maxYawObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	float maxYaw = lua_getnumber(maxYawObj);
	float maxPitch = lua_getnumber(maxPitchObj);
	float maxRoll = lua_getnumber(maxRollObj);
	uint joint3 = (uint)luaFtol(lua_getnumber(joint3Obj));
	uint joint2 = (uint)luaFtol(lua_getnumber(joint2Obj));
	uint joint1 = (uint)luaFtol(lua_getnumber(joint1Obj));

	g_grim->_pSputActor->setActorHead(pActor, joint1, joint2, joint3, maxRoll, maxPitch, maxYaw);
}

void Lua_V1::PutActorAtInterest() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	Room *pRoom = g_grim->_pSputRoom->getCurrentRoom();

	if (pRoom) {
		RdVector3 interest;
		interest[0] = pRoom->pSetups[pRoom->curSetup].camera.interest[0];
		interest[1] = pRoom->pSetups[pRoom->curSetup].camera.interest[1];
		interest[2] = pRoom->pSetups[pRoom->curSetup].camera.interest[2];
		g_grim->_pSputActor->setActorPosition(pActor, interest);
	}
}

void Lua_V1::SetActorFollowBoxes() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object followObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	int bFollow = 1;

	if (lua_isnil(followObj))
		bFollow = 0;

	g_grim->_pSputActor->enableWalkSystem(pActor, bFollow);
}

void Lua_V1::SetActorConstrain() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object constrainObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	int bConstrain = !lua_isnil(constrainObj);
	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra) {
		warning("SetActorConstrain(): the actor has no extra data");
		return;
	}

	g_grim->_pSputActor->setActorConstrain(pActor, bConstrain);
}

void Lua_V1::GetVisibleThings() {
	lua_Object actorObj = lua_getparam(1);
	SputActor *pActor = nullptr;

	if (lua_isnil(actorObj)) {
		pActor = g_grim->_pSputActor->getSelectedActor();

		if (!pActor)
			return;
	} else if (isSputActor(actorObj)) {
		pActor = getSputActor(actorObj);
	}

	if (!pActor) {
		warning("GetVisibleThings(): the parameter is not an actor");
		return;
	}

	lua_Object result = lua_createtable();

	// The demo checks all the actors, not the ones linked to the set
	if (g_grim->getGameFlags() & ADGF_DEMO) {
		int numActors = g_grim->_pSputActor->getNumActors();

		for (int i = 0; i < numActors; i++) {
			SputActor *pTestActor = &g_grim->_pSputActor->getActors()[i];
			float distance;

			if (pTestActor != pActor && !(pActor->flags & 1) && g_grim->_pSputActor->canActorSeeActor(pActor, pTestActor, &distance)) {
				lua_pushobject(result);
				pushSputActor(g_grim->_pSputActor->getActorHandle(pTestActor));
				lua_pushnumber(1.0f);
				lua_settable();
			}
		}

		lua_pushobject(result);
		return;
	}

	if (!pActor->curSetName[0])
		lua_pushobject(result);

	RoomActorLink *pLink = g_grim->_pSputRoom->getRoomActors(pActor->curSetName);

	while (pLink) {
		SputActor *pTestActor = &g_grim->_pSputActor->getActors()[pLink->actorIndex];

		if (pTestActor != pActor && !(pActor->flags & 1)) {
			float distance;

			if (g_grim->_pSputActor->canActorSeeActor(pActor, pTestActor, &distance)) {
				lua_pushobject(result);
				pushSputActor(g_grim->_pSputActor->getActorHandle(pTestActor));
				lua_pushnumber(1.0f);
				lua_settable();
			}
		}

		pLink = pLink->pNext;
	}

	lua_pushobject(result);
}

void Lua_V1::SetShadowColor() {
	int r = luaFtol(lua_getnumber(lua_getparam(1)));
	int g = luaFtol(lua_getnumber(lua_getparam(2)));
	int b = luaFtol(lua_getnumber(lua_getparam(3)));

	g_grim->_pSputCostume->setShadowColor(r, g, b);
}

void Lua_V1::SetShadowColorDemo() {
	// The demo stores the color and never reads it
}

void Lua_V1::KillActorShadows() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	g_grim->_pSputActor->resetShadows(pActor);
}

void Lua_V1::SetActiveShadow() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object shadowIdObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	pActor->pExtra->activeShadow = luaFtol(lua_getnumber(shadowIdObj));
}

void Lua_V1::SetActorShadowPoint() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "SetActorShadowPoint"))
		return;

	float z = lua_getnumber(zObj);
	float y = lua_getnumber(yObj);
	float x = lua_getnumber(xObj);

	g_grim->_pSputActor->setShadowPoint(pActor, x, y, z);
}

void Lua_V1::SetActorShadowPlane() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nameObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "SetActorShadowPlane"))
		return;

	const char *name = lua_getstring(nameObj);

	if (!name) {
		warning("SetActorShadowPlane(): the plane name is not a string");
		return;
	}

	g_grim->_pSputActor->setShadowPlane(pActor, name);
}

void Lua_V1::AddShadowPlane() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nameObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	const char *name = lua_getstring(nameObj);

	if (!name) {
		warning("AddShadowPlane(): the plane name is not a string");
		return;
	}

	g_grim->_pSputActor->addShadowClipPlane(pActor, pActor->pExtra->activeShadow, name);
}

void Lua_V1::ActivateActorShadow() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object shadowIdObj = lua_getparam(2);
	lua_Object stateObj = lua_getparam(3);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor || !hasActorExtra(pActor, "ActivateActorShadow"))
		return;

	int bActive = !lua_isnil(stateObj);

	g_grim->_pSputActor->activateActorShadow(pActor, luaFtol(lua_getnumber(shadowIdObj)), bActive);
}

void Lua_V1::SetActorShadowValid() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object numObj = lua_getparam(2);

	if (!isSputActor(actorObj)) {
		lua_pushnil();
		return;
	}

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	g_grim->_pSputActor->setShadowValid(pActor, pActor->pExtra->activeShadow, luaFtol(lua_getnumber(numObj)));
}

void Lua_V1::GetActorRect() {
	lua_Object actorObj = lua_getparam(1);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (pActor && pActor->onScreen) {
		lua_pushnumber((float)pActor->drawnRect.x);
		lua_pushnumber((float)pActor->drawnRect.y);
		lua_pushnumber((float)pActor->drawnRect.width);
		lua_pushnumber((float)pActor->drawnRect.height);
	} else {
		lua_pushnil();
	}
}

void Lua_V1::SetActorInvClipNode() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nodeObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	int nodeId;

	if (lua_isnil(nodeObj))
		nodeId = -1;
	else
		nodeId = luaFtol(lua_getnumber(nodeObj));

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!pActor->pExtra)
		g_grim->_pSputActor->allocateActorExtra(pActor);

	pActor->pExtra->subClipNode = nodeId;
}

void Lua_V1::SetActorClipPlane() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object normalXObj = lua_getparam(2);
	lua_Object normalYObj = lua_getparam(3);
	lua_Object normalZObj = lua_getparam(4);
	lua_Object pointXObj = lua_getparam(5);
	lua_Object pointYObj = lua_getparam(6);
	lua_Object pointZObj = lua_getparam(7);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	if (!lua_isnumber(normalXObj) || !lua_isnumber(normalYObj) || !lua_isnumber(normalZObj) ||
	    !lua_isnumber(pointXObj) || !lua_isnumber(pointYObj) || !lua_isnumber(pointZObj))
		return;

	RdVector3 normal;
	RdVector3 point;

	normal[0] = lua_getnumber(normalXObj);
	normal[1] = lua_getnumber(normalYObj);
	normal[2] = lua_getnumber(normalZObj);
	g_grim->_pRd->normalize3Acc(normal);

	point[0] = lua_getnumber(pointXObj);
	point[1] = lua_getnumber(pointYObj);
	point[2] = lua_getnumber(pointZObj);

	g_grim->_pSputActor->setClipPlane(pActor, normal, point);
}

void Lua_V1::SetActorClipActive() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object stateObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	g_grim->_pSputActor->setClipPlaneActive(pActor, luaFtol(lua_getnumber(stateObj)));
}

void Lua_V1::SetActorFrustrumCull() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object stateObj = lua_getparam(2);

	if (!isSputActor(actorObj))
		return;

	SputActor *pActor = getSputActor(actorObj);

	if (!pActor)
		return;

	g_grim->_pSputActor->setFrustrumCull(pActor, luaFtol(lua_getnumber(stateObj)));
}

void Lua_V1::DriveActorTo() {
	// nothing to implement
	// dummy opcode in original
}

} // end of namespace Grim
