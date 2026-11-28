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
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

void SpriteLibrary::spriteAnimate() {
	_inTick = true;

	if (_engine->_version >= kVersionV1_1 && _serviceProcSet)
		_engine->audioLibrary()->audioService();

	if (_tickCursor < _spriteCount) {
		do {
			SpriteObject *object = findSprite(tickCursorSprite(), "spriteAnimate");
			if (object && object->queueCurrent && object->scriptActive) {
				bool repeat;
				do {
					repeat = processSpriteAnimationStep(tickCursorSprite());
					if (repeat)
						repeat = processSpriteQueueCommand(tickCursorSprite());

					if (_serviceProcSet)
						_engine->audioLibrary()->audioService();
				} while (repeat);
			}

			_tickCursor++;
		} while (_tickCursor < _spriteCount);
	}

	_tickCursor = 0;
	_inTick = false;
}

uint16 SpriteLibrary::tickCursorSprite() {
	if (_tickCursor < 0 || _tickCursor >= TG_SPR_MAX_SPRITES) {
		warning("SpriteLibrary::tickCursorSprite(): the tick cursor is outside the sprite table");
		return 0;
	}

	return _spriteTable[_tickCursor];
}

bool SpriteLibrary::spriteSetQueue(uint16 sprite, const byte *queue, uint16 size, uint16 tag) {
	if (!queue || size == 0)
		return false;

	SpriteObject *object = findSprite(sprite, "spriteSetQueue");
	if (!object)
		return false;

	byte *copy = _engine->utilsLibrary()->tgMalloc(size, 0);
	if (!copy)
		return false;

	spriteEmptyQueue(sprite);
	memcpy(copy, queue, size);

	object->queueStart = copy;
	object->queueEnd = copy + size;
	object->queueTag = tag;

	if (_engine->_version >= kVersionV2_0) {
		object->queueCapacityStart = copy;
		object->queueCapacity = size;
	}

	processSpriteQueueCommand(sprite);
	return true;
}

void SpriteLibrary::spriteEmptyQueue(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "spriteEmptyQueue");
	if (!object)
		return;

	_engine->utilsLibrary()->tgFree(object->queueStart);
	object->queueStart = nullptr;
	object->queueEnd = nullptr;
	object->queueCurrent = nullptr;
	object->moveDue = 0;
	object->frameDue = 0;
	object->moveInterval = 0;
	object->frameInterval = 0;
	object->timingMode = 0;
	object->loopsDisabled = false;
	object->queueTag = 0;
}

void SpriteLibrary::spriteSetMessageProc() {
	_messageProcSet = true;
}

void SpriteLibrary::spriteSetServiceProc() {
	_serviceProcSet = true;
}

void SpriteLibrary::spriteSetDebugProc(uint32 value) {
	_debugProc = value;
}

bool SpriteLibrary::processSpriteQueueCommand(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "processSpriteQueueCommand");
	if (!object)
		return false;

	object->queueCurrent = nextQueueRecordWithLoops(sprite);
	byte *command = object->queueCurrent;
	if (!command) {
		spriteEmptyQueue(sprite);
		return false;
	}

	switch (READ_LE_UINT16(command)) {
	case 1:
		WRITE_LE_UINT16(command + 6, spriteObjectSetCelRange(sprite, READ_LE_UINT16(command + 2), READ_LE_UINT16(command + 2), READ_LE_UINT16(command + 4)));
		break;

	case 2:
		command[0xC] = 0;
		break;

	case 3:
		WRITE_LE_INT16(command + 0xC, object->x);
		WRITE_LE_INT16(command + 0xE, object->y);
		WRITE_LE_UINT16(command + 0x10, 0);
		break;

	case 5:
		WRITE_LE_INT16(command + 0xA, object->x);
		WRITE_LE_INT16(command + 0xC, object->y);
		WRITE_LE_UINT16(command + 0xE, 0);
		if (READ_LE_UINT16(command + 4) != 0)
			computeInterpolatedMoveSteps(command);
		else
			WRITE_LE_UINT16(command + 0x10, READ_LE_UINT16(command + 2));
		break;

	case 7:
		command[6] = 0;
		break;

	case 9:
		object->timingMode = command[2];
		object->moveDue = 0;
		object->frameDue = 0;
		processSpriteQueueCommand(sprite);
		return object->queueCurrent;

	default:
		break;
	}

	return false;
}

