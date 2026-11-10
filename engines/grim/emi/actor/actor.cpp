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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/anode.h"
#include "engines/grim/emi/cdcvertexarray.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/lipdata.h"
#include "engines/grim/emi/lresource.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/puck_v2.h"
#include "engines/grim/emi/walkplanecollection.h"

namespace Grim {

const float Actor::kDegreesToRadians = 3.14159274f * 0.00555555569f;

int Actor::_nextId = 1;
Common::List<Actor *> Actor::_actors;
Actor *Actor::_selectedActor = nullptr;
Actor *Actor::_speakers[Actor::kNumSpeakers];
int Actor::_numSpeakers = 0;
int Actor::_speechMode = 2;
int Actor::_actorTextRate = 6;
IRect Actor::offscreenTextRect;
Actor *Actor::_collisionActors[Actor::kMaxCollisionActors];
int Actor::_collisionCount = 0;
CDrawContext *Actor::_drawContext = nullptr;
CDCVertexArray *Actor::_shadowVertices = nullptr;
CVector Actor::_hitPoint;
CVector Actor::_collisionOldPosition;
CVector Actor::_collisionNewPosition;

Actor::Actor(const char *newActorName) {
	prevPos.x = 0.0f;
	prevPos.y = 0.0f;
	prevPos.z = 0.0f;
	drawnRect[0] = 0;
	drawnRect[1] = 0;
	drawnRect[2] = 0;
	drawnRect[3] = 0;
	memset(choreSlotIds, 0, kNumChoreSlots * sizeof(uint));

	id = _nextId++;
	_actors.push_back(this);

	if (newActorName)
		actorName = newActorName;
	else
		warning("Actor::Actor(): the actor name is null");

	actorFlags = 0;
	mode = 0;
	reserved312 = 0.0f;
	onScreen = false;
	inOverworld = false;
	room.setResource(nullptr);
	sortOrder = 0;
	parentActorId = 0;
	memset(parentNodeName, 0, sizeof(parentNodeName));
	drawFlags |= 0x21000;

	for (int i = 0; i < kNumChoreSlots; i++) {
		choreSlotIds[i] = 0;
		choreSlotNames[i].clear();
		choreSlotSetNames[i].clear();
	}

	extra = nullptr;
	Common::sprintf_s(name, "Actor %d", id);
}

Actor::~Actor() {
	if (room.resource) {
		Room_v2 *currentRoom = nullptr;

		if (room.lock())
			currentRoom = room.typed;

		if (currentRoom)
			currentRoom->removeActor(this);
		else
			warning("Actor::~Actor(): the room could not be locked");

		room.unlock();
		room.unlock();
	}

	Common::List<uint>::iterator choreIt;

	for (choreIt = choreIds.begin(); choreIt != choreIds.end(); ++choreIt) {
		if (ChoreInstance::isInstanceValid(*choreIt)) {
			ChoreInstance *instance = ChoreInstance::getInstance(*choreIt);

			if (instance)
				delete instance;
		}
	}

	choreIds.clear();

	if (extra) {
		if (extra->puck) {
			delete extra->puck;
			extra->puck = nullptr;
		}

		delete extra;
		extra = nullptr;
	}

	Common::List<Actor *>::iterator actorIt = _actors.begin();

	while (actorIt != _actors.end()) {
		if (*actorIt == this)
			actorIt = _actors.erase(actorIt);
		else
			++actorIt;
	}

	// The disasm leaves the actor in the overworld list and in the selected/speaker slots
}

int Actor::allocateActor(const char *newActorName) {
	Actor *actor = new Actor(newActorName);

	return actor->id;
}

void Actor::freeActor(int actorId) {
	Actor *actor = actorDeref(actorId);

	if (actor)
		delete actor;
}

void Actor::allocateActorExtra() {
	extra = new Extra_v2();
}

bool Actor::putInSet(const char *roomName) {
	if (room.resource) {
		activateShadow(false, nullptr);

		for (int removedBefore = 0;; removedBefore++) {
			Room_v2 *currentRoom = nullptr;

			if (room.lock())
				currentRoom = room.typed;

			int removed = 0;

			if (currentRoom)
				removed = currentRoom->removeActor(this);
			else
				warning("Actor::putInSet(): the room could not be locked");

			bool removedAgain = removedBefore < removed;

			if (currentRoom)
				room.unlock();

			if (!removedAgain)
				break;

			room.unlock();
		}

		room.setResource(nullptr);
	}

	if (!roomName) {
		removeFinishedChores();

		return true;
	}

	room.resource = gpResourceManager->findResource(roomName);

	if (!room.resource) {
		room.resource = gpResourceManager->createResource(roomName);

		if (!room.resource)
			room.allocateResource();

		if (!room.resource) {
			warning("Actor::putInSet(): could not create the room resource '%s'", roomName);

			return false;
		}

		room.resource->resSetName(roomName);
		gpResourceManager->addResource(room.resource);
	}

	room.setResource(room.resource);

	if (!room.resource)
		return false;

	if (!room.lock())
		return false;

	Room_v2 *newRoom = room.typed;

	if (!newRoom)
		return false;

	newRoom->addActor(this);

	if (extra && extra->puck && extra->inBoxes) {
		extra->puck->reset();
		extra->puck->validate(newRoom->walkPlanes);
	}

	return true;
}

bool Actor::startup(CDrawContext *drawContext) {
	_drawContext = drawContext;

	if (drawContext)
		_shadowVertices = drawContext->createVertexArray(2000, nullptr, nullptr, nullptr);

	for (int i = 0; i < kNumSpeakers; i++)
		_speakers[i] = nullptr;

	LipData::sortMouthTable();

	return true;
}

void Actor::shutdown() {
	// The disasm leaves dangling actor pointers at shutdown, let's clear them first
	Overworld::getInstance()->clearActors();

	_selectedActor = nullptr;

	for (int i = 0; i < kNumSpeakers; i++)
		_speakers[i] = nullptr;

	_numSpeakers = 0;

	freeAllActors();

	if (_drawContext && _shadowVertices)
		delete _shadowVertices;

	_shadowVertices = nullptr;
	_drawContext = nullptr;

	Overworld::destroyInstance();
}

void Actor::freeAllActors() {
	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it)
		(*it)->detachFromParent();

