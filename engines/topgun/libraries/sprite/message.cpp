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

#include "topgun/libraries/sprite/sprite_v1_2_3.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool SpriteLibrary_v1_2::spriteSendMessage(uint16 sprite, uint16 count, const uint16 *words) {
	if (!words) {
		warning("SpriteLibrary_v1_2::spriteSendMessage(): the message has no words");
		return false;
	}

	spriteEmptyQueue(sprite);

	const uint16 *params = words + 1;
	bool result = true;

	switch (words[0]) {
	case kSprV1_2OpPlayRange:
		result = buildPlayRangeMessage(1, params, sprite);
		break;

	case kSprV1_2OpCompositeBackground:
		compositeSpriteIntoBackground(sprite);
		break;

	case kSprV1_2OpQuadPath:
		result = buildQuadPathMessage(1, params, sprite);
		break;

	case kSprV1_2OpHide:
		spriteHide(sprite, true);
		break;

	case kSprV1_2OpLinearMove:
		result = buildLinearMoveMessage(1, params, sprite);
		break;

	case kSprV1_2OpSetPosition:
		updateSpritePosition(sprite, (int16)params[0], (int16)params[1], params[2]);
		break;

	case kSprV1_2OpTimingMode: {
		SpriteObject_v1_2 *object = findSprite(sprite, "spriteSendMessage");
		if (!object)
			return false;

		object->timingMode = (byte)params[0];
		object->moveDue = 0;
		object->frameDue = 0;
		break;
	}

	case kSprV1_2OpFrameInterval:
		spriteSetMotion(sprite, params[0]);
		break;

	case kSprV1_2OpMoveInterval:
		spriteSetSpeed(sprite, params[0]);
		break;

	default:
		result = false;
		break;
	}

	return result;
}

bool SpriteLibrary_v1_2_3::spritePostMessage(uint16 sprite, uint16 count, const uint16 *words) {
	return dispatchSpriteMessage(0, count, words, sprite);
}

bool SpriteLibrary_v1_2_3::spriteSendMessage(uint16 sprite, uint16 count, const uint16 *words) {
	return dispatchSpriteMessage(1, count, words, sprite);
}

bool SpriteLibrary_v1_2_3::dispatchSpriteMessage(uint16 mode, uint16 count, const uint16 *words, uint16 sprite) {
	if (!words) {
		warning("SpriteLibrary_v1_2_3::dispatchSpriteMessage(): the message has no words");
		return false;
	}

	const uint16 *params = words + 1;
	uint16 message = words[0];

	if (_engine->_version >= kVersionV1_2_8) {
		if (message == kSprV1_2_8MsgMarkLoopStart)
			return markQueueLoopStart(mode, sprite);

		if (message == kSprV1_2_8MsgLoopJump)
			return buildQueueLoopJumpMessage(mode, count, params, sprite);
	}

	if (_engine->_version >= kVersionV1_2_6) {
		if (message == kSprV1_2_6MsgDelayedMove)
			return buildDelayedMoveMessage(mode, count, params, sprite);

		if (message == kSprV1_2_6MsgSetActive)
			return buildSetActiveMessage(mode, params, sprite);

		if (message >= kSprV1_2_6MsgFrameInterval && message <= kSprV1_2_6MsgShowFrame)
			message--;
		else if (message == kSprV1_2_6MsgHostEvent)
			message = kSprV1_2_3MsgHostEvent;
		else if (message > kSprV1_2_6MsgSetActive)
			return false;
	}

	switch (message) {
	case kSprV1_2_3MsgPlayRange:
		return buildPlayRangeMessage(mode, params, sprite);

	case kSprV1_2_3MsgMultiCel:
		return buildMultiCelMessage(mode, count, params, sprite);

	case kSprV1_2_3MsgComposite:
		return buildCompositeMessage(mode, sprite);

	case kSprV1_2_3MsgQuadPath:
		return buildQuadPathMessage(mode, params, sprite);

	case kSprV1_2_3MsgFlip:
		return buildFlipMessage(mode, params, sprite);

	case kSprV1_2_3MsgHide:
		return buildHideMessage(mode, sprite);

	case kSprV1_2_3MsgLinearMove:
		return buildLinearMoveMessage(mode, params, sprite);

	case kSprV1_2_3MsgWait:
		return buildWaitMessage(mode, params, sprite);

	case kSprV1_2_3MsgSetPosition:
		return buildSetPositionMessage(mode, count, params, sprite);

	case kSprV1_2_3MsgTimingMode:
		return buildTimingModeMessage(mode, params, sprite);

	case kSprV1_2_3MsgFrameInterval:
		return buildFrameIntervalMessage(mode, params, sprite);

	case kSprV1_2_3MsgSetRange:
		return buildSetRangeMessage(mode, params, sprite);

	case kSprV1_2_3MsgMoveInterval:
		return buildMoveIntervalMessage(mode, count, params, sprite);

	case kSprV1_2_3MsgShowFrame:
		return buildShowFrameMessage(mode, count, params, sprite);

	case kSprV1_2_3MsgHostEvent:
		return buildHostEventMessage(mode, count, params, sprite);

	default:
		return false;
	}
}

