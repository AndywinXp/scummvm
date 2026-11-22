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

#include "topgun/libraries/sprite/sprite_v1_2_1.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_1.h"

namespace Topgun {

bool SpriteLibrary_v1_2_1::spritePostMessage(uint16 sprite, uint16 count, const uint16 *words) {
	return dispatchSpriteMessage(0, count, words, sprite);
}

bool SpriteLibrary_v1_2_1::spriteSendMessage(uint16 sprite, uint16 count, const uint16 *words) {
	return dispatchSpriteMessage(1, count, words, sprite);
}

bool SpriteLibrary_v1_2_1::dispatchSpriteMessage(uint16 mode, uint16 count, const uint16 *words, uint16 sprite) {
	if (!words) {
		warning("SpriteLibrary_v1_2_1::dispatchSpriteMessage(): the message has no words");
		return false;
	}

	const uint16 *params = words + 1;

	switch (words[0]) {
	case kSprV1_2_1MsgPlayRange:
		return buildPlayRangeMessage(mode, params, sprite);

	case kSprV1_2_1MsgMultiCel:
		return buildMultiCelMessage(mode, count, params, sprite);

	case kSprV1_2_1MsgComposite:
		return buildCompositeMessage(mode, sprite);

	case kSprV1_2_1MsgQuadPath:
		return buildQuadPathMessage(mode, params, sprite);

	case kSprV1_2_1MsgFlip:
		return buildFlipMessage(mode, params, sprite);

	case kSprV1_2_1MsgHide:
		return buildHideMessage(mode, sprite);

	case kSprV1_2_1MsgLinearMove:
		return buildLinearMoveMessage(mode, params, sprite);

	case kSprV1_2_1MsgWait:
		return buildWaitMessage(mode, params, sprite);

	case kSprV1_2_1MsgSetPosition:
		return buildSetPositionMessage(mode, count, params, sprite);

	case kSprV1_2_1MsgTimingMode:
		return buildTimingModeMessage(mode, params, sprite);

	case kSprV1_2_1MsgFrameInterval:
		return buildFrameIntervalMessage(mode, params, sprite);

	case kSprV1_2_1MsgSetRange:
		return buildSetRangeMessage(mode, params, sprite);

	case kSprV1_2_1MsgMoveInterval:
		return buildMoveIntervalMessage(mode, count, params, sprite);

	case kSprV1_2_1MsgShowFrame:
		return buildShowFrameMessage(mode, count, params, sprite);

	case kSprV1_2_1MsgHostEvent:
		return buildHostEventMessage(mode, count, params, sprite);

	default:
		return false;
	}
}

byte *SpriteLibrary_v1_2_1::allocQueueRecord(uint16 opcode, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "allocQueueRecord");
	if (!object)
		return nullptr;

	if (object->queueStart)
		return growQueueForRecord(opcode, sprite);

	uint16 size = _queueCommandSizeTable[opcode];
	uint32 capacity = (uint16)(size + TG_SPR_V1_2_1_QUEUE_BLOCK_SLACK) & TG_SPR_V1_2_1_QUEUE_BLOCK_ROUNDING;

	// The original leaves the bytes of a new block uninitialized
	byte *block = engine()->utilsLibrary()->tgMalloc(capacity, 1);
	if (!block)
		return nullptr;

	object->queueStart = block;
	object->queueCurrent = nullptr;
	object->queueEnd = block + size;
	object->queueCapacityStart = block;
	object->queueCapacity = capacity;

	WRITE_LE_UINT16(block, opcode);
	return block;
}

