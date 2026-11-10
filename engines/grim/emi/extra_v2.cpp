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

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/extra_v2.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

Extra_v2::Extra_v2() {
	lookAtActive = false;
	lookAtHasTarget = false;
	lookTargetActorId = 0;

	for (int i = 0; i < kNumLookLimits; i++) {
		lookLimits[i].x = 0.0f;
		lookLimits[i].y = 0.0f;
		lookLimits[i].z = 1.0f;
	}

	lookCosFov = 0.5f;
	lookRate = Actor::kDegreesToRadians * 270.0f;
	collisionScale = 1.0f;
	timeScale = 1.0f;
	walkRate = 0.4f;
	turnRate = 135.0f;
	walkDominate = false;
	reflectionAngle = 60.0f;

	Color white;

	white.r = 255;
	white.g = 255;
	white.b = 255;
	talkColor = white;

	walking = false;
	turnDir = 0;
	resting = false;
	aligned = false;
	snapTurn = false;
	offsetYaw = 0.0f;
	alignOff = 0.0f;
	numPathPoints = 0;
	pathIndex = 0;
	puck = nullptr;
	inBoxes = false;
	talking = false;
	soundHandle = 0;
	talkTimerMs = 0;
	sayLineId[0] = 0;
	textEventId = 0;
	lipsyncActive = false;
	visemeCurrent = 0;
	collisionMode = 0;
	inCollision = false;

	// The disasm leaves visemeMs and the tail of sayLineId uninitialized
	visemeMs = 0;

	for (int i = 1; i < kSayLineIdSize; i++)
		sayLineId[i] = 0;
}

void *Extra_v2::operator new(size_t size) {
	return gpMemoryServices->malloc(size);
}

void Extra_v2::operator delete(void *block) {
	gpMemoryServices->free(block);
}

} // End of namespace Grim
