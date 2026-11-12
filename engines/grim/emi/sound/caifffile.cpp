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
#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/movie/codecs/vima.h"

namespace Grim {

static const uint32 kTagForm = MKTAG('F', 'O', 'R', 'M');
static const uint32 kTagAifc = MKTAG('A', 'I', 'F', 'C');
static const uint32 kTagAiff = MKTAG('A', 'I', 'F', 'F');
static const uint32 kTagComm = MKTAG('C', 'O', 'M', 'M');
static const uint32 kTagSsnd = MKTAG('S', 'S', 'N', 'D');
static const uint32 kTagMark = MKTAG('M', 'A', 'R', 'K');
static const uint32 kTagInst = MKTAG('I', 'N', 'S', 'T');
static const uint32 kTagAppl = MKTAG('A', 'P', 'P', 'L');
static const uint32 kTagFver = MKTAG('F', 'V', 'E', 'R');
static const uint32 kCompressionNone = MKTAG('N', 'O', 'N', 'E');
static const uint32 kCompressionSmsh = MKTAG('S', 'M', 'S', 'H');
static const uint32 kCompressionVima = MKTAG('V', 'I', 'M', 'A');
static const uint32 kFverValue = 0xA2805140;
static const uint kExtendedSize = 10;
static const uint kMarkerNameBufferSize = 80;
static const uint kMarkerNameMaxLength = 79;
static const int kMarkerHeaderSize = 7;
static const int kCommonChunkSize = 18;
static const uint kWriteChunkSize = 0x400;
static const uint kHeaderBufferSize = 160;
static const uint kVimaPadding = 8;
static const byte kMaxVimaStep = 88;

uint16 CAiffFile::readBigEndian16(LFile *file) {
	byte raw[2] = { 0, 0 };

	file->read(raw, 2);

	return READ_BE_UINT16(raw);
}

uint32 CAiffFile::readBigEndian32(LFile *file) {
	byte raw[4] = { 0, 0, 0, 0 };

	file->read(raw, 4);

	return READ_BE_UINT32(raw);
}

bool CAiffFile::read(LFile *file, CWaveSound *sound) {
	return parse(file, sound, nullptr, nullptr);
}

bool CAiffFile::write(LFile *file, CWaveSound *sound) {
	uint bytesPerFrame = (uint)(sound->bitsPerSample >> 3) * sound->numChannels;

	// The original divides by zero here
	if (!bytesPerFrame) {
		warning("CAiffFile::write(): invalid sample format");

		return false;
	}

	static const byte extended11025[kExtendedSize] = { 0x40, 0x0C, 0xAC, 0x44, 0, 0, 0, 0, 0, 0 };
	static const byte extended22050[kExtendedSize] = { 0x40, 0x0D, 0xAC, 0x44, 0, 0, 0, 0, 0, 0 };
	static const byte extended44100[kExtendedSize] = { 0x40, 0x0E, 0xAC, 0x44, 0, 0, 0, 0, 0, 0 };

	byte header[kHeaderBufferSize];
	uint size = 0;
	uint hasLoop = 0;
	uint loopStartFrame = 0;
	uint loopEndFrame = 0;

	memset(header, 0, sizeof(header));

	if (sound->loopEnd < sound->dataSize) {
		loopStartFrame = sound->loopStart / bytesPerFrame;
		hasLoop = 1;
		loopEndFrame = sound->loopEnd / bytesPerFrame;
	}

	memcpy(header + size, "FORM", 4);
	size += 4;
	WRITE_BE_UINT32(header + size, sound->dataSize + 54 * hasLoop + 64);
	size += 4;
	memcpy(header + size, "AIFCCOMM", 8);
	size += 8;
	WRITE_BE_UINT32(header + size, 24);
	size += 4;
	WRITE_BE_UINT16(header + size, sound->numChannels);
	size += 2;
	WRITE_BE_UINT32(header + size, sound->dataSize / bytesPerFrame);
	size += 4;
	WRITE_BE_UINT16(header + size, sound->bitsPerSample);
	size += 2;

	int sampleRate = (int)sound->sampleRate;

	if (sampleRate == 11025)
		memcpy(header + size, extended11025, kExtendedSize);
	else if (sampleRate == 44100)
		memcpy(header + size, extended44100, kExtendedSize);
	else
		memcpy(header + size, extended22050, kExtendedSize);

	size += kExtendedSize;
	memcpy(header + size, "NONE", 4);
	size += 4;
	size += 2;
	memcpy(header + size, "FVER", 4);
	size += 4;
	WRITE_BE_UINT32(header + size, 4);
	size += 4;
	WRITE_BE_UINT32(header + size, kFverValue);
	size += 4;

	if (hasLoop) {
		memcpy(header + size, "MARK", 4);
		size += 4;
		WRITE_BE_UINT32(header + size, 18);
		size += 4;
		WRITE_BE_UINT16(header + size, 2);
		size += 2;
		WRITE_BE_UINT16(header + size, 1);
		size += 2;
		WRITE_BE_UINT32(header + size, loopStartFrame);
		size += 4;
		size += 2;
		WRITE_BE_UINT16(header + size, 2);
		size += 2;
		WRITE_BE_UINT32(header + size, loopEndFrame);
		size += 4;
		size += 2;
		memcpy(header + size, "INST", 4);
		size += 4;
		WRITE_BE_UINT32(header + size, 20);
		size += 4;
		header[size++] = 60;
		header[size++] = 0;
		header[size++] = 0;
		header[size++] = 127;
		header[size++] = 0;
		header[size++] = 127;
		size += 2;
		WRITE_BE_UINT16(header + size, 1);
		size += 2;
		WRITE_BE_UINT16(header + size, 1);
		size += 2;
		WRITE_BE_UINT16(header + size, 2);
		size += 2;
		size += 6;
	}

	memcpy(header + size, "SSND", 4);
	size += 4;
	WRITE_BE_UINT32(header + size, sound->dataSize + 8);
	size += 4;
	size += 8;

	file->write(header, size);

	char chunk[kWriteChunkSize];
	uint position = 0;
	uint remaining = sound->dataSize;
	bool swapBytes = false;

	if (sound->bitsPerSample == 16)
		swapBytes = sound->littleEndian;

	while (remaining) {
		uint count = kWriteChunkSize;

		if (remaining <= kWriteChunkSize)
			count = remaining;

		sound->readSamples(chunk, &position, count, false);

		if (swapBytes) {
			for (uint i = 0; i < count / 2; i++) {
				char temp = chunk[i * 2];

				chunk[i * 2] = chunk[i * 2 + 1];
				chunk[i * 2 + 1] = temp;
			}
		}

		file->write(chunk, count);

		remaining -= count;
	}

	return true;
}

bool CAiffFile::parse(LFile *file, CWaveSound *sound, int *dataPosition, uint *dataSize) {
	bool result = true;
	AiffHeader header;
	uint32 chunk[2] = { 0, 0 };

	numMarkers = 0;
	loopEndId = 0;
	loopBeginId = 0;

	header.formSize = 0;
	header.isAifc = false;

	if (!readFormHeader(file, &header))
		return false;

	if (file->tell() < (int)header.formSize) {
		while (readChunkHeader(file, chunk)) {
			int chunkSize = chunk[1];

			switch (chunk[0]) {
			case kTagInst:
				result = readInstrument(file, sound);

				break;

			case kTagAppl:
				result = skipChunk(file, sound, chunkSize);

				break;

			case kTagComm:
				result = readCommonChunk(file, &chunkSize, &header, sound);

				break;

			case kTagFver:
				if (readBigEndian32(file) != kFverValue)
					result = false;

				break;

			case kTagMark:
				result = readMarkers(file, sound, chunkSize);

				break;

			case kTagSsnd:
				result = readSoundData(file, sound, chunkSize, dataPosition, dataSize);

				break;

			default:
				file->seek(chunkSize, SEEK_CUR);

				break;
			}

			if (file->tell() >= (int)header.formSize)
				break;
		}

		if (!result)
			return false;
	}

	uint beginPosition = 0;
	uint endPosition = sound->dataSize;

	for (int i = 0; i < numMarkers; i++) {
		if (markerId[i] == loopBeginId)
			beginPosition = markerPos[i];

		if (markerId[i] == loopEndId)
			endPosition = markerPos[i];
	}

	sound->loopStart = beginPosition;
	sound->loopEnd = endPosition;

	return result;
}

bool CAiffFile::readMarkers(LFile *file, CWaveSound *sound, int chunkSize) {
	uint16 count = readBigEndian16(file);
	int remaining = chunkSize - 2;

	numMarkers = 0;

	for (uint16 i = 0; i < count; i++) {
		if (remaining < kMarkerHeaderSize)
			break;

		uint16 id = readBigEndian16(file);
		uint32 frame = readBigEndian32(file);
		uint position = frame * (uint)(sound->bitsPerSample >> 3) * sound->numChannels;
		byte nameLength = 0;

		file->read(&nameLength, 1);

		remaining -= kMarkerHeaderSize;

		if (nameLength) {
			char name[kMarkerNameBufferSize];
			uint readLength = nameLength;

			if (readLength > kMarkerNameMaxLength)
				readLength = kMarkerNameMaxLength;

			file->read(name, readLength);

			if (nameLength <= kMarkerNameMaxLength) {
				name[nameLength] = 0;
			} else {
				file->seek((int)nameLength - (int)kMarkerNameMaxLength, SEEK_CUR);
				name[kMarkerNameMaxLength] = 0;
			}

			remaining -= nameLength;

			if (!scumm_strnicmp(name, "beg", 3))
				loopBeginId = id;
			else if (!scumm_strnicmp(name, "end", 3))
				loopEndId = id;
		}

		if (((nameLength - 1) & 1) != 0) {
			file->seek(1, SEEK_CUR);
			remaining--;
		}

		// The disasm does not check the number of markers
		if (numMarkers < kMaxMarkers) {
			markerId[numMarkers] = id;
			markerPos[numMarkers] = position;
			numMarkers++;
		} else {
			warning("CAiffFile::readMarkers(): too many markers");
		}
	}

	return true;
}

bool CAiffFile::readInstrument(LFile *file, CWaveSound *sound) {
	byte skipped[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	uint16 sustainLoop[3] = { 0, 0, 0 };
	uint16 releaseLoop[3] = { 0, 0, 0 };

	file->read(skipped, 8);

	readLoop(file, sustainLoop);
	readLoop(file, releaseLoop);

	if (sustainLoop[0]) {
		loopBeginId = sustainLoop[1];
		loopEndId = sustainLoop[2];

		return true;
	}

	if (releaseLoop[0]) {
		loopBeginId = releaseLoop[1];
		loopEndId = releaseLoop[2];
	}

	return true;
}

bool CAiffFile::readFormHeader(LFile *file, AiffHeader *header) {
	if (readBigEndian32(file) != kTagForm)
		return false;

	header->formSize = readBigEndian32(file);

	uint32 type = readBigEndian32(file);

	if (type == kTagAifc) {
		header->isAifc = true;

		return true;
	}

	if (type != kTagAiff)
		return false;

	header->isAifc = false;

	return true;
}

bool CAiffFile::readChunkHeader(LFile *file, uint32 *header) {
	byte raw[4] = { 0, 0, 0, 0 };

	file->read(raw, 4);

	header[0] = READ_BE_UINT32(raw);

	uint count = file->read(raw, 4);

	header[1] = READ_BE_UINT32(raw);

	return count != 0;
}

bool CAiffFile::readCommonChunk(LFile *file, const int *chunkSize, const AiffHeader *header, CWaveSound *sound) {
	sound->numChannels = readBigEndian16(file);

	uint32 frames = readBigEndian32(file);

	sound->bitsPerSample = readBigEndian16(file);

	byte extended[kExtendedSize] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	file->read(extended, kExtendedSize);

	sound->sampleRate = extendedToDouble(extended);

	if (header->isAifc) {
		uint32 compression = readBigEndian32(file);

		switch (compression) {
		case kCompressionNone:
			sound->formatTag = CWaveSound::kFormatPcm;
			sound->dataSize = (uint)(sound->bitsPerSample >> 3) * sound->numChannels * frames;

			break;

		case kCompressionSmsh:
			sound->formatTag = CWaveSound::kFormatSmsh;
			sound->dataSize = (uint)(sound->bitsPerSample >> 3) * sound->numChannels * frames;

			break;

		case kCompressionVima:
			sound->formatTag = CWaveSound::kFormatVima;

			break;

		default:
			return false;
		}

		byte nameLength = 0;

		file->read(&nameLength, 1);

		if ((nameLength & 1) == 0)
			nameLength++;

		file->seek(nameLength, SEEK_CUR);

		return true;
	}

	sound->dataSize = (uint)(sound->bitsPerSample >> 3) * sound->numChannels * frames;
	sound->formatTag = CWaveSound::kFormatPcm;

	file->seek(*chunkSize - kCommonChunkSize, SEEK_CUR);

	return true;
}

bool CAiffFile::readSoundData(LFile *file, CWaveSound *sound, int chunkSize, int *dataPosition, uint *dataSizeOut) {
	uint bytesPerFrame = (uint)(sound->bitsPerSample >> 3) * sound->numChannels;

	// The original divides by zero here
	if (!bytesPerFrame) {
		warning("CAiffFile::readSoundData(): invalid sample format");

		return false;
	}

	uint frames = sound->dataSize / bytesPerFrame;
	int dataBytes = chunkSize - 8;

	if (readBigEndian32(file) != 0)
		return false;

	if (readBigEndian32(file) != 0)
		return false;

	if (dataPosition && dataSizeOut) {
		*dataPosition = file->tell();
		*dataSizeOut = dataBytes;

		file->seek(dataBytes, SEEK_SET);

		// The disasm seeks to the absolute offset of the padding
		if (dataBytes & 1)
			file->seek(1, SEEK_SET);

		return true;
	}

	if (sound->formatTag == CWaveSound::kFormatVima) {
		uint compressedSize = readBigEndian32(file);
		uint decompressedSize = readBigEndian32(file);

		if (compressedSize > CWaveSound::getRemainingSize(file) || (uint64)decompressedSize > (uint64)compressedSize * 4) {
			warning("CAiffFile::readSoundData(): invalid compressed block size");

			return false;
		}

		uint inputSize = compressedSize + decompressedSize * 2 + kVimaPadding;
		byte *block = new byte[inputSize];

		memset(block, 0, inputSize);
		file->read(block, compressedSize);

		sound->reallocData(decompressedSize);
		sound->dataSize = decompressedSize;

		decodeVimaBlockReset(sound->samples, block, decompressedSize);

		delete[] block;

		sound->formatTag = CWaveSound::kFormatPcm;

		return true;
	}

	if (dataBytes < 0 || (uint)dataBytes > CWaveSound::getRemainingSize(file)) {
		warning("CAiffFile::readSoundData(): sound data bigger than the file");

		return false;
	}

	sound->reallocData(dataBytes);
	file->read(sound->samples, sound->allocatedSize);

	// The disasm seeks to the absolute offset of the padding
	if (dataBytes & 1)
		file->seek(1, SEEK_SET);

	if (sound->formatTag != CWaveSound::kFormatPcm)
		return true;

	if (sound->bitsPerSample <= 8) {
		uint count = frames;

		if (count > sound->allocatedSize) {
			warning("CAiffFile::readSoundData(): more frames than data");
			count = sound->allocatedSize;
		}

		byte *sample = sound->samples;

		while (count) {
			*sample ^= 0x80;
			sample++;
			count--;
		}

		return true;
	}

	if (sound->bitsPerSample > 16)
		return false;

	uint numWords = (uint)sound->numChannels * frames;

	if (numWords > sound->allocatedSize / 2) {
		warning("CAiffFile::readSoundData(): more frames than data");
		numWords = sound->allocatedSize / 2;
	}

	byte *sample = sound->samples;

	for (uint i = 0; i < numWords; i++) {
		byte temp = sample[0];

		sample[0] = sample[1];
		sample[1] = temp;
		sample += 2;
	}

	return true;
}

bool CAiffFile::skipChunk(LFile *file, CWaveSound *sound, int chunkSize) {
	int skipSize = chunkSize;

	if (chunkSize & 1)
		skipSize = chunkSize + 1;

	file->seek(skipSize, SEEK_CUR);

	return true;
}

bool CAiffFile::readLoop(LFile *file, uint16 *loop) {
	loop[0] = readBigEndian16(file);
	loop[1] = readBigEndian16(file);
	loop[2] = readBigEndian16(file);

	return true;
}

double CAiffFile::extendedToDouble(const byte *extended) {
	int shift = 0x401E - (((extended[0] & 0x7F) << 8) | extended[1]);

	// The disasm shifts by an unchecked amount
	if (shift < 0 || shift > 31) {
		warning("CAiffFile::extendedToDouble(): invalid exponent");

		return 0.0;
	}

	return (double)(int)(READ_BE_UINT32(extended + 2) >> shift);
}

void CAiffFile::decodeVimaBlockReset(byte *dest, const byte *source, uint size) {
	decodeVimaBlock(dest, source, size);
}

void CAiffFile::decodeVimaBlock(byte *dest, const byte *source, uint size) {
	byte step = source[0];
	bool stereo = (step & 0x80) != 0;

	if (stereo)
		step = (byte)~step;

	if (step > kMaxVimaStep || (stereo && source[3] > kMaxVimaStep)) {
		warning("CAiffFile::decodeVimaBlock(): invalid step index");
		memset(dest, 0, size);

		return;
	}

	uint16 vimaTable[kVimaTableSize];

	vimaInit(vimaTable);

	decompressVima(source, (int16 *)dest, size, vimaTable, false);
}

} // End of namespace Grim
