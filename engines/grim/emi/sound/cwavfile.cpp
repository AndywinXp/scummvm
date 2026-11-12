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
#include "common/file.h"
#include "common/textconsole.h"

#include "engines/grim/emi/sound/cwavesound.h"
#include "engines/grim/emi/sound/cwavfile.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/movie/codecs/vima.h"

namespace Grim {

struct McmpBlock {
	uint compSize;
	uint offset;
	uint decompSize;
};

static const uint32 kTagRiff = MKTAG('R', 'I', 'F', 'F');
static const uint32 kTagWave = MKTAG('W', 'A', 'V', 'E');
static const uint32 kTagFmt = MKTAG('f', 'm', 't', ' ');
static const uint32 kTagData = MKTAG('d', 'a', 't', 'a');
static const uint32 kTagFact = MKTAG('f', 'a', 'c', 't');
static const uint kWriteChunkSize = 0x400;
static const uint kWavHeaderSize = 44;
static const uint kMcmpHeaderSize = 6;
static const uint kMcmpEntrySize = 9;
static const uint kMcmpDataBase = 18;
static const uint kMcmpRatio = 5;
static const uint kMcmpPadding = 8;
static const byte kMaxVimaStep = 88;
static const uint32 kMaxChunkSkip = 0x7FFFFFFF;

CWavFile::CWavFile(EmiEngine *engine) : _engine(engine), _vimaTableReady(false) {
	_mcmpReadBuffer = new byte[kMcmpReadBufferSize]();
	_mcmpDecodeBuffer = new byte[kMcmpDecodeBufferSize]();
}

CWavFile::~CWavFile() {
	delete[] _mcmpReadBuffer;
	_mcmpReadBuffer = nullptr;

	delete[] _mcmpDecodeBuffer;
	_mcmpDecodeBuffer = nullptr;
}

bool CWavFile::read(LFile *file, CWaveSound *sound) {
	bool formatRead = false;
	byte raw[4] = { 0, 0, 0, 0 };
	byte raw16[2] = { 0, 0 };
	uint32 header[2] = { 0, 0 };

	file->read(raw, 4);

	if (READ_BE_UINT32(raw) != kTagRiff)
		return false;

	file->read(raw, 4);

	int riffSize = READ_LE_UINT32(raw);

	file->read(raw, 4);

	if (READ_BE_UINT32(raw) != kTagWave)
		return false;

	while (true) {
		if (file->tell() >= riffSize || !readChunkHeader(file, header))
			break;

		if (header[0] == kTagData) {
			if (!formatRead)
				return false;

			if (header[1] > CWaveSound::getRemainingSize(file)) {
				warning("CWavFile::read(): data chunk bigger than the file");

				return false;
			}

			sound->dataSize = header[1];
			sound->reallocData(header[1]);
			file->read(sound->samples, header[1]);
		} else if (header[0] == kTagFmt) {
			if (formatRead)
				return false;

			if (header[1] > kMaxChunkSkip) {
				warning("CWavFile::read(): invalid chunk size");

				return false;
			}

			formatRead = true;

			file->read(raw16, 2);

			if (READ_LE_UINT16(raw16) != 1)
				return false;

			sound->formatTag = CWaveSound::kFormatPcm;

			file->read(raw16, 2);

			sound->numChannels = READ_LE_UINT16(raw16);

			if ((int16)sound->numChannels < 1 || (int16)sound->numChannels > 2)
				return false;

			file->read(raw, 4);

			sound->sampleRate = (double)READ_LE_UINT32(raw);

			file->read(raw, 4);
			file->read(raw16, 2);
			file->read(raw16, 2);

			sound->bitsPerSample = READ_LE_UINT16(raw16);

			file->seek(-16, SEEK_CUR);
			file->seek(header[1], SEEK_CUR);
		} else {
			if (header[1] > kMaxChunkSkip) {
				warning("CWavFile::read(): invalid chunk size");

				return false;
			}

			file->seek(header[1], SEEK_CUR);
		}
	}

	sound->loopStart = 0;
	sound->loopEnd = sound->dataSize;

	return true;
}

bool CWavFile::readMcmpFromPath(LFile *file, CWaveSound *sound) {
	byte *buffer = new byte[kPathBufferSize];

	// The disasm leaves this buffer uninitialized
	memset(buffer, 0, kPathBufferSize * sizeof(byte));

	uint decodedSize = 0;

	if (decompressMcmpFromPath(file, buffer, kPathBufferSize, &decodedSize)) {
		delete[] buffer;

		return false;
	}

	bool result = parseWav(buffer, decodedSize, sound);

	delete[] buffer;

	return result;
}

bool CWavFile::readMcmp(LFile *file, CWaveSound *sound) {
	int position = file->tell();

	file->seek(0, SEEK_END);

	int endPosition = file->tell();

	file->seek(position, SEEK_SET);

	if (endPosition < position) {
		warning("CWavFile::readMcmp(): invalid file size");

		return false;
	}

	uint size = kMcmpRatio * (endPosition - position);
	byte *buffer = (byte *)_engine->getMemoryServices()->mallocImpl(size * sizeof(byte));

	if (!buffer)
		return false;

	uint decodedSize = 0;

	if (decompressMcmp(file, buffer, size, &decodedSize)) {
		_engine->getMemoryServices()->freeImpl(buffer);

		return false;
	}

	bool result = parseWav(buffer, decodedSize, sound);

	_engine->getMemoryServices()->freeImpl(buffer);

	return result;
}

bool CWavFile::write(LFile *file, CWaveSound *sound) {
	byte header[kWavHeaderSize];
	uint bytesPerSample = sound->bitsPerSample >> 3;

	memcpy(header, "RIFF", 4);
	WRITE_LE_UINT32(header + 4, sound->dataSize + 36);
	memcpy(header + 8, "WAVEfmt ", 8);
	WRITE_LE_UINT32(header + 16, 16);
	WRITE_LE_UINT16(header + 20, 1);
	WRITE_LE_UINT16(header + 22, sound->numChannels);
	WRITE_LE_UINT32(header + 24, (uint32)(int)sound->sampleRate);
	WRITE_LE_UINT32(header + 28, (uint32)((int64)sound->sampleRate * sound->numChannels * bytesPerSample));
	WRITE_LE_UINT16(header + 32, sound->numChannels * bytesPerSample);
	WRITE_LE_UINT16(header + 34, sound->bitsPerSample);
	memcpy(header + 36, "data", 4);
	WRITE_LE_UINT32(header + 40, sound->dataSize);

	file->write(header, kWavHeaderSize);

	char chunk[kWriteChunkSize];
	uint position = 0;
	uint remaining = sound->dataSize;

	while (remaining) {
		uint size = kWriteChunkSize;

		if (remaining <= kWriteChunkSize)
			size = remaining;

		sound->readSamples(chunk, &position, size, false);
		file->write(chunk, size);

		remaining -= size;
	}

	return true;
}

bool CWavFile::parseWav(const byte *buffer, uint bufferSize, CWaveSound *sound) {
	bool formatRead = false;
	int position = 0;
	byte raw[4] = { 0, 0, 0, 0 };
	byte raw16[2] = { 0, 0 };
	uint32 header[2] = { 0, 0 };

	memRead(buffer, bufferSize, &position, raw, 4);

	if (READ_BE_UINT32(raw) != kTagRiff)
		return false;

	memRead(buffer, bufferSize, &position, raw, 4);

	int riffSize = READ_LE_UINT32(raw);

	memRead(buffer, bufferSize, &position, raw, 4);

	if (READ_BE_UINT32(raw) != kTagWave)
		return false;

	// The disasm does not stop at the end of the buffer
	while (position < riffSize && (uint)position < bufferSize && readChunkHeaderMem(buffer, bufferSize, &position, header)) {
		if (header[0] != kTagData && header[1] > kMaxChunkSkip) {
			warning("CWavFile::parseWav(): invalid chunk size");

			return false;
		}

		switch (header[0]) {
		case kTagData:
			if (!formatRead)
				return false;

			if ((uint)position > bufferSize || header[1] > bufferSize - position) {
				warning("CWavFile::parseWav(): data chunk bigger than the buffer");

				return false;
			}

			sound->dataSize = header[1];
			sound->reallocData(header[1]);
			memRead(buffer, bufferSize, &position, sound->samples, header[1]);

			// The disasm seeks to the absolute offset given by the chunk size
			if (sound->bitsPerSample != 16)
				memSeek(&position, riffSize, header[1], 0);

			break;

		case kTagFact:
			memSeek(&position, riffSize, header[1], 1);

			break;

		case kTagFmt:
			if (formatRead)
				return false;

			formatRead = true;

			memRead(buffer, bufferSize, &position, raw16, 2);

			if (READ_LE_UINT16(raw16) != 1)
				return false;

			sound->formatTag = CWaveSound::kFormatPcm;

			memRead(buffer, bufferSize, &position, raw16, 2);

			sound->numChannels = READ_LE_UINT16(raw16);

			if ((int16)sound->numChannels < 1 || (int16)sound->numChannels > 2)
				return false;

			memRead(buffer, bufferSize, &position, raw, 4);

			sound->sampleRate = (double)READ_LE_UINT32(raw);

			memRead(buffer, bufferSize, &position, raw, 4);
			memRead(buffer, bufferSize, &position, raw16, 2);
			memRead(buffer, bufferSize, &position, raw16, 2);

			sound->bitsPerSample = READ_LE_UINT16(raw16);

			memSeek(&position, riffSize, 0xFFFFFFF0, 1);
			memSeek(&position, riffSize, header[1], 1);

			break;

		default:
			memSeek(&position, riffSize, header[1], 1);

			break;
		}
	}

	sound->loopStart = 0;
	sound->loopEnd = sound->dataSize;

	return true;
}

bool CWavFile::readChunkHeader(LFile *file, uint32 *header) {
	byte raw[4] = { 0, 0, 0, 0 };
	uint count = file->read(raw, 4);

	header[0] = READ_BE_UINT32(raw);

	if (!count)
		return false;

	count = file->read(raw, 4);
	header[1] = READ_LE_UINT32(raw);

	return count != 0;
}

bool CWavFile::readChunkHeaderMem(const byte *buffer, uint bufferSize, int *position, uint32 *header) {
	byte raw[4] = { 0, 0, 0, 0 };

	memRead(buffer, bufferSize, position, raw, 4);

	header[0] = READ_BE_UINT32(raw);

	memRead(buffer, bufferSize, position, raw, 4);

	header[1] = READ_LE_UINT32(raw);

	return true;
}

void CWavFile::memRead(const byte *buffer, uint bufferSize, int *position, void *dest, uint count) {
	if (*position < 0 || (uint)*position > bufferSize || count > bufferSize - *position) {
		warning("CWavFile::memRead(): read outside of the buffer");
		memset(dest, 0, count);
	} else {
		memcpy(dest, buffer + *position, count);
	}

	*position += count;
}

int CWavFile::memSeek(int *position, uint limit, uint offset, int whence) {
	if (whence == 0) {
		if (offset > limit)
			return 1;

		*position = offset;

		return 0;
	}

	if (whence != 1) {
		if (whence != 2)
			return 0;

		if (offset > limit)
			return 1;

		*position = limit - offset;

		return 0;
	}

	int newPosition = *position + offset;

	if ((uint)newPosition > limit || newPosition < 0)
		return 1;

	*position = newPosition;

	return 0;
}

int CWavFile::decompressMcmp(LFile *file, byte *output, uint outputSize, uint *decodedSize) {
	byte header[kMcmpHeaderSize] = { 0, 0, 0, 0, 0, 0 };

	*decodedSize = 0;

	file->read(header, kMcmpHeaderSize);

	if (memcmp(header, "MCMP", 4) != 0)
		return 3;

	uint numBlocks = READ_BE_UINT16(header + 4);

	file->seek(0, SEEK_END);

	uint fileSize = file->tell();

	file->seek(kMcmpHeaderSize, SEEK_SET);

	// The disasm reads and indexes the entry table wrongly beyond 255 blocks
	byte *table = new byte[numBlocks * kMcmpEntrySize + 1];

	memset(table, 0, numBlocks * kMcmpEntrySize + 1);
	file->read(table, numBlocks * kMcmpEntrySize);

	McmpBlock *blocks = new McmpBlock[numBlocks + 1];
	uint offset = kMcmpEntrySize * numBlocks + kMcmpDataBase;

	for (uint i = 0; i < numBlocks; i++) {
		blocks[i].decompSize = READ_BE_UINT32(table + i * kMcmpEntrySize + 1);
		blocks[i].compSize = READ_BE_UINT32(table + i * kMcmpEntrySize + 5);
		blocks[i].offset = offset;

		offset += blocks[i].compSize;
	}

	delete[] table;

	uint position = 0;
	int result = 0;

	for (uint i = 0; i < numBlocks; i++) {
		uint compSize = blocks[i].compSize;
		uint decompSize = blocks[i].decompSize;

		if (blocks[i].offset > fileSize || compSize > fileSize || decompSize > outputSize - position) {
			warning("CWavFile::decompressMcmp(): block %u outside of the file or of the buffer", i);
			result = 3;

			break;
		}

		file->seek(blocks[i].offset, SEEK_SET);

		if (compSize == decompSize) {
			file->read(output + position, compSize);
		} else {
			// A valid block needs at least four bits for each 16 bit sample
			if ((uint64)decompSize > (uint64)compSize * 4) {
				warning("CWavFile::decompressMcmp(): block %u is bigger than its compressed size allows", i);
				result = 3;

				break;
			}

			uint inputSize = compSize + decompSize * 2 + kMcmpPadding;
			byte *input = new byte[inputSize];

			memset(input, 0, inputSize);
			file->read(input, compSize);

			decodeMcmpBlock(output + position, input, decompSize);

			delete[] input;
		}

		position += decompSize;
	}

	delete[] blocks;

	*decodedSize = position;

	return result;
}

const char *CWavFile::getFilePath(LFile *file) {
	const char *path = nullptr;

	if (!file)
		return path;

	// The disasm returns the first dword of the file object, which is its vtable
	memcpy(&path, (const void *)file, sizeof(path));

	return path;
}

int CWavFile::decompressMcmpFromPath(LFile *file, byte *output, uint outputSize, uint *decodedSize) {
	const char *path = getFilePath(file);
	char name[kMaxPathLength + 1];
	uint length = 0;

	*decodedSize = 0;

	if (path) {
		while (length < kMaxPathLength && path[length]) {
			name[length] = path[length];
			length++;
		}
	}

	name[length] = 0;

	Common::File stream;

	if (!stream.open(Common::Path(name)))
		return 1;

	stream.read(_mcmpReadBuffer, kMcmpReadBufferSize);

	if (memcmp(_mcmpReadBuffer, "MCMP", 4) != 0) {
		stream.close();

		return 3;
	}

	uint numBlocks = READ_BE_UINT16(_mcmpReadBuffer + 4);

	if (kMcmpHeaderSize + numBlocks * kMcmpEntrySize > (uint)kMcmpReadBufferSize) {
		warning("CWavFile::decompressMcmpFromPath(): too many blocks");
		stream.close();

		return 3;
	}

	McmpBlock *blocks = new McmpBlock[numBlocks + 1];
	uint offset = kMcmpEntrySize * numBlocks + kMcmpDataBase;

	for (uint i = 0; i < numBlocks; i++) {
		const byte *entry = _mcmpReadBuffer + kMcmpHeaderSize + i * kMcmpEntrySize;

		blocks[i].decompSize = READ_BE_UINT32(entry + 1);
		blocks[i].compSize = READ_BE_UINT32(entry + 5);
		blocks[i].offset = offset;

		offset += blocks[i].compSize;
	}

	uint position = 0;
	int result = 0;

	for (uint i = 0; i < numBlocks; i++) {
		uint compSize = blocks[i].compSize;
		uint decompSize = blocks[i].decompSize;

		if (compSize > (uint)kMcmpReadBufferSize || decompSize > outputSize - position) {
			warning("CWavFile::decompressMcmpFromPath(): block %u does not fit the buffers", i);
			result = 3;

			break;
		}

		stream.seek(blocks[i].offset, SEEK_SET);
		stream.read(_mcmpReadBuffer, compSize);

		if (compSize == decompSize) {
			memcpy(output + position, _mcmpReadBuffer, decompSize);
		} else {
			if (decompSize > (uint)kMcmpDecodeBufferSize || (uint64)compSize + (uint64)decompSize * 2 + kMcmpPadding > (uint64)kMcmpReadBufferSize) {
				warning("CWavFile::decompressMcmpFromPath(): block %u does not fit the buffers", i);
				result = 3;

				break;
			}

			decodeMcmpBlock(_mcmpDecodeBuffer, _mcmpReadBuffer, decompSize);
			memcpy(output + position, _mcmpDecodeBuffer, decompSize);
		}

		position += decompSize;
	}

	delete[] blocks;
	stream.close();

	*decodedSize = position;

	return result;
}

void CWavFile::decodeMcmpBlock(byte *dest, const byte *source, uint size) {
	byte step = source[0];
	bool stereo = (step & 0x80) != 0;

	if (stereo)
		step = (byte)~step;

	if (step > kMaxVimaStep || (stereo && source[3] > kMaxVimaStep)) {
		warning("CWavFile::decodeMcmpBlock(): invalid step index");
		memset(dest, 0, size);

		return;
	}

	if (!_vimaTableReady) {
		vimaInit(_vimaTable);
		_vimaTableReady = true;
	}

	uint numSamples = size / 2 + 1;
	int16 *decoded = new int16[numSamples];

	memset(decoded, 0, numSamples * sizeof(int16));

	decompressVima(source, decoded, size, _vimaTable, false);

	memcpy(dest, decoded, size);

	delete[] decoded;
}

} // End of namespace Grim
