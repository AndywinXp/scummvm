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

#include "topgun/libraries/resource/resource_v1_2.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

ResourceLibrary_v1_2::ResourceLibrary_v1_2(TopgunEngine_v1_2 *engine) : ResourceLibrary(engine) {
}

ResourceLibrary_v1_2::~ResourceLibrary_v1_2() {
	while (!_files.empty())
		resCloseFile(_files.begin()->_key);

	for (WaveStreamMap::iterator i = _waveStreams.begin(); i != _waveStreams.end(); ++i)
		delete i->_value;

	_waveStreams.clear();
}

TopgunEngine_v1_2 *ResourceLibrary_v1_2::engine() const {
	return static_cast<TopgunEngine_v1_2 *>(_engine);
}

ResourceLibrary_v1_2::ResourceFile *ResourceLibrary_v1_2::findFile(uint16 file, const char *caller) {
	return static_cast<ResourceFile *>(ResourceLibrary::findFile(file, caller));
}

bool ResourceLibrary_v1_2::readResourceFileHeader(ResourceFile *file) {
	if (file->stream->read(file->header, TG_RES_V1_2_HEADER_SIZE) != TG_RES_V1_2_HEADER_SIZE)
		return false;

	const byte *header = file->header;
	file->magic = READ_LE_UINT16(header);
	file->entryCount = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_RESOURCE_COUNT_OFFSET);
	file->paletteCount = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_PALETTE_COUNT_OFFSET);
	file->nameTableSize = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_NAME_TABLE_SIZE_OFFSET);
	file->variableCount = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_VARIABLE_COUNT_OFFSET);
	file->chained = (header[TG_RES_V1_2_HEADER_CHAINED_OFFSET] != 0);
	file->variableTableCount = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_VARIABLE_TABLE_OFFSET);
	file->stringCount = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_STRING_COUNT_OFFSET);
	file->stringPoolSize = READ_LE_UINT16(header + TG_RES_V1_2_HEADER_STRING_POOL_SIZE_OFFSET);
	file->residentSplit = READ_LE_UINT32(header + TG_RES_V1_2_HEADER_RESIDENT_SPLIT_OFFSET);
	file->residentSize = READ_LE_UINT32(header + TG_RES_V1_2_HEADER_RESIDENT_SIZE_OFFSET);

	if (file->magic != TG_RES_HEADER_MAGIC)
		return false;

	if (file->entryCount == 0)
		return false;

	byte sum = 0;
	for (uint32 i = 0; i < TG_RES_V1_2_HEADER_SIZE; i++)
		sum += header[i];

	return sum == 0;
}

bool ResourceLibrary_v1_2::loadVariableTable(ResourceFile *file) {
	if (file->variableTableCount == 0)
		return true;

	// The game computes 31 * count in 16 bits
	uint32 size = (uint32)TG_RES_V1_2_VARIABLE_RECORD_SIZE * file->variableTableCount;
	file->variables = engine()->utilsLibrary()->tgMalloc(size, 0);
	if (!file->variables)
		return false;

	return file->stream->read(file->variables, size) == size;
}

bool ResourceLibrary_v1_2::loadStringPool(ResourceFile *file) {
	if (file->stringPoolSize == 0)
		return true;

	uint32 size = file->stringPoolSize;
	file->stringPool = engine()->utilsLibrary()->tgMalloc(size, 0);
	if (!file->stringPool)
		return false;

	return file->stream->read(file->stringPool, size) == size;
}

bool ResourceLibrary_v1_2::loadResidentChunks(ResourceFile *file) {
	if (file->residentSize == 0)
		return true;

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	uint32 size = file->residentSize;
	if (file->residentSplit != TG_RES_V1_2_NO_SPLIT)
		size = file->residentSplit;

	file->residentChunk1 = utilsLibrary->tgMalloc(size, 0);
	if (!file->residentChunk1)
		return false;

	if (file->stream->read(file->residentChunk1, size) != size)
		return false;

	if (file->residentSplit == TG_RES_V1_2_NO_SPLIT)
		return true;

	size = file->residentSize - file->residentSplit;
	file->residentChunk2 = utilsLibrary->tgMalloc(size, 0);
	if (!file->residentChunk2)
		return false;

	return file->stream->read(file->residentChunk2, size) == size;
}

