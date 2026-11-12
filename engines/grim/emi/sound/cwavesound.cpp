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
#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/sound/caifffile.h"
#include "engines/grim/emi/sound/cmp3file.h"
#include "engines/grim/emi/sound/cscxfile.h"
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/sound/cwavfile.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"

namespace Grim {

static const uint kDecodedBlockSize = 0x1000;
static const uint kDecodedBlockShift = 12;
static const uint kSmshSamplePairs = 1024;
static const byte kSmshEscape = 0x80;

CWaveSound::CWaveSound() {
	littleEndian = true;
	sampleRate = 0.0;
	loopEnd = 0xFFFFFFFF;
	decodedBlockIndex = -1;
	formatTag = kFormatNone;
	bitsPerSample = 0;
	numChannels = 0;
	loopStart = 0;
	samples = nullptr;
	allocatedSize = 0;
	dataSize = 0;
	decodedBlock = nullptr;
	spuLoops = false;
	spuLoopStart = 0;
}

CWaveSound::~CWaveSound() {
	if (decodedBlock) {
		delete[] decodedBlock;
		decodedBlock = nullptr;
	}

	if (samples) {
		freeAudioMemory(samples);
		samples = nullptr;
	}
}

void CWaveSound::reset() {
	// The disasm drops the decoded block without freeing it
	decodedBlock = nullptr;
	decodedBlockIndex = -1;
}

int CWaveSound::getType() {
	return kTypeWave;
}

bool CWaveSound::is16Bit() {
	return bitsPerSample == 16;
}

int CWaveSound::getSampleFormat() {
	if (numChannels == 1) {
		if (bitsPerSample == 8)
			return 1;

		if (bitsPerSample == 16)
			return 3;
	} else if (numChannels == 2) {
		if (bitsPerSample == 8)
			return 2;

		if (bitsPerSample == 16)
			return 4;
	}

	warning("CWaveSound::getSampleFormat(): illegal format");

	return 0;
}

bool CWaveSound::reallocData(uint size) {
	byte *oldSamples = samples;

	samples = (byte *)allocAudioMemory(size);

	if (!samples) {
		samples = oldSamples;

		return false;
	}

	if (oldSamples) {
		uint copySize = allocatedSize;

		if (copySize >= size)
			copySize = size;

		memcpy(samples, oldSamples, copySize);
		freeAudioMemory(oldSamples);
	}

	allocatedSize = size;

	return true;
}

bool CWaveSound::readSamples(char *dest, uint *position, uint count, bool loop) {
	if (!dataSize)
		return false;

	if (!allocatedSize)
		return false;

	if (*position >= dataSize)
		return false;

	uint nextPosition = 0;
	uint segmentLength = getSegmentLength(*position, &nextPosition, loop);
	uint remaining = count;
	uint chunk = count;

	if (count >= segmentLength)
		chunk = segmentLength;

	if (count) {
		while (true) {
			copyData(dest, *position, chunk);

			remaining -= chunk;

			if (!remaining) {
				*position += chunk;

				return true;
			}

			dest += chunk;

			if (nextPosition >= dataSize)
				break;

			*position = nextPosition;
			segmentLength = getSegmentLength(nextPosition, &nextPosition, loop);

			if (remaining >= segmentLength)
				chunk = segmentLength;
			else
				chunk = remaining;
		}

		memset(dest, 0, remaining);
		*position = dataSize;
	}

	return true;
}

uint CWaveSound::getSegmentLength(uint position, uint *nextPosition, bool loop) {
	uint end;

	if (loop) {
		*nextPosition = loopStart;
		end = loopEnd;

		if (end >= dataSize)
			return dataSize - position;
	} else {
		*nextPosition = dataSize;
		end = dataSize;
	}

	return end - position;
}

void CWaveSound::copyData(char *dest, uint sourceOffset, uint count) {
	if (formatTag == kFormatPcm) {
		if (!samples || sourceOffset > allocatedSize) {
			warning("CWaveSound::copyData(): offset outside of the samples");
			memset(dest, 0, count);

			return;
		}

		uint copySize = count;

		if (copySize > allocatedSize - sourceOffset)
			copySize = allocatedSize - sourceOffset;

		memcpy(dest, samples + sourceOffset, copySize);

		if (copySize < count) {
			warning("CWaveSound::copyData(): count outside of the samples");
			memset(dest + copySize, 0, count - copySize);
		}
	} else if (formatTag == kFormatSmsh) {
		int remaining = count;
		uint blockOffset = sourceOffset & (kDecodedBlockSize - 1);
		int lastBlock = (sourceOffset + count) >> kDecodedBlockShift;
		int block = sourceOffset >> kDecodedBlockShift;
		uint left = count;
		uint written = 0;

		if (block <= lastBlock) {
			while (true) {
				if (block != decodedBlockIndex)
					decodeBlock(block);

				if (!decodedBlock) {
					warning("CWaveSound::copyData(): no decoded block");
					memset(dest + written, 0, left);

					break;
				}

				uint chunk = kDecodedBlockSize - blockOffset;

				if ((int)chunk >= remaining)
					chunk = remaining;

				memcpy(dest + written, decodedBlock + blockOffset, chunk);

				left -= chunk;
				written += chunk;
				blockOffset = 0;
				block++;

				if (block > lastBlock)
					break;

				remaining = left;
			}
		}
	} else {
		memset(dest, 0, count);

		if (_engine->getConsoleServices())
			_engine->getConsoleServices()->logError("CWaveSound::CopyData   Unknown wave format: %c%c%c%c (%08x)", (int)((formatTag >> 24) & 0xFF), (int)((formatTag >> 16) & 0xFF), (int)((formatTag >> 8) & 0xFF), (int)(formatTag & 0xFF), formatTag);
		else
			warning("CWaveSound::copyData(): unknown wave format %08x", formatTag);
	}
}

void CWaveSound::decodeBlock(uint block) {
	if ((int)block == decodedBlockIndex || !samples)
		return;

	if (!decodedBlock)
		decodedBlock = new char[kDecodedBlockSize];

	if (allocatedSize < 4) {
		warning("CWaveSound::decodeBlock(): the samples have no block table");

		return;
	}

	uint numBlocks = READ_LE_UINT32(samples) >> kDecodedBlockShift;

	// The original divides by zero here
	if (numBlocks == 0) {
		warning("CWaveSound::decodeBlock(): the sound has no blocks");

		return;
	}

	uint index = block % numBlocks;

	decodedBlockIndex = index;

	uint tableOffset = 4 + index * 4;

	if (tableOffset + 4 > allocatedSize) {
		warning("CWaveSound::decodeBlock(): block table outside of the samples");
		memset(decodedBlock, 0, kDecodedBlockSize);

		return;
	}

	uint blockOffset = READ_LE_UINT32(samples + tableOffset);

	if (blockOffset > allocatedSize || allocatedSize - blockOffset < 3) {
		warning("CWaveSound::decodeBlock(): block outside of the samples");
		memset(decodedBlock, 0, kDecodedBlockSize);

		return;
	}

	const byte *source = samples + blockOffset + 2;
	const byte *end = samples + allocatedSize;
	int shiftFirst = *source >> 4;
	int shiftSecond = *source & 0x0F;
	source++;

	for (uint pair = 0; pair < kSmshSamplePairs; pair++) {
		for (uint half = 0; half < 2; half++) {
			uint outputOffset = (pair * 2 + half) * 2;

			if (source >= end) {
				warning("CWaveSound::decodeBlock(): block data outside of the samples");
				memset(decodedBlock + outputOffset, 0, kDecodedBlockSize - outputOffset);

				return;
			}

			int value = *source;
			source++;

			uint16 decoded;

			if (value == kSmshEscape) {
				if (source + 2 > end) {
					warning("CWaveSound::decodeBlock(): block data outside of the samples");
					memset(decodedBlock + outputOffset, 0, kDecodedBlockSize - outputOffset);

					return;
				}

				decoded = (source[0] << 8) + source[1];
				source += 2;
			} else {
				int shift = (half == 0) ? shiftFirst : shiftSecond;

				decoded = (uint16)((int)(int8)value * (1 << shift));
			}

			WRITE_LE_UINT16(decodedBlock + outputOffset, decoded);
		}
	}
}

void *CWaveSound::allocAudioMemory(uint size) {
	byte *block = new byte[size];

	memset(block, 0, size);

	return block;
}

void CWaveSound::freeAudioMemory(void *block) {
	if (block)
		delete[] (byte *)block;
}

uint CWaveSound::getRemainingSize(LFile *file) {
	int position = file->tell();

	file->seek(0, SEEK_END);

	int end = file->tell();

	file->seek(position, SEEK_SET);

	if (end < position)
		return 0;

	return end - position;
}

bool CWaveSound::resLoadItem(const char *itemName) {
	bool result = false;
	const char *fileName = itemName;

	if (*fileName == '@')
		fileName++;

	const char *dot = strchr(fileName, '.');

	if (!dot)
		return result;

	const char *extension = dot + 1;

	if (!scumm_strnicmp(extension, "AIF", 3)) {
		LFile *file = _engine->getFileServices()->open(fileName, "rb");

		if (file) {
			CAiffFile aiffFile;

			result = aiffFile.read(file, this);
			_engine->getFileServices()->close(file, true);
		}

		return result;
	}

	if (!scumm_strnicmp(extension, "WAV", 3) || !scumm_strnicmp(extension, "wVC", 3)) {
		CWavFile wavFile(_engine);
		LFile *file = _engine->getFileServices()->open(fileName, "rb");

		if (!file)
			return result;

		if (!scumm_strnicmp(extension, "wVC", 3)) {
			result = wavFile.readMcmp(file, this);
		} else {
			result = wavFile.read(file, this);

			if (!result) {
				CAiffFile aiffFile;

				file->seek(0, SEEK_SET);
				result = aiffFile.read(file, this);
			}
		}

		_engine->getFileServices()->close(file, true);

		return result;
	}

	if (!scumm_strnicmp(extension, "MP3", 3)) {
		CMp3File mp3File;
		LFile *file = _engine->getFileServices()->open(fileName, "rb");

		if (file) {
			result = mp3File.read(file, this);
			_engine->getFileServices()->close(file, true);
		}

		return result;
	}

	if (!scumm_strnicmp(extension, "SCX", 3)) {
		CScxFile scxFile;
		LFile *file = _engine->getFileServices()->open(fileName, "rb");

		if (file) {
			result = scxFile.read(file, this);
			_engine->getFileServices()->close(file, true);

			if (!result)
				warning("CWaveSound::resLoadItem(): can't parse sound data of %s", itemName);
		}

		return result;
	}

	warning("CWaveSound::resLoadItem(): unknown sound extension in %s", itemName);

	return result;
}

} // End of namespace Grim
