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
#include "common/events.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/display.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

void TopgunEngine_v1_2::o_setScriptTimer(const byte *record) {
	uint32 delay;

	if (record[0x0A] != 0)
		delay = operandValue(READ_LE_UINT16(record + 6));
	else
		delay = READ_LE_UINT32(record + 6);

	uint16 id = READ_LE_UINT16(record + 2);
	uint16 index = 0;

	if (_numScheduledTimers != 0) {
		do {
			if (_scheduledTimers[index].id == id)
				break;

			index++;
		} while (index < _numScheduledTimers);
	}

	if (index >= TG_HOST_V1_2_MAX_SCHEDULED_TIMERS)
		return;

	if (_numScheduledTimers == index)
		_numScheduledTimers++;

	ScheduledTimer_v1_2 &timer = _scheduledTimers[index];

	timer.id = id;
	timer.delay = delay;
	timer.expiry = getEngineMillis() + delay;
	timer.script = READ_LE_UINT16(record + 4);
	timer.repeat = record[0x0B];
}

void TopgunEngine_v1_2::o_cancelScriptTimer(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);
	uint16 index = 0;

	if (_numScheduledTimers != 0) {
		do {
			if (_scheduledTimers[index].id == id)
				break;

			index++;
		} while (index < _numScheduledTimers);
	}

	if (_numScheduledTimers != index)
		removeScheduledTimer(index);
}

uint16 TopgunEngine_v1_2::removeScheduledTimer(uint16 index) {
	_numScheduledTimers--;

	for (uint16 i = index; i < _numScheduledTimers; i++)
		_scheduledTimers[i] = _scheduledTimers[i + 1];

	if (_numScheduledTimers > index)
		index--;

	return index;
}

void TopgunEngine_v1_2::serviceScheduledMsgTimerQueue() {
	uint16 index = 0;

	if (_numScheduledTimers == 0)
		return;

	do {
		ScheduledTimer_v1_2 &timer = _scheduledTimers[index];

		if (timer.expiry <= getEngineMillis()) {
			uint16 script = timer.script;

			if (timer.repeat != 0)
				timer.expiry = getEngineMillis() + timer.delay;
			else
				index = removeScheduledTimer(index);

			postRunScript(script, 0, 0);
		}

		index++;
	} while (index < _numScheduledTimers);
}

bool TopgunEngine_v1_2::o_addCollisionPair(const byte *record) {
	addCollisionPair(record);
	return true;
}

bool TopgunEngine_v1_2::addCollisionPair(const byte *record) {
	uint16 first = (uint16)(operandValue(READ_LE_UINT16(record + 2)) - TG_HOST_V1_2_LITERAL_BIAS);
	uint16 second = (uint16)(operandValue(READ_LE_UINT16(record + 4)) - TG_HOST_V1_2_LITERAL_BIAS);
	uint16 index = 0;

	if (_numCollisionPairs != 0) {
		do {
			if (_collisionPairs[index].first == first && _collisionPairs[index].second == second)
				break;

			index++;
		} while (index < _numCollisionPairs);
	}

	if (index >= TG_HOST_V1_2_MAX_COLLISION_PAIRS)
		return false;

	uint16 script = 0;
	if (READ_LE_UINT16(record + 6) != 0)
		script = operandValue(READ_LE_UINT16(record + 6));

	if (script == 0) {
		removeCollisionPair(index);
		return true;
	}

	CollisionPair_v1_2 &pair = _collisionPairs[index];

	pair.first = first;
	pair.second = second;
	pair.script = (uint16)(script - TG_HOST_V1_2_LITERAL_BIAS);

	if (READ_LE_UINT16(record + 8) != 0)
		pair.argument1 = (uint16)(operandValue(READ_LE_UINT16(record + 8)) - TG_HOST_V1_2_LITERAL_BIAS);
	else
		pair.argument1 = 0;

	if (READ_LE_UINT16(record + 10) != 0)
		pair.argument2 = (uint16)(operandValue(READ_LE_UINT16(record + 10)) - TG_HOST_V1_2_LITERAL_BIAS);
	else
		pair.argument2 = 0;

	pair.repeat = record[0x0C];

	if (index == _numCollisionPairs)
		_numCollisionPairs++;

	return true;
}

uint16 TopgunEngine_v1_2::removeCollisionPair(uint16 index) {
	if (_numCollisionPairs <= index)
		return 0;

	_numCollisionPairs--;

	for (uint16 i = index; i < _numCollisionPairs; i++)
		_collisionPairs[i] = _collisionPairs[i + 1];

	if (_numCollisionPairs > index)
		index--;

	return index;
}

