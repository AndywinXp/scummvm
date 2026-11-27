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

#ifndef TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_H
#define TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_H

#include "common/stream.h"

#include "topgun/libraries/resource/resource.h"

// Container header
#define TG_RES_V1_2_HEADER_SIZE                    0x76
#define TG_RES_V1_2_HEADER_TITLE_OFFSET            0x04
#define TG_RES_V1_2_HEADER_TITLE_MAX_SIZE          0x50
#define TG_RES_V1_2_HEADER_RESOURCE_COUNT_OFFSET   0x54
#define TG_RES_V1_2_HEADER_PALETTE_COUNT_OFFSET    0x56
#define TG_RES_V1_2_HEADER_NAME_TABLE_SIZE_OFFSET  0x58
#define TG_RES_V1_2_HEADER_VARIABLE_COUNT_OFFSET   0x5A
#define TG_RES_V1_2_HEADER_CHAINED_OFFSET          0x5C
#define TG_RES_V1_2_HEADER_CHECKSUM_OFFSET         0x5D
#define TG_RES_V1_2_HEADER_VARIABLE_TABLE_OFFSET   0x5E
#define TG_RES_V1_2_HEADER_STRING_COUNT_OFFSET     0x62
#define TG_RES_V1_2_HEADER_STRING_POOL_SIZE_OFFSET 0x64
#define TG_RES_V1_2_HEADER_RESIDENT_SPLIT_OFFSET   0x66
#define TG_RES_V1_2_HEADER_RESIDENT_SIZE_OFFSET    0x6A
#define TG_RES_V1_2_NO_SPLIT                       0xFFFFFFFF

// Container header of the version with the 0x50 byte records (Best Shot)
#define TG_RES_V1_2_1_HEADER_SIZE                  0x7C
#define TG_RES_V1_2_1_HEADER_RECORD_COUNT_OFFSET   0x54
#define TG_RES_V1_2_1_HEADER_INDEX_COUNT_OFFSET    0x56
#define TG_RES_V1_2_1_HEADER_PALETTE_COUNT_OFFSET  0x58
#define TG_RES_V1_2_1_HEADER_MAX_FADE_OFFSET       0x5A
#define TG_RES_V1_2_1_HEADER_MAX_TRANS_OFFSET      0x5C
#define TG_RES_V1_2_1_HEADER_NAME_TABLE_SIZE_OFFSET 0x5E
#define TG_RES_V1_2_1_HEADER_VARIABLE_COUNT_OFFSET 0x60
#define TG_RES_V1_2_1_HEADER_CHAINED_OFFSET        0x62
#define TG_RES_V1_2_1_HEADER_CHECKSUM_OFFSET       0x63
#define TG_RES_V1_2_1_HEADER_VARIABLE_TABLE_OFFSET 0x64
#define TG_RES_V1_2_1_HEADER_STRING_COUNT_OFFSET   0x68
#define TG_RES_V1_2_1_HEADER_STRING_POOL_SIZE_OFFSET 0x6A
#define TG_RES_V1_2_1_HEADER_RESIDENT_SPLIT_OFFSET 0x6C
#define TG_RES_V1_2_1_HEADER_RESIDENT_SIZE_OFFSET  0x70
#define TG_RES_V1_2_1_RECORD_SIZE                  0x50
#define TG_RES_V1_2_1_NOTIFY_BITMAP_ERROR          0x590

// The German and French TuneLand add 14 bytes to that header, which the library does not read
#define TG_RES_V1_2_2_HEADER_SIZE                  0x8A

