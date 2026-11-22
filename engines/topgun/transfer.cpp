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

#include "topgun/topgun_v1_2_1.h"

namespace Topgun {

bool TopgunEngine_v1_2::transferSpriteContents(uint16 sourceId, uint16 destinationId, bool freeSource) {
	uint16 sourceIndex = operandValue(sourceId);
	if (!isRecordIndexValid(sourceIndex, "transferSpriteContents"))
		return false;

	uint16 sourceSprite = _records[sourceIndex].handle;

	SpriteInfo_v1_2 sourceInfo;
	spriteLib()->spriteGetInfo(sourceSprite, sourceInfo);
	spriteLib()->spriteEmptyQueue(sourceSprite);
	spriteLib()->spriteClearUpdate(sourceSprite);

	if (freeSource)
		hideSourceSpriteAndAddFrameRefs(sourceInfo, sourceId);

	uint16 destinationIndex = operandValue(destinationId);
	if (!isRecordIndexValid(destinationIndex, "transferSpriteContents"))
		return false;

	bool destinationExists = (_records[destinationIndex].handle != 0);

	if (!destinationExists) {
		if (!validateAndGetResourceByType(destinationId, (_version >= kVersionV1_2_1) ? kResV1_2_1Sprite : kResV1_2Sprite))
			return false;
	}

	uint16 destinationSprite = _records[destinationIndex].handle;

	SpriteInfo_v1_2 destinationInfo;
	spriteLib()->spriteGetInfo(destinationSprite, destinationInfo);

	bool appended = false;

	if (!destinationExists || destinationInfo.visible == 0) {
		if (appendQueueBitmapsToSprite(sourceInfo, destinationId)) {
			spriteLib()->spriteTransfer(sourceSprite, destinationSprite);
			appended = true;
		}
	}

	if (!appended && !freeSource)
		spriteLib()->spriteHide(sourceSprite, true);

	spriteLib()->spriteSwapOrder(sourceSprite, destinationSprite, true, false);

	if (freeSource)
		destroySourceSpriteAfterTransfer(sourceInfo, sourceId);

	return true;
}

void TopgunEngine_v1_2::hideSourceSpriteAndAddFrameRefs(const SpriteInfo_v1_2 &info, uint16 sourceId) {
	uint16 sourceIndex = operandValue(sourceId);
	if (!isRecordIndexValid(sourceIndex, "hideSourceSpriteAndAddFrameRefs"))
		return;

	spriteLib()->spriteEnable(_records[sourceIndex].handle, 0);

	for (uint16 i = 0; i < info.frameCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		if (!info.frames[i]) {
			warning("TopgunEngine_v1_2::hideSourceSpriteAndAddFrameRefs(): the frame %d is not loaded", i);
			continue;
		}

		uint16 frameIndex = operandValue(READ_LE_UINT16(info.frames[i] + ((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID)));
		if (isRecordIndexValid(frameIndex, "hideSourceSpriteAndAddFrameRefs"))
			_records[frameIndex].refCount++;
	}

	const byte *data = _records[sourceIndex].data;
	if (!data)
		return;

	uint16 memberCountOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_SPRITE_DATA_MEMBER_COUNT : TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT;
	uint16 membersOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_SPRITE_DATA_MEMBERS : TG_HOST_V1_2_SPRITE_DATA_MEMBERS;
	uint16 count = READ_LE_UINT16(data + memberCountOffset);
	bool deferBitmaps = (_version >= kVersionV1_2_1 && data[TG_HOST_V1_2_1_SPRITE_DATA_DEFER_FLAG] != 0);

	for (uint16 i = 0; i < count; i++) {
		uint16 member = READ_LE_UINT16(data + membersOffset + 2 * i);
		uint16 type = resourceLib()->resGetType(_file, operandValue(member));
		bool referenceCounted;

		if (_version >= kVersionV1_2_1)
			referenceCounted = (type == kResV1_2_1Bitmap || type == kResV1_2_1Movie || type == kResV1_2_1Text || type == kResV1_2_1BitmapGroup);
		else
			referenceCounted = (type == kResV1_2Bitmap || type == kResV1_2Movie || type == kResV1_2Text);

		if (!referenceCounted) {
			destroyResourceObjectByType(member, type);
			continue;
		}

		bool release = true;

		if (deferBitmaps) {
			for (uint16 j = 0; j < i && release; j++) {
				if (READ_LE_UINT16(data + membersOffset + 2 * j) == member)
					release = false;
			}
		}

		if (release)
			decrementResourceRefCountAndMaybeDestroy(member);
	}
}

void TopgunEngine_v1_2::destroySourceSpriteAfterTransfer(const SpriteInfo_v1_2 &info, uint16 sourceId) {
	uint16 sourceIndex = operandValue(sourceId);
	if (!isRecordIndexValid(sourceIndex, "destroySourceSpriteAfterTransfer"))
		return;

	uint16 sprite = _records[sourceIndex].handle;

	if (_version >= kVersionV1_2_1 && sprite == _sceneSprite)
		_sceneSprite = 0;

	spriteLib()->spriteDestroy(sprite, true, false);
	_records[sourceIndex].object = 0;
	_records[sourceIndex].handle = 0;

	for (uint16 i = 0; i < info.frameCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		if (!info.frames[i]) {
			if (_version >= kVersionV1_2_1)
				warning("TopgunEngine_v1_2::destroySourceSpriteAfterTransfer(): the frame %d is not loaded", i);

			continue;
		}

		decrementResourceRefCountAndMaybeDestroy(READ_LE_UINT16(info.frames[i] + ((_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID)));
	}
}

void TopgunEngine_v1_2::decrementResourceRefCountAndMaybeDestroy(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "decrementResourceRefCountAndMaybeDestroy"))
		return;

	if (_version >= kVersionV1_2_1) {
		ResourceRecord &record = _records[index];

		if (record.data && record.refCount != 0) {
			record.refCount--;

			if (record.refCount == 0)
				destroyResourceObjectByType(id, resourceLib()->resGetType(_file, index));
		}

		return;
	}

	if (_records[index].refCount != 0)
		_records[index].refCount--;

	if (_records[index].refCount == 0)
		destroyResourceObjectByType(id, resourceLib()->resGetType(_file, index));
}

bool TopgunEngine_v1_2::appendQueueBitmapsToSprite(const SpriteInfo_v1_2 &info, uint16 destinationId) {
	trimSpriteQueueBitmaps(destinationId);

	if (info.frameCount == 0)
		return true;

	uint16 index = operandValue(destinationId);
	if (!isRecordIndexValid(index, "appendQueueBitmapsToSprite"))
		return false;

	ResourceRecord &record = _records[index];
	byte *data = record.data;
	if (!data)
		return false;

	uint16 memberCountOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_SPRITE_DATA_MEMBER_COUNT : TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT;
	uint16 membersOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_SPRITE_DATA_MEMBERS : TG_HOST_V1_2_SPRITE_DATA_MEMBERS;
	uint16 bitmapResourceIdOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID;
	uint16 movieType = (_version >= kVersionV1_2_1) ? kResV1_2_1Movie : kResV1_2Movie;

	uint16 oldCount = READ_LE_UINT16(data + memberCountOffset);
	uint16 newCount = (uint16)(oldCount + info.frameCount);

	if (membersOffset + 2 * (uint32)newCount > resourceLib()->resGetSize(_file, index)) {
		warning("TopgunEngine_v1_2::appendQueueBitmapsToSprite(): the member list of resource %d is too small for %d frames", index, info.frameCount);
		return false;
	}

	byte *slot = data + membersOffset + 2 * oldCount;
	uint16 sprite = record.handle;

	for (uint16 i = 0; i < info.frameCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		byte *frame = info.frames[i];
		if (!frame) {
			warning("TopgunEngine_v1_2::appendQueueBitmapsToSprite(): the frame %d is not loaded", i);
			return false;
		}

		uint16 id = READ_LE_UINT16(frame + bitmapResourceIdOffset);
		WRITE_LE_UINT16(slot, id);

		uint16 frameIndex = operandValue(id);
		if (isRecordIndexValid(frameIndex, "appendQueueBitmapsToSprite")) {
			_records[frameIndex].refCount++;

			if (resourceLib()->resGetType(_file, frameIndex) == movieType)
				stopMovieByHandle(_records[frameIndex].object);
		}

		if (sprite == _sceneSprite) {
			if (!spriteLib()->spriteAppendBitmap(sprite, CelReference(frame, id)))
				return false;
		}

		slot += 2;
	}

	WRITE_LE_UINT16(data + memberCountOffset, newCount);

	if (_sceneSprite != sprite)
		spriteLib()->spriteCallback(sprite, true);

	return true;
}

void TopgunEngine_v1_2::trimSpriteQueueBitmaps(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "trimSpriteQueueBitmaps"))
		return;

	byte *data = _records[index].data;
	if (!data)
		return;

	uint16 memberCountOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_SPRITE_DATA_MEMBER_COUNT : TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT;
	uint16 membersOffset = (_version >= kVersionV1_2_1) ? TG_HOST_V1_2_1_SPRITE_DATA_MEMBERS : TG_HOST_V1_2_SPRITE_DATA_MEMBERS;
	uint16 capacity;

	if (_version >= kVersionV1_2_1)
		capacity = READ_LE_UINT16(data + TG_HOST_V1_2_1_SPRITE_DATA_ORIGINAL_COUNT);
	else
		capacity = (uint16)((resourceLib()->resGetSize(_file, index) - TG_HOST_V1_2_SPRITE_DATA_MEMBER_BASE) >> 1);

	uint16 count = READ_LE_UINT16(data + memberCountOffset);

	if (count > capacity) {
		for (uint16 i = capacity; i < count; i++)
			decrementResourceRefCountAndMaybeDestroy(READ_LE_UINT16(data + membersOffset + 2 * i));

		WRITE_LE_UINT16(data + memberCountOffset, capacity);
	}
}

