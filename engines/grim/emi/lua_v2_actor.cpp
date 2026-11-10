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

#include <math.h>

#include "common/endian.h"
#include "common/list.h"
#include "common/textconsole.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/cmatrix.h"
#include "engines/grim/emi/cnode.h"
#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/cvector.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/puck_v2.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/rprimitiveinstance.h"
#include "engines/grim/emi/walkplane_v2.h"
#include "engines/grim/emi/walkplanecollection.h"

namespace Grim {

static const float kWalkRateScale = 3.28f;
static const float kWalkRateInverseScale = 0.30487806f;
static const float kMinDirectionLength = 1e-20f;
static const float kFlatAngleLimit = 1.000001f;
static const int kForwardAngleLimit = 65;
static const float kMaxPointAtAngle = 3.4028235e38f;

bool Lua_V2::isActor(lua_Object object) {
	return lua_isuserdata(object) && lua_tag(object) == MKTAG('R', 'T', 'C', 'A');
}

Actor *Lua_V2::getActor(lua_Object object, const char *opcodeName) {
	int actorId = lua_getuserdata(object);
	Actor *actor = Actor::actorDeref(actorId);

	if (!actor)
		warning("Lua_V2::%s(): the actor %d does not exist", opcodeName, actorId);

	return actor;
}

void Lua_V2::LoadActor() {
	lua_Object nameObj = lua_getparam(1);

	if (!lua_isstring(nameObj))
		return;

	int actorId = Actor::allocateActor(lua_getstring(nameObj));

	if (actorId < 0)
		lua_pushnil();
	else
		lua_pushusertag(actorId, MKTAG('R', 'T', 'C', 'A'));
}

void Lua_V2::UnloadActor() {
	lua_Object actorObj = lua_getparam(1);

	if (isActor(actorObj))
		Actor::freeActor(lua_getuserdata(actorObj));
}

void Lua_V2::GetActorTimeScale() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "GetActorTimeScale");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	lua_pushnumber(actor->extra->timeScale);
}

void Lua_V2::SetActorTimeScale() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object scaleObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorTimeScale");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	float scale;

	if (lua_isnumber(scaleObj))
		scale = lua_getnumber(scaleObj);
	else
		scale = 1.0f;

	if (!(scale >= 0.0f))
		scale = -scale;

	actor->extra->timeScale = scale;
}

void Lua_V2::SetActorWalkDominate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object dominateObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	bool dominate = lua_isnil(dominateObj) != 0;
	Actor *actor = getActor(actorObj, "SetActorWalkDominate");

	if (!actor)
		return;

	actor->setWalkDominate(dominate);
}

void Lua_V2::SetSelectedActor() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	int previousId = Actor::setSelectedActor(lua_getuserdata(actorObj));

	if (previousId < 0)
		lua_pushnil();
	else
		lua_pushusertag(previousId, MKTAG('R', 'T', 'C', 'A'));
}

void Lua_V2::SetActorWalkChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object choreSetObj = lua_getparam(3);

	if (!isActor(actorObj) || (!lua_isstring(choreObj) && !lua_isnil(choreObj)) || (!lua_isstring(choreSetObj) && !lua_isnil(choreSetObj)))
		return;

	Actor *actor = getActor(actorObj, "SetActorWalkChore");

	if (!actor)
		return;

	LString choreSetName;
	LString choreName;

	if (lua_isstring(choreSetObj))
		choreSetName = lua_getstring(choreSetObj);

	if (lua_isstring(choreObj))
		choreName = lua_getstring(choreObj);

	actor->setWalkChore(choreName, choreSetName);
}

void Lua_V2::SetActorTurnChores() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object leftChoreObj = lua_getparam(2);
	lua_Object rightChoreObj = lua_getparam(3);
	lua_Object choreSetObj = lua_getparam(4);

	if (!isActor(actorObj) || (!lua_isstring(leftChoreObj) && !lua_isnil(leftChoreObj)) || (!lua_isstring(rightChoreObj) && !lua_isnil(rightChoreObj)) || (!lua_isstring(choreSetObj) && !lua_isnil(choreSetObj)))
		return;

	Actor *actor = getActor(actorObj, "SetActorTurnChores");

	if (!actor)
		return;

	LString choreSetName;
	LString rightChoreName;
	LString leftChoreName;

	if (lua_isstring(choreSetObj))
		choreSetName = lua_getstring(choreSetObj);

	if (lua_isstring(rightChoreObj))
		rightChoreName = lua_getstring(rightChoreObj);

	if (lua_isstring(leftChoreObj))
		leftChoreName = lua_getstring(leftChoreObj);

	actor->setTurnChores(leftChoreName, choreSetName, rightChoreName, choreSetName);
}

void Lua_V2::SetActorRestChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object choreSetObj = lua_getparam(3);

	if (!isActor(actorObj) || (!lua_isstring(choreObj) && !lua_isnil(choreObj)) || (!lua_isstring(choreSetObj) && !lua_isnil(choreSetObj)))
		return;

	Actor *actor = getActor(actorObj, "SetActorRestChore");

	if (!actor)
		return;

	LString choreSetName;
	LString choreName;

	if (lua_isstring(choreSetObj))
		choreSetName = lua_getstring(choreSetObj);

	if (lua_isstring(choreObj))
		choreName = lua_getstring(choreObj);

	actor->setRestChore(choreName, choreSetName);
}

void Lua_V2::SetActorMumblechore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object choreSetObj = lua_getparam(3);

	if (!isActor(actorObj) || (!lua_isstring(choreObj) && !lua_isnil(choreObj)) || (!lua_isstring(choreSetObj) && !lua_isnil(choreSetObj)))
		return;

	Actor *actor = getActor(actorObj, "SetActorMumblechore");

	if (!actor)
		return;

	LString choreSetName;
	LString choreName;

	if (lua_isstring(choreSetObj))
		choreSetName = lua_getstring(choreSetObj);

	if (lua_isstring(choreObj))
		choreName = lua_getstring(choreObj);

	actor->setMumbleChore(choreName, choreSetName);
}

void Lua_V2::SetActorTalkChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object indexObj = lua_getparam(2);
	lua_Object choreObj = lua_getparam(3);
	lua_Object choreSetObj = lua_getparam(4);

	if (!isActor(actorObj) || !lua_isnumber(indexObj) || (!lua_isstring(choreObj) && !lua_isnil(choreObj)) || (!lua_isstring(choreSetObj) && !lua_isnil(choreSetObj)))
		return;

	int index = luaFtol(lua_getnumber(indexObj));

	if ((unsigned int)index > 10)
		return;

	Actor *actor = getActor(actorObj, "SetActorTalkChore");

	if (!actor)
		return;

	LString choreSetName;
	LString choreName;

	if (lua_isstring(choreSetObj))
		choreSetName = lua_getstring(choreSetObj);

	if (lua_isstring(choreObj))
		choreName = lua_getstring(choreObj);

	actor->setTalkChore(index, choreName, choreSetName);
}

