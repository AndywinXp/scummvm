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

#ifndef GRIM_CHORE_COS_UTIL_H
#define GRIM_CHORE_COS_UTIL_H

#include "common/scummsys.h"

#include "engines/grim/chore/cos.h"

namespace Grim {

class CosUtil {
public:
	CosUtil(Cos *pCos);

	int write(const char *path, CosCostume *pCostume, const char *header);

	CosComponent *compPtrFromHandle(CosCostume *pCostume, int compIndex);
	int adjustComponent(CosCostume *pCostume, int whichComponent);
	int componentHasAdjuster(CosCostume *pCostume, int whichComponent);

	int addChore(CosCostume *pCostume);
	void deleteChore(CosCostume *pCostume, int choreNum);
	void nameChore(CosCostume *pCostume, int choreNum, const char *name);
	int countChores(CosCostume *pCostume);
	const char *choreName(CosCostume *pCostume, int choreNum);

	int deleteKey(CosCostume *pCostume, int whichChore, int whichComponent, int time);
	CosChoreKey *queryValueAtTime(CosCostume *pCostume, int whichChore, int whichComponent, int time);
	int addFSKey(CosCostume *pCostume, int choreNum, int compNum, int time, int value);
	int addIntKey(CosCostume *pCostume, int choreNum, int compNum, int time, int value);
	int addFloatKey(CosCostume *pCostume, int choreNum, int compNum, int time, float value);

	int countTags();
	const char *tagInfo(int whichTag, int *pFourcc);

private:
	int collectTags(CosCostume *pCostume);
	CosChoreKey *addKey(CosCostume *pCostume, int choreNum, int compNum, int time);

	Cos *_pCos;
};

} // End of namespace Grim

#endif
