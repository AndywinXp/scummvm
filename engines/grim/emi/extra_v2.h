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

#ifndef GRIM_EMI_EXTRA_V2_H
#define GRIM_EMI_EXTRA_V2_H

#include "engines/grim/emi/color.h"
#include "engines/grim/emi/cquaternion.h"
#include "engines/grim/emi/cvector.h"
#include "engines/grim/emi/lfile.h"

namespace Grim {

class EmiEngine;
class Puck_v2;

class Extra_v2 {
public:
	enum {
		kMaxPathPoints = 30,
		kNumLookLimits = 4,
		kSayLineIdSize = 12
	};

	explicit Extra_v2(EmiEngine *engine);

	void *operator new(size_t size);
	void operator delete(void *block);

	bool lookAtActive;
	bool lookAtHasTarget;
	CVector lookTarget;
	CVector lookOffset;
	int lookTargetActorId;
	CQuaternion lookGoalQuat;
	CVector lookDirection;
	CQuaternion lookCurQuat;
	CVector lookLimits[kNumLookLimits];
	LString lookJointName;
	float lookCosFov;
	float lookRate;
	bool walking;
	int turnDir;
	bool resting;
	float walkRate;
	float turnRate;
	bool aligned;
	bool snapTurn;
	float offsetYaw;
	float alignOff;
	CVector walkTarget;
	CVector goal;
	int numPathPoints;
	CVector path[kMaxPathPoints];
	int pathIndex;
	Puck_v2 *puck;
	bool inBoxes;
	bool walkDominate;
	float reflectionAngle;
	bool talking;
	int soundHandle;
	int talkTimerMs;
	char sayLineId[kSayLineIdSize];
	int textEventId;
	bool lipsyncActive;
	int visemeCurrent;
	int visemeMs;
	int collisionMode;
	float collisionScale;
	bool inCollision;
	Color talkColor;
	float timeScale;

private:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
