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

#ifndef GRIM_EMI_CWAVESOUND_H
#define GRIM_EMI_CWAVESOUND_H

#include "common/endian.h"
#include "common/scummsys.h"

#include "engines/grim/emi/sound/lsound.h"

namespace Grim {

class CWaveSound : public LSound {
public:
	enum {
		kFormatNone = MKTAG('N', 'O', 'N', 'E'),
		kFormatPcm = MKTAG('P', 'C', 'M', ' '),
		kFormatSmsh = MKTAG('S', 'M', 'S', 'H'),
		kFormatMp3 = MKTAG('M', 'P', '3', ' '),
		kFormatVima = MKTAG('V', 'I', 'M', 'A'),
		kTypeWave = MKTAG('W', 'A', 'V', 'E')
	};

	CWaveSound();
	~CWaveSound() override;

	void reset() override;
	int getType() override;

	bool is16Bit();
	int getSampleFormat();
	bool reallocData(uint size);
	bool readSamples(char *dest, uint *position, uint count, bool loop);
	uint getSegmentLength(uint position, uint *nextPosition, bool loop);
	void copyData(char *dest, uint sourceOffset, uint count);
	void decodeBlock(uint block);

	static void *allocAudioMemory(uint size);
	static void freeAudioMemory(void *block);
	static uint getRemainingSize(LFile *file);

	uint32 _formatTag;
	double _sampleRate;
	uint16 _bitsPerSample;
	uint16 _numChannels;
	bool _littleEndian;
	uint _loopStart;
	uint _loopEnd;
	uint _allocatedSize;
	uint _dataSize;
	byte *_samples;
	char *_decodedBlock;
	int _decodedBlockIndex;
	bool _spuLoops;
	uint _spuLoopStart;

protected:
	bool resLoadItem(const char *itemName) override;
};

} // End of namespace Grim

#endif
