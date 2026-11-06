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

#ifndef GRIM_SPUTACTOR_H
#define GRIM_SPUTACTOR_H

#include "common/scummsys.h"

#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"
#include "engines/grim/std/std_display.h"

namespace Grim {

class Puck;
struct ModelInst;

struct CostumeNode {
	int hCostumeInstance;
	RdThing *pHeadThing;
	ModelInst *pHeadList;
	int aHeadJoint[3];
	float headTurnRate;
	float headYawLimit;
	float headPitchLimit;
	float headLookRef;
	int walkChore;
	int turnLeftChore;
	int turnRightChore;
	int restChore;
	int aChore[16];
	int mumbleChore;
	ResRef rCostume;
	CostumeNode *pNext;
};

// TODO: incomplete, the remaining fields come with the sputActor port
struct Extra {
	int numPathPoints;
	RdVector3 aPath[30];
	Puck *puck;
	int inBoxes;
	float collisionScale;
	int field_3F8;
};

struct SputActor {
	ResRef *pColorMap;
	int flags;
	int mode;
	RdVector3 actorPos;
	RdVector3 prevPosition;
	RdVector3 actorRot;
	float scale;
	int matrixValid;
	RdMatrix34 orientMatrix;
	int onScreen;
	StdRect drawnRect;
	char curSetName[32];
	int bInUse;
	CostumeNode *pCostumeStack;
	Extra *pExtra;
};

} // End of namespace Grim

#endif