void ResourceLibrary_v1_2::writeVariableTableBack(ResourceFile *file) {
	if (file->variableTableCount == 0 || !file->variablesDirty)
		return;

	uint32 offset = TG_RES_V1_2_HEADER_SIZE + (uint32)TG_RES_INDEX_ENTRY_SIZE * file->entryCount;
	if (!file->stream->seek(offset, SEEK_SET) || file->stream->pos() != offset)
		return;

	// Writing the table back is not supported
	warning("ResourceLibrary_v1_2::writeVariableTableBack(): the variables were changed but the file is read only");
}

bool ResourceLibrary_v1_2::readChainedBlocks(ResourceFile *file, byte *buffer, uint32 count) {
	while (count != 0) {
		uint32 chunk = (uint16)(TG_RES_V1_2_CHAIN_DATA_SIZE - file->chainOffset);
		if (chunk > count)
			chunk = count;

		if (file->stream->read(buffer, chunk) != chunk)
			return false;

		count -= chunk;
		buffer += chunk;
		file->chainOffset += chunk;

		if (file->chainOffset >= TG_RES_V1_2_CHAIN_DATA_SIZE) {
			// The chunk size stays in place if the link cannot be read
			byte link[TG_RES_V1_2_CHAIN_LINK_SIZE];
			WRITE_LE_UINT32(link, chunk);
			file->stream->read(link, TG_RES_V1_2_CHAIN_LINK_SIZE);
			file->stream->seek(READ_LE_UINT32(link), SEEK_SET);
			file->chainOffset = 0;
		}
	}

	return true;
}

bool ResourceLibrary_v1_2::readResourceBytes(ResourceFile *file, byte *buffer, uint32 count) {
	if (file->chained)
		return readChainedBlocks(file, buffer, count);

	return file->stream->read(buffer, count) == count;
}

bool ResourceLibrary_v1_2::readChunkedWithCallback(ResourceFile *file, byte *buffer, uint32 count) {
	if (count == 0)
		return true;

	if (file->callbackInterval == 0) {
		warning("ResourceLibrary_v1_2::readChunkedWithCallback(): the callback interval is zero");
		return false;
	}

	do {
		uint32 chunk = ((int32)count <= file->callbackInterval) ? count : (uint32)file->callbackInterval;
		if (!readResourceBytes(file, buffer, chunk))
			return false;

		buffer += chunk;
		file->callbackCounter += chunk;
		count -= chunk;

		if (count != 0 && (int32)file->callbackCounter >= file->callbackInterval) {
			uint32 savedCounter = file->callbackCounter;
			int32 savedInterval = file->callbackInterval;
			uint16 savedChainOffset = file->chainOffset;
			int64 position = file->stream->pos();

			engine()->resourceLoadProgressCallback();

			file->callbackInterval = savedInterval;
			file->callbackCounter = savedCounter - savedInterval;
			file->chainOffset = savedChainOffset;
			file->stream->seek(position, SEEK_SET);
		}
	} while (count != 0);

	return true;
}

uint16 ResourceLibrary_v1_2::resOpenFile(const Common::String &path) {
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

	record->lzwState = lzwCreateStateFar();
	if (!record->lzwState) {
		resCloseFile(file);
		return 0;
	}

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	// The game computes 10 * count in 16 bits, which overflows with 6554 or more entries
	uint32 indexSize = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount;
	if (indexSize > 0xFFFF)
		warning("ResourceLibrary_v1_2::resOpenFile(): too many entries (%d)", record->entryCount);

	record->index = utilsLibrary->tgMalloc(indexSize, 0);
	if (!record->index || record->stream->read(record->index, indexSize) != indexSize) {
		resCloseFile(file);
		return 0;
	}

	if (!loadVariableTable(record) || !loadStringPool(record) || !loadResidentChunks(record)) {
		resCloseFile(file);
		return 0;
	}

	return file;
}