void Lua_V2::SetActorWalkRate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rateObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(rateObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorWalkRate");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	actor->extra->walkRate = (float)((double)lua_getnumber(rateObj) * (double)kWalkRateScale);
}

void Lua_V2::GetActorWalkRate() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "GetActorWalkRate");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	lua_pushnumber((float)((double)actor->extra->walkRate * (double)kWalkRateInverseScale));
}

void Lua_V2::SetActorTurnRate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rateObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(rateObj))
		return;

	float rate = lua_getnumber(rateObj);
	Actor *actor = getActor(actorObj, "SetActorTurnRate");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	if (!(rate >= 0.0f))
		rate = -rate;

	actor->extra->turnRate = rate;
}

void Lua_V2::GetActorTurnRate() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "GetActorTurnRate");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	lua_pushnumber(actor->extra->turnRate);
}

void Lua_V2::GetActorPos() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if ((actor->nodeFlags & CNode::kPositionValid) == 0)
		actor->computeLocalPositionFromWorld();

	lua_pushnumber(actor->pos.x);

	if ((actor->nodeFlags & CNode::kPositionValid) == 0)
		actor->computeLocalPositionFromWorld();

	lua_pushnumber(actor->pos.y);

	if ((actor->nodeFlags & CNode::kPositionValid) == 0)
		actor->computeLocalPositionFromWorld();

	lua_pushnumber(actor->pos.z);
}

void Lua_V2::GetActorWorldPos() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	lua_pushnumber(actor->world.pos.x);

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	lua_pushnumber(actor->world.pos.y);

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	lua_pushnumber(actor->world.pos.z);
}

void Lua_V2::GetActorPuckVector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object addObj = lua_getparam(2);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	bool addPosition = !lua_isnil(addObj);
	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor) {
		lua_pushnil();

		return;
	}

	if (!actor->extra)
		actor->allocateActorExtra();

	Puck_v2 *puck = actor->extra->puck;

	if (!puck) {
		lua_pushnil();

		return;
	}

	if ((puck->nodeFlags & CNode::kWorldValid) == 0)
		puck->updateWorldTransform();

	float vectorX = puck->world.rot.m[6];
	float vectorY = puck->world.rot.m[7];
	float vectorZ = puck->world.rot.m[8];
	float resultX = vectorX;
	float resultY = vectorY;
	float resultZ = vectorZ;

	if (addPosition) {
		if ((puck->nodeFlags & CNode::kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		resultX = vectorX + puck->pos.x;
		resultY = vectorY + puck->pos.y;
		resultZ = vectorZ + puck->pos.z;
	}

	lua_pushnumber(resultX);
	lua_pushnumber(resultY);
	lua_pushnumber(resultZ);
}

void Lua_V2::GetActorYawToPoint() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object pointObj = lua_getparam(2);
	float point[3];

	point[0] = 0.0f;
	point[1] = 0.0f;
	point[2] = 0.0f;

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));
	lua_Object xObj;
	lua_Object yObj;
	lua_Object zObj;

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

	point[0] = lua_getnumber(xObj);
	point[1] = lua_getnumber(yObj);
	point[2] = lua_getnumber(zObj);

	if (!actor) {
		warning("Lua_V2::GetActorYawToPoint(): the actor does not exist");

		return;
	}

	if (!actor->extra)
		actor->allocateActorExtra();

	if (!actor->extra->puck)
		actor->enablePuck(true);

	lua_pushnumber(actor->extra->puck->getYawToPoint(point));
}

void Lua_V2::SetActorReflection() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object angleObj = lua_getparam(2);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "SetActorReflection");

	if (!actor)
		return;

	actor->setReflection(lua_getnumber(angleObj));
}

void Lua_V2::PutActorInSet() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object roomObj = lua_getparam(2);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor) {
		lua_pushnil();

		return;
	}

	if (lua_isnil(roomObj)) {
		actor->putInSet(nullptr);
		lua_pushnil();

		return;
	}

	if (!lua_isstring(roomObj)) {
		lua_pushnil();

		return;
	}

	if (!actor->putInSet(lua_getstring(roomObj))) {
		lua_pushnil();

		return;
	}

	lua_pushnumber(1.0f);
}

void Lua_V2::PutActorAt() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	float position[3];

	position[0] = 0.0f;
	position[1] = 0.0f;
	position[2] = 0.0f;

	if (!isActor(actorObj) || !lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	Actor *actor = getActor(actorObj, "PutActorAt");

	if (!actor)
		return;

	position[0] = lua_getnumber(xObj);
	position[1] = lua_getnumber(yObj);
	position[2] = lua_getnumber(zObj);
	actor->setActorPosition(position);
}

void Lua_V2::PutActorInOverworld() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (actor)
		Overworld::getInstance()->putActor(actor);
}

void Lua_V2::RemoveActorFromOverworld() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (actor)
		Overworld::getInstance()->removeActor(actor);
}

void Lua_V2::ClearOverworld() {
	Overworld::getInstance()->clearActors();
}

void Lua_V2::ToggleOverworld() {
	lua_Object enableObj = lua_getparam(1);
	bool enable = false;

	if (!lua_isnil(enableObj))
		enable = luaFtol(lua_getnumber(enableObj)) != 0;

	Overworld::getInstance()->enabled = enable;
}

