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

#include "topgun/libraries/sprite/sprite_v1_2.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

void SpriteLibrary_v1_2::spriteAnimate() {
	_tickNesting++;

	uint16 aborted = 0;
	if (_serviceProcSet)
		callServiceProc();

	bool resetCursor = true;
	if (_tickCursor < (int)_spriteCount) {
		bool processing = true;
		while (processing) {
			if (aborted != 0) {
				resetCursor = false;
				break;
			}

			SpriteObject_v1_2 *object = lookupSprite(currentTickSprite());
			if (object && object->queueCurrent && object->scriptActive != 0) {
				bool again = true;
				while (again && aborted == 0) {
					bool finished = processSpriteAnimationStep(currentTickSprite());
					if (aborted == 0)
						aborted = callDebugProc(currentTickSprite());

					if (finished)
						finished = processSpriteQueueCommand(currentTickSprite());

					if (_serviceProcSet)
						callServiceProc();

					again = finished;
				}
			}

			_tickCursor++;
			if (_tickCursor >= (int)_spriteCount)
				processing = false;
		}
	}

	if (resetCursor)
		_tickCursor = 0;

	_tickNesting--;
}

bool SpriteLibrary_v1_2::processSpriteQueueCommand(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "processSpriteQueueCommand");
	if (!object)
		return false;

	object->queueCurrent = nextQueueRecordWithLoops(sprite);
	byte *command = object->queueCurrent;
	if (!command) {
		spriteEmptyQueue(sprite);
		return false;
	}

	switch (READ_LE_UINT16(command)) {
	case kSprV1_2OpPlayRange:
		WRITE_LE_UINT16(command + 6, spriteObjectSetCelRange(sprite, READ_LE_UINT16(command + 2), READ_LE_UINT16(command + 2), READ_LE_UINT16(command + 4)));
		break;

	case kSprV1_2OpMultiCel:
		command[0xC] = 0;
		break;

	case kSprV1_2OpQuadPath:
		WRITE_LE_INT16(command + 0x0D, object->x);
		WRITE_LE_INT16(command + 0x0F, object->y);
		WRITE_LE_UINT16(command + 0x11, decodeOperand(READ_LE_UINT16(command + 5)));
		WRITE_LE_UINT16(command + 0x13, decodeOperand(READ_LE_UINT16(command + 7)));
		WRITE_LE_UINT16(command + 0x15, decodeOperand(READ_LE_UINT16(command + 9)));
		WRITE_LE_UINT16(command + 0x17, decodeOperand(READ_LE_UINT16(command + 0x0B)));

		if (command[4] != 0) {
			WRITE_LE_UINT16(command + 0x11, (uint16)(READ_LE_UINT16(command + 0x11) + object->x));
			WRITE_LE_UINT16(command + 0x13, (uint16)(READ_LE_UINT16(command + 0x13) + object->y));
			WRITE_LE_UINT16(command + 0x15, (uint16)(READ_LE_UINT16(command + 0x15) + object->x));
			WRITE_LE_UINT16(command + 0x17, (uint16)(READ_LE_UINT16(command + 0x17) + object->y));
		}

		WRITE_LE_UINT16(command + 0x19, 0);
		WRITE_LE_UINT16(command + 0x1B, decodeOperand(READ_LE_UINT16(command + 2)));
		break;

	case kSprV1_2OpLinearMove:
		WRITE_LE_INT16(command + 0x0A, object->x);
		WRITE_LE_INT16(command + 0x0C, object->y);
		WRITE_LE_UINT16(command + 0x0E, decodeOperand(READ_LE_UINT16(command + 6)));
		WRITE_LE_UINT16(command + 0x10, decodeOperand(READ_LE_UINT16(command + 8)));

		if (command[5] != 0) {
			WRITE_LE_UINT16(command + 0x0E, (uint16)(READ_LE_UINT16(command + 0x0E) + object->x));
			WRITE_LE_UINT16(command + 0x10, (uint16)(READ_LE_UINT16(command + 0x10) + object->y));
		}

		WRITE_LE_UINT16(command + 0x12, 0);
		if (command[4] != 0)
			computeInterpolatedMoveSteps(command);
		else
			WRITE_LE_UINT16(command + 0x14, decodeOperand(READ_LE_UINT16(command + 2)));
		break;

	case kSprV1_2OpWait:
		command[4] = 0;
		break;

	case kSprV1_2OpTimingMode:
		object->timingMode = command[2];
		object->moveDue = 0;
		object->frameDue = 0;
		processSpriteQueueCommand(sprite);
		return object->queueCurrent;

	case kSprV1_2OpFrameInterval:
		spriteSetMotion(sprite, decodeOperand(READ_LE_UINT16(command + 2)));
		processSpriteQueueCommand(sprite);
		return object->queueCurrent;

	case kSprV1_2OpSetRange:
		spriteObjectSetCelRange(sprite, READ_LE_UINT16(command + 2), READ_LE_UINT16(command + 4), READ_LE_UINT16(command + 6));
		processSpriteQueueCommand(sprite);
		return object->queueCurrent;

	case kSprV1_2OpMoveInterval:
		spriteSetSpeed(sprite, decodeOperand(READ_LE_UINT16(command + 2)));
		if (READ_LE_UINT16(command + 4) != 0)
			spriteSetMotion(sprite, decodeOperand(READ_LE_UINT16(command + 4)));

		processSpriteQueueCommand(sprite);
		return object->queueCurrent;

	default:
		break;
	}

	return false;
}