byte *SpriteLibrary_v1_2_1::growQueueForRecord(uint16 opcode, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "growQueueForRecord");
	if (!object)
		return nullptr;

	uint32 used = (uint32)(object->queueEnd - object->queueStart);
	uint32 needed = used + _queueCommandSizeTable[opcode];
	uint32 capacity = used;
	if (object->queueCapacityStart == object->queueStart)
		capacity = object->queueCapacity;

	if (needed > capacity) {
		uint32 newCapacity = (uint16)(needed + TG_SPR_V1_2_1_QUEUE_BLOCK_SLACK) & TG_SPR_V1_2_1_QUEUE_BLOCK_ROUNDING;

		int32 currentOffset = -1;
		if (object->queueCurrent)
			currentOffset = (int32)(object->queueCurrent - object->queueStart);

		byte *block = engine()->utilsLibrary()->tgRealloc(object->queueStart, newCapacity, 1);
		if (!block)
			return nullptr;

		if (currentOffset >= 0)
			object->queueCurrent = block + currentOffset;

		object->queueStart = block;
		object->queueEnd = block + used;
		object->queueCapacityStart = block;
		object->queueCapacity = newCapacity;
	}

	byte *record = object->queueEnd;
	WRITE_LE_UINT16(record, opcode);
	object->queueEnd = object->queueStart + needed;
	return record;
}

void SpriteLibrary_v1_2_1::resetQueueForImmediate(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "resetQueueForImmediate");
	if (!object)
		return;

	object->queueEnd = object->queueStart;
	object->queueCurrent = object->queueStart;
	object->advancePending = false;
	object->loopsDisabled = false;
}

void SpriteLibrary_v1_2_1::runImmediateRecord(byte *record, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "runImmediateRecord");
	if (!object)
		return;

	object->queueCurrent = record;
	if (processSpriteAnimationStep(sprite))
		spriteEmptyQueue(sprite);
}

