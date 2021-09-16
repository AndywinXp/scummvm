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
#include "scumm/dimuse_v2/dimuse_v2_files.h"

namespace Scumm {

DiMUSEFilesHandler::DiMUSEFilesHandler(DiMUSE_v2 *engine, ScummEngine_v7 *vm) {
	_engine = engine;
	_sound = new DiMUSESndMgr(vm, true);
	assert(_sound);
	_vm = vm;
}

DiMUSEFilesHandler::~DiMUSEFilesHandler() {
	delete _sound;
}

int DiMUSEFilesHandler::init() {
	return 0;
}

int DiMUSEFilesHandler::deinit() {
	return 0;
}

void DiMUSEFilesHandler::saveLoad(Common::Serializer &ser) {
	int curSound = 0;
	ser.syncArray(_currentSpeechFile, 60, Common::Serializer::SByte, VER(103));
	if (ser.isSaving()) {
		DiMUSESndMgr::SoundDesc *soundsToSave = _sound->getSounds();
		for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
			ser.syncAsSint32LE(soundsToSave[l].soundId, VER(103));
		}
	}

	if (ser.isLoading()) {
		for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
			ser.syncAsSint32LE(curSound, VER(103));
			if (curSound) {
				openSound(curSound);
				if (curSound != kTalkSoundID)
					closeSound(curSound);
			}
		}
	}
}

uint8 *DiMUSEFilesHandler::getSoundAddrData(int soundId) {
	// This function is always used for SFX (tracks which do not
	// have a stream pointer), hence the use of the resource address
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		if (_vm->_res->isResourceLoaded(rtSound, soundId))
			return _vm->getResourceAddress(rtSound, soundId);
		else
			return NULL;
	}
	debug(5, "DiMUSEFilesHandler::getSoundAddrData(): soundId is 0 or out of range");
	return NULL;
}

// Always returns 0 in disasm
int DiMUSEFilesHandler::fetchMap(int soundId) {
	return 0;
}

int DiMUSEFilesHandler::getNextSound(int soundId) {
	int foundSoundId = 0;
	do {
		foundSoundId = _engine->diMUSEGetNextSound(foundSoundId);
		if (!foundSoundId)
			return -1;
	} while (foundSoundId != soundId);
	return 2;
}

int DiMUSEFilesHandler::checkIdInRange(int soundId) {
	return (soundId != 0 && soundId < 0xFFFFFFF0);
}

int DiMUSEFilesHandler::seek(int soundId, int offset, int mode, int bufId) {
	// This function and files_read() are used for sounds for which a stream is needed
	// (speech and music), therefore they will always refer to sounds in a bundle file
	// The seeked position is in reference to the decompressed sound
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[60] = "";
		getFilenameFromSoundId(soundId, fileName);

		DiMUSESndMgr::SoundDesc *s = _sound->findSoundById(soundId);
		if (s) {
			int resultingOffset = s->bundle->seekFile(offset, mode);

			return resultingOffset;
		} 
		debug(5, "DiMUSEFilesHandler::seek(): can't find sound %d (%s); did you forget to open it?", soundId, fileName);
	} else {
		debug(5, "DiMUSEFilesHandler::seek(): soundId is 0 or out of range");
	}

	return 0;
}

int DiMUSEFilesHandler::read(int soundId, uint8 *buf, int size, int bufId) {
	// This function and files_seek() are used for sounds for which a stream is needed
	// (speech and music), therefore they will always refer to sounds in a bundle file
	// TODO: Does this work with speech?
	if (soundId != 0 && soundId < 0xFFFFFFF0) {
		char fileName[60] = "";
		getFilenameFromSoundId(soundId, fileName);

		DiMUSESndMgr::SoundDesc *s = _sound->getSounds();
		DiMUSESndMgr::SoundDesc *curSnd = NULL;
		for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
			curSnd = &s[i];
			if (curSnd->inUse) {
				if (curSnd->soundId == soundId) {
					uint8 *tmpBuf;
					//debug(5, "DiMUSE_v2::files_read(): trying to read (%d) bytes of data from file %s", size, fileName);
					int resultingSize = curSnd->bundle->readFile(fileName, size, &tmpBuf, ((_vm->_game.id == GID_CMI) && !(_vm->_game.features & GF_DEMO)));

					if (resultingSize != size)
						debug(5, "DiMUSEFilesHandler::read(): WARNING: tried to read %d bytes, got %d instead (soundId %d (%s))", size, resultingSize, soundId, fileName);
					memcpy(buf, tmpBuf, size);
					free(tmpBuf);
					return resultingSize;
				}
			}
		}

		debug(5, "DiMUSEFilesHandler::read(): can't find sound %d (%s); did you forget to open it?", soundId, fileName);

	} else {
		debug(5, "DiMUSEFilesHandler::read(): soundId is 0 or out of range");
	}
	
	return 0;
}

	
DiMUSESoundBuffer *DiMUSEFilesHandler::getBufInfo(int bufId) {
	if (bufId > 0 && bufId <= 4) {
		return &_soundBuffers[bufId];
	}

	debug(5, "DiMUSEFilesHandler::getBufInfo(): ERROR: invalid buffer id");
	return NULL;
}

