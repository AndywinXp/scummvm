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

int Imuse::waveInit() {
	if (tracksInit())
		return -1;
	return 0;
}

int Imuse::waveTerminate() {
	return 0;
}

int Imuse::wavePause() {
	Common::StackLock lock(_mutex);
	tracksPause();
	return 0;
}

int Imuse::waveResume() {
	Common::StackLock lock(_mutex);
	tracksResume();
	return 0;
}

void Imuse::waveSaveLoad(Common::Serializer &ser) {
	Common::StackLock lock(_mutex);
	tracksSaveLoad(ser);
}

void Imuse::waveUpdateGroupVolumes() {
	Common::StackLock lock(_mutex);
	tracksSetGroupVol();
}

int Imuse::waveStartSound(int soundId, int priority) {
	Common::StackLock lock(_mutex);
	return tracksStartSound(soundId, priority, 0);
}

int Imuse::waveStopSound(int soundId) {
	Common::StackLock lock(_mutex);
	return tracksStopSound(soundId);
}

int Imuse::waveStopAllSounds() {
	Common::StackLock lock(_mutex);
	return tracksStopAllSounds();
}

int Imuse::waveGetNextSound(int soundId) {
	return tracksGetNextSound(soundId);
}

int Imuse::waveSetParam(int soundId, int transitionType, int value) {
	Common::StackLock lock(_mutex);
	return tracksSetParam(soundId, transitionType, value);
}

int Imuse::waveGetParam(int soundId, int transitionType) {
	Common::StackLock lock(_mutex);
	return tracksGetParam(soundId, transitionType);
}

int Imuse::waveSetHook(int soundId, int hookId) {
	return tracksSetHook(soundId, hookId);
}

int Imuse::waveGetHook(int soundId) {
	return tracksGetHook(soundId);
}

int Imuse::waveStartStream(int soundId, int priority, int bufferId) {
	if (soundId == 0)
		return -1;

	Common::StackLock lock(_mutex);
	return tracksStartSound(soundId, priority, bufferId);
}

int Imuse::waveSwitchStream(int oldSoundId, int newSoundId, int fadeLengthMs, int fadeSyncFlag2, int fadeSyncFlag1) {
	Common::StackLock lock(_mutex);
	return dispatchSwitchStream(oldSoundId, newSoundId, fadeLengthMs, fadeSyncFlag2, fadeSyncFlag1);
}

int Imuse::waveProcessStreams() {
	Common::StackLock lock(_mutex);
	return streamerProcessStreams();
}

void Imuse::waveQueryStream(int soundId, int32 &bufSize, int32 &criticalSize, int32 &freeSpace, int &paused) {
	Common::StackLock lock(_mutex);
	tracksQueryStream(soundId, bufSize, criticalSize, freeSpace, paused);
}

int Imuse::waveFeedStream(int soundId, uint8 *srcBuf, int32 sizeToFeed, int paused) {
	Common::StackLock lock(_mutex);
	return tracksFeedStream(soundId, srcBuf, sizeToFeed, paused);
}

} // End of namespace Grim