bool SpriteLibrary_v1_2::processSpriteAnimationStep(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "processSpriteAnimationStep");
	if (!object)
		return true;

	byte *command = object->queueCurrent;
	if (!command) {
		warning("SpriteLibrary_v1_2::processSpriteAnimationStep(): the sprite has no current command");
		return true;
	}

	uint16 opcode = READ_LE_UINT16(command);
	bool finished = true;

	switch (opcode) {
	case kSprV1_2OpPlayRange:
		finished = tickPlayRangeRecord(sprite);
		break;

	case kSprV1_2OpMultiCel:
		finished = tickMultiCelRecord(sprite);
		break;

	case kSprV1_2OpCompositeBackground:
		compositeSpriteIntoBackground(sprite);
		break;

	case kSprV1_2OpQuadPath:
		finished = tickQuadPathRecord(sprite);
		break;

	case kSprV1_2OpFlip:
		tickFlipRecord(sprite);
		break;

	case kSprV1_2OpHide:
		spriteHide(sprite, true);
		break;

	case kSprV1_2OpLinearMove:
		finished = tickLinearMoveRecord(sprite);
		break;

	case kSprV1_2OpDelayedMove:
		finished = tickDelayedMoveRecord(sprite);
		break;

	case kSprV1_2OpWait:
		finished = tickWaitRecord(sprite);
		break;

	case kSprV1_2OpSetPosition:
		tickSetPositionRecord(sprite, command);
		break;

	case kSprV1_2OpShowFrame:
		spriteShow(sprite, READ_LE_UINT16(command + 2), true);
		break;

	case kSprV1_2OpHostEvent:
		if (!_inHostCallback)
			fireSpriteQueueCallbackMsg(sprite, _queueCommandSizeTable[opcode]);

		finished = false;
		break;

	case kSprV1_2OpHostEventPayload:
		finished = fireSpriteQueueCallbackMsg(sprite, READ_LE_UINT16(command + 2));
		break;

	case kSprV1_2OpMovieFrame:
		finished = tickMovieFrameRecord(sprite);
		break;

	default:
		break;
	}

	return finished;
}

