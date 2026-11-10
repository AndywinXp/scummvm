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

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lip.h"

namespace Grim {

struct LipMouthEntry {
	uint32 code;
	uint32 shape;
};

static const uint kLipBufferSize = 2000;
static const uint kLipHeaderSize = 8;
static const int kLipMaxEntries = (kLipBufferSize - kLipHeaderSize) / sizeof(LipEntry) - 1;
static const int kLipRestCode = 95;
static const int kLipRestShape = 5;
static const int kLipUnknownShape = 6;

static byte gLipBuffer[kLipBufferSize];

static LipMouthEntry gLipMouthTable[] = {
	{ 95, 5 }, { 593, 6 }, { 97, 6 }, { 230, 6 }, { 652, 13 }, { 596, 6 }, { 601, 6 }, { 98, 11 },
	{ 679, 7 }, { 100, 7 }, { 240, 10 }, { 603, 13 }, { 616, 13 }, { 602, 14 }, { 605, 14 }, { 101, 6 },
	{ 102, 9 }, { 103, 13 }, { 609, 13 }, { 104, 13 }, { 618, 13 }, { 105, 8 }, { 676, 7 }, { 107, 7 },
	{ 108, 10 }, { 619, 10 }, { 109, 11 }, { 110, 13 }, { 331, 13 }, { 111, 12 }, { 112, 11 }, { 114, 7 },
	{ 635, 7 }, { 633, 7 }, { 115, 7 }, { 643, 7 }, { 116, 7 }, { 638, 7 }, { 952, 10 }, { 650, 14 },
	{ 117, 14 }, { 118, 9 }, { 119, 14 }, { 106, 13 }, { 122, 7 }, { 658, 7 }
};

static const int kLipMouthTableSize = ARRAYSIZE(gLipMouthTable);

int readLipFile(const char *name, LipData **lipData) {
	const char *source = name;
	int result = 0;

	if (*source == '@')
		source++;

	LString path(source);
	uint dot = path.rfind('.');

	if (dot != LString::npos)
		path.erase(dot);

	path += ".lip";

	LFile *file = gpFileServices->open(path.c_str(), "rb");

	if (file) {
		int count = file->read(gLipBuffer, kLipBufferSize);

		if (file->eof())
			result = count;

		gpFileServices->close(file, true);
	}

	*lipData = (LipData *)gLipBuffer;

	return result;
}

int lipCodeToMouthShape(int code) {
	if (code == kLipRestCode)
		return kLipRestShape;

	LipMouthEntry key;

	key.code = code;
	key.shape = 0;

	const LipMouthEntry *found = (const LipMouthEntry *)bsearch(&key, gLipMouthTable, kLipMouthTableSize, sizeof(LipMouthEntry), compareLipMouthEntries);

	if (found)
		return (uint16)found->shape;

	return kLipUnknownShape;
}

int compareLipMouthEntries(const void *first, const void *second) {
	return *(const int *)first - *(const int *)second;
}

int findLipCode(const LipData *lip, int position) {
	int count = (int)READ_LE_UINT32(lip->count);
	int target = position >> 4;

	if (count > kLipMaxEntries) {
		warning("findLipCode(): entry count %d is out of range", count);
		count = kLipMaxEntries;
	}

	for (int i = 0; i < count; i++) {
		int entryTime = READ_LE_INT16(lip->entries[i].time);

		if (target < entryTime)
			return kLipRestCode;

		if (target < READ_LE_INT16(lip->entries[i + 1].time))
			return READ_LE_INT16(lip->entries[i].code);
	}

	return kLipRestCode;
}

void sortLipMouthTable() {
	qsort(gLipMouthTable, kLipMouthTableSize, sizeof(LipMouthEntry), compareLipMouthEntries);
}

} // End of namespace Grim