bool SpriteLibrary_v1_2_1::buildPlayRangeMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(kSprV1_2_1OpPlayRange, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, params[0]);
	WRITE_LE_UINT16(record + 4, params[1]);
	WRITE_LE_UINT16(record + 8, params[2]);

	if (mode != 0) {
		uint16 remaining = spriteObjectSetCelRange(sprite, READ_LE_UINT16(record + 2), READ_LE_UINT16(record + 2), READ_LE_UINT16(record + 4));
		WRITE_LE_UINT16(record + 6, remaining);
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 2, (uint16)(READ_LE_UINT16(record + 2) - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 4, (uint16)(READ_LE_UINT16(record + 4) - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 8, (uint16)(READ_LE_UINT16(record + 8) - TG_SPR_V1_2_1_LITERAL_BIAS));

		SpriteObject_v1_2 *object = findSprite(sprite, "buildPlayRangeMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_1::buildMultiCelMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	uint16 paramCount = (uint16)(count - 2);
	if (paramCount < 3 || paramCount > 5)
		return false;

	if (mode != 0)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(kSprV1_2_1OpMultiCel, sprite);
	if (!record)
		return false;

	uint16 frames = (uint16)(paramCount - 1);
	for (uint16 i = 0; i < frames; i++)
		WRITE_LE_UINT16(record + 2 + 2 * i, (uint16)(params[i] - TG_SPR_V1_2_1_LITERAL_BIAS));

	WRITE_LE_UINT16(record + 0x0A, params[frames]);
	record[0x0D] = (byte)frames;
	record[0x0C] = 0;

	if (mode != 0) {
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 0x0A, (uint16)(READ_LE_UINT16(record + 0x0A) - TG_SPR_V1_2_1_LITERAL_BIAS));

		SpriteObject_v1_2 *object = findSprite(sprite, "buildMultiCelMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_1::buildCompositeMessage(uint16 mode, uint16 sprite) {
	if (mode != 0) {
		compositeSpriteIntoBackground(sprite, 0);
		return true;
	}

	return allocQueueRecord(kSprV1_2_1OpCompositeBackground, sprite);
}

bool SpriteLibrary_v1_2_1::buildQuadPathMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(kSprV1_2_1OpQuadPath, sprite);
	if (!record)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "buildQuadPathMessage");
	if (!object)
		return false;

	if (mode != 0) {
		WRITE_LE_INT16(record + 0x0E, object->x);
		WRITE_LE_INT16(record + 0x10, object->y);
		WRITE_LE_UINT16(record + 0x12, params[0]);
		WRITE_LE_UINT16(record + 0x14, params[1]);
		WRITE_LE_UINT16(record + 0x16, params[2]);
		WRITE_LE_UINT16(record + 0x18, params[3]);
		WRITE_LE_UINT16(record + 0x1C, params[4]);

		if (params[5] != 0) {
			WRITE_LE_UINT16(record + 0x12, (uint16)(READ_LE_UINT16(record + 0x12) + object->x));
			WRITE_LE_UINT16(record + 0x14, (uint16)(READ_LE_UINT16(record + 0x14) + object->y));
			WRITE_LE_UINT16(record + 0x16, (uint16)(READ_LE_UINT16(record + 0x16) + object->x));
			WRITE_LE_UINT16(record + 0x18, (uint16)(READ_LE_UINT16(record + 0x18) + object->y));
		}

		WRITE_LE_UINT16(record + 0x1A, 0);
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 6, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 8, (uint16)(params[1] - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 0x0A, (uint16)(params[2] - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 0x0C, (uint16)(params[3] - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 2, (uint16)(params[4] - TG_SPR_V1_2_1_LITERAL_BIAS));
		record[4] = (params[5] != 0) ? 1 : 0;

		if (!object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_1::buildFlipMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildFlipMessage");
		if (!object)
			return false;

		uint16 previous = flipFlags(object);
		object->mirrorX = (params[0] != 0);
		object->flipY = (params[1] != 0);

		if (flipFlags(object) != previous) {
			if (object->partCount != 0)
				recomputeCelBoxArray(sprite);

			renderSpriteToSurface(sprite);
		}

		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpFlip, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(((params[0] != 0) ? 1 : 0) - TG_SPR_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, (uint16)(((params[1] != 0) ? 1 : 0) - TG_SPR_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 8, 0x8ACF);
	WRITE_LE_UINT16(record + 6, 0x8ACF);
	return true;
}

bool SpriteLibrary_v1_2_1::buildHideMessage(uint16 mode, uint16 sprite) {
	if (mode != 0) {
		spriteHide(sprite, true);
		return true;
	}

	return allocQueueRecord(kSprV1_2_1OpHide, sprite);
}

bool SpriteLibrary_v1_2_1::buildLinearMoveMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(kSprV1_2_1OpLinearMove, sprite);
	if (!record)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "buildLinearMoveMessage");
	if (!object)
		return false;

	if (mode != 0) {
		WRITE_LE_INT16(record + 0x0A, object->x);
		WRITE_LE_INT16(record + 0x0C, object->y);
		WRITE_LE_UINT16(record + 0x0E, params[0]);
		WRITE_LE_UINT16(record + 0x10, params[1]);
		WRITE_LE_UINT16(record + 0x14, params[2]);

		if (params[4] != 0) {
			WRITE_LE_UINT16(record + 0x0E, (uint16)(READ_LE_UINT16(record + 0x0E) + object->x));
			WRITE_LE_UINT16(record + 0x10, (uint16)(READ_LE_UINT16(record + 0x10) + object->y));
		}

		if (params[3] != 0)
			computeInterpolatedMoveSteps(record);

		WRITE_LE_UINT16(record + 0x12, 0);
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 6, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 8, (uint16)(params[1] - TG_SPR_V1_2_1_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 2, (uint16)(params[2] - TG_SPR_V1_2_1_LITERAL_BIAS));
		record[4] = (params[3] != 0) ? 1 : 0;
		record[5] = (params[4] != 0) ? 1 : 0;

		if (!object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_1::buildHostEventMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	if (count < 3 || count > 9)
		return false;

	if (mode != 0)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(kSprV1_2_1OpHostEventFixed, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));

	uint16 extraWords = (uint16)(count - 3);
	if (extraWords > TG_SPR_V1_2_1_MAX_HOST_EVENT_WORDS) {
		warning("SpriteLibrary_v1_2_1::buildHostEventMessage(): %d extra words do not fit in the record", extraWords);
		extraWords = TG_SPR_V1_2_1_MAX_HOST_EVENT_WORDS;
	}

	for (uint16 i = 0; i < extraWords; i++)
		WRITE_LE_UINT16(record + 6 + 2 * i, (uint16)(params[1 + i] - TG_SPR_V1_2_1_LITERAL_BIAS));

	WRITE_LE_UINT16(record + 4, extraWords);

	SpriteObject_v1_2 *object = findSprite(sprite, "buildHostEventMessage");
	if (object && !object->queueCurrent)
		processSpriteQueueCommand(sprite);

	return true;
}

bool SpriteLibrary_v1_2_1::buildWaitMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0)
		return true;

	byte *record = allocQueueRecord(kSprV1_2_1OpWait, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, 0);

	SpriteObject_v1_2 *object = findSprite(sprite, "buildWaitMessage");
	if (object && !object->queueCurrent)
		processSpriteQueueCommand(sprite);

	return true;
}

bool SpriteLibrary_v1_2_1::buildSetPositionMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	uint16 paramCount = (uint16)(count - 2);
	if (paramCount < 2)
		return false;

	int16 x = (int16)params[0];
	int16 y = (int16)params[1];
	uint16 relative = (paramCount > 2) ? params[2] : 0;

	if (mode != 0) {
		updateSpritePosition(sprite, x, y, relative, true);
		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpSetPosition, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, (uint16)(params[1] - TG_SPR_V1_2_1_LITERAL_BIAS));
	record[6] = (relative != 0) ? 1 : 0;
	return true;
}

bool SpriteLibrary_v1_2_1::buildTimingModeMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildTimingModeMessage");
		if (!object)
			return false;

		object->timingMode = (params[0] != 0) ? 1 : 0;
		object->moveDue = 0;
		object->frameDue = 0;
		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpTimingMode, sprite);
	if (!record)
		return false;

	record[2] = (params[0] != 0) ? 1 : 0;
	return true;
}