byte *SpriteLibrary_v1_2::allocQueueRecord(uint16 explicitSize, uint16 opcode, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "allocQueueRecord");
	if (!object)
		return nullptr;

	if (object->queueStart)
		return growQueueForRecord(explicitSize, opcode, sprite);

	uint16 size = _queueCommandSizeTable[opcode];
	if (explicitSize != 0)
		size = explicitSize;

	uint16 slack = TG_SPR_V1_2_QUEUE_BLOCK_SLACK;
	uint16 rounding = TG_SPR_V1_2_QUEUE_BLOCK_ROUNDING;
	if (_engine->_version >= kVersionV1_2_8) {
		slack = TG_SPR_V1_2_8_QUEUE_BLOCK_SLACK;
		rounding = TG_SPR_V1_2_8_QUEUE_BLOCK_ROUNDING;
	}

	uint32 capacity = (uint16)(size + slack) & rounding;

	// The original leaves the bytes of a new block uninitialized
	byte *block = engine()->utilsLibrary()->tgMalloc(capacity, 1);
	if (!block)
		return nullptr;

	object->queueStart = block;
	object->queueCurrent = (_engine->_version >= kVersionV1_2_3) ? nullptr : block;
	object->queueEnd = block + size;
	object->queueCapacityStart = block;
	object->queueCapacity = capacity;

	WRITE_LE_UINT16(block, opcode);
	return block;
}

byte *SpriteLibrary_v1_2::growQueueForRecord(uint16 explicitSize, uint16 opcode, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "growQueueForRecord");
	if (!object)
		return nullptr;

	uint16 size = _queueCommandSizeTable[opcode];
	if (explicitSize != 0)
		size = explicitSize;

	uint32 used = (uint32)(object->queueEnd - object->queueStart);
	uint32 needed = used + size;
	uint32 capacity = used;
	if (object->queueCapacityStart == object->queueStart)
		capacity = object->queueCapacity;

	if (needed > capacity) {
		uint16 slack = TG_SPR_V1_2_QUEUE_BLOCK_SLACK;
		uint16 rounding = TG_SPR_V1_2_QUEUE_BLOCK_ROUNDING;
		if (_engine->_version >= kVersionV1_2_8) {
			slack = TG_SPR_V1_2_8_QUEUE_BLOCK_SLACK;
			rounding = TG_SPR_V1_2_8_QUEUE_BLOCK_ROUNDING;
		}

		uint32 newCapacity = (uint16)(needed + slack) & rounding;

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

void SpriteLibrary_v1_2::resetQueueForImmediate(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "resetQueueForImmediate");
	if (!object)
		return;

	object->queueEnd = object->queueStart;
	object->queueCurrent = object->queueStart;
	object->advancePending = false;
	object->loopsDisabled = false;
}

void SpriteLibrary_v1_2::runImmediateRecord(byte *record, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "runImmediateRecord");
	if (!object)
		return;

	object->queueCurrent = record;
	if (processSpriteAnimationStep(sprite))
		spriteEmptyQueue(sprite);
}

