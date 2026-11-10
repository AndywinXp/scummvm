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

#ifndef GRIM_EMI_SOUNDS_H
#define GRIM_EMI_SOUNDS_H

#include "common/scummsys.h"

namespace Grim {

class CNode;
class LipData;
class LResource;

class Sounds {
public:
	static LResource *createWaveSoundResource();
	static uint loadSound(const char *soundName, int mode);
	static void startSound(uint soundHandle, int startMode, bool loop, int volume);
	static void stopSound(uint soundHandle);
	static void stopSoundPlayback(uint soundHandle);
	static void releaseSoundWhenDone(uint soundHandle);
	static bool isSoundPlaying(uint soundHandle);
	static bool isSoundPlayingNoFiniteLoop(uint soundHandle);
	static int getSoundPosition(uint soundHandle);
	static int getSoundLength(uint soundHandle);
	static LipData *getSoundLipData(uint soundHandle);
	static bool isSoundPaused(uint soundHandle);
	static void pauseSound(uint soundHandle);
	static void resumeSound(uint soundHandle);
	static void setAudioMixAhead(const int16 *mixAhead, bool store);
	static bool soundsUpdateThrottled(const bool *force);
	static void setSoundListener(CNode *listener);

	static int16 audioMixAhead;
};

} // End of namespace Grim

#endif
