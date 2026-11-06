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

#ifndef GRIM_SPUTROOM_H
#define GRIM_SPUTROOM_H

#include "common/scummsys.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"

namespace Grim {

struct SputObjectState;

// TODO: incomplete, the remaining fields come with the sputRoom port
struct RoomCamera {
	RdVector3 position;
	RdVector3 interest;
	RdCamera *prdCamera;
	RdMatrix34 posMatrix;
};

// TODO: incomplete, the remaining fields come with the sputRoom port
struct RoomSetup {
	RoomCamera camera;
	ResRef bitmapRef;
	ResRef zbufferRef;
	SputObjectState *ObjectStates;
};

struct RoomLight {
	char aName[32];
	RdVector3 position;
	RdLight light;
};

struct RoomActorLink {
	int actorIndex;
	RoomActorLink *pPrev;
	RoomActorLink *pNext;
};

// TODO: incomplete, the remaining fields come with the sputRoom port
struct RoomActorNode {
	RoomActorLink *pActors;
};

// TODO: incomplete, the remaining fields come with the sputRoom port
struct Room {
	ResRef *apColormaps;
	int field_88;
	RoomSetup *pSetups;
	uint numSetups;
	int curSetup;
	uint numLights;
	RoomLight aLights[64];
	RoomActorNode *pActorNode;
};

} // End of namespace Grim

#endif
