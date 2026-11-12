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

#ifndef GRIM_EMI_AUDIOSYSTEM_H
#define GRIM_EMI_AUDIOSYSTEM_H

#include "common/scummsys.h"

#include "engines/grim/emi/cnoderef.h"
#include "engines/grim/lua/lua.h"

namespace Grim {

class CAudioChannel;
class CAudioDevice;
class CNode;
class CSoundCue;

class AudioSystem {
public:
	typedef void LipCallback(const char *soundName, CSoundCue *cue);

	AudioSystem();
	~AudioSystem();

	void *operator new(size_t size);
	void operator delete(void *block);

	static bool createAudioDevice();
	static void destroyAudioDevice();
	static void updateAudioDevice();
	static CAudioChannel *allocateAudioChannel(CSoundCue *cue);
	static void setAudioListener(CNode *node);
	static bool isCueInRange(CSoundCue *cue);
	static float setMasterGain(float gain);
	static float getMasterGain();
	static float setCategoryGain(int category, float gain);
	static float getCategoryGain(int category);
	static void setGlobalReverbEnabled(int enabled);
	static bool setGlobalReverb(int preset, const float *first, const float *second, const float *third, const float *fourth, const float *fifth);
	static bool getGlobalReverb(int *preset, float *first, float *second, float *third, float *fourth, float *fifth);
	static bool setSonyReverb(int type, int depth, int delay, int feedback);
	static bool setAudioDevicePreference(const uint16 *preference);
	static void stopAllChannels();
	static bool saveAudioSettings(tSaveRestoreAccess *access);
	static bool restoreAudioSettings(tSaveRestoreAccess *access);

	static int16 refCount;
	static LipCallback *lipCallback;
	static int debugLevel;
	static bool audioAvailable;
	static bool digitalHandleReleased;
	static CAudioDevice *device;
	static int reverbPreset;
	static float reverbValues[5];
	static byte reverbEnabled;
	static int sonyReverbType;
	static int sonyReverbDepth;
	static int sonyReverbDelay;
	static int sonyReverbFeedback;
	static CNodeRef listener;
	static float masterGain;
	static float categoryGain[3];
	static uint16 devicePreference;
};

} // End of namespace Grim

#endif
