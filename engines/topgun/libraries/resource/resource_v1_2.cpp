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

#include "topgun/libraries/resource/resource_v1_2_3.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

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
	bool hasRecords = (_engine->_version >= kVersionV1_2_1);
	bool hasPositionTable = (_engine->_version >= kVersionV1_2_2);
	uint16 headerSize = hasRecords ? TG_RES_V1_2_1_HEADER_SIZE : TG_RES_V1_2_HEADER_SIZE;
	if (hasPositionTable)
		headerSize = TG_RES_V1_2_2_HEADER_SIZE;

	if (file->stream->read(file->header, headerSize) != headerSize)
		return false;

	const byte *header = file->header;
	file->magic = READ_LE_UINT16(header);

	if (hasRecords) {
		file->recordCount = READ_LE_UINT16(header + TG_RES_V1_2_1_HEADER_RECORD_COUNT_OFFSET);
		file->maxFadeColors = READ_LE_UINT16(header + TG_RES_V1_2_1_HEADER_MAX_FADE_OFFSET);
		file->maxTransparentColors = READ_LE_UINT16(header + TG_RES_V1_2_1_HEADER_MAX_TRANS_OFFSET);
	}

	file->entryCount = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_INDEX_COUNT_OFFSET : TG_RES_V1_2_HEADER_RESOURCE_COUNT_OFFSET));
	file->paletteCount = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_PALETTE_COUNT_OFFSET : TG_RES_V1_2_HEADER_PALETTE_COUNT_OFFSET));
	file->nameTableSize = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_NAME_TABLE_SIZE_OFFSET : TG_RES_V1_2_HEADER_NAME_TABLE_SIZE_OFFSET));
	file->variableCount = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_VARIABLE_COUNT_OFFSET : TG_RES_V1_2_HEADER_VARIABLE_COUNT_OFFSET));
	file->chained = (header[hasRecords ? TG_RES_V1_2_1_HEADER_CHAINED_OFFSET : TG_RES_V1_2_HEADER_CHAINED_OFFSET] != 0);
	file->variableTableCount = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_VARIABLE_TABLE_OFFSET : TG_RES_V1_2_HEADER_VARIABLE_TABLE_OFFSET));

	if (hasPositionTable) {
		file->variablePairCount = READ_LE_UINT16(header + TG_RES_V1_2_2_HEADER_VARIABLE_PAIR_COUNT_OFFSET);
		file->stringCount = READ_LE_UINT16(header + TG_RES_V1_2_2_HEADER_STRING_COUNT_OFFSET);
		file->stringPoolSize = READ_LE_UINT16(header + TG_RES_V1_2_2_HEADER_STRING_POOL_SIZE_OFFSET);
		file->residentSplit = READ_LE_UINT32(header + TG_RES_V1_2_2_HEADER_RESIDENT_SPLIT_OFFSET);
		file->residentSize = READ_LE_UINT32(header + TG_RES_V1_2_2_HEADER_RESIDENT_SIZE_OFFSET);
	} else {
		file->stringCount = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_STRING_COUNT_OFFSET : TG_RES_V1_2_HEADER_STRING_COUNT_OFFSET));
		file->stringPoolSize = READ_LE_UINT16(header + (hasRecords ? TG_RES_V1_2_1_HEADER_STRING_POOL_SIZE_OFFSET : TG_RES_V1_2_HEADER_STRING_POOL_SIZE_OFFSET));
		file->residentSplit = READ_LE_UINT32(header + (hasRecords ? TG_RES_V1_2_1_HEADER_RESIDENT_SPLIT_OFFSET : TG_RES_V1_2_HEADER_RESIDENT_SPLIT_OFFSET));
		file->residentSize = READ_LE_UINT32(header + (hasRecords ? TG_RES_V1_2_1_HEADER_RESIDENT_SIZE_OFFSET : TG_RES_V1_2_HEADER_RESIDENT_SIZE_OFFSET));
	}

	if (file->magic != TG_RES_HEADER_MAGIC)
		return false;

	if (file->entryCount == 0)
		return false;

	byte sum = 0;
	for (uint32 i = 0; i < headerSize; i++)
		sum += header[i];

	return sum == 0;
}

bool ResourceLibrary_v1_2::loadVariableTable(ResourceFile *file) {
	if (file->variableTableCount == 0)
		return true;

	// The game computes the record size * count in 16 bits
	uint32 recordSize = TG_RES_V1_2_VARIABLE_RECORD_SIZE;
	if (_engine->_version >= kVersionV1_2_2)
		recordSize = TG_RES_V1_2_2_VARIABLE_RECORD_SIZE;

	uint32 size = recordSize * file->variableTableCount;
	file->variables = engine()->utilsLibrary()->tgMalloc(size, 0);
	if (!file->variables)
		return false;

	if (_engine->_version >= kVersionV1_2_2 && !file->stream->seek(READ_LE_UINT32(file->header + TG_RES_V1_2_2_HEADER_VARIABLE_TABLE_POSITION_OFFSET), SEEK_SET))
		return false;

	return file->stream->read(file->variables, size) == size;
}

bool ResourceLibrary_v1_2::loadVariablePairs(ResourceFile *file) {
	if (file->variablePairCount == 0)
		return true;

	// The game computes 4 * count in 16 bits
	uint32 size = (uint16)(TG_RES_V1_2_2_VARIABLE_PAIR_SIZE * file->variablePairCount);
	file->variablePairs = engine()->utilsLibrary()->tgMalloc(size, 0);
	if (!file->variablePairs)
		return false;

	if (!file->stream->seek(READ_LE_UINT32(file->header + TG_RES_V1_2_2_HEADER_VARIABLE_PAIRS_POSITION_OFFSET), SEEK_SET))
		return false;

	return file->stream->read(file->variablePairs, size) == size;
}

bool ResourceLibrary_v1_2::loadStringPool(ResourceFile *file) {
	if (file->stringPoolSize == 0)
		return true;

	uint32 size = file->stringPoolSize;
	file->stringPool = engine()->utilsLibrary()->tgMalloc(size, 0);
	if (!file->stringPool)
		return false;

	if (_engine->_version >= kVersionV1_2_2 && !file->stream->seek(READ_LE_UINT32(file->header + TG_RES_V1_2_2_HEADER_STRING_POOL_POSITION_OFFSET), SEEK_SET))
		return false;

	return file->stream->read(file->stringPool, size) == size;
}

