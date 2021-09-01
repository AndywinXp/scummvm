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

int DiMUSE_v2::files_moduleInit() {
	return 0;
}

int DiMUSE_v2::files_moduleDeinit() {
	return 0;
}

uint8 *DiMUSE_v2::files_getSoundAddrData(int soundId) {
	// This function is always used for SFX (tracks which do not have a
	// stream pointer), hence the use of IMUSE_RESOURCE
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[20] = "";
		files_getFilenameFromSoundId(soundId, fileName);
		
		DiMUSESndMgr::SoundDesc *s = _sound->openSound(soundId, fileName, IMUSE_RESOURCE, IMUSE_BUFFER_SFX, -1);

		return s->resPtr;
	}
	debug(5, "DiMUSE_v2::files_getSoundAddrData(): soundId is 0 or out of range");
	return NULL;
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
	// This function and files_read() are used for sounds for which a stream is needed
	// (speech and music), therefore they will always refer to sounds in a bundle file
	// The seeked position is in reference to the decompressed sound
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[23] = "";
		files_getFilenameFromSoundId(soundId, fileName);

		DiMUSESndMgr::SoundDesc *s = _sound->findSoundById(soundId);
		if (s) {
			int resultingOffset = s->bundle->seekFile(fileName, offset, mode);

			return resultingOffset;
		} 
		debug(5, "DiMUSE_v2::files_seek(): can't find sound %d (%s); did you forget to open it?", soundId, fileName);
	} else {
		debug(5, "DiMUSE_v2::files_seek(): soundId is 0 or out of range");
	}

	return 0;
}

int DiMUSE_v2::files_read(int soundId, uint8 *buf, int loadIndex, int size, int bufId) {
	// This function and files_seek() are used for sounds for which a stream is needed
	// (speech and music), therefore they will always refer to sounds in a bundle file
	// TODO: Does this work with speech?
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[23] = "";
		files_getFilenameFromSoundId(soundId, fileName);

		DiMUSESndMgr::SoundDesc *s = _sound->getSounds();
		DiMUSESndMgr::SoundDesc *curSnd = NULL;
		for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
			curSnd = &s[i];
			if (curSnd->inUse) {
				if (curSnd->soundId == soundId) {
					bool uncompressedBundle = false;
					uint8 *tmpBuf; debug(5, "DiMUSE_v2::files_read(): loadIndex (%d)", loadIndex);
					int resultingSize = curSnd->bundle->decompressSampleByName(fileName, loadIndex, size, &tmpBuf, ((_vm->_game.id == GID_CMI) && !(_vm->_game.features & GF_DEMO)), uncompressedBundle);
					memcpy(buf, tmpBuf, size);
					return resultingSize;
				}
			}
		}

		debug(5, "DiMUSE_v2::files_read(): can't find sound %d (%s); did you forget to open it?", soundId, fileName);

	} else {
		debug(5, "DiMUSE_v2::files_read(): soundId is 0 or out of range");
	}
	
	return 0;
}

	
DiMUSE_v2::iMUSESoundBuffer *DiMUSE_v2::files_getBufInfo(int bufId) {
	if (bufId) {
		return &_soundBuffers[bufId];
	}

	debug(5, "ERR: bufInfoFunc failure");
	return NULL;
}

void DiMUSE_v2::files_openSound(int soundId) {
	char fileName[23] = "";
	files_getFilenameFromSoundId(soundId, fileName);
	DiMUSESndMgr::SoundDesc *s = NULL;
	s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, IMUSE_VOLGRP_MUSIC, -1);
	if (!s)
		s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, IMUSE_VOLGRP_MUSIC, 1);
	if (!s)
		s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, IMUSE_VOLGRP_MUSIC, 2);
	if (!s)
		debug(5, "DiMUSE_v2::files_openSound(): can't open sound %d (%s)", soundId, fileName);
}

void DiMUSE_v2::files_closeSound(int soundId) {
	_sound->scheduleSoundForDeallocation(soundId);
}

void DiMUSE_v2::files_closeAllSounds() {
	DiMUSESndMgr::SoundDesc *s = _sound->getSounds();
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		if (s[i].inUse)
			files_closeSound(s->soundId);
	}

	flushTracks();
}

void DiMUSE_v2::files_getFilenameFromSoundId(int soundId, char *fileName) {
	int i = 0;

	if (_vm->_game.id == GID_CMI) {
		if (soundId < 2000) {
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
