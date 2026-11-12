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
#include "common/ustr.h"

#include "engines/grim/emi/actor/actor.h"
#include "engines/grim/emi/costume/choreinstance.h"
#include "engines/grim/emi/graphics/color.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/scene/overworld.h"
#include "engines/grim/emi/actor/puck_v2.h"
#include "engines/grim/emi/nodes/rprimitive.h"
#include "engines/grim/emi/nodes/rprimitiveinstance.h"
#include "engines/grim/emi/scene/walkplanecollection.h"

namespace Grim {

static const int kSavedStringBufferSize = 512;
static const uint kMaxSkippedStateSize = 65536;
static const char *const kOverworldRoomName = "[OW]";

static void writeSavedString(SaveRestoreAccess *access, const LString &string) {
	int length = (int)string.size() + 1;

	access(&length, sizeof(length));
	access((void *)string.c_str(), length);
}

static bool readSavedString(SaveRestoreAccess *access, LString &string) {
	char buffer[kSavedStringBufferSize + 1];
	uint length = 0;

	access(&length, sizeof(length));

	if (length > (uint)kSavedStringBufferSize) {
		warning("Actor::restore(): invalid string length %u", length);

		return false;
	}

	access(buffer, length);
	buffer[length] = 0;
	string = buffer;

	return true;
}

static void writeSavedFlag(SaveRestoreAccess *access, bool value) {
	byte raw = value ? 1 : 0;

	access(&raw, sizeof(raw));
}

static bool readSavedFlag(SaveRestoreAccess *access) {
	byte raw = 0;

	access(&raw, sizeof(raw));

	return raw != 0;
}

static void restoreNodeEuler(CNode *node, const CVector &angles) {
	node->_eulerDeg = angles;
	node->_nodeFlags = (node->_nodeFlags & ~CNode::kQuaternionValid) | CNode::kEulerValid | CNode::kLocalSet;
	node->updateLocalState();
}

bool Actor::save(SaveRestoreAccess *access) {
	writeSavedString(access, _actorName);
	access(&_id, sizeof(_id));
	access(&_actorFlags, sizeof(_actorFlags));
	access(&_mode, sizeof(_mode));

	if ((_nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	access(&_pos, sizeof(_pos));

	if ((_nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	access(&_eulerDeg, sizeof(_eulerDeg));
	access(&_prevPos, sizeof(_prevPos));
	access(&_reserved312, sizeof(_reserved312));
	writeSavedFlag(access, _onScreen);
	access(&_sortOrder, sizeof(_sortOrder));

	if (_parentActorId && !actorDeref(_engine, _parentActorId))
		_parentActorId = 0;

	access(&_parentActorId, sizeof(_parentActorId));

	if (_parentActorId) {
		if (_parent) {
			access(_parent->_name, CNode::kNameSize);
		} else {
			char emptyName[CNode::kNameSize];

			warning("Actor::save(): the parent node is missing");
			memset(emptyName, 0, sizeof(emptyName));
			access(emptyName, CNode::kNameSize);
		}
	}

	for (int i = 0; i < 4; i++)
		access(&_drawnRect[i], sizeof(int));

	LString roomName;

	if (_room._resource) {
		bool locked = _room.lock();

		if (locked && _room._typed)
			roomName = _room._typed->_name;
		else
			warning("Actor::save(): the room could not be locked");

		if (locked)
			_room.unlock();
	} else if (_inOverworld) {
		roomName = kOverworldRoomName;
	}

	writeSavedString(access, roomName);

	removeFinishedChores();

	int choreCount = (int)_choreIds.size();

	access(&choreCount, sizeof(choreCount));

	Common::List<uint>::iterator choreIt;

	for (choreIt = _choreIds.begin(); choreIt != _choreIds.end(); ++choreIt) {
		uint choreId = *choreIt;

		access(&choreId, sizeof(choreId));

		if (ChoreInstance::isInstanceValid(_engine, choreId)) {
			ChoreInstance *instance = ChoreInstance::getInstance(_engine, choreId);

			if (instance)
				instance->save(access);
		}
	}

	for (int i = 0; i < kNumChoreSlots; i++) {
		writeSavedString(access, _choreSlotNames[i]);
		writeSavedString(access, _choreSlotSetNames[i]);
		access(&_choreSlotIds[i], sizeof(uint));
	}

	int primitiveCount = (int)_instances.size();

	access(&primitiveCount, sizeof(primitiveCount));

	for (int i = 0; i < primitiveCount; i++) {
		RPrimitiveInstance *instance = _instances[i];

		if (instance->_prim) {
			writeSavedString(access, instance->_prim->_name);
		} else {
			warning("Actor::save(): the primitive instance has no primitive");
			writeSavedString(access, LString());
		}

		int stateSize = instance->getSaveSize();

		access(&stateSize, sizeof(stateSize));
		instance->saveState(access);
	}

	int shadowCount = (int)_shadows.size();

	access(&shadowCount, sizeof(shadowCount));

	for (int i = 0; i < shadowCount; i++)
		writeSavedString(access, _shadows[i]->_name);

	writeSavedFlag(access, _extra != nullptr);

	if (_extra) {
		writeSavedFlag(access, _extra->_lookAtActive);
		writeSavedFlag(access, _extra->_lookAtHasTarget);
		access(&_extra->_lookTarget, sizeof(CVector));
		access(&_extra->_lookOffset, sizeof(CVector));
		access(&_extra->_lookTargetActorId, sizeof(int));
		access(&_extra->_lookGoalQuat, sizeof(CQuaternion));
		access(&_extra->_lookDirection, sizeof(CVector));
		access(&_extra->_lookCurQuat, sizeof(CQuaternion));
		access(_extra->_lookLimits, Extra_v2::kNumLookLimits * sizeof(CVector));
		writeSavedString(access, _extra->_lookJointName);
		access(&_extra->_lookCosFov, sizeof(float));
		access(&_extra->_lookRate, sizeof(float));
		writeSavedFlag(access, _extra->_walking);

		// The disasm saves only the first byte of turnDir
		byte turnDirLow = (byte)(_extra->_turnDir & 0xFF);

		access(&turnDirLow, sizeof(turnDirLow));
		writeSavedFlag(access, _extra->_resting);
		access(&_extra->_walkRate, sizeof(float));
		access(&_extra->_turnRate, sizeof(float));
		writeSavedFlag(access, _extra->_aligned);
		writeSavedFlag(access, _extra->_snapTurn);
		access(&_extra->_offsetYaw, sizeof(float));
		access(&_extra->_alignOff, sizeof(float));
		access(&_extra->_walkTarget, sizeof(CVector));
		access(&_extra->_goal, sizeof(CVector));
		access(&_extra->_numPathPoints, sizeof(int));
		access(_extra->_path, Extra_v2::kMaxPathPoints * sizeof(CVector));
		access(&_extra->_pathIndex, sizeof(int));
		writeSavedFlag(access, _extra->_puck != nullptr);
		writeSavedFlag(access, _extra->_inBoxes);
		writeSavedFlag(access, _extra->_walkDominate);
		access(&_extra->_reflectionAngle, sizeof(float));
		writeSavedFlag(access, _extra->_talking);
		access(&_extra->_talkTimerMs, sizeof(int));
		access(_extra->_sayLineId, Extra_v2::kSayLineIdSize);
		access(&_extra->_textEventId, sizeof(int));
		writeSavedFlag(access, _extra->_lipsyncActive);
		access(&_extra->_soundHandle, sizeof(int));
		access(&_extra->_visemeCurrent, sizeof(int));
		access(&_extra->_collisionMode, sizeof(int));
		access(&_extra->_collisionScale, sizeof(float));
		writeSavedFlag(access, _extra->_inCollision);
		access(&_extra->_talkColor, sizeof(Color));
		access(&_extra->_timeScale, sizeof(float));

		if (_extra->_puck) {
			Puck_v2 *puck = _extra->_puck;

			if ((puck->_nodeFlags & kPositionValid) == 0)
				puck->computeLocalPositionFromWorld();

			access(&puck->_pos, sizeof(CVector));

			if ((puck->_nodeFlags & kEulerValid) == 0)
				puck->computeLocalMatrixFromWorld();

			access(&puck->_eulerDeg, sizeof(CVector));

			// The disasm saves only the first byte of the reflection angle
			byte reflectionRaw[sizeof(float)];

			memcpy(reflectionRaw, &puck->_reflectionAngle, sizeof(reflectionRaw));
			access(&reflectionRaw[0], sizeof(byte));
			writeSavedFlag(access, puck->_constrained);
			access(puck->_constraint, sizeof(puck->_constraint));
			access(puck->_lineA, sizeof(puck->_lineA));
			access(puck->_lineB, sizeof(puck->_lineB));
		}
	}

	access(&_drawFlags, sizeof(_drawFlags));

	return true;
}

bool Actor::restore(SaveRestoreAccess *access) {
	CVector savedVector;

	if (!readSavedString(access, _actorName))
		return false;

	access(&_id, sizeof(_id));
	access(&_actorFlags, sizeof(_actorFlags));
	access(&_mode, sizeof(_mode));
	access(&savedVector, sizeof(savedVector));
	setLocalPosition(savedVector);
	access(&savedVector, sizeof(savedVector));
	restoreNodeEuler(this, savedVector);
	access(&_prevPos, sizeof(_prevPos));
	access(&_reserved312, sizeof(_reserved312));
	_onScreen = readSavedFlag(access);
	access(&_sortOrder, sizeof(_sortOrder));
	access(&_parentActorId, sizeof(_parentActorId));

	if (_parentActorId)
		access(_parentNodeName, CNode::kNameSize);

	for (int i = 0; i < 4; i++)
		access(&_drawnRect[i], sizeof(int));

	LString savedRoomName;

	if (!readSavedString(access, savedRoomName))
		return false;

	if (savedRoomName == kOverworldRoomName)
		_engine->getOverworld()->putActor(this);
	else if (!savedRoomName.empty())
		putInSet(savedRoomName.c_str());
	else
		putInSet(nullptr);

	uint choreCount = 0;

	access(&choreCount, sizeof(choreCount));

	for (uint i = 0; i < choreCount; i++) {
		uint choreId = 0;

		access(&choreId, sizeof(choreId));

		if (ChoreInstance::isInstanceValid(_engine, choreId)) {
			ChoreInstance *existing = ChoreInstance::getInstance(_engine, choreId);

			if (existing)
				delete existing;
		}

		choreId = ChoreInstance::newInstanceWithId(_engine, this, choreId);

		if (ChoreInstance::isInstanceValid(_engine, choreId)) {
			ChoreInstance *instance = ChoreInstance::getInstance(_engine, choreId);

			if (instance)
				instance->restore(access);
		}

		_choreIds.push_back(choreId);
	}

	for (int i = 0; i < kNumChoreSlots; i++) {
		if (!readSavedString(access, _choreSlotNames[i]))
			return false;

		if (!readSavedString(access, _choreSlotSetNames[i]))
			return false;

		access(&_choreSlotIds[i], sizeof(uint));
	}

	uint primitiveCount = 0;

	access(&primitiveCount, sizeof(primitiveCount));

	for (uint i = 0; i < primitiveCount; i++) {
		LString primitiveName;
		bool found = false;

		if (!readSavedString(access, primitiveName))
			return false;

		for (uint j = 0; j < _instances.size() && !found; j++) {
			RPrimitiveInstance *instance = _instances[j];

			if (instance->_prim && instance->_prim->_name == primitiveName && !instance->_stateRestored) {
				int stateSize = 0;

				access(&stateSize, sizeof(stateSize));
				instance->restoreState(access);
				found = true;
			}
		}

		if (!found) {
			uint stateSize = 0;

			Common::String message = Common::String::format("Could not find file %s.", primitiveName.c_str());

			warning("%s", message.c_str());
			_engine->showMessage(message.c_str(), "Error!");
			access(&stateSize, sizeof(stateSize));

			if (stateSize > kMaxSkippedStateSize) {
				warning("Actor::restore(): invalid primitive state size %u", stateSize);

				return false;
			}

			byte discard[256];

			while (stateSize > 0) {
				uint chunk = stateSize < sizeof(discard) ? stateSize : (uint)sizeof(discard);

				access(discard, chunk);
				stateSize -= chunk;
			}
		}
	}

	uint shadowCount = 0;

	access(&shadowCount, sizeof(shadowCount));

	for (uint i = 0; i < shadowCount; i++) {
		LString shadowName;

		if (!readSavedString(access, shadowName))
			return false;

		bool locked = _room.lock();
		Room_v2 *lockedRoom = locked ? _room._typed : nullptr;

		if (lockedRoom)
			activateShadow(true, lockedRoom->findShadow(shadowName));
		else
			warning("Actor::restore(): the room could not be locked");

		if (locked)
			_room.unlock();
	}

	if (readSavedFlag(access)) {
		_extra = nullptr;
		allocateActorExtra();

		_extra->_lookAtActive = readSavedFlag(access);
		_extra->_lookAtHasTarget = readSavedFlag(access);
		access(&_extra->_lookTarget, sizeof(CVector));
		access(&_extra->_lookOffset, sizeof(CVector));
		access(&_extra->_lookTargetActorId, sizeof(int));
		access(&_extra->_lookGoalQuat, sizeof(CQuaternion));
		access(&_extra->_lookDirection, sizeof(CVector));
		access(&_extra->_lookCurQuat, sizeof(CQuaternion));
		access(_extra->_lookLimits, Extra_v2::kNumLookLimits * sizeof(CVector));

		if (!readSavedString(access, _extra->_lookJointName))
			return false;

		access(&_extra->_lookCosFov, sizeof(float));
		access(&_extra->_lookRate, sizeof(float));
		_extra->_walking = readSavedFlag(access);

		// The disasm restores only the first byte of turnDir
		byte turnDirLow = 0;

		access(&turnDirLow, sizeof(turnDirLow));
		_extra->_turnDir = (_extra->_turnDir & ~0xFF) | turnDirLow;
		_extra->_resting = readSavedFlag(access);
		access(&_extra->_walkRate, sizeof(float));
		access(&_extra->_turnRate, sizeof(float));
		_extra->_aligned = readSavedFlag(access);
		_extra->_snapTurn = readSavedFlag(access);
		access(&_extra->_offsetYaw, sizeof(float));
		access(&_extra->_alignOff, sizeof(float));
		access(&_extra->_walkTarget, sizeof(CVector));
		access(&_extra->_goal, sizeof(CVector));
		access(&_extra->_numPathPoints, sizeof(int));
		access(_extra->_path, Extra_v2::kMaxPathPoints * sizeof(CVector));
		access(&_extra->_pathIndex, sizeof(int));

		bool hasPuck = readSavedFlag(access);

		_extra->_inBoxes = readSavedFlag(access);
		_extra->_walkDominate = readSavedFlag(access);
		access(&_extra->_reflectionAngle, sizeof(float));
		_extra->_talking = readSavedFlag(access);
		access(&_extra->_talkTimerMs, sizeof(int));
		access(_extra->_sayLineId, Extra_v2::kSayLineIdSize);
		access(&_extra->_textEventId, sizeof(int));
		_extra->_lipsyncActive = readSavedFlag(access);
		access(&_extra->_soundHandle, sizeof(int));
		access(&_extra->_visemeCurrent, sizeof(int));
		access(&_extra->_collisionMode, sizeof(int));
		access(&_extra->_collisionScale, sizeof(float));
		_extra->_inCollision = readSavedFlag(access);
		access(&_extra->_talkColor, sizeof(Color));
		access(&_extra->_timeScale, sizeof(float));

		if (hasPuck) {
			Puck_v2 *puck = new Puck_v2(_engine, _extra->_reflectionAngle);
			CVector puckVector;

			_extra->_puck = puck;
			puck->_actorName = _actorName;

			access(&puckVector, sizeof(puckVector));
			puck->setLocalPosition(puckVector);
			access(&puckVector, sizeof(puckVector));
			restoreNodeEuler(puck, puckVector);

			byte reflectionRaw[sizeof(float)];

			memcpy(reflectionRaw, &puck->_reflectionAngle, sizeof(reflectionRaw));
			access(&reflectionRaw[0], sizeof(byte));
			memcpy(&puck->_reflectionAngle, reflectionRaw, sizeof(reflectionRaw));
			puck->_constrained = readSavedFlag(access);
			access(puck->_constraint, sizeof(puck->_constraint));
			access(puck->_lineA, sizeof(puck->_lineA));
			access(puck->_lineB, sizeof(puck->_lineB));

			if (_extra->_inBoxes && _room._resource) {
				bool locked = _room.lock();
				Room_v2 *lockedRoom = locked ? _room._typed : nullptr;

				if (lockedRoom)
					puck->validate(lockedRoom->_walkPlanes);
				else
					warning("Actor::restore(): the room could not be locked");

				if (locked)
					_room.unlock();
			}
		}

		if (_extra->_talking) {
			for (int i = 0; i < kNumSpeakers; i++) {
				if (!_engine->_speakers[i]) {
					_engine->_speakers[i] = this;
					_engine->_numSpeakers++;

					break;
				}
			}
		}
	}

	access(&_drawFlags, sizeof(_drawFlags));

	return true;
}

void Actor::saveActors(EmiEngine *engine, SaveRestoreAccess *access) {
	int selectedId = -1;

	if (engine->_selectedActor)
		selectedId = engine->_selectedActor->_id;

	access(&selectedId, sizeof(selectedId));

	int actorCount = (int)engine->_actors.size();

	access(&actorCount, sizeof(actorCount));

	Common::List<Actor *>::iterator it;

	for (it = engine->_actors.begin(); it != engine->_actors.end(); ++it)
		(*it)->save(access);

	access(&engine->_nextChoreInstanceId, sizeof(engine->_nextChoreInstanceId));
	access(&engine->_nextActorId, sizeof(engine->_nextActorId));
}

void Actor::restoreActors(EmiEngine *engine, SaveRestoreAccess *access) {
	for (int i = 0; i < kNumSpeakers; i++)
		engine->_speakers[i] = nullptr;

	engine->_numSpeakers = 0;

	int selectedId = 0;
	int actorCount = 0;

	access(&selectedId, sizeof(selectedId));
	access(&actorCount, sizeof(actorCount));

	for (int i = 0; i < actorCount; i++) {
		Actor *actor = new Actor(engine, "Restored");

		actor->restore(access);
	}

	access(&engine->_nextChoreInstanceId, sizeof(engine->_nextChoreInstanceId));
	access(&engine->_nextActorId, sizeof(engine->_nextActorId));

	engine->_selectedActor = nullptr;

	if (selectedId >= 0)
		engine->_selectedActor = actorDeref(engine, selectedId);

	Common::List<Actor *>::iterator it;

	for (it = engine->_actors.begin(); it != engine->_actors.end(); ++it) {
		Actor *actor = *it;

		if (!actor->_parentActorId)
			continue;

		Actor *parentActor = actorDeref(engine, actor->_parentActorId);

		if (parentActor) {
			char nodeName[CNode::kNameSize + 1];

			memcpy(nodeName, actor->_parentNodeName, CNode::kNameSize);
			nodeName[CNode::kNameSize] = 0;

			CNode *parentNode = parentActor->findNode(nodeName);

			if (!parentNode)
				parentNode = parentActor;

			actor->setParent(parentNode);
		} else {
			actor->_parentActorId = 0;
			actor->_parentNodeName[0] = 0;
		}
	}
}

} // End of namespace Grim