	it = _actors.begin();

	while (it != _actors.end()) {
		Actor *actor = *it;

		++it;

		if (actor)
			delete actor;
	}

	_actors.clear();
}

int Actor::setSelectedActor(int actorId) {
	int previousId = -1;

	if (_selectedActor)
		previousId = _selectedActor->id;

	if (actorId >= 0)
		_selectedActor = actorDeref(actorId);

	return previousId;
}

Actor *Actor::getSelectedActor() {
	return _selectedActor;
}

Actor *Actor::actorDeref(int actorId) {
	Common::List<Actor *>::iterator it;

	for (it = _actors.begin(); it != _actors.end(); ++it) {
		if (*it && (*it)->id == actorId)
			return *it;
	}

	return nullptr;
}

int Actor::getActorHandle(Actor *actor) {
	if (actor)
		return actor->id;

	return 0;
}

void Actor::attachNode(CNode *node) {
	if (node) {
		registerANodes(node);
		node->setParent(this);
	}
}

void Actor::detachNode(CNode *node) {
	if (node) {
		unregisterANodes(node);
		node->detachFromParent();
	}
}

void Actor::registerANodes(CNode *node) {
	for (CNode *current = node; current; current = current->nextSibling) {
		registerANodes(current->firstChild);

		ANode *animNode = dynamic_cast<ANode *>(current);

		if (animNode)
			animNodes.push_back(animNode);
	}
}

void Actor::unregisterANodes(CNode *node) {
	for (CNode *current = node; current; current = current->nextSibling) {
		unregisterANodes(current->firstChild);

		ANode *animNode = dynamic_cast<ANode *>(current);

		if (animNode) {
			Common::List<ANode *>::iterator it = animNodes.begin();

			while (it != animNodes.end()) {
				if (*it == animNode)
					it = animNodes.erase(it);
				else
					++it;
			}
		}
	}
}

} // End of namespace Grim
