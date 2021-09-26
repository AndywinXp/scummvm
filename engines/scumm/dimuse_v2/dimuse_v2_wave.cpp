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
	return 0;
}

int DiMUSE_v2::waveTerminate() {
	return 0;
}

int DiMUSE_v2::wavePause() {
	Common::StackLock lock(_mutex);
	tracksPause();
	return 0;
}

int DiMUSE_v2::waveResume() {
	Common::StackLock lock(_mutex);
	tracksResume();
	return 0;
}

void DiMUSE_v2::waveSaveLoad(Common::Serializer &ser) {
	Common::StackLock lock(_mutex);
	tracksSaveLoad(ser);
}

void DiMUSE_v2::waveUpdateGroupVolumes() {
	Common::StackLock lock(_mutex);
	tracksSetGroupVol();
}

int DiMUSE_v2::waveStartSound(int soundId, int priority) {
	Common::StackLock lock(_mutex);
	return tracksStartSound(soundId, priority, 0);
}

int DiMUSE_v2::waveStopSound(int soundId) {
	Common::StackLock lock(_mutex);
	return tracksStopSound(soundId);
}

int DiMUSE_v2::waveStopAllSounds() {
	Common::StackLock lock(_mutex);
	return tracksStopAllSounds();
}

int DiMUSE_v2::waveGetNextSound(int soundId) {
	Common::StackLock lock(_mutex);
	return tracksGetNextSound(soundId);
}

int DiMUSE_v2::waveSetParam(int soundId, int opcode, int value) {
	Common::StackLock lock(_mutex);
	return tracksSetParam(soundId, opcode, value);
}

int DiMUSE_v2::waveGetParam(int soundId, int opcode) {
	Common::StackLock lock(_mutex);
	return tracksGetParam(soundId, opcode);
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

	Common::StackLock lock(_mutex);
	return tracksStartSound(soundId, priority, bufferId);
}

int DiMUSE_v2::waveSwitchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1) {
	Common::StackLock lock(_mutex);
	return dispatchSwitchStream(oldSoundId, newSoundId, fadeLengthMs, fadeSyncFlag2, fadeSyncFlag1);
}

int DiMUSE_v2::waveProcessStreams() {
	Common::StackLock lock(_mutex);
	return streamerProcessStreams();
}

int DiMUSE_v2::waveQueryStream(int soundId, int *bufSize, int *criticalSize, int *freeSpace, int *paused) {
	Common::StackLock lock(_mutex);
	return tracksQueryStream(soundId, bufSize, criticalSize, freeSpace, paused);
}

int DiMUSE_v2::waveFeedStream(int soundId, uint8 *srcBuf, int sizeToFeed, int paused) {
	Common::StackLock lock(_mutex);
	return tracksFeedStream(soundId, srcBuf, sizeToFeed, paused);
}

int DiMUSE_v2::waveLipSync(int soundId, int syncId, int msPos, int *width, int *height) {
	return tracksLipSync(soundId, syncId, msPos, width, height);
}

} // End of namespace Scumm
