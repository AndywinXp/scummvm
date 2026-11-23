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

#ifndef TOPGUN_GAMES_ACE_ACE1UTIL_H
#define TOPGUN_GAMES_ACE_ACE1UTIL_H

#include "common/scummsys.h"
#include "common/str.h"

#include "topgun/plugin.h"

namespace Topgun {

enum {
	kAce1UtilWalkTableSize = 48,
	kAce1UtilPathPointCount = 100,
	kAce1UtilWalkIndexTableSize = 8
};

class Ace1UtilPlugin : public Plugin {
public:
	Ace1UtilPlugin();

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	uint16 _walkIncX[kAce1UtilWalkTableSize] = {};
	uint16 _walkIncY[kAce1UtilWalkTableSize] = {};
	uint16 _walkDelay[kAce1UtilWalkTableSize] = {};
	uint16 _walkQ[kAce1UtilWalkTableSize] = {};
	uint16 _begCel[kAce1UtilWalkTableSize] = {};
	uint16 _endCel[kAce1UtilWalkTableSize] = {};
	uint16 _pathX[kAce1UtilPathPointCount] = {};
	uint16 _pathY[kAce1UtilPathPointCount] = {};
	uint16 _pathDir[kAce1UtilPathPointCount] = {};
	uint16 _arWalkNdx[kAce1UtilWalkIndexTableSize] = {};

	void initWalkTables();
	uint16 vsWalkIncX(uint16 index);
	uint16 vsWalkIncY(uint16 index);
	uint16 vsWalkDelay(uint16 index);
	uint16 vsWalkQ(uint16 index);
	uint16 vsBegCel(uint16 index);
	uint16 vsEndCel(uint16 index);
	uint16 setPathX(uint16 index, uint16 value);
	uint16 getPathX(uint16 index);
	uint16 setPathY(uint16 index, uint16 value);
	uint16 getPathY(uint16 index);
	uint16 setPathDir(uint16 index, uint16 value);
	uint16 getPathDir(uint16 index);
	uint16 arWalkNdx(uint16 index);
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_ACE_ACE1UTIL_H
