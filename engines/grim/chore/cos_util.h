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
	CosUtil(Cos *cos);

	int write(const char *path, CosCostume *costume, const char *header);

	CosComponent *compPtrFromHandle(CosCostume *costume, int compIndex);
	int adjustComponent(CosCostume *costume, int whichComponent);
	int componentHasAdjuster(CosCostume *costume, int whichComponent);

	int addChore(CosCostume *costume);
	void deleteChore(CosCostume *costume, int choreNum);
	void nameChore(CosCostume *costume, int choreNum, const char *name);
	int countChores(CosCostume *costume);
	const char *choreName(CosCostume *costume, int choreNum);

	int deleteKey(CosCostume *costume, int whichChore, int whichComponent, int time);
	CosChoreKey *queryValueAtTime(CosCostume *costume, int whichChore, int whichComponent, int time);
	int addFSKey(CosCostume *costume, int choreNum, int compNum, int time, int value);
	int addIntKey(CosCostume *costume, int choreNum, int compNum, int time, int value);
	int addFloatKey(CosCostume *costume, int choreNum, int compNum, int time, float value);

	int countTags();
	const char *tagInfo(int whichTag, int *fourcc);

private:
	int collectTags(CosCostume *costume);
	CosChoreKey *addKey(CosCostume *costume, int choreNum, int compNum, int time);

	Cos *_cos;
};

} // End of namespace Grim

#endif
