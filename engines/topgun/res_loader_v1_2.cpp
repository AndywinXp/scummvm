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

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

void TopgunEngine_v1_2::freeBitmapBlobIfUnreferenced(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "freeBitmapBlobIfUnreferenced"))
		return;

	ResourceRecord &record = _records[index];

	if (record.data && record.refCount == 0) {
		if (id == _backgroundId) {
			_backgroundBitmap = nullptr;
			spriteLib()->spriteSetBackground(nullptr, true, 0, nullptr);
		}

		utilsLibrary()->tgFree(record.data);
		record.data = nullptr;
	}
}

bool TopgunEngine_v1_2::destroyGroupResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyGroupResourceObject"))
		return false;

	ResourceRecord &record = _records[index];

	if (!record.data) {
		if (!loadOrGetResourceData(id))
			return false;
	}

	const byte *list = record.data;
	uint16 count = (uint16)(record.size >> 1);

	for (uint16 i = 0; i < count; i++) {
		uint16 member = READ_LE_UINT16(list + 2 * i);
		destroyResourceObjectByType(member, resourceLib()->resGetType(_file, operandValue(member)));
	}

	record.data = nullptr;
	return true;
}

bool TopgunEngine_v1_2::destroySpriteResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroySpriteResourceObject"))
		return true;

	if (_dragSpriteId == id)
		_dragSpriteId = 0;

	ResourceRecord &record = _records[index];
	uint16 sprite = record.handle;

	if (_freeingAll && sprite != 0 && _sceneSprite == sprite)
		return releaseSceneSpriteRecord(id);

	if (sprite != 0)
		spriteLib()->spriteDestroy(sprite, !_freeingAll, false);

	record.handle = 0;

	if (record.object != 0) {
		const byte *data = record.data;
		record.object = 0;

		uint16 count = READ_LE_UINT16(data + TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT);

		for (uint16 i = 0; i < count; i++) {
			uint16 member = READ_LE_UINT16(data + TG_HOST_V1_2_SPRITE_DATA_MEMBERS + 2 * i);
			uint16 memberIndex = operandValue(member);
			if (!isRecordIndexValid(memberIndex, "destroySpriteResourceObject"))
				continue;

			uint16 type = resourceLib()->resGetType(_file, memberIndex);

			if (type == kResV1_2Text)
				unregisterTextObjectSpriteUse(_records[memberIndex].object, sprite);

			if ((type == kResV1_2Bitmap || type == kResV1_2Movie || type == kResV1_2Text) && _records[memberIndex].refCount != 0)
				_records[memberIndex].refCount--;

			destroyResourceObjectByType(member, type);
		}

		if (_sceneSprite == sprite)
			_sceneSprite = 0;
	}

	return true;
}

bool TopgunEngine_v1_2::releaseSceneSpriteRecord(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "releaseSceneSpriteRecord"))
		return true;

	_records[index].handle = 0;
	_records[index].object = 0;
	return true;
}

void TopgunEngine_v1_2::destroyWaveResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyWaveResourceObject"))
		return;

	ResourceRecord &record = _records[index];

	if (_freeingAll && record.handle == _sceneSpriteWave)
		return;

	if (record.handle != 0)
		audioLib()->audioDestroyWave(record.handle);

	if (record.handle == _sceneSpriteWave)
		_sceneSpriteWave = 0;

	record.handle = 0;
}

bool TopgunEngine_v1_2::ensureBitmapResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureBitmapResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.data)
		return true;

	byte *bitmap = loadResourceWithProcessingPause(id, nullptr);
	if (!bitmap)
		return false;

	record.data = bitmap;
	record.refCount = 0;
	return true;
}

bool TopgunEngine_v1_2::loadOrGetResourceData(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "loadOrGetResourceData"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.data)
		return true;

	uint32 size = 0;
	byte *block = loadResourceWithProcessingPause(id, &size);
	if (!block)
		return false;

	record.data = block;
	record.size = size;
	return true;
}

bool TopgunEngine_v1_2::ensureGroupResourceLoaded(uint16 id) {
	uint32 size = 0;
	byte *list = loadResourceWithProcessingPause(id, &size);

	uint16 count = (uint16)(size >> 1);
	uint16 loaded = 0;

	if (list) {
		while (loaded < count) {
			uint16 member = READ_LE_UINT16(list + 2 * loaded);
			if (!validateAndGetResourceByType(member, resourceLib()->resGetType(_file, operandValue(member))))
				break;

			loaded++;
		}
	}

	return count == loaded;
}