byte *SpriteLibrary_v1_2::nextQueueRecordWithLoops(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "nextQueueRecordWithLoops");
	if (!object)
		return nullptr;

	byte *record;
	if (!object->queueCurrent) {
		record = object->queueStart;
	} else {
		record = object->queueCurrent;
		uint16 opcode = READ_LE_UINT16(record);
		if (opcode == kSprV1_2OpHostEventPayload) {
			record += READ_LE_UINT16(record + 2);
		} else {
			if (opcode >= TG_SPR_V1_2_NUM_QUEUE_OPCODES) {
				warning("SpriteLibrary_v1_2::nextQueueRecordWithLoops(): queue opcode %d is out of range", opcode);
				return nullptr;
			}

			record += _queueCommandSizeTable[opcode];
		}
	}

	for (;;) {
		if (record >= object->queueEnd)
			return nullptr;

		for (;;) {
			if (READ_LE_UINT16(record) != kSprV1_2OpLoop)
				return record;

			if (!object->loopsDisabled && READ_LE_UINT16(record + 2) != 1) {
				uint16 remaining = READ_LE_UINT16(record + 2);
				if (remaining != 0)
					WRITE_LE_UINT16(record + 2, remaining - 1);

				record = object->queueStart + READ_LE_UINT16(record + 6);
				if (record >= object->queueEnd) {
					warning("SpriteLibrary_v1_2::nextQueueRecordWithLoops(): loop target is beyond the end of the queue");
					return nullptr;
				}
			} else {
				WRITE_LE_UINT16(record + 2, READ_LE_UINT16(record + 4));
				record += _queueCommandSizeTable[kSprV1_2OpLoop];
				break;
			}
		}
	}
}

void SpriteLibrary_v1_2::computeInterpolatedMoveSteps(byte *command) {
	int16 startX = READ_LE_INT16(command + 0x0A);
	int16 startY = READ_LE_INT16(command + 0x0C);
	int16 endX = READ_LE_INT16(command + 0x0E);
	int16 endY = READ_LE_INT16(command + 0x10);

	uint16 distanceX = (startX < endX) ? (uint16)(endX - startX) : (uint16)(startX - endX);
	uint16 distanceY = (startY < endY) ? (uint16)(endY - startY) : (uint16)(startY - endY);
	uint16 longest = (distanceY >= distanceX) ? distanceY : distanceX;

	uint16 speed = decodeOperand(READ_LE_UINT16(command + 2));
	if (speed == 0)
		speed = 1;

	uint16 steps = (uint16)(longest / speed);
	if (longest % speed != 0)
		steps++;

	WRITE_LE_UINT16(command + 0x14, steps);
}

bool SpriteLibrary_v1_2::fireSpriteQueueCallbackMsg(uint16 sprite, uint16 size) {
	if (!_messageProcSet)
		return true;

	if (_inHostCallback)
		return false;

	if (_hostEventsSuppressed)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "fireSpriteQueueCallbackMsg");
	if (!object || !object->queueCurrent)
		return false;

	_inHostCallback = true;

	uint16 copySize = size;
	if (copySize > TG_SPR_V1_2_EVENT_RECORD_SIZE) {
		warning("SpriteLibrary_v1_2::fireSpriteQueueCallbackMsg(): the host event record is larger than the %d byte buffer", TG_SPR_V1_2_EVENT_RECORD_SIZE);
		copySize = TG_SPR_V1_2_EVENT_RECORD_SIZE;
	}

	if (object->queueCurrent + copySize > object->queueEnd) {
		warning("SpriteLibrary_v1_2::fireSpriteQueueCallbackMsg(): the host event record is beyond the end of the queue");
		copySize = (uint16)(object->queueEnd - object->queueCurrent);
	}

	memcpy(_eventRecord, object->queueCurrent, copySize);

	object->scriptActive = 0;
	object->inCallback = true;
	object->advancePending = true;

	engine()->spriteMessageService(_eventRecord);

	_inHostCallback = false;

	if (currentTickSprite() != sprite && findSpriteIndex(sprite) == -1)
		return false;

	object = lookupSprite(sprite);
	if (!object || !object->inCallback)
		return false;

	object->scriptActive = 1;
	object->inCallback = false;

	if (object->advancePending)
		processSpriteQueueCommand(sprite);

	object = lookupSprite(sprite);
	if (object)
		object->advancePending = false;

	return false;
}

void SpriteLibrary_v1_2::notifySpriteFrameChange(uint16 sprite) {
	if (!_messageProcSet)
		return;

	SpriteObject_v1_2 *object = findSprite(sprite, "notifySpriteFrameChange");
	if (!object)
		return;

	byte message[TG_SPR_V1_2_NOTIFICATION_RECORD_SIZE];
	memset(message, 0, sizeof(message));
	WRITE_LE_UINT16(message, TG_SPR_V1_2_FRAME_NOTIFICATION);
	WRITE_LE_UINT16(message + 2, object->id);

	object->scriptActive = 0;
	object->notifyOnFrame = false;
	object->inCallback = true;

	engine()->spriteMessageService(message);

	if (currentTickSprite() != sprite && findSpriteIndex(sprite) == -1)
		return;

	object = lookupSprite(sprite);
	if (object && object->inCallback) {
		object->scriptActive = 1;
		object->inCallback = false;
	}
}