// Container header of the version with the table of block positions (Peter and the Wolf)
#define TG_RES_V1_2_3_HEADER_SIZE                  0xB8
#define TG_RES_V1_2_3_HEADER_VARIABLE_PAIR_COUNT_OFFSET 0x66
#define TG_RES_V1_2_3_HEADER_STRING_COUNT_OFFSET   0x6A
#define TG_RES_V1_2_3_HEADER_STRING_POOL_SIZE_OFFSET 0x6C
#define TG_RES_V1_2_3_HEADER_RESIDENT_SPLIT_OFFSET 0x6E
#define TG_RES_V1_2_3_HEADER_RESIDENT_SIZE_OFFSET  0x72
#define TG_RES_V1_2_3_HEADER_MOVIE_BUFFER_SIZE_OFFSET 0x76
#define TG_RES_V1_2_3_HEADER_INDEX_POSITION_OFFSET 0x82
#define TG_RES_V1_2_3_HEADER_RECORDS_POSITION_OFFSET 0x86
#define TG_RES_V1_2_3_HEADER_VARIABLE_TABLE_POSITION_OFFSET 0x8A
#define TG_RES_V1_2_3_HEADER_VARIABLE_PAIRS_POSITION_OFFSET 0x8E
#define TG_RES_V1_2_3_HEADER_STRING_POOL_POSITION_OFFSET 0x92
#define TG_RES_V1_2_3_HEADER_RESIDENT_POSITION_OFFSET 0x96
#define TG_RES_V1_2_3_HEADER_MOVIE_BUFFER_POSITION_OFFSET 0x9A
#define TG_RES_V1_2_3_HEADER_PALETTE_POSITION_OFFSET 0x9E
#define TG_RES_V1_2_3_HEADER_NAME_TABLE_POSITION_OFFSET 0xA2
#define TG_RES_V1_2_3_HEADER_LIBRARY_NAME_OFFSET   0xAA

// Resident types
#define TG_RES_V1_2_RESIDENT_FIRST_TYPE            5
#define TG_RES_V1_2_RESIDENT_LAST_TYPE             12
#define TG_RES_V1_2_DISK_TYPE_IN_RESIDENT_RANGE    10
#define TG_RES_V1_2_1_RESIDENT_LAST_TYPE           13
#define TG_RES_V1_2_3_MOVIE_BUFFER_TYPE            6
#define TG_RES_V1_2_3_RESIDENT_FIRST_TYPE          7
#define TG_RES_V1_2_3_RESIDENT_LAST_TYPE           14

// Variable table records
#define TG_RES_V1_2_VARIABLE_RECORD_SIZE           0x1F
#define TG_RES_V1_2_VARIABLE_VALUE_OFFSET          2
#define TG_RES_V1_2_VARIABLE_USED_OFFSET           4
#define TG_RES_V1_2_VARIABLE_NAME_OFFSET           5
#define TG_RES_V1_2_VARIABLE_NAME_SIZE             26
#define TG_RES_V1_2_3_VARIABLE_RECORD_SIZE         0x1C
#define TG_RES_V1_2_3_VARIABLE_NAME_OFFSET         2
#define TG_RES_V1_2_3_VARIABLE_PAIR_SIZE           4
#define TG_RES_V1_2_3_MIDI_HEADER_SIZE             6
#define TG_RES_V1_2_3_LIBRARY_NAME_SIZE            14

// Chained blocks: 0x1000 bytes, 0xFFC of data and 4 of link
#define TG_RES_V1_2_CHAIN_DATA_SIZE                0xFFC
#define TG_RES_V1_2_CHAIN_LINK_SIZE                4

#define TG_RES_V1_2_EXTRACT_BLOCK_SIZE             0x2000
#define TG_RES_V1_2_EXTRACT_PREFIX_SIZE            5
#define TG_RES_V1_2_EXTRACT_COMPRESSED_OFFSET      4
#define TG_RES_V1_2_WAVE_HEADER_SIZE               0x18
#define TG_RES_V1_2_SIZE_UNKNOWN                   0xFFFFFFFF

// Bitmap records
#define TG_RES_V1_2_BITMAP_HEADER_SIZE             0x14
#define TG_RES_V1_2_BITMAP_PITCH_OFFSET            4
#define TG_RES_V1_2_BITMAP_HEIGHT_OFFSET           6

