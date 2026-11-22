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

#ifndef TOPGUN_LIBRARIES_RESOURCE_RESOURCE_H
#define TOPGUN_LIBRARIES_RESOURCE_RESOURCE_H

#include "common/file.h"
#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/str.h"

#define TG_RES_HEADER_SIZE                  14
#define TG_RES_HEADER_MAGIC                 0x4C37
#define TG_RES_INDEX_ENTRY_SIZE             10
#define TG_RES_WAVE_STREAM_HEADER_SIZE      22
#define TG_RES_WAVE_STREAM_HEADER_SIZE_V1_1 23
#define TG_RES_COMPRESSION_FLAG_OFFSET      22
#define TG_RES_ADPCM_HEADER_SIZE            10
#define TG_RES_ADPCM_STEP_TABLE_ROWS        89
#define TG_RES_ADPCM_MAX_STEP_POSITION      0x2C0
#define TG_RES_CREATE_SOUND_COMMAND         0xC9
#define TG_RES_BITMAP_TYPE                  1
#define TG_RES_RESIDENT_TYPE                7
#define TG_RES_RESIDENT_ID_LIST_TYPE        10
#define TG_RES_BITMAP_RECORD_MIN_SIZE       8
#define TG_RES_MAX_RUN_LENGTH               200

namespace Topgun {

class TopgunEngine;

class ResourceLibrary {
public:
	ResourceLibrary(TopgunEngine *engine);
	virtual ~ResourceLibrary();

	virtual uint16 resCloseFile(uint16 file);
	uint16 resCountResources(uint16 file);
	uint16 resCountVariables(uint16 file);
	virtual uint16 resCreateWaveEvent(uint16 file, uint16 index);
	uint32 resGetSize(uint16 file, uint16 index);
	uint16 resGetType(uint16 file, uint16 index);
	virtual byte *resLoadNameTable(uint16 file, uint32 *outSize);
	virtual uint16 resLoadPalette(uint16 file, byte *buffer);
	virtual byte *resLoadResource(uint16 file, uint16 index, uint32 *outSize);
	virtual uint16 resOpenFile(const Common::String &path);
	uint32 resRead(uint16 file, byte *buffer, uint32 count);
	uint32 resSeek(uint16 file, int32 offset, int16 origin);
	virtual uint32 resSeekResource(uint16 file, uint16 index);
	bool resSetCallback(uint16 file, bool callbackActive, uint32 interval);
	void waveDone(uint16 waveInfo, uint16 flag);
	virtual int32 waveRead(uint16 waveInfo, byte *buffer, int32 count);
	int32 waveSeek(uint16 waveInfo, int32 offset, int16 origin);

protected:
	struct ResourceEntry {
		uint16 type;
		uint32 offset;
		uint32 size;

		ResourceEntry() : type(0), offset(0), size(0) {}
	};

	struct ResourceFile {
		Common::File *stream;
		bool callbackSet;
		byte *index;
		byte *extra;
		uint16 magic;
		uint16 version;
		uint16 entryCount;
		uint16 dwordCount;
		uint16 nameBlockSize;
		uint16 variableCount;
		uint16 extraSize;
		uint32 callbackCounter;
		int32 callbackInterval;

		ResourceFile() : stream(nullptr), callbackSet(false), index(nullptr), extra(nullptr), magic(0),
			version(0), entryCount(0), dwordCount(0), nameBlockSize(0), variableCount(0), extraSize(0),
			callbackCounter(0), callbackInterval(0) {}
		virtual ~ResourceFile() {}
	};

	struct WaveStreamInfo {
		uint16 file;
		uint32 position;
		bool compressed;
		uint32 end;
		byte *buffer;
		uint32 bufferSize;
		uint32 available;
		uint32 consumed;
		uint16 channels;
		int32 predictor[2];
		int32 stepPosition[2];

		WaveStreamInfo() : file(0), position(0), compressed(false), end(0), buffer(nullptr), bufferSize(0),
			available(0), consumed(0), channels(0) {
			predictor[0] = predictor[1] = 0;
			stepPosition[0] = stepPosition[1] = 0;
		}
	};

	typedef Common::HashMap<uint16, ResourceFile *> FileMap;
	typedef Common::HashMap<uint16, WaveStreamInfo *> WaveInfoMap;

	void decodeRleCelBitmap(const byte *source, uint32 sourceSize, byte *destination, uint32 destinationSize);
	byte *expandCelBitmapRecord(byte *compressed, uint32 compressedSize, uint32 *outSize);
	bool readChunkedWithCallback(ResourceFile *file, byte *buffer, uint32 count);
	bool readResourceFileHeader(ResourceFile *file);
	virtual ResourceFile *findFile(uint16 file, const char *caller);
	uint16 registerFile(ResourceFile *record);
	uint16 registerWaveInfo(WaveStreamInfo *info);
	void destroyWaveInfo(WaveStreamInfo *info);
	bool openCompressedWave(ResourceFile *file, WaveStreamInfo *info, const byte *header);
	void resetAdpcmState(WaveStreamInfo *info);
	void decodeAdpcm(WaveStreamInfo *info, byte *destination, const byte *source, uint32 inputBytes);
	int32 readCompressedWave(ResourceFile *file, WaveStreamInfo *info, byte *buffer, int32 count);
	void readEntry(const ResourceFile *file, uint16 index, ResourceEntry &entry) const;

	TopgunEngine *_engine;
	FileMap _files;
	uint16 _nextFileHandle = 1;
	WaveInfoMap _waveInfos;
	uint16 _nextWaveHandle = 1;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_RESOURCE_RESOURCE_H
