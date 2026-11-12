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

#ifndef GRIM_EMI_CWAVECHANNEL_H
#define GRIM_EMI_CWAVECHANNEL_H

#include "engines/grim/emi/caudiochannel.h"

namespace Grim {

class CSoundCue;
class EMIMilesDriver;
struct EMIMilesSample;

class CWaveChannel : public CAudioChannel {
public:
	CWaveChannel(EMIMilesDriver *driver);
	~CWaveChannel() override;

	void *operator new(size_t size);
	void operator delete(void *block);

	int update() override;
	bool assignToSample(CSoundCue *newCue) override;
	void release() override;
	void start() override;
	void stop() override;
	void pause() override;
	void resume() override;
	int getStatus() override;
	int getPositionMs(int *totalMs) override;
	void setPositionMs(int milliseconds) override;
	void setVolume(float volume) override;
	void setPlaybackRate(int rate) override;
	void setPan(float pan) override;
	bool setReverb(int preset, float time, float mix, float unusedFirst, float damping, float unusedSecond) override;
	bool hasFiniteLoopCount() override;
	void setLoopCount(int count) override;
	void setCutoff(float nearDistance, float farDistance) override;
	void applyCueSettings(CSoundCue *newCue) override;

	void updateSpatialization();
	void setSpuSampleVolume(int volume);
	void setSpuSamplePan(int pan);
	void setSpuStreamVolume(int volume);
	void endSpuSample();

	static bool getAttenuatedVolumes(float volume, float balance, int *left, int *right);
	static void setSpuStereo(bool stereo);

	static bool spuStereo;

	bool started;
	EMIMilesSample *hSample;
	CSoundCue *cue;
	float spuVolume;
	float spuBalance;

private:
	EMIMilesDriver *_driver;
};

} // End of namespace Grim

#endif