uint16 ResourceLibrary_v1_2::resCloseFile(uint16 file) {
	ResourceFile *record = findFile(file, "resCloseFile");
	if (!record)
		return 0;

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	writeVariableTableBack(record);

	if (record->lzwState)
		lzwFreeStateFar(record->lzwState);

	delete record->stream;
	utilsLibrary->tgFree(record->index);
	utilsLibrary->tgFree(record->residentChunk1);
	utilsLibrary->tgFree(record->residentChunk2);
	utilsLibrary->tgFree(record->variables);
	utilsLibrary->tgFree(record->stringPool);
	delete record;
	_files.erase(file);
	return 0;
}

byte *ResourceLibrary_v1_2::resLoadNameTable(uint16 file, uint32 *outSize) {
	ResourceFile *record = findFile(file, "resLoadNameTable");
	if (!record)
		return nullptr;

	if (record->nameTableSize == 0)
		return nullptr;

	uint32 offset = TG_RES_V1_2_HEADER_SIZE + (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount + (uint16)(record->paletteCount * 4)
		+ (uint16)(TG_RES_V1_2_VARIABLE_RECORD_SIZE * record->variableTableCount) + record->stringPoolSize + record->residentSize;
	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return nullptr;

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	uint32 size = record->nameTableSize;
	byte *buffer = utilsLibrary->tgMalloc(size, 0);
	if (!buffer)
		return nullptr;

	if (record->stream->read(buffer, size) != size)
		return utilsLibrary->tgFree(buffer);

	if (outSize)
		*outSize = size;

	return buffer;
}

uint16 ResourceLibrary_v1_2::resLoadPalette(uint16 file, byte *buffer) {
	ResourceFile *record = findFile(file, "resLoadPalette");
	if (!record)
		return 0;

	if (!buffer)
		return 0;

	if (record->paletteCount == 0)
		return 0;

	uint32 offset = TG_RES_V1_2_HEADER_SIZE + (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount
		+ (uint16)(TG_RES_V1_2_VARIABLE_RECORD_SIZE * record->variableTableCount) + record->stringPoolSize + record->residentSize;
	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return 0;

	uint16 byteCount = (uint16)(record->paletteCount * 4);
	if (record->stream->read(buffer, byteCount) != byteCount)
		return 0;

	return record->paletteCount;
}

byte *ResourceLibrary_v1_2::resLoadResource(uint16 file, uint16 index, uint32 *outSize) {
	ResourceFile *record = findFile(file, "resLoadResource");
	if (!record)
		return nullptr;

	if (record->entryCount <= index)
		return nullptr;

	ResourceEntry entry;
	readEntry(record, index, entry);

	if (entry.type >= TG_RES_V1_2_RESIDENT_FIRST_TYPE && entry.type <= TG_RES_V1_2_RESIDENT_LAST_TYPE && entry.type != TG_RES_V1_2_DISK_TYPE_IN_RESIDENT_RANGE) {
		if (outSize)
			*outSize = entry.size;

		if (entry.offset < record->residentSplit) {
			if (!record->residentChunk1) {
				warning("ResourceLibrary_v1_2::resLoadResource(): the file has no resident block");
				return nullptr;
			}

			return record->residentChunk1 + (uint16)entry.offset;
		}

		if (!record->residentChunk2) {
			warning("ResourceLibrary_v1_2::resLoadResource(): the file has no second resident block");
			return nullptr;
		}

		return record->residentChunk2 + (uint16)(entry.offset - record->residentSplit);
	}

	if (!record->stream->seek(entry.offset, SEEK_SET) || record->stream->pos() != entry.offset)
		return nullptr;

	record->chainOffset = 0;

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	byte *buffer = utilsLibrary->tgMalloc(entry.size, 0);
	if (!buffer)
		return nullptr;

	bool success;
	if (record->callbackSet)
		success = readChunkedWithCallback(record, buffer, entry.size);
	else
		success = readResourceBytes(record, buffer, entry.size);

	if (!success)
		return utilsLibrary->tgFree(buffer);

	if (entry.type == TG_RES_BITMAP_TYPE) {
		byte *bitmap = expandCelBitmapRecord(record, buffer, entry.size, outSize);
		utilsLibrary->tgFree(buffer);
		return bitmap;
	}

	if (outSize)
		*outSize = entry.size;

	return buffer;
}

uint32 ResourceLibrary_v1_2::resSeekResource(uint16 file, uint16 index) {
	ResourceFile *record = findFile(file, "resSeekResource");
	if (!record)
		return 0;

	if (record->entryCount <= index)
		return 0;

	ResourceEntry entry;
	readEntry(record, index, entry);

	if (!record->stream->seek(entry.offset, SEEK_SET) || record->stream->pos() != entry.offset)
		return 0;

	record->chainOffset = 0;
	return entry.size;
}

uint32 ResourceLibrary_v1_2::resReadData(uint16 file, byte *buffer, uint32 count, uint32 offset) {
	ResourceFile *record = findFile(file, "resReadData");
	if (!record)
		return 0;

	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return 0;

	record->chainOffset = 0;

	bool success;
	if (record->callbackSet)
		success = readChunkedWithCallback(record, buffer, count);
	else
		success = readResourceBytes(record, buffer, count);

	return success ? count : 0;
}

byte *ResourceLibrary_v1_2::expandCelBitmapRecord(ResourceFile *file, byte *source, uint32 sourceSize, uint32 *outSize) {
	if (sourceSize < TG_RES_V1_2_BITMAP_HEADER_SIZE) {
		warning("ResourceLibrary_v1_2::expandCelBitmapRecord(): bitmap record is too short");
		return nullptr;
	}

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	uint32 headerLength = READ_LE_UINT16(source);
	uint32 size = (uint32)READ_LE_UINT16(source + TG_RES_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(source + TG_RES_V1_2_BITMAP_HEIGHT_OFFSET) + headerLength;
	byte *destination = utilsLibrary->tgMalloc(size, 0);
	if (!destination)
		return nullptr;

	if (!file->lzwState) {
		decodeRleCelBitmap(source, sourceSize, destination, size);
	} else {
		if (size < TG_RES_V1_2_BITMAP_HEADER_SIZE) {
			warning("ResourceLibrary_v1_2::expandCelBitmapRecord(): bitmap size is smaller than its header");
			return utilsLibrary->tgFree(destination);
		}

		memcpy(destination, source, TG_RES_V1_2_BITMAP_HEADER_SIZE);

		const byte *sourceEnd = (sourceSize == TG_RES_V1_2_SIZE_UNKNOWN) ? nullptr : source + sourceSize;
		DecodeLimits limits(sourceEnd, destination + size);
		if (!limits.sourceAvailable(source, headerLength)) {
			warning("ResourceLibrary_v1_2::expandCelBitmapRecord(): bitmap header overruns its buffer");
			return utilsLibrary->tgFree(destination);
		}

		if (headerLength > size) {
			warning("ResourceLibrary_v1_2::expandCelBitmapRecord(): bitmap header overruns its destination");
			return utilsLibrary->tgFree(destination);
		}

		decodeSegmentedBitmapStream(file->lzwState, source + headerLength, destination + headerLength, limits);
	}

	if (outSize)
		*outSize = size;

	// The pixel pointer field at +0x10 is not filled
	return destination;
}

byte *ResourceLibrary_v1_2::resExpandBitmap(uint16 file, byte *bitmap) {
	ResourceFile *record = findFile(file, "resExpandBitmap");
	if (!record)
		return nullptr;

	return expandCelBitmapRecord(record, bitmap, TG_RES_V1_2_SIZE_UNKNOWN, nullptr);
}

bool ResourceLibrary_v1_2::extractCompressedResourceToFile(uint16 file, ResourceFile *record, Common::WriteStream *output, uint32 compressedSize, uint32 rawSize) {
	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	byte *compressed = utilsLibrary->tgMalloc(compressedSize, 0);
	if (!compressed)
		return false;

	bool result = false;

	byte *raw = utilsLibrary->tgMalloc(rawSize, 0);
	if (raw) {
		if (resRead(file, compressed, compressedSize) == compressedSize) {
			DecodeLimits limits(compressed + compressedSize, raw + rawSize);
			uint32 written = decodeSegmentedBitmapStream(record->lzwState, compressed, raw, limits);
			if (written != 0 && output->write(raw, rawSize) == rawSize)
				result = true;
		}

		utilsLibrary->tgFree(raw);
	}

	utilsLibrary->tgFree(compressed);
	return result;
}

bool ResourceLibrary_v1_2::resExtractFile(uint16 file, uint16 index, Common::WriteStream *output) {
	if (resSeekResource(file, index) == 0xFFFFFFFF)
		return false;

	ResourceFile *record = findFile(file, "resExtractFile");
	if (!record)
		return false;

	byte prefix[TG_RES_V1_2_EXTRACT_PREFIX_SIZE];
	if (resRead(file, prefix, TG_RES_V1_2_EXTRACT_PREFIX_SIZE) != TG_RES_V1_2_EXTRACT_PREFIX_SIZE)
		return false;

	uint32 rawSize = READ_LE_UINT32(prefix);
	uint32 remaining = resGetSize(file, index) - TG_RES_V1_2_EXTRACT_PREFIX_SIZE;

	if (prefix[4] != 0)
		return extractCompressedResourceToFile(file, record, output, remaining, rawSize);

	uint32 offset = resSeek(file, 0, SEEK_CUR);

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();
	byte *buffer = utilsLibrary->tgMalloc(TG_RES_V1_2_EXTRACT_BLOCK_SIZE, 0);
	if (!buffer)
		return false;

	bool success = true;

	while ((int32)remaining > 0 && success) {
		uint32 chunk = ((int32)remaining <= TG_RES_V1_2_EXTRACT_BLOCK_SIZE) ? remaining : TG_RES_V1_2_EXTRACT_BLOCK_SIZE;

		success = (resReadData(file, buffer, chunk, offset) == chunk);
		if (success) {
			if (record->callbackSet)
				engine()->resourceLoadProgressCallback();

			success = (output->write(buffer, chunk) == chunk);

			if (record->callbackSet)
				engine()->resourceLoadProgressCallback();

			offset += chunk;
			remaining -= chunk;
		}
	}

	utilsLibrary->tgFree(buffer);
	return success;
}

uint16 ResourceLibrary_v1_2::resEnumResources(uint16 file) {
	ResourceFile *record = findFile(file, "resEnumResources");
	if (!record)
		return 0;

	for (uint16 i = 0; i < record->entryCount; i++) {
		ResourceEntry entry;
		readEntry(record, i, entry);

		if (!engine()->resourceEnumCallback(entry.type, entry.offset, entry.size))
			break;
	}

	return 1;
}

uint16 ResourceLibrary_v1_2::resEnumVariables(uint16 file) {
	ResourceFile *record = findFile(file, "resEnumVariables");
	if (!record)
		return 0;

	for (uint16 i = 0; i < record->variableTableCount; i++) {
		const byte *variable = record->variables + (uint32)TG_RES_V1_2_VARIABLE_RECORD_SIZE * i;
		if (variable[TG_RES_V1_2_VARIABLE_USED_OFFSET] == 0)
			continue;

		if (!engine()->resourceVariableCallback(READ_LE_UINT16(variable), READ_LE_UINT16(variable + TG_RES_V1_2_VARIABLE_VALUE_OFFSET),
				(const char *)variable + TG_RES_V1_2_VARIABLE_NAME_OFFSET))
			break;
	}

	return 1;
}

bool ResourceLibrary_v1_2::resSetVariable(uint16 file, const char *name, uint16 value) {
	ResourceFile *record = findFile(file, "resSetVariable");
	if (!record)
		return false;

	if (record->variableTableCount == 0)
		return false;

	if (!name || name[0] == 0)
		return false;

	size_t nameLength = strlen(name);
	if (nameLength >= TG_RES_V1_2_VARIABLE_NAME_SIZE)
		return false;

	for (uint16 i = 0; i < record->variableTableCount; i++) {
		byte *variable = record->variables + (uint32)TG_RES_V1_2_VARIABLE_RECORD_SIZE * i;
		if (variable[TG_RES_V1_2_VARIABLE_USED_OFFSET] == 0)
			continue;

		// The terminator is part of the comparison
		if (memcmp(variable + TG_RES_V1_2_VARIABLE_NAME_OFFSET, name, nameLength + 1) == 0) {
			WRITE_LE_UINT16(variable + TG_RES_V1_2_VARIABLE_VALUE_OFFSET, value);
			record->variablesDirty = true;
			return true;
		}
	}

	return false;
}

uint16 ResourceLibrary_v1_2::resGetIndex(uint16 file, const char *name) {
	ResourceFile *record = findFile(file, "resGetIndex");
	if (!record)
		return 0xFFFF;

	if (record->variableTableCount == 0)
		return 0xFFFF;

	if (!name)
		return 0xFFFF;

	size_t nameLength = strlen(name);
	if (nameLength == 0)
		return 0;

	if (nameLength >= TG_RES_V1_2_VARIABLE_NAME_SIZE)
		return 0xFFFF;

	for (uint16 i = 0; i < record->variableTableCount; i++) {
		const byte *variable = record->variables + (uint32)TG_RES_V1_2_VARIABLE_RECORD_SIZE * i;

		// Only the records that are not in use are looked at
		if (variable[TG_RES_V1_2_VARIABLE_USED_OFFSET] != 0)
			continue;

		if (memcmp(variable + TG_RES_V1_2_VARIABLE_NAME_OFFSET, name, nameLength + 1) == 0)
			return READ_LE_UINT16(variable);
	}

	return 0xFFFF;
}

bool ResourceLibrary_v1_2::resGetVariable(uint16 file, const char *name, uint16 *value) {
	ResourceFile *record = findFile(file, "resGetVariable");
	if (!record)
		return false;

	if (record->variableTableCount == 0)
		return false;

	if (!name || !value)
		return false;

	size_t nameLength = strlen(name);
	if (nameLength == 0 || nameLength >= TG_RES_V1_2_VARIABLE_NAME_SIZE)
		return false;

	for (uint16 i = 0; i < record->variableTableCount; i++) {
		const byte *variable = record->variables + (uint32)TG_RES_V1_2_VARIABLE_RECORD_SIZE * i;
		if (variable[TG_RES_V1_2_VARIABLE_USED_OFFSET] == 0)
			continue;

		if (memcmp(variable + TG_RES_V1_2_VARIABLE_NAME_OFFSET, name, nameLength + 1) == 0) {
			*value = READ_LE_UINT16(variable + TG_RES_V1_2_VARIABLE_VALUE_OFFSET);
			return true;
		}
	}

	return false;
}

uint16 ResourceLibrary_v1_2::resGetTitle(uint16 file, byte *destination, uint16 count) {
	ResourceFile *record = findFile(file, "resGetTitle");
	if (!record)
		return 0;

	if (count > TG_RES_V1_2_HEADER_TITLE_MAX_SIZE)
		count = TG_RES_V1_2_HEADER_TITLE_MAX_SIZE;

	if (count != 0)
		memmove(destination, record->header + TG_RES_V1_2_HEADER_TITLE_OFFSET, count);

	return count;
}

const char *ResourceLibrary_v1_2::resGetConstStr(uint16 file, uint16 offset) {
	ResourceFile *record = findFile(file, "resGetConstStr");
	if (!record)
		return nullptr;

	if (!record->stringPool)
		return nullptr;

	if (record->stringPoolSize <= offset)
		return nullptr;

	return (const char *)record->stringPool + offset;
}

uint16 ResourceLibrary_v1_2::resCountStrings(uint16 file) {
	ResourceFile *record = findFile(file, "resCountStrings");
	if (!record)
		return 0;

	return record->stringCount;
}

uint16 ResourceLibrary_v1_2::registerWaveInfo(WaveStreamInfo *info) {
	for (uint32 attempt = 0; attempt < 0xFFFF; attempt++) {
		uint16 handle = _nextWaveStreamHandle;
		_nextWaveStreamHandle = (_nextWaveStreamHandle == 0xFFFF) ? 1 : _nextWaveStreamHandle + 1;

		if (!_waveStreams.contains(handle)) {
			_waveStreams[handle] = info;
			return handle;
		}
	}

	return 0;
}

uint16 ResourceLibrary_v1_2::createWaveStreamInfo(uint16 file, ResourceFile *record) {
	WaveStreamInfo *info = new WaveStreamInfo();
	info->file = file;
	info->position = (uint32)record->stream->pos();
	info->startPosition = info->position;
	info->chainOffset = record->chainOffset;
	info->startChainOffset = record->chainOffset;

	uint16 waveInfo = registerWaveInfo(info);
	if (waveInfo == 0)
		delete info;

	return waveInfo;
}

uint16 ResourceLibrary_v1_2::resCreateWaveEvent(uint16 file, uint16 index) {
	if (resSeekResource(file, index) == 0)
		return 0;

	ResourceFile *record = findFile(file, "resCreateWaveEvent");
	if (!record)
		return 0;

	byte header[TG_RES_V1_2_WAVE_HEADER_SIZE];
	if (!readResourceBytes(record, header, TG_RES_V1_2_WAVE_HEADER_SIZE))
		return 0;

	uint16 waveInfo = createWaveStreamInfo(file, record);
	if (waveInfo == 0)
		return 0;

	return engine()->audioCreateWave(header, waveInfo);
}

int32 ResourceLibrary_v1_2::waveRead(uint16 waveInfo, byte *buffer, int32 count) {
	if (!_waveStreams.contains(waveInfo)) {
		warning("ResourceLibrary_v1_2::waveRead(): invalid wave handle %d", waveInfo);
		return 0;
	}

	WaveStreamInfo *info = _waveStreams[waveInfo];
	ResourceFile *record = findFile(info->file, "waveRead");
	if (!record)
		return 0;

	bool callbackSet = record->callbackSet;
	record->callbackSet = false;

	uint32 result = resReadData(info->file, buffer, (uint32)count, info->position);

	record->callbackSet = callbackSet;

	info->position += result;
	return (int32)result;
}

uint32 ResourceLibrary_v1_2::waveRewind(uint16 waveInfo) {
	if (!_waveStreams.contains(waveInfo)) {
		warning("ResourceLibrary_v1_2::waveRewind(): invalid wave handle %d", waveInfo);
		return 0;
	}

	WaveStreamInfo *info = _waveStreams[waveInfo];
	info->position = info->startPosition;
	info->chainOffset = info->startChainOffset;
	return info->position;
}

void ResourceLibrary_v1_2::waveDone(uint16 waveInfo) {
	if (!_waveStreams.contains(waveInfo)) {
		warning("ResourceLibrary_v1_2::waveDone(): invalid wave handle %d", waveInfo);
		return;
	}

	delete _waveStreams[waveInfo];
	_waveStreams.erase(waveInfo);
}

byte *ResourceLibrary_v1_2::swinFree(byte *block) {
	if (block)
		engine()->utilsLibrary()->tgFree(block);

	return nullptr;
}

byte *ResourceLibrary_v1_2::swinMalloc(uint32 size, uint16 zeroInit) {
	if (size == 0)
		return nullptr;

	return engine()->utilsLibrary()->tgMalloc(size, zeroInit);
}

byte *ResourceLibrary_v1_2::swinRealloc(byte *block, uint32 size, uint16 zeroInit) {
	// This allocation is always zero filled
	if (!block)
		return swinMalloc(size, 1);

	return engine()->utilsLibrary()->tgRealloc(block, size, zeroInit);
}

ResourceLibrary_v1_2::LzwState *ResourceLibrary_v1_2::lzwCreateStateFar() {
	return lzwAllocState();
}

void ResourceLibrary_v1_2::lzwFreeStateFar(LzwState *state) {
	lzwFreeState(state);
}

ResourceLibrary_v1_2::LzwState *ResourceLibrary_v1_2::lzwAllocState() {
	LzwState *state = new LzwState();
	state->tableSize = TG_RES_V1_2_LZW_TABLE_SIZE;
	state->dictionary = swinMalloc((uint32)TG_RES_V1_2_LZW_TABLE_SIZE * TG_RES_V1_2_LZW_ALLOCATION_FACTOR, 0);
	if (!state->dictionary) {
		lzwFreeState(state);
		return nullptr;
	}

	state->prefix = (uint16 *)state->dictionary;
	state->suffix = state->dictionary + 2 * (uint32)state->tableSize;
	state->length = state->suffix + state->tableSize;
	return state;
}

void ResourceLibrary_v1_2::lzwFreeState(LzwState *state) {
	if (state->dictionary)
		swinFree(state->dictionary);

	delete state;
}

} // End of namespace Topgun
