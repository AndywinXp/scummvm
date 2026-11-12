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

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/cscxfile.h"
#include "engines/grim/emi/cwavesound.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static const uint32 kTagScrx = MKTAG('S', 'C', 'R', 'X');
static const uint32 kTagVagp = MKTAG('V', 'A', 'G', 'p');
static const int kAdpcmFilters[CScxFile::kAdpcmFilterCount][2] = {
	{ 0, 0 }, { 60, 0 }, { 115, -52 }, { 98, -55 }, { 122, -60 }
};

CScxFile::CScxFile() {
	numChannels = 0;
	interleave = 0;
	headerOffset = 0;
	rate = 0;

	for (int i = 0; i < kMaxChannels; i++)
		vagSize[i] = 0;
}

void *CScxFile::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CScxFile::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

bool CScxFile::read(LFile *file, CWaveSound *sound) {
	uint size = CWaveSound::getRemainingSize(file);

	if (!size) {
		warning("CScxFile::read(): empty file");

		return false;
	}

	byte *data = new byte[size];

	if (file->read(data, size) != size) {
		warning("CScxFile::read(): short read");
		delete[] data;

		return false;
	}

	bool result = parse(data, size, sound);

	delete[] data;

	return result;
}

bool CScxFile::parse(const byte *data, uint size, CWaveSound *sound) {
	if (size < 4) {
		warning("CScxFile::parse(): file too short");

		return false;
	}

	uint32 tag = READ_BE_UINT32(data);

	if (tag == kTagScrx) {
		if (size < kScxHeaderSize + kVagHeaderSize) {
			warning("CScxFile::parse(): header too short");

			return false;
		}

		numChannels = data[kChannelsOffset];
		interleave = READ_LE_UINT16(data + kInterleaveOffset);
		headerOffset = kScxHeaderSize;

		if (READ_BE_UINT32(data + headerOffset) != kTagVagp) {
			warning("CScxFile::parse(): no VAGp header");

			return false;
		}
	} else if (tag == kTagVagp) {
		if (size < kVagHeaderSize) {
			warning("CScxFile::parse(): header too short");

			return false;
		}

		numChannels = 1;
		interleave = 0;
		headerOffset = 0;
	} else {
		warning("CScxFile::parse(): unsupported format");

		return false;
	}

	if (numChannels < 1 || numChannels > kMaxChannels) {
		warning("CScxFile::parse(): %d channels not supported", numChannels);

		return false;
	}

	if (numChannels > 1 && (interleave <= kVagHeaderSize || interleave % kAdpcmBlockSize != 0)) {
		warning("CScxFile::parse(): invalid interleave %u", interleave);

		return false;
	}

	rate = READ_BE_UINT32(data + headerOffset + kVagRateOffset);

	for (int channel = 0; channel < numChannels; channel++) {
		uint channelHeader = headerOffset + channel * interleave;

		if (channelHeader + kVagHeaderSize > size) {
			warning("CScxFile::parse(): VAGp header past end of file");

			return false;
		}

		vagSize[channel] = READ_BE_UINT32(data + channelHeader + kVagSizeOffset);

		if (vagSize[channel] > size) {
			warning("CScxFile::parse(): VAGp size %u bigger than file", vagSize[channel]);

			return false;
		}
	}

	int16 *channelSamples[kMaxChannels] = { nullptr, nullptr };
	uint channelFrames[kMaxChannels] = { 0, 0 };
	uint frames = 0;
	uint loopBlock = 0;
	bool loops = false;

	for (int channel = 0; channel < numChannels; channel++) {
		uint blocks = vagSize[channel] / kAdpcmBlockSize;
		uint channelLoopBlock = 0;
		bool channelLoops = false;

		channelSamples[channel] = new int16[blocks * kAdpcmBlockSamples + 1];
		channelFrames[channel] = decodeChannel(data, size, channel, channelSamples[channel], blocks, &channelLoopBlock, &channelLoops) * kAdpcmBlockSamples;

		// The loop point comes from the left voice
		if (channel == 0) {
			loopBlock = channelLoopBlock;
			loops = channelLoops;
		}

		if (channelFrames[channel] > frames)
			frames = channelFrames[channel];
	}

	// The voice is released at the end, so add one silent block for looping cues
	uint totalFrames = loops ? frames : frames + kAdpcmBlockSamples;
	uint frameBytes = numChannels * sizeof(int16);
	uint pcmSize = totalFrames * frameBytes;
	bool result = sound->reallocData(pcmSize);

	if (result) {
		memset(sound->samples, 0, pcmSize);

		for (int channel = 0; channel < numChannels; channel++) {
			for (uint frame = 0; frame < channelFrames[channel]; frame++)
				WRITE_LE_INT16(sound->samples + (frame * numChannels + channel) * sizeof(int16), channelSamples[channel][frame]);
		}

		sound->formatTag = CWaveSound::kFormatPcm;
		sound->sampleRate = (double)rate;
		sound->bitsPerSample = 16;
		sound->numChannels = (uint16)numChannels;
		sound->littleEndian = true;
		sound->loopStart = 0;
		sound->loopEnd = 0xFFFFFFFF;
		sound->dataSize = pcmSize;
		sound->spuLoops = loops;
		sound->spuLoopStart = (loops ? loopBlock * kAdpcmBlockSamples : frames) * frameBytes;
	} else {
		warning("CScxFile::parse(): out of memory");
	}

	for (int channel = 0; channel < kMaxChannels; channel++)
		delete[] channelSamples[channel];

	return result;
}