// Segment stream tags: upper three bits are the mode, lower thirteen the length
#define TG_RES_V1_2_SEGMENT_MODE_MASK              0xE000
#define TG_RES_V1_2_SEGMENT_LENGTH_MASK            0x1FFF
#define TG_RES_V1_2_SEGMENT_RAW                    0x0000
#define TG_RES_V1_2_SEGMENT_RLE                    0x2000
#define TG_RES_V1_2_SEGMENT_LZW_10_BITS            0x4000
#define TG_RES_V1_2_SEGMENT_LZW_11_BITS            0x6000
#define TG_RES_V1_2_SEGMENT_LZW_12_BITS            0x8000
#define TG_RES_V1_2_SEGMENT_END                    0xE000

// LZW dictionary: three tables of 0x139D entries, allocated as a block of 7 * 0x139D bytes
#define TG_RES_V1_2_LZW_TABLE_SIZE                 0x139D
#define TG_RES_V1_2_LZW_ALLOCATION_FACTOR          7
#define TG_RES_V1_2_LZW_FIRST_ENTRY                0x100
#define TG_RES_V1_2_LZW_MASK_COUNT                 8
#define TG_RES_V1_2_LZW_MIN_CODE_WIDTH             9
#define TG_RES_V1_2_LZW_MAX_CODE_WIDTH             12
#define TG_RES_V1_2_LZW_DEFAULT_CODE_WIDTH         11

// Container header of the later version, which has its own library
#define TG_RES_V1_2_4_HEADER_SIZE                  0x11E
#define TG_RES_V1_2_4_MAX_RESIDENT_CHUNKS          16

// Container header of the version after that: longer title, 24 resident chunks, longer library name
#define TG_RES_V1_2_6_HEADER_SIZE                  0x142
#define TG_RES_V1_2_6_MAX_RESIDENT_CHUNKS          24
#define TG_RES_V1_2_6_HEADER_TITLE_MAX_SIZE        0x52
#define TG_RES_V1_2_6_LIBRARY_NAME_SIZE            16

// Container header of the version after that: platform tag, title moved after it, 32 resident chunks, no library name
#define TG_RES_V1_2_12_HEADER_SIZE                 0x162
#define TG_RES_V1_2_12_MAX_RESIDENT_CHUNKS         32
#define TG_RES_V1_2_12_HEADER_TITLE_OFFSET         0x06
#define TG_RES_V1_2_12_HEADER_TITLE_MAX_SIZE       0x50

namespace Topgun {

class TopgunEngine_v1_2;

class ResourceLibrary_v1_2 : public ResourceLibrary {
public:
	ResourceLibrary_v1_2(TopgunEngine_v1_2 *engine);
	~ResourceLibrary_v1_2() override;

	// Exports with the same role as the base library ones
	uint16 resCloseFile(uint16 file) override;
	uint16 resCreateWaveEvent(uint16 file, uint16 index) override;
	byte *resLoadNameTable(uint16 file, uint32 *outSize) override;
	uint16 resLoadPalette(uint16 file, byte *buffer) override;
	byte *resLoadResource(uint16 file, uint16 index, uint32 *outSize) override;
	uint16 resOpenFile(const Common::String &path) override;
	uint32 resSeekResource(uint16 file, uint16 index) override;
	int32 waveRead(uint16 waveInfo, byte *buffer, int32 count) override;

	// New exports
	virtual byte *resExpandBitmap(uint16 file, byte *bitmap);
	uint32 resReadData(uint16 file, byte *buffer, uint32 count, uint32 offset);
	virtual bool resExtractFile(uint16 file, uint16 index, Common::WriteStream *output);
	uint16 resEnumResources(uint16 file);
	virtual uint16 resEnumVariables(uint16 file);
	bool resSetVariable(uint16 file, const char *name, uint16 value);
	virtual uint16 resGetIndex(uint16 file, const char *name);
	bool resGetVariable(uint16 file, const char *name, uint16 *value);
	uint16 resGetTitle(uint16 file, byte *destination, uint16 count);
	uint16 resGetHeaderRec(uint16 file, byte *destination, uint16 count);
	virtual const char *resGetConstStr(uint16 file, uint16 offset);
	uint16 resCountStrings(uint16 file);
	void resSetNotify(uint16 file, uint16 window);
	uint16 resCreateMidiEvent(uint16 file, uint16 index);
	uint32 resTell(uint16 file);
	uint16 resGetLibraryName(uint16 file, char *destination, uint16 count);
	uint16 resGetMaxFadeColors(uint16 file);
	uint16 resGetMaxTransColors(uint16 file);