void Lua_V2::WalkActorVector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object yawObj = lua_getparam(5);
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom || !isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "WalkActorVector");

	if (!actor)
		return;

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj))
		return;

	float x = lua_getnumber(xObj);
	float y = lua_getnumber(yObj);
	float z = lua_getnumber(zObj);
	Camera *camera = currentRoom->getSetupCamera(-1);

	if (!camera) {
		warning("Lua_V2::WalkActorVector(): the room has no camera");

		return;
	}

	if ((camera->nodeFlags & CNode::kWorldValid) == 0)
		camera->updateWorldTransform();

	const float *cameraMatrix = camera->world.rot.m;

	if (lua_isnumber(yawObj)) {
		CVector eulerYaw;
		CMatrix yawMatrix;

		eulerYaw.x = 0.0f;
		eulerYaw.y = lua_getnumber(yawObj);
		eulerYaw.z = 0.0f;
		yawMatrix.setFromEuler(eulerYaw);

		double rotatedX = ((double)z * yawMatrix.m[6] + (double)y * yawMatrix.m[3]) + (double)x * yawMatrix.m[0];
		double rotatedY = ((double)z * yawMatrix.m[7] + (double)y * yawMatrix.m[4]) + (double)x * yawMatrix.m[1];
		double rotatedZ = ((double)z * yawMatrix.m[8] + (double)y * yawMatrix.m[5]) + (double)x * yawMatrix.m[2];

		x = (float)rotatedX;
		y = (float)rotatedY;
		z = (float)rotatedZ;
	}

	double cameraX = ((double)z * cameraMatrix[6] + (double)y * cameraMatrix[3]) + (double)x * cameraMatrix[0];
	double cameraY = ((double)z * cameraMatrix[7] + (double)y * cameraMatrix[4]) + (double)x * cameraMatrix[1];
	double cameraZ = ((double)z * cameraMatrix[8] + (double)y * cameraMatrix[5]) + (double)x * cameraMatrix[2];
	float storedY = (float)cameraY;
	float directionX = (float)cameraX;
	float directionY = storedY;
	float directionZ = (float)cameraZ;
	double length = sqrt((cameraZ * cameraZ + (double)storedY * storedY) + cameraX * cameraX);

	if (length >= (double)kMinDirectionLength) {
		double inverse = 1.0 / length;

		directionX = (float)(cameraX * inverse);
		directionY = (float)((double)storedY * inverse);
		directionZ = (float)(cameraZ * inverse);
	}

	if ((actor->nodeFlags & CNode::kEulerValid) == 0) {
		actor->computeLocalMatrixFromWorld();

		if ((actor->nodeFlags & CNode::kEulerValid) == 0)
			actor->computeLocalMatrixFromWorld();
	}

	double yawRadians = (double)Actor::kDegreesToRadians * actor->eulerDeg.y;
	float sinValue = (float)sin(yawRadians);
	// The disasm squares the y component here
	double alignment = (cos(yawRadians) * directionZ + (double)directionY * directionY) + (double)sinValue * directionX;
	float alignmentValue = (float)alignment;
	float turnAngle;

	if (!(alignment > 1.0) || !(alignmentValue < kFlatAngleLimit))
		turnAngle = (float)(acos((double)alignmentValue) * WalkPlane_v2::kRadiansToDegrees);
	else
		turnAngle = 0.0f;

	if (!actor->extra || !actor->extra->puck) {
		warning("Lua_V2::WalkActorVector(): the actor has no puck");

		return;
	}

	float turnRate = actor->extra->turnRate;

	if (!(turnRate >= 0.0f))
		turnRate = -turnRate;

	float turnStep = EmiEngine::getFrameTime() * turnRate;

	if ((actor->nodeFlags & CNode::kPositionValid) == 0)
		actor->computeLocalPositionFromWorld();

	float target[3];

	target[0] = directionX + actor->pos.x;
	target[1] = directionY + actor->pos.y;
	target[2] = directionZ + actor->pos.z;
	actor->extra->puck->pointAt(target, turnStep + turnStep);

	int angleValue = luaFtol(turnAngle);

	if (angleValue > -kForwardAngleLimit && angleValue < kForwardAngleLimit) {
		actor->mode &= ~1U;
		actor->forward();
	}
}

void Lua_V2::WalkActorForward() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "WalkActorForward");

	if (!actor)
		return;

	actor->mode &= ~1U;
	actor->forward();
}

void Lua_V2::ActorStopMoving() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "ActorStopMoving");

	if (!actor)
		return;

	actor->mode &= ~1U;

	if (!actor->extra || !actor->extra->puck)
		return;

	Puck_v2 *puck = actor->extra->puck;

	puck->reset();

	if ((actor->nodeFlags & CNode::kPositionValid) == 0)
		actor->computeLocalPositionFromWorld();

	puck->setLocalPosition(actor->pos);

	if ((actor->nodeFlags & CNode::kEulerValid) == 0)
		actor->computeLocalMatrixFromWorld();

	puck->eulerDeg = actor->eulerDeg;
	puck->nodeFlags = (puck->nodeFlags & ~CNode::kQuaternionValid) | CNode::kEulerValid | CNode::kLocalSet;
	puck->updateLocalState();

	if (actor->extra->inBoxes && actor->room.resource) {
		Room_v2 *lockedRoom = nullptr;
		WalkPlaneCollection *collection = nullptr;
		bool locked = actor->room.lock();

		if (locked)
			lockedRoom = actor->room.typed;

		if (lockedRoom)
			collection = lockedRoom->walkPlanes;
		else
			warning("Lua_V2::ActorStopMoving(): the room could not be locked");

		if (locked)
			actor->room.unlock();

		puck->validate(collection);
	}

	if ((puck->nodeFlags & CNode::kPositionValid) == 0)
		puck->computeLocalPositionFromWorld();

	actor->setLocalPosition(puck->pos);

	if ((puck->nodeFlags & CNode::kEulerValid) == 0)
		puck->computeLocalMatrixFromWorld();

	actor->eulerDeg = puck->eulerDeg;
	actor->nodeFlags = (actor->nodeFlags & ~CNode::kQuaternionValid) | CNode::kEulerValid | CNode::kLocalSet;
	actor->updateLocalState();
}

void Lua_V2::WalkActorTo() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object targetObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	lua_Object goalObj = lua_getparam(5);
	float target[3];
	float goal[3];

	target[0] = 0.0f;
	target[1] = 0.0f;
	target[2] = 0.0f;
	goal[0] = 0.0f;
	goal[1] = 0.0f;
	goal[2] = 0.0f;

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "WalkActorTo");

	if (!actor)
		return;

	if (lua_isnumber(targetObj)) {
		target[0] = lua_getnumber(targetObj);
		target[1] = lua_getnumber(yObj);
		target[2] = lua_getnumber(zObj);
	} else if (isActor(targetObj)) {
		Actor *targetActor = getActor(targetObj, "WalkActorTo");

		if (!targetActor)
			return;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[0] = targetActor->world.pos.x;
		target[1] = targetActor->world.pos.y;
		target[2] = targetActor->world.pos.z;
	} else {
		return;
	}

	// The disasm reads the same parameter for the three goal components
	goal[0] = lua_getnumber(goalObj);
	goal[1] = lua_getnumber(goalObj);
	goal[2] = lua_getnumber(goalObj);
	actor->actorWalkTo(target, goal, nullptr);
}

void Lua_V2::WalkActorToAvoiding() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object avoidedObj = lua_getparam(2);
	lua_Object targetObj = lua_getparam(3);
	lua_Object yObj = lua_getparam(4);
	lua_Object zObj = lua_getparam(5);
	lua_Object goalObj = lua_getparam(6);
	float target[3];
	float goal[3];

	target[0] = 0.0f;
	target[1] = 0.0f;
	target[2] = 0.0f;
	goal[0] = 0.0f;
	goal[1] = 0.0f;
	goal[2] = 0.0f;

	if (!isActor(actorObj) || !isActor(avoidedObj))
		return;

	Actor *actor = getActor(actorObj, "WalkActorToAvoiding");
	Actor *avoidedActor = Actor::actorDeref(lua_getuserdata(avoidedObj));

	if (!actor)
		return;

	if (lua_isnumber(targetObj)) {
		target[0] = lua_getnumber(targetObj);
		target[1] = lua_getnumber(yObj);
		target[2] = lua_getnumber(zObj);
	} else if (isActor(targetObj)) {
		Actor *targetActor = getActor(targetObj, "WalkActorToAvoiding");

		if (!targetActor)
			return;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[0] = targetActor->world.pos.x;
		target[1] = targetActor->world.pos.y;
		target[2] = targetActor->world.pos.z;
	} else {
		return;
	}

	// The disasm reads the same parameter for the three goal components
	goal[0] = lua_getnumber(goalObj);
	goal[1] = lua_getnumber(goalObj);
	goal[2] = lua_getnumber(goalObj);
	actor->actorWalkTo(target, goal, avoidedActor);
}

