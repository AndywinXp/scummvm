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

#ifndef TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_3_H
#define TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_3_H

#include "topgun/libraries/resource/resource_v1_2.h"

// Container header
#define TG_RES_V1_2_3_HEADER_PROBE_SIZE            0x56
#define TG_RES_V1_2_3_HEADER_SIZE_OFFSET           2
#define TG_RES_V1_2_3_HEADER_FORMAT_OFFSET         0x54
#define TG_RES_V1_2_3_FORMAT_VERSION               2
#define TG_RES_V1_2_3_HEADER_LIMITS_OFFSET         0x5C
#define TG_RES_V1_2_3_HEADER_CHUNK_COUNT_OFFSET    0xA0
#define TG_RES_V1_2_3_HEADER_MAX_FADE_OFFSET       0xA2
#define TG_RES_V1_2_3_HEADER_MAX_TRANS_OFFSET      0xA4
#define TG_RES_V1_2_3_HEADER_EXTRA_SLOTS_OFFSET    0xA6
#define TG_RES_V1_2_3_HEADER_STRING_COUNT_OFFSET   0xA8
#define TG_RES_V1_2_3_HEADER_VARIABLE_COUNT_OFFSET 0xAA
#define TG_RES_V1_2_3_HEADER_BUILD_TYPE_OFFSET     0xAC
#define TG_RES_V1_2_3_HEADER_CHAINED_OFFSET        0xAE
#define TG_RES_V1_2_3_HEADER_LIBRARY_NAME_OFFSET   0xB0
#define TG_RES_V1_2_3_LIBRARY_NAME_SIZE            14
#define TG_RES_V1_2_3_HEADER_KEY_TABLE_OFFSET      0xBE
#define TG_RES_V1_2_3_KEY_ENTRY_SIZE               8
#define TG_RES_V1_2_3_KEY_SIZE_OFFSET              4
#define TG_RES_V1_2_3_KEY_COUNT                    12

// Key resources
#define TG_RES_V1_2_3_KEY_INDEX                    0
#define TG_RES_V1_2_3_KEY_VARIABLES                3
#define TG_RES_V1_2_3_KEY_STRINGS                  4
#define TG_RES_V1_2_3_KEY_RESIDENT                 5
#define TG_RES_V1_2_3_KEY_PALETTE                  6
#define TG_RES_V1_2_3_KEY_NAME_TABLE               7
#define TG_RES_V1_2_3_KEY_PLUGIN_NAMES             10
#define TG_RES_V1_2_3_VARIABLE_PAIR_SIZE           4
#define TG_RES_V1_2_3_PALETTE_ENTRY_SIZE           4

// Resource types: 1..7 are read from the file, 8..16 stay in memory
#define TG_RES_V1_2_3_DISK_FIRST_TYPE              1
#define TG_RES_V1_2_3_DISK_LAST_TYPE               7
#define TG_RES_V1_2_3_RESIDENT_FIRST_TYPE          8
#define TG_RES_V1_2_3_RESIDENT_LAST_TYPE           16

// Bitmap records
#define TG_RES_V1_2_3_BITMAP_WIDTH_OFFSET          2
#define TG_RES_V1_2_3_BITMAP_FLAGS_BYTE_OFFSET     9
#define TG_RES_V1_2_3_BITMAP_COMPRESSED_MASK       0x80
#define TG_RES_V1_2_3_NOTIFY_BITMAP_ERROR          0x590

// Wave and MIDI headers
#define TG_RES_V1_2_3_WAVE_HEADER_SIZE             0x1C
#define TG_RES_V1_2_3_MIDI_HEADER_SIZE             4

// Extraction prefix: DWORD raw size and WORD compression flag
#define TG_RES_V1_2_3_EXTRACT_PREFIX_SIZE          6
#define TG_RES_V1_2_3_EXTRACT_COMPRESSED_OFFSET    4

namespace Topgun {

class TopgunEngine_v1_2_3;

class ResourceLibrary_v1_2_3 : public ResourceLibrary_v1_2 {
	friend class ResourceLibrary_v1_2;

public:
	ResourceLibrary_v1_2_3(TopgunEngine_v1_2_3 *engine);

	// Exports with the same role as the previous version ones
	byte *resLoadNameTable(uint16 file, uint32 *outSize) override;
	uint16 resLoadPalette(uint16 file, byte *buffer) override;
	byte *resLoadResource(uint16 file, uint16 index, uint32 *outSize) override;
	int32 waveRead(uint16 waveInfo, byte *buffer, int32 count) override;
	uint16 resEnumVariables(uint16 file) override;
	const char *resGetConstStr(uint16 file, uint16 offset) override;

	// New exports
	uint16 resCountResources(uint16 file, uint16 *outCount);
	uint16 resCreateMidiEvent(uint16 file, uint16 index);
	void resSetNotify(uint16 file, uint16 window);
	uint32 resTell(uint16 file);
	uint16 resGetLibraryName(uint16 file, char *destination, uint16 count);
	uint16 resGetMaxFadeColors(uint16 file);
	uint16 resGetMaxTransColors(uint16 file);
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
	void notifyHost(uint16 message, ResourceFile *file);

	// Key table of the header
	static uint32 keyOffset(const ResourceFile *file, uint16 key);
	static uint32 keySize(const ResourceFile *file, uint16 key);

	// Bitmaps
	byte *expandCelBitmapRecord(ResourceFile *file, byte *source, uint32 sourceSize, uint32 *outSize) override;

private:
	TopgunEngine_v1_2_3 *engine() const;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_RESOURCE_RESOURCE_V1_2_3_H