bool TopgunEngine_v1_2::loadTransPaletteResource(uint16 id) {
	destroySceneMainObjects(_releaseOnPalette);

	if (!loadOrGetResourceData(id))
		return false;

	const ResourceRecord &record = _records[operandValue(id)];
	spriteLib()->spriteChangePalette(TG_HOST_V1_2_TRANS_PALETTE_FIRST, (uint16)(record.size >> 2), record.data, true);
	return true;
}

bool TopgunEngine_v1_2::ensureSpriteResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureSpriteResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.handle != 0)
		return true;

	ensureResourceDataLoaded(id);

	byte *data = record.data;
	if (!data) {
		warning("TopgunEngine_v1_2::ensureSpriteResourceLoaded(): resource %d has no data", index);
		return false;
	}

	uint16 sprite = spriteLib()->spriteCreate((int16)READ_LE_UINT16(data + TG_HOST_V1_2_SPRITE_DATA_LEVEL), CelReference());
	if (sprite == 0)
		return false;

	spriteLib()->spriteSetId(sprite, (uint16)(index - TG_HOST_V1_2_LITERAL_BIAS));

	if (data[TG_HOST_V1_2_SPRITE_DATA_PICK_FLAG] != 0)
		spriteLib()->spriteSetPickMode(sprite, 1);

	bool deferBitmaps = (data[TG_HOST_V1_2_SPRITE_DATA_DEFER_FLAG] != 0 && _deferBitmapLoading);

	if (!attachResourceListToSprite(sprite, data + TG_HOST_V1_2_SPRITE_DATA_MEMBERS, READ_LE_UINT16(data + TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT), deferBitmaps)) {
		destroySpriteResourceObject(id);
		return false;
	}

	record.handle = sprite;
	record.object = 1;
	spriteLib()->spriteEnable(sprite, 1);
	return true;
}

bool TopgunEngine_v1_2::attachResourceListToSprite(uint16 spriteHandle, const byte *idList, uint16 count, bool deferBitmaps) {
	CelReference frames[TG_HOST_V1_2_MAX_FRAME_BATCH];
	uint16 numFrames = 0;
	uint16 waveHandle = 0;
	uint16 index = 0;

	while (index < count) {
		uint16 member = READ_LE_UINT16(idList + 2 * index);
		uint16 memberIndex = operandValue(member);
		uint16 type = resourceLib()->resGetType(_file, memberIndex);

		if (!(deferBitmaps && type == kResV1_2Bitmap)) {
			if (!validateAndGetResourceByType(member, type))
				break;
		}

		if (!isRecordIndexValid(memberIndex, "attachResourceListToSprite"))
			break;

		ResourceRecord &record = _records[memberIndex];

		if (type == kResV1_2TransPalette) {
			_sceneSprite = spriteHandle;
		} else if (type == kResV1_2Bitmap || type == kResV1_2Text) {
			if (!record.data) {
				frames[numFrames] = CelReference(nullptr, member);
			} else {
				frames[numFrames] = CelReference(record.data, READ_LE_UINT16(record.data + TG_HOST_V1_2_BITMAP_RESOURCE_ID));
				record.refCount++;
			}

			numFrames++;

			if (numFrames >= TG_HOST_V1_2_MAX_FRAME_BATCH) {
				if (!spriteLib()->spriteAppendBitmaps(spriteHandle, frames, numFrames))
					break;

				numFrames = 0;
			}

			if (type == kResV1_2Text) {
				if (!registerTextObjectSpriteUse(record.object, spriteHandle))
					break;
			}
		} else if (type == kResV1_2Movie) {
			record.refCount++;
		} else if (type == kResV1_2Wave) {
			waveHandle = record.handle;
		}

		index++;
	}

	if (index < count)
		return false;

	if (numFrames != 0) {
		if (!spriteLib()->spriteAppendBitmaps(spriteHandle, frames, numFrames))
			return false;
	}

	if (spriteHandle == _sceneSprite)
		_sceneSpriteWave = waveHandle;

	return true;
}

bool TopgunEngine_v1_2::ensureWaveResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureWaveResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.handle == 0) {
		if (_cdAudioPlaying)
			_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

		record.handle = resourceLib()->resCreateWaveEvent(_file, index);
	}

	return true;
}

bool TopgunEngine_v1_2::destroyResourceObjectByType(uint16 id, uint16 type) {
	bool result = true;

	switch (type) {
	case kResV1_2Bitmap:
		freeBitmapBlobIfUnreferenced(id);
		break;
	case kResV1_2Wave:
		destroyWaveResourceObject(id);
		break;
	case kResV1_2Movie:
		destroyMovieResourceObject(id);
		break;
	case kResV1_2Text:
		destroyTextObjectResource(id);
		break;
	case kResV1_2Group:
		result = destroyGroupResourceObject(id);
		break;
	case kResV1_2Sprite:
		result = destroySpriteResourceObject(id);
		break;
	default:
		break;
	}

	return result;
}