void Lua_V2::ActorLookAt() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object targetObj = lua_getparam(2);
	lua_Object secondObj = lua_getparam(3);
	lua_Object rateObj = lua_getparam(5);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "ActorLookAt");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	if (lua_isnumber(rateObj))
		actor->extra->lookRate = (float)((double)lua_getnumber(rateObj) * (double)Actor::kDegreesToRadians);

	if (lua_isnil(targetObj)) {
		actor->extra->lookTarget.x = 0.0f;
		actor->extra->lookTarget.y = 0.0f;
		actor->extra->lookTarget.z = 0.0f;
		actor->extra->lookTargetActorId = 0;

		if (lua_isnumber(secondObj))
			actor->extra->lookRate = (float)((double)lua_getnumber(secondObj) * (double)Actor::kDegreesToRadians);

		actor->extra->lookAtHasTarget = false;

		return;
	}

	if (lua_isnumber(targetObj)) {
		lua_Object yObj = lua_getparam(3);
		lua_Object zObj = lua_getparam(4);
		float y;
		float z;

		actor->extra->lookTargetActorId = 0;

		if (lua_isnumber(zObj))
			z = lua_getnumber(zObj);
		else
			z = 0.0f;

		if (lua_isnumber(yObj))
			y = lua_getnumber(yObj);
		else
			y = 0.0f;

		actor->extra->lookTarget.x = lua_getnumber(targetObj);
		actor->extra->lookTarget.y = y;
		actor->extra->lookTarget.z = z;

		if (lua_isnumber(rateObj))
			actor->extra->lookRate = (float)((double)lua_getnumber(rateObj) * (double)Actor::kDegreesToRadians);
	} else {
		if (!isActor(targetObj))
			return;

		actor->extra->lookTargetActorId = lua_getuserdata(targetObj);
		actor->extra->lookTarget.x = 0.0f;
		actor->extra->lookTarget.y = 0.0f;
		actor->extra->lookTarget.z = 0.0f;

		if (lua_isnumber(secondObj))
			actor->extra->lookRate = (float)((double)lua_getnumber(secondObj) * (double)Actor::kDegreesToRadians);
	}

	actor->extra->lookAtHasTarget = true;
}

void Lua_V2::IsActorLooking() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "IsActorLooking");

	if (!actor)
		return;

	if (actor->extra && actor->extra->lookAtHasTarget)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::SetActorLookRate() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rateObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(rateObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorLookRate");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	actor->extra->lookRate = (float)((double)lua_getnumber(rateObj) * (double)Actor::kDegreesToRadians);
}

void Lua_V2::GetActorLookRate() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "GetActorLookRate");

	if (!actor || !actor->extra) {
		lua_pushnil();

		return;
	}

	lua_pushnumber(WalkPlane_v2::kRadiansToDegrees * actor->extra->lookRate);
}

void Lua_V2::SetActorFOV() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object fovObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(fovObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorFOV");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	actor->extra->lookCosFov = (float)cos(((double)lua_getnumber(fovObj) * (double)Actor::kDegreesToRadians) * 0.5);
}

void Lua_V2::GetActorFOV() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "GetActorFOV");

	if (!actor || !actor->extra) {
		lua_pushnil();

		return;
	}

	lua_pushnumber((float)((acos((double)actor->extra->lookCosFov) * (double)WalkPlane_v2::kRadiansToDegrees) * 2.0));
}

void Lua_V2::GetVisibleThings() {
	lua_Object actorObj = lua_getparam(1);
	Actor *viewer;

	if (lua_isnil(actorObj)) {
		viewer = Actor::getSelectedActor();

		if (!viewer)
			return;
	} else {
		if (!isActor(actorObj))
			return;

		viewer = getActor(actorObj, "GetVisibleThings");

		if (!viewer)
			return;
	}

	lua_Object table = lua_createtable();

	if (!viewer->room.resource) {
		lua_pushobject(table);

		return;
	}

	if (viewer->room.lock()) {
		Room_v2 *currentRoom = viewer->room.typed;

		if (currentRoom) {
			Common::List<Actor *>::iterator it;

			for (it = currentRoom->actors.begin(); it != currentRoom->actors.end(); ++it) {
				Actor *other = *it;
				float distance;

				if (other && other != viewer && (viewer->actorFlags & 1) == 0 && viewer->canActorSeeActor(other, &distance)) {
					lua_pushobject(table);
					lua_pushusertag(Actor::getActorHandle(other), MKTAG('R', 'T', 'C', 'A'));
					lua_pushnumber(1.0f);
					lua_settable();
				}
			}
		}
	}

	lua_pushobject(table);
	viewer->room.unlock();
}

void Lua_V2::SetActorLighting() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object modeObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if (lua_isnil(modeObj) || !lua_isnumber(modeObj))
		return;

	int mode = luaFtol(lua_getnumber(modeObj));

	if (mode == 0)
		actor->drawFlags &= 0xFFFBEFFFU;
	else if (mode == 1)
		actor->drawFlags = (actor->drawFlags & 0xFFFBFFFFU) | 0x21000U;
	else if (mode == 2)
		actor->drawFlags = (actor->drawFlags & 0xFFF9FFFFU) | 0x1000U;
	else
		actor->drawFlags = (actor->drawFlags & 0xFFFDEFFFU) | 0x40000U;
}

void Lua_V2::SetActorHead() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object jointObj = lua_getparam(2);
	lua_Object xObj = lua_getparam(3);
	lua_Object yObj = lua_getparam(4);
	lua_Object zObj = lua_getparam(5);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	float offset[3];

	offset[0] = 0.0f;
	offset[1] = 0.0f;
	offset[2] = 0.0f;

	if (lua_isnil(jointObj)) {
		actor->setActorHead(LString(), offset);
	} else if (lua_isstring(jointObj)) {
		LString jointName = lua_getstring(jointObj);

		if (!lua_isnil(xObj) && lua_isnumber(xObj))
			offset[0] = lua_getnumber(xObj);

		if (!lua_isnil(yObj) && lua_isnumber(yObj))
			offset[1] = lua_getnumber(yObj);

		if (!lua_isnil(zObj) && lua_isnumber(zObj))
			offset[2] = lua_getnumber(zObj);

		actor->setActorHead(jointName, offset);
	}
}

