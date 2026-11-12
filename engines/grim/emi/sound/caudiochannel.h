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

#ifndef GRIM_EMI_CAUDIOCHANNEL_H
#define GRIM_EMI_CAUDIOCHANNEL_H

#include "common/scummsys.h"

namespace Grim {

class CSoundCue;
class EmiEngine;

class CAudioChannel {
public:
	explicit CAudioChannel(EmiEngine *engine);
	virtual ~CAudioChannel();

	virtual int update() = 0;
	virtual bool assignToSample(CSoundCue *newCue) = 0;
	virtual void release() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void pause() = 0;
	virtual void resume() = 0;
	virtual int getStatus() = 0;
	virtual int getPositionMs(int *totalMs) = 0;
	virtual void setPositionMs(int milliseconds) = 0;
	virtual bool isPaused();
	virtual void setPaused(bool value);
	virtual void setVolume(float volume) = 0;
	virtual void setPlaybackRate(int rate) = 0;
	virtual void setPan(float pan) = 0;
	virtual bool setReverb(int preset, float time, float mix, float unusedFirst, float damping, float unusedSecond) = 0;
	virtual bool hasFiniteLoopCount() = 0;
	virtual void setLoopCount(int count) = 0;
	virtual void setCutoff(float nearDistance, float farDistance) = 0;
	virtual void applyCueSettings(CSoundCue *newCue) = 0;

	bool paused;

protected:
	EmiEngine *_engine;
};

} // End of namespace Grim

#endif
