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
	char fileName[20] = "";
	files_getFilenameFromSoundId(soundId, fileName);
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

int DiMUSE_v2::files_seek(int soundId, int offset, int mode, int bufId) {
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[20] = "";
		files_getFilenameFromSoundId(soundId, fileName);
		int volIdGroup = bufId == 2 ? 3 : 1;
		DiMUSESndMgr::SoundDesc *s = _sound->openSound(soundId, fileName, 2, volIdGroup, -1);


		if (!s)
			s = _sound->openSound(soundId, fileName, 2, volIdGroup, 1);
		if (!s)
			s = _sound->openSound(soundId, fileName, 2, volIdGroup, 2);
		if (!s)
			return -1;

		int resultingOffset = s->bundle->seekFile(fileName, offset, mode);

		/*if (soundId != 0 && soundId < 0xFFFFFFF0) {
			if (files_initDataPtr->seekFunc != NULL) {
				return files_initDataPtr->seekFunc(soundId, offset, mode);
			}
		}
		debug(5, "ERR: seekFunc failure");*/
		_sound->closeSound(s);
		return resultingOffset;
	}
	debug(5, "DiMUSE_v2::files_seek(): soundId is 0 or out of range");
	return 0;
}

int DiMUSE_v2::files_read(int soundId, uint8 *buf, int size, int bufId) {
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[20] = "";
		files_getFilenameFromSoundId(soundId, fileName);

		int volIdGroup = bufId == 2 ? 3 : 1;
		bool dummy = false;
		//_bundle->decompressSampleByName(fileName, 0, size, &buf, ((_vm->_game.id == GID_CMI) && !(_vm->_game.features & GF_DEMO)), dummy);
		DiMUSESndMgr::SoundDesc *s = _sound->openSound(soundId, fileName, 2, volIdGroup, -1);

		if (!s)
			s = _sound->openSound(soundId, fileName, 2, volIdGroup, 1);
		if (!s)
			s = _sound->openSound(soundId, fileName, 2, volIdGroup, 2);
		if (!s)
			return -1;

		int resultingSize = s->bundle->decompressSampleByName(fileName, 0, size, &buf, ((_vm->_game.id == GID_CMI) && !(_vm->_game.features & GF_DEMO)), dummy);
		/*if (soundId != 0 && soundId < 0xFFFFFFF0) {
			if (files_initDataPtr->readFunc != NULL) {
				return files_initDataPtr->readFunc(soundId, buf, size);
			}
		}
		debug(5, "ERR: readFunc failure");*/
		_sound->closeSound(s);
		return resultingSize;
	}
	debug(5, "DiMUSE_v2::files_read(): soundId is 0 or out of range");
	return 0;
}

	
DiMUSE_v2::iMUSESoundBuffer *DiMUSE_v2::files_getBufInfo(int bufId) {
	if (bufId) {
		return &_soundBuffers[bufId];
	}

	debug(5, "ERR: bufInfoFunc failure");
	return NULL;
}

void DiMUSE_v2::files_getFilenameFromSoundId(int soundId, char *fileName) {
	int i = 0;

	if (_vm->_game.id == GID_CMI) {
		if (soundId < 2000) {
			int i = 0;
			while (_comiStateMusicTable[i].soundId != -1) {
				if (_comiStateMusicTable[i].soundId == soundId) {
					iMUSE_strcpy(fileName, (char *)_comiStateMusicTable[i].filename);
					return;
				}
				i++;
			}
		} else {
			while (_comiSeqMusicTable[i].soundId != -1) {
				if (_comiSeqMusicTable[i].soundId == soundId) {
					iMUSE_strcpy(fileName, (char *)_comiSeqMusicTable[i].filename);
					return;
				}
				i++;
			}
		}
	} else {
		if (soundId < 2000) {
			int i = 0;
			while (_digStateMusicTable[i].soundId != -1) {
				if (_digStateMusicTable[i].soundId == soundId) {
					iMUSE_strcpy(fileName, (char *)_digStateMusicTable[i].filename);
					return;
				}
				i++;
			}
		} else {
			while (_digSeqMusicTable[i].soundId != -1) {
				if (_digSeqMusicTable[i].soundId == soundId) {
					iMUSE_strcpy(fileName, (char *)_digSeqMusicTable[i].filename);
					return;
				}
				i++;
			}
		}
	}
}

} // End of namespace Scumm
