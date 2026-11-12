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

#ifndef GRIM_EMI_CSCXFILE_H
#define GRIM_EMI_CSCXFILE_H

#include "common/scummsys.h"

#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class CWaveSound;

class CScxFile {
public:
	enum {
		kScxHeaderSize = 0x80,
		kChannelsOffset = 7,
		kInterleaveOffset = 8,
		kVagHeaderSize = 48,
		kVagSizeOffset = 12,
		kVagRateOffset = 16,
		kAdpcmBlockSize = 16,
		kAdpcmBlockSamples = 28,
		kAdpcmFilterCount = 5,
		kMaxChannels = 2
	};

	enum {
		kBlockEnd = 1,
		kBlockRepeat = 2,
		kBlockLoopStart = 4
	};

	CScxFile();

	void *operator new(size_t size);
	void operator delete(void *block);

	bool read(LFile *file, CWaveSound *sound);
	bool parse(const byte *data, uint size, CWaveSound *sound);
	uint getChannelOffset(int channel, uint position) const;
	uint decodeChannel(const byte *data, uint size, int channel, int16 *out, uint blocks, uint *loopBlock, bool *loops) const;

	static void decodeAdpcmBlock(const byte *block, int16 *out, int *history1, int *history2);

	int _numChannels;
	uint _interleave;
	uint _headerOffset;
	uint _rate;
	uint _vagSize[kMaxChannels];
};

} // End of namespace Grim

#endif
