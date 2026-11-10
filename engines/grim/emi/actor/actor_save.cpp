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

#include "gui/error.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/puck_v2.h"
#include "engines/grim/emi/rprimitive.h"
#include "engines/grim/emi/rprimitiveinstance.h"
#include "engines/grim/emi/walkplanecollection.h"

namespace Grim {

static const int kSavedStringBufferSize = 512;
static const uint kMaxSkippedStateSize = 65536;
static const char *const kOverworldRoomName = "[OW]";

static void writeSavedString(tSaveRestoreAccess *access, const LString &string) {
	int length = (int)string.size() + 1;

	access(&length, sizeof(length));
	access((void *)string.c_str(), length);
}

static bool readSavedString(tSaveRestoreAccess *access, LString &string) {
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

static void writeSavedFlag(tSaveRestoreAccess *access, bool value) {
	byte raw = value ? 1 : 0;

	access(&raw, sizeof(raw));
}

static bool readSavedFlag(tSaveRestoreAccess *access) {
	byte raw = 0;

	access(&raw, sizeof(raw));

	return raw != 0;
}

static void restoreNodeEuler(CNode *node, const CVector &angles) {
	node->eulerDeg = angles;
	node->nodeFlags = (node->nodeFlags & ~CNode::kQuaternionValid) | CNode::kEulerValid | CNode::kLocalSet;
	node->updateLocalState();
}

bool Actor::save(tSaveRestoreAccess *access) {
	writeSavedString(access, actorName);
	access(&id, sizeof(id));
	access(&actorFlags, sizeof(actorFlags));
	access(&mode, sizeof(mode));

	if ((nodeFlags & kPositionValid) == 0)
		computeLocalPositionFromWorld();

	access(&pos, sizeof(pos));

	if ((nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	access(&eulerDeg, sizeof(eulerDeg));
	access(&prevPos, sizeof(prevPos));
	access(&reserved312, sizeof(reserved312));
	writeSavedFlag(access, onScreen);
	access(&sortOrder, sizeof(sortOrder));

	if (parentActorId && !actorDeref(parentActorId))
		parentActorId = 0;

	access(&parentActorId, sizeof(parentActorId));

	if (parentActorId) {
		if (parent) {
			access(parent->name, CNode::kNameSize);
		} else {
			char emptyName[CNode::kNameSize];

			warning("Actor::save(): the parent node is missing");
			memset(emptyName, 0, sizeof(emptyName));
			access(emptyName, CNode::kNameSize);
		}
	}

	for (int i = 0; i < 4; i++)
		access(&drawnRect[i], sizeof(int));

	LString roomName;

	if (room.resource) {
		bool locked = room.lock();

		if (locked && room.typed)
			roomName = room.typed->name;
		else
			warning("Actor::save(): the room could not be locked");

		if (locked)
			room.unlock();
	} else if (inOverworld) {
		roomName = kOverworldRoomName;
	}

	writeSavedString(access, roomName);

	removeFinishedChores();

	int choreCount = (int)choreIds.size();

	access(&choreCount, sizeof(choreCount));

	Common::List<uint>::iterator choreIt;

	for (choreIt = choreIds.begin(); choreIt != choreIds.end(); ++choreIt) {
		uint choreId = *choreIt;

		access(&choreId, sizeof(choreId));

		if (ChoreInstance::isInstanceValid(choreId)) {
			ChoreInstance *instance = ChoreInstance::getInstance(choreId);

			if (instance)
				instance->save(access);
		}
	}

	for (int i = 0; i < kNumChoreSlots; i++) {
		writeSavedString(access, choreSlotNames[i]);
		writeSavedString(access, choreSlotSetNames[i]);
		access(&choreSlotIds[i], sizeof(uint));
	}

	int primitiveCount = (int)instances.size();

	access(&primitiveCount, sizeof(primitiveCount));

	for (int i = 0; i < primitiveCount; i++) {
		RPrimitiveInstance *instance = instances[i];

		if (instance->prim) {
			writeSavedString(access, instance->prim->name);
		} else {
			warning("Actor::save(): the primitive instance has no primitive");
			writeSavedString(access, LString());
		}

		int stateSize = instance->getSaveSize();

		access(&stateSize, sizeof(stateSize));
		instance->saveState(access);
	}

	int shadowCount = (int)shadows.size();

	access(&shadowCount, sizeof(shadowCount));

	for (int i = 0; i < shadowCount; i++)
		writeSavedString(access, shadows[i]->name);

	writeSavedFlag(access, extra != nullptr);

	if (extra) {
		writeSavedFlag(access, extra->lookAtActive);
		writeSavedFlag(access, extra->lookAtHasTarget);
		access(&extra->lookTarget, sizeof(CVector));
		access(&extra->lookOffset, sizeof(CVector));
		access(&extra->lookTargetActorId, sizeof(int));
		access(&extra->lookGoalQuat, sizeof(CQuaternion));
		access(&extra->lookDirection, sizeof(CVector));
		access(&extra->lookCurQuat, sizeof(CQuaternion));
		access(extra->lookLimits, Extra_v2::kNumLookLimits * sizeof(CVector));
		writeSavedString(access, extra->lookJointName);
		access(&extra->lookCosFov, sizeof(float));
		access(&extra->lookRate, sizeof(float));
		writeSavedFlag(access, extra->walking);

		// The disasm saves only the first byte of turnDir
		byte turnDirLow = (byte)(extra->turnDir & 0xFF);

		access(&turnDirLow, sizeof(turnDirLow));
		writeSavedFlag(access, extra->resting);
		access(&extra->walkRate, sizeof(float));
		access(&extra->turnRate, sizeof(float));
		writeSavedFlag(access, extra->aligned);
		writeSavedFlag(access, extra->snapTurn);
		access(&extra->offsetYaw, sizeof(float));
		access(&extra->alignOff, sizeof(float));
		access(&extra->walkTarget, sizeof(CVector));
		access(&extra->goal, sizeof(CVector));
		access(&extra->numPathPoints, sizeof(int));
		access(extra->path, Extra_v2::kMaxPathPoints * sizeof(CVector));
		access(&extra->pathIndex, sizeof(int));
		writeSavedFlag(access, extra->puck != nullptr);
		writeSavedFlag(access, extra->inBoxes);
		writeSavedFlag(access, extra->walkDominate);
		access(&extra->reflectionAngle, sizeof(float));
		writeSavedFlag(access, extra->talking);
		access(&extra->talkTimerMs, sizeof(int));
		access(extra->sayLineId, Extra_v2::kSayLineIdSize);
		access(&extra->textEventId, sizeof(int));
		writeSavedFlag(access, extra->lipsyncActive);
		access(&extra->soundHandle, sizeof(int));
		access(&extra->visemeCurrent, sizeof(int));
		access(&extra->collisionMode, sizeof(int));
		access(&extra->collisionScale, sizeof(float));
		writeSavedFlag(access, extra->inCollision);
		access(&extra->talkColor, sizeof(Color));
		access(&extra->timeScale, sizeof(float));

		if (extra->puck) {
			Puck_v2 *puck = extra->puck;

			if ((puck->nodeFlags & kPositionValid) == 0)
				puck->computeLocalPositionFromWorld();

			access(&puck->pos, sizeof(CVector));

			if ((puck->nodeFlags & kEulerValid) == 0)
				puck->computeLocalMatrixFromWorld();

			access(&puck->eulerDeg, sizeof(CVector));

			// The disasm saves only the first byte of the reflection angle
			byte reflectionRaw[sizeof(float)];

			memcpy(reflectionRaw, &puck->reflectionAngle, sizeof(reflectionRaw));
			access(&reflectionRaw[0], sizeof(byte));
			writeSavedFlag(access, puck->constrained);
			access(puck->constraint, sizeof(puck->constraint));
			access(puck->lineA, sizeof(puck->lineA));
			access(puck->lineB, sizeof(puck->lineB));
		}
	}

	access(&drawFlags, sizeof(drawFlags));

	return true;
}

bool Actor::restore(tSaveRestoreAccess *access) {
	CVector savedVector;

	if (!readSavedString(access, actorName))
		return false;

	access(&id, sizeof(id));
	access(&actorFlags, sizeof(actorFlags));
	access(&mode, sizeof(mode));
	access(&savedVector, sizeof(savedVector));
	setLocalPosition(savedVector);
	access(&savedVector, sizeof(savedVector));
	restoreNodeEuler(this, savedVector);
	access(&prevPos, sizeof(prevPos));
	access(&reserved312, sizeof(reserved312));
	onScreen = readSavedFlag(access);
	access(&sortOrder, sizeof(sortOrder));
	access(&parentActorId, sizeof(parentActorId));

	if (parentActorId)
		access(parentNodeName, CNode::kNameSize);

	for (int i = 0; i < 4; i++)
		access(&drawnRect[i], sizeof(int));

	LString savedRoomName;

	if (!readSavedString(access, savedRoomName))
		return false;

	if (savedRoomName == kOverworldRoomName)
		Overworld::getInstance()->putActor(this);
	else if (!savedRoomName.empty())
		putInSet(savedRoomName.c_str());
	else
		putInSet(nullptr);

	uint choreCount = 0;

	access(&choreCount, sizeof(choreCount));

	for (uint i = 0; i < choreCount; i++) {
		uint choreId = 0;

		access(&choreId, sizeof(choreId));

		if (ChoreInstance::isInstanceValid(choreId)) {
			ChoreInstance *existing = ChoreInstance::getInstance(choreId);

			if (existing)
				delete existing;
		}

		choreId = ChoreInstance::newInstanceWithId(this, choreId);

		if (ChoreInstance::isInstanceValid(choreId)) {
			ChoreInstance *instance = ChoreInstance::getInstance(choreId);

			if (instance)
				instance->restore(access);
		}

		choreIds.push_back(choreId);
	}

	for (int i = 0; i < kNumChoreSlots; i++) {
		if (!readSavedString(access, choreSlotNames[i]))
			return false;

		if (!readSavedString(access, choreSlotSetNames[i]))
			return false;

		access(&choreSlotIds[i], sizeof(uint));
	}

	uint primitiveCount = 0;

	access(&primitiveCount, sizeof(primitiveCount));

	for (uint i = 0; i < primitiveCount; i++) {
		LString primitiveName;
		bool found = false;

		if (!readSavedString(access, primitiveName))
			return false;

		for (uint j = 0; j < instances.size() && !found; j++) {
			RPrimitiveInstance *instance = instances[j];

			if (instance->prim && instance->prim->name == primitiveName && !instance->stateRestored) {
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
			GUI::displayErrorDialog(Common::U32String(message));
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

		bool locked = room.lock();
		Room_v2 *lockedRoom = locked ? room.typed : nullptr;

		if (lockedRoom)
			activateShadow(true, lockedRoom->findShadow(shadowName));
		else
			warning("Actor::restore(): the room could not be locked");

		if (locked)
			room.unlock();
	}

	if (readSavedFlag(access)) {
		extra = nullptr;
		allocateActorExtra();

		extra->lookAtActive = readSavedFlag(access);
		extra->lookAtHasTarget = readSavedFlag(access);
		access(&extra->lookTarget, sizeof(CVector));
		access(&extra->lookOffset, sizeof(CVector));
		access(&extra->lookTargetActorId, sizeof(int));
		access(&extra->lookGoalQuat, sizeof(CQuaternion));
		access(&extra->lookDirection, sizeof(CVector));
		access(&extra->lookCurQuat, sizeof(CQuaternion));
		access(extra->lookLimits, Extra_v2::kNumLookLimits * sizeof(CVector));

		if (!readSavedString(access, extra->lookJointName))
			return false;

		access(&extra->lookCosFov, sizeof(float));
		access(&extra->lookRate, sizeof(float));
		extra->walking = readSavedFlag(access);

		// The disasm restores only the first byte of turnDir
		byte turnDirLow = 0;

		access(&turnDirLow, sizeof(turnDirLow));
		extra->turnDir = (extra->turnDir & ~0xFF) | turnDirLow;
		extra->resting = readSavedFlag(access);
		access(&extra->walkRate, sizeof(float));
		access(&extra->turnRate, sizeof(float));
		extra->aligned = readSavedFlag(access);
		extra->snapTurn = readSavedFlag(access);
		access(&extra->offsetYaw, sizeof(float));
		access(&extra->alignOff, sizeof(float));
		access(&extra->walkTarget, sizeof(CVector));
		access(&extra->goal, sizeof(CVector));
		access(&extra->numPathPoints, sizeof(int));
		access(extra->path, Extra_v2::kMaxPathPoints * sizeof(CVector));
		access(&extra->pathIndex, sizeof(int));

		bool hasPuck = readSavedFlag(access);

		extra->inBoxes = readSavedFlag(access);
		extra->walkDominate = readSavedFlag(access);
		access(&extra->reflectionAngle, sizeof(float));
		extra->talking = readSavedFlag(access);
		access(&extra->talkTimerMs, sizeof(int));
		access(extra->sayLineId, Extra_v2::kSayLineIdSize);
		access(&extra->textEventId, sizeof(int));
		extra->lipsyncActive = readSavedFlag(access);
		access(&extra->soundHandle, sizeof(int));
		access(&extra->visemeCurrent, sizeof(int));
		access(&extra->collisionMode, sizeof(int));
		access(&extra->collisionScale, sizeof(float));
		extra->inCollision = readSavedFlag(access);
		access(&extra->talkColor, sizeof(Color));
		access(&extra->timeScale, sizeof(float));

		if (hasPuck) {
			Puck_v2 *puck = new Puck_v2(extra->reflectionAngle);
			CVector puckVector;

			extra->puck = puck;
			puck->actorName = actorName;

			access(&puckVector, sizeof(puckVector));
			puck->setLocalPosition(puckVector);
			access(&puckVector, sizeof(puckVector));
			restoreNodeEuler(puck, puckVector);

			byte reflectionRaw[sizeof(float)];

			memcpy(reflectionRaw, &puck->reflectionAngle, sizeof(reflectionRaw));
			access(&reflectionRaw[0], sizeof(byte));
			memcpy(&puck->reflectionAngle, reflectionRaw, sizeof(reflectionRaw));
			puck->constrained = readSavedFlag(access);
			access(puck->constraint, sizeof(puck->constraint));
			access(puck->lineA, sizeof(puck->lineA));
			access(puck->lineB, sizeof(puck->lineB));

			if (extra->inBoxes && room.resource) {
				bool locked = room.lock();
				Room_v2 *lockedRoom = locked ? room.typed : nullptr;

				if (lockedRoom)
					puck->validate(lockedRoom->walkPlanes);
				else
					warning("Actor::restore(): the room could not be locked");

				if (locked)
					room.unlock();
			}
		}

		if (extra->talking) {
			for (int i = 0; i < kNumSpeakers; i++) {
				if (!_speakers[i]) {
					_speakers[i] = this;
					_numSpeakers++;

					break;
				}
			}
		}
	}

	access(&drawFlags, sizeof(drawFlags));

	return true;
}

void Actor::saveActors(tSaveRestoreAccess *access) {
	int selectedId = -1;

	if (_selectedActor)
		selectedId = _selectedActor->id;

	access(&selectedId, sizeof(selectedId));

	int actorCount = (int)_actors.size();

	access(&actorCount, sizeof(actorCount));

	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it)
		(*it)->save(access);

	access(&ChoreInstance::_nextId, sizeof(ChoreInstance::_nextId));
	access(&_nextId, sizeof(_nextId));
}

void Actor::restoreActors(tSaveRestoreAccess *access) {
	for (int i = 0; i < kNumSpeakers; i++)
		_speakers[i] = nullptr;

	_numSpeakers = 0;

	int selectedId = 0;
	int actorCount = 0;

	access(&selectedId, sizeof(selectedId));
	access(&actorCount, sizeof(actorCount));

	for (int i = 0; i < actorCount; i++) {
		Actor *actor = new Actor("Restored");

		actor->restore(access);
	}

	access(&ChoreInstance::_nextId, sizeof(ChoreInstance::_nextId));
	access(&_nextId, sizeof(_nextId));

	_selectedActor = nullptr;

	if (selectedId >= 0)
		_selectedActor = actorDeref(selectedId);

	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		Actor *actor = *it;

		if (!actor->parentActorId)
			continue;

		Actor *parentActor = actorDeref(actor->parentActorId);

		if (parentActor) {
			char nodeName[CNode::kNameSize + 1];

			memcpy(nodeName, actor->parentNodeName, CNode::kNameSize);
			nodeName[CNode::kNameSize] = 0;

			CNode *parentNode = parentActor->findNode(nodeName);

			if (!parentNode)
				parentNode = parentActor;

			actor->setParent(parentNode);
		} else {
			actor->parentActorId = 0;
			actor->parentNodeName[0] = 0;
		}
	}
}

} // End of namespace Grim
