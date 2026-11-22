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

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource.h"
#include "topgun/libraries/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

ResourceLibrary::ResourceLibrary(TopgunEngine *engine) : _engine(engine) {
}

ResourceLibrary::~ResourceLibrary() {
	while (!_files.empty())
		resCloseFile(_files.begin()->_key);

	for (WaveInfoMap::iterator i = _waveInfos.begin(); i != _waveInfos.end(); ++i)
		delete i->_value;

	_waveInfos.clear();
}

void ResourceLibrary::decodeRleCelBitmap(const byte *source, uint32 sourceSize, byte *destination, uint32 destinationSize) {
	uint32 headerLength = READ_LE_UINT16(source);
	if (headerLength > sourceSize || headerLength > destinationSize) {
		warning("ResourceLibrary::decodeRleCelBitmap(): stream overruns its buffer");
		return;
	}

	memcpy(destination, source, headerLength);

	uint32 sourcePosition = headerLength;
	uint32 destinationPosition = headerLength;
	bool finished = false;

	while (sourcePosition < sourceSize) {
		byte command = source[sourcePosition++];
		uint32 count = 0;
		bool fill = true;

		if (command == 0) {
			if (sourceSize - sourcePosition < 2)
				break;

			int16 word = (int16)READ_LE_UINT16(source + sourcePosition);
			sourcePosition += 2;

			if (word == 0) {
				finished = true;
				break;
			}

			if (word > 0) {
				count = word;
			} else {
				count = (uint16)(0 - word);
				fill = false;
			}
		} else if (command <= TG_RES_MAX_RUN_LENGTH) {
			count = command;
		} else {
			count = command - TG_RES_MAX_RUN_LENGTH;
			fill = false;
		}

		if (count > destinationSize - destinationPosition)
			break;

		if (fill) {
			if (sourcePosition >= sourceSize)
				break;

			memset(destination + destinationPosition, source[sourcePosition], count);
			sourcePosition++;
		} else {
			if (count > sourceSize - sourcePosition)
				break;

			memcpy(destination + destinationPosition, source + sourcePosition, count);
			sourcePosition += count;
		}

		destinationPosition += count;
	}

	if (!finished)
		warning("ResourceLibrary::decodeRleCelBitmap(): stream overruns its buffer");
}

byte *ResourceLibrary::expandCelBitmapRecord(byte *compressed, uint32 compressedSize, uint32 *outSize) {
	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	if (compressedSize < TG_RES_BITMAP_RECORD_MIN_SIZE) {
		warning("ResourceLibrary::expandCelBitmapRecord(): bitmap record is too short");
		return utilsLibrary->tgFree(compressed);
	}

	uint32 size = (uint32)READ_LE_UINT16(compressed + 4) * READ_LE_UINT16(compressed + 6) + READ_LE_UINT16(compressed);
	byte *destination = utilsLibrary->tgMalloc(size, 0);
	if (!destination)
		return utilsLibrary->tgFree(compressed);

	decodeRleCelBitmap(compressed, compressedSize, destination, size);

	if (outSize)
		*outSize = size;

	utilsLibrary->tgFree(compressed);
	return destination;
}

bool ResourceLibrary::readChunkedWithCallback(ResourceFile *file, byte *buffer, uint32 count) {
	int64 position = file->stream->pos();

	if (count == 0)
		return true;

	if (file->callbackInterval == 0) {
		warning("ResourceLibrary::readChunkedWithCallback(): the callback interval is zero");
		return false;
	}

	do {
		uint32 chunk = ((int32)count <= file->callbackInterval) ? count : (uint32)file->callbackInterval;
		if (file->stream->read(buffer, chunk) != chunk)
			return false;

		buffer += chunk;
		position += chunk;
		file->callbackCounter += chunk;
		count -= chunk;

		if (count != 0 && (int32)file->callbackCounter >= file->callbackInterval) {
			uint32 savedCounter = file->callbackCounter;
			int32 savedInterval = file->callbackInterval;

			_engine->resourceLoadProgressCallback();

			file->callbackInterval = savedInterval;
			file->callbackCounter = savedCounter - savedInterval;
			file->stream->seek(position, SEEK_SET);
		}
	} while (count != 0);

	return true;
}

