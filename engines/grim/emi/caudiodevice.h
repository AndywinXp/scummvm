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

#ifndef GRIM_EMI_CAUDIODEVICE_H
#define GRIM_EMI_CAUDIODEVICE_H

#include "common/scummsys.h"

namespace Grim {

class CAudioChannel;
class CSoundCue;

class CAudioDevice {
public:
	CAudioDevice();
	virtual ~CAudioDevice();

	virtual int getType() = 0;
	virtual void releaseDigitalHandle() = 0;
	virtual void reacquireDigitalHandle() = 0;
	virtual void pauseAll() = 0;
	virtual void resumeAll() = 0;
	virtual void stopAll() = 0;
	virtual void update() = 0;
	virtual void updateChannelsSpatialization() = 0;
	virtual CAudioChannel *allocateChannel(CSoundCue *cue) = 0;
	virtual int getChannelCount() = 0;
	virtual CAudioChannel *getChannel(uint index) = 0;
	virtual bool setMixAhead(const uint16 *mixAhead) = 0;
	virtual float setMasterVolume(float volume) = 0;
	virtual float getMasterVolume() = 0;
	virtual float setCategoryVolume(int category) = 0;
	virtual bool setReverb(const int *preset, const float *time, const float *mix, const float *unusedFirst, const float *damping, const float *unusedSecond) = 0;
	virtual int getCPUPercent();
	virtual int getLatency();
	virtual int getActiveSampleCount();
};

} // End of namespace Grim

#endif
