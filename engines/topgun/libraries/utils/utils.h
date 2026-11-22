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

#ifndef TOPGUN_LIBRARIES_UTILS_UTILS_H
#define TOPGUN_LIBRARIES_UTILS_UTILS_H

#include "common/array.h"
#include "common/hash-ptr.h"
#include "common/hashmap.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"

// Memory error notification
#define TG_UTILS_MEMORY_ERROR_MESSAGE       0x464
#define TG_UTILS_ERROR_OUT_OF_MEMORY        2
#define TG_UTILS_ERROR_ALLOC_ZERO           4
#define TG_UTILS_CURRENT_TASK               1

// Angles are in tenths of a degree
#define TG_UTILS_ANGLE_FULL_TURN            3600
#define TG_UTILS_ANGLE_HALF_TURN            1800
#define TG_UTILS_ANGLE_QUARTER_TURN         900
#define TG_UTILS_ANGLE_THREE_QUARTER_TURN   2700
// This is the exact number the game expects (radians to tenths of a degree, a bit above 1800 / pi)
#define TG_UTILS_ANGLE_SCALE                572.9746936176986

// Processor detection
#define TG_UTILS_CPU_FLAG_286               0x0002
#define TG_UTILS_CPU_FLAG_386               0x0004
#define TG_UTILS_CPU_FLAG_486               0x0008
#define TG_UTILS_CPU_FLAG_086               0x0040
#define TG_UTILS_CPU_FLAG_186               0x0080
#define TG_UTILS_SYSTEM_FLAG_PROTECTED      0x0001
#define TG_UTILS_SYSTEM_FLAG_ENHANCED       0x0020
#define TG_UTILS_CPUID_PROBE_FLAG           0x0020
#define TG_UTILS_PROCESSOR_PENTIUM          5

namespace Topgun {

class TopgunEngine;

class UtilsLibrary {
public:
	UtilsLibrary(TopgunEngine *engine);
	~UtilsLibrary();

	// Memory
	void copyHugeBytes(byte *destination, const byte *source, uint32 count);
	void mapBytes(byte *buffer, uint32 count, const byte *table);
	byte *tgMalloc(uint32 size, uint16 zeroInit);
	byte *tgPtrCpy(const byte *block);
	byte *tgRealloc(byte *block, uint32 newSize, uint16 zeroInit);
	byte *tgFree(byte *block);
	uint16 memSetNotify(uint16 window);

	// Processor
	bool libMain();
	uint8 getProcessorType();

	// Math
	int16 normalizeAngle(int16 angle);
	int16 getQuadrant(int16 angle);
	int16 sine(int16 angle);
	int16 cosine(int16 angle);
	int32 tangent(int16 angle);
	int16 getAngle(int16 y, int16 x);
	uint16 getIntersect(Common::Point *intersection, int16 x1, int16 y1, int16 x2, int16 y2, int16 x3, int16 y3, int16 x4, int16 y4);

private:
	typedef Common::HashMap<byte *, uint32> BlockSizeMap;

	struct NotifyEntry {
		uint16 task;
		uint16 window;

		NotifyEntry() : task(0), window(0) {}
	};

	// Memory error notification
	bool notifyTableAddEntry(uint16 window);
	void notifyTableRemoveEntry(int16 index);
	int16 notifyTableFindCurrentTask();

	// Processor
	uint16 getUtilsProfileInt(const char *key, uint16 defaultValue);
	uint32 getSystemFlags();
	uint16 checkCpuidSupport();
	uint8 detectProcessorType();

	TopgunEngine *_engine;
	BlockSizeMap _blockSizes;
	Common::Array<NotifyEntry> _notifyTable;
	uint16 _pentiumCheckEnabled = 0;
	uint8 _processorType = 0;
};

} // End of namespace Topgun

#endif // TOPGUN_LIBRARIES_UTILS_UTILS_H
