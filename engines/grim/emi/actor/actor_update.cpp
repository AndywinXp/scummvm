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
#include "engines/grim/emi/anode.h"
#include "engines/grim/emi/camera.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/overworld.h"

namespace Grim {

void Actor::updateAllActors(EmiEngine *engine, float deltaTime) {
	engine->_updateAllActorsTimer.start();

	engine->_collisionCount = 0;

	Room_v2 *currentRoom = Room_v2::getCurrentRoom(engine);

	if (!currentRoom) {
		engine->_updateAllActorsTimer.stop();

		return;
	}

	updateSpeechOnly(engine, deltaTime);

	Common::List<Actor *>::iterator it;

	for (it = currentRoom->actors.begin(); it != currentRoom->actors.end(); ++it) {
		Actor *current = *it;

		if (!current)
			continue;

		if (current->extra)
			current->extra->inCollision = false;

		if (!current->instances.empty()) {
			if (engine->_collisionCount < kMaxCollisionActors) {
				engine->_collisionActors[engine->_collisionCount] = current;
				engine->_collisionCount++;
			} else {
				warning("Actor::updateAllActors(): too many actors in the collision list");
			}
		}

		Camera *setupCamera = currentRoom->getSetupCamera(-1);

		if (setupCamera)
			current->onScreen = current->calcOnScreen(setupCamera);

		current->update(deltaTime);
	}

	Overworld *overworld = engine->getOverworld();

	for (it = overworld->actors.begin(); it != overworld->actors.end(); ++it) {
		Actor *current = *it;

		if (!current)
			continue;

		if (!current->instances.empty()) {
			if (engine->_collisionCount < kMaxCollisionActors) {
				engine->_collisionActors[engine->_collisionCount] = current;
				engine->_collisionCount++;
			} else {
				warning("Actor::updateAllActors(): too many actors in the collision list");
			}
		}

		current->onScreen = true;
		current->update(deltaTime);
	}

	updatePositions(engine);
	engine->_updateAllActorsTimer.stop();
}

void Actor::updateSpeechOnly(EmiEngine *engine, float deltaTime) {
	for (int i = 0; i < kNumSpeakers; i++) {
		if (engine->_speakers[i])
			engine->_speakers[i]->updateActorSpeech(deltaTime);
	}
}

void Actor::update(float deltaTime) {
	if (extra)
		deltaTime = deltaTime * extra->timeScale;

	if ((mode & 1) != 0)
		followPath(deltaTime);

	if (extra && extra->puck)
		updateWalk(deltaTime);

	removeFinishedChores();
	updateWalkChores();

	Common::List<uint>::iterator choreIt;

	for (choreIt = choreIds.begin(); choreIt != choreIds.end(); ++choreIt) {
		if (ChoreInstance::isInstanceValid(_engine, *choreIt)) {
			ChoreInstance *instance = ChoreInstance::getInstance(_engine, *choreIt);

			instance->update(deltaTime, false);
		}
	}

	if (onScreen) {
		Common::List<ANode *>::iterator nodeIt;

		for (nodeIt = animNodes.begin(); nodeIt != animNodes.end(); ++nodeIt)
			(*nodeIt)->updateANode();

		if (extra && extra->lookAtActive)
			setLookAtJoints(deltaTime);
	}
}

bool Actor::updatePositions(EmiEngine *engine) {
	for (int i = 0; i < engine->_collisionCount; i++) {
		Actor *current = engine->_collisionActors[i];

		if (current->extra) {
			if ((current->nodeFlags & kWorldValid) == 0)
				current->updateWorldTransform();

			// The disasm treats an unordered comparison as equal
			bool moved = (current->world.pos.x < current->prevPos.x || current->world.pos.x > current->prevPos.x)
				|| (current->world.pos.y < current->prevPos.y || current->world.pos.y > current->prevPos.y)
				|| (current->world.pos.z < current->prevPos.z || current->world.pos.z > current->prevPos.z);

			if (moved)
				current->resolveCollisions();
		}
	}

	return true;
}

} // End of namespace Grim
