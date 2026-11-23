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

#include "common/textconsole.h"

#include "topgun/games/ace/ace1util.h"

namespace Topgun {

enum {
	kFunctionVsWalkIncX,
	kFunctionVsWalkIncY,
	kFunctionVsWalkDelay,
	kFunctionGetPathX,
	kFunctionSetPathX,
	kFunctionGetPathY,
	kFunctionSetPathY,
	kFunctionGetPathDir,
	kFunctionSetPathDir,
	kFunctionVsWalkQ,
	kFunctionVsBegCel,
	kFunctionVsEndCel,
	kFunctionArWalkNdx,
	kFunctionCount
};

Ace1UtilPlugin::Ace1UtilPlugin() {
}

bool Ace1UtilPlugin::init() {
	initWalkTables();
	return true;
}

void Ace1UtilPlugin::free() {
}

int Ace1UtilPlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("VSWALKINCX"))
		return kFunctionVsWalkIncX;
	if (name.equalsIgnoreCase("VSWALKINCY"))
		return kFunctionVsWalkIncY;
	if (name.equalsIgnoreCase("VSWALKDELAY"))
		return kFunctionVsWalkDelay;
	if (name.equalsIgnoreCase("GETPATHX"))
		return kFunctionGetPathX;
	if (name.equalsIgnoreCase("SETPATHX"))
		return kFunctionSetPathX;
	if (name.equalsIgnoreCase("GETPATHY"))
		return kFunctionGetPathY;
	if (name.equalsIgnoreCase("SETPATHY"))
		return kFunctionSetPathY;
	if (name.equalsIgnoreCase("GETPATHDIR"))
		return kFunctionGetPathDir;
	if (name.equalsIgnoreCase("SETPATHDIR"))
		return kFunctionSetPathDir;
	if (name.equalsIgnoreCase("?VSWALKQ@@ZCIII@Z"))
		return kFunctionVsWalkQ;
	if (name.equalsIgnoreCase("?VSBEGCEL@@ZCIII@Z"))
		return kFunctionVsBegCel;
	if (name.equalsIgnoreCase("?VSENDCEL@@ZCIII@Z"))
		return kFunctionVsEndCel;
	if (name.equalsIgnoreCase("?ARWALKNDX@@ZCIII@Z"))
		return kFunctionArWalkNdx;

	return -1;
}