void Lua_V2::SetActorHeadLimits() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object horizontalObj = lua_getparam(2);
	lua_Object upObj = lua_getparam(3);
	lua_Object downObj = lua_getparam(4);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if (lua_isnil(horizontalObj) || !lua_isnumber(horizontalObj) || lua_isnil(upObj) || !lua_isnumber(upObj) || lua_isnil(downObj) || !lua_isnumber(downObj))
		return;

	float downLimit = (float)((double)lua_getnumber(downObj) * (double)Actor::kDegreesToRadians);
	float upLimit = (float)((double)lua_getnumber(upObj) * (double)Actor::kDegreesToRadians);
	float horizontalLimit = (float)((double)lua_getnumber(horizontalObj) * (double)Actor::kDegreesToRadians);

	actor->setHeadLimits(horizontalLimit, upLimit, downLimit);
}

void Lua_V2::SetActorVisibility() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object visibleObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorVisibility");

	if (!actor)
		return;

	if (!visibleObj || lua_isnil(visibleObj))
		actor->actorFlags |= 2U;
	else
		actor->actorFlags &= ~2U;
}

void Lua_V2::SetActorFollowBoxes() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object enableObj = lua_getparam(2);
	bool enable = true;

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorFollowBoxes");

	if (!actor)
		return;

	if (!enableObj || lua_isnil(enableObj))
		enable = false;

	actor->enableWalkSystem(enable);
}

void Lua_V2::ActorActivateShadow() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object activeObj = lua_getparam(2);
	lua_Object shadowObj = lua_getparam(3);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	bool active = luaFtol(lua_getnumber(activeObj)) != 0;
	Shadow_v2 *shadow = nullptr;

	if (lua_isstring(shadowObj)) {
		LString shadowName = lua_getstring(shadowObj);
		Room_v2 *currentRoom = Room_v2::getCurrentRoom();

		if (!currentRoom) {
			warning("Lua_V2::ActorActivateShadow(): there is no current room");

			return;
		}

		shadow = currentRoom->findShadow(shadowName);
	}

	actor->activateShadow(active, shadow);
}

void Lua_V2::ShutUpActor() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (actor)
		actor->shutUp(true);
}

void Lua_V2::EnableActorPuck() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object enableObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (actor)
		actor->enablePuck(luaFtol(lua_getnumber(enableObj)) != 0);
}

void Lua_V2::SetActorGlobalAlpha() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object alphaObj = lua_getparam(2);
	lua_Object nameObj = lua_getparam(3);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if (actor->instances.empty()) {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetActorGlobalAlpha: Actor isn't wearing any primitives!\n");
		else
			warning("SetActorGlobalAlpha: Actor isn't wearing any primitives!");

		return;
	}

	if (!lua_isnumber(alphaObj))
		return;

	float alpha = lua_getnumber(alphaObj);
	RPrimitiveInstance *instance = actor->instances[0];

	if (!lua_isnil(nameObj) && lua_isstring(nameObj)) {
		for (uint i = 0; i < actor->instances.size(); i++) {
			instance = actor->instances[i];

			if (instance) {
				if (!instance->prim) {
					warning("Lua_V2::SetActorGlobalAlpha(): the primitive instance has no primitive");

					continue;
				}

				if (instance->prim->name == lua_getstring(nameObj))
					break;
			}
		}
	}

	if (!instance)
		return;

	if (alpha == -1.0f)
		instance->globalAlphaMode = RPrimitiveInstance::kAlphaOff;
	else if (alpha == 2.0f)
		instance->globalAlphaMode = RPrimitiveInstance::kAlphaSet;
	else if (alpha == 3.0f)
		instance->globalAlphaMode = RPrimitiveInstance::kAlphaModulate;
	else
		instance->globalAlpha = alpha;
}

void Lua_V2::SetActorLocalAlpha() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object indexObj = lua_getparam(2);
	lua_Object alphaObj = lua_getparam(3);
	lua_Object nameObj = lua_getparam(4);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if (actor->instances.empty()) {
		if (gpConsoleServices)
			gpConsoleServices->logError("SetActorLocalAlpha: Actor isn't wearing any primitives!\n");
		else
			warning("SetActorLocalAlpha: Actor isn't wearing any primitives!");

		return;
	}

	if (!lua_isnumber(indexObj))
		return;

	int index = luaFtol(lua_getnumber(indexObj));

	if (!lua_isnumber(alphaObj))
		return;

	float alpha = lua_getnumber(alphaObj);
	RPrimitiveInstance *instance = actor->instances[0];

	if (!lua_isnil(nameObj) && lua_isstring(nameObj)) {
		for (uint i = 0; i < actor->instances.size(); i++) {
			instance = actor->instances[i];

			if (instance) {
				if (!instance->prim) {
					warning("Lua_V2::SetActorLocalAlpha(): the primitive instance has no primitive");

					continue;
				}

				if (instance->prim->name == lua_getstring(nameObj))
					break;
			}
		}
	}

	if (!instance)
		return;

	if (alpha == -1.0f)
		instance->matAlphaMode = RPrimitiveInstance::kAlphaOff;
	else if (alpha == 2.0f)
		instance->matAlphaMode = RPrimitiveInstance::kAlphaSet;
	else if (alpha == 3.0f)
		instance->matAlphaMode = RPrimitiveInstance::kAlphaModulate;
	else
		instance->matAlpha[index] = alpha;
}

void Lua_V2::SetActorCollisionMode() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object modeObj = lua_getparam(2);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "SetActorCollisionMode");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	actor->extra->collisionMode = luaFtol(lua_getnumber(modeObj));
}

void Lua_V2::SetActorCollisionScale() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object scaleObj = lua_getparam(2);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "SetActorCollisionScale");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	actor->extra->collisionScale = lua_getnumber(scaleObj);
}

void Lua_V2::SetActorSortOrder() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object sortOrderObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(sortOrderObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorSortOrder");

	if (!actor)
		return;

	actor->sortOrder = luaFtol(lua_getnumber(sortOrderObj));
}

void Lua_V2::GetActorSortOrder() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "GetActorSortOrder");
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!actor)
		return;

	if (!currentRoom) {
		warning("Lua_V2::GetActorSortOrder(): there is no current room");

		return;
	}

	lua_pushnumber((float)actor->getSortOrder(currentRoom->currentSetup));
}

