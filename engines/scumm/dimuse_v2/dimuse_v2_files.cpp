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
	_sound = new ImuseDigiSndMgr(vm, true);
	assert(_sound);
	_vm = vm;
}

DiMUSEFilesHandler::~DiMUSEFilesHandler() {
	delete _sound;
}

void DiMUSEFilesHandler::saveLoad(Common::Serializer &ser) {
	int curSound = 0;
	ImuseDigiSndMgr::SoundDesc *sounds = _sound->getSounds();

	ser.syncArray(_currentSpeechFile, 60, Common::Serializer::SByte, VER(103));
	if (ser.isSaving()) {
		for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
			ser.syncAsSint32LE(sounds[l].soundId, VER(103));
		}
	}

	if (ser.isLoading()) {
		// Close prior sounds if we're reloading (needed for edge cases like the recipe book in COMI)
		for (int l = 0; l < MAX_IMUSE_SOUNDS; l++) {
			_sound->closeSound(&sounds[l]);
		}

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
	Common::StackLock lock(_mutex);
	// This function is always used for SFX (tracks which do not
	// have a stream pointer), hence the use of the resource address
	if (soundId != 0) {
		_vm->ensureResourceLoaded(rtSound, soundId);
		_vm->_res->lock(rtSound, soundId);
		byte *ptr = _vm->getResourceAddress(rtSound, soundId);
		if (!ptr) {
			_vm->_res->unlock(rtSound, soundId);
			return NULL;
		}
		return ptr;
			
	}
	debug(5, "DiMUSEFilesHandler::getSoundAddrData(): soundId is 0 or out of range");
	return NULL;
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

int DiMUSEFilesHandler::seek(int soundId, int offset, int mode, int bufId) {
	// This function and files_read() are used for sounds for which a stream is needed
	// (speech and music), therefore they will always refer to sounds in a bundle file
	// The seeked position is in reference to the decompressed sound

	// A soundId > 10000 is a SAN cutscene
	if ((_vm->_game.id == GID_DIG) && (soundId > kTalkSoundID))
		return 0;

	if (soundId != 0) {
		char fileName[60] = "";
		getFilenameFromSoundId(soundId, fileName, sizeof(fileName));

		ImuseDigiSndMgr::SoundDesc *s = _sound->findSoundById(soundId);
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
	if (soundId != 0) {
		char fileName[60] = "";
		getFilenameFromSoundId(soundId, fileName, sizeof(fileName));

		ImuseDigiSndMgr::SoundDesc *s = _sound->getSounds();
		ImuseDigiSndMgr::SoundDesc *curSnd = NULL;
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

int DiMUSEFilesHandler::openSound(int soundId) {
	char fileName[60] = "";
	getFilenameFromSoundId(soundId, fileName, sizeof(fileName));
	ImuseDigiSndMgr::SoundDesc *s = NULL;
	int groupId = soundId == kTalkSoundID ? IMUSE_VOLGRP_VOICE : IMUSE_VOLGRP_MUSIC;
	s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, groupId, -1);
	if (!s)
		s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, groupId, 1);
	if (!s)
		s = _sound->openSound(soundId, fileName, IMUSE_BUNDLE, groupId, 2);
	if (!s) {
		debug(5, "DiMUSEFilesHandler::openSound(): can't open sound %d (%s)", soundId, fileName);
		return 1;
	}

	return 0;
}

void DiMUSEFilesHandler::closeSound(int soundId) {
	_sound->scheduleSoundForDeallocation(soundId);
}

void DiMUSEFilesHandler::closeAllSounds() {
	ImuseDigiSndMgr::SoundDesc *s = _sound->getSounds();
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		if (s[i].inUse)
			closeSound(s->soundId);
	}

	_engine->flushTracks();
}

void DiMUSEFilesHandler::getFilenameFromSoundId(int soundId, char *fileName, size_t size) {
	int i = 0;

	if (soundId == kTalkSoundID) {
		Common::strlcpy(fileName, _currentSpeechFile, size);
	}

	if (_vm->_game.id == GID_CMI) {
		if (soundId < 2000) {
			while (_comiStateMusicTable[i].soundId != -1) {
				if (_comiStateMusicTable[i].soundId == soundId) {
					Common::strlcpy(fileName, _comiStateMusicTable[i].filename, size);
					return;
				}
				i++;
			}
		} else {
			while (_comiSeqMusicTable[i].soundId != -1) {
				if (_comiSeqMusicTable[i].soundId == soundId) {
					Common::strlcpy(fileName, _comiSeqMusicTable[i].filename, size);
					return;
				}
				i++;
			}
		}
	} else {
		if (soundId < 2000) {
			while (_digStateMusicTable[i].soundId != -1) {
				if (_digStateMusicTable[i].soundId == soundId) {
					Common::strlcpy(fileName, _digStateMusicTable[i].filename, size);
					return;
				}
				i++;
			}
		} else {
			while (_digSeqMusicTable[i].soundId != -1) {
				if (_digSeqMusicTable[i].soundId == soundId) {
					Common::strlcpy(fileName, _digSeqMusicTable[i].filename, size);
					return;
				}
				i++;
			}
		}
	}
}

void DiMUSEFilesHandler::allocSoundBuffer(int bufId, int size, int loadSize, int criticalSize) {
	DiMUSESoundBuffer *selectedSoundBuf;

	selectedSoundBuf = &_soundBuffers[bufId];
	selectedSoundBuf->buffer = (uint8 *)malloc(size);
	selectedSoundBuf->bufSize = size;
	selectedSoundBuf->loadSize = loadSize;
	selectedSoundBuf->criticalSize = criticalSize;
}

void DiMUSEFilesHandler::deallocSoundBuffer(int bufId) {
	DiMUSESoundBuffer *selectedSoundBuf;

	selectedSoundBuf = &_soundBuffers[bufId];
	free(selectedSoundBuf->buffer);
	selectedSoundBuf->buffer = NULL;
}

void DiMUSEFilesHandler::flushSounds() {
	ImuseDigiSndMgr::SoundDesc *s = _sound->getSounds();
	for (int i = 0; i < MAX_IMUSE_SOUNDS; i++) {
		ImuseDigiSndMgr::SoundDesc *curSnd = &s[i];
		if (curSnd && curSnd->inUse) {
			if (curSnd->scheduledForDealloc)
				if (!_engine->diMUSEGetParam(curSnd->soundId, P_SND_TRACK_NUM) && !_engine->diMUSEGetParam(curSnd->soundId, 0x200))
					_sound->closeSound(curSnd);
		}
	}
}

int DiMUSEFilesHandler::setCurrentSpeechFile(const char *fileName) {
	Common::strlcpy(_currentSpeechFile, fileName, sizeof(_currentSpeechFile));
	if (openSound(kTalkSoundID))
		return 1;

	return 0;
}

void DiMUSEFilesHandler::closeSoundImmediatelyById(int soundId) {
	_sound->closeSoundById(soundId);
}

} // End of namespace Scumm
