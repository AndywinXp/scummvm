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

#include "engines/grim/imuse/imuse_engine.h"

namespace Grim {

IMuseLipSync *IMuse::grimSoundLipSyncLoad(const char *source) {
	if (!source) {
		warning("IMuse::grimSoundLipSyncLoad(): no file name");
		return nullptr;
	}

	IMuseResource *res = soundsOpenFile(source);

	if (!res)
		return nullptr;

	IMuseLipSync *lipSync = new IMuseLipSync();

	memset(lipSync, 0, sizeof(IMuseLipSync));
	strncpy(lipSync->filename, source, 31);
	lipSync->filename[31] = 0;

	soundsFileSeek(res, 0, SEEK_END);
	int32 size = soundsFileTell(res);
	soundsFileSeek(res, 0, SEEK_SET);

	lipSync->size = size;
	lipSync->ptr = new uint8[size];
	copyDataFromResource(res, lipSync->ptr, size);
	soundsCloseFile(res);

	return lipSync;
}

void IMuse::grimSoundLipSyncFree(IMuseLipSync *lipSync) {
	if (!lipSync) {
		warning("IMuse::grimSoundLipSyncFree(): no lipsync data");
		return;
	}

	if (lipSync->ptr) {
		delete[] lipSync->ptr;
		lipSync->ptr = nullptr;
	}

	delete lipSync;
}

} // End of namespace Grim