bool SpriteLibrary::processSpriteAnimationStep(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "processSpriteAnimationStep");
	if (!object)
		return true;

	byte *command = object->queueCurrent;
	if (!command) {
		warning("SpriteLibrary::processSpriteAnimationStep(): the sprite has no current command");
		return true;
	}

	bool finished = true;

	switch (READ_LE_UINT16(command)) {
	case 1:
		finished = tickPlayRangeRecord(sprite);
		break;

	case 2:
		finished = tickMultiCelRecord(sprite);
		break;

	case 3:
		finished = tickQuadPathRecord(sprite);
		break;

	case 4:
		spriteHide(sprite, true);
		break;

	case 5:
		finished = tickLinearMoveRecord(sprite);
		break;

	case 6:
		finished = tickDelayedMoveRecord(sprite);
		break;

	case 7:
		finished = tickWaitRecord(sprite);
		break;

	case 8:
		spriteSetPosition(sprite, READ_LE_INT16(command + 2), READ_LE_INT16(command + 4));
		break;

	case 10:
		spriteSetMotion(sprite, READ_LE_UINT32(command + 2));
		break;

	case 11:
		spriteObjectSetCelRange(sprite, READ_LE_UINT16(command + 2), READ_LE_UINT16(command + 4), READ_LE_UINT16(command + 6));
		break;

	case 12:
		spriteSetSpeed(sprite, READ_LE_UINT32(command + 2));
		break;

	case 13:
		spriteShow(sprite, READ_LE_UINT16(command + 2), true);
		break;

	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
		fireSpriteQueueCallbackMsg(sprite);
		finished = false;
		break;

	default:
		break;
	}

	return finished;
}

byte *SpriteLibrary::nextQueueRecordWithLoops(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "nextQueueRecordWithLoops");
	if (!object)
		return nullptr;

	byte *record;
	if (object->queueCurrent) {
		uint16 opcode = READ_LE_UINT16(object->queueCurrent);
		if (opcode >= TG_SPR_NUM_QUEUE_COMMANDS) {
			warning("SpriteLibrary::nextQueueRecordWithLoops(): queue opcode %d is out of range", opcode);
			return nullptr;
		}

		record = object->queueCurrent + _queueCommandSizeTable[opcode];
	} else {
		record = object->queueStart;
	}

	if (record >= object->queueEnd)
		return nullptr;

	while (READ_LE_UINT16(record) == 15) {
		if (object->loopsDisabled || READ_LE_UINT32(record + 2) == 1) {
			WRITE_LE_UINT32(record + 2, READ_LE_UINT32(record + 6));
			record += _queueCommandSizeTable[15];
			if (record >= object->queueEnd)
				return nullptr;
		} else {
			uint32 remaining = READ_LE_UINT32(record + 2);
			if (remaining != 0)
				WRITE_LE_UINT32(record + 2, remaining - 1);

			record = object->queueStart + READ_LE_UINT16(record + 0xA);
			if (record >= object->queueEnd) {
				warning("SpriteLibrary::nextQueueRecordWithLoops(): loop target is beyond the end of the queue");
				return nullptr;
			}
		}
	}

	return record;
}

void SpriteLibrary::computeInterpolatedMoveSteps(byte *command) {
	int16 startX = READ_LE_INT16(command + 0xA);
	int16 startY = READ_LE_INT16(command + 0xC);
	int16 endX = READ_LE_INT16(command + 6);
	int16 endY = READ_LE_INT16(command + 8);

	uint16 distanceX = (endX > startX) ? (uint16)(endX - startX) : (uint16)(startX - endX);
	uint16 distanceY = (endY > startY) ? (uint16)(endY - startY) : (uint16)(startY - endY);
	uint16 longest = (distanceX >= distanceY) ? distanceX : distanceY;
	uint16 speed = READ_LE_UINT16(command + 2);

	if (speed == 0) {
		warning("SpriteLibrary::computeInterpolatedMoveSteps(): the speed is zero");
		WRITE_LE_UINT16(command + 0x10, 0);
		return;
	}

	uint16 rounded = (uint16)(longest + (speed >> 1));
	WRITE_LE_UINT16(command + 0x10, rounded / speed);
}