uint16 SpriteLibrary_v1_2::callMovieProc(uint16 sprite) {
	if (!_movieProcSet)
		return 1;

	SpriteObject_v1_2 *object = findSprite(sprite, "callMovieProc");
	if (!object || !object->queueCurrent)
		return 0;

	object->scriptActive = 0;
	object->inCallback = true;

	byte *frame = object->movieFrame;
	uint16 finished = (engine()->spriteMovieService(sprite, object->queueCurrent, &frame) == 0) ? 1 : 0;

	if (currentTickSprite() != sprite && findSpriteIndex(sprite) == -1)
		return 0;

	object = lookupSprite(sprite);
	if (!object)
		return 0;

	object->movieFrame = frame;

	if (!object->inCallback)
		return 0;

	object->scriptActive = 1;
	object->inCallback = false;
	return finished;
}

bool SpriteLibrary_v1_2::tickMovieFrameRecord(uint16 sprite) {
	if (callMovieProc(sprite) != 0)
		return true;

	SpriteObject_v1_2 *object = findSprite(sprite, "tickMovieFrameRecord");
	if (!object || !object->queueCurrent)
		return false;

	if (object->queueCurrent[0x0A] == 0)
		return false;

	byte *frame = object->movieFrame;
	if (!frame)
		return false;

	object->currentFrame = frame;

	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 widthMinusOne = (int16)(width - 1);
	int16 heightMinusOne = (int16)(height - 1);

	Common::Rect oldBox = object->boundingBox;

	object->boundingBox.left = (int16)(object->x + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET) - widthMinusOne / 2);
	object->boundingBox.right = (int16)(width + object->boundingBox.left);
	object->boundingBox.top = (int16)(object->y + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET) - heightMinusOne / 2);
	object->boundingBox.bottom = (int16)(height + object->boundingBox.top);

	object->useUnionRefresh = intersectRectLocal(oldBox, object->boundingBox, oldBox);
	object->framePending = false;
	object->visible = true;

	if (object->notifyOnFrame)
		notifySpriteFrameChange(sprite);

	renderSpriteToSurface(sprite);
	return false;
}

bool SpriteLibrary_v1_2::tickPlayRangeRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickPlayRangeRecord");
	if (!object)
		return false;

	uint32 now = engine()->getEngineMillis();
	if (now < object->frameDue)
		return false;

	byte *command = object->queueCurrent;
	uint16 remaining = READ_LE_UINT16(command + 6);
	if (remaining == 0)
		return true;

	uint16 interval = READ_LE_UINT16(command + 8);
	if (interval != TG_SPR_V1_2_INTERVAL_KEEP)
		object->frameInterval = interval;

	if (object->timingMode == 0 || object->frameDue == 0)
		object->frameDue = now;

	object->frameDue += object->frameInterval;
	object->framePending = true;
	object->visible = true;
	renderSpriteToSurface(sprite);

	remaining--;

	object = lookupSprite(sprite);
	if (!object)
		return false;

	if (object->queueCurrent == command)
		WRITE_LE_UINT16(command + 6, remaining);

	if (remaining != 0)
		return false;

	return object->frameInterval == 0;
}

