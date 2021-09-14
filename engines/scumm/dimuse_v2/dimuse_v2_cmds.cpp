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
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TO_LE32(x)	(x)
#define TO_LE16(x)	(x)
#else
#define TO_LE32(x)	bswap_32(x)
#define TO_LE16(x)	bswap_16(x)
#endif
namespace Scumm {

int DiMUSE_v2::cmdsHandleCmd(int cmd, int arg_0, int arg_1, int arg_2, int arg_3, int arg_4,
	int arg_5, int arg_6, int arg_7, int arg_8, int arg_9,
	int arg_10, int arg_11, int arg_12, int arg_13) {

	// Convert the character constant (single quotes '') to string
	char marker[5];
	if (cmd == 17 || cmd == 18 || cmd == 19) {
		for (int i = 0; i < 4; i++) {
#if defined SCUMM_BIG_ENDIAN	
			marker[i] = (arg_1 >> (8 * i)) & 0xff;		
#elif defined SCUMM_LITTLE_ENDIAN
			marker[3 - i] = (arg_1 >> (8 * i)) & 0xff;
#endif
		}
		marker[4] = '\0';
	}

 	switch (cmd) {
	case 0:
		return cmdsInit();
	case 1: // cmds_terminate
		break; 
	case 2: // cmds_print
		break; 
	case 3:
		return cmdsPause();
	case 4:
		return cmdsResume();
	case 5: // cmds_save
		break;
	case 6: // cmds_restore
		break;
	case 7:
		_groupsHandler->setGroupVol(arg_0, arg_1);
		break;
	case 8:
		cmdsStartSound(arg_0, arg_1);
		break;
	case 9:
		cmdsStopSound(arg_0);
		break;
	case 10:
		cmdsStopAllSounds();
		break;
	case 11:
		return cmdsGetNextSound(arg_0);
	case 12:
		cmdsSetParam(arg_0, arg_1, arg_2);
		break;
	case 13:
		return cmdsGetParam(arg_0, arg_1);
	case 14:
		return _fadesHandler->fadeParam(arg_0, arg_1, arg_2, arg_3, arg_4);
	case 15:
		return cmdsSetHook(arg_0, arg_1);
	case 16:
		return cmdsGetHook(arg_0);
	case 17:		
		return _triggersHandler->setTrigger(arg_0, marker, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8, arg_9, arg_10, arg_11, arg_12, arg_13);
	case 18:
		return _triggersHandler->checkTrigger(arg_0, marker, arg_2);
	case 19:
		return _triggersHandler->clearTrigger(arg_0, marker, arg_2);
	case 20:
		return _triggersHandler->deferCommand(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8, arg_9, arg_10, arg_11, arg_12, arg_13);
	case 21:
	case 22:
	case 23:
	case 24:
		// Empty opcodes
		return 0;
	case 25:
		return waveStartStream(arg_0, arg_1, arg_2);
	case 26:
		return waveSwitchStream(arg_0, arg_1, arg_2, arg_3, arg_4);
	case 27:
		return waveProcessStreams();
	case 28:
		return waveQueryStream(arg_0, (int *)arg_1, (int *)arg_2, (int *)arg_3, (int *)arg_4);
	case 29:
		return waveFeedStream(arg_0, (int *)arg_1, arg_2, arg_3);
	case 30:
		return waveLipSync(arg_0, arg_1, arg_2, (int32 *)arg_3, (int32 *)arg_4);
	default:
		debug(5, "DiMUSE_v2::cmds_handleCmds(): bogus opcode ignored.");
		return -1;
	}

	return 0;
}

// Validated
int DiMUSE_v2::cmdsInit() {
	_cmdsRunning60HzCount = 0;
	_cmdsRunning10HzCount = 0;

	if (_filesHandler->init() || _groupsHandler->init() || _fadesHandler->init() ||
		_triggersHandler->init() || waveInit() || _timerHandler->init()) {
		return -1;
	}

	_cmdsPauseCount = 0;
	return 48;
}

int DiMUSE_v2::cmdsDeinit() {
	_timerHandler->deinit();
	waveTerminate();
	waveOutDeinit();
	_triggersHandler->deinit();
	_fadesHandler->deinit();
	_groupsHandler->deinit();
	_filesHandler->deinit();
	_cmdsPauseCount = 0;
	_cmdsRunning60HzCount = 0;
	_cmdsRunning10HzCount = 0;

	return 0;
}

int DiMUSE_v2::cmdsTerminate() {
	return 0;
}

int DiMUSE_v2::cmdsPause() {
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

int DiMUSE_v2::cmdsResume() {
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

/*
int DiMUSE_v2::cmds_save(int * buffer, int bufferSize) {
	if (bufferSize < 10000) {
		debug(5, "ERR: save buffer too small...");
		return -1;
	}

	buffer[0] = 0x30;
	buffer[1] = 6;

	int savedSize = fades_save((buffer + 8), bufferSize - 8);
	if (savedSize >= 0) {
		int tmpSize = savedSize + 8;
		savedSize = triggers_save(buffer + tmpSize, bufferSize - tmpSize);
		if (savedSize >= 0) {
			tmpSize += savedSize;
		}
		savedSize = wave_save(buffer + tmpSize, bufferSize - tmpSize);
		if (savedSize >= 0)
			savedSize += tmpSize;
	}

	return savedSize;
}*/

/*int DiMUSE_v2::cmds_restore(int *buffer) {
	fades_moduleDeinit();
	triggers_clear();
	wave_stopAllSounds();

	if (buffer[0] != 0x30) {
		printf("ERR: restore buffer contains bad data...");
		return -1;
	}

	if (buffer[1] != *(int *)cmd_initDataPtr->waveMixCount) {
		printf("ERR: waveMixCount changed between save ");
		return -1;
	}

	int fadesSize = fades_restore(buffer + 2);
	int triggersSize = triggers_restore(buffer + fadesSize + 8);
	int waveSize = wave_restore(buffer + fadesSize + triggersSize + 8);
	return fadesSize + triggersSize + waveSize + 8;
}*/

int DiMUSE_v2::cmdsStartSound(int soundId, int priority) {
	uint8 *src = _filesHandler->getSoundAddrData(soundId);

	if (src == NULL) {
		debug(5, "DiMUSE_v2::cmds_startSound(): ERROR: resource address for sound %d is NULL", soundId);
		return -1;
	}

	// Check for the "iMUS" header
	if (READ_BE_UINT32(src) == MKTAG('i', 'M', 'U', 'S'))
		return waveStartSound(soundId, priority);

	return -1;
}

int DiMUSE_v2::cmdsStopSound(int soundId) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveStopSound(soundId);
}

int DiMUSE_v2::cmdsStopAllSounds() {
	return _triggersHandler->clearAllTriggers() | waveStopAllSounds();
}

int DiMUSE_v2::cmdsGetNextSound(int soundId) {
	return waveGetNextSound(soundId);
}

int DiMUSE_v2::cmdsSetParam(int soundId, int subCmd, int value) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveSetParam(soundId, subCmd, value);
}

int DiMUSE_v2::cmdsGetParam(int soundId, int subCmd) {
	int result = _filesHandler->getNextSound(soundId);

	if (subCmd != 0) {
		if (subCmd == 0x200) {
			return _triggersHandler->countPendingSounds(soundId);
		}

		if (result == 2) {
			return waveGetParam(soundId, subCmd);
		}

		result = (subCmd == 0x100) - 1;
	}

	return result;
}

int DiMUSE_v2::cmdsSetHook(int soundId, int hookId) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveSetHook(soundId, hookId);
}

int DiMUSE_v2::cmdsGetHook(int soundId) {
	int result = _filesHandler->getNextSound(soundId);

	if (result != 2)
		return -1;

	return waveGetHook(soundId);
}

} // End of namespace Scumm