void SpriteLibrary::fireSpriteQueueCallbackMsg(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "fireSpriteQueueCallbackMsg");
	if (!object || !object->queueCurrent)
		return;

	byte record[TG_SPR_QUEUE_RECORD_COPY_SIZE];
	memset(record, 0, sizeof(record));
	memcpy(record, object->queueCurrent, _queueCommandSizeTable[READ_LE_UINT16(object->queueCurrent)]);

	processSpriteQueueCommand(sprite);

	object->scriptActive = 0;
	object->inCallback = true;
	if (_messageProcSet)
		_engine->spriteMessageService(record);

	if (findSpriteIndex(sprite) != -1) {
		object = findSprite(sprite, "fireSpriteQueueCallbackMsg");
		if (object && object->inCallback) {
			object->scriptActive = 1;
			object->inCallback = false;
		}
	}
}

void SpriteLibrary::notifySpriteFrameChange(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "notifySpriteFrameChange");
	if (!object)
		return;

	byte message[TG_SPR_QUEUE_RECORD_COPY_SIZE];
	memset(message, 0, sizeof(message));
	WRITE_LE_UINT16(message, TG_SPR_FRAME_NOTIFICATION);
	WRITE_LE_UINT16(message + 2, object->id);

	object->scriptActive = 0;
	object->notifyOnFrame = false;
	object->inCallback = true;
	if (_messageProcSet)
		_engine->spriteMessageService(message);

	if (findSpriteIndex(sprite) != -1) {
		object = findSprite(sprite, "notifySpriteFrameChange");
		if (object && object->inCallback) {
			object->scriptActive = 1;
			object->inCallback = false;
		}
	}
}

bool SpriteLibrary::tickPlayRangeRecord(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "tickPlayRangeRecord");
	if (!object)
		return false;

	uint32 now = _engine->getEngineMillis();
	if (now < object->frameDue)
		return false;

	byte *command = object->queueCurrent;
	if (READ_LE_UINT16(command + 6) == 0)
		return true;

	if (READ_LE_UINT32(command + 8) != 0xFFFFFFFF)
		object->frameInterval = READ_LE_UINT32(command + 8);

	if (object->timingMode == 0 || object->frameDue == 0)
		object->frameDue = now;

	object->frameDue += object->frameInterval;
	object->framePending = true;
	object->visible = true;
	renderSpriteToSurface(sprite);

	uint16 remaining = READ_LE_UINT16(command + 6) - 1;
	WRITE_LE_UINT16(command + 6, remaining);
	if (remaining != 0)
		return false;

	return object->frameInterval == 0;
}

bool SpriteLibrary::tickMultiCelRecord(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "tickMultiCelRecord");
	if (!object)
		return false;

	uint32 now = _engine->getEngineMillis();
	if (now < object->frameDue)
		return false;

	byte *command = object->queueCurrent;
	if (command[0xC] != 0)
		return true;

	if (READ_LE_UINT32(command + 8) != 0xFFFFFFFF)
		object->frameInterval = READ_LE_UINT32(command + 8);

	if (object->timingMode == 0 || object->frameDue == 0)
		object->frameDue = now;

	object->frameDue += object->frameInterval;

	Common::Rect oldBox = object->boundingBox;
	if (!object->parts)
		object->parts = SpritePartBlockPtr(new SpritePartBlock());

	recomputeCelBoxArray(sprite, command);
	object->useUnionRefresh = !oldBox.findIntersectingRect(object->boundingBox).isEmpty();
	object->visible = true;
	command[0xC] = 1;
	object->framePending = false;
	if (object->notifyOnFrame)
		notifySpriteFrameChange(sprite);

	renderSpriteToSurface(sprite);
	return object->frameInterval == 0;
}

bool SpriteLibrary::tickQuadPathRecord(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "tickQuadPathRecord");
	if (!object)
		return false;

	bool finished = false;
	uint32 now = _engine->getEngineMillis();

	if (now >= object->moveDue) {
		if (object->timingMode == 0 || object->moveDue == 0)
			object->moveDue = now;

		object->moveDue += object->moveInterval;
		if (object->rangeStart != object->rangeEnd && now >= object->frameDue) {
			object->framePending = true;
			if (object->timingMode == 0 || object->frameDue == 0)
				object->frameDue = now;

			object->frameDue += object->frameInterval;
		}

		finished = stepQuadPath(sprite);
	} else if (object->rangeStart != object->rangeEnd && now >= object->frameDue) {
		object->framePending = true;
		if (object->timingMode == 0 || object->frameDue == 0)
			object->frameDue = now;

		object->frameDue += object->frameInterval;
		object->useUnionRefresh = true;
		renderSpriteToSurface(sprite);
	}

	return finished;
}

