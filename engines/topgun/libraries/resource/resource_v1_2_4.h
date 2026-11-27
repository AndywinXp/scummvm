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

#ifndef TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_4_H
#define TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_4_H

#include "topgun/libraries/resource/resource_v1_2.h"

// Container header
#define TG_RES_V1_2_3_1_HEADER_SIZE                0xE8
#define TG_RES_V1_2_3_1_HEADER_PROBE_SIZE          0x56
#define TG_RES_V1_2_3_1_HEADER_SIZE_OFFSET         2
#define TG_RES_V1_2_3_1_HEADER_FORMAT_OFFSET       0x54
#define TG_RES_V1_2_3_1_FORMAT_VERSION             2
#define TG_RES_V1_2_3_1_HEADER_RESIDENT_SPLIT_OFFSET 0x5C
#define TG_RES_V1_2_3_1_HEADER_MAX_FADE_OFFSET     0x64
#define TG_RES_V1_2_3_1_HEADER_MAX_TRANS_OFFSET    0x66
#define TG_RES_V1_2_3_1_HEADER_STRING_COUNT_OFFSET 0x68
#define TG_RES_V1_2_3_1_HEADER_VARIABLE_COUNT_OFFSET 0x6A
#define TG_RES_V1_2_3_1_HEADER_BUILD_TYPE_OFFSET   0x6E
#define TG_RES_V1_2_3_1_HEADER_CHAINED_OFFSET      0x70
#define TG_RES_V1_2_3_1_HEADER_LIBRARY_NAME_OFFSET 0x72
#define TG_RES_V1_2_3_1_HEADER_KEY_TABLE_OFFSET    0x80
#define TG_RES_V1_2_3_1_KEY_ENTRY_SIZE             8
#define TG_RES_V1_2_3_1_KEY_SIZE_OFFSET            4
#define TG_RES_V1_2_3_1_KEY_COUNT                  13

#define TG_RES_V1_2_3_1_KEY_INDEX                  0
#define TG_RES_V1_2_3_1_KEY_VARIABLES              3
#define TG_RES_V1_2_3_1_KEY_STRINGS                4
#define TG_RES_V1_2_3_1_KEY_RESIDENT               5
#define TG_RES_V1_2_3_1_KEY_MOVIE_BUFFER           6
#define TG_RES_V1_2_3_1_KEY_PALETTE                7
#define TG_RES_V1_2_3_1_KEY_NAME_TABLE             8
#define TG_RES_V1_2_3_1_KEY_PLUGIN_NAMES           12
#define TG_RES_V1_2_3_1_PALETTE_ENTRY_SIZE         4

// Resource types: 8 stays in the movie buffer, 9..16 in the resident block
#define TG_RES_V1_2_3_1_MOVIE_BUFFER_TYPE          8
#define TG_RES_V1_2_3_1_RESIDENT_FIRST_TYPE        9
#define TG_RES_V1_2_3_1_RESIDENT_LAST_TYPE         16

#define TG_RES_V1_2_3_1_BITMAP_WIDTH_OFFSET        2
#define TG_RES_V1_2_3_1_MIDI_HEADER_SIZE           8

// Container header
#define TG_RES_V1_2_3_2_HEADER_SIZE                0xFE
#define TG_RES_V1_2_3_2_MAX_RESIDENT_CHUNKS        8
#define TG_RES_V1_2_3_2_HEADER_LIMITS_OFFSET       0x5C
#define TG_RES_V1_2_3_2_HEADER_CHUNK_COUNT_OFFSET  0x80
#define TG_RES_V1_2_3_2_HEADER_MAX_FADE_OFFSET     0x82
#define TG_RES_V1_2_3_2_HEADER_MAX_TRANS_OFFSET    0x84
#define TG_RES_V1_2_3_2_HEADER_EXTRA_SLOTS_OFFSET  0x86
#define TG_RES_V1_2_3_2_HEADER_STRING_COUNT_OFFSET 0x88
#define TG_RES_V1_2_3_2_HEADER_VARIABLE_COUNT_OFFSET 0x8A
#define TG_RES_V1_2_3_2_HEADER_BUILD_TYPE_OFFSET   0x8C
#define TG_RES_V1_2_3_2_HEADER_CHAINED_OFFSET      0x8E
#define TG_RES_V1_2_3_2_HEADER_LIBRARY_NAME_OFFSET 0x90
#define TG_RES_V1_2_3_2_HEADER_KEY_TABLE_OFFSET    0x9E
#define TG_RES_V1_2_3_2_KEY_COUNT                  12
#define TG_RES_V1_2_3_2_KEY_PALETTE                6
#define TG_RES_V1_2_3_2_KEY_NAME_TABLE             7