uint CScxFile::getChannelOffset(int channel, uint position) const {
	uint channelPosition = position + kVagHeaderSize;
	uint channelHeader = headerOffset + channel * interleave;

	if (!interleave)
		return channelHeader + channelPosition;

	return channelHeader + (channelPosition / interleave) * interleave * numChannels + channelPosition % interleave;
}

uint CScxFile::decodeChannel(const byte *data, uint size, int channel, int16 *out, uint blocks, uint *loopBlock, bool *loops) const {
	int history1 = 0;
	int history2 = 0;

	*loopBlock = 0;
	*loops = false;

	for (uint block = 0; block < blocks; block++) {
		uint offset = getChannelOffset(channel, block * kAdpcmBlockSize);

		if (offset > size || size - offset < kAdpcmBlockSize) {
			warning("CScxFile::decodeChannel(): ADPCM data past end of file");

			return block;
		}

		const byte *source = data + offset;
		byte flags = source[1];

		decodeAdpcmBlock(source, out + block * kAdpcmBlockSamples, &history1, &history2);

		if ((flags & kBlockLoopStart) != 0)
			*loopBlock = block;

		if ((flags & kBlockEnd) != 0) {
			*loops = (flags & kBlockRepeat) != 0;

			return block + 1;
		}
	}

	return blocks;
}

void CScxFile::decodeAdpcmBlock(const byte *block, int16 *out, int *history1, int *history2) {
	int shift = block[0] & 0x0F;
	int filter = block[0] >> 4;

	if (filter >= kAdpcmFilterCount)
		filter = 0;

	int coefficient1 = kAdpcmFilters[filter][0];
	int coefficient2 = kAdpcmFilters[filter][1];

	for (int i = 0; i < kAdpcmBlockSamples; i++) {
		int nibble = (block[2 + i / 2] >> ((i & 1) * 4)) & 0x0F;

		if (nibble >= 8)
			nibble -= 16;

		int sample = (nibble * 4096) >> shift;

		sample += (*history1 * coefficient1 + *history2 * coefficient2 + 32) >> 6;
		sample = CLIP(sample, -32768, 32767);

		*history2 = *history1;
		*history1 = sample;
		out[i] = (int16)sample;
	}
}

} // End of namespace Grim
