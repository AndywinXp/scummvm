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
#include "scumm/he/basketball/geotypes.h"

namespace Scumm {

Basketball::Basketball(ScummEngine_v100he *vm) {
	_vm = vm;
	//g_court = new CBBallCourt();
	//g_shields = new CCollisionShieldVector();
}

Basketball::~Basketball() {}

int Basketball::U32_Float_To_Int(float input) {
	int output = 0;

	if (input < 0)
		output = (int)(input - .5);
	else if (input > 0)
		output = (int)(input + .5);

	return output;
}

int Basketball::U32_Double_To_Int(double input) {
	int output = 0;

	if (input < 0)
		output = (int)(input - .5);
	else if (input > 0)
		output = (int)(input + .5);

	return output;
}

void Basketball::FillBallTargetList(const CCollisionSphere *pSourceObject, CCollisionObjectVector *pTargetList) {
	// Add all of the court objects
	g_court.m_objectTree.SelectObjectsInBound(pSourceObject->GetBigBoundingBox(), pTargetList);

	// Add the shields
	CCollisionShieldVector::const_iterator shieldIt;

	for (shieldIt = g_shields.begin(); shieldIt != g_shields.end(); ++shieldIt) {
		if (!shieldIt->m_ignore) {
			pTargetList->push_back(&(*shieldIt));
		}
	}

	// Add all of the home players
	Common::Array<CCollisionPlayer>::const_iterator homePlayerIt;

	for (homePlayerIt = g_court.m_homePlayerList.begin();
		 homePlayerIt != g_court.m_homePlayerList.end();
		 ++homePlayerIt) {
		if (!homePlayerIt->m_ignore) {
			pTargetList->push_back(&(*homePlayerIt));
		}
	}

	// Add all of the away players
	Common::Array<CCollisionPlayer>::const_iterator awayPlayerIt;

	for (awayPlayerIt = g_court.m_awayPlayerList.begin();
		 awayPlayerIt != g_court.m_awayPlayerList.end();
		 ++awayPlayerIt) {
		if (!awayPlayerIt->m_ignore) {
			pTargetList->push_back(&(*awayPlayerIt));
		}
	}
}

void Basketball::FillPlayerTargetList(const CCollisionPlayer *pSourceObject, CCollisionObjectVector *pTargetList) {
	// Add all of the court objects
	g_court.m_objectTree.SelectObjectsInBound(pSourceObject->GetBigBoundingBox(), pTargetList);

	// Add the shields if the player has the ball
	if (pSourceObject->m_playerHasBall) {
		CCollisionShieldVector::const_iterator shieldIt;

		for (shieldIt = g_shields.begin(); shieldIt != g_shields.end(); ++shieldIt) {
			if (!shieldIt->m_ignore) {
				pTargetList->push_back(&(*shieldIt));
			}
		}
	}

	// Add the basketball
	if (!g_court.m_basketball.m_ignore) {
		pTargetList->push_back(*(CCollisionObjectVector *)&g_court.m_basketball);
	}

	// Add the virtual basketball
	if (!g_court.m_virtualBall.m_ignore) {
		pTargetList->push_back(*(CCollisionObjectVector *)&g_court.m_virtualBall);
	}

	// Add all of the home players
	Common::Array<CCollisionPlayer>::const_iterator homePlayerIt;

	for (homePlayerIt = g_court.m_homePlayerList.begin();
		 homePlayerIt != g_court.m_homePlayerList.end();
		 ++homePlayerIt) {
		if ((pSourceObject != &(*homePlayerIt)) &&
			(!homePlayerIt->m_ignore)) {
			pTargetList->push_back(&(*homePlayerIt));
		}
	}

	// Add all of the away players
	Common::Array<CCollisionPlayer>::const_iterator awayPlayerIt;

	for (awayPlayerIt = g_court.m_awayPlayerList.begin();
		 awayPlayerIt != g_court.m_awayPlayerList.end();
		 ++awayPlayerIt) {
		if ((pSourceObject != &(*awayPlayerIt)) &&
			(!awayPlayerIt->m_ignore)) {
			pTargetList->push_back(&(*awayPlayerIt));
		}
	}
}

} // End of namespace Scumm