bool SpriteLibrary_v1_2::buildPlayRangeMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0 && _engine->_version >= kVersionV1_2_3)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(0, kSprV1_2OpPlayRange, sprite);
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
		WRITE_LE_UINT16(record + 2, (uint16)(READ_LE_UINT16(record + 2) - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 4, (uint16)(READ_LE_UINT16(record + 4) - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 8, (uint16)(READ_LE_UINT16(record + 8) - TG_SPR_V1_2_3_LITERAL_BIAS));

		SpriteObject_v1_2 *object = findSprite(sprite, "buildPlayRangeMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildMultiCelMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	uint16 paramCount = (uint16)(count - 2);
	uint16 maxParamCount = (_engine->_version >= kVersionV1_2_7) ? 11 : 5;
	if (paramCount < 3 || paramCount > maxParamCount)
		return false;

	if (mode != 0)
		resetQueueForImmediate(sprite);

	uint16 explicitSize = 0;
	uint16 celsOffset = 2;
	if (_engine->_version >= kVersionV1_2_7) {
		explicitSize = (uint16)((paramCount + 3) * 2);
		celsOffset = 8;
	}

	byte *record = allocQueueRecord(explicitSize, kSprV1_2_3OpMultiCel, sprite);
	if (!record)
		return false;

	uint16 frames = (uint16)(paramCount - 1);
	for (uint16 i = 0; i < frames; i++)
		WRITE_LE_UINT16(record + celsOffset + 2 * i, (uint16)(params[i] - TG_SPR_V1_2_3_LITERAL_BIAS));

	if (_engine->_version >= kVersionV1_2_7) {
		WRITE_LE_UINT16(record + 2, (uint16)(params[frames] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 6, frames);
		WRITE_LE_UINT16(record + 4, 0);
	} else if (_engine->_version >= kVersionV1_2_5) {
		WRITE_LE_UINT16(record + 0x0A, (uint16)(params[frames] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 0x0E, frames);
		WRITE_LE_UINT16(record + 0x0C, 0);
	} else {
		WRITE_LE_UINT16(record + 0x0A, params[frames]);
		record[0x0D] = (byte)frames;
		record[0x0C] = 0;
	}

	if (mode != 0) {
		runImmediateRecord(record, sprite);
	} else {
		if (_engine->_version < kVersionV1_2_5)
			WRITE_LE_UINT16(record + 0x0A, (uint16)(READ_LE_UINT16(record + 0x0A) - TG_SPR_V1_2_3_LITERAL_BIAS));

		SpriteObject_v1_2 *object = findSprite(sprite, "buildMultiCelMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::markQueueLoopStart(uint16 mode, uint16 sprite) {
	if (mode != 0)
		return true;

	SpriteObject_v1_2 *object = findSprite(sprite, "markQueueLoopStart");
	if (!object)
		return false;

	object->queueLoopStart = (uint16)(object->queueEnd - object->queueStart);
	return true;
}

bool SpriteLibrary_v1_2_3::buildQueueLoopJumpMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	if (mode != 0)
		return true;

	byte *record = allocQueueRecord(0, kSprV1_2OpLoop, sprite);
	if (!record)
		return false;

	if (count != 0)
		WRITE_LE_UINT16(record + 2, params[0]);
	else
		WRITE_LE_UINT16(record + 2, 0);

	WRITE_LE_UINT16(record + 4, READ_LE_UINT16(record + 2));

	SpriteObject_v1_2 *object = findSprite(sprite, "buildQueueLoopJumpMessage");
	if (!object)
		return false;

	WRITE_LE_UINT16(record + 6, object->queueLoopStart);

	if (!object->queueCurrent)
		processSpriteQueueCommand(sprite);

	return true;
}

bool SpriteLibrary_v1_2_3::buildCompositeMessage(uint16 mode, uint16 sprite) {
	if (mode != 0) {
		compositeSpriteIntoBackground(sprite, 0);
		return true;
	}

	byte *record = allocQueueRecord(0, kSprV1_2_3OpCompositeBackground, sprite);
	if (!record)
		return false;

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildCompositeMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2::buildQuadPathMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0 && _engine->_version >= kVersionV1_2_3)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(0, kSprV1_2OpQuadPath, sprite);
	if (!record)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "buildQuadPathMessage");
	if (!object)
		return false;

	if (mode != 0) {
		uint16 shift = (_engine->_version >= kVersionV1_2_3) ? 1 : 0;

		WRITE_LE_INT16(record + 0x0D + shift, object->x);
		WRITE_LE_INT16(record + 0x0F + shift, object->y);
		WRITE_LE_UINT16(record + 0x11 + shift, params[0]);
		WRITE_LE_UINT16(record + 0x13 + shift, params[1]);
		WRITE_LE_UINT16(record + 0x15 + shift, params[2]);
		WRITE_LE_UINT16(record + 0x17 + shift, params[3]);
		WRITE_LE_UINT16(record + 0x1B + shift, params[4]);

		if (params[5] != 0) {
			WRITE_LE_UINT16(record + 0x11 + shift, (uint16)(READ_LE_UINT16(record + 0x11 + shift) + object->x));
			WRITE_LE_UINT16(record + 0x13 + shift, (uint16)(READ_LE_UINT16(record + 0x13 + shift) + object->y));
			WRITE_LE_UINT16(record + 0x15 + shift, (uint16)(READ_LE_UINT16(record + 0x15 + shift) + object->x));
			WRITE_LE_UINT16(record + 0x17 + shift, (uint16)(READ_LE_UINT16(record + 0x17 + shift) + object->y));
		}

		WRITE_LE_UINT16(record + 0x19 + shift, 0);
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 6, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 8, (uint16)(params[1] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 0x0A, (uint16)(params[2] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 0x0C, (uint16)(params[3] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 2, (uint16)(params[4] - TG_SPR_V1_2_3_LITERAL_BIAS));
		if (_engine->_version >= kVersionV1_2_5)
			WRITE_LE_UINT16(record + 4, (params[5] != 0) ? 1 : 0);
		else
			record[4] = (params[5] != 0) ? 1 : 0;

		if (!object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildFlipMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildFlipMessage");
		if (!object)
			return false;

		uint16 previous = flipFlags(object);
		bool mirrorX = (params[0] != 0);
		bool flipY = (params[1] != 0);
		uint16 current = (uint16)((mirrorX ? 1 : 0) | (flipY ? 0x100 : 0));

		if (_engine->_version >= kVersionV1_2_6 && !object->active) {
			object->shadowMirrorX = mirrorX;
			object->shadowFlipY = flipY;
		} else {
			object->mirrorX = mirrorX;
			object->flipY = flipY;
		}

		if (current != previous) {
			if (object->partCount != 0)
				recomputeCelBoxArray(sprite);

			renderSpriteToSurface(sprite);
		}

		return true;
	}

	byte *record = allocQueueRecord(0, kSprV1_2_3OpFlip, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(((params[0] != 0) ? 1 : 0) - TG_SPR_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, (uint16)(((params[1] != 0) ? 1 : 0) - TG_SPR_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 8, 0x8ACF);
	WRITE_LE_UINT16(record + 6, 0x8ACF);

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildFlipMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildHideMessage(uint16 mode, uint16 sprite) {
	if (mode != 0) {
		spriteHide(sprite, true);
		return true;
	}

	byte *record = allocQueueRecord(0, kSprV1_2_3OpHide, sprite);
	if (!record)
		return false;

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildHideMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2::buildLinearMoveMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0 && _engine->_version >= kVersionV1_2_3)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(0, kSprV1_2OpLinearMove, sprite);
	if (!record)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "buildLinearMoveMessage");
	if (!object)
		return false;

	uint16 shift = (_engine->_version >= kVersionV1_2_5) ? 2 : 0;

	if (mode != 0) {
		WRITE_LE_INT16(record + 0x0A + shift, object->x);
		WRITE_LE_INT16(record + 0x0C + shift, object->y);
		WRITE_LE_UINT16(record + 0x0E + shift, params[0]);
		WRITE_LE_UINT16(record + 0x10 + shift, params[1]);
		WRITE_LE_UINT16(record + 0x14 + shift, params[2]);

		if (params[4] != 0) {
			WRITE_LE_UINT16(record + 0x0E + shift, (uint16)(READ_LE_UINT16(record + 0x0E + shift) + object->x));
			WRITE_LE_UINT16(record + 0x10 + shift, (uint16)(READ_LE_UINT16(record + 0x10 + shift) + object->y));
		}

		if (params[3] != 0)
			computeInterpolatedMoveSteps(record);

		WRITE_LE_UINT16(record + 0x12 + shift, 0);
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 6 + shift, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 8 + shift, (uint16)(params[1] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 2, (uint16)(params[2] - TG_SPR_V1_2_3_LITERAL_BIAS));

		if (_engine->_version >= kVersionV1_2_5) {
			WRITE_LE_UINT16(record + 4, (params[3] != 0) ? 1 : 0);
			WRITE_LE_UINT16(record + 6, (params[4] != 0) ? 1 : 0);
		} else {
			record[4] = (params[3] != 0) ? 1 : 0;
			record[5] = (params[4] != 0) ? 1 : 0;
		}

		if (!object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildHostEventMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	if (count < 3 || count > 9)
		return false;

	if (mode != 0)
		resetQueueForImmediate(sprite);

	uint16 opcode = kSprV1_2_3OpHostEventFixed;
	if (_engine->_version >= kVersionV1_2_6)
		opcode = kSprV1_2_6OpHostEventFixed;

	byte *record = allocQueueRecord(0, opcode, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));

	uint16 extraWords = (uint16)(count - 3);
	uint16 maxExtraWords = (_engine->_version >= kVersionV1_2_5) ? TG_SPR_V1_2_5_MAX_HOST_EVENT_WORDS : TG_SPR_V1_2_3_MAX_HOST_EVENT_WORDS;
	if (extraWords > maxExtraWords) {
		warning("SpriteLibrary_v1_2_3::buildHostEventMessage(): %d extra words do not fit in the record", extraWords);
		extraWords = maxExtraWords;
	}

	for (uint16 i = 0; i < extraWords; i++)
		WRITE_LE_UINT16(record + 6 + 2 * i, (uint16)(params[1 + i] - TG_SPR_V1_2_3_LITERAL_BIAS));

	WRITE_LE_UINT16(record + 4, extraWords);

	SpriteObject_v1_2 *object = findSprite(sprite, "buildHostEventMessage");
	if (object && !object->queueCurrent)
		processSpriteQueueCommand(sprite);

	return true;
}

bool SpriteLibrary_v1_2_3::buildWaitMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0)
		return true;

	byte *record = allocQueueRecord(0, kSprV1_2_3OpWait, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, 0);

	SpriteObject_v1_2 *object = findSprite(sprite, "buildWaitMessage");
	if (object && !object->queueCurrent)
		processSpriteQueueCommand(sprite);

	return true;
}

bool SpriteLibrary_v1_2_3::buildDelayedMoveMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	if (count != 5)
		return false;

	if (mode != 0)
		resetQueueForImmediate(sprite);

	byte *record = allocQueueRecord(0, kSprV1_2_3OpDelayedMove, sprite);
	if (!record)
		return false;

	if (mode != 0) {
		WRITE_LE_UINT16(record + 4, params[0]);
		WRITE_LE_UINT16(record + 6, params[1]);
		WRITE_LE_UINT16(record + 2, (params[2] != 0) ? 1 : 0);
		runImmediateRecord(record, sprite);
	} else {
		WRITE_LE_UINT16(record + 4, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 6, (uint16)(params[1] - TG_SPR_V1_2_3_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 2, (params[2] != 0) ? 1 : 0);

		SpriteObject_v1_2 *object = findSprite(sprite, "buildDelayedMoveMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildSetActiveMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		setSpriteActive(sprite, params[0], true);
		return true;
	}

	byte *record = allocQueueRecord(0, kSprV1_2_6OpSetActive, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (params[0] != 0) ? 1 : 0);

	SpriteObject_v1_2 *object = findSprite(sprite, "buildSetActiveMessage");
	if (object && !object->queueCurrent)
		processSpriteQueueCommand(sprite);

	return true;
}

bool SpriteLibrary_v1_2_3::buildSetPositionMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
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

	byte *record = allocQueueRecord(0, kSprV1_2_3OpSetPosition, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, (uint16)(params[1] - TG_SPR_V1_2_3_LITERAL_BIAS));
	if (_engine->_version >= kVersionV1_2_5) {
		WRITE_LE_UINT16(record + 6, (relative != 0) ? 1 : 0);

		SpriteObject_v1_2 *object = findSprite(sprite, "buildSetPositionMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	} else {
		record[6] = (relative != 0) ? 1 : 0;
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildTimingModeMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildTimingModeMessage");
		if (!object)
			return false;

		object->timingMode = (params[0] != 0) ? 1 : 0;
		object->moveDue = 0;
		object->frameDue = 0;
		return true;
	}

	byte *record = allocQueueRecord(0, kSprV1_2_3OpTimingMode, sprite);
	if (!record)
		return false;

	if (_engine->_version >= kVersionV1_2_5) {
		WRITE_LE_UINT16(record + 2, (params[0] != 0) ? 1 : 0);

		SpriteObject_v1_2 *object = findSprite(sprite, "buildTimingModeMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	} else {
		record[2] = (params[0] != 0) ? 1 : 0;
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildSetRangeMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		spriteObjectSetCelRange(sprite, params[0], params[1], params[2]);
		return true;
	}

	uint16 opcode = kSprV1_2_3OpSetRange;
	if (_engine->_version >= kVersionV1_2_6)
		opcode = kSprV1_2_6OpSetRange;

	byte *record = allocQueueRecord(0, opcode, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 4, (uint16)(params[1] - TG_SPR_V1_2_3_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 6, (uint16)(params[2] - TG_SPR_V1_2_3_LITERAL_BIAS));

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildSetRangeMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildFrameIntervalMessage(uint16 mode, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildFrameIntervalMessage");
		if (!object)
			return false;

		uint32 interval = params[0];
		if (_engine->_version >= kVersionV1_2_5)
			interval = (uint32)(int32)(int16)params[0];

		if (object->queueStart) {
			object->frameDue -= object->frameInterval;
			object->frameInterval = interval;
			object->frameDue += object->frameInterval;
		} else {
			spriteSetMotion(sprite, interval);
		}

		return true;
	}

	uint16 opcode = kSprV1_2_3OpFrameInterval;
	if (_engine->_version >= kVersionV1_2_6)
		opcode = kSprV1_2_6OpFrameInterval;

	byte *record = allocQueueRecord(0, opcode, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildFrameIntervalMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildMoveIntervalMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	if (mode != 0) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildMoveIntervalMessage");
		if (!object)
			return false;

		uint32 interval = params[0];
		if (_engine->_version >= kVersionV1_2_5)
			interval = (uint32)(int32)(int16)params[0];

		if (object->queueStart) {
			object->moveDue -= object->moveInterval;
			object->moveInterval = interval;
			object->moveDue += object->moveInterval;
		} else {
			spriteSetSpeed(sprite, interval);
		}

		if (count > 1)
			buildFrameIntervalMessage(mode, params + 1, sprite);

		return true;
	}

	uint16 opcode = kSprV1_2_3OpMoveInterval;
	if (_engine->_version >= kVersionV1_2_6)
		opcode = kSprV1_2_6OpMoveInterval;

	byte *record = allocQueueRecord(0, opcode, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, (uint16)(params[0] - TG_SPR_V1_2_3_LITERAL_BIAS));
	if (count > 1)
		WRITE_LE_UINT16(record + 4, (uint16)(params[1] - TG_SPR_V1_2_3_LITERAL_BIAS));

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildMoveIntervalMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

bool SpriteLibrary_v1_2_3::buildShowFrameMessage(uint16 mode, uint16 count, const uint16 *params, uint16 sprite) {
	uint16 frame = (count != 0) ? params[0] : 0xFFFF;

	if (mode != 0) {
		spriteShow(sprite, frame, true);
		return true;
	}

	uint16 opcode = kSprV1_2_3OpShowFrame;
	if (_engine->_version >= kVersionV1_2_6)
		opcode = kSprV1_2_6OpShowFrame;

	byte *record = allocQueueRecord(0, opcode, sprite);
	if (!record)
		return false;

	WRITE_LE_UINT16(record + 2, frame);

	if (_engine->_version >= kVersionV1_2_5) {
		SpriteObject_v1_2 *object = findSprite(sprite, "buildShowFrameMessage");
		if (object && !object->queueCurrent)
			processSpriteQueueCommand(sprite);
	}

	return true;
}

} // End of namespace Topgun