	// Wave stream callbacks
	void waveDone(uint16 waveInfo);
	uint32 waveRewind(uint16 waveInfo);

	// Global memory allocator
	byte *swinFree(byte *block);
	byte *swinMalloc(uint32 size, uint16 zeroInit);
	byte *swinRealloc(byte *block, uint32 size, uint16 zeroInit);

protected:
	struct LzwState {
		byte *dictionary;
		uint16 *prefix;
		byte *suffix;
		byte *length;
		uint16 unusedBits;
		uint16 endCode;
		uint16 maxEntry;
		uint16 codeWidth;
		uint16 tableSize;
		uint16 firstChar;
		uint16 currentCode;
		uint16 previousCode;
		uint16 nextEntry;
		uint16 maskIndex;
		uint16 masks[TG_RES_V1_2_LZW_MASK_COUNT];
		byte leftover;

		LzwState() : dictionary(nullptr), prefix(nullptr), suffix(nullptr), length(nullptr), unusedBits(0), endCode(0),
			maxEntry(0), codeWidth(0), tableSize(0), firstChar(0), currentCode(0), previousCode(0), nextEntry(0),
			maskIndex(0), leftover(0) {
			for (uint16 i = 0; i < TG_RES_V1_2_LZW_MASK_COUNT; i++)
				masks[i] = 0;
		}
	};

	struct DecodeLimits {
		const byte *sourceEnd;
		byte *destinationEnd;
		bool failed;

		DecodeLimits(const byte *sourceLimit, byte *destinationLimit) : sourceEnd(sourceLimit),
			destinationEnd(destinationLimit), failed(false) {}

		bool sourceAvailable(const byte *position, uint32 count) const {
			if (!sourceEnd)
				return true;

			return position <= sourceEnd && (uint32)(sourceEnd - position) >= count;
		}

		bool destinationAvailable(const byte *position, uint32 count) const {
			return position <= destinationEnd && (uint32)(destinationEnd - position) >= count;
		}
	};

	struct ResourceFile : public ResourceLibrary::ResourceFile {
		byte header[TG_RES_V1_2_3_HEADER_SIZE];
		uint16 recordCount;
		uint16 paletteCount;
		uint16 nameTableSize;
		bool chained;
		uint16 variableTableCount;
		uint16 stringCount;
		uint16 stringPoolSize;
		uint32 residentSplit;
		uint32 residentSize;
		byte *residentChunk1;
		byte *residentChunk2;
		byte *movieBuffer;
		byte *variables;
		byte *stringPool;
		uint16 chainOffset;
		bool variablesDirty;
		LzwState *lzwState;

		// Fields of the later version
		byte fullHeader[TG_RES_V1_2_12_HEADER_SIZE];
		uint32 limits[TG_RES_V1_2_12_MAX_RESIDENT_CHUNKS + 1];
		byte *residentChunks[TG_RES_V1_2_12_MAX_RESIDENT_CHUNKS];
		byte *variablePairs;
		uint16 chunkCount;
		uint16 maxFadeColors;
		uint16 maxTransparentColors;
		uint16 extraSlots;
		uint16 buildType;
		uint16 indexCount;
		uint16 variablePairCount;
		uint16 notifyWindow;
		Common::HashMap<uint16, byte *> dynamicResident;