// Resource types: 1..7 are read from the file, 8..16 stay in memory
#define TG_RES_V1_2_3_2_DISK_FIRST_TYPE            1
#define TG_RES_V1_2_3_2_DISK_LAST_TYPE             7
#define TG_RES_V1_2_3_2_RESIDENT_FIRST_TYPE        8

// Wave and MIDI headers
#define TG_RES_V1_2_3_2_WAVE_HEADER_SIZE           0x1A
#define TG_RES_V1_2_3_2_MIDI_HEADER_SIZE           4

// Extraction prefix: DWORD raw size and compression flag
#define TG_RES_V1_2_3_2_EXTRACT_PREFIX_SIZE        6

// Container header
#define TG_RES_V1_2_4_HEADER_CHUNK_COUNT_OFFSET    0xA0
#define TG_RES_V1_2_4_HEADER_MAX_FADE_OFFSET       0xA2
#define TG_RES_V1_2_4_HEADER_MAX_TRANS_OFFSET      0xA4
#define TG_RES_V1_2_4_HEADER_EXTRA_SLOTS_OFFSET    0xA6
#define TG_RES_V1_2_4_HEADER_STRING_COUNT_OFFSET   0xA8
#define TG_RES_V1_2_4_HEADER_VARIABLE_COUNT_OFFSET 0xAA
#define TG_RES_V1_2_4_HEADER_BUILD_TYPE_OFFSET     0xAC
#define TG_RES_V1_2_4_HEADER_CHAINED_OFFSET        0xAE
#define TG_RES_V1_2_4_HEADER_LIBRARY_NAME_OFFSET   0xB0
#define TG_RES_V1_2_4_HEADER_KEY_TABLE_OFFSET      0xBE
#define TG_RES_V1_2_4_KEY_PLUGIN_NAMES             10

// Bitmap records
#define TG_RES_V1_2_3_1_BITMAP_FLAGS_BYTE_OFFSET   9
#define TG_RES_V1_2_3_1_BITMAP_COMPRESSED_MASK     0x80

// Wave header
#define TG_RES_V1_2_4_WAVE_HEADER_SIZE             0x1C

// Container header with the longer title, the 24 resident chunks and the longer library name
#define TG_RES_V1_2_6_HEADER_PROBE_SIZE            0x58
#define TG_RES_V1_2_6_HEADER_FORMAT_OFFSET         0x56
#define TG_RES_V1_2_6_HEADER_LIMITS_OFFSET         0x5E
#define TG_RES_V1_2_6_HEADER_CHUNK_COUNT_OFFSET    0xC2
#define TG_RES_V1_2_6_HEADER_MAX_FADE_OFFSET       0xC4
#define TG_RES_V1_2_6_HEADER_MAX_TRANS_OFFSET      0xC6
#define TG_RES_V1_2_6_HEADER_EXTRA_SLOTS_OFFSET    0xC8
#define TG_RES_V1_2_6_HEADER_STRING_COUNT_OFFSET   0xCA
#define TG_RES_V1_2_6_HEADER_VARIABLE_COUNT_OFFSET 0xCC
#define TG_RES_V1_2_6_HEADER_BUILD_TYPE_OFFSET     0xCE
#define TG_RES_V1_2_6_HEADER_CHAINED_OFFSET        0xD0
#define TG_RES_V1_2_6_HEADER_LIBRARY_NAME_OFFSET   0xD2
#define TG_RES_V1_2_6_HEADER_KEY_TABLE_OFFSET      0xE2

// Resource types: 1..9 are read from the file, 10..18 stay in memory
#define TG_RES_V1_2_6_DISK_LAST_TYPE               9
#define TG_RES_V1_2_6_RESIDENT_FIRST_TYPE          10
#define TG_RES_V1_2_6_RESIDENT_LAST_TYPE           18
#define TG_RES_V1_2_6_BITMAP_FLAGS_BYTE_OFFSET     8

