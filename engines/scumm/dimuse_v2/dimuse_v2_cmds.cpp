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

	int DiMUSE_v2::cmds_handleCmds(int cmd, int arg_0, int arg_1, int arg_2, int arg_3, int arg_4,
		int arg_5, int arg_6, int arg_7, int arg_8, int arg_9,
		int arg_10, int arg_11, int arg_12, int arg_13) {

 		switch (cmd) {
		case 0:
			return cmds_init();
		case 1:
			return cmds_terminate();
		case 2:
			return cmds_print(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
		case 3:
			return cmds_pause();
		case 4:
			return cmds_resume();
		case 5: // cmds_save
			break;
		case 6: // cmds_restore
			break;
		case 7:
			groups_setGroupVol(arg_0, arg_1);
			break;
		case 8:
			cmds_startSound(arg_0, arg_1);
			break;
		case 9:
			cmds_stopSound(arg_0);
			break;
		case 10:
			cmds_stopAllSounds();
			break;
		case 11:
			cmds_getNextSound(arg_0);
			break;
		case 12:
			cmds_setParam(arg_0, arg_1, arg_2);
			break;
		case 13:
			cmds_getParam(arg_0, arg_1);
			break;
		case 14:
			return fades_fadeParam(arg_0, arg_1, arg_2, arg_3);
		case 15:
			return cmds_setHook(arg_0, arg_1);
		case 16:
			return cmds_getHook(arg_0);
		case 17:
			return triggers_setTrigger(arg_0, (char *)arg_1, arg_2); // TODO: check for number of args 
		case 18:
			return triggers_checkTrigger(arg_0, (char *)arg_1, arg_2);
		case 19:
			return triggers_clearTrigger(arg_0, (char *)arg_1, arg_2);
		case 20:
			// triggers_deferCommand
			break;
		case 21:
		case 22:
		case 23:
		case 24:
			// Empty opcodes
			return 0;
		case 25:
			return wave_startSound(arg_0, arg_1);
		case 26:
			return wave_switchStream(arg_0, arg_1, arg_2, arg_3, arg_4);
		case 27:
			return wave_processStreams();
		case 28:
			return wave_queryStream(arg_0, (int *)arg_1, (int *)arg_2, (int *)arg_3, (int *)arg_4);
		case 29:
			return wave_feedStream(arg_0, arg_1, arg_2, arg_3);
		case 30:
			return wave_lipSync(arg_0, arg_1, arg_2, (int *)arg_3, (int *)arg_4);
		default:
			debug(5, "DiMUSE_v2::cmds_handleCmds(): bogus opcode ignored.");
			return -1;
		}
		
		/*

		if (func != NULL && cmd < 30) {
			if (cmd != 17 && cmd != 20)
				func(arg_0, arg_1, arg_2, arg_3, arg_4,
					arg_5, arg_6, arg_7, arg_8, arg_9);
			else
				func(arg_0, arg_1, arg_2, arg_3, arg_4,
					arg_5, arg_6, arg_7, arg_8, arg_9,
					arg_10, arg_11, arg_12, arg_13);
		}*/

		
		return -1;

	}

	// Validated
	int DiMUSE_v2::cmds_init() {
		cmd_hostIntHandler = 0;
		cmd_hostIntUsecCount = 20000;
		cmd_runningHostCount = 0;
		cmd_running60HzCount = 0;
		cmd_running10HzCount = 0;
		//initDataPtr->num60hzIterations = 0;

		if (files_moduleInit() || groups_moduleInit() || /*fades_moduleInit() ||*/
			/*triggers_moduleInit(initDataPtr) ||*/ wave_init() || timer_moduleInit()) {
			return -1;
		}

		//cmd_initDataPtr = initDataPtr;
		cmd_pauseCount = 0;
		return 48;
	}

	// Validated
	int DiMUSE_v2::cmds_deinit() {
		//cmd_initDataPtr = NULL;
		timer_moduleDeinit();
		wave_terminate();
		/*waveout_free();
		triggers_clear();
		fades_moduleDeinit();*/
		groups_moduleDeinit();
		files_moduleDeinit();
		cmd_pauseCount = 0;
		cmd_hostIntHandler = 0;
		cmd_hostIntUsecCount = 0;
		cmd_runningHostCount = 0;
		cmd_running60HzCount = 0;
		cmd_running10HzCount = 0;

		return 0;
	}

	// Validated
	int DiMUSE_v2::cmds_terminate() {
		//vh_ReleaseModule("iMUSE");
		return 0;
	}

	// Duplicate save/load stuff, I don't think we need it for the time being
	int DiMUSE_v2::cmds_persistence(int cmd, void *funcCall) {
		return 0;
	}

	// Validated (or at least good enough)
	int DiMUSE_v2::cmds_print(int param1, int param2, int param3, int param4, int param5, int param6, int param7) {
		debug(5, "cmds_print(): %d %d %d %d %d %d %d", param1, param2, param3, param4, param5, param6, param7);
		return 0;
	}

	// Validated
	int DiMUSE_v2::cmds_pause() {
		int result = 0;

		if (cmd_pauseCount == 0) {
			result = wave_pause();
		}

		if (!result) {
			result = cmd_pauseCount + 1;
		}
		cmd_pauseCount++;

		return result;
	}

	// Validated
	int DiMUSE_v2::cmds_resume() {
		int result = 0;

		if (cmd_pauseCount == 1) {
			result = wave_resume();
		}

		if (cmd_pauseCount != 0) {
			cmd_pauseCount--;
		}

		if (!result) {
			result = cmd_pauseCount;
		}

		return result;
	}

	/*
	// Validated
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

	// Validated
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

	// Validated
	int DiMUSE_v2::cmds_startSound(int soundId, int priority) {
		uint8 *src = files_getSoundAddrData(soundId);
		int stringIndex = 0;

		if (src == NULL) {
			debug(5, "ERR: null sound addr in StartSound()...");
			return -1;
		}

		// Check for the "iMUS" header
		uint8 *iMUSstring = (uint8*)TO_LE32("iMUS");
		do {
			if (src[stringIndex] != iMUSstring[stringIndex])
				break;
			stringIndex += 1;
		} while (stringIndex < 4);

		if (stringIndex == 4) {
			return wave_startSound(soundId, priority);
		}

		return -1;
	}

	// Validated
	int DiMUSE_v2::cmds_stopSound(int soundId) {
		int result = files_getNextSound(soundId);

		if (result != 2)
			return -1;

		return wave_stopSound(soundId);
	}

	// Validated
	int DiMUSE_v2::cmds_stopAllSounds() {
		int result = 1;// fades_moduleDeinit();
		result |= 1;//triggers_clear();
		result |= wave_stopAllSounds();

		return result;
	}

	// Validated
	int DiMUSE_v2::cmds_getNextSound(int soundId) {
		return wave_getNextSound(soundId);
	}

	// Validated
	int DiMUSE_v2::cmds_setParam(int soundId, int subCmd, int value) {
		int result = files_getNextSound(soundId);

		if (result != 2)
			return -1;

		return wave_setParam(soundId, subCmd, value);
	}

	// Validated
	int DiMUSE_v2::cmds_getParam(int soundId, int subCmd) {
		int result = files_getNextSound(soundId);

		if (subCmd != 0) {
			if (subCmd == 0x200) {
				return 0;// triggers_countPendingSounds(soundId);
			}

			if (result == 2) {
				return wave_getParam(soundId, subCmd);
			}

			result = (subCmd == 0x100) - 1;
		}

		return result;
	}

	// Validated
	int DiMUSE_v2::cmds_setHook(int soundId, int hookId) {
		int result = files_getNextSound(soundId);

		if (result != 2)
			return -1;

		return wave_setHook(soundId, hookId);
	}

	// Validated
	int DiMUSE_v2::cmds_getHook(int soundId) {
		int result = files_getNextSound(soundId);

		if (result != 2)
			return -1;

		return wave_getHook(soundId);
	}

	// Validated
	int DiMUSE_v2::cmds_debug() {
		debug(5, "initImuseDataPtr: %p", 0);
		debug(5, "pauseCount: %d", cmd_pauseCount);
		debug(5, "hostIntHandler: %p", cmd_hostIntHandler);
		debug(5, "hostIntUsecCount: %d", cmd_hostIntUsecCount);
		debug(5, "runningHostCount: %d", cmd_runningHostCount);
		debug(5, "running60HzCount: %d", cmd_running60HzCount);
		debug(5, "running10HzCount: %d", cmd_running10HzCount);

		return 0;
	}
} // End of namespace Scumm
