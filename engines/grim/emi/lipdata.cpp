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
#include "engines/grim/emi/lipdata.h"
#include "engines/grim/emi/sounds.h"

namespace Grim {

static const uint kLipHeaderSize = 8;
static const int kLipMaxEntries = (LipData::kBufferSize - kLipHeaderSize) / sizeof(LipEntry) - 1;
static const int kLipRestCode = 95;
static const int kLipRestShape = 5;
static const int kLipUnknownShape = 6;

byte LipData::_buffer[kBufferSize];

LipData::MouthEntry LipData::_mouthTable[kMouthTableSize] = {
	{ 95, 5 }, { 593, 6 }, { 97, 6 }, { 230, 6 }, { 652, 13 }, { 596, 6 }, { 601, 6 }, { 98, 11 },
	{ 679, 7 }, { 100, 7 }, { 240, 10 }, { 603, 13 }, { 616, 13 }, { 602, 14 }, { 605, 14 }, { 101, 6 },
	{ 102, 9 }, { 103, 13 }, { 609, 13 }, { 104, 13 }, { 618, 13 }, { 105, 8 }, { 676, 7 }, { 107, 7 },
	{ 108, 10 }, { 619, 10 }, { 109, 11 }, { 110, 13 }, { 331, 13 }, { 111, 12 }, { 112, 11 }, { 114, 7 },
	{ 635, 7 }, { 633, 7 }, { 115, 7 }, { 643, 7 }, { 116, 7 }, { 638, 7 }, { 952, 10 }, { 650, 14 },
	{ 117, 14 }, { 118, 9 }, { 119, 14 }, { 106, 13 }, { 122, 7 }, { 658, 7 }
};

int LipData::readFile(const char *name, LipData **lipData) {
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
		int readCount = file->read(_buffer, kBufferSize);

		if (file->eof())
			result = readCount;

		gpFileServices->close(file, true);
	}

	*lipData = (LipData *)_buffer;

	return result;
}

int LipData::codeToMouthShape(int code) {
	if (code == kLipRestCode)
		return kLipRestShape;

	MouthEntry key;

	key.code = code;
	key.shape = 0;

	const MouthEntry *found = (const MouthEntry *)bsearch(&key, _mouthTable, kMouthTableSize, sizeof(MouthEntry), compareMouthEntries);

	if (found)
		return (uint16)found->shape;

	return kLipUnknownShape;
}

int LipData::compareMouthEntries(const void *first, const void *second) {
	return *(const int *)first - *(const int *)second;
}

int LipData::findCode(int position) const {
	int entryCount = (int)READ_LE_UINT32(count);
	int target = position >> 4;

	if (entryCount > kLipMaxEntries) {
		warning("LipData::findCode(): entry count %d is out of range", entryCount);
		entryCount = kLipMaxEntries;
	}

	for (int i = 0; i < entryCount; i++) {
		int entryTime = READ_LE_INT16(entries[i].time);

		if (target < entryTime)
			return kLipRestCode;

		if (target < READ_LE_INT16(entries[i + 1].time))
			return READ_LE_INT16(entries[i].code);
	}

	return kLipRestCode;
}

void LipData::sortMouthTable() {
	qsort(_mouthTable, kMouthTableSize, sizeof(MouthEntry), compareMouthEntries);
}

int LipData::getLipMouthShape(uint soundHandle, int position) {
	LipData *lipData = Sounds::getSoundLipData(soundHandle);

	if (!lipData)
		return 0;

	int soundPosition = position;

	if (position < 0)
		soundPosition = Sounds::getSoundPosition(soundHandle);
	else if (position > Sounds::getSoundLength(soundHandle))
		return 0;

	return codeToMouthShape(lipData->findCode(soundPosition));
}

} // End of namespace Grim
