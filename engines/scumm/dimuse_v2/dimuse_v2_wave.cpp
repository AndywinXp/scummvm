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
		_waveSlicingHalted = 0;
		return 0;
	}

	int DiMUSE_v2::waveTerminate() {
		return 0;
	}

	int DiMUSE_v2::wavePause() {
		_waveSlicingHalted++;
		tracksPause();
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return 0;
	}

	int DiMUSE_v2::waveResume() {
		_waveSlicingHalted++;
		tracksResume();
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return 0;
	}

	void DiMUSE_v2::waveSaveLoad(Common::Serializer &ser) {
		_waveSlicingHalted++;
		tracksSaveLoad(ser);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
	}

	void DiMUSE_v2::waveUpdateGroupVolumes() {
		_waveSlicingHalted++;
		tracksSetGroupVol();
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
	}

	int DiMUSE_v2::waveStartSound(int soundId, int priority) {
		_waveSlicingHalted++;
		int result = tracksStartSound(soundId, priority, 0);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveStopSound(int soundId) {
		_waveSlicingHalted++;
		int result = tracksStopSound(soundId);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveStopAllSounds() {
		_waveSlicingHalted++;
		int result = tracksStopAllSounds();
		if (_waveSlicingHalted != 0) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveGetNextSound(int soundId) {
		_waveSlicingHalted++;
		int result = tracksGetNextSound(soundId);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveSetParam(int soundId, int opcode, int value) {
		_waveSlicingHalted++;
		int result = tracksSetParam(soundId, opcode, value);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveGetParam(int soundId, int opcode) {
		_waveSlicingHalted++;
		int result = tracksGetParam(soundId, opcode);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
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
		_waveSlicingHalted++;
		int result = tracksStartSound(soundId, priority, bufferId);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveSwitchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1) {
		_waveSlicingHalted++;
		int result = dispatchSwitchStream(oldSoundId, newSoundId, fadeLengthMs, fadeSyncFlag2, fadeSyncFlag1);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveProcessStreams() {
		_waveSlicingHalted++;
		int result = streamerProcessStreams();
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
		_waveSlicingHalted++;
		int result = tracksQueryStream(soundId, bufSize, criticalSize, freeSpace, paused);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveFeedStream(int soundId, int *srcBuf, int sizeToFeed, int paused) {
		_waveSlicingHalted++;
		int result = tracksFeedStream(soundId, (uint8 *)srcBuf, sizeToFeed, paused);
		if (_waveSlicingHalted) {
			_waveSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::waveLipSync(int soundId, int syncId, int msPos, int *width, int *height) {
		return tracksLipSync(soundId, syncId, msPos, width, height);
	}
} // End of namespace Scumm
