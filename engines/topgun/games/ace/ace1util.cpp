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
	// Words that the module file sets ahead of the tables
	_data[0] = 0x0100;
	_data[(0x1C - kAce1UtilDataStart) / 2] = 1;
	_data[(0x1E - kAce1UtilDataStart) / 2] = 0xFFFF;
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
		writeData(kAce1UtilWalkIncXOffset, i, (uint16)incX[i]);
		writeData(kAce1UtilWalkIncYOffset, i, (uint16)incY[i]);
		writeData(kAce1UtilWalkDelayOffset, i, (uint16)delay[i]);
	}
}

uint16 *Ace1UtilPlugin::dataWord(uint16 tableOffset, uint16 index) {
	// An index past a table reaches its neighbors, and the offset wraps in the data segment
	uint16 offset = (uint16)(tableOffset + 2 * index);

	if (offset < kAce1UtilDataStart || offset >= kAce1UtilDataEnd) {
		warning("Ace1UtilPlugin::dataWord(): index %d of the table at %d is out of range", index, tableOffset);
		return nullptr;
	}

	return &_data[(offset - kAce1UtilDataStart) / 2];
}

uint16 Ace1UtilPlugin::readData(uint16 tableOffset, uint16 index) {
	uint16 *word = dataWord(tableOffset, index);
	if (!word)
		return 0;

	return *word;
}

void Ace1UtilPlugin::writeData(uint16 tableOffset, uint16 index, uint16 value) {
	uint16 *word = dataWord(tableOffset, index);

	if (word)
		*word = value;
}

uint16 Ace1UtilPlugin::vsWalkIncX(uint16 index) {
	return readData(kAce1UtilWalkIncXOffset, index);
}

uint16 Ace1UtilPlugin::vsWalkIncY(uint16 index) {
	return readData(kAce1UtilWalkIncYOffset, index);
}

uint16 Ace1UtilPlugin::vsWalkDelay(uint16 index) {
	return readData(kAce1UtilWalkDelayOffset, index);
}

uint16 Ace1UtilPlugin::vsWalkQ(uint16 index) {
	return readData(kAce1UtilWalkQOffset, index);
}

uint16 Ace1UtilPlugin::vsBegCel(uint16 index) {
	return readData(kAce1UtilBegCelOffset, index);
}

uint16 Ace1UtilPlugin::vsEndCel(uint16 index) {
	return readData(kAce1UtilEndCelOffset, index);
}

uint16 Ace1UtilPlugin::setPathX(uint16 index, uint16 value) {
	writeData(kAce1UtilPathXOffset, index, value);
	return value;
}

uint16 Ace1UtilPlugin::getPathX(uint16 index) {
	return readData(kAce1UtilPathXOffset, index);
}

uint16 Ace1UtilPlugin::setPathY(uint16 index, uint16 value) {
	writeData(kAce1UtilPathYOffset, index, value);
	return value;
}

uint16 Ace1UtilPlugin::getPathY(uint16 index) {
	return readData(kAce1UtilPathYOffset, index);
}

uint16 Ace1UtilPlugin::setPathDir(uint16 index, uint16 value) {
	writeData(kAce1UtilPathDirOffset, index, value);
	return value;
}

uint16 Ace1UtilPlugin::getPathDir(uint16 index) {
	return readData(kAce1UtilPathDirOffset, index);
}

uint16 Ace1UtilPlugin::arWalkNdx(uint16 index) {
	return readData(kAce1UtilWalkIndexOffset, index);
}

} // End of namespace Topgun
