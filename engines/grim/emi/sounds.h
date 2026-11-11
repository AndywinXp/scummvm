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

#include "engines/grim/emi/csoundcue.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class AudioSystem;
class CNode;
class LipData;
class LResource;

class Sounds {
public:
	static LResource *createWaveSoundResource();
	static float soundVolumeToGain(int volume);
	static int soundGainToVolume(float gain);
	static uint16 registerSound(CSoundCue *cue);
	static void unregisterSound(uint16 soundHandle);
	static CSoundCue *findSoundCue(uint soundHandle);
	static bool isCueRegistered(CSoundCue *cue);
	static bool soundsUpdate(const bool *force);
	static bool soundsUpdateThrottled(const bool *force);
	static void startSounds(bool startMusicNow);
	static void shutdownSounds();
	static void releaseAllSounds();
	static void releaseUnlockedSounds();
	static void soundCueLipCallback(const char *soundName, CSoundCue *cue);
	static void saveSounds(tSaveRestoreAccess *access);
	static void restoreSounds(tSaveRestoreAccess *access);
	static void setSoundSystemEnabled(bool enabled);
	static void setMusicSystemEnabled(bool enabled);
	static uint loadSound(const char *soundName, int category);
	static uint loadSound(const LString &soundName, int category);
	static void playSound(const LString &soundName, int volume);
	static void playSoundFrom(const LString &soundName, float x, float y, float z, int16 volume);
	static void playLoadedSoundFrom(uint soundHandle, float x, float y, float z, bool loop, int16 volume);
	static void startSound(uint soundHandle, int startMode, bool loop, int volume);
	static void stopSound(uint soundHandle);
	static void stopSoundPlayback(uint soundHandle);
	static void releaseSoundWhenDone(uint soundHandle);
	static void pauseSound(uint soundHandle);
	static void resumeSound(uint soundHandle);
	static bool isSoundPlaying(uint soundHandle);
	static bool isSoundPlayingNoFiniteLoop(uint soundHandle);
	static bool isSoundPaused(uint soundHandle);
	static void setSoundPosition(uint soundHandle, float x, float y, float z);
	static bool setSoundVolume(uint soundHandle, int volume);
	static bool getSoundVolume(uint soundHandle, int *volume);
	static int getSoundPosition(uint soundHandle);
	static int getSoundLength(uint soundHandle);
	static LipData *getSoundLipData(uint soundHandle);
	static bool setCategoryVolume(int category, int volume);
	static int getCategoryVolume(int category);
	static void setCategoryEnabled(int category, bool enabled);
	static void setVoiceFxEnabled(int enabled);
	static void setReverb(int preset, float *param1, float *param2, float *param3, float *param4, float *param5);
	static void restoreMasterGain();
	static void lowerMasterGainForMovie();
	static void setAudioMixAhead(const int16 *mixAhead, bool store);
	static void setSoundListener(CNode *listener);
	static void suspendAudio();
	static void resumeAudio();
	static void pauseAll();
	static void resumeAll();
	static void stopAllSounds();

	static void updateMusic();
	static void startMusic(int newState);
	static int stopMusic();
	static void setMusicVolume(float volume);
	static void setMusicState(int newState, int startMs, const bool &immediate);
	static void saveMusicState(tSaveRestoreAccess *access);
	static void restoreMusicState(tSaveRestoreAccess *access);
	static void pauseMusicStreams();
	static void resumeMusicStreams();
	static void stopAllMusic();
	static void selectMusicSet(int16 set);
	static bool consumeMusicFlags(const uint &musicId, bool &looped, bool &ended);
	static void pushMusicState(int musicId, const bool &immediate);
	static void popMusicState(const bool &immediate);
	static void flushMusicStateStack();
	static int getMusicPositionMs(const uint &musicId);

	static AudioSystem *audioSystem;
	static CSoundCue::SoundCueMap soundMap;
	static uint16 soundCounter;
	static int16 audioMixAhead;
	static bool soundsEnabled;
	static bool musicEnabled;
	static bool musicCategoryEnabled;
	static bool voiceEnabled;
	static bool sfxEnabled;
	static byte voiceFxEnabled;
	static int savedMusicState;
	static bool musicActive;
	static int musicState;
	static bool musicSet;
	static float musicVolume;
	static int pushedMusicState;
	static int pushedMusicPosition;
	static uint lastUpdateMs;
	static uint lastThrottledUpdateMs;
	static float dbStep;
	static float invDbStep;
	static float unityGain;
	static float defaultMasterGain;
	static float movieMasterGain;
};

} // End of namespace Grim

#endif
