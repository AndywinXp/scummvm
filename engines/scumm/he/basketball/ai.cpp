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

#include "scumm/he/intern_he.h"

#include "scumm/he/basketball/basketball.h"
#include "scumm/he/basketball/court.h"
#include "scumm/he/basketball/geotypes.h"
#include "scumm/he/logic_he.h"

namespace Scumm {

inline int GetPlayerTeam(int whichPlayer) {
	return ((whichPlayer < AWAY_PLAYER_1) ? TEAM_HOME : TEAM_AWAY);
}

inline int GetOpponentTeam(int whichPlayer) {
	return ((whichPlayer < AWAY_PLAYER_1) ? TEAM_AWAY : TEAM_HOME);
}

int LogicHEBasketball::U32_User_Get_Player_Closest_To_Ball(int teamIndex) {
	assert((TEAM_HOME <= teamIndex) && (teamIndex <= TEAM_AWAY));

	int finalPlayerIndex = NO_PLAYER;
	int shortestDistance2 = 0x7FFFFFFF;

	// Go through all of the players on the team and calculate the distance between
	// them and the ball.
	Common::Array<CCollisionPlayer> *pPlayerList =
		(teamIndex == TEAM_HOME) ?
		&_vm->_basketball->g_court.m_homePlayerList : &_vm->_basketball->g_court.m_awayPlayerList;

	for (size_t playerIndex = 0; playerIndex < pPlayerList->size(); ++playerIndex) {
		CCollisionPlayer *pCurrentPlayer = &((*pPlayerList)[playerIndex]);

		if (pCurrentPlayer->m_playerIsInGame) {

			int distance2 = (
				((pCurrentPlayer->m_center.x - _vm->_basketball->g_court.m_basketball.m_center.x) *
				 (pCurrentPlayer->m_center.x - _vm->_basketball->g_court.m_basketball.m_center.x)) +
				((pCurrentPlayer->m_center.y - _vm->_basketball->g_court.m_basketball.m_center.y) *
				 (pCurrentPlayer->m_center.y - _vm->_basketball->g_court.m_basketball.m_center.y)));

			// Keep track of the player closest to the ball.
			if (distance2 < shortestDistance2) {
				finalPlayerIndex = playerIndex;
				shortestDistance2 = distance2;
			}
		}
	}

	assert(finalPlayerIndex != NO_PLAYER);

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, (*pPlayerList)[finalPlayerIndex].m_objectID);

	return 1;
}

int LogicHEBasketball::U32_User_Get_Player_Closest_To_Ball() {
	int finalPlayerIndex = NO_PLAYER;
	int shortestDistance2 = 0x7FFFFFFF;
	Common::Array<CCollisionPlayer> *pPlayerList = nullptr;

	// Go through all of the players on both teams and calculate the distance between
	// them and the ball.
	for (size_t playerIndex = 0; playerIndex < _vm->_basketball->g_court.m_homePlayerList.size(); ++playerIndex) {
		CCollisionPlayer *pCurrentPlayer = &_vm->_basketball->g_court.m_homePlayerList[playerIndex];

		if (pCurrentPlayer->m_playerIsInGame) {
			int distance2 = (
				((pCurrentPlayer->m_center.x - _vm->_basketball->g_court.m_basketball.m_center.x) *
				 (pCurrentPlayer->m_center.x - _vm->_basketball->g_court.m_basketball.m_center.x)) +
				((pCurrentPlayer->m_center.y - _vm->_basketball->g_court.m_basketball.m_center.y) *
				 (pCurrentPlayer->m_center.y - _vm->_basketball->g_court.m_basketball.m_center.y)));

			// Keep track of the player closest to the ball.
			if (distance2 < shortestDistance2) {
				finalPlayerIndex = playerIndex;
				shortestDistance2 = distance2;
				pPlayerList = &_vm->_basketball->g_court.m_homePlayerList;
			}
		}
	}

	for (size_t playerIndex = 0; playerIndex < _vm->_basketball->g_court.m_awayPlayerList.size(); ++playerIndex) {
		CCollisionPlayer *pCurrentPlayer = &_vm->_basketball->g_court.m_awayPlayerList[playerIndex];

		if (pCurrentPlayer->m_playerIsInGame) {
			int distance2 = (
				((pCurrentPlayer->m_center.x - _vm->_basketball->g_court.m_basketball.m_center.x) *
				 (pCurrentPlayer->m_center.x - _vm->_basketball->g_court.m_basketball.m_center.x)) +
				((pCurrentPlayer->m_center.y - _vm->_basketball->g_court.m_basketball.m_center.y) *
				 (pCurrentPlayer->m_center.y - _vm->_basketball->g_court.m_basketball.m_center.y)));

			// Keep track of the player closest to the ball.
			if (distance2 < shortestDistance2) {
				finalPlayerIndex = playerIndex;
				shortestDistance2 = distance2;
				pPlayerList = &_vm->_basketball->g_court.m_awayPlayerList;
			}
		}
	}

	assert(finalPlayerIndex != NO_PLAYER);

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, (*pPlayerList)[finalPlayerIndex].m_objectID);

	return 1;
}