bool SpriteLibrary_v1_2::tickMultiCelRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickMultiCelRecord");
	if (!object)
		return false;

	uint32 now = engine()->getEngineMillis();
	if (now < object->frameDue)
		return false;

	byte *command = object->queueCurrent;
	if (command[0x0C] != 0)
		return true;

	uint16 interval = READ_LE_UINT16(command + 0x0A);
	if (interval != TG_SPR_V1_2_INTERVAL_KEEP)
		object->frameInterval = interval;

	if (object->timingMode == 0 || object->frameDue == 0)
		object->frameDue = now;

	object->frameDue += object->frameInterval;

	Common::Rect oldBox = object->boundingBox;
	if (!object->parts)
		object->parts = SpritePartBlockPtr(new SpritePartBlock());

	uint16 count = command[0x0D];
	if (count > TG_SPR_V1_2_NUM_SPRITE_PARTS) {
		warning("SpriteLibrary_v1_2::tickMultiCelRecord(): %d parts do not fit in the part block", count);
		count = TG_SPR_V1_2_NUM_SPRITE_PARTS;
	}

	for (uint16 i = 0; i < count; i++) {
		uint16 index = READ_LE_UINT16(command + 2 + 2 * i);
		if (object->frames.size() <= index)
			return true;

		byte *frame = object->frames[index];
		if (!frame) {
			frame = getCelBitmap(object->frameIds[index], sprite);

			object = findSprite(sprite, "tickMultiCelRecord");
			if (!object)
				return false;
		}

		if (!frame)
			return true;

		object->parts->parts[i].frame = frame;
	}

	object->partCount = (byte)count;
	recomputeCelBoxArray(sprite);
	object->useUnionRefresh = intersectRectLocal(oldBox, object->boundingBox, oldBox);
	object->visible = true;
	command[0x0C] = 1;
	object->framePending = false;

	if (object->notifyOnFrame)
		notifySpriteFrameChange(sprite);

	renderSpriteToSurface(sprite);

	object = lookupSprite(sprite);
	return !object || object->frameInterval == 0;
}

bool SpriteLibrary_v1_2::tickQuadPathRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickQuadPathRecord");
	if (!object)
		return false;

	bool finished = false;
	uint32 now = engine()->getEngineMillis();

	if (now >= object->moveDue) {
		if (object->timingMode == 0 || object->moveDue == 0)
			object->moveDue = now;

		object->moveDue += object->moveInterval;
		if (object->animates && now >= object->frameDue) {
			object->framePending = true;
			if (object->timingMode == 0 || object->frameDue == 0)
				object->frameDue = now;

			object->frameDue += object->frameInterval;
		}

		finished = stepQuadPath(sprite);
	} else if (object->animates && now >= object->frameDue) {
		object->framePending = true;
		if (object->timingMode == 0 || object->frameDue == 0)
			object->frameDue = now;

		object->frameDue += object->frameInterval;
		object->useUnionRefresh = true;
		renderSpriteToSurface(sprite);
	}

	return finished;
}

bool SpriteLibrary_v1_2::tickLinearMoveRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickLinearMoveRecord");
	if (!object)
		return false;

	bool finished = false;
	uint32 now = engine()->getEngineMillis();

	if (now >= object->moveDue) {
		if (object->timingMode == 0 || object->moveDue == 0)
			object->moveDue = now;

		object->moveDue += object->moveInterval;
		if (object->animates && now >= object->frameDue) {
			object->framePending = true;
			if (object->timingMode == 0 || object->frameDue == 0)
				object->frameDue = now;

			object->frameDue += object->frameInterval;
		}

		finished = stepLinearMove(sprite);
	} else if (object->animates && now >= object->frameDue) {
		object->framePending = true;
		if (object->timingMode == 0 || object->frameDue == 0)
			object->frameDue = now;

		object->frameDue += object->frameInterval;
		object->useUnionRefresh = true;
		renderSpriteToSurface(sprite);
	}

	return finished;
}

bool SpriteLibrary_v1_2::tickDelayedMoveRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickDelayedMoveRecord");
	if (!object)
		return false;

	bool finished = false;
	bool frameChanged = false;
	uint32 now = engine()->getEngineMillis();

	if (object->animates && now >= object->frameDue) {
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
		int16 x = (int16)decodeOperand(READ_LE_UINT16(command + 3));
		int16 y = (int16)decodeOperand(READ_LE_UINT16(command + 5));
		updateSpritePosition(sprite, x, y, command[2]);
		finished = true;
	} else if (frameChanged) {
		renderSpriteToSurface(sprite);
	}

	return finished;
}

