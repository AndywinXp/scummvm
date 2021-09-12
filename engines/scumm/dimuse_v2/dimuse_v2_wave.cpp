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

	int DiMUSE_v2::wave_init() {
		if (tracks_moduleInit())
			return -1;
		wvSlicingHalted = 0;
		return 0;
	}

	int DiMUSE_v2::wave_terminate() {
		return 0;
	}

	int DiMUSE_v2::wave_pause() {
		wvSlicingHalted++;
		tracks_pause();
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return 0;
	}

	int DiMUSE_v2::wave_resume() {
		wvSlicingHalted++;
		tracks_resume();
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return 0;
	}

	int DiMUSE_v2::wave_save(unsigned char *buffer, int bufferSize) {
		wvSlicingHalted++;
		int result = tracks_save(buffer, bufferSize);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}

		return result;
	}

	int DiMUSE_v2::wave_restore(unsigned char *buffer) {
		wvSlicingHalted++;
		int result = tracks_restore(buffer);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	void DiMUSE_v2::wave_setGroupVol() {
		wvSlicingHalted++;
		tracks_setGroupVol();
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
	}

	int DiMUSE_v2::wave_startSound(int soundId, int priority) {
		wvSlicingHalted++;
		int result = tracks_startSound(soundId, priority, 0);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_stopSound(int soundId) {
		wvSlicingHalted++;
		int result = tracks_stopSound(soundId);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_stopAllSounds() {
		wvSlicingHalted++;
		int result = tracks_stopAllSounds();
		if (wvSlicingHalted != 0) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_getNextSound(int soundId) {
		wvSlicingHalted++;
		int result = tracks_getNextSound(soundId);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_setParam(int soundId, int opcode, int value) {
		wvSlicingHalted++;
		int result = tracks_setParam(soundId, opcode, value);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_getParam(int soundId, int opcode) {
		wvSlicingHalted++;
		int result = tracks_getParam(soundId, opcode);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_setHook(int soundId, int hookId) {
		return tracks_setHook(soundId, hookId);
	}

	int DiMUSE_v2::wave_getHook(int soundId) {
		return tracks_getHook(soundId);
	}

	int DiMUSE_v2::wave_startStream(int soundId, int priority, int bufferId) {
		if (!files_checkRange(soundId))
			return -1;
		wvSlicingHalted++;
		int result = tracks_startSound(soundId, priority, bufferId);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_switchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1) {
		wvSlicingHalted++;
		int result = dispatch_switchStream(oldSoundId, newSoundId, fadeLengthMs, fadeSyncFlag2, fadeSyncFlag1);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_processStreams() {
		wvSlicingHalted++;
		int result = streamer_processStreams();
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_queryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
		wvSlicingHalted++;
		int result = tracks_queryStream(soundId, bufSize, criticalSize, freeSpace, paused);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_feedStream(int soundId, int *srcBuf, int sizeToFeed, int paused) {
		wvSlicingHalted++;
		int result = tracks_feedStream(soundId, (uint8 *)srcBuf, sizeToFeed, paused);
		if (wvSlicingHalted) {
			wvSlicingHalted--;
		}
		return result;
	}

	int DiMUSE_v2::wave_lipSync(int soundId, int syncId, int msPos, int *width, int *height) {
		return tracks_lipSync(soundId, syncId, msPos, width, height);
	}
} // End of namespace Scumm