void DiMUSEFilesHandler::openSound(int soundId) {
	char fileName[60] = "";
	getFilenameFromSoundId(soundId, fileName);
	DiMUSESndMgr::SoundDesc *s = NULL;
	int groupId = soundId == kTalkSoundID ? IMUSE_VOLGRP_VOICE : IMUSE_VOLGRP_MUSIC;
	s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, groupId, -1);
	if (!s)
		s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, groupId, 1);
	if (!s)
		s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, groupId, 2);
	if (!s)
		debug(5, "DiMUSEFilesHandler::openSound(): can't open sound %d (%s)", soundId, fileName);
}

void DiMUSEFilesHandler::closeSound(int soundId) {
	_sound->scheduleSoundForDeallocation(soundId);
}

void DiMUSEFilesHandler::closeAllSounds() {
	DiMUSESndMgr::SoundDesc *s = _sound->getSounds();
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		if (s[i].inUse)
			closeSound(s->soundId);
	}

	_engine->flushTracks();
}

void DiMUSEFilesHandler::getFilenameFromSoundId(int soundId, char *fileName) {
	int i = 0;

	if (soundId == kTalkSoundID) {
		strncpy(fileName, _currentSpeechFile, 60);
	}

	if (_vm->_game.id == GID_CMI) {
		if (soundId < 2000) {
			while (_comiStateMusicTable[i].soundId != -1) {
				if (_comiStateMusicTable[i].soundId == soundId) {
					strncpy(fileName, (char *)_comiStateMusicTable[i].filename, 60);
					return;
				}
				i++;
			}
		} else {
			while (_comiSeqMusicTable[i].soundId != -1) {
				if (_comiSeqMusicTable[i].soundId == soundId) {
					strncpy(fileName, (char *)_comiSeqMusicTable[i].filename, 60);
					return;
				}
				i++;
			}
		}
	} else {
		if (soundId < 2000) {
			while (_digStateMusicTable[i].soundId != -1) {
				if (_digStateMusicTable[i].soundId == soundId) {
					strncpy(fileName, (char *)_digStateMusicTable[i].filename, 60);
					return;
				}
				i++;
			}
		} else {
			while (_digSeqMusicTable[i].soundId != -1) {
				if (_digSeqMusicTable[i].soundId == soundId) {
					strncpy(fileName, (char *)_digSeqMusicTable[i].filename, 60);
					return;
				}
				i++;
			}
		}
	}
}

void DiMUSEFilesHandler::diMUSEAllocSoundBuffer(int bufId, int size, int loadSize, int criticalSize) {
	DiMUSESoundBuffer *selectedSoundBuf;

	selectedSoundBuf = &_soundBuffers[bufId];
	selectedSoundBuf->buffer = (uint8 *)malloc(size);
	selectedSoundBuf->bufSize = size;
	selectedSoundBuf->loadSize = loadSize;
	selectedSoundBuf->criticalSize = criticalSize;
}

void DiMUSEFilesHandler::diMUSEDeallocSoundBuffer(int bufId) {
	DiMUSESoundBuffer *selectedSoundBuf;

	selectedSoundBuf = &_soundBuffers[bufId];
	free(selectedSoundBuf->buffer);
	selectedSoundBuf->buffer = NULL;
}

void DiMUSEFilesHandler::flushSounds() {
	DiMUSESndMgr::SoundDesc *s = _sound->getSounds();
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		DiMUSESndMgr::SoundDesc *curSnd = &s[i];
		if (curSnd && curSnd->inUse) {
			if (curSnd->scheduledForDealloc)
				if (!_engine->diMUSEGetParam(curSnd->soundId, 0x100) && !_engine->diMUSEGetParam(curSnd->soundId, 0x200))
					_sound->closeSound(curSnd);
		}
	}
}

void DiMUSEFilesHandler::setCurrentSpeechFile(const char *fileName) {
	strncpy(_currentSpeechFile, fileName, 60);
}

void DiMUSEFilesHandler::closeSoundImmediatelyById(int soundId) {
	_sound->closeSoundById(soundId);
}

} // End of namespace Scumm