void Lua_V2::AttachActor() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object parentObj = lua_getparam(2);
	lua_Object nodeObj = lua_getparam(3);

	if (!isActor(actorObj) || !isActor(parentObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));
	Actor *parent = Actor::actorDeref(lua_getuserdata(parentObj));

	if (!actor || !parent)
		return;

	CNode *node = parent;

	if (lua_isstring(nodeObj)) {
		node = parent->findNode(lua_getstring(nodeObj));

		if (!node)
			node = parent;
	}

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	CVector position = actor->world.pos;

	node->worldToLocal(&position);
	actor->setActorPosition(&position.x);

	CQuaternion actorOrientation = actor->getWorldOrientation();
	CQuaternion nodeOrientation = node->getWorldOrientation();

	nodeOrientation.invert();

	double resultW = (((double)actorOrientation.w * nodeOrientation.w - (double)nodeOrientation.x * actorOrientation.x) - (double)actorOrientation.y * nodeOrientation.y) - (double)actorOrientation.z * nodeOrientation.z;
	double resultZ = (((double)actorOrientation.w * nodeOrientation.z + (double)actorOrientation.z * nodeOrientation.w) + (double)nodeOrientation.y * actorOrientation.x) - (double)actorOrientation.y * nodeOrientation.x;
	double resultY = (((double)actorOrientation.w * nodeOrientation.y + (double)actorOrientation.y * nodeOrientation.w) + (double)actorOrientation.z * nodeOrientation.x) - (double)nodeOrientation.z * actorOrientation.x;
	double resultX = (((double)actorOrientation.y * nodeOrientation.z + (double)actorOrientation.w * nodeOrientation.x) + (double)nodeOrientation.w * actorOrientation.x) - (double)actorOrientation.z * nodeOrientation.y;

	actor->quat = CQuaternion((float)resultX, (float)resultY, (float)resultZ, (float)resultW);
	actor->nodeFlags = (actor->nodeFlags & ~CNode::kEulerValid) | CNode::kQuaternionValid | CNode::kLocalSet;
	actor->updateLocalState();
	actor->setParent(node);
	actor->sortOrder = -2;
	actor->parentActorId = lua_getuserdata(parentObj);
}

void Lua_V2::DetachActor() {
	lua_Object actorObj = lua_getparam(1);

	// The disasm does not check the type of the parameter
	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	float position[3];

	position[0] = actor->world.pos.x;
	position[1] = actor->world.pos.y;
	position[2] = actor->world.pos.z;
	actor->setActorPosition(position);
	actor->quat = actor->getWorldOrientation();
	actor->nodeFlags = (actor->nodeFlags & ~CNode::kEulerValid) | CNode::kQuaternionValid | CNode::kLocalSet;
	actor->updateLocalState();

	Actor *parent = Actor::actorDeref(actor->parentActorId);

	if (parent) {
		Room_v2 *currentRoom = Room_v2::getCurrentRoom();

		if (currentRoom)
			actor->sortOrder = parent->getSortOrder(currentRoom->currentSetup);
		else
			warning("Lua_V2::DetachActor(): there is no current room");
	}

	actor->detachFromParent();
	actor->parentActorId = 0;
}

void Lua_V2::IsActorInSector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object nameObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	if (!lua_isstring(nameObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "IsActorInSector");
	LString planeName = lua_getstring(nameObj);
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!actor || !currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::IsActorInSector(): missing actor or walk planes");
		lua_pushnil();

		return;
	}

	WalkPlane_v2 *plane = currentRoom->walkPlanes->planeNamed(planeName);

	if (!plane) {
		lua_pushnil();

		return;
	}

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	if (plane->within(&actor->world.pos.x, true)) {
		lua_pushnumber((float)plane->number);
		lua_pushstring(plane->name.c_str());
		lua_pushnumber((float)plane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V2::GetActorSector() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object typeObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(typeObj))
		return;

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();
	Actor *actor = getActor(actorObj, "GetActorSector");

	if (!actor || !currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::GetActorSector(): missing actor or walk planes");
		lua_pushnil();

		return;
	}

	int typeMask = luaFtol(lua_getnumber(typeObj));

	if ((actor->nodeFlags & CNode::kWorldValid) == 0)
		actor->updateWorldTransform();

	WalkPlane_v2 *plane = currentRoom->walkPlanes->planeAt(&actor->world.pos.x, typeMask);

	if (plane) {
		lua_pushnumber((float)plane->number);
		lua_pushstring(plane->name.c_str());
		lua_pushnumber((float)plane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V2::IsPointInSector() {
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

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::IsPointInSector(): missing walk planes");
		lua_pushnil();

		return;
	}

	LString planeName = lua_getstring(nameObj);
	WalkPlane_v2 *plane = currentRoom->walkPlanes->planeNamed(planeName);

	if (plane && plane->within(point, true)) {
		lua_pushnumber((float)plane->number);
		lua_pushstring(plane->name.c_str());
		lua_pushnumber((float)plane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V2::GetPointSector() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object zObj = lua_getparam(3);
	lua_Object typeObj = lua_getparam(4);

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj)) {
		lua_pushnil();

		return;
	}

	int typeMask;
	float point[3];

	if (!lua_isnil(typeObj))
		typeMask = luaFtol(lua_getnumber(typeObj));
	else
		typeMask = 4096;

	point[0] = lua_getnumber(xObj);
	point[1] = lua_getnumber(yObj);
	point[2] = lua_getnumber(zObj);

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::GetPointSector(): missing walk planes");
		lua_pushnil();

		return;
	}

	WalkPlane_v2 *plane = currentRoom->walkPlanes->planeAt(point, typeMask);

	if (plane) {
		lua_pushnumber((float)plane->number);
		lua_pushstring(plane->name.c_str());
		lua_pushnumber((float)plane->type);
	} else {
		lua_pushnil();
	}
}

void Lua_V2::TurnActor() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object directionObj = lua_getparam(2);

	if (!isActor(actorObj) || !lua_isnumber(directionObj))
		return;

	Actor *actor = getActor(actorObj, "TurnActor");

	if (!actor)
		return;

	actor->mode &= ~1U;
	actor->turn(lua_getnumber(directionObj));
}

void Lua_V2::GetActorRot() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor)
		return;

	if ((actor->nodeFlags & CNode::kEulerValid) == 0)
		actor->computeLocalMatrixFromWorld();

	lua_pushnumber(actor->eulerDeg.x);

	if ((actor->nodeFlags & CNode::kEulerValid) == 0)
		actor->computeLocalMatrixFromWorld();

	lua_pushnumber(actor->eulerDeg.y);

	if ((actor->nodeFlags & CNode::kEulerValid) == 0)
		actor->computeLocalMatrixFromWorld();

	lua_pushnumber(actor->eulerDeg.z);
}

void Lua_V2::SetActorRot() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object pitchObj = lua_getparam(2);
	lua_Object yawObj = lua_getparam(3);
	lua_Object rollObj = lua_getparam(4);
	lua_Object snapObj = lua_getparam(5);
	float rotation[3];

	rotation[0] = 0.0f;
	rotation[1] = 0.0f;
	rotation[2] = 0.0f;

	if (!isActor(actorObj) || !lua_isnumber(pitchObj) || !lua_isnumber(yawObj) || !lua_isnumber(rollObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorRot");

	if (!actor)
		return;

	rotation[0] = lua_getnumber(pitchObj);
	rotation[1] = lua_getnumber(yawObj);
	rotation[2] = lua_getnumber(rollObj);
	actor->setActorRotation(rotation, lua_isnil(snapObj) != 0);
}

void Lua_V2::SetActorPitch() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object pitchObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorPitch");

	if (!actor)
		return;

	float rotation[3];

	rotation[0] = 0.0f;
	rotation[1] = 0.0f;
	rotation[2] = 0.0f;

	if ((actor->nodeFlags & CNode::kEulerValid) == 0) {
		actor->computeLocalMatrixFromWorld();

		if ((actor->nodeFlags & CNode::kEulerValid) == 0)
			actor->computeLocalMatrixFromWorld();
	}

	float currentRoll = actor->eulerDeg.z;
	float currentYaw = actor->eulerDeg.y;

	rotation[0] = lua_getnumber(pitchObj);
	rotation[1] = currentYaw;
	rotation[2] = currentRoll;
	actor->setActorRotation(rotation, true);
}