bool TopgunEngine_v1_2::o_transferSprite(const byte *record) {
	uint16 sourceId = READ_LE_UINT16(record + 2);
	uint16 destinationId = READ_LE_UINT16(record + 4);
	uint16 dataId = READ_LE_UINT16(record + 6);
	uint16 script = READ_LE_UINT16(record + 8);
	bool freeSource = (record[0x0A] != 0);
	bool flag = (record[0x0B] != 0);

	if (_version >= kVersionV1_2_1) {
		TopgunEngine_v1_2_1 *newer = static_cast<TopgunEngine_v1_2_1 *>(this);

		if (!newer->validateResourceType(sourceId, kResV1_2_1Sprite))
			return false;

		if (!newer->validateResourceType(destinationId, kResV1_2_1Sprite))
			return false;
	}

	uint16 sourceIndex = operandValue(sourceId);
	if (!isRecordIndexValid(sourceIndex, "o_transferSprite"))
		return true;

	if (_records[sourceIndex].handle == 0)
		return true;

	bool transferred = true;

	if (_version < kVersionV1_2_1 || operandValue(destinationId) != sourceIndex) {
		transferred = transferSpriteContents(sourceId, destinationId, freeSource);

		if (freeSource && _records[sourceIndex].handle != 0)
			destroyResourceObjectByType(sourceId, (_version >= kVersionV1_2_1) ? kResV1_2_1Sprite : kResV1_2Sprite);
	}

	if (!transferred)
		return false;

	if (!o_setSpriteQueue(destinationId, dataId, flag))
		return false;

	if (script != 0 && !runScriptResource(script))
		return false;

	return true;
}

} // End of namespace Topgun
