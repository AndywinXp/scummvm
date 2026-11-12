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

#ifndef GRIM_EMI_MILES_AUDIO_PS2_SPU2REVERB_H
#define GRIM_EMI_MILES_AUDIO_PS2_SPU2REVERB_H

#include "common/scummsys.h"

namespace Grim {

class EMISpu2Reverb {
public:
	enum {
		kModeCount = 10,
		kModeEcho = 7,
		kModeDelay = 8,
		kModeClear = 0x100,
		kRegisterCount = 32,
		kRamSize = 0x100000,
		kResampleTaps = 39,
		kResampleBufferSize = 64
	};

	enum {
		kRegApf1Size = 0,
		kRegApf2Size = 1,
		kRegIirVolume = 2,
		kRegComb1Volume = 3,
		kRegComb2Volume = 4,
		kRegComb3Volume = 5,
		kRegComb4Volume = 6,
		kRegWallVolume = 7,
		kRegApf1Volume = 8,
		kRegApf2Volume = 9,
		kRegSameLeftDst = 10,
		kRegSameRightDst = 11,
		kRegComb1LeftSrc = 12,
		kRegComb1RightSrc = 13,
		kRegComb2LeftSrc = 14,
		kRegComb2RightSrc = 15,
		kRegSameLeftSrc = 16,
		kRegSameRightSrc = 17,
		kRegDiffLeftDst = 18,
		kRegDiffRightDst = 19,
		kRegComb3LeftSrc = 20,
		kRegComb3RightSrc = 21,
		kRegComb4LeftSrc = 22,
		kRegComb4RightSrc = 23,
		kRegDiffLeftSrc = 24,
		kRegDiffRightSrc = 25,
		kRegApf1LeftDst = 26,
		kRegApf1RightDst = 27,
		kRegApf2LeftDst = 28,
		kRegApf2RightDst = 29,
		kRegInputLeft = 30,
		kRegInputRight = 31
	};

	EMISpu2Reverb();
	~EMISpu2Reverb();

	void *operator new(size_t size);
	void operator delete(void *block);

	int setEffectAttr(int mode, int depthLeft, int depthRight, int delay, int feedback);
	void setEffectEnable(bool enabled);
	void setEffectVolume(int left, int right);
	void setEffectEndAddress(uint32 address);

	void process(int32 inLeft, int32 inRight, int32 *outLeft, int32 *outRight);

private:
	uint32 getEffectEndAddress() const;
	void clearEffectWorkArea(int mode);
	void writeRegisters(const uint16 *registers);

	uint32 getIndex(uint32 offset) const;
	int32 downsample(int channel) const;
	void upsample(int32 *left, int32 *right) const;
	static int32 multiply(int32 a, int32 b);
	static int32 clampMix(int32 value);

	int16 *_ram;

	int _mode;
	bool _effectEnabled;
	int32 _effectVolumeLeft;
	int32 _effectVolumeRight;
	uint32 _effectStart;
	uint32 _effectEnd;

	uint32 _addresses[kRegisterCount];
	int16 _volumes[kRegisterCount];

	uint32 _cycles;
	uint32 _bufferPosition;
	int16 _downBuffer[2][kResampleBufferSize * 2];
	int16 _upBuffer[2][kResampleBufferSize * 2];
};

} // End of namespace Grim

#endif