bool SpriteLibrary_v1_2::tickWaitRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickWaitRecord");
	if (!object)
		return false;

	uint32 now = engine()->getEngineMillis();
	byte *command = object->queueCurrent;
	if (command[4] != 0)
		return now >= object->frameDue;

	command[4] = 1;
	if (object->timingMode == 0 || object->frameDue == 0)
		object->frameDue = now;

	object->frameDue += decodeOperand(READ_LE_UINT16(command + 2));
	return false;
}

void SpriteLibrary_v1_2::tickFlipRecord(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "tickFlipRecord");
	if (!object)
		return;

	uint16 previous = flipFlags(object);
	byte *command = object->queueCurrent;

	object->mirrorX = (decodeOperand(READ_LE_UINT16(command + 2)) != 0);
	object->flipY = (decodeOperand(READ_LE_UINT16(command + 4)) != 0);

	if (flipFlags(object) != previous)
		renderSpriteToSurface(sprite);
}

void SpriteLibrary_v1_2::tickSetPositionRecord(uint16 sprite, const byte *command) {
	int16 x = (int16)decodeOperand(READ_LE_UINT16(command + 2));
	int16 y = (int16)decodeOperand(READ_LE_UINT16(command + 4));
	updateSpritePosition(sprite, x, y, command[6]);
}

bool SpriteLibrary_v1_2::stepQuadPath(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "stepQuadPath");
	if (!object)
		return false;

	byte *command = object->queueCurrent;
	uint16 step = (uint16)(READ_LE_UINT16(command + 0x19) + 1);
	WRITE_LE_UINT16(command + 0x19, step);
	uint16 steps = READ_LE_UINT16(command + 0x1B);

	if (step <= steps) {
		Common::Point point = computeQuadCurvePoint(step, steps, READ_LE_INT16(command + 0x0D), READ_LE_INT16(command + 0x0F),
		                                            READ_LE_INT16(command + 0x11), READ_LE_INT16(command + 0x13),
		                                            READ_LE_INT16(command + 0x15), READ_LE_INT16(command + 0x17));
		updateSpritePosition(sprite, point.x, point.y, 0);
	}

	return step >= steps;
}

bool SpriteLibrary_v1_2::stepLinearMove(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "stepLinearMove");
	if (!object)
		return false;

	byte *command = object->queueCurrent;
	uint16 step = (uint16)(READ_LE_UINT16(command + 0x12) + 1);
	WRITE_LE_UINT16(command + 0x12, step);
	uint16 total = READ_LE_UINT16(command + 0x14);

	if (step <= total) {
		Common::Point point = computeLinePoint(step, total, READ_LE_INT16(command + 0x0A), READ_LE_INT16(command + 0x0C),
		                                       READ_LE_INT16(command + 0x0E), READ_LE_INT16(command + 0x10));
		updateSpritePosition(sprite, point.x, point.y, 0);
	}

	return step >= total;
}

void SpriteLibrary_v1_2::applyFrameChange(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "applyFrameChange");
	if (!object)
		return;

	if (object->notifyOnFrame) {
		notifySpriteFrameChange(sprite);

		object = findSprite(sprite, "applyFrameChange");
		if (!object)
			return;
	}

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
		warning("SpriteLibrary_v1_2::applyFrameChange(): frame index %d is out of range", object->currentFrameIndex);
		return;
	}

	byte *frame = object->frames[object->currentFrameIndex];
	if (!frame) {
		frame = getCelBitmap(object->frameIds[object->currentFrameIndex], sprite);

		object = findSprite(sprite, "applyFrameChange");
		if (!object)
			return;
	}

	if (!frame)
		return;

	object->currentFrame = frame;

	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 widthMinusOne = (int16)(width - 1);
	int16 heightMinusOne = (int16)(height - 1);

	Common::Rect oldBox = object->boundingBox;

	object->boundingBox.left = (int16)(object->x + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET) - widthMinusOne / 2);
	object->boundingBox.right = (int16)(width + object->boundingBox.left);
	object->boundingBox.top = (int16)(object->y + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET) - heightMinusOne / 2);
	object->boundingBox.bottom = (int16)(height + object->boundingBox.top);

	object->useUnionRefresh = intersectRectLocal(oldBox, object->boundingBox, oldBox);
	object->framePending = false;
}

} // End of namespace Topgun
