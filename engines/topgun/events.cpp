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
#include "topgun/topgun_v1_2_3.h"

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

	if (index >= TG_HOST_V1_2_MAX_SCHEDULED_TIMERS) {
		if (_version >= kVersionV1_2_5)
			fatalError(116, "Out of timers.");

		return;
	}

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
	if (_version >= kVersionV1_2_3 && record[4] != 0) {
		_numScheduledTimers = 0;
		return;
	}

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

	if (_version >= kVersionV1_2_3) {
		if ((int16)index <= _scheduledTimerIndex)
			_scheduledTimerIndex--;

		return index;
	}

	if (_numScheduledTimers > index)
		index--;

	return index;
}

void TopgunEngine_v1_2::serviceScheduledMsgTimerQueue() {
	uint16 index = 0;

	if (_version >= kVersionV1_2_3) {
		if (_timersPaused)
			return;

		_scheduledTimerIndex = 0;
	}

	if (_numScheduledTimers == 0)
		return;

	do {
		if (_version >= kVersionV1_2_3)
			index = (uint16)_scheduledTimerIndex;

		ScheduledTimer_v1_2 &timer = _scheduledTimers[index];

		if (timer.expiry <= getEngineMillis()) {
			uint16 script = timer.script;

			if (timer.repeat != 0)
				timer.expiry = getEngineMillis() + timer.delay;
			else
				index = removeScheduledTimer(index);

			if (_version >= kVersionV1_2_3)
				runScriptResource(script);
			else
				postRunScript(script, 0, 0);
		}

		if (_version >= kVersionV1_2_3)
			_scheduledTimerIndex++;
		else
			index++;
	} while ((_version >= kVersionV1_2_3) ? ((uint16)_scheduledTimerIndex < _numScheduledTimers) : (index < _numScheduledTimers));
}

bool TopgunEngine_v1_2::o_addCollisionPair(const byte *record) {
	addCollisionPair(record);
	return true;
}

bool TopgunEngine_v1_2::appendCollisionPair() {
	uint16 maxPairs = TG_HOST_V1_2_MAX_COLLISION_PAIRS;

	if (_version >= kVersionV1_2_3)
		maxPairs = TG_INPUT_V1_2_3_MAX_COLLISION_PAIRS;
	else if (_version >= kVersionV1_2_1)
		maxPairs = TG_HOST_V1_2_1_MAX_COLLISION_PAIRS;

	if (_numCollisionPairs >= maxPairs)
		return false;

	if (_numCollisionPairs < _collisionPairs.size())
		_collisionPairs[_numCollisionPairs] = CollisionPair_v1_2();
	else
		_collisionPairs.push_back(CollisionPair_v1_2());

	_numCollisionPairs++;
	return true;
}

