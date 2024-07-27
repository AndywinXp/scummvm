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

#ifndef SCUMM_HE_BASKETBALL_COURT_H
#define SCUMM_HE_BASKETBALL_COURT_H

#ifdef ENABLE_HE

#include "scumm/he/intern_he.h"

#include "scumm/he/basketball/basketball.h"
#include "scumm/he/basketball/collision.h"
#include "scumm/he/basketball/geotypes.h"
#include "scumm/he/logic_he.h"

namespace Scumm {

	// COURT DEFINES

#define MAX_PLAYERS_ON_TEAM  5

#define NO_PLAYER           -1
#define FIRST_PLAYER         0
#define AWAY_PLAYER_1        5
#define LAST_PLAYER          9

#define TEAM_HOME            0
#define TEAM_AWAY            1

#define SHOT_SPOT_RADIUS     13

#define NO_COURT             0
#define COURT_DOBBAGUCHI     1
#define COURT_SMITH_BROS     2
#define COURT_SANDY_FLATS    3
#define COURT_QUEENS         4
#define COURT_PLAYGROUND     5
#define COURT_SCHEFFLER      6
#define COURT_POLK           7
#define COURT_MCMILLAN       8
#define COURT_CROWN_HILL     9
#define COURT_MEMORIAL       10
#define COURT_TECH_STATE     11
#define COURT_HUMONGOUS      12
#define COURT_MOON           13
#define COURT_BARN           14
#define COURT_COUNT          15

class CBBallCourt {
public:
	CBBallCourt() { m_objectCount = 0; }
	~CBBallCourt() {}

	int GetPlayerIndex(int playerID);
	CCollisionPlayer *GetPlayerPtr(int playerID);
	CCollisionBasketball *GetBallPtr(int ballID);

	Common::Array<CCollisionPlayer> *GET_PLAYER_LIST_PTR(int i) {
		return (i < AWAY_PLAYER_1) ? &m_homePlayerList : &m_awayPlayerList;
	}

	Common::Array<CCollisionPlayer> *GET_OPPONENT_LIST_PTR(int i) {
		return (i < AWAY_PLAYER_1) ? &m_awayPlayerList : &m_homePlayerList;
	}

	Common::String m_name;

	CCollisionBasketball m_basketball;
	CCollisionBasketball m_virtualBall;

	Common::Array<CCollisionBox> m_objectList;
	CCollisionObjectTree m_objectTree;
	Common::Array<CCollisionPlayer> m_homePlayerList;
	Common::Array<CCollisionPlayer> m_awayPlayerList;
	int m_objectCount;
	int m_backboardIndex[2];
	U32Sphere m_shotSpot[2];

};

} // End of namespace Scumm

#endif // ENABLE_HE

#endif // SCUMM_HE_BASKETBALL_COURT_H
