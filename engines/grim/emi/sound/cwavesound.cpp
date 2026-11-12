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
	_littleEndian = true;
	_sampleRate = 0.0;
	_loopEnd = 0xFFFFFFFF;
	_decodedBlockIndex = -1;
	_formatTag = kFormatNone;
	_bitsPerSample = 0;
	_numChannels = 0;
	_loopStart = 0;
	_samples = nullptr;
	_allocatedSize = 0;
	_dataSize = 0;
	_decodedBlock = nullptr;
	_spuLoops = false;
	_spuLoopStart = 0;
}

CWaveSound::~CWaveSound() {
	if (_decodedBlock) {
		delete[] _decodedBlock;
		_decodedBlock = nullptr;
	}

	if (_samples) {
		freeAudioMemory(_samples);
		_samples = nullptr;
	}
}

void CWaveSound::reset() {
	// The disasm drops the decoded block without freeing it
	_decodedBlock = nullptr;
	_decodedBlockIndex = -1;
}

int CWaveSound::getType() {
	return kTypeWave;
}

bool CWaveSound::is16Bit() {
	return _bitsPerSample == 16;
}

int CWaveSound::getSampleFormat() {
	if (_numChannels == 1) {
		if (_bitsPerSample == 8)
			return 1;

		if (_bitsPerSample == 16)
			return 3;
	} else if (_numChannels == 2) {
		if (_bitsPerSample == 8)
			return 2;

		if (_bitsPerSample == 16)
			return 4;
	}

	warning("CWaveSound::getSampleFormat(): illegal format");

	return 0;
}

bool CWaveSound::reallocData(uint size) {
	byte *oldSamples = _samples;

	_samples = (byte *)allocAudioMemory(size);

	if (!_samples) {
		_samples = oldSamples;

		return false;
	}

	if (oldSamples) {
		uint copySize = _allocatedSize;

		if (copySize >= size)
			copySize = size;

		memcpy(_samples, oldSamples, copySize);
		freeAudioMemory(oldSamples);
	}

	_allocatedSize = size;

	return true;
}

bool CWaveSound::readSamples(char *dest, uint *position, uint count, bool loop) {
	if (!_dataSize)
		return false;

	if (!_allocatedSize)
		return false;

	if (*position >= _dataSize)
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

			if (nextPosition >= _dataSize)
				break;

			*position = nextPosition;
			segmentLength = getSegmentLength(nextPosition, &nextPosition, loop);

			if (remaining >= segmentLength)
				chunk = segmentLength;
			else
				chunk = remaining;
		}

		memset(dest, 0, remaining);
		*position = _dataSize;
	}

	return true;
}

uint CWaveSound::getSegmentLength(uint position, uint *nextPosition, bool loop) {
	uint end;

	if (loop) {
		*nextPosition = _loopStart;
		end = _loopEnd;

		if (end >= _dataSize)
			return _dataSize - position;
	} else {
		*nextPosition = _dataSize;
		end = _dataSize;
	}

	return end - position;
}

void CWaveSound::copyData(char *dest, uint sourceOffset, uint count) {
	if (_formatTag == kFormatPcm) {
		if (!_samples || sourceOffset > _allocatedSize) {
			warning("CWaveSound::copyData(): offset outside of the samples");
			memset(dest, 0, count);

			return;
		}

		uint copySize = count;

		if (copySize > _allocatedSize - sourceOffset)
			copySize = _allocatedSize - sourceOffset;

		memcpy(dest, _samples + sourceOffset, copySize);

		if (copySize < count) {
			warning("CWaveSound::copyData(): count outside of the samples");
			memset(dest + copySize, 0, count - copySize);
		}
	} else if (_formatTag == kFormatSmsh) {
		int remaining = count;
		uint blockOffset = sourceOffset & (kDecodedBlockSize - 1);
		int lastBlock = (sourceOffset + count) >> kDecodedBlockShift;
		int block = sourceOffset >> kDecodedBlockShift;
		uint left = count;
		uint written = 0;

		if (block <= lastBlock) {
			while (true) {
				if (block != _decodedBlockIndex)
					decodeBlock(block);

				if (!_decodedBlock) {
					warning("CWaveSound::copyData(): no decoded block");
					memset(dest + written, 0, left);

					break;
				}

				uint chunk = kDecodedBlockSize - blockOffset;

				if ((int)chunk >= remaining)
					chunk = remaining;

				memcpy(dest + written, _decodedBlock + blockOffset, chunk);

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
			_engine->getConsoleServices()->logError("CWaveSound::CopyData   Unknown wave format: %c%c%c%c (%08x)", (int)((_formatTag >> 24) & 0xFF), (int)((_formatTag >> 16) & 0xFF), (int)((_formatTag >> 8) & 0xFF), (int)(_formatTag & 0xFF), _formatTag);
		else
			warning("CWaveSound::copyData(): unknown wave format %08x", _formatTag);
	}
}

void CWaveSound::decodeBlock(uint block) {
	if ((int)block == _decodedBlockIndex || !_samples)
		return;

	if (!_decodedBlock)
		_decodedBlock = new char[kDecodedBlockSize];

	if (_allocatedSize < 4) {
		warning("CWaveSound::decodeBlock(): the samples have no block table");

		return;
	}

	uint numBlocks = READ_LE_UINT32(_samples) >> kDecodedBlockShift;

	// The original divides by zero here
	if (numBlocks == 0) {
		warning("CWaveSound::decodeBlock(): the sound has no blocks");

		return;
	}

	uint index = block % numBlocks;

	_decodedBlockIndex = index;

	uint tableOffset = 4 + index * 4;

	if (tableOffset + 4 > _allocatedSize) {
		warning("CWaveSound::decodeBlock(): block table outside of the samples");
		memset(_decodedBlock, 0, kDecodedBlockSize);

		return;
	}

	uint blockOffset = READ_LE_UINT32(_samples + tableOffset);

	if (blockOffset > _allocatedSize || _allocatedSize - blockOffset < 3) {
		warning("CWaveSound::decodeBlock(): block outside of the samples");
		memset(_decodedBlock, 0, kDecodedBlockSize);

		return;
	}

	const byte *source = _samples + blockOffset + 2;
	const byte *end = _samples + _allocatedSize;
	int shiftFirst = *source >> 4;
	int shiftSecond = *source & 0x0F;
	source++;

	for (uint pair = 0; pair < kSmshSamplePairs; pair++) {
		for (uint half = 0; half < 2; half++) {
			uint outputOffset = (pair * 2 + half) * 2;

			if (source >= end) {
				warning("CWaveSound::decodeBlock(): block data outside of the samples");
				memset(_decodedBlock + outputOffset, 0, kDecodedBlockSize - outputOffset);

				return;
			}

			int value = *source;
			source++;

			uint16 decoded;

			if (value == kSmshEscape) {
				if (source + 2 > end) {
					warning("CWaveSound::decodeBlock(): block data outside of the samples");
					memset(_decodedBlock + outputOffset, 0, kDecodedBlockSize - outputOffset);

					return;
				}

				decoded = (source[0] << 8) + source[1];
				source += 2;
			} else {
				int shift = (half == 0) ? shiftFirst : shiftSecond;

				decoded = (uint16)((int)(int8)value * (1 << shift));
			}

			WRITE_LE_UINT16(_decodedBlock + outputOffset, decoded);
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