bool TopgunEngine_v1_2::addCollisionPair(const byte *record) {
	uint16 literalBias = (_version >= kVersionV1_2_3) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

	if (_version >= kVersionV1_2_3) {
		TopgunEngine_v1_2_3 *newer = static_cast<TopgunEngine_v1_2_3 *>(this);

		if (record[0x0D] != 0) {
			_numCollisionPairs = 0;
			return true;
		}

		uint16 spriteType = kResV1_2_3Sprite;
		if (_version >= kVersionV1_2_9)
			spriteType = kResV1_2_9Sprite;
		else if (_version >= kVersionV1_2_5)
			spriteType = kResV1_2_5Sprite;

		if (!newer->validateResourceType(READ_LE_UINT16(record + 2), spriteType))
			return false;

		if (!newer->validateResourceType(READ_LE_UINT16(record + 4), spriteType))
			return false;
	}

	uint16 first = (uint16)(operandValue(READ_LE_UINT16(record + 2)) - literalBias);
	uint16 second = (uint16)(operandValue(READ_LE_UINT16(record + 4)) - literalBias);
	byte allowInvisibleFirst = 0;
	byte allowInvisibleSecond = 0;

	if (_version >= kVersionV1_2_3) {
		allowInvisibleFirst = record[0x0E];
		allowInvisibleSecond = record[0x0F];

		if (second < first) {
			uint16 swap = first;
			first = second;
			second = swap;

			byte swapFlag = allowInvisibleFirst;
			allowInvisibleFirst = allowInvisibleSecond;
			allowInvisibleSecond = swapFlag;
		}
	}

	uint16 index = 0;

	if (_numCollisionPairs != 0) {
		do {
			if (_collisionPairs[index].first == first && _collisionPairs[index].second == second)
				break;

			index++;
		} while (index < _numCollisionPairs);
	}

	if (index >= _numCollisionPairs) {
		if (!appendCollisionPair())
			return false;

		index = _numCollisionPairs - 1;
	}

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
	pair.script = (uint16)(script - literalBias);

	if (READ_LE_UINT16(record + 8) != 0)
		pair.argument1 = (uint16)(operandValue(READ_LE_UINT16(record + 8)) - literalBias);
	else
		pair.argument1 = 0;

	if (READ_LE_UINT16(record + 10) != 0)
		pair.argument2 = (uint16)(operandValue(READ_LE_UINT16(record + 10)) - literalBias);
	else
		pair.argument2 = 0;

	pair.repeat = record[0x0C];

	if (_version >= kVersionV1_2_3) {
		pair.allowInvisibleFirst = allowInvisibleFirst;
		pair.allowInvisibleSecond = allowInvisibleSecond;
		pair.sameLevelOnly = record[0x10];
	}

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

uint16 TopgunEngine_v1_2_3::removeNativeTimer(uint16 index) {
	_numNativeTimers--;

	for (uint16 i = index; i < _numNativeTimers; i++)
		_nativeTimers[i] = _nativeTimers[i + 1];

	if (_numNativeTimers > index)
		index--;

	return index;
}

void TopgunEngine_v1_2_3::findNativeTimer(uint16 id) {
	uint16 index = 0;

	if (_numNativeTimers != 0) {
		do {
			if (_nativeTimers[index].id == id)
				break;

			index++;
		} while (index < _numNativeTimers);
	}

	if (_numNativeTimers != index)
		removeNativeTimer(index);
}

void TopgunEngine_v1_2_3::addNativeTimer(uint16 id, uint32 interval, uint16 proc, byte repeat) {
	uint16 index = 0;

	if (_numNativeTimers != 0) {
		do {
			if (_nativeTimers[index].id == id)
				break;

			index++;
		} while (index < _numNativeTimers);
	}

	if (index >= TG_INPUT_V1_2_3_MAX_NATIVE_TIMERS) {
		if (_version >= kVersionV1_2_5)
			fatalError(116, "Out of timers.");

		return;
	}

	if (_numNativeTimers == index)
		_numNativeTimers++;

	NativeTimer_v1_2_3 &timer = _nativeTimers[index];

	timer.id = id;
	timer.interval = interval;
	timer.expiry = getEngineMillis() + interval;
	timer.proc = proc;
	timer.repeat = repeat;
}

void TopgunEngine_v1_2_3::serviceNativeTimerQueue() {
	if (_timersPaused)
		return;

	uint16 index = 0;

	if (_numNativeTimers == 0)
		return;

	do {
		NativeTimer_v1_2_3 &timer = _nativeTimers[index];

		if (timer.expiry <= getEngineMillis()) {
			uint16 proc = timer.proc;
			uint16 id = timer.id;

			if (timer.repeat != 0)
				timer.expiry = getEngineMillis() + timer.interval;
			else
				index = removeNativeTimer(index);

			if (proc == kNativeTimerV1_2_3ComboHistory)
				comboHistoryTimeoutCallback((byte)id);
			else if (proc == kNativeTimerV1_2_3KeyRelease)
				keyReleaseDelayTimerCallback((byte)id);
		}

		index++;
	} while (index < _numNativeTimers);
}

uint16 TopgunEngine_v1_2_3::builtinPauseTimers(uint16 flag) {
	uint32 now = getEngineMillis();

	_timersPaused = ((byte)flag != 0);

	if (_timersPaused) {
		_timersPausedAt = now;
		return 109;
	}

	uint32 elapsed = now - _timersPausedAt;

	for (uint16 i = 0; i < _numScheduledTimers; i++)
		_scheduledTimers[i].expiry += elapsed;

	for (uint16 i = 0; i < _numNativeTimers; i++)
		_nativeTimers[i].expiry += elapsed;

	return 109;
}

void TopgunEngine_v1_2_3::serviceCollisionPairs() {
	uint16 index = 0;
	uint16 pairIndex = 0;

	if (_numCollisionPairs == 0)
		return;

	do {
		// The element read advances by one per pass, also after a removal; it is skipped when it lies beyond the last element
		if (pairIndex < _numCollisionPairs) {
			const CollisionPair_v1_2 &pair = _collisionPairs[pairIndex];
			uint16 firstIndex = operandValue(pair.first);
			uint16 secondIndex = operandValue(pair.second);

			if (isRecordIndexValid(firstIndex, "serviceCollisionPairs") && isRecordIndexValid(secondIndex, "serviceCollisionPairs")) {
				uint16 firstSprite = _records[firstIndex].handle;
				uint16 secondSprite = _records[secondIndex].handle;

				if (firstSprite != 0 && secondSprite != 0 && spriteLib()->spriteHitDetect(firstSprite, secondSprite, pair.allowInvisibleFirst, pair.allowInvisibleSecond, pair.sameLevelOnly) != 0) {
					uint16 script = pair.script;
					uint16 argument1 = pair.argument1;
					uint16 argument2 = pair.argument2;

					if (pair.repeat == 0)
						index = removeCollisionPair(index);

					if (!enqueuePostedScriptRun(script, argument1, argument2))
						return;
				}
			}
		}

		pairIndex++;
		index++;
	} while (index < _numCollisionPairs);
}

} // End of namespace Topgun