// Container header with the platform tag, 32 resident chunks, 14 keys and no library name
#define TG_RES_V1_2_12_HEADER_TAG_OFFSET            4
#define TG_RES_V1_2_12_HEADER_TAG_VALUE             0x3631
#define TG_RES_V1_2_12_HEADER_CHUNK_COUNT_OFFSET    0xE2
#define TG_RES_V1_2_12_HEADER_MAX_FADE_OFFSET       0xE4
#define TG_RES_V1_2_12_HEADER_MAX_TRANS_OFFSET      0xE6
#define TG_RES_V1_2_12_HEADER_EXTRA_SLOTS_OFFSET    0xE8
#define TG_RES_V1_2_12_HEADER_STRING_COUNT_OFFSET   0xEA
#define TG_RES_V1_2_12_HEADER_VARIABLE_COUNT_OFFSET 0xEC
#define TG_RES_V1_2_12_HEADER_BUILD_TYPE_OFFSET     0xEE
#define TG_RES_V1_2_12_HEADER_CHAINED_OFFSET        0xF0
#define TG_RES_V1_2_12_HEADER_KEY_TABLE_OFFSET      0xF2
#define TG_RES_V1_2_12_KEY_COUNT                    14

// Resource types: 1..10 are read from the file, 11..20 stay in memory
#define TG_RES_V1_2_12_DISK_LAST_TYPE               10
#define TG_RES_V1_2_12_RESIDENT_FIRST_TYPE          11
#define TG_RES_V1_2_12_RESIDENT_LAST_TYPE           20
#define TG_RES_V1_2_12_SPRITE_TYPE                  18

// Table resources: a block of 0x20 header bytes followed by the data, flagged as owned at +6
#define TG_RES_V1_2_12_TABLE_TYPE                   10
#define TG_RES_V1_2_12_TABLE_HEADER_SIZE            0x20
#define TG_RES_V1_2_12_TABLE_OWNED_OFFSET           6

// MIDI header with a six byte layout
#define TG_RES_V1_2_7_MIDI_HEADER_SIZE             6

// Sprite data records, which are reset when copied in memory
#define TG_RES_V1_2_7_1_SPRITE_TYPE                17
#define TG_RES_V1_2_7_1_SPRITE_HEADER_SIZE         0x14
#define TG_RES_V1_2_7_1_SPRITE_CLICK_SCRIPT        0
#define TG_RES_V1_2_7_1_SPRITE_CLICK_ARGUMENT      2
#define TG_RES_V1_2_7_1_SPRITE_UNK_04              4
#define TG_RES_V1_2_7_1_SPRITE_UNK_06              6
#define TG_RES_V1_2_7_1_SPRITE_DRAG_SCRIPT         8
#define TG_RES_V1_2_7_1_SPRITE_DROP_SCRIPT         0x0A
#define TG_RES_V1_2_7_1_SPRITE_DISABLED            0x10
#define TG_RES_V1_2_7_1_SPRITE_DRAGGABLE           0x13

namespace Topgun {

class TopgunEngine_v1_2;

class ResourceLibrary_v1_2_4 : public ResourceLibrary_v1_2 {
	friend class ResourceLibrary_v1_2;

public:
	ResourceLibrary_v1_2_4(TopgunEngine_v1_2 *engine);

	// Exports with the same role as the previous version ones
	byte *resLoadNameTable(uint16 file, uint32 *outSize) override;
	uint16 resLoadPalette(uint16 file, byte *buffer) override;
	byte *resLoadResource(uint16 file, uint16 index, uint32 *outSize) override;
	const char *resGetConstStr(uint16 file, uint16 offset) override;

	// New exports
	uint16 resCountResources(uint16 file, uint16 *outCount);
	byte resGetBuildType(uint16 file);
	byte *resLoadKeyResource(uint16 file, uint16 key, uint32 *outSize);
	uint16 resCopyResource(uint16 file, uint16 index);
	bool resDeleteResource(uint16 file, uint16 index);

protected:
	// Container
	bool readResourceFileHeader(ResourceFile *file) override;
	bool loadIndexPages(ResourceFile *record);
	bool loadKey3Table(uint16 file, ResourceFile *record);
	bool loadResidentChunks(ResourceFile *record) override;
	bool loadConstStringPool(uint16 file, ResourceFile *record);
	byte *getIndexEntry(ResourceFile *file, uint16 index);

	// Key table of the header
	uint32 keyOffset(const ResourceFile *file, uint16 key);
	uint32 keySize(const ResourceFile *file, uint16 key);
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_4_H
