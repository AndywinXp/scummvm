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

#ifndef GRIM_EMI_LIP_H
#define GRIM_EMI_LIP_H

#include "common/scummsys.h"

namespace Grim {

struct LipEntry {
	byte time[2];
	byte code[2];
};

struct LipData {
	byte reserved0[4];
	byte count[4];
	LipEntry entries[1];
};

int readLipFile(const char *name, LipData **lipData);
int lipCodeToMouthShape(int code);
int compareLipMouthEntries(const void *first, const void *second);
int findLipCode(const LipData *lip, int position);
void sortLipMouthTable();

} // End of namespace Grim

#endif