bool SpriteLibrary::tickLinearMoveRecord(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "tickLinearMoveRecord");
	if (!object)
		return false;

	bool finished = false;
	uint32 now = _engine->getEngineMillis();

	if (now >= object->moveDue) {
		if (object->timingMode == 0 || object->moveDue == 0)
			object->moveDue = now;

		object->moveDue += object->moveInterval;
		if (object->rangeStart != object->rangeEnd && now >= object->frameDue) {
			object->framePending = true;
			if (object->timingMode == 0 || object->frameDue == 0)
				object->frameDue = now;

			object->frameDue += object->frameInterval;
		}

		finished = stepLinearMove(sprite);
	} else if (object->rangeStart != object->rangeEnd && now >= object->frameDue) {
		object->framePending = true;
		if (object->timingMode == 0 || object->frameDue == 0)
			object->frameDue = now;

		object->frameDue += object->frameInterval;
		object->useUnionRefresh = true;
		renderSpriteToSurface(sprite);
	}

	return finished;
}

bool SpriteLibrary::tickDelayedMoveRecord(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "tickDelayedMoveRecord");
	if (!object)
		return false;

	bool finished = false;
	bool frameChanged = false;
	uint32 now = _engine->getEngineMillis();

	if (object->rangeStart != object->rangeEnd && now >= object->frameDue) {
		object->framePending = true;
		if (object->timingMode == 0 || object->frameDue == 0)
			object->frameDue = now;

		object->frameDue += object->frameInterval;
		frameChanged = true;
	}

	if (now >= object->moveDue) {
		if (object->timingMode == 0 || object->moveDue == 0)
			object->moveDue = now;

		object->moveDue += object->moveInterval;

		byte *command = object->queueCurrent;
		spriteSetPosition(sprite, READ_LE_INT16(command + 2), READ_LE_INT16(command + 4));
		finished = true;
	} else if (frameChanged) {
		renderSpriteToSurface(sprite);
	}

	return finished;
}

bool SpriteLibrary::tickWaitRecord(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "tickWaitRecord");
	if (!object)
		return false;

	uint32 now = _engine->getEngineMillis();
	byte *command = object->queueCurrent;
	if (command[6] != 0)
		return now >= object->frameDue;

	command[6] = 1;
	if (object->timingMode == 0 || object->frameDue == 0)
		object->frameDue = now;

	object->frameDue += READ_LE_UINT32(command + 2);
	return false;
}

bool SpriteLibrary::stepQuadPath(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "stepQuadPath");
	if (!object)
		return false;

	byte *command = object->queueCurrent;
	uint16 step = READ_LE_UINT16(command + 0x10) + 1;
	WRITE_LE_UINT16(command + 0x10, step);
	uint16 steps = READ_LE_UINT16(command + 2);

	if (steps >= step) {
		Common::Point point = computeQuadCurvePoint(step, steps, READ_LE_INT16(command + 0xC), READ_LE_INT16(command + 0xE),
		                                            READ_LE_INT16(command + 4), READ_LE_INT16(command + 6),
		                                            READ_LE_INT16(command + 8), READ_LE_INT16(command + 0xA));
		spriteSetPosition(sprite, point.x, point.y);
	}

	return steps <= step;
}

bool SpriteLibrary::stepLinearMove(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "stepLinearMove");
	if (!object)
		return false;

	byte *command = object->queueCurrent;
	uint16 step = READ_LE_UINT16(command + 0xE) + 1;
	WRITE_LE_UINT16(command + 0xE, step);
	uint16 total = READ_LE_UINT16(command + 0x10);

	if (step <= total) {
		Common::Point point = computeLinePoint(step, total, READ_LE_INT16(command + 0xA), READ_LE_INT16(command + 0xC),
		                                       READ_LE_INT16(command + 6), READ_LE_INT16(command + 8));
		spriteSetPosition(sprite, point.x, point.y);
	}

	return step >= total;
}

