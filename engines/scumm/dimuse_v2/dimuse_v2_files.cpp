/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "scumm/dimuse_v2/dimuse_v2.h"

namespace Scumm {
	// Validated
	int DiMUSE_v2::files_moduleInit() {
		return 0;
	}

	// Validated
	int DiMUSE_v2::files_moduleDeinit() {
		return 0;
	}

	uint8 *DiMUSE_v2::files_getSoundAddrData(int soundId) {
		/*
		if ((soundId != 0 && soundId < 0xFFFFFFF0) && files_initDataPtr != NULL) {
			if (files_initDataPtr->getSoundDataAddr) {
				return (uint8 *)files_initDataPtr->getSoundDataAddr(soundId);
			}
		}
		debug(5, "ERR: soundAddrFunc failure");*/
		debug(5, "DiMUSE_v2::files_getSoundAddrData(): UNIMPLEMENTED");
		return 0;
	}

	// Always returns 0 in disasm
	int DiMUSE_v2::files_fetchMap(int soundId) {
		return 0;
	}

	int DiMUSE_v2::files_getNextSound(int soundId) {
		int foundSoundId = 0;
		do {
			foundSoundId = wave_getNextSound(foundSoundId);
			if (!foundSoundId)
				return -1;
		} while (foundSoundId != soundId);
		return 2;
	}

	int DiMUSE_v2::files_checkRange(int soundId) {
		return (soundId != 0 && soundId < 0xFFFFFFF0);
	}

	int DiMUSE_v2::files_seek(int soundId, int offset, int mode) {
		/*if (soundId != 0 && soundId < 0xFFFFFFF0) {
			if (files_initDataPtr->seekFunc != NULL) {
				return files_initDataPtr->seekFunc(soundId, offset, mode);
			}
		}
		debug(5, "ERR: seekFunc failure");*/
		debug(5, "DiMUSE_v2::files_seek(): UNIMPLEMENTED");
		return 0;
	}

	int DiMUSE_v2::files_read(int soundId, uint8 *buf, int size) {
		/*if (soundId != 0 && soundId < 0xFFFFFFF0) {
			if (files_initDataPtr->readFunc != NULL) {
				return files_initDataPtr->readFunc(soundId, buf, size);
			}
		}
		debug(5, "ERR: readFunc failure");*/
		debug(5, "DiMUSE_v2::files_read(): UNIMPLEMENTED");
		return 0;
	}

	/*
	iMUSESoundBuffer *DiMUSE_v2::files_getBufInfo(int bufId) {
		if (bufId && files_initDataPtr->bufInfoFunc != NULL) {
			return files_initDataPtr->bufInfoFunc(bufId);
		}
		printf("ERR: bufInfoFunc failure");
		return NULL;
	}*/
} // End of namespace Scumm