bool ResourceLibrary::readResourceFileHeader(ResourceFile *file) {
	byte header[TG_RES_HEADER_SIZE];
	if (file->stream->read(header, TG_RES_HEADER_SIZE) != TG_RES_HEADER_SIZE)
		return false;

	file->magic = READ_LE_UINT16(header);
	file->version = READ_LE_UINT16(header + 2);
	file->entryCount = READ_LE_UINT16(header + 4);
	file->dwordCount = READ_LE_UINT16(header + 6);
	file->nameBlockSize = READ_LE_UINT16(header + 8);
	file->variableCount = READ_LE_UINT16(header + 10);
	file->extraSize = READ_LE_UINT16(header + 12);

	if (file->magic != TG_RES_HEADER_MAGIC)
		return false;

	return file->entryCount != 0;
}

ResourceLibrary::ResourceFile *ResourceLibrary::findFile(uint16 file, const char *caller) {
	if (file == 0)
		return nullptr;

	if (!_files.contains(file)) {
		warning("ResourceLibrary::%s(): invalid file handle %d", caller, file);
		return nullptr;
	}

	return _files[file];
}

uint16 ResourceLibrary::registerFile(ResourceFile *record) {
	for (uint32 attempt = 0; attempt < 0xFFFF; attempt++) {
		uint16 handle = _nextFileHandle;
		_nextFileHandle = (_nextFileHandle == 0xFFFF) ? 1 : _nextFileHandle + 1;

		if (!_files.contains(handle)) {
			_files[handle] = record;
			return handle;
		}
	}

	return 0;
}

uint16 ResourceLibrary::registerWaveInfo(WaveStreamInfo *info) {
	for (uint32 attempt = 0; attempt < 0xFFFF; attempt++) {
		uint16 handle = _nextWaveHandle;
		_nextWaveHandle = (_nextWaveHandle == 0xFFFF) ? 1 : _nextWaveHandle + 1;

		if (!_waveInfos.contains(handle)) {
			_waveInfos[handle] = info;
			return handle;
		}
	}

	return 0;
}

void ResourceLibrary::destroyWaveInfo(WaveStreamInfo *info) {
	_engine->utilsLibrary()->tgFree(info->buffer);
	delete info;
}

bool ResourceLibrary::openCompressedWave(ResourceFile *file, WaveStreamInfo *info, const byte *header) {
	byte compressedHeader[TG_RES_ADPCM_HEADER_SIZE];
	if (file->stream->read(compressedHeader, TG_RES_ADPCM_HEADER_SIZE) != TG_RES_ADPCM_HEADER_SIZE)
		return false;

	info->position += TG_RES_ADPCM_HEADER_SIZE;
	info->end = info->position + READ_LE_UINT32(compressedHeader + 4);
	info->bufferSize = READ_LE_UINT32(header + 0x0A);
	info->buffer = _engine->utilsLibrary()->tgMalloc(info->bufferSize, 0);
	if (!info->buffer)
		return false;

	if (READ_LE_UINT16(compressedHeader + 8) != 1)
		return false;

	info->channels = READ_LE_UINT16(header + 4);
	info->compressed = true;
	return true;
}

void ResourceLibrary::resetAdpcmState(WaveStreamInfo *info) {
	info->predictor[0] = info->predictor[1] = 0;
	info->stepPosition[0] = info->stepPosition[1] = 0;
}

void ResourceLibrary::decodeAdpcm(WaveStreamInfo *info, byte *destination, const byte *source, uint32 inputBytes) {
	static const uint16 stepTable[TG_RES_ADPCM_STEP_TABLE_ROWS] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
		157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
		3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};

	static const int8 stepAdjust[16] = {
		-8, -8, -8, -8, 16, 32, 48, 64,
		-8, -8, -8, -8, 16, 32, 48, 64
	};

	for (uint32 i = 0; i < inputBytes; i++) {
		for (uint16 half = 0; half < 2; half++) {
			byte nibble = (half == 0) ? (source[i] & 0x0F) : (source[i] >> 4);
			uint16 channel = (info->channels == 2) ? half : 0;

			uint32 step = stepTable[info->stepPosition[channel] >> 3];
			uint32 delta = step >> 3;
			if (nibble & 1)
				delta += step >> 2;

			if (nibble & 2)
				delta += step >> 1;

			if (nibble & 4)
				delta += step;

			int32 predictor = info->predictor[channel];
			predictor += (nibble & 8) ? -(int32)delta : (int32)delta;
			info->predictor[channel] = CLIP<int32>(predictor, -0x8000, 0x7FFF);

			info->stepPosition[channel] = CLIP<int32>(info->stepPosition[channel] + stepAdjust[nibble], 0, TG_RES_ADPCM_MAX_STEP_POSITION);

			WRITE_LE_UINT16(destination, (uint16)info->predictor[channel]);
			destination += 2;
		}
	}
}