Common::Point SpriteLibrary::computeQuadCurvePoint(uint16 step, uint16 steps, int16 x0, int16 y0, int16 viaX, int16 viaY, int16 endX, int16 endY) {
	int16 viaRelativeX = (int16)(viaX - x0);
	int16 endRelativeX = (int16)(endX - x0);
	int16 viaRelativeY = (int16)(viaY - y0);
	int16 endRelativeY = (int16)(endY - y0);

	if (steps == 0) {
		warning("SpriteLibrary::computeQuadCurvePoint(): the step count is zero");
		Common::Point start;
		start.x = x0;
		start.y = y0;
		return start;
	}

	uint32 fraction = (((uint32)step << 10) + (steps >> 1)) / steps;
	uint16 u = (uint16)fraction;
	uint32 weight = (uint32)u * (uint16)(0x400 - u);

	int16 controlX = (int16)((uint16)(viaRelativeX * 4) - (uint16)endRelativeX);
	int64 sumX = (int64)(int32)weight * controlX + (int64)((uint32)u * u) * endRelativeX;
	if (_engine->_version < kVersionV1_2_4)
		sumX += (sumX < 0) ? -0x80000 : 0x80000;

	int16 controlY = (int16)((uint16)(viaRelativeY * 4) - (uint16)endRelativeY);
	int64 sumY = (int64)(int32)weight * controlY + (int64)((uint32)u * u) * endRelativeY;
	if (_engine->_version < kVersionV1_2_4)
		sumY += (sumY < 0) ? -0x80000 : 0x80000;

	Common::Point point;
	point.x = (int16)((uint16)(sumX >> 20) + (uint16)x0);
	point.y = (int16)((uint16)(sumY >> 20) + (uint16)y0);
	return point;
}

Common::Point SpriteLibrary::computeLinePoint(uint16 step, uint16 total, int16 x0, int16 y0, int16 x1, int16 y1) {
	if (total == 0) {
		warning("SpriteLibrary::computeLinePoint(): the step count is zero");
		Common::Point start;
		start.x = x0;
		start.y = y0;
		return start;
	}

	int16 deltaY = (int16)(y1 - y0);
	int16 quotientY;
	if (deltaY >= 0)
		quotientY = (int16)(((uint32)(uint16)deltaY * step + (total >> 1)) / total);
	else
		quotientY = (int16)(((int32)deltaY * (int16)step - (total >> 1)) / (int16)total);

	int16 deltaX = (int16)(x1 - x0);
	int16 quotientX;
	if (deltaX >= 0)
		quotientX = (int16)(((uint32)(uint16)deltaX * step + (total >> 1)) / total);
	else
		quotientX = (int16)(((int32)deltaX * (int16)step - (total >> 1)) / (int16)total);

	Common::Point point;
	point.x = (int16)(quotientX + x0);
	point.y = (int16)(quotientY + y0);
	return point;
}

void SpriteLibrary::applyFrameChange(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "applyFrameChange");
	if (!object)
		return;

	if (object->notifyOnFrame)
		notifySpriteFrameChange(sprite);

	object->partCount = 0;
	object->currentFrameIndex = object->nextFrameIndex;
	if (object->forward) {
		object->nextFrameIndex++;
		if (object->nextFrameIndex > object->rangeEnd)
			object->nextFrameIndex = object->rangeStart;
	} else {
		object->nextFrameIndex--;
		if ((int16)object->nextFrameIndex < (int16)object->rangeEnd)
			object->nextFrameIndex = object->rangeStart;
	}

	if (object->currentFrameIndex >= object->frames.size()) {
		warning("SpriteLibrary::applyFrameChange(): frame index %d is out of range", object->currentFrameIndex);
		return;
	}

	byte *frame = object->frames[object->currentFrameIndex];
	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 widthMinusOne = (int16)(width - 1);
	int16 heightMinusOne = (int16)(height - 1);

	Common::Rect oldBox = object->boundingBox;

	object->boundingBox.left = (int16)(object->x + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET) - widthMinusOne / 2);
	object->boundingBox.right = (int16)(width + object->boundingBox.left);
	object->boundingBox.top = (int16)(object->y + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET) - heightMinusOne / 2);
	object->boundingBox.bottom = (int16)(height + object->boundingBox.top);

	object->useUnionRefresh = !oldBox.findIntersectingRect(object->boundingBox).isEmpty();
	object->currentFrame = frame;
	object->framePending = false;
}

} // End of namespace Topgun