void TopgunEngine_v1_2::serviceCollisionPairs() {
	uint16 index = 0;

	if (_numCollisionPairs == 0)
		return;

	do {
		const CollisionPair_v1_2 &pair = _collisionPairs[index];
		uint16 firstIndex = operandValue(pair.first);
		uint16 secondIndex = operandValue(pair.second);

		if (isRecordIndexValid(firstIndex, "serviceCollisionPairs") && isRecordIndexValid(secondIndex, "serviceCollisionPairs")) {
			uint16 firstSprite = _records[firstIndex].handle;
			uint16 secondSprite = _records[secondIndex].handle;

			if (firstSprite != 0 && secondSprite != 0 && spriteLib()->spriteHitDetect(firstSprite, secondSprite) != 0) {
				uint16 script = pair.script;
				uint16 argument1 = pair.argument1;
				uint16 argument2 = pair.argument2;

				if (pair.repeat == 0)
					index = removeCollisionPair(index);

				postRunScript(script, argument1, argument2);
			}
		}

		index++;
	} while (index < _numCollisionPairs);
}

bool TopgunEngine_v1_2::o_setHoverRegion(const byte *record) {
	if (READ_LE_UINT16(record + 0x0A) == 0)
		return removeHoverRegion(record);

	return addHoverRegion(record);
}

bool TopgunEngine_v1_2::addHoverRegion(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 2));
	int16 top = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 8));
	uint16 index = 0;

	if (_numHoverRegions != 0) {
		do {
			const HoverRegion_v1_2 &region = _hoverRegions[index];

			if (region.left == left && region.top == top && region.right == right && region.bottom == bottom)
				break;

			index++;
		} while (index < _numHoverRegions);
	}

	if (_numHoverRegions <= index) {
		if (_numHoverRegions >= TG_HOST_V1_2_MAX_HOVER_REGIONS)
			return false;

		index = _numHoverRegions;
		_numHoverRegions++;
	}

	HoverRegion_v1_2 &region = _hoverRegions[index];

	region.left = left;
	region.top = top;
	region.right = right;
	region.bottom = bottom;
	region.enterScript = READ_LE_UINT16(record + 0x0A);
	region.leaveScript = READ_LE_UINT16(record + 0x0C);

	Common::Point cursor = g_system->getEventManager()->getMousePos();
	transformPointForDisplayZoom(cursor);

	if (region.left <= cursor.x && region.right > cursor.x && region.top <= cursor.y && region.bottom > cursor.y)
		updateHoverRegionFromPoint(cursor.x, cursor.y);

	return true;
}

bool TopgunEngine_v1_2::removeHoverRegion(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 2));
	int16 top = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 8));
	uint16 index = 0;

	if (_numHoverRegions != 0) {
		do {
			const HoverRegion_v1_2 &region = _hoverRegions[index];

			if (region.left == left && region.top == top && region.right == right && region.bottom == bottom)
				break;

			index++;
		} while (index < _numHoverRegions);
	}

	if (_numHoverRegions > index) {
		_numHoverRegions--;

		for (uint16 i = index; i < _numHoverRegions; i++)
			_hoverRegions[i] = _hoverRegions[i + 1];

		if (_hoverRegionCurrent != 0) {
			uint16 current = _hoverRegionCurrent - 1;

			if (current == index)
				_hoverRegionCurrent = 0;
			else if (current > index)
				_hoverRegionCurrent--;
		}
	}

	return true;
}

void TopgunEngine_v1_2::updateHoverRegionFromPoint(int16 x, int16 y) {
	int16 found = (int16)_numHoverRegions - 1;

	while (found >= 0) {
		const HoverRegion_v1_2 &region = _hoverRegions[found];

		if (region.left <= x && region.right > x && region.top <= y && region.bottom > y)
			break;

		found--;
	}

	int16 current = (int16)_hoverRegionCurrent - 1;

	if (current != found) {
		if (current >= 0) {
			uint16 script = _hoverRegions[current].leaveScript;

			if (script != 0)
				postRunScript(script, 0, 0);
		}

		_hoverRegionCurrent = 0;
	}

	if (found >= 0 && current != found) {
		uint16 script = _hoverRegions[found].enterScript;

		if (script != 0)
			postRunScript(script, 0, 0);

		_hoverRegionCurrent = (uint16)(found + 1);
	}
}

} // End of namespace Topgun