int32 ResourceLibrary::readCompressedWave(ResourceFile *file, WaveStreamInfo *info, byte *buffer, int32 count) {
	if (count == 0)
		return 0;

	if (count < 0 || (count & 3) != 0) {
		warning("ResourceLibrary::readCompressedWave(): the byte count %d is not a multiple of 4", count);
		return -1;
	}

	int32 total = 0;

	while (true) {
		if (info->available == 0) {
			info->available = MIN(info->end - info->position, info->bufferSize);
			if (info->available == 0)
				return -1;

			if (file->stream->read(info->buffer, info->available) != info->available)
				return -1;

			info->position += info->available;
			info->consumed = 0;
		}

		uint32 inputBytes = MIN((uint32)(count >> 2), info->available);
		decodeAdpcm(info, buffer, info->buffer + info->consumed, inputBytes);

		info->consumed += inputBytes;
		info->available -= inputBytes;
		buffer += inputBytes * 4;
		total += inputBytes * 4;
		count -= inputBytes * 4;

		if (count == 0)
			return total;
	}
}

void ResourceLibrary::readEntry(const ResourceFile *file, uint16 index, ResourceEntry &entry) const {
	const byte *data = file->index + (uint32)index * TG_RES_INDEX_ENTRY_SIZE;
	entry.type = READ_LE_UINT16(data);
	entry.offset = READ_LE_UINT32(data + 2);
	entry.size = READ_LE_UINT32(data + 6);
}

uint16 ResourceLibrary::resCloseFile(uint16 file) {
	ResourceFile *record = findFile(file, "resCloseFile");
	if (!record)
		return 0;

	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	delete record->stream;
	utilsLibrary->tgFree(record->index);
	utilsLibrary->tgFree(record->extra);
	delete record;
	_files.erase(file);
	return 0;
}

uint16 ResourceLibrary::resCountResources(uint16 file) {
	ResourceFile *record = findFile(file, "resCountResources");
	if (!record)
		return 0;

	return record->entryCount;
}

uint16 ResourceLibrary::resCountVariables(uint16 file) {
	ResourceFile *record = findFile(file, "resCountVariables");
	if (!record)
		return 0;

	return record->variableCount;
}

uint16 ResourceLibrary::resCreateWaveEvent(uint16 file, uint16 index) {
	if (resSeekResource(file, index) == 0)
		return 0;

	ResourceFile *record = findFile(file, "resCreateWaveEvent");

	bool hasCompressionFlag = (_engine->_version >= 1);
	uint32 headerSize = hasCompressionFlag ? TG_RES_WAVE_STREAM_HEADER_SIZE_V1_1 : TG_RES_WAVE_STREAM_HEADER_SIZE;

	byte header[TG_RES_WAVE_STREAM_HEADER_SIZE_V1_1];
	if (record->stream->read(header, headerSize) != headerSize)
		return 0;

	WaveStreamInfo *info = new WaveStreamInfo();
	info->file = file;
	info->position = (uint32)record->stream->pos();

	if (hasCompressionFlag && header[TG_RES_COMPRESSION_FLAG_OFFSET] != 0 && !openCompressedWave(record, info, header)) {
		destroyWaveInfo(info);
		return 0;
	}

	uint16 waveInfo = registerWaveInfo(info);
	if (waveInfo == 0) {
		destroyWaveInfo(info);
		return 0;
	}

	SoundCreateInfo createInfo;
	createInfo.waveInfo = waveInfo;
	createInfo.header = header + 2;
	return (uint16)_engine->audioLibrary()->audioCommand(TG_RES_CREATE_SOUND_COMMAND, READ_LE_UINT16(header), 0, &createInfo);
}

uint32 ResourceLibrary::resGetSize(uint16 file, uint16 index) {
	ResourceFile *record = findFile(file, "resGetSize");
	if (!record)
		return 0;

	if (record->entryCount <= index)
		return 0;

	ResourceEntry entry;
	readEntry(record, index, entry);
	return entry.size;
}

