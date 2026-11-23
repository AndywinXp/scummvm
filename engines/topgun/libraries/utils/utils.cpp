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

#include "common/config-manager.h"
#include "common/textconsole.h"

#include "topgun/libraries/utils/utils.h"
#include "topgun/libraries/utils/utils_tables.h"
#include "topgun/topgun.h"

namespace Topgun {

UtilsLibrary::UtilsLibrary(TopgunEngine *engine) : _engine(engine) {
	if (_engine->_version >= kVersionV1_2)
		libMain();
}

UtilsLibrary::~UtilsLibrary() {
}

void UtilsLibrary::copyHugeBytes(byte *destination, const byte *source, uint32 count) {
	memmove(destination, source, count);
}

void UtilsLibrary::mapBytes(byte *buffer, uint32 count, const byte *table) {
	for (uint32 i = 0; i < count; i++)
		buffer[i] = table[buffer[i]];
}

byte *UtilsLibrary::tgMalloc(uint32 size, uint16 zeroInit) {
	if (size == 0) {
		if (_engine->_version >= kVersionV1_2) {
			int16 index = notifyTableFindCurrentTask();
			if (index >= 0 && _notifyTable[index].window != 0)
				_engine->postMessage(TG_UTILS_MEMORY_ERROR_MESSAGE, TG_UTILS_ERROR_ALLOC_ZERO, 0);

			return nullptr;
		}

		if (_engine->_version >= kVersionV1_1)
			error("UtilsLibrary::tgMalloc(): invalid zero size");

		return nullptr;
	}

	byte *block = (byte *)malloc(size);
	if (!block) {
		if (_engine->_version >= kVersionV1_2) {
			int16 index = notifyTableFindCurrentTask();
			if (index >= 0 && _notifyTable[index].window != 0)
				_engine->postMessage(TG_UTILS_MEMORY_ERROR_MESSAGE, TG_UTILS_ERROR_OUT_OF_MEMORY, 0);

			return nullptr;
		}

		error("UtilsLibrary::tgMalloc(): not enough memory to allocate %u bytes", size);
	}

	if (zeroInit != 0)
		memset(block, 0, size);

	_blockSizes[block] = size;
	return block;
}

byte *UtilsLibrary::tgPtrCpy(const byte *block) {
	if (!block)
		return nullptr;

	if (!_blockSizes.contains((byte *)block)) {
		warning("UtilsLibrary::tgPtrCpy(): block was not allocated by tgMalloc");
		return nullptr;
	}

	uint32 size = _blockSizes[(byte *)block];
	byte *copy = tgMalloc(size, 0);
	memcpy(copy, block, size);
	return copy;
}

byte *UtilsLibrary::tgRealloc(byte *block, uint32 newSize, uint16 zeroInit) {
	if (!block)
		return tgMalloc(newSize, zeroInit);

	if (!_blockSizes.contains(block)) {
		warning("UtilsLibrary::tgRealloc(): block was not allocated by tgMalloc");
		return nullptr;
	}

	if (newSize == 0) {
		if (_engine->_version >= kVersionV1_2) {
			int16 index = notifyTableFindCurrentTask();
			if (index >= 0 && _notifyTable[index].window != 0)
				_engine->postMessage(TG_UTILS_MEMORY_ERROR_MESSAGE, TG_UTILS_ERROR_ALLOC_ZERO, 0);

			return block;
		}

		error("UtilsLibrary::tgRealloc(): invalid zero size");
	}

	uint32 oldSize = _blockSizes[block];
	byte *newBlock = (byte *)realloc(block, newSize);
	if (!newBlock) {
		if (_engine->_version >= kVersionV1_2) {
			int16 index = notifyTableFindCurrentTask();
			if (index >= 0 && _notifyTable[index].window != 0)
				_engine->postMessage(TG_UTILS_MEMORY_ERROR_MESSAGE, TG_UTILS_ERROR_OUT_OF_MEMORY, 0);

			return nullptr;
		}

		error("UtilsLibrary::tgRealloc(): not enough memory to allocate %u bytes", newSize);
	}

	if (zeroInit != 0 && newSize > oldSize)
		memset(newBlock + oldSize, 0, newSize - oldSize);

	_blockSizes.erase(block);
	_blockSizes[newBlock] = newSize;
	return newBlock;
}

byte *UtilsLibrary::tgFree(byte *block) {
	if (!block)
		return nullptr;

	if (!_blockSizes.contains(block)) {
		warning("UtilsLibrary::tgFree(): block was not allocated by tgMalloc");
		return nullptr;
	}

	_blockSizes.erase(block);
	free(block);
	return nullptr;
}

uint16 UtilsLibrary::memSetNotify(uint16 window) {
	int16 index = notifyTableFindCurrentTask();
	if (window == 0) {
		if (index < 0)
			return 0;

		notifyTableRemoveEntry(index);
		return 1;
	}

	if (index < 0)
		return notifyTableAddEntry(window) ? 1 : 0;

	_notifyTable[index].window = window;
	return 1;
}

bool UtilsLibrary::notifyTableAddEntry(uint16 window) {
	NotifyEntry entry;
	entry.task = TG_UTILS_CURRENT_TASK;
	entry.window = window;
	_notifyTable.push_back(entry);
	return true;
}

void UtilsLibrary::notifyTableRemoveEntry(int16 index) {
	_notifyTable.remove_at(index);
}

int16 UtilsLibrary::notifyTableFindCurrentTask() {
	if (_notifyTable.empty())
		return -1;

	uint16 task = TG_UTILS_CURRENT_TASK;
	for (int16 i = 0; i < (int16)_notifyTable.size(); i++) {
		// The task of the entry is overwritten with the current one, so the first entry always matches
		_notifyTable[i].task = task;
		if (_notifyTable[i].task != 0)
			return i;
	}

	return -1;
}

bool UtilsLibrary::libMain() {
	_pentiumCheckEnabled = getUtilsProfileInt("Pentium", 1);
	_processorType = detectProcessorType();
	return true;
}

uint8 UtilsLibrary::getProcessorType() {
	return _processorType;
}

uint16 UtilsLibrary::getUtilsProfileInt(const char *key, uint16 defaultValue) {
	Common::String name = key;
	name.toLowercase();
	if (ConfMan.hasKey(name))
		return (uint16)ConfMan.getInt(name);

	return defaultValue;
}

uint32 UtilsLibrary::getSystemFlags() {
	return TG_UTILS_SYSTEM_FLAG_PROTECTED | TG_UTILS_CPU_FLAG_486 | TG_UTILS_SYSTEM_FLAG_ENHANCED;
}

uint16 UtilsLibrary::checkCpuidSupport() {
	return TG_UTILS_CPUID_PROBE_FLAG;
}

uint8 UtilsLibrary::detectProcessorType() {
	uint16 flags = (uint16)getSystemFlags();
	if (flags & TG_UTILS_CPU_FLAG_086)
		return 0;

	if (flags & TG_UTILS_CPU_FLAG_186)
		return 1;

	if (flags & TG_UTILS_CPU_FLAG_286)
		return 2;

	if (flags & TG_UTILS_CPU_FLAG_386)
		return 3;

	if (!(flags & TG_UTILS_CPU_FLAG_486))
		return 0;

	if (_pentiumCheckEnabled != 0 && checkCpuidSupport() != 0)
		return TG_UTILS_PROCESSOR_PENTIUM;

	return 4;
}

int16 UtilsLibrary::normalizeAngle(int16 angle) {
	int16 result = angle % TG_UTILS_ANGLE_FULL_TURN;
	if (result < 0)
		result += TG_UTILS_ANGLE_FULL_TURN;

	return result;
}

int16 UtilsLibrary::getQuadrant(int16 angle) {
	if (angle < TG_UTILS_ANGLE_QUARTER_TURN)
		return 0;

	if (angle < TG_UTILS_ANGLE_HALF_TURN)
		return 1;

	if (angle < TG_UTILS_ANGLE_THREE_QUARTER_TURN)
		return 2;

	return 3;
}

int16 UtilsLibrary::sine(int16 angle) {
	angle = normalizeAngle(angle);
	bool negative = (angle >= TG_UTILS_ANGLE_HALF_TURN);
	if (negative)
		angle -= TG_UTILS_ANGLE_HALF_TURN;

	int16 index = angle % TG_UTILS_ANGLE_QUARTER_TURN;
	if (angle >= TG_UTILS_ANGLE_QUARTER_TURN)
		index = TG_UTILS_ANGLE_QUARTER_TURN - index;

	int16 value = kSineTable[index];
	return negative ? -value : value;
}

int16 UtilsLibrary::cosine(int16 angle) {
	angle = normalizeAngle(angle - TG_UTILS_ANGLE_QUARTER_TURN);
	int16 index = angle % TG_UTILS_ANGLE_QUARTER_TURN;
	bool negative = (angle < TG_UTILS_ANGLE_HALF_TURN);
	if (!negative)
		angle -= TG_UTILS_ANGLE_HALF_TURN;

	if (angle >= TG_UTILS_ANGLE_QUARTER_TURN)
		index = TG_UTILS_ANGLE_QUARTER_TURN - index;

	int16 value = kSineTable[index];
	return negative ? -value : value;
}

int32 UtilsLibrary::tangent(int16 angle) {
	angle = normalizeAngle(angle);
	bool negative = (angle > TG_UTILS_ANGLE_HALF_TURN);
	if (negative)
		angle -= TG_UTILS_ANGLE_HALF_TURN;

	int32 value;
	if (angle == TG_UTILS_ANGLE_HALF_TURN) {
		// One entry past the end of the tangent table: Monty Python reads the first two sine table values, later games read out of bounds data
		if (_engine->_version >= kVersionV1_2_3)
			warning("UtilsLibrary::tangent(): the angle is one entry past the end of the tangent table");

		value =(int32)(((uint32)(uint16)kSineTable[1] << 16) | (uint16)kSineTable[0]);
	} else {
		value = kTangentTable[angle];
	}

	return negative ? -value : value;
}

int16 UtilsLibrary::getAngle(int16 y, int16 x) {
	long double radians = atan2((long double)y, (long double)x);
	long double scaled = radians * (long double)TG_UTILS_ANGLE_SCALE;
	int16 angle = 0;
	if (scaled >= (long double)INT64_MIN && scaled < (long double)INT64_MAX + 1)
		angle = (int16)(int64)scaled;
	else
		warning("UtilsLibrary::getAngle(): the angle is out of range");

	if (angle < 0)
		angle += TG_UTILS_ANGLE_FULL_TURN;

	return angle;
}

uint16 UtilsLibrary::getIntersect(Common::Point *intersection, int16 x1, int16 y1, int16 x2, int16 y2, int16 x3, int16 y3, int16 x4, int16 y4) {
	int16 kindA = (x2 == x1) ? -1 : ((y2 == y1) ? 1 : 0);
	int16 kindB = (x4 == x3) ? -1 : ((y4 == y3) ? 1 : 0);
	if (kindA == kindB && kindB != 0)
		return 0;

	long double slopeAExtended = 0.0L;
	if (kindA == 0)
		slopeAExtended = (long double)(int16)(y1 - y2) / (long double)(int16)(x1 - x2);

	double slopeA = (double)slopeAExtended;

	long double slopeBExtended = 0.0L;
	if (kindB == 0)
		slopeBExtended = (long double)(int16)(y3 - y4) / (long double)(int16)(x3 - x4);

	double slopeB = (double)slopeBExtended;

	// The second slope is compared before being rounded to a double
	if (slopeBExtended == (long double)slopeA)
		return 0;

	double interceptA = (double)((long double)y1 - (long double)x1 * (long double)slopeA);
	double interceptB = (double)((long double)y3 - (long double)x3 * (long double)slopeB);

	int16 resultX = 0;
	int16 resultY = 0;
	long double valueX = 0.0L;
	long double valueY = 0.0L;
	bool computeX = false;
	bool computeY = false;

	if (kindA == 0) {
		if (kindB == 0) {
			long double denominator = (long double)slopeA - (long double)slopeB;
			if (denominator == 0.0L) {
				// Guard against a division by zero
				warning("UtilsLibrary::getIntersect(): the slopes are equal");
			} else {
				valueX = ((long double)interceptB - (long double)interceptA) / denominator;
			}

			computeX = true;
		} else if (kindB == -1) {
			resultX = x3;
		} else {
			valueX = ((long double)y3 - (long double)interceptA) / (long double)slopeA;
			computeX = true;
		}
	} else if (kindA == -1) {
		resultX = x1;
	} else if (kindB != 0) {
		resultX = x3;
	} else {
		valueX = ((long double)y1 - (long double)interceptB) / (long double)slopeB;
		computeX = true;
	}

	// NaN and values that do not fit in 64 bits give 0
	if (computeX) {
		if (valueX >= (long double)INT64_MIN && valueX < (long double)INT64_MAX + 1)
			resultX = (int16)(int64)valueX;
		else
			warning("UtilsLibrary::getIntersect(): the X coordinate is out of range");
	}

	if (kindA == 0 && kindB != 1) {
		valueY = (long double)resultX * (long double)slopeA + (long double)interceptA;
		computeY = true;
	} else if (kindA == -1 && kindB == 0) {
		valueY = (long double)resultX * (long double)slopeB + (long double)interceptB;
		computeY = true;
	} else if (kindA == 1) {
		resultY = y1;
	} else {
		resultY = y3;
	}

	if (computeY) {
		if (valueY >= (long double)INT64_MIN && valueY < (long double)INT64_MAX + 1)
			resultY = (int16)(int64)valueY;
		else
			warning("UtilsLibrary::getIntersect(): the Y coordinate is out of range");
	}

	intersection->x = resultX;
	intersection->y = resultY;
	return 1;
}

void UtilsLibrary::collide(int16 x1, int16 y1, int16 x2, int16 y2, int16 *speed1, int16 *angle1, int16 *speed2, int16 *angle2) {
	if (*speed1 == 0 && *speed2 == 0)
		return;

	int16 axis;
	if (y1 == TG_UTILS_COLLIDE_AXIS_MARKER)
		axis = x1;
	else
		axis = normalizeAngle(getAngle((int16)(x2 - x1), (int16)(y2 - y1)));

	*angle1 = (int16)(*angle1 - axis);
	*angle2 = (int16)(*angle2 - axis);

	int32 first = (int32)cosine(*angle1) * (int32)*speed1 / TG_UTILS_TRIG_SCALE;
	int32 second = (int32)sine(*angle1) * (int32)*speed1 / TG_UTILS_TRIG_SCALE;
	int32 third = (int32)cosine(*angle2) * (int32)*speed2 / TG_UTILS_TRIG_SCALE;
	int32 fourth = (int32)sine(*angle2) * (int32)*speed2 / TG_UTILS_TRIG_SCALE;

	int32 sum = (int32)((uint32)third * (uint32)third + (uint32)second * (uint32)second);
	long double root = sqrt((long double)sum);
	*speed1 = 0;
	if (root >= (long double)INT64_MIN && root < (long double)INT64_MAX + 1)
		*speed1 = (int16)(int64)root;
	else
		warning("UtilsLibrary::collide(): the first speed is out of range");

	sum = (int32)((uint32)fourth * (uint32)fourth + (uint32)first * (uint32)first);
	root = sqrt((long double)sum);
	*speed2 = 0;
	if (root >= (long double)INT64_MIN && root < (long double)INT64_MAX + 1)
		*speed2 = (int16)(int64)root;
	else
		warning("UtilsLibrary::collide(): the second speed is out of range");

	long double radians = atan2((long double)second, (long double)third) * (long double)TG_UTILS_ANGLE_SCALE;
	int16 newAngle = 0;
	if (radians >= (long double)INT64_MIN && radians < (long double)INT64_MAX + 1)
		newAngle = (int16)(int64)radians;
	else
		warning("UtilsLibrary::collide(): the first angle is out of range");

	*angle1 = normalizeAngle((int16)(newAngle + axis));

	radians = atan2((long double)fourth, (long double)first) * (long double)TG_UTILS_ANGLE_SCALE;
	newAngle = 0;
	if (radians >= (long double)INT64_MIN && radians < (long double)INT64_MAX + 1)
		newAngle = (int16)(int64)radians;
	else
		warning("UtilsLibrary::collide(): the second angle is out of range");

	*angle2 = normalizeAngle((int16)(newAngle + axis));
}

int32 UtilsLibrary::distance(int16 x1, int16 y1, int16 x2, int16 y2) {
	int32 deltaX = (int16)(x2 - x1);
	int32 deltaY = (int16)(y2 - y1);
	int32 sum = (int32)((uint32)deltaX * (uint32)deltaX + (uint32)deltaY * (uint32)deltaY);
	long double root = sqrt((long double)sum);
	if (root >= (long double)INT64_MIN && root < (long double)INT64_MAX + 1)
		return (int32)(int64)root;

	// NaN and values that do not fit in 64 bits give 0
	warning("UtilsLibrary::distance(): the distance is out of range");
	return 0;
}

int32 UtilsLibrary::squareRoot(int16 value) {
	long double root = sqrt((long double)value);
	if (root >= (long double)INT64_MIN && root < (long double)INT64_MAX + 1)
		return (int32)(int64)root;

	warning("UtilsLibrary::squareRoot(): the value is out of range");
	return 0;
}

int16 UtilsLibrary::reflectAngle(int16 angle, int16 axis) {
	return normalizeAngle((int16)(2 * axis - angle));
}

void UtilsLibrary::copyHugeStr(char *destination, const char *source) {
	char character;
	do {
		character = *source++;
		*destination++ = character;
	} while (character != 0);
}

uint16 UtilsLibrary::hugeStrLen(const char *string) {
	uint16 length = 0xFFFF;
	do {
		length++;
	} while (*string++ != 0);

	return length;
}

} // End of namespace Topgun