int Basketball::NumOpponentsInCone(int team, float width_distance_ratio, const U32FltVector2D &end, const U32FltVector2D &focus) {
	// end is the end point of the center line of the cone
	// width_distance_ratio ratio defines the width of the cone = width at dist 1

	int count = 0;

	Line2D line(focus, end);

	Common::Array<CCollisionPlayer> *pPlayerList = (team == TEAM_HOME) ? &_vm->_basketball->g_court.m_homePlayerList : &_vm->_basketball->g_court.m_awayPlayerList;

	for (Common::Array<CCollisionPlayer>::iterator pCurrentPlayer = pPlayerList->begin(); pCurrentPlayer != pPlayerList->end(); ++pCurrentPlayer) {
		U32FltVector2D point = line.ProjectPoint(pCurrentPlayer->m_center);

		if (focus.Distance2(pCurrentPlayer->m_center) < (point.Distance2(focus) * width_distance_ratio * width_distance_ratio) && line.InBetween(point, focus, end)) {
			count++;
		}
	}

	return count;
}

bool IsPointInBounds(const U32FltVector2D &point) {
	return ((point.x > 0) &&
			(point.x < MAX_WORLD_X) &&
			(point.y > 0) &&
			(point.y < MAX_WORLD_Y));
}

float Basketball::Congestion(U32FltVector2D pos, bool ignore, int whichPlayer) {
	float congestion = 0.0f;

	for (int team = TEAM_HOME; team <= TEAM_AWAY; ++team) {
		Common::Array<CCollisionPlayer> *pPlayerList = (team == TEAM_HOME) ? &g_court.m_homePlayerList : &g_court.m_awayPlayerList;

		for (Common::Array<CCollisionPlayer>::iterator pCurrentPlayer = pPlayerList->begin(); pCurrentPlayer != pPlayerList->end(); ++pCurrentPlayer) {
			if (pCurrentPlayer->m_playerIsInGame && (ignore && !(pCurrentPlayer->m_objectID == whichPlayer))) {
				float distance2 = pos.Distance2(pCurrentPlayer->m_center);

				if (distance2 == 0.0f)
					return 3.402823466e+38F;

				congestion += (1 / distance2);
			}
		}
	}

	return congestion;
}

int LogicHEBasketball::U32_User_Get_Open_Spot(int whichPlayer, U32FltVector2D upperLeft, U32FltVector2D lowerRight, U32FltVector2D passer, bool attract, U32FltVector2D attract_point) {
	int x_granularity = 5;
	int y_granularity = 5;

	int rectWidth = fabs(upperLeft.x - lowerRight.x);
	int rectHeight = fabs(upperLeft.y - lowerRight.y);

	float x_mesh = rectWidth / (x_granularity + 1);
	float y_mesh = rectHeight / (y_granularity + 1);

	float start_x = upperLeft.x + x_mesh / 2;
	float start_y = upperLeft.y + y_mesh / 2;

	float x = start_x, y = start_y;

	float best_congestion = 3.402823466e+38F;

	U32FltVector2D best_point, point;

	int opponent_team = GetOpponentTeam(whichPlayer);

	float tmp;

	for (int ii = 0; ii < x_granularity; ii++) {
		for (int jj = 0; jj < y_granularity; jj++) {
			tmp = _vm->_basketball->Congestion(point = U32FltVector2D(x, y), true, whichPlayer);

			if (attract) {
				tmp -= (4 * (1 / point.Distance2(attract_point)));
			}

			if (tmp < best_congestion &&
				IsPointInBounds(point)) {
				best_congestion = tmp;
				best_point = point;
			}

			y += y_mesh;
		}

		x += x_mesh;
		y = start_y;
	}

	if (best_congestion == 3.402823466e+38F) {
		return 0;
	} else {
		writeScummVar(_vm1->VAR_U32_USER_VAR_A, best_point.x);
		writeScummVar(_vm1->VAR_U32_USER_VAR_B, best_point.y);

		return 1;
	}
}

int IsPlayerInBounds(U32Sphere *pPlayer) {
	assert(pPlayer);

	return (((pPlayer->m_center.x - pPlayer->m_radius) > -COLLISION_EPSILON) &&
			((pPlayer->m_center.x + pPlayer->m_radius) < (MAX_WORLD_X + COLLISION_EPSILON)) &&
			((pPlayer->m_center.y - pPlayer->m_radius) > -COLLISION_EPSILON) &&
			((pPlayer->m_center.y + pPlayer->m_radius) < (MAX_WORLD_Y + COLLISION_EPSILON)));
}

int LogicHEBasketball::U32_User_Is_Player_In_Bounds(int playerID) {
	U32Cylinder *pPlayer = _vm->_basketball->g_court.GetPlayerPtr(playerID);

	int bIsPlayerInBounds = IsPlayerInBounds(pPlayer);

	writeScummVar(_vm1->VAR_U32_USER_VAR_A, bIsPlayerInBounds);

	return 1;
}

int LogicHEBasketball::U32_User_Is_Ball_In_Bounds() {
	int bIsBallInBounds = IsPlayerInBounds((U32Sphere *)&_vm->_basketball->g_court.m_basketball);
	writeScummVar(_vm1->VAR_U32_USER_VAR_A, bIsBallInBounds);
	return 1;
}

} // End of namespace Scumm
