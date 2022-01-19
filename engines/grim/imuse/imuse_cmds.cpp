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

#include "engines/grim/imuse/imuse_engine.h"

namespace Grim {

// We have some uintptr arguments as commands 28, 29 and 30 actually require pointer arguments
// Unfortunately this makes function calls for other command a little less pretty...
int Imuse::cmdsHandleCmd(int cmd, uint8 *ptr, int a, int b, int c, int d, int e,
	int f, int g, int h, int i, int j, int k, int l, int m, int n) {

	// Convert the character constant (single quotes '') to string
	char marker[5];
	if (cmd == 17 || cmd == 18 || cmd == 19) {
		for (int index = 0; index < 4; index++) {
#if defined SCUMM_BIG_ENDIAN
			marker[index] = (b >> (8 * index)) & 0xff;
#elif defined SCUMM_LITTLE_ENDIAN
			marker[3 - index] = (b >> (8 * index)) & 0xff;
#endif
		}
		marker[4] = '\0';
	}

	switch (cmd) {
	case 0:
		return cmdsInit();
	case 3:
		return cmdsPause();
	case 4:
		return cmdsResume();
	case 7:
		return _groupsHandler->setGroupVol(a, b);
	case 8:
		cmdsStartSound(a, b);
		break;
	case 9:
		cmdsStopSound(a);
		break;
	case 10:
		cmdsStopAllSounds();
		break;
	case 11:
		return cmdsGetNextSound(a);
	case 12:
		cmdsSetParam(a, b, c);
		break;
	case 13:
		return cmdsGetParam(a, b);
	case 14:
		return _fadesHandler->fadeParam(a, b, c, d);
	case 15:
		return cmdsSetHook(a, b);
	case 16:
		return cmdsGetHook(a);
	case 17:
		return _triggersHandler->setTrigger(a, marker, c, d, e, f, g, h, i, j, k, l, m, n);
	case 18:
		return _triggersHandler->checkTrigger(a, marker, c);
	case 19:
		return _triggersHandler->clearTrigger(a, marker, c);
	case 20:
		return _triggersHandler->deferCommand(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
	case 25:
		return waveStartStream(a, b, c);
	case 26:
		return waveSwitchStream(a, b, c, d, e);
	case 27:
		return waveProcessStreams();
	case 29:
		return waveFeedStream(a, ptr, c, d);
	default:
		Debug::debug(Debug::Sound, "Imuse::cmdsHandleCmd(): bogus/unused transitionType ignored (%d).", cmd);
		return -1;
	}

	return 0;
}

int Imuse::cmdsInit() {
	_cmdsRunning50HzCount = 0;
	_cmdsRunning5HzCount = 0;

	if (_groupsHandler->init() || _fadesHandler->init() ||
		_triggersHandler->init() || waveInit()) {
		return -1;
	}

	_cmdsPauseCount = 0;
	return 48;
}

int Imuse::cmdsDeinit() {
	waveTerminate();
	waveOutDeinit();
	_triggersHandler->deinit();
	_fadesHandler->deinit();
	_cmdsPauseCount = 0;
	_cmdsRunning50HzCount = 0;
	_cmdsRunning5HzCount = 0;

	return 0;
}

int Imuse::cmdsTerminate() {
	return 0;
}

int Imuse::cmdsPause() {
	int result = 0;

	if (_cmdsPauseCount == 0) {
		result = wavePause();
	}

	if (!result) {
		result = _cmdsPauseCount + 1;
	}
	_cmdsPauseCount++;

	return result;
}

int Imuse::cmdsResume() {
	int result = 0;

	if (_cmdsPauseCount == 1) {
		result = waveResume();
	}

	if (_cmdsPauseCount != 0) {
		_cmdsPauseCount--;
	}

	if (!result) {
		result = _cmdsPauseCount;
	}

	return result;
}

void Imuse::cmdsSaveLoad(Common::Serializer &ser) {
	// Serialize in this order:
	// - Open files
	// - Fades
	// - Triggers
	// - Pass the control to waveSaveLoad and then tracksSaveLoad, which will serialize:
	//     - Dispatches
	//     - Tracks (with SYNCs, if the game is COMI)
	// - State and sequence info
	// - Attributes
	// - Full Throttle's music cue ID

	_filesHandler->saveLoad(ser);
	_fadesHandler->saveLoad(ser);
	_triggersHandler->saveLoad(ser);
	waveSaveLoad(ser);
	ser.syncAsSint32LE(_curMusicState, VER(103));
	ser.syncAsSint32LE(_curMusicSeq, VER(103));
	ser.syncAsSint32LE(_nextSeqToPlay, VER(103));
	ser.syncArray(_attributes, 188, Common::Serializer::Sint32LE, VER(103));
	ser.syncAsSint32LE(_curMusicCue, VER(103));
}

int Imuse::cmdsStartSound(int soundId, int priority) {
	uint8 *src = _filesHandler->getSoundAddrData(soundId);

	if (src == nullptr) {
		Debug::debug(Debug::Sound, "Imuse::cmdsStartSound(): ERROR: resource address for sound %d is NULL", soundId);
		return -1;
	}

	// Check for the "iMUS" header
	if (READ_BE_UINT32(src) == MKTAG('i', 'M', 'U', 'S'))
		return waveStartSound(soundId, priority);

	return -1;
}

int Imuse::cmdsStopSound(int soundId) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveStopSound(soundId);
}

int Imuse::cmdsStopAllSounds() {
	return _triggersHandler->clearAllTriggers() | waveStopAllSounds();
}

int Imuse::cmdsGetNextSound(int soundId) {
	return waveGetNextSound(soundId);
}

int Imuse::cmdsSetParam(int soundId, int subCmd, int value) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveSetParam(soundId, subCmd, value);
}

int Imuse::cmdsGetParam(int soundId, int subCmd) {
	int result = _filesHandler->getNextSound(soundId);

	if (subCmd != 0) {
		if (subCmd == DIMUSE_P_TRIGS_SNDS) {
			return _triggersHandler->countPendingSounds(soundId);
		}

		if (result == 2) {
			return waveGetParam(soundId, subCmd);
		}

		result = (subCmd == DIMUSE_P_SND_TRACK_NUM) - 1;
	}

	return result;
}

int Imuse::cmdsSetHook(int soundId, int hookId) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveSetHook(soundId, hookId);
}

int Imuse::cmdsGetHook(int soundId) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveGetHook(soundId);
}

} // End of namespace Scumm