void Lua_V2::SetActorRoll() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object rollObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "SetActorRoll");

	if (!actor)
		return;

	float rotation[3];

	rotation[0] = 0.0f;
	rotation[1] = 0.0f;
	rotation[2] = 0.0f;

	if ((actor->nodeFlags & CNode::kEulerValid) == 0) {
		actor->computeLocalMatrixFromWorld();

		if ((actor->nodeFlags & CNode::kEulerValid) == 0)
			actor->computeLocalMatrixFromWorld();
	}

	float newRoll = lua_getnumber(rollObj);

	rotation[0] = actor->eulerDeg.x;
	rotation[1] = actor->eulerDeg.y;
	rotation[2] = newRoll;
	actor->setActorRotation(rotation, true);
}

void Lua_V2::IsActorTurning() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "IsActorTurning");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	// The disasm pushes the number and then nil
	if (actor->extra->alignOff > 0.0f || actor->extra->alignOff < 0.0f)
		lua_pushnumber(1.0f);

	lua_pushnil();
}

void Lua_V2::PointActorAt() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object targetObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	float target[3];

	target[0] = 0.0f;
	target[1] = 0.0f;
	target[2] = 0.0f;

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "PointActorAt");

	if (!actor)
		return;

	if (isActor(targetObj)) {
		Actor *targetActor = getActor(targetObj, "PointActorAt");

		if (!targetActor)
			return;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[0] = targetActor->world.pos.x;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[1] = targetActor->world.pos.y;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[2] = targetActor->world.pos.z;
	} else {
		target[0] = lua_getnumber(targetObj);
		target[1] = lua_getnumber(yObj);
		target[2] = lua_getnumber(zObj);
	}

	if (!actor->extra) {
		warning("Lua_V2::PointActorAt(): the actor has no extra data");

		return;
	}

	if (!actor->extra->puck)
		actor->enablePuck(true);

	actor->extra->puck->pointAt(target, kMaxPointAtAngle);
	actor->extra->snapTurn = true;
}

void Lua_V2::TurnActorTo() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object targetObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	lua_Object zObj = lua_getparam(4);
	float target[3];

	target[0] = 0.0f;
	target[1] = 0.0f;
	target[2] = 0.0f;

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "TurnActorTo");

	if (!actor)
		return;

	if (isActor(targetObj)) {
		Actor *targetActor = getActor(targetObj, "TurnActorTo");

		if (!targetActor)
			return;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[0] = targetActor->world.pos.x;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[1] = targetActor->world.pos.y;

		if ((targetActor->nodeFlags & CNode::kWorldValid) == 0)
			targetActor->updateWorldTransform();

		target[2] = targetActor->world.pos.z;
	} else {
		target[0] = lua_getnumber(targetObj);
		target[1] = lua_getnumber(yObj);
		target[2] = lua_getnumber(zObj);
	}

	if (!actor->extra || !actor->extra->puck)
		actor->enablePuck(true);

	float turnRate = actor->extra->turnRate;

	if (!(turnRate >= 0.0f))
		turnRate = -turnRate;

	float turnStep = EmiEngine::getFrameTime() * turnRate;
	float turned = actor->extra->puck->pointAt(target, turnStep);

	if (turned < 0.0f)
		turned = -turned;

	if (turned >= turnStep)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::PlayActorChore() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object choreSetObj = lua_getparam(3);
	lua_Object loopObj = lua_getparam(4);
	lua_Object fadeObj = lua_getparam(5);
	float fadeSeconds = 0.0f;
	bool looping = false;

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!lua_isstring(choreObj) || !lua_isstring(choreSetObj)) {
		lua_pushnil();

		return;
	}

	if (!actor) {
		warning("Lua_V2::PlayActorChore(): the actor does not exist");

		return;
	}

	if (!lua_isnil(loopObj) && lua_isnumber(loopObj)) {
		float loopValue = lua_getnumber(loopObj);

		looping = loopValue > 0.0f || loopValue < 0.0f;

		if (!lua_isnil(fadeObj) && lua_isnumber(fadeObj))
			fadeSeconds = lua_getnumber(fadeObj);
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LString choreName = lua_getstring(choreObj);
	uint startedId;
	uint instanceId = *actor->startChore(&startedId, choreName, choreSetName);

	if (!(fadeSeconds > 0.0f)) {
		if (ChoreInstance::isInstanceValid(instanceId))
			ChoreInstance::getInstance(instanceId)->setPlaying();
	} else if (ChoreInstance::isInstanceValid(instanceId)) {
		ChoreInstance::getInstance(instanceId)->fadeIn(fadeSeconds);
	}

	if (ChoreInstance::isInstanceValid(instanceId))
		ChoreInstance::getInstance(instanceId)->setLooping(looping);

	lua_pushnumber((float)instanceId);
}

void Lua_V2::PlayActorChoreLooping() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object choreObj = lua_getparam(2);
	lua_Object choreSetObj = lua_getparam(3);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!lua_isstring(choreObj) || !lua_isstring(choreSetObj)) {
		lua_pushnil();

		return;
	}

	if (!actor) {
		warning("Lua_V2::PlayActorChoreLooping(): the actor does not exist");

		return;
	}

	LString choreSetName = lua_getstring(choreSetObj);
	LString choreName = lua_getstring(choreObj);
	uint startedId;
	uint instanceId = *actor->startChore(&startedId, choreName, choreSetName);

	if (ChoreInstance::isInstanceValid(instanceId))
		ChoreInstance::getInstance(instanceId)->setPlaying();

	if (ChoreInstance::isInstanceValid(instanceId))
		ChoreInstance::getInstance(instanceId)->setLooping(true);

	lua_pushnumber((float)instanceId);
}

void Lua_V2::IsActorMoving() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "IsActorMoving");

	if (!actor)
		return;

	if ((actor->mode & 1) != 0)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::IsActorResting() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj))
		return;

	Actor *actor = getActor(actorObj, "IsActorResting");

	if (!actor)
		return;

	if (!actor->extra)
		actor->allocateActorExtra();

	if (actor->extra->resting)
		lua_pushnumber(1.0f);
	else
		lua_pushnil();
}

void Lua_V2::GetActorChores() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (!actor) {
		lua_pushnil();

		return;
	}

	lua_Object table = lua_createtable();
	uint foundId;
	uint count = 0;

	for (uint choreId = *actor->findChoreInstance(&foundId, 0); choreId; choreId = *actor->findChoreInstance(&foundId, choreId)) {
		lua_pushobject(table);
		lua_pushnumber((float)count);
		lua_pushnumber((float)choreId);
		lua_settable();
		count++;
	}

	lua_pushobject(table);
	lua_pushstring("count");
	lua_pushnumber((float)count);
	lua_settable();
	lua_pushobject(table);
}

