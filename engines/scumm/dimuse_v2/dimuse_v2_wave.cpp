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

	int DiMUSE_v2::waveInit() {
		if (tracksInit())
			return -1;
		_wvSlicingHalted = 0;
		return 0;
	}

	int DiMUSE_v2::waveTerminate() {
		return 0;
	}

	int DiMUSE_v2::wavePause() {
		_wvSlicingHalted++;
		tracksPause();
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return 0;
	}

	int DiMUSE_v2::waveResume() {
		_wvSlicingHalted++;
		tracksResume();
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return 0;
	}

	int DiMUSE_v2::waveSave(unsigned char *buffer, int bufferSize) {
		_wvSlicingHalted++;
		int result = tracksSave(buffer, bufferSize);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}

		return result;
	}

	int DiMUSE_v2::waveRestore(unsigned char *buffer) {
		_wvSlicingHalted++;
		int result = tracksRestore(buffer);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	void DiMUSE_v2::waveUpdateGroupVolumes() {
		_wvSlicingHalted++;
		tracksSetGroupVol();
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
	}

	int DiMUSE_v2::waveStartSound(int soundId, int priority) {
		_wvSlicingHalted++;
		int result = tracksStartSound(soundId, priority, 0);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveStopSound(int soundId) {
		_wvSlicingHalted++;
		int result = tracksStopSound(soundId);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveStopAllSounds() {
		_wvSlicingHalted++;
		int result = tracksStopAllSounds();
		if (_wvSlicingHalted != 0) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveGetNextSound(int soundId) {
		_wvSlicingHalted++;
		int result = tracksGetNextSound(soundId);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveSetParam(int soundId, int opcode, int value) {
		_wvSlicingHalted++;
		int result = tracksSetParam(soundId, opcode, value);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveGetParam(int soundId, int opcode) {
		_wvSlicingHalted++;
		int result = tracksGetParam(soundId, opcode);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveSetHook(int soundId, int hookId) {
		return tracksSetHook(soundId, hookId);
	}

	int DiMUSE_v2::waveGetHook(int soundId) {
		return tracksGetHook(soundId);
	}

	int DiMUSE_v2::waveStartStream(int soundId, int priority, int bufferId) {
		if (!_filesHandler->checkIdInRange(soundId))
			return -1;
		_wvSlicingHalted++;
		int result = tracksStartSound(soundId, priority, bufferId);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveSwitchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1) {
		_wvSlicingHalted++;
		int result = dispatchSwitchStream(oldSoundId, newSoundId, fadeLengthMs, fadeSyncFlag2, fadeSyncFlag1);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveProcessStreams() {
		_wvSlicingHalted++;
		int result = streamerProcessStreams();
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
		_wvSlicingHalted++;
		int result = tracksQueryStream(soundId, bufSize, criticalSize, freeSpace, paused);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveFeedStream(int soundId, int *srcBuf, int sizeToFeed, int paused) {
		_wvSlicingHalted++;
		int result = tracksFeedStream(soundId, (uint8 *)srcBuf, sizeToFeed, paused);
		if (_wvSlicingHalted) {
			_wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveLipSync(int soundId, int syncId, int msPos, int *width, int *height) {
		return tracksLipSync(soundId, syncId, msPos, width, height);
	}
} // End of namespace Scumm
