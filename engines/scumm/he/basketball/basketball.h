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

#ifndef SCUMM_HE_BASKETBALL_BASKETBALL_H
#define SCUMM_HE_BASKETBALL_BASKETBALL_H

#ifdef ENABLE_HE

#include "scumm/he/intern_he.h"
#include "scumm/he/logic_he.h"
#include "scumm/he/basketball/court.h"

namespace Scumm {

#define BB_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define BB_MAX(a, b) (((a) > (b)) ? (a) : (b))

// If a and b are >= zero, return the minimum of the two.
// If a and b are <= zero, return the greater of the two.
// If only one is >= zero, return that one.
#define BB_MIN_GREATER_THAN_ZERO(a, b) ((a < 0) ? ((b < 0) ? ((a > b) ? a : b) : b) : ((b < 0) ? a : ((a > b) ? b : a)))

// GEOMETRIC TRANSLATION DEFINES
	
#define SCREEN_SIZE_Y				640

#define COURT_X_OFFSET				18		// pixels from left of screen to beginning of court.
#define COURT_Y_OFFSET				33      // pixels from bottom of screen to beginning of court.      

#define TRANSLATED_MAX_Y			302		// pixels from bottom of court to top of court.
#define TRANSLATED_MID_Y            186     // pixels from bottom of the court to center court.
#define TRANSLATED_DIAGONAL_Y       431     // pixel length of the baseline. 
#define TRANSLATED_MAX_START_X		308		// this is how far over x = 0 is pushed when y = MAX-WORLD-Y	
#define TRANSLATED_FAR_MAX_X		950		// length of the far side of field ( world-y = MAX-WORLD-Y) when translated
#define TRANSLATED_NEAR_MAX_X		1564	// length of near side of field (world-y = MAX-WORLD-Y when translated

// length and width of the court in game world units
#define WORLD_UNIT_MULTIPLIER       160
#define MAX_WORLD_X					(75 * WORLD_UNIT_MULTIPLIER)
#define MAX_WORLD_Y 				(50 * WORLD_UNIT_MULTIPLIER)

// Coordinates of the center of the hoop.
#define BASKET_PUSH_BACK_DIST (0.5 * WORLD_UNIT_MULTIPLIER) // This is a hack to make up for our bad translations.
 
#define BASKET_X              (int)((5.25 * WORLD_UNIT_MULTIPLIER) - BASKET_PUSH_BACK_DIST)
#define BASKET_Y              (25 * WORLD_UNIT_MULTIPLIER)
#define BASKET_Z              (10 * WORLD_UNIT_MULTIPLIER)

// The screen coordinate that sandwich where on the screen scaling occurs.
// These coordinates are given in pixels from the bottom of the court
#define TOP_SCALING_PIXEL_CUTOFF    (1000 - COURT_Y_OFFSET) 
#define BOTTOM_SCALING_PIXEL_CUTOFF (0 - COURT_Y_OFFSET)


// COLLISION DEFINES

#define RIM_CE                (float).4
#define BACKBAORD_CE          (float).3
#define FLOOR_CE              (float).65

#define LEFT_BASKET           0
#define RIGHT_BASKET          1

#define RIM_WIDTH             (WORLD_UNIT_MULTIPLIER / 11)
#define RIM_RADIUS            ((3 * WORLD_UNIT_MULTIPLIER) / 4)

#define MAX_BALL_COLLISION_PASSES   10
#define MAX_PLAYER_COLLISION_PASSES 3
#define COLLISION_EPSILON           0.5F


// AVOIDANCE DEFINES

#define AVOIDANCE_LOOK_AHEAD_DISTANCE  4000
#define OBSTACLE_AVOIDANCE_DISTANCE    75
#define AVOIDANCE_SAFETY_DISTANCE      1
#define MAX_AVOIDANCE_ANGLE_SIN        1.0F

class LogicHEBasketball;

class Basketball {

public:
	Basketball(ScummEngine_v100he *vm);
	~Basketball();

	int U32_Float_To_Int(float input);
	int U32_Double_To_Int(double input);

	int NumOpponentsInCone(int team, float width_distance_ratio, const U32FltVector2D &end, const U32FltVector2D &focus);
	float Congestion(U32FltVector2D pos, bool ignore, int whichPlayer);
	void FillPlayerTargetList(const CCollisionPlayer *pSourceObject, CCollisionObjectVector *pTargetList);
	void FillBallTargetList(const CCollisionSphere *pSourceObject, CCollisionObjectVector *pTargetList);

	CBBallCourt g_court;
	CCollisionShieldVector g_shields;

private:
	ScummEngine_v100he *_vm;

};

} // End of namespace Scumm

#endif // ENABLE_HE

#endif // SCUMM_HE_BASKETBALL_BASKETBALL_H