uint16 ResourceLibrary::resGetType(uint16 file, uint16 index) {
	ResourceFile *record = findFile(file, "resGetType");
	if (!record)
		return 0;

	if (record->entryCount <= index)
		return 0;

	ResourceEntry entry;
	readEntry(record, index, entry);
	return entry.type;
}

byte *ResourceLibrary::resLoadNameTable(uint16 file, uint32 *outSize) {
	ResourceFile *record = findFile(file, "resLoadNameTable");
	if (!record)
		return nullptr;

	if (record->nameBlockSize == 0)
		return nullptr;

	uint32 offset = TG_RES_HEADER_SIZE + (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount + record->extraSize + (uint16)(record->dwordCount * 4);
	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return nullptr;

	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	uint32 size = record->nameBlockSize;
	byte *buffer = utilsLibrary->tgMalloc(size, 0);
	if (!buffer)
		return nullptr;

	if (record->stream->read(buffer, size) != size)
		return utilsLibrary->tgFree(buffer);

	if (outSize)
		*outSize = size;

	return buffer;
}

uint16 ResourceLibrary::resLoadPalette(uint16 file, byte *buffer) {
	ResourceFile *record = findFile(file, "resLoadPalette");
	if (!record)
		return 0;

	if (!buffer)
		return 0;

	if (record->dwordCount == 0)
		return 0;

	uint32 offset = TG_RES_HEADER_SIZE + (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount + record->extraSize;
	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return 0;

	uint16 byteCount = (uint16)(record->dwordCount * 4);
	if (record->stream->read(buffer, byteCount) != byteCount)
		return 0;

	return record->dwordCount;
}

byte *ResourceLibrary::resLoadResource(uint16 file, uint16 index, uint32 *outSize) {
	ResourceFile *record = findFile(file, "resLoadResource");
	if (!record)
		return nullptr;

	if (record->entryCount <= index)
		return nullptr;

	ResourceEntry entry;
	readEntry(record, index, entry);

	if (entry.type == TG_RES_RESIDENT_TYPE || (entry.type == TG_RES_RESIDENT_ID_LIST_TYPE && _engine->_version >= 1)) {
		if (!record->extra) {
			warning("ResourceLibrary::resLoadResource(): the file has no resident block");
			return nullptr;
		}

		if (outSize)
			*outSize = entry.size;

		return record->extra + (uint16)entry.offset;
	}

	if (!record->stream->seek(entry.offset, SEEK_SET) || record->stream->pos() != entry.offset)
		return nullptr;

	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	byte *buffer = utilsLibrary->tgMalloc(entry.size, 0);
	if (!buffer)
		return nullptr;

	bool success;
	if (record->callbackSet)
		success = readChunkedWithCallback(record, buffer, entry.size);
	else
		success = (record->stream->read(buffer, entry.size) == entry.size);

	if (!success)
		return utilsLibrary->tgFree(buffer);

	if (entry.type == TG_RES_BITMAP_TYPE)
		return expandCelBitmapRecord(buffer, entry.size, outSize);

	if (outSize)
		*outSize = entry.size;

	return buffer;
}

uint16 ResourceLibrary::resOpenFile(const Common::String &path) {
	ResourceFile *record = new ResourceFile();
	record->stream = new Common::File();
	if (!record->stream->open(Common::Path(path))) {
		delete record->stream;
		delete record;
		return 0;
	}

	uint16 file = registerFile(record);
	if (file == 0) {
		delete record->stream;
		delete record;
		return 0;
	}

	if (!readResourceFileHeader(record)) {
		resCloseFile(file);
		return 0;
	}

	UtilsLibrary *utilsLibrary = _engine->utilsLibrary();

	// TODO: the original computes this size in 16 bits and overflows with 6554 or more entries; check whether any game relies on this overflow
	uint32 indexSize = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount;
	if (indexSize > 0xFFFF)
		warning("ResourceLibrary::resOpenFile(): too many entries (%d)", record->entryCount);

	record->index = utilsLibrary->tgMalloc(indexSize, 0);
	if (!record->index || record->stream->read(record->index, indexSize) != indexSize) {
		resCloseFile(file);
		return 0;
	}

	if (record->extraSize != 0) {
		record->extra = utilsLibrary->tgMalloc(record->extraSize, 0);
		if (!record->extra || record->stream->read(record->extra, record->extraSize) != record->extraSize) {
			resCloseFile(file);
			return 0;
		}
	}

	return file;
}

uint32 ResourceLibrary::resRead(uint16 file, byte *buffer, uint32 count) {
	ResourceFile *record = findFile(file, "resRead");
	if (!record) {
		if (file == 0)
			warning("ResourceLibrary::resRead(): invalid file handle 0");

		return 0xFFFFFFFF;
	}

	return record->stream->read(buffer, count);
}

uint32 ResourceLibrary::resSeek(uint16 file, int32 offset, int16 origin) {
	ResourceFile *record = findFile(file, "resSeek");
	if (!record) {
		if (file == 0)
			warning("ResourceLibrary::resSeek(): invalid file handle 0");

		return 0xFFFFFFFF;
	}

	if (origin < SEEK_SET || origin > SEEK_END) {
		warning("ResourceLibrary::resSeek(): invalid origin %d", origin);
		return 0xFFFFFFFF;
	}

	if (!record->stream->seek(offset, origin))
		return 0xFFFFFFFF;

	return (uint32)record->stream->pos();
}

uint32 ResourceLibrary::resSeekResource(uint16 file, uint16 index) {
	ResourceFile *record = findFile(file, "resSeekResource");
	if (!record)
		return 0;

	if (record->entryCount <= index)
		return 0;

	ResourceEntry entry;
	readEntry(record, index, entry);

	if (!record->stream->seek(entry.offset, SEEK_SET) || record->stream->pos() != entry.offset)
		return 0;

	return entry.size;
}

bool ResourceLibrary::resSetCallback(uint16 file, bool callbackActive, uint32 interval) {
	ResourceFile *record = findFile(file, "resSetCallback");
	if (!record) {
		if (file == 0)
			warning("ResourceLibrary::resSetCallback(): invalid file handle 0");

		return false;
	}

	bool previous = record->callbackSet;
	record->callbackSet = callbackActive;
	record->callbackInterval = (int32)interval;
	record->callbackCounter = 0;
	return previous;
}

void ResourceLibrary::waveDone(uint16 waveInfo, uint16 flag) {
	if (!_waveInfos.contains(waveInfo)) {
		warning("ResourceLibrary::waveDone(): invalid wave handle %d", waveInfo);
		return;
	}

	destroyWaveInfo(_waveInfos[waveInfo]);
	_waveInfos.erase(waveInfo);
}

int32 ResourceLibrary::waveRead(uint16 waveInfo, byte *buffer, int32 count) {
	if (!_waveInfos.contains(waveInfo)) {
		warning("ResourceLibrary::waveRead(): invalid wave handle %d", waveInfo);
		return -1;
	}

	WaveStreamInfo *info = _waveInfos[waveInfo];
	if (!_files.contains(info->file)) {
		warning("ResourceLibrary::waveRead(): file is closed");
		return -1;
	}

	ResourceFile *record = _files[info->file];
	record->stream->seek(info->position, SEEK_SET);

	if (info->compressed)
		return readCompressedWave(record, info, buffer, count);

	uint32 bytesRead = record->stream->read(buffer, count);
	info->position += bytesRead;
	return (int32)bytesRead;
}

int32 ResourceLibrary::waveSeek(uint16 waveInfo, int32 offset, int16 origin) {
	if (!_waveInfos.contains(waveInfo)) {
		warning("ResourceLibrary::waveSeek(): invalid wave handle %d", waveInfo);
		return -1;
	}

	WaveStreamInfo *info = _waveInfos[waveInfo];
	if (!_files.contains(info->file)) {
		warning("ResourceLibrary::waveSeek(): file is closed");
		return -1;
	}

	if (origin < SEEK_SET || origin > SEEK_END) {
		warning("ResourceLibrary::waveSeek(): invalid origin %d", origin);
		return -1;
	}

	ResourceFile *record = _files[info->file];
	if (!record->stream->seek(offset, origin))
		return -1;

	info->position = (uint32)record->stream->pos();

	if (info->compressed) {
		resetAdpcmState(info);
		info->available = 0;
	}

	return (int32)info->position;
}

} // End of namespace Topgun
