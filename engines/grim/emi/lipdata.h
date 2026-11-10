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

#ifndef GRIM_EMI_LIPDATA_H
#define GRIM_EMI_LIPDATA_H

#include "common/scummsys.h"

namespace Grim {

struct LipEntry {
	byte time[2];
	byte code[2];
};

class LipData {
public:
	enum {
		kBufferSize = 2000,
		kMouthTableSize = 46
	};

	static int readFile(const char *name, LipData **lipData);
	static int codeToMouthShape(int code);
	static int compareMouthEntries(const void *first, const void *second);
	static void sortMouthTable();

	int findCode(int position) const;

	byte reserved0[4];
	byte count[4];
	LipEntry entries[1];

private:
	struct MouthEntry {
		uint32 code;
		uint32 shape;
	};

	static byte _buffer[kBufferSize];
	static MouthEntry _mouthTable[kMouthTableSize];
};

} // End of namespace Grim

#endif