		ResourceFile() : recordCount(0), paletteCount(0), nameTableSize(0), chained(false), variableTableCount(0), stringCount(0),
			stringPoolSize(0), residentSplit(0), residentSize(0), residentChunk1(nullptr), residentChunk2(nullptr),
			movieBuffer(nullptr), variables(nullptr), stringPool(nullptr), chainOffset(0), variablesDirty(false), lzwState(nullptr),
			variablePairs(nullptr), chunkCount(0), maxFadeColors(0), maxTransparentColors(0), extraSlots(0),
			buildType(0), indexCount(0), variablePairCount(0), notifyWindow(0) {
			for (uint16 i = 0; i < TG_RES_V1_2_3_HEADER_SIZE; i++)
				header[i] = 0;

			for (uint16 i = 0; i < TG_RES_V1_2_12_HEADER_SIZE; i++)
				fullHeader[i] = 0;

			for (uint16 i = 0; i <= TG_RES_V1_2_12_MAX_RESIDENT_CHUNKS; i++)
				limits[i] = 0;

			for (uint16 i = 0; i < TG_RES_V1_2_12_MAX_RESIDENT_CHUNKS; i++)
				residentChunks[i] = nullptr;
		}
	};

	struct WaveStreamInfo {
		uint16 file;
		uint32 position;
		uint32 startPosition;
		uint16 chainOffset;
		uint16 startChainOffset;

		WaveStreamInfo() : file(0), position(0), startPosition(0), chainOffset(0), startChainOffset(0) {}
	};

	typedef Common::HashMap<uint16, WaveStreamInfo *> WaveStreamMap;

	// Container
	ResourceFile *findFile(uint16 file, const char *caller) override;
	virtual bool readResourceFileHeader(ResourceFile *file);
	bool loadVariableTable(ResourceFile *file);
	bool loadVariablePairs(ResourceFile *file);
	bool loadStringPool(ResourceFile *file);
	virtual bool loadResidentChunks(ResourceFile *file);
	bool loadMovieBuffer(uint16 file, ResourceFile *record);
	void writeVariableTableBack(ResourceFile *file);
	void notifyHost(uint16 message, ResourceFile *file);
	bool readChainedBlocks(ResourceFile *file, byte *buffer, uint32 count);
	bool readResourceBytes(ResourceFile *file, byte *buffer, uint32 count);
	bool readChunkedWithCallback(ResourceFile *file, byte *buffer, uint32 count);

	// Wave streams
	uint16 createWaveStreamInfo(uint16 file, ResourceFile *record);
	uint16 registerWaveInfo(WaveStreamInfo *info);

	// Bitmaps
	byte *expandCelBitmapRecord(ResourceFile *file, byte *source, uint32 sourceSize, uint32 *outSize);
	bool extractCompressedResourceToFile(uint16 file, ResourceFile *record, Common::WriteStream *output, uint32 compressedSize, uint32 rawSize);

	// Segment stream decoder
	LzwState *lzwCreateStateFar();
	void lzwFreeStateFar(LzwState *state);
	LzwState *lzwAllocState();
	void lzwFreeState(LzwState *state);
	static uint32 decodeSegmentedBitmapStream(LzwState *state, const byte *source, byte *destination, DecodeLimits &limits);
	bool decodeSegmentedBitmapStream(LzwState *state, const byte *source, byte *destination, uint32 remaining, const byte *sourceEnd);
	static void rleExpandBitmap(byte *destination, uint32 destinationSize, const byte *source, uint32 sourceSize);
	static void lzwInitCodeWidth(LzwState *state, uint16 bits);
	static uint16 lzwReadCode(LzwState *state, const byte *&source, DecodeLimits &limits);
	static uint16 lzwWriteString(LzwState *state, uint16 code, byte *&output, DecodeLimits &limits);
	static uint32 lzwDecodeChunk(LzwState *state, const byte *source, byte *destination, DecodeLimits &limits);
	static uint32 lzwDecodeSubChunks(LzwState *state, int32 length, const byte *source, byte *destination, DecodeLimits &limits);
	static uint32 decodeRleSegment(byte *destination, const byte *source, uint32 length, DecodeLimits &limits);

	WaveStreamMap _waveStreams;
	uint16 _nextWaveStreamHandle = 1;

	// Header size of the containers with the 0x50 byte records
	uint16 _recordsHeaderSize = TG_RES_V1_2_1_HEADER_SIZE;

	TopgunEngine_v1_2 *engine() const;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_H