uint16 Ace1UtilPlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("Ace1UtilPlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 1;
	switch (function) {
	case kFunctionSetPathX:
	case kFunctionSetPathY:
	case kFunctionSetPathDir:
		expected = 2;
		break;
	}

	if (count < expected)
		warning("Ace1UtilPlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[2];
	for (int i = 0; i < 2; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	switch (function) {
	case kFunctionVsWalkIncX:
		return vsWalkIncX(values[0]);
	case kFunctionVsWalkIncY:
		return vsWalkIncY(values[0]);
	case kFunctionVsWalkDelay:
		return vsWalkDelay(values[0]);
	case kFunctionGetPathX:
		return getPathX(values[0]);
	case kFunctionSetPathX:
		return setPathX(values[0], values[1]);
	case kFunctionGetPathY:
		return getPathY(values[0]);
	case kFunctionSetPathY:
		return setPathY(values[0], values[1]);
	case kFunctionGetPathDir:
		return getPathDir(values[0]);
	case kFunctionSetPathDir:
		return setPathDir(values[0], values[1]);
	case kFunctionVsWalkQ:
		return vsWalkQ(values[0]);
	case kFunctionVsBegCel:
		return vsBegCel(values[0]);
	case kFunctionVsEndCel:
		return vsEndCel(values[0]);
	case kFunctionArWalkNdx:
		return arWalkNdx(values[0]);
	}

	return 0;
}

void Ace1UtilPlugin::initWalkTables() {
	const int16 incX[kAce1UtilWalkTableSize] = {
		2, 3, -1, -4, -5, 4,
		7, 6, 8, 4, 9, -6,
		3, 15, 15, 14, 14, -2,
		3, 12, 7, 11, 6, 0,
		-1, -4, 6, -2, 1, 1,
		-3, -12, -7, -11, -6, 0,
		-3, -15, -15, -14, -14, 2,
		-7, -6, -8, -4, -9, 6
	};

	const int16 incY[kAce1UtilWalkTableSize] = {
		-8, -8, -6, -9, -8, -6,
		-7, -11, -3, -8, 0, 1,
		-4, -2, 2, -3, 4, 2,
		3, 13, 4, 1, 3, 14,
		3, 6, 3, 1, 5, -1,
		3, 13, 4, 1, 3, 14,
		-4, -2, 2, -3, 4, 2,
		-7, -11, -3, -8, 0, 1
	};

	const int16 delay[kAce1UtilWalkTableSize] = {
		72, 72, 48, 72, 48, 48,
		72, 72, 48, 72, 48, 48,
		72, 48, 72, 48, 48, 72,
		72, 48, 72, 48, 48, 72,
		72, 48, 72, 48, 48, 72,
		72, 48, 72, 48, 48, 72,
		72, 48, 72, 48, 48, 72,
		72, 72, 48, 72, 48, 48
	};

	for (int i = 0; i < kAce1UtilWalkTableSize; i++) {
		_walkIncX[i] = (uint16)incX[i];
		_walkIncY[i] = (uint16)incY[i];
		_walkDelay[i] = (uint16)delay[i];
	}
}

uint16 Ace1UtilPlugin::vsWalkIncX(uint16 index) {
	if (index >= kAce1UtilWalkTableSize) {
		warning("Ace1UtilPlugin::vsWalkIncX(): invalid index %d", index);
		return 0;
	}

	return _walkIncX[index];
}

uint16 Ace1UtilPlugin::vsWalkIncY(uint16 index) {
	if (index >= kAce1UtilWalkTableSize) {
		warning("Ace1UtilPlugin::vsWalkIncY(): invalid index %d", index);
		return 0;
	}

	return _walkIncY[index];
}

uint16 Ace1UtilPlugin::vsWalkDelay(uint16 index) {
	if (index >= kAce1UtilWalkTableSize) {
		warning("Ace1UtilPlugin::vsWalkDelay(): invalid index %d", index);
		return 0;
	}

	return _walkDelay[index];
}

uint16 Ace1UtilPlugin::vsWalkQ(uint16 index) {
	if (index >= kAce1UtilWalkTableSize) {
		warning("Ace1UtilPlugin::vsWalkQ(): invalid index %d", index);
		return 0;
	}

	return _walkQ[index];
}

uint16 Ace1UtilPlugin::vsBegCel(uint16 index) {
	if (index >= kAce1UtilWalkTableSize) {
		warning("Ace1UtilPlugin::vsBegCel(): invalid index %d", index);
		return 0;
	}

	return _begCel[index];
}

uint16 Ace1UtilPlugin::vsEndCel(uint16 index) {
	if (index >= kAce1UtilWalkTableSize) {
		warning("Ace1UtilPlugin::vsEndCel(): invalid index %d", index);
		return 0;
	}

	return _endCel[index];
}

uint16 Ace1UtilPlugin::setPathX(uint16 index, uint16 value) {
	if (index >= kAce1UtilPathPointCount) {
		warning("Ace1UtilPlugin::setPathX(): invalid index %d", index);
		return 0;
	}

	_pathX[index] = value;
	return value;
}

uint16 Ace1UtilPlugin::getPathX(uint16 index) {
	if (index >= kAce1UtilPathPointCount) {
		warning("Ace1UtilPlugin::getPathX(): invalid index %d", index);
		return 0;
	}

	return _pathX[index];
}

uint16 Ace1UtilPlugin::setPathY(uint16 index, uint16 value) {
	if (index >= kAce1UtilPathPointCount) {
		warning("Ace1UtilPlugin::setPathY(): invalid index %d", index);
		return 0;
	}

	_pathY[index] = value;
	return value;
}

uint16 Ace1UtilPlugin::getPathY(uint16 index) {
	if (index >= kAce1UtilPathPointCount) {
		warning("Ace1UtilPlugin::getPathY(): invalid index %d", index);
		return 0;
	}

	return _pathY[index];
}

uint16 Ace1UtilPlugin::setPathDir(uint16 index, uint16 value) {
	if (index >= kAce1UtilPathPointCount) {
		warning("Ace1UtilPlugin::setPathDir(): invalid index %d", index);
		return 0;
	}

	_pathDir[index] = value;
	return value;
}

uint16 Ace1UtilPlugin::getPathDir(uint16 index) {
	if (index >= kAce1UtilPathPointCount) {
		warning("Ace1UtilPlugin::getPathDir(): invalid index %d", index);
		return 0;
	}

	return _pathDir[index];
}

uint16 Ace1UtilPlugin::arWalkNdx(uint16 index) {
	if (index >= kAce1UtilWalkIndexTableSize) {
		warning("Ace1UtilPlugin::arWalkNdx(): invalid index %d", index);
		return 0;
	}

	return _arWalkNdx[index];
}

} // End of namespace Topgun