void Lua_V2::StopActorChores() {
	lua_Object actorObj = lua_getparam(1);
	lua_Object skipLoopingObj = lua_getparam(2);

	if (!isActor(actorObj))
		return;

	Actor *actor = Actor::actorDeref(lua_getuserdata(actorObj));

	if (actor)
		actor->stopChores(!lua_isnil(skipLoopingObj));
}

void Lua_V2::IsSectorActive() {
	lua_Object sectorObj = lua_getparam(1);

	if (lua_isnumber(sectorObj) || lua_isstring(sectorObj)) {
		Room_v2 *currentRoom = Room_v2::getCurrentRoom();

		if (currentRoom && currentRoom->walkPlanes) {
			WalkPlane_v2 *plane;

			if (!lua_isstring(sectorObj)) {
				plane = currentRoom->walkPlanes->planeNumbered(luaFtol(lua_getnumber(sectorObj)));
			} else {
				LString planeName = lua_getstring(sectorObj);

				plane = currentRoom->walkPlanes->planeNamed(planeName);
			}

			// The disasm pushes 1.0 and then nil here
			if (plane && plane->active)
				lua_pushnumber(1.0f);
		} else if (currentRoom) {
			warning("Lua_V2::IsSectorActive(): missing walk planes");
		}
	}

	lua_pushnil();
}

void Lua_V2::MakeSectorActive() {
	lua_Object sectorObj = lua_getparam(1);

	if (!lua_isnumber(sectorObj) && !lua_isstring(sectorObj))
		return;

	lua_Object activeObj = lua_getparam(2);
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::MakeSectorActive(): missing room or walk planes");

		return;
	}

	WalkPlane_v2 *plane;

	if (lua_isnumber(sectorObj)) {
		plane = currentRoom->walkPlanes->planeNumbered(luaFtol(lua_getnumber(sectorObj)));
	} else {
		LString planeName = lua_getstring(sectorObj);

		plane = currentRoom->walkPlanes->planeNamed(planeName);
	}

	if (plane)
		plane->active = !lua_isnil(activeObj);
}

void Lua_V2::GetSectorOppositeEdge() {
	lua_Object actorObj = lua_getparam(1);

	if (!isActor(actorObj)) {
		lua_pushnil();

		return;
	}

	lua_Object sectorObj = lua_getparam(2);

	if (!lua_isstring(sectorObj)) {
		lua_pushnil();

		return;
	}

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::GetSectorOppositeEdge(): missing room or walk planes");

		return;
	}

	LString planeName = lua_getstring(sectorObj);
	WalkPlane_v2 *plane = currentRoom->walkPlanes->planeNamed(planeName);

	if (!plane) {
		lua_pushnil();

		return;
	}

	Actor *actor = getActor(actorObj, "GetSectorOppositeEdge");

	if (actor)
		actor->leaveSector(plane);
}

void Lua_V2::GetSectorName() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object zObj = lua_getparam(3);
	float point[3];

	point[0] = 0.0f;
	point[1] = 0.0f;
	point[2] = 0.0f;

	if (!lua_isnumber(xObj) || !lua_isnumber(yObj) || !lua_isnumber(zObj)) {
		if (gpConsoleServices)
			gpConsoleServices->logError("GetSectorName: one or more of x, y, z is not a number!\n");
		else
			warning("GetSectorName: one or more of x, y, z is not a number!");

		lua_pushnil();

		return;
	}

	point[0] = lua_getnumber(xObj);
	point[1] = lua_getnumber(yObj);
	point[2] = lua_getnumber(zObj);

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom || !currentRoom->walkPlanes) {
		warning("Lua_V2::GetSectorName(): missing room or walk planes");
		lua_pushnil();

		return;
	}

	WalkPlane_v2 *plane = currentRoom->walkPlanes->planeAt(point, -1);

	if (!plane) {
		lua_pushnil();

		return;
	}

	lua_pushstring(plane->name.c_str());
}

void Lua_V2::GetAngleBetweenActors() {
	lua_Object firstObj = lua_getparam(1);
	lua_Object secondObj = lua_getparam(2);

	if (!isActor(firstObj) || !isActor(secondObj)) {
		lua_pushnil();

		return;
	}

	Actor *first = Actor::actorDeref(lua_getuserdata(firstObj));
	Actor *second = Actor::actorDeref(lua_getuserdata(secondObj));

	if (!first || !second) {
		lua_pushnil();

		return;
	}

	if ((first->nodeFlags & CNode::kEulerValid) == 0)
		first->computeLocalMatrixFromWorld();

	float sinValue = (float)sin((double)Actor::kDegreesToRadians * first->eulerDeg.y);

	if ((first->nodeFlags & CNode::kEulerValid) == 0)
		first->computeLocalMatrixFromWorld();

	float cosValue = (float)cos((double)Actor::kDegreesToRadians * first->eulerDeg.y);

	if ((first->nodeFlags & CNode::kWorldValid) == 0)
		first->updateWorldTransform();

	if ((second->nodeFlags & CNode::kWorldValid) == 0)
		second->updateWorldTransform();

	double differenceX = (double)second->world.pos.x - first->world.pos.x;
	float differenceXValue = (float)differenceX;
	float differenceYValue = (float)((double)second->world.pos.y - first->world.pos.y);
	float differenceZValue = (float)((double)second->world.pos.z - first->world.pos.z);
	double facingLength = sqrt((double)cosValue * cosValue + (double)sinValue * sinValue);

	if (facingLength >= (double)kMinDirectionLength) {
		double facingInverse = 1.0 / facingLength;

		sinValue = (float)(facingInverse * sinValue);
		cosValue = (float)(facingInverse * cosValue);
	}

	double directionX = differenceX;
	double directionY = differenceYValue;
	double directionZ = differenceZValue;
	double directionLength = sqrt(((double)differenceZValue * differenceZValue + (double)differenceXValue * differenceXValue) + (double)differenceYValue * differenceYValue);
	float directionLengthValue = (float)directionLength;

	if (directionLength >= (double)kMinDirectionLength) {
		double directionInverse = 1.0 / (double)directionLengthValue;
		float directionInverseValue = (float)directionInverse;

		directionX = directionInverse * differenceXValue;
		directionY = (double)directionInverseValue * differenceYValue;
		directionZ = (double)differenceZValue * directionInverseValue;
	}

	// The disasm multiplies the y component by zero here
	double alignment = (directionZ * cosValue + directionY * 0.0) + directionX * sinValue;
	float alignmentValue = (float)alignment;

	if (!(alignment > 1.0) || !(alignmentValue < kFlatAngleLimit))
		lua_pushnumber((float)(acos((double)alignmentValue) * WalkPlane_v2::kRadiansToDegrees));
	else
		lua_pushnumber(0.0f);
}

} // end of namespace Grim
