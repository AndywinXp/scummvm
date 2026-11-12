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

#ifndef GRIM_EMI_CWAVEAUDIODEVICE_H
#define GRIM_EMI_CWAVEAUDIODEVICE_H

#include "common/array.h"
#include "common/scummsys.h"

#include "engines/grim/emi/caudiodevice.h"

namespace Grim {

class CWaveChannel;
class EMIMilesDriver;

class CWaveAudioDevice : public CAudioDevice {
public:
	CWaveAudioDevice(int channelCount);
	~CWaveAudioDevice() override;

	void *operator new(size_t size);
	void operator delete(void *block);

	int getType() override;
	void releaseDigitalHandle() override;
	void reacquireDigitalHandle() override;
	void pauseAll() override;
	void resumeAll() override;
	void stopAll() override;
	void update() override;
	void updateChannelsSpatialization() override;
	CAudioChannel *allocateChannel(CSoundCue *cue) override;
	int getChannelCount() override;
	CAudioChannel *getChannel(uint index) override;
	bool setMixAhead(const uint16 *mixAhead) override;
	float setMasterVolume(float volume) override;
	float getMasterVolume() override;
	float setCategoryVolume(int category) override;
	bool setReverb(const int *preset, const float *time, const float *mix, const float *unusedFirst, const float *damping, const float *unusedSecond) override;
	int getCPUPercent() override;
	int getLatency() override;
	int getActiveSampleCount() override;

	static int getMilesDigitalDriver();
	static EMIMilesDriver *getMilesDriver();

	bool lost;
	Common::Array<CWaveChannel *> channels;
	int numChannels;
	int lastCheckMs;

private:
	EMIMilesDriver *_driver;
};

} // End of namespace Grim

#endif
