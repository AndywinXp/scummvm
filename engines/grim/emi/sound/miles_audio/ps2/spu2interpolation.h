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

#ifndef GRIM_EMI_MILES_AUDIO_PS2_SPU2INTERPOLATION_H
#define GRIM_EMI_MILES_AUDIO_PS2_SPU2INTERPOLATION_H

#include "common/scummsys.h"

namespace Grim {

class EMISpu2Interpolation {
public:
	enum {
		kPhaseCount = 256,
		kTapCount = 4,
		kOutputRate = 48000,
		kPitchShift = 12,
		kMaxPitch = 0x3FFF,
		kFractionMask = 0x0FFF
	};

	static int32 interpolate(uint32 counter, const int32 *history);

private:
	static const int16 kTable[kPhaseCount][kTapCount];
};

} // End of namespace Grim

#endif
