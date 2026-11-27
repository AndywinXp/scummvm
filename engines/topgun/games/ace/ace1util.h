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

// Offsets of the tables in the data segment, which follow one another
enum {
	kAce1UtilDataStart = 0x10,
	kAce1UtilWalkIncXOffset = 0x24,
	kAce1UtilWalkIncYOffset = 0x84,
	kAce1UtilWalkDelayOffset = 0xE4,
	kAce1UtilWalkQOffset = 0x144,
	kAce1UtilBegCelOffset = 0x1A4,
	kAce1UtilEndCelOffset = 0x204,
	kAce1UtilPathXOffset = 0x264,
	kAce1UtilPathYOffset = 0x32C,
	kAce1UtilPathDirOffset = 0x3F4,
	kAce1UtilWalkIndexOffset = 0x4BC,
	kAce1UtilDataEnd = 0x4CC,
	kAce1UtilDataWords = (kAce1UtilDataEnd - kAce1UtilDataStart) / 2,
	kAce1UtilWalkTableSize = 48
};

class Ace1UtilPlugin : public Plugin {
public:
	Ace1UtilPlugin();

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	// The data segment from the start-up words to the end of the last table
	uint16 _data[kAce1UtilDataWords] = {};

	void initWalkTables();
	uint16 *dataWord(uint16 tableOffset, uint16 index);
	uint16 readData(uint16 tableOffset, uint16 index);
	void writeData(uint16 tableOffset, uint16 index, uint16 value);
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
