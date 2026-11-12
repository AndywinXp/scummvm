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

#ifndef GRIM_EMI_MILES_AUDIO_REVERB_H
#define GRIM_EMI_MILES_AUDIO_REVERB_H

#include "common/scummsys.h"

namespace Grim {

class EMIMilesDriver;
class LMemoryServices;

class EMIMilesFilter {
public:
	enum {
		kWorkSamples = 360,
		kLineCount = 6
	};

	static EMIMilesFilter *create(LMemoryServices *memoryServices, int driverRate);
	~EMIMilesFilter();

	void *operator new(size_t size);
	void operator delete(void *block);

	void setTime(float milliseconds);
	void setMix(float mix);
	void setDamping(float hertz);
	void setEnvironment(float environment);
	bool isActive() const;

	bool process(const byte **src, uint32 *srcFract, const byte *srcEnd, int32 **dst, const int32 *dstEnd, int32 *prevL, int32 *prevR, int ratio, int scaleL, int scaleR, int reverbScale, EMIMilesDriver *mixer, uint flags);

private:
	EMIMilesFilter(LMemoryServices *memoryServices, int driverRate);

	void recomputeParameters();

	LMemoryServices *_memoryServices;
	float _rate;
	bool _active;
	float _timeMs;
	float _predelayMs;
	float _dampingHz;
	float _mix;
	float _environment;

	float *_lineBuffer[kLineCount];
	int _lineIndex[kLineCount];
	int _lineSize[kLineCount];
	float _lineGain[kLineCount];

	float *_predelayBuffer;
	int _predelayIndex;
	int _predelayLength;

	float _x1;
	float _x2;
	float _y1;
	float _y2;
	float _b0;
	float _b1;
	float _b2;
	float _a1;
	float _a2;

	int32 _work[kWorkSamples];
};

} // End of namespace Grim

#endif