bool ResourceLibrary_v1_2::loadResidentChunks(ResourceFile *file) {
	if (file->residentSize == 0)
		return true;

	if (_engine->_version == kVersionV1_2_2 && !file->stream->seek(READ_LE_UINT32(file->header + TG_RES_V1_2_2_HEADER_RESIDENT_POSITION_OFFSET), SEEK_SET))
		return false;

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

bool ResourceLibrary_v1_2::loadMovieBuffer(uint16 file, ResourceFile *record) {
	if (_engine->_version >= kVersionV1_2_2_1) {
		ResourceLibrary_v1_2_3 *newer = static_cast<ResourceLibrary_v1_2_3 *>(this);
		if (newer->keySize(record, TG_RES_V1_2_2_1_KEY_MOVIE_BUFFER) == 0)
			return true;

		record->movieBuffer = newer->resLoadKeyResource(file, TG_RES_V1_2_2_1_KEY_MOVIE_BUFFER, nullptr);
		return record->movieBuffer != nullptr;
	}

	uint32 size = READ_LE_UINT32(record->header + TG_RES_V1_2_2_HEADER_MOVIE_BUFFER_SIZE_OFFSET);
	if (size == 0)
		return true;

	if (!record->stream->seek(READ_LE_UINT32(record->header + TG_RES_V1_2_2_HEADER_MOVIE_BUFFER_POSITION_OFFSET), SEEK_SET))
		return false;

	record->movieBuffer = engine()->utilsLibrary()->tgMalloc(size, 0);
	if (!record->movieBuffer)
		return false;

	return record->stream->read(record->movieBuffer, size) == size;
}

void ResourceLibrary_v1_2::writeVariableTableBack(ResourceFile *file) {
	if (file->variableTableCount == 0 || !file->variablesDirty)
		return;

	// The game does not add the size of the 0x50 byte records
	uint32 headerSize = (_engine->_version >= kVersionV1_2_1) ? TG_RES_V1_2_1_HEADER_SIZE : TG_RES_V1_2_HEADER_SIZE;
	uint32 offset = headerSize + (uint32)TG_RES_INDEX_ENTRY_SIZE * file->entryCount;
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

	if (_engine->_version >= kVersionV1_2_2_1) {
		ResourceLibrary_v1_2_3 *newer = static_cast<ResourceLibrary_v1_2_3 *>(this);

		if (!newer->loadIndexPages(record) || !newer->loadKey3Table(file, record) || !newer->loadConstStringPool(file, record)
				|| !newer->loadResidentChunks(record)) {
			resCloseFile(file);
			return 0;
		}

		if (_engine->_version < kVersionV1_2_2_2 && !loadMovieBuffer(file, record)) {
			resCloseFile(file);
			return 0;
		}

		return file;
	}

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	if (_engine->_version >= kVersionV1_2_1) {
		// The game computes 0x50 * count in 16 bits; the records are skipped
		uint32 recordsSize = (uint16)(TG_RES_V1_2_1_RECORD_SIZE * record->recordCount);
		if (_engine->_version >= kVersionV1_2_2 && recordsSize != 0
				&& !record->stream->seek(READ_LE_UINT32(record->header + TG_RES_V1_2_2_HEADER_RECORDS_POSITION_OFFSET), SEEK_SET)) {
			resCloseFile(file);
			return 0;
		}

		int64 position = record->stream->pos();
		if (recordsSize != 0 && (position + recordsSize > record->stream->size() || !record->stream->seek(recordsSize, SEEK_CUR))) {
			resCloseFile(file);
			return 0;
		}
	}

	// The game computes 10 * count in 16 bits, which overflows with 6554 or more entries
	uint32 indexSize = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount;
	if (indexSize > 0xFFFF)
		warning("ResourceLibrary_v1_2::resOpenFile(): too many entries (%d)", record->entryCount);

	record->index = utilsLibrary->tgMalloc(indexSize, 0);
	if (!record->index) {
		resCloseFile(file);
		return 0;
	}

	if (_engine->_version >= kVersionV1_2_2 && !record->stream->seek(READ_LE_UINT32(record->header + TG_RES_V1_2_2_HEADER_INDEX_POSITION_OFFSET), SEEK_SET)) {
		resCloseFile(file);
		return 0;
	}

	if (record->stream->read(record->index, indexSize) != indexSize) {
		resCloseFile(file);
		return 0;
	}

	if (!loadVariableTable(record) || !loadStringPool(record) || !loadResidentChunks(record)) {
		resCloseFile(file);
		return 0;
	}

	if (_engine->_version >= kVersionV1_2_2 && (!loadVariablePairs(record) || !loadMovieBuffer(file, record))) {
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
	utilsLibrary->tgFree(record->movieBuffer);
	utilsLibrary->tgFree(record->variables);

	uint16 maxChunks = TG_RES_V1_2_3_MAX_RESIDENT_CHUNKS;
	if (_engine->_version >= kVersionV1_2_10)
		maxChunks = TG_RES_V1_2_10_MAX_RESIDENT_CHUNKS;
	else if (_engine->_version >= kVersionV1_2_5)
		maxChunks = TG_RES_V1_2_5_MAX_RESIDENT_CHUNKS;

	for (uint16 i = 0; i < maxChunks; i++)
		utilsLibrary->tgFree(record->residentChunks[i]);

	for (Common::HashMap<uint16, byte *>::iterator i = record->dynamicResident.begin(); i != record->dynamicResident.end(); ++i)
		utilsLibrary->tgFree(i->_value);

	utilsLibrary->tgFree(record->variablePairs);
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

	uint32 offset;
	if (_engine->_version >= kVersionV1_2_2) {
		offset = READ_LE_UINT32(record->header + TG_RES_V1_2_2_HEADER_NAME_TABLE_POSITION_OFFSET);
	} else {
		offset = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount + (uint16)(record->paletteCount * 4)
			+ (uint16)(TG_RES_V1_2_VARIABLE_RECORD_SIZE * record->variableTableCount) + record->stringPoolSize + record->residentSize;
		if (_engine->_version >= kVersionV1_2_1)
			offset += TG_RES_V1_2_1_HEADER_SIZE + (uint16)(TG_RES_V1_2_1_RECORD_SIZE * record->recordCount);
		else
			offset += TG_RES_V1_2_HEADER_SIZE;
	}

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

	uint32 offset;
	if (_engine->_version >= kVersionV1_2_2) {
		offset = READ_LE_UINT32(record->header + TG_RES_V1_2_2_HEADER_PALETTE_POSITION_OFFSET);
	} else {
		offset = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount
			+ (uint16)(TG_RES_V1_2_VARIABLE_RECORD_SIZE * record->variableTableCount) + record->stringPoolSize + record->residentSize;
		if (_engine->_version >= kVersionV1_2_1)
			offset += TG_RES_V1_2_1_HEADER_SIZE + (uint16)(TG_RES_V1_2_1_RECORD_SIZE * record->recordCount);
		else
			offset += TG_RES_V1_2_HEADER_SIZE;
	}

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

	bool wideResidentRange = (_engine->_version >= kVersionV1_2_1);
	uint16 firstResidentType = TG_RES_V1_2_RESIDENT_FIRST_TYPE;
	uint16 lastResidentType = wideResidentRange ? TG_RES_V1_2_1_RESIDENT_LAST_TYPE : TG_RES_V1_2_RESIDENT_LAST_TYPE;
	if (_engine->_version >= kVersionV1_2_2) {
		firstResidentType = TG_RES_V1_2_2_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_2_RESIDENT_LAST_TYPE;
	}

	uint16 movieBufferType = TG_RES_V1_2_2_MOVIE_BUFFER_TYPE;
	if (_engine->_version >= kVersionV1_2_2_1) {
		firstResidentType = TG_RES_V1_2_2_1_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_2_1_RESIDENT_LAST_TYPE;
		movieBufferType = TG_RES_V1_2_2_1_MOVIE_BUFFER_TYPE;
	}

	if (entry.type >= firstResidentType && entry.type <= lastResidentType && (wideResidentRange || entry.type != TG_RES_V1_2_DISK_TYPE_IN_RESIDENT_RANGE)) {
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

	if (_engine->_version >= kVersionV1_2_2 && entry.type == movieBufferType) {
		if (outSize)
			*outSize = entry.size;

		if (!record->movieBuffer) {
			warning("ResourceLibrary_v1_2::resLoadResource(): the file has no movie buffer");
			return nullptr;
		}

		return record->movieBuffer + (uint16)entry.offset;
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

	uint16 lastDiskType = TG_RES_V1_2_2_2_DISK_LAST_TYPE;
	if (_engine->_version >= kVersionV1_2_10)
		lastDiskType = TG_RES_V1_2_10_DISK_LAST_TYPE;
	else if (_engine->_version >= kVersionV1_2_5)
		lastDiskType = TG_RES_V1_2_5_DISK_LAST_TYPE;

	if (_engine->_version >= kVersionV1_2_2_2 && (entry.type < TG_RES_V1_2_2_2_DISK_FIRST_TYPE || entry.type > lastDiskType))
		return 0;

	if (!record->stream->seek(entry.offset, SEEK_SET) || record->stream->pos() != entry.offset)
		return 0;

	record->chainOffset = 0;
	return entry.size;
}

uint32 ResourceLibrary_v1_2::resTell(uint16 file) {
	ResourceFile *record = findFile(file, "resTell");
	if (!record)
		return 0xFFFFFFFF;

	return (uint32)record->stream->pos();
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
	uint32 pixelSize = (uint32)READ_LE_UINT16(source + TG_RES_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(source + TG_RES_V1_2_BITMAP_HEIGHT_OFFSET);
	uint32 size = pixelSize + headerLength;
	byte *destination = utilsLibrary->tgMalloc(size, 0);
	if (!destination)
		return nullptr;

	if (size < TG_RES_V1_2_BITMAP_HEADER_SIZE) {
		warning("ResourceLibrary_v1_2::expandCelBitmapRecord(): bitmap size is smaller than its header");
		return utilsLibrary->tgFree(destination);
	}

	const byte *sourceEnd = (sourceSize == TG_RES_V1_2_SIZE_UNKNOWN) ? nullptr : source + sourceSize;
	if (sourceEnd && headerLength > sourceSize) {
		warning("ResourceLibrary_v1_2::expandCelBitmapRecord(): bitmap header overruns its buffer");
		return utilsLibrary->tgFree(destination);
	}

	uint16 flagsOffset = TG_RES_V1_2_2_1_BITMAP_FLAGS_BYTE_OFFSET;
	if (_engine->_version >= kVersionV1_2_5)
		flagsOffset = TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET;

	if (_engine->_version >= kVersionV1_2_2_1 && (source[flagsOffset] & TG_RES_V1_2_2_1_BITMAP_COMPRESSED_MASK)) {
		rleExpandBitmap(destination, size, source, sourceSize);
		destination[flagsOffset] &= ~TG_RES_V1_2_2_1_BITMAP_COMPRESSED_MASK;
	} else if (_engine->_version < kVersionV1_2_3 && !file->lzwState) {
		decodeRleCelBitmap(source, sourceSize, destination, size);
	} else {
		memcpy(destination, source, TG_RES_V1_2_BITMAP_HEADER_SIZE);

		if (_engine->_version >= kVersionV1_2_1) {
			if (!decodeSegmentedBitmapStream(file->lzwState, source + headerLength, destination + headerLength, pixelSize, sourceEnd)) {
				notifyHost(TG_RES_V1_2_1_NOTIFY_BITMAP_ERROR, file);
				return utilsLibrary->tgFree(destination);
			}
		} else {
			DecodeLimits limits(sourceEnd, destination + size);
			decodeSegmentedBitmapStream(file->lzwState, source + headerLength, destination + headerLength, limits);
		}
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
			if (_engine->_version >= kVersionV1_2_1) {
				if (decodeSegmentedBitmapStream(record->lzwState, compressed, raw, rawSize, compressed + compressedSize) && output->write(raw, rawSize) == rawSize)
					result = true;
			} else {
				DecodeLimits limits(compressed + compressedSize, raw + rawSize);
				uint32 written = decodeSegmentedBitmapStream(record->lzwState, compressed, raw, limits);
				if (written != 0 && output->write(raw, rawSize) == rawSize)
					result = true;
			}
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

	byte prefix[TG_RES_V1_2_2_2_EXTRACT_PREFIX_SIZE];
	uint16 prefixSize = (_engine->_version >= kVersionV1_2_2_2) ? TG_RES_V1_2_2_2_EXTRACT_PREFIX_SIZE : TG_RES_V1_2_EXTRACT_PREFIX_SIZE;
	if (resRead(file, prefix, prefixSize) != prefixSize)
		return false;

	uint32 rawSize = READ_LE_UINT32(prefix);
	uint32 remaining = resGetSize(file, index) - prefixSize;
	bool compressed = (prefix[TG_RES_V1_2_EXTRACT_COMPRESSED_OFFSET] != 0);

	if (compressed)
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

	if (_engine->_version >= kVersionV1_2_2) {
		for (uint16 i = 0; i < record->variablePairCount; i++) {
			const byte *pair = record->variablePairs + (uint32)TG_RES_V1_2_2_VARIABLE_PAIR_SIZE * i;
			bool keepGoing;
			if (_engine->_version >= kVersionV1_2_3)
				keepGoing = static_cast<TopgunEngine_v1_2_3 *>(engine())->resourceVariablePairCallback(READ_LE_UINT16(pair), READ_LE_UINT16(pair + 2));
			else
				keepGoing = engine()->resourceVariablePairCallback(READ_LE_UINT16(pair), READ_LE_UINT16(pair + 2));

			if (!keepGoing)
				break;
		}

		return 1;
	}

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

	if (_engine->_version >= kVersionV1_2_2) {
		for (uint16 i = 0; i < record->variableTableCount; i++) {
			const byte *variable = record->variables + (uint32)TG_RES_V1_2_2_VARIABLE_RECORD_SIZE * i;

			if (memcmp(variable + TG_RES_V1_2_2_VARIABLE_NAME_OFFSET, name, nameLength + 1) == 0)
				return READ_LE_UINT16(variable);
		}

		return 0xFFFF;
	}

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

	uint16 maxSize = TG_RES_V1_2_HEADER_TITLE_MAX_SIZE;
	if (_engine->_version >= kVersionV1_2_10)
		maxSize = TG_RES_V1_2_10_HEADER_TITLE_MAX_SIZE;
	else if (_engine->_version >= kVersionV1_2_5)
		maxSize = TG_RES_V1_2_5_HEADER_TITLE_MAX_SIZE;

	if (count > maxSize)
		count = maxSize;

	uint16 titleOffset = TG_RES_V1_2_HEADER_TITLE_OFFSET;
	if (_engine->_version >= kVersionV1_2_10)
		titleOffset = TG_RES_V1_2_10_HEADER_TITLE_OFFSET;

	if (count != 0)
		memmove(destination, record->header + titleOffset, count);

	return count;
}

uint16 ResourceLibrary_v1_2::resGetHeaderRec(uint16 file, byte *destination, uint16 count) {
	ResourceFile *record = findFile(file, "resGetHeaderRec");
	if (!record)
		return 0;

	if (count > TG_RES_V1_2_1_HEADER_SIZE)
		count = TG_RES_V1_2_1_HEADER_SIZE;

	if (count != 0)
		memmove(destination, record->header, count);

	return count;
}

void ResourceLibrary_v1_2::resSetNotify(uint16 file, uint16 window) {
	ResourceFile *record = findFile(file, "resSetNotify");
	if (!record)
		return;

	record->notifyWindow = window;
}

void ResourceLibrary_v1_2::notifyHost(uint16 message, ResourceFile *file) {
	if (file->notifyWindow != 0)
		engine()->resourceSendMessage(file->notifyWindow, message);
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

uint16 ResourceLibrary_v1_2::resGetLibraryName(uint16 file, char *destination, uint16 count) {
	if (!destination) {
		warning("ResourceLibrary_v1_2::resGetLibraryName(): no destination");
		return 0;
	}

	destination[0] = 0;

	if (_engine->_version >= kVersionV1_2_10) {
		warning("ResourceLibrary_v1_2::resGetLibraryName(): the container has no library name");
		return 0;
	}

	ResourceFile *record = findFile(file, "resGetLibraryName");
	if (!record)
		return 0;

	uint16 maxSize = TG_RES_V1_2_2_LIBRARY_NAME_SIZE;
	if (_engine->_version >= kVersionV1_2_5)
		maxSize = TG_RES_V1_2_5_LIBRARY_NAME_SIZE;

	if (count > maxSize)
		count = maxSize;

	const byte *name = record->header + TG_RES_V1_2_2_HEADER_LIBRARY_NAME_OFFSET;
	if (_engine->_version >= kVersionV1_2_5)
		name = record->fullHeader + TG_RES_V1_2_5_HEADER_LIBRARY_NAME_OFFSET;
	else if (_engine->_version >= kVersionV1_2_3)
		name = record->fullHeader + TG_RES_V1_2_3_HEADER_LIBRARY_NAME_OFFSET;
	else if (_engine->_version >= kVersionV1_2_2_2)
		name = record->fullHeader + TG_RES_V1_2_2_2_HEADER_LIBRARY_NAME_OFFSET;
	else if (_engine->_version >= kVersionV1_2_2_1)
		name = record->fullHeader + TG_RES_V1_2_2_1_HEADER_LIBRARY_NAME_OFFSET;

	Common::strlcpy(destination, (const char *)name, count);
	return count;
}

uint16 ResourceLibrary_v1_2::resGetMaxFadeColors(uint16 file) {
	ResourceFile *record = findFile(file, "resGetMaxFadeColors");
	if (!record)
		return 0;

	return record->maxFadeColors;
}

uint16 ResourceLibrary_v1_2::resGetMaxTransColors(uint16 file) {
	ResourceFile *record = findFile(file, "resGetMaxTransColors");
	if (!record)
		return 0;

	return record->maxTransparentColors;
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

	if (_engine->_version >= kVersionV1_2_2)
		record->chainOffset = 0;

	byte header[TG_RES_V1_2_3_WAVE_HEADER_SIZE];
	uint16 headerSize = TG_RES_V1_2_WAVE_HEADER_SIZE;
	if (_engine->_version >= kVersionV1_2_3)
		headerSize = TG_RES_V1_2_3_WAVE_HEADER_SIZE;
	else if (_engine->_version >= kVersionV1_2_2_2)
		headerSize = TG_RES_V1_2_2_2_WAVE_HEADER_SIZE;

	if (!readResourceBytes(record, header, headerSize))
		return 0;

	uint16 waveInfo = createWaveStreamInfo(file, record);
	if (waveInfo == 0)
		return 0;

	if (_engine->_version >= kVersionV1_2_2_2)
		return static_cast<TopgunEngine_v1_2_3 *>(engine())->audioCreateWaveEvent(header, waveInfo);

	return engine()->audioCreateWave(header, waveInfo);
}

uint16 ResourceLibrary_v1_2::resCreateMidiEvent(uint16 file, uint16 index) {
	if (resSeekResource(file, index) == 0)
		return 0;

	ResourceFile *record = findFile(file, "resCreateMidiEvent");
	if (!record)
		return 0;

	byte header[TG_RES_V1_2_2_1_MIDI_HEADER_SIZE];
	uint16 headerSize = TG_RES_V1_2_2_MIDI_HEADER_SIZE;
	if (_engine->_version >= kVersionV1_2_6)
		headerSize = TG_RES_V1_2_6_MIDI_HEADER_SIZE;
	else if (_engine->_version >= kVersionV1_2_2_2)
		headerSize = TG_RES_V1_2_2_2_MIDI_HEADER_SIZE;
	else if (_engine->_version >= kVersionV1_2_2_1)
		headerSize = TG_RES_V1_2_2_1_MIDI_HEADER_SIZE;

	if (!readResourceBytes(record, header, headerSize))
		return 0;

	uint16 midiInfo = createWaveStreamInfo(file, record);
	if (midiInfo == 0)
		return 0;

	if (_engine->_version >= kVersionV1_2_3)
		return static_cast<TopgunEngine_v1_2_3 *>(engine())->audioCreateMidiEvent(header, midiInfo);

	return engine()->audioCreateMidiEvent(header, midiInfo);
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

	if (_engine->_version >= kVersionV1_2_2) {
		record->chainOffset = info->chainOffset;

		record->stream->seek(info->position, SEEK_SET);
		bool success = readResourceBytes(record, buffer, (uint32)count);

		info->chainOffset = record->chainOffset;
		record->callbackSet = callbackSet;
		info->position = (uint32)record->stream->pos();

		return success ? count : 0;
	}

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
	uint32 dictionarySize = (uint32)TG_RES_V1_2_LZW_TABLE_SIZE * TG_RES_V1_2_LZW_ALLOCATION_FACTOR;
	if (_engine->_version >= kVersionV1_2_5)
		state->dictionary = engine()->utilsLibrary()->tgMalloc(dictionarySize, 0);
	else
		state->dictionary = swinMalloc(dictionarySize, 0);

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
	if (state->dictionary) {
		if (_engine->_version >= kVersionV1_2_5)
			engine()->utilsLibrary()->tgFree(state->dictionary);
		else
			swinFree(state->dictionary);
	}

	delete state;
}

ResourceLibrary_v1_2_3::ResourceLibrary_v1_2_3(TopgunEngine_v1_2 *engine) : ResourceLibrary_v1_2(engine) {
}

uint32 ResourceLibrary_v1_2_3::keyOffset(const ResourceFile *file, uint16 key) {
	uint16 keyTableOffset = TG_RES_V1_2_3_HEADER_KEY_TABLE_OFFSET;
	if (_engine->_version >= kVersionV1_2_10)
		keyTableOffset = TG_RES_V1_2_10_HEADER_KEY_TABLE_OFFSET;
	else if (_engine->_version >= kVersionV1_2_5)
		keyTableOffset = TG_RES_V1_2_5_HEADER_KEY_TABLE_OFFSET;
	else if (_engine->_version < kVersionV1_2_2_2)
		keyTableOffset = TG_RES_V1_2_2_1_HEADER_KEY_TABLE_OFFSET;
	else if (_engine->_version < kVersionV1_2_3)
		keyTableOffset = TG_RES_V1_2_2_2_HEADER_KEY_TABLE_OFFSET;

	return READ_LE_UINT32(file->fullHeader + keyTableOffset + TG_RES_V1_2_2_1_KEY_ENTRY_SIZE * key);
}

uint32 ResourceLibrary_v1_2_3::keySize(const ResourceFile *file, uint16 key) {
	uint16 keyTableOffset = TG_RES_V1_2_3_HEADER_KEY_TABLE_OFFSET;
	if (_engine->_version >= kVersionV1_2_10)
		keyTableOffset = TG_RES_V1_2_10_HEADER_KEY_TABLE_OFFSET;
	else if (_engine->_version >= kVersionV1_2_5)
		keyTableOffset = TG_RES_V1_2_5_HEADER_KEY_TABLE_OFFSET;
	else if (_engine->_version < kVersionV1_2_2_2)
		keyTableOffset = TG_RES_V1_2_2_1_HEADER_KEY_TABLE_OFFSET;
	else if (_engine->_version < kVersionV1_2_3)
		keyTableOffset = TG_RES_V1_2_2_2_HEADER_KEY_TABLE_OFFSET;

	return READ_LE_UINT32(file->fullHeader + keyTableOffset + TG_RES_V1_2_2_1_KEY_ENTRY_SIZE * key + TG_RES_V1_2_2_1_KEY_SIZE_OFFSET);
}

bool ResourceLibrary_v1_2_3::readResourceFileHeader(ResourceFile *file) {
	byte *header = file->fullHeader;

	uint16 probeSize = TG_RES_V1_2_2_1_HEADER_PROBE_SIZE;
	uint16 formatOffset = TG_RES_V1_2_2_1_HEADER_FORMAT_OFFSET;
	uint16 maxHeaderSize = TG_RES_V1_2_3_HEADER_SIZE;
	uint16 limitsOffset = TG_RES_V1_2_2_2_HEADER_LIMITS_OFFSET;
	uint16 maxChunks = TG_RES_V1_2_3_MAX_RESIDENT_CHUNKS;
	uint16 chunkCountOffset = TG_RES_V1_2_3_HEADER_CHUNK_COUNT_OFFSET;
	uint16 maxFadeOffset = TG_RES_V1_2_3_HEADER_MAX_FADE_OFFSET;
	uint16 maxTransOffset = TG_RES_V1_2_3_HEADER_MAX_TRANS_OFFSET;
	uint16 extraSlotsOffset = TG_RES_V1_2_3_HEADER_EXTRA_SLOTS_OFFSET;
	uint16 stringCountOffset = TG_RES_V1_2_3_HEADER_STRING_COUNT_OFFSET;
	uint16 variableCountOffset = TG_RES_V1_2_3_HEADER_VARIABLE_COUNT_OFFSET;
	uint16 buildTypeOffset = TG_RES_V1_2_3_HEADER_BUILD_TYPE_OFFSET;
	uint16 chainedOffset = TG_RES_V1_2_3_HEADER_CHAINED_OFFSET;

	if (_engine->_version >= kVersionV1_2_5) {
		probeSize = TG_RES_V1_2_5_HEADER_PROBE_SIZE;
		formatOffset = TG_RES_V1_2_5_HEADER_FORMAT_OFFSET;
		maxHeaderSize = TG_RES_V1_2_5_HEADER_SIZE;
		limitsOffset = TG_RES_V1_2_5_HEADER_LIMITS_OFFSET;
		maxChunks = TG_RES_V1_2_5_MAX_RESIDENT_CHUNKS;
		chunkCountOffset = TG_RES_V1_2_5_HEADER_CHUNK_COUNT_OFFSET;
		maxFadeOffset = TG_RES_V1_2_5_HEADER_MAX_FADE_OFFSET;
		maxTransOffset = TG_RES_V1_2_5_HEADER_MAX_TRANS_OFFSET;
		extraSlotsOffset = TG_RES_V1_2_5_HEADER_EXTRA_SLOTS_OFFSET;
		stringCountOffset = TG_RES_V1_2_5_HEADER_STRING_COUNT_OFFSET;
		variableCountOffset = TG_RES_V1_2_5_HEADER_VARIABLE_COUNT_OFFSET;
		buildTypeOffset = TG_RES_V1_2_5_HEADER_BUILD_TYPE_OFFSET;
		chainedOffset = TG_RES_V1_2_5_HEADER_CHAINED_OFFSET;
	}

	if (_engine->_version >= kVersionV1_2_10) {
		maxHeaderSize = TG_RES_V1_2_10_HEADER_SIZE;
		maxChunks = TG_RES_V1_2_10_MAX_RESIDENT_CHUNKS;
		chunkCountOffset = TG_RES_V1_2_10_HEADER_CHUNK_COUNT_OFFSET;
		maxFadeOffset = TG_RES_V1_2_10_HEADER_MAX_FADE_OFFSET;
		maxTransOffset = TG_RES_V1_2_10_HEADER_MAX_TRANS_OFFSET;
		extraSlotsOffset = TG_RES_V1_2_10_HEADER_EXTRA_SLOTS_OFFSET;
		stringCountOffset = TG_RES_V1_2_10_HEADER_STRING_COUNT_OFFSET;
		variableCountOffset = TG_RES_V1_2_10_HEADER_VARIABLE_COUNT_OFFSET;
		buildTypeOffset = TG_RES_V1_2_10_HEADER_BUILD_TYPE_OFFSET;
		chainedOffset = TG_RES_V1_2_10_HEADER_CHAINED_OFFSET;
	}

	if (_engine->_version < kVersionV1_2_2_2) {
		maxHeaderSize = TG_RES_V1_2_2_1_HEADER_SIZE;
		maxFadeOffset = TG_RES_V1_2_2_1_HEADER_MAX_FADE_OFFSET;
		maxTransOffset = TG_RES_V1_2_2_1_HEADER_MAX_TRANS_OFFSET;
		stringCountOffset = TG_RES_V1_2_2_1_HEADER_STRING_COUNT_OFFSET;
		variableCountOffset = TG_RES_V1_2_2_1_HEADER_VARIABLE_COUNT_OFFSET;
		buildTypeOffset = TG_RES_V1_2_2_1_HEADER_BUILD_TYPE_OFFSET;
		chainedOffset = TG_RES_V1_2_2_1_HEADER_CHAINED_OFFSET;
	} else if (_engine->_version < kVersionV1_2_3) {
		maxHeaderSize = TG_RES_V1_2_2_2_HEADER_SIZE;
		maxChunks = TG_RES_V1_2_2_2_MAX_RESIDENT_CHUNKS;
		chunkCountOffset = TG_RES_V1_2_2_2_HEADER_CHUNK_COUNT_OFFSET;
		maxFadeOffset = TG_RES_V1_2_2_2_HEADER_MAX_FADE_OFFSET;
		maxTransOffset = TG_RES_V1_2_2_2_HEADER_MAX_TRANS_OFFSET;
		extraSlotsOffset = TG_RES_V1_2_2_2_HEADER_EXTRA_SLOTS_OFFSET;
		stringCountOffset = TG_RES_V1_2_2_2_HEADER_STRING_COUNT_OFFSET;
		variableCountOffset = TG_RES_V1_2_2_2_HEADER_VARIABLE_COUNT_OFFSET;
		buildTypeOffset = TG_RES_V1_2_2_2_HEADER_BUILD_TYPE_OFFSET;
		chainedOffset = TG_RES_V1_2_2_2_HEADER_CHAINED_OFFSET;
	}

	if (file->stream->read(header, probeSize) != probeSize)
		return false;

	if (READ_LE_UINT16(header) != TG_RES_HEADER_MAGIC)
		return false;

	if (READ_LE_UINT16(header + formatOffset) != TG_RES_V1_2_2_1_FORMAT_VERSION)
		return false;

	if (_engine->_version >= kVersionV1_2_10 && READ_LE_UINT16(header + TG_RES_V1_2_10_HEADER_TAG_OFFSET) != TG_RES_V1_2_10_HEADER_TAG_VALUE)
		return false;

	uint16 headerSize = READ_LE_UINT16(header + TG_RES_V1_2_2_1_HEADER_SIZE_OFFSET);
	if (headerSize < probeSize)
		return false;

	if (headerSize > maxHeaderSize) {
		warning("ResourceLibrary_v1_2_3::readResourceFileHeader(): header size %d is too big", headerSize);
		return false;
	}

	uint32 remaining = headerSize - probeSize;
	if (file->stream->read(header + probeSize, remaining) != remaining)
		return false;

	if (keySize(file, TG_RES_V1_2_2_1_KEY_INDEX) == 0)
		return false;

	byte sum = 0;
	for (uint16 i = 0; i < headerSize; i++)
		sum += header[i];

	WRITE_LE_UINT16(header + TG_RES_V1_2_2_1_HEADER_SIZE_OFFSET, maxHeaderSize);

	if (sum != 0)
		return false;

	// The title and the inherited accessors work on the first bytes of the header
	memcpy(file->header, header, TG_RES_V1_2_HEADER_SIZE);

	file->magic = READ_LE_UINT16(header);
	file->maxFadeColors = READ_LE_UINT16(header + maxFadeOffset);
	file->maxTransparentColors = READ_LE_UINT16(header + maxTransOffset);
	file->stringCount = READ_LE_UINT16(header + stringCountOffset);
	file->variableCount = READ_LE_UINT16(header + variableCountOffset);
	file->buildType = header[buildTypeOffset];
	file->chained = (header[chainedOffset] != 0);

	if (_engine->_version < kVersionV1_2_2_2) {
		file->residentSplit = READ_LE_UINT32(header + TG_RES_V1_2_2_1_HEADER_RESIDENT_SPLIT_OFFSET);
		file->residentSize = keySize(file, TG_RES_V1_2_2_1_KEY_RESIDENT);
		return true;
	}

	file->chunkCount = READ_LE_UINT16(header + chunkCountOffset);
	file->extraSlots = READ_LE_UINT16(header + extraSlotsOffset);

	for (uint16 i = 0; i <= maxChunks; i++)
		file->limits[i] = READ_LE_UINT32(header + limitsOffset + 4 * i);

	return true;
}

bool ResourceLibrary_v1_2_3::loadIndexPages(ResourceFile *record) {
	uint32 size = keySize(record, TG_RES_V1_2_2_1_KEY_INDEX);
	if (size == 0)
		return true;

	record->stream->seek(keyOffset(record, TG_RES_V1_2_2_1_KEY_INDEX), SEEK_SET);

	// The game keeps the entry counts in 16 bits
	uint32 count = size / TG_RES_INDEX_ENTRY_SIZE;
	if (count > 0xFFFF)
		warning("ResourceLibrary_v1_2_3::loadIndexPages(): too many entries (%d)", count);

	record->indexCount = (uint16)count;
	record->entryCount = (uint16)(record->indexCount + record->extraSlots);
	if (record->entryCount == 0)
		return true;

	// The slots after the entries of the file are free
	uint32 indexSize = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->entryCount;
	record->index = engine()->utilsLibrary()->tgMalloc(indexSize, 1);
	if (!record->index)
		return false;

	uint32 fileIndexSize = (uint32)TG_RES_INDEX_ENTRY_SIZE * record->indexCount;
	return record->stream->read(record->index, fileIndexSize) == fileIndexSize;
}

bool ResourceLibrary_v1_2_3::loadKey3Table(uint16 file, ResourceFile *record) {
	if (keySize(record, TG_RES_V1_2_2_1_KEY_VARIABLES) == 0)
		return true;

	uint32 size = 0;
	record->variablePairs = resLoadKeyResource(file, TG_RES_V1_2_2_1_KEY_VARIABLES, &size);
	if (!record->variablePairs)
		return false;

	record->variablePairCount = (uint16)(size / TG_RES_V1_2_2_VARIABLE_PAIR_SIZE);
	return true;
}

bool ResourceLibrary_v1_2_3::loadResidentChunks(ResourceFile *record) {
	if (keySize(record, TG_RES_V1_2_2_1_KEY_RESIDENT) == 0)
		return true;

	record->stream->seek(keyOffset(record, TG_RES_V1_2_2_1_KEY_RESIDENT), SEEK_SET);

	if (_engine->_version < kVersionV1_2_2_2)
		return ResourceLibrary_v1_2::loadResidentChunks(record);

	uint16 maxChunks = TG_RES_V1_2_3_MAX_RESIDENT_CHUNKS;
	if (_engine->_version >= kVersionV1_2_10)
		maxChunks = TG_RES_V1_2_10_MAX_RESIDENT_CHUNKS;
	else if (_engine->_version >= kVersionV1_2_5)
		maxChunks = TG_RES_V1_2_5_MAX_RESIDENT_CHUNKS;
	else if (_engine->_version < kVersionV1_2_3)
		maxChunks = TG_RES_V1_2_2_2_MAX_RESIDENT_CHUNKS;

	if (record->chunkCount > maxChunks) {
		warning("ResourceLibrary_v1_2_3::loadResidentChunks(): too many chunks (%d)", record->chunkCount);
		return false;
	}

	uint32 previousLimit = 0;
	for (uint16 i = 0; i < record->chunkCount; i++) {
		uint32 size = record->limits[i + 1] - previousLimit;
		previousLimit = record->limits[i + 1];

		record->residentChunks[i] = engine()->utilsLibrary()->tgMalloc(size, 0);
		if (!record->residentChunks[i])
			return false;

		if (record->stream->read(record->residentChunks[i], size) != size)
			return false;
	}

	return true;
}

bool ResourceLibrary_v1_2_3::loadConstStringPool(uint16 file, ResourceFile *record) {
	if (keySize(record, TG_RES_V1_2_2_1_KEY_STRINGS) == 0)
		return true;

	record->stringPool = resLoadKeyResource(file, TG_RES_V1_2_2_1_KEY_STRINGS, nullptr);
	return record->stringPool != nullptr;
}

byte *ResourceLibrary_v1_2_3::getIndexEntry(ResourceFile *file, uint16 index) {
	return file->index + (uint32)index * TG_RES_INDEX_ENTRY_SIZE;
}

byte *ResourceLibrary_v1_2_3::resLoadKeyResource(uint16 file, uint16 key, uint32 *outSize) {
	ResourceFile *record = findFile(file, "resLoadKeyResource");
	if (!record)
		return nullptr;

	uint16 keyCount = TG_RES_V1_2_2_2_KEY_COUNT;
	if (_engine->_version >= kVersionV1_2_10)
		keyCount = TG_RES_V1_2_10_KEY_COUNT;
	else if (_engine->_version < kVersionV1_2_2_2)
		keyCount = TG_RES_V1_2_2_1_KEY_COUNT;

	if (key >= keyCount)
		return nullptr;

	uint32 size = keySize(record, key);
	if (outSize)
		*outSize = size;

	if (size == 0)
		return nullptr;

	UtilsLibrary *utilsLibrary = engine()->utilsLibrary();

	byte *buffer = utilsLibrary->tgMalloc(size, 0);
	if (!buffer)
		return nullptr;

	uint32 offset = keyOffset(record, key);
	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return utilsLibrary->tgFree(buffer);

	if (record->stream->read(buffer, size) != size)
		return utilsLibrary->tgFree(buffer);

	return buffer;
}

byte *ResourceLibrary_v1_2_3::resLoadNameTable(uint16 file, uint32 *outSize) {
	uint16 key = TG_RES_V1_2_2_2_KEY_NAME_TABLE;
	if (_engine->_version < kVersionV1_2_2_2)
		key = TG_RES_V1_2_2_1_KEY_NAME_TABLE;

	return resLoadKeyResource(file, key, outSize);
}

uint16 ResourceLibrary_v1_2_3::resLoadPalette(uint16 file, byte *buffer) {
	ResourceFile *record = findFile(file, "resLoadPalette");
	if (!record)
		return 0;

	if (!buffer)
		return 0;

	uint16 key = TG_RES_V1_2_2_2_KEY_PALETTE;
	if (_engine->_version < kVersionV1_2_2_2)
		key = TG_RES_V1_2_2_1_KEY_PALETTE;

	uint32 size = keySize(record, key);
	if (size == 0)
		return 0;

	uint32 offset = keyOffset(record, key);
	if (!record->stream->seek(offset, SEEK_SET) || record->stream->pos() != offset)
		return 0;

	if (record->stream->read(buffer, size) != size)
		return 0;

	return (uint16)(size / TG_RES_V1_2_2_1_PALETTE_ENTRY_SIZE);
}

byte *ResourceLibrary_v1_2_3::resLoadResource(uint16 file, uint16 index, uint32 *outSize) {
	ResourceFile *record = findFile(file, "resLoadResource");
	if (!record)
		return nullptr;

	if (record->entryCount <= index)
		return nullptr;

	ResourceEntry entry;
	readEntry(record, index, entry);

	if (_engine->_version < kVersionV1_2_2_2 && entry.type >= TG_RES_V1_2_2_1_MOVIE_BUFFER_TYPE && entry.type <= TG_RES_V1_2_2_1_RESIDENT_LAST_TYPE)
		return ResourceLibrary_v1_2::resLoadResource(file, index, outSize);

	uint16 firstResidentType = TG_RES_V1_2_2_2_RESIDENT_FIRST_TYPE;
	uint16 lastResidentType = TG_RES_V1_2_2_1_RESIDENT_LAST_TYPE;
	uint16 flagsOffset = TG_RES_V1_2_2_1_BITMAP_FLAGS_BYTE_OFFSET;
	if (_engine->_version >= kVersionV1_2_10) {
		firstResidentType = TG_RES_V1_2_10_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_10_RESIDENT_LAST_TYPE;
		flagsOffset = TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET;
	} else if (_engine->_version >= kVersionV1_2_5) {
		firstResidentType = TG_RES_V1_2_5_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_5_RESIDENT_LAST_TYPE;
		flagsOffset = TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET;
	}

	if (entry.type >= firstResidentType && entry.type <= lastResidentType) {
		if (outSize)
			*outSize = entry.size;

		// The slots after the entries of the file hold copies made in memory
		if (record->indexCount <= index) {
			if (!record->dynamicResident.contains(index)) {
				warning("ResourceLibrary_v1_2_3::resLoadResource(): resource %d has no copy in memory", index);
				return nullptr;
			}

			return record->dynamicResident[index];
		}

		uint16 chunk = 0;
		if (record->chunkCount != 0) {
			while (chunk < record->chunkCount && entry.offset >= record->limits[chunk + 1])
				chunk++;
		}

		if (chunk >= record->chunkCount || !record->residentChunks[chunk]) {
			warning("ResourceLibrary_v1_2_3::resLoadResource(): resource %d is outside the resident area", index);
			return nullptr;
		}

		if (chunk != 0)
			entry.offset -= record->limits[chunk];

		return record->residentChunks[chunk] + (uint16)entry.offset;
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
		if (entry.size <= flagsOffset) {
			warning("ResourceLibrary_v1_2_3::resLoadResource(): bitmap record is too short");
			return utilsLibrary->tgFree(buffer);
		}

		// The compressed sprites are returned as they are stored
		if ((buffer[flagsOffset] & TG_RES_V1_2_2_1_BITMAP_COMPRESSED_MASK) == 0) {
			byte *bitmap = expandCelBitmapRecord(record, buffer, entry.size, outSize);
			utilsLibrary->tgFree(buffer);
			return bitmap;
		}
	}

	if (_engine->_version >= kVersionV1_2_10 && entry.type == TG_RES_V1_2_10_TABLE_TYPE) {
		if (entry.size < TG_RES_V1_2_10_TABLE_HEADER_SIZE) {
			warning("ResourceLibrary_v1_2_3::resLoadResource(): table record %d is too short", index);
			return utilsLibrary->tgFree(buffer);
		}

		// The far pointer to the data is stored as its offset from the start of the block
		WRITE_LE_UINT32(buffer, TG_RES_V1_2_10_TABLE_HEADER_SIZE);
		WRITE_LE_UINT16(buffer + TG_RES_V1_2_10_TABLE_OWNED_OFFSET, 1);
	}

	if (outSize)
		*outSize = entry.size;

	return buffer;
}

uint16 ResourceLibrary_v1_2_3::resCountResources(uint16 file, uint16 *outCount) {
	ResourceFile *record = findFile(file, "resCountResources");
	if (!record)
		return 0;

	if (outCount)
		*outCount = (uint16)(record->entryCount - record->extraSlots);

	return record->entryCount;
}

const char *ResourceLibrary_v1_2_3::resGetConstStr(uint16 file, uint16 offset) {
	ResourceFile *record = findFile(file, "resGetConstStr");
	if (!record)
		return nullptr;

	if (!record->stringPool)
		return nullptr;

	if (keySize(record, TG_RES_V1_2_2_1_KEY_STRINGS) <= offset)
		return nullptr;

	return (const char *)record->stringPool + offset;
}

byte ResourceLibrary_v1_2_3::resGetBuildType(uint16 file) {
	ResourceFile *record = findFile(file, "resGetBuildType");
	if (!record)
		return 0;

	return (byte)record->buildType;
}

uint16 ResourceLibrary_v1_2_3::resCopyResource(uint16 file, uint16 index) {
	ResourceFile *record = findFile(file, "resCopyResource");
	if (!record)
		return 0;

	if (record->entryCount <= index)
		return 0;

	byte *entry = getIndexEntry(record, index);
	uint16 type = READ_LE_UINT16(entry);
	if (type == 0)
		return 0;

	uint16 slot = record->indexCount;
	while (slot < record->entryCount && READ_LE_UINT16(getIndexEntry(record, slot)) != 0)
		slot++;

	if (record->entryCount <= slot)
		return 0;

	byte *slotEntry = getIndexEntry(record, slot);

	uint16 firstResidentType = TG_RES_V1_2_2_2_RESIDENT_FIRST_TYPE;
	uint16 lastResidentType = TG_RES_V1_2_2_1_RESIDENT_LAST_TYPE;
	uint16 spriteType = TG_RES_V1_2_6_1_SPRITE_TYPE;
	if (_engine->_version >= kVersionV1_2_10) {
		firstResidentType = TG_RES_V1_2_10_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_10_RESIDENT_LAST_TYPE;
		spriteType = TG_RES_V1_2_10_SPRITE_TYPE;
	} else if (_engine->_version >= kVersionV1_2_5) {
		firstResidentType = TG_RES_V1_2_5_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_5_RESIDENT_LAST_TYPE;
	}

	if (type >= firstResidentType && type <= lastResidentType) {
		uint32 size = READ_LE_UINT32(entry + 6);

		UtilsLibrary *utilsLibrary = engine()->utilsLibrary();
		byte *copy = utilsLibrary->tgMalloc(size, 0);
		if (!copy)
			return 0;

		if (_engine->_version >= kVersionV1_2_6_1) {
			byte *source = resLoadResource(file, index, nullptr);
			if (!source) {
				utilsLibrary->tgFree(copy);
				return 0;
			}

			memcpy(copy, source, size);

			if (type == spriteType) {
				if (size < TG_RES_V1_2_6_1_SPRITE_HEADER_SIZE) {
					warning("ResourceLibrary_v1_2_3::resCopyResource(): sprite record %d is too short", index);
					utilsLibrary->tgFree(copy);
					return 0;
				}

				WRITE_LE_UINT16(copy + TG_RES_V1_2_6_1_SPRITE_CLICK_SCRIPT, 0);
				WRITE_LE_UINT16(copy + TG_RES_V1_2_6_1_SPRITE_CLICK_ARGUMENT, 0);
				WRITE_LE_UINT16(copy + TG_RES_V1_2_6_1_SPRITE_DRAG_SCRIPT, 0);
				WRITE_LE_UINT16(copy + TG_RES_V1_2_6_1_SPRITE_DROP_SCRIPT, 0);
				WRITE_LE_UINT16(copy + TG_RES_V1_2_6_1_SPRITE_UNK_06, READ_LE_UINT16(copy + TG_RES_V1_2_6_1_SPRITE_UNK_04));
				copy[TG_RES_V1_2_6_1_SPRITE_DRAGGABLE] = 0;
				copy[TG_RES_V1_2_6_1_SPRITE_DISABLED] = 0;
			}
		} else {
			record->stream->seek(keyOffset(record, TG_RES_V1_2_2_1_KEY_RESIDENT) + READ_LE_UINT32(entry + 2), SEEK_SET);
			if (record->stream->read(copy, size) != size) {
				utilsLibrary->tgFree(copy);
				return 0;
			}
		}

		// The copy is kept in a table since the index entry cannot hold a pointer
		WRITE_LE_UINT16(slotEntry, type);
		WRITE_LE_UINT32(slotEntry + 2, 0);
		WRITE_LE_UINT32(slotEntry + 6, size);
		record->dynamicResident[slot] = copy;
		return slot;
	}

	memmove(slotEntry, entry, TG_RES_INDEX_ENTRY_SIZE);
	return slot;
}

bool ResourceLibrary_v1_2_3::resDeleteResource(uint16 file, uint16 index) {
	ResourceFile *record = findFile(file, "resDeleteResource");
	if (!record)
		return false;

	if (record->entryCount <= index)
		return false;

	if (record->indexCount > index)
		return false;

	byte *entry = getIndexEntry(record, index);
	uint16 type = READ_LE_UINT16(entry);

	uint16 firstResidentType = TG_RES_V1_2_2_2_RESIDENT_FIRST_TYPE;
	uint16 lastResidentType = TG_RES_V1_2_2_1_RESIDENT_LAST_TYPE;
	if (_engine->_version >= kVersionV1_2_10) {
		firstResidentType = TG_RES_V1_2_10_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_10_RESIDENT_LAST_TYPE;
	} else if (_engine->_version >= kVersionV1_2_5) {
		firstResidentType = TG_RES_V1_2_5_RESIDENT_FIRST_TYPE;
		lastResidentType = TG_RES_V1_2_5_RESIDENT_LAST_TYPE;
	}

	if (type >= firstResidentType && type <= lastResidentType && record->dynamicResident.contains(index)) {
		engine()->utilsLibrary()->tgFree(record->dynamicResident[index]);
		record->dynamicResident.erase(index);
	}

	memset(entry, 0, TG_RES_INDEX_ENTRY_SIZE);
	return true;
}

} // End of namespace Topgun