bool SpriteLibrary_v1_2_1::buildSetRangeMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		spriteObjectSetCelRange(sprite, params[0], params[1], params[2]);
		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpSetRange, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, (uint16)(params[1] - TG_SPR_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 6, (uint16)(params[2] - TG_SPR_V1_2_1_LITERAL_BIAS));
	return true;
}

bool SpriteLibrary_v1_2_1::buildFrameIntervalMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildFrameIntervalMessage");
		if (!object)
			return false;

		if (object->queueStart) {
			object->frameDue -= object->frameInterval;
			object->frameInterval = params[0];
			object->frameDue += object->frameInterval;
		} else {
			spriteSetMotion(sprite, params[0]);
		}

		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpFrameInterval, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
	return true;
}

bool SpriteLibrary_v1_2_1::buildMoveIntervalMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildMoveIntervalMessage");
		if (!object)
			return false;

		if (object->queueStart) {
			object->moveDue -= object->moveInterval;
			object->moveInterval = params[0];
			object->moveDue += object->moveInterval;
		} else {
			spriteSetSpeed(sprite, params[0]);
		}

		if (count > 1)
			buildFrameIntervalMessage(mode, params + 1, sprite);

		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpMoveInterval, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_1_LITERAL_BIAS));
	if (count > 1)
		WRITE_LE_UINT16(record + 4, (uint16)(params[1] - TG_SPR_V1_2_1_LITERAL_BIAS));

	return true;
}

bool SpriteLibrary_v1_2_1::buildShowFrameMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	uint16 frame = (count != 0) ? params[0] : 0xFFFF;

	if (mode != 0) {
		spriteShow(sprite, frame, true);
		return true;
	}

	byte *record = allocQueueRecord(kSprV1_2_1OpShowFrame, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, frame);
	return true;
}

} // End of namespace Topgun