void TopgunEngine_v1_2::resetResourcesAndTransientState() {
	_freeingAll = true;

	for (uint16 index = 0; index < _numResources; index++)
		destroyResourceObjectByType((uint16)(index - TG_HOST_V1_2_LITERAL_BIAS), resourceLib()->resGetType(_file, index));

	_freeingAll = false;
	_backgroundId = 0;
	_numClickRegions = 0;
	_numScheduledTimers = 0;
	_numResources = 0;

	delete[] _records;
	_records = nullptr;

	if (_resourceNamesTable)
		_resourceNamesTable = utilsLibrary()->tgFree(_resourceNamesTable);

	if (_resourceNameOffsets)
		_resourceNameOffsets = utilsLibrary()->tgFree(_resourceNameOffsets);
}

void TopgunEngine_v1_2::destroySceneMainObjects(bool release) {
	if (release && _sceneSprite != 0)
		spriteLib()->spriteDestroy(_sceneSprite, true, true);

	if (release && _sceneSpriteWave != 0)
		audioLib()->audioDestroyWave(_sceneSpriteWave);

	_sceneSprite = 0;
	_sceneSpriteWave = 0;
	_releaseOnPalette = false;
}

bool TopgunEngine_v1_2::validateAndGetResourceByType(uint16 id, uint16 type) {
	if (resourceLib()->resGetType(_file, operandValue(id)) != type) {
		fatalError("Invalid resource.");
		return false;
	}

	switch (type) {
	case kResV1_2Bitmap:
		return ensureBitmapResourceLoaded(id);
	case kResV1_2Wave:
		return ensureWaveResourceLoaded(id);
	case kResV1_2SpriteQueue:
	case kResV1_2OpList:
	case kResV1_2Script:
		return loadOrGetResourceData(id);
	case kResV1_2Movie:
		return ensureMovieResourceLoaded(id);
	case kResV1_2Text:
		return ensureTextObjectResourceLoaded(id);
	case kResV1_2Group:
		return ensureGroupResourceLoaded(id);
	case kResV1_2TransPalette:
		return loadTransPaletteResource(id);
	case kResV1_2Sprite:
		return ensureSpriteResourceLoaded(id);
	default:
		return false;
	}
}

bool TopgunEngine_v1_2::isResourceObjectLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "isResourceObjectLoaded"))
		return false;

	const ResourceRecord &record = _records[index];

	switch (resourceLib()->resGetType(_file, index)) {
	case kResV1_2Bitmap:
	case kResV1_2Movie:
	case kResV1_2SpriteQueue:
	case kResV1_2OpList:
	case kResV1_2Script:
		return record.data;
	case kResV1_2Wave:
	case kResV1_2Sprite:
		return record.handle != 0;
	case kResV1_2Text:
		return record.object != 0;
	default:
		return false;
	}
}

byte *TopgunEngine_v1_2::loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) {
	uint16 index = operandValue(id);
	uint16 type = resourceLib()->resGetType(_file, index);

	if (type >= kResV1_2SpriteQueue && type <= kResV1_2Sprite)
		return resourceLib()->resLoadResource(_file, index, sizeOut);

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	spriteLib()->spriteSetSendMsg(0);
	resourceLoadProgressCallback();

	bool previousCallback = resourceLib()->resSetCallback(_file, true, TG_HOST_PROGRESS_INTERVAL);
	bool savedLoading = _loading;
	_loading = true;
	uint16 savedId = _loadingResourceId;
	_loadingResourceId = id;

	byte *block = resourceLib()->resLoadResource(_file, index, sizeOut);

	spriteLib()->spriteSetSendMsg(1);
	_loading = savedLoading;
	_loadingResourceId = savedId;
	resourceLib()->resSetCallback(_file, previousCallback, TG_HOST_PROGRESS_INTERVAL);
	return block;
}

bool TopgunEngine_v1_2::ensureResourceDataLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureResourceDataLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.data)
		return true;

	uint32 size = 0;
	byte *data = loadResourceWithProcessingPause(id, &size);
	if (!data) {
		warning("TopgunEngine_v1_2::ensureResourceDataLoaded(): resource %d cannot be loaded", index);
		return false;
	}

	record.data = data;

	WRITE_LE_UINT16(data + TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT, (uint16)((size - TG_HOST_V1_2_SPRITE_DATA_MEMBER_BASE) >> 1));
	return true;
}

} // End of namespace Topgun
