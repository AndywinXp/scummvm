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

namespace Scumm {

int CBBallCourt::GetPlayerIndex(int playerID) {
	assert((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER));

	Common::Array<CCollisionPlayer> *pPlayerList = GET_PLAYER_LIST_PTR(playerID);
	for (size_t i = 0; i < pPlayerList->size(); i++) {
		if ((*pPlayerList)[i].m_objectID == playerID) {
			return i;
		}
	}

	error("Tried to find a player in the player list that was not there.");
	return 0;
}

CCollisionPlayer *CBBallCourt::GetPlayerPtr(int playerID) {
	assert((FIRST_PLAYER <= playerID) && (playerID <= LAST_PLAYER));

	Common::Array<CCollisionPlayer> *pPlayerList = GET_PLAYER_LIST_PTR(playerID);

	size_t listSize = pPlayerList->size();
	for (size_t i = 0; i < listSize; i++) {
		if ((*pPlayerList)[i].m_objectID == playerID) {
			return &((*pPlayerList)[i]);
		}
	}

	debug("Tried to find a player in the player list that was not there.");
	return 0;
}

CCollisionBasketball *CBBallCourt::GetBallPtr(int ballID) {
	CCollisionBasketball *pSourceBall;

	if (ballID == m_basketball.m_objectID) {
		pSourceBall = &m_basketball;
	} else if (ballID == m_virtualBall.m_objectID) {
		pSourceBall = &m_virtualBall;
	} else {
		debug("Invalid ball ID passed to U32_User_Detect_Ball_Collision.");
		pSourceBall = &m_basketball;
	}

	return pSourceBall;
}

} // End of namespace Scumm
