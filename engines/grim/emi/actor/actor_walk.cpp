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

#include "common/textconsole.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/choreinstance.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/puck_v2.h"
#include "engines/grim/emi/walkplane_v2.h"
#include "engines/grim/emi/walkplanecollection.h"

namespace Grim {

void Actor::followPath(float deltaTime) {
	const float maxAngle = 3.4028235e38f;

	if (!extra) {
		warning("Actor::followPath(): missing extra data");

		return;
	}

	if ((nodeFlags & kWorldValid) == 0)
		updateWorldTransform();

	double deltaX = (double)world.pos.x - extra->walkTarget.x;
	double deltaY = (double)world.pos.y - extra->walkTarget.y;
	double deltaZ = (double)world.pos.z - extra->walkTarget.z;
	double distance = sqrt((deltaZ * deltaZ + deltaY * deltaY) + deltaX * deltaX);

	float rate = extra->walkRate;

	if (rate < 0.0f)
		rate = -rate;

	float limit = (float)((double)rate * extra->timeScale * deltaTime * 1.25);

	// The disasm takes the branches below also when the comparisons are unordered
	if (!(distance >= limit)) {
		do {
			if (extra->pathIndex >= extra->numPathPoints)
				break;

			extra->pathIndex++;
			setActorPosition(&extra->walkTarget.x);

			if (extra->pathIndex < Extra_v2::kMaxPathPoints)
				extra->walkTarget = extra->path[extra->pathIndex];
			else
				warning("Actor::followPath(): path index out of range");

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			deltaY = (double)world.pos.y - extra->walkTarget.y;
			deltaZ = (double)world.pos.z - extra->walkTarget.z;
			deltaX = (double)world.pos.x - extra->walkTarget.x;
			distance = sqrt((deltaZ * deltaZ + deltaY * deltaY) + deltaX * deltaX);
		} while (!(distance >= limit));

		if (extra->pathIndex == extra->numPathPoints) {
			mode &= ~1U;

			if (ChoreInstance::isInstanceValid(choreSlotIds[1]))
				ChoreInstance::getInstance(choreSlotIds[1])->stop();

			choreSlotIds[1] = 0;

			return;
		}
	}

	mode |= 1U;

	if (extra->puck)
		extra->puck->pointAt(&extra->walkTarget.x, maxAngle);
	else
		warning("Actor::followPath(): missing puck");

	double cosLimit = cos((double)kDegreesToRadians * 80.0);
	double cosAlign = cos((double)kDegreesToRadians * extra->alignOff);

	if (!(cosLimit >= cosAlign))
		forward();
}

void Actor::updateWalk(float deltaTime) {
	Puck_v2 *puck = extra->puck;
	int turnDirection = extra->turnDir;

	if (turnDirection != 0 && (mode & 1) == 0)
		puck->rotate((float)-((double)deltaTime * extra->timeScale * extra->turnRate * (double)turnDirection));

	if (extra->walking) {
		float distance = (float)((double)extra->timeScale * extra->walkRate * deltaTime);
		int allowLeft = 1;
		int allowRight = 1;

		if (!extra->walkDominate) {
			allowLeft = extra->turnDir != -1;
			allowRight = extra->turnDir != 1;
		}

		extra->walking = puck->forward(distance, allowLeft, allowRight);
	}

	extra->aligned = alignToPuck(puck);

	if (extra->turnDir == 0 && !extra->aligned) {
		if (extra->alignOff > 180.0f)
			extra->turnDir = 1;
		else
			extra->turnDir = -1;
	}

	if ((puck->nodeFlags & kPositionValid) == 0)
		puck->computeLocalPositionFromWorld();

	setLocalPosition(puck->pos);
}

bool Actor::alignToPuck(Puck_v2 *puck) {
	if ((nodeFlags & kEulerValid) == 0)
		computeLocalMatrixFromWorld();

	float savedX = eulerDeg.x;
	float savedY = eulerDeg.y;
	float savedZ = eulerDeg.z;

	if ((puck->nodeFlags & kEulerValid) == 0)
		puck->computeLocalMatrixFromWorld();

	double difference = puck->eulerDeg.y;

	if (difference - difference != 0.0 || (double)savedY - savedY != 0.0) {
		warning("Actor::alignToPuck(): invalid yaw");
		difference = 0.0;
		savedY = 0.0f;
	}

	while (difference < savedY)
		difference += 360.0;

	difference = difference - savedY;

	while (difference >= 360.0)
		difference -= 360.0;

	double sum = difference + extra->offsetYaw;
	float alignAngle = (float)sum;

	// The disasm compares the sum before it is rounded to float
	if (!(sum >= 0.5) || alignAngle > 359.5f) {
		extra->snapTurn = false;
		extra->alignOff = 0.0f;

		return true;
	}

	double turnStep = 360.0;

	extra->alignOff = alignAngle;

	if (!extra->snapTurn) {
		float frameTime = EmiEngine::getFrameTime();

		turnStep = (double)frameTime * extra->timeScale * extra->turnRate;
	}

	bool reversed = false;

	if (alignAngle > 180.0f) {
		reversed = true;
		alignAngle = 360.0f - alignAngle;
	}

	if (!(turnStep + 0.01f >= alignAngle))
		alignAngle = (float)(((double)alignAngle - turnStep) * 0.1f + turnStep);

	double direction = reversed ? -1.0 : 1.0;

	eulerDeg.x = savedX;
	eulerDeg.z = savedZ;
	eulerDeg.y = (float)(direction * alignAngle + savedY);

	nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
	updateLocalState();

	// The disasm returns the previous value of snapTurn
	bool previousSnapTurn = extra->snapTurn;

	extra->snapTurn = false;

	return previousSnapTurn;
}

void Actor::turn(float degrees) {
	if (!extra) {
		warning("Actor::turn(): missing extra data");

		return;
	}

	if (!extra->puck)
		enablePuck(true);

	if (degrees > 0.0f)
		extra->turnDir = 1;
	else
		extra->turnDir = -1;
}

void Actor::forward() {
	if (!extra) {
		warning("Actor::forward(): missing extra data");

		return;
	}

	if (!extra->puck)
		enablePuck(true);

	extra->walking = true;
}

void Actor::enableWalkSystem(bool enable) {
	if (!extra)
		allocateActorExtra();

	if (enable) {
		extra->inBoxes = true;
		enablePuck(true);

		if (room.resource) {
			Room_v2 *lockedRoom = nullptr;

			if (room.lock())
				lockedRoom = room.typed;

			WalkPlaneCollection *collection = nullptr;

			if (lockedRoom)
				collection = lockedRoom->walkPlanes;
			else
				warning("Actor::enableWalkSystem(): the room could not be locked");

			extra->puck->validate(collection);

			if (lockedRoom)
				room.unlock();
		}
	} else {
		extra->inBoxes = false;

		if (extra->puck)
			extra->puck->plane = nullptr;
	}
}

void Actor::enablePuck(bool enable) {
	if (!extra)
		allocateActorExtra();

	if (enable) {
		if (extra->puck) {
			extra->puck->reset();
		} else {
			const float maxAngle = 3.4028235e38f;
			float target[3];

			Puck_v2 *newPuck = new Puck_v2(extra->reflectionAngle);

			newPuck->actorName = actorName;

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			newPuck->setLocalPosition(world.pos);

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			target[0] = world.rot.m[6];
			target[1] = world.rot.m[7];
			target[2] = world.rot.m[8];

			if ((nodeFlags & kWorldValid) == 0)
				updateWorldTransform();

			target[0] = target[0] + world.pos.x;
			target[1] = target[1] + world.pos.y;
			target[2] = target[2] + world.pos.z;

			newPuck->pointAt(target, maxAngle);
			extra->puck = newPuck;
		}
	} else if (extra->puck) {
		delete extra->puck;
		extra->puck = nullptr;
	}
}

void Actor::setActorPosition(const float *position) {
	if (extra && extra->puck) {
		Puck_v2 *puck = extra->puck;
		CVector newPosition;

		puck->reset();

		newPosition.x = position[0];
		newPosition.y = position[1];
		newPosition.z = position[2];
		puck->setLocalPosition(newPosition);

		if (extra->inBoxes) {
			Room_v2 *lockedRoom = nullptr;

			if (room.lock())
				lockedRoom = room.typed;

			WalkPlaneCollection *collection = nullptr;

			if (lockedRoom) {
				collection = lockedRoom->walkPlanes;
				room.unlock();
			} else {
				warning("Actor::setActorPosition(): the room could not be locked");
			}

			puck->validate(collection);
		}

		if ((puck->nodeFlags & kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		setLocalPosition(puck->pos);
	} else {
		CVector newPosition;

		newPosition.x = position[0];
		newPosition.y = position[1];
		newPosition.z = position[2];
		setLocalPosition(newPosition);
	}
}

void Actor::setActorRotation(const float *rotation, bool snap) {
	mode &= ~1U;

	if (extra && extra->puck) {
		Puck_v2 *puck = extra->puck;

		puck->eulerDeg.y = rotation[1];
		puck->eulerDeg.x = 0.0f;
		puck->eulerDeg.z = 0.0f;
		puck->nodeFlags = (puck->nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
		puck->updateLocalState();

		extra->snapTurn = snap;

		if ((nodeFlags & kEulerValid) == 0)
			computeLocalMatrixFromWorld();

		extra->alignOff = rotation[1] - eulerDeg.y;

		if ((nodeFlags & kEulerValid) == 0)
			computeLocalMatrixFromWorld();

		float currentYaw = eulerDeg.y;

		eulerDeg.x = rotation[0];
		eulerDeg.y = currentYaw;
		eulerDeg.z = rotation[2];
		nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
		updateLocalState();
	} else {
		eulerDeg.x = rotation[0];
		eulerDeg.y = rotation[1];
		eulerDeg.z = rotation[2];
		nodeFlags = (nodeFlags & ~kQuaternionValid) | kEulerValid | kLocalSet;
		updateLocalState();
	}
}

void Actor::setReflection(float angle) {
	if (!extra) {
		warning("Actor::setReflection(): missing extra data");

		return;
	}

	if (extra->puck)
		extra->puck->reflectionAngle = angle;
}

void Actor::setWalkDominate(bool dominate) {
	if (!extra) {
		warning("Actor::setWalkDominate(): missing extra data");

		return;
	}

	extra->walkDominate = dominate;
}

void Actor::leaveSector(WalkPlane_v2 *plane) {
	if (!plane)
		return;

	if (!extra) {
		warning("Actor::leaveSector(): missing extra data");

		return;
	}

	Puck_v2 *puck = extra->puck;

	if (puck) {
		plane->jumpOut(puck);

		if ((puck->nodeFlags & kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		setActorPosition(&puck->pos.x);
	}
}

void Actor::actorWalkTo(float *target, const float *goal, Actor *other) {
	// The disasm's output buffer holds only 30 points but the avoiding path can grow past that, let's give it room
	enum {
		kOutputPathCapacity = 100
	};

	float inputPath[Extra_v2::kMaxPathPoints][3];
	float outputPath[kOutputPathCapacity][3];
	float start[3];

	memset(inputPath, 0, sizeof(inputPath));
	memset(outputPath, 0, sizeof(outputPath));

	mode &= ~1U;

	if (!extra) {
		warning("Actor::actorWalkTo(): missing extra data");

		return;
	}

	if (extra->puck) {
		Room_v2 *lockedRoom = nullptr;

		if (room.lock())
			lockedRoom = room.typed;

		WalkPlaneCollection *collection = nullptr;

		if (lockedRoom) {
			collection = lockedRoom->walkPlanes;
			room.unlock();
		} else {
			warning("Actor::actorWalkTo(): the room could not be locked");
		}

		if (!collection) {
			warning("Actor::actorWalkTo(): the room has no walk planes");

			return;
		}

		Puck_v2 *puck = extra->puck;
		WalkPlane_v2 *plane = collection->planeClosest(target, 0x1000);

		if (plane)
			plane->closestPoint(target);

		if ((puck->nodeFlags & kPositionValid) == 0)
			puck->computeLocalPositionFromWorld();

		start[0] = puck->pos.x;
		start[1] = puck->pos.y;
		start[2] = puck->pos.z;

		if (collection->buildPath(start, target, true)) {
			float (*pathPoints)[3];
			int pointCount = WalkPlaneCollection::getPath(&pathPoints);

			for (int i = 0; i < pointCount; i++) {
				inputPath[i][0] = pathPoints[i][0];
				inputPath[i][1] = pathPoints[i][1];
				inputPath[i][2] = pathPoints[i][2];
			}

			int newCount = buildAvoidingPath(other, &inputPath[0][0], pointCount, &outputPath[0][0]);

			if (newCount > Extra_v2::kMaxPathPoints) {
				warning("Actor::actorWalkTo(): too many path points");
				newCount = Extra_v2::kMaxPathPoints;
			}

			extra->numPathPoints = newCount;
			extra->pathIndex = 1;

			for (int i = 0; i < newCount; i++) {
				extra->path[i].x = outputPath[i][0];
				extra->path[i].y = outputPath[i][1];
				extra->path[i].z = outputPath[i][2];
			}

			extra->walkTarget = extra->path[extra->pathIndex];

			if (newCount > 1)
				mode |= 1U;
		}

		extra->goal.x = goal[0];
		extra->goal.y = goal[1];
		extra->goal.z = goal[2];
	}
}

} // End of namespace Grim
