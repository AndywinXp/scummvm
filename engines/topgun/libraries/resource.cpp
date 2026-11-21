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
		} else if (command <= kMaxRunLength) {
			count = command;
		} else {
			count = command - kMaxRunLength;
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

	if (compressedSize < kBitmapRecordMinSize) {
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
	byte header[kHeaderSize];
	if (file->stream->read(header, kHeaderSize) != kHeaderSize)
		return false;

	file->magic = READ_LE_UINT16(header);
	file->version = READ_LE_UINT16(header + 2);
	file->entryCount = READ_LE_UINT16(header + 4);
	file->dwordCount = READ_LE_UINT16(header + 6);
	file->nameBlockSize = READ_LE_UINT16(header + 8);
	file->variableCount = READ_LE_UINT16(header + 10);
	file->extraSize = READ_LE_UINT16(header + 12);

	if (file->magic != kHeaderMagic)
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

void ResourceLibrary::readEntry(const ResourceFile *file, uint16 index, ResourceEntry &entry) const {
	const byte *data = file->index + (uint32)index * kIndexEntrySize;
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

	byte header[kWaveStreamHeaderSize];
	if (record->stream->read(header, kWaveStreamHeaderSize) != kWaveStreamHeaderSize)
		return 0;

	WaveStreamInfo *info = new WaveStreamInfo();
	info->file = file;
	info->position = (uint32)record->stream->pos();

	uint16 waveInfo = registerWaveInfo(info);
	if (waveInfo == 0) {
		delete info;
		return 0;
	}

	SoundCreateInfo createInfo;
	createInfo.waveInfo = waveInfo;
	createInfo.header = header + 2;
	return (uint16)_engine->audioLibrary()->audioCommand(kCreateSoundCommand, READ_LE_UINT16(header), 0, &createInfo);
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

	uint32 offset = kHeaderSize + (uint32)kIndexEntrySize * record->entryCount + record->extraSize + (uint16)(record->dwordCount * 4);
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

	uint32 offset = kHeaderSize + (uint32)kIndexEntrySize * record->entryCount + record->extraSize;
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

	if (entry.type == kResidentType) {
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

	if (entry.type == kBitmapType)
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
	uint32 indexSize = (uint32)kIndexEntrySize * record->entryCount;
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

	delete _waveInfos[waveInfo];
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
	return (int32)info->position;
}

} // End of namespace Topgun
