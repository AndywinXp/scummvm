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
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

#define TG_LOADER_MAX_FRAME_BATCH       50
#define TG_LOADER_NUM_SYSTEM_COLORS     10
#define TG_LOADER_NUM_FILE_COLORS       200
#define TG_LOADER_LIBRARY_COLORS_FIRST  (TG_LOADER_NUM_SYSTEM_COLORS + TG_LOADER_NUM_FILE_COLORS)
#define TG_LOADER_NUM_LIBRARY_COLORS    36

namespace Topgun {

void TopgunEngine_v1_0::freeBitmapBlobIfUnreferenced(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.data && record.refCount == 0) {
		if (_backgroundId == id) {
			_buffer2 = record.data;
			_backgroundId = 0;
		} else {
			utilsLibrary()->tgFree(record.data);
		}

		record.data = nullptr;
	}
}

void TopgunEngine_v1_0::destroyDataResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.data)
		utilsLibrary()->tgFree(record.data);

	record.data = nullptr;
}

bool TopgunEngine_v1_0::destroyGroupResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	if (!record.data) {
		if (!loadOrGetResourceData(id))
			return false;
	}

	byte *list = record.data;
	uint16 count = (uint16)(record.size >> 1);

	for (uint16 i = 0; i < count; i++) {
		uint16 member = READ_LE_UINT16(list + 2 * i);
		destroyResourceObjectByType(member, resourceLibrary()->resGetType(_file, member));
	}

	if (count != 0)
		warning("TopgunEngine_v1_0::destroyGroupResourceObject(): the group list is freed through its original pointer");

	utilsLibrary()->tgFree(list);
	record.data = nullptr;
	return true;
}

bool TopgunEngine_v1_0::destroySpriteResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	uint16 sprite = record.handle;
	if (_freeingAll && _sceneSprite == sprite)
		return true;

	if (sprite != 0)
		spriteLibrary()->spriteDestroy(sprite, !_freeingAll, false);

	record.handle = 0;

	if (!record.idList)
		return true;

	byte *list = record.idList;
	record.idList = nullptr;

	uint16 count = record.numMembers;
	for (uint16 i = 0; i < count; i++) {
		uint16 member = READ_LE_UINT16(list + 2 * i);
		uint16 type = resourceLibrary()->resGetType(_file, member);

		if (type == kResourceBitmap && _records[member].refCount != 0)
			_records[member].refCount--;

		destroyResourceObjectByType(member, type);
	}

	if (_sceneSprite == sprite)
		_sceneSprite = 0;

	utilsLibrary()->tgFree(list);
	return true;
}

void TopgunEngine_v1_0::destroyWaveResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	if (_freeingAll && record.handle == _sceneSpriteWave)
		return;

	if (record.handle != 0)
		audioLibrary()->audioCommand(kDestroySoundCommand, record.handle, 0, nullptr);

	if (record.handle == _sceneSpriteWave)
		_sceneSpriteWave = 0;

	record.handle = 0;
}

bool TopgunEngine_v1_0::ensureBitmapResourceLoaded(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.data)
		return true;

	byte *bitmap = loadResourceWithProcessingPause(id, nullptr);
	if (!bitmap)
		return false;

	record.data = bitmap;
	record.refCount = 0;
	return true;
}

bool TopgunEngine_v1_0::loadOrGetResourceData(uint16 id) {
	ResourceRecord &record = _records[id];

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

bool TopgunEngine_v1_0::ensureGroupResourceLoaded(uint16 id) {
	uint32 size = 0;
	byte *list = loadResourceWithProcessingPause(id, &size);
	if (!list)
		return false;

	uint16 count = (uint16)(size >> 1);
	uint16 loaded = 0;

	while (loaded < count) {
		uint16 member = READ_LE_UINT16(list + 2 * loaded);
		if (!validateAndGetResourceByType(member, resourceLibrary()->resGetType(_file, member)))
			break;

		loaded++;
	}

	if (loaded != 0)
		warning("TopgunEngine_v1_0::ensureGroupResourceLoaded(): the group list is freed through its original pointer");

	utilsLibrary()->tgFree(list);
	return loaded == count;
}

bool TopgunEngine_v1_0::loadTransPaletteResource(uint16 id) {
	if (!loadOrGetResourceData(id))
		return false;

	ResourceRecord &record = _records[id];
	spriteLibrary()->spriteChangePalette(TG_LOADER_LIBRARY_COLORS_FIRST, (uint16)(record.size >> 2), record.data);
	return true;
}

byte *TopgunEngine_v1_0::loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) {
	uint16 type = resourceLibrary()->resGetType(_file, id);
	if (type == kResourceScript)
		return resourceLibrary()->resLoadResource(_file, id, sizeOut);

	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	resourceLoadProgressCallback();

	bool previousCallback = resourceLibrary()->resSetCallback(_file, true, TG_HOST_PROGRESS_INTERVAL);
	bool savedLoading = _loading;
	_loading = true;
	uint16 savedId = _loadingResourceId;
	_loadingResourceId = id;

	byte *block = resourceLibrary()->resLoadResource(_file, id, sizeOut);

	_loading = savedLoading;
	_loadingResourceId = savedId;
	resourceLibrary()->resSetCallback(_file, previousCallback, TG_HOST_PROGRESS_INTERVAL);
	return block;
}

bool TopgunEngine_v1_0::ensureSpriteResourceLoaded(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.handle != 0)
		return true;

	uint32 size = 0;
	byte *list = loadResourceWithProcessingPause(id, &size);
	if (!list)
		return false;

	uint16 sprite = spriteLibrary()->spriteCreate(nullptr);
	if (sprite == 0) {
		if (!utilsLibrary()->tgFree(list))
			return false;

		return true;
	}

	spriteLibrary()->spriteSetId(sprite, id);

	record.idList = list;
	record.handle = sprite;

	uint16 count = (uint16)(size >> 1);
	record.numMembers = count;

	bool attached = attachResourceListToSprite(sprite, list, count);
	if (!attached)
		destroySpriteResourceObject(id);
	else
		spriteLibrary()->spriteEnable(sprite, 1);

	return attached;
}

bool TopgunEngine_v1_0::attachResourceListToSprite(uint16 spriteHandle, const byte *idList, uint16 count) {
	byte *frames[TG_LOADER_MAX_FRAME_BATCH];
	uint16 numFrames = 0;
	uint16 waveHandle = 0;
	uint16 index = 0;

	while (index < count) {
		uint16 member = READ_LE_UINT16(idList + 2 * index);
		uint16 type = resourceLibrary()->resGetType(_file, member);

		if (!validateAndGetResourceByType(member, type))
			break;

		if (type == kResourceTransPalette) {
			_sceneSprite = spriteHandle;
		} else if (type == kResourceBitmap) {
			frames[numFrames] = _records[member].data;
			_records[member].refCount++;
			numFrames++;

			if (numFrames >= TG_LOADER_MAX_FRAME_BATCH) {
				if (!spriteLibrary()->spriteAppendBitmaps(spriteHandle, frames, numFrames))
					break;

				numFrames = 0;
			}
		} else if (type == kResourceWave) {
			waveHandle = _records[member].handle;
		}

		index++;
	}

	if (index < count)
		return false;

	if (numFrames != 0) {
		if (!spriteLibrary()->spriteAppendBitmaps(spriteHandle, frames, numFrames))
			return false;
	}

	if (spriteHandle == _sceneSprite)
		_sceneSpriteWave = waveHandle;

	return true;
}

bool TopgunEngine_v1_0::ensureWaveResourceLoaded(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.handle != 0)
		return true;

	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	record.handle = resourceLibrary()->resCreateWaveEvent(_file, id);
	return record.handle != 0;
}

bool TopgunEngine_v1_0::destroyResourceObjectByType(uint16 id, uint16 type) {
	if (!_records || id >= _numResources) {
		warning("TopgunEngine_v1_0::destroyResourceObjectByType(): resource id %d is out of range", id);
		return true;
	}

	bool result = true;

	switch (type) {
	case kResourceBitmap:
		freeBitmapBlobIfUnreferenced(id);
		break;
	case kResourceBlob:
	case kResourceTransPalette:
		destroyDataResourceObject(id);
		break;
	case kResourceWave:
		destroyWaveResourceObject(id);
		break;
	case kResourceGroup:
		result = destroyGroupResourceObject(id);
		break;
	case kResourceSprite:
		result = destroySpriteResourceObject(id);
		break;
	default:
		break;
	}

	return result;
}

void TopgunEngine_v1_0::resetResourcesAndTransientState() {
	_freeingAll = true;

	for (uint16 id = 0; id < _numResources; id++)
		destroyResourceObjectByType(id, resourceLibrary()->resGetType(_file, id));

	_freeingAll = false;
	_backgroundId = 0;
	_numHotspots = 0;
	_numTimers = 0;

	for (uint16 id = 0; id < _numResources; id++) {
		utilsLibrary()->tgFree(_records[id].idList);
		_records[id].idList = nullptr;
	}

	_numResources = 0;

	delete[] _records;
	_records = nullptr;

	if (_resourceNamesTable)
		_resourceNamesTable = utilsLibrary()->tgFree(_resourceNamesTable);
}

void TopgunEngine_v1_0::destroySceneMainObjects() {
	if (_sceneSprite != 0)
		_sceneSprite = spriteLibrary()->spriteDestroy(_sceneSprite, true, true);

	if (_sceneSpriteWave != 0)
		audioLibrary()->audioCommand(kDestroySoundCommand, _sceneSpriteWave, 0, nullptr);

	_sceneSpriteWave = 0;
}

bool TopgunEngine_v1_0::validateAndGetResourceByType(uint16 id, uint16 type) {
	if (!_records || id >= _numResources) {
		warning("TopgunEngine_v1_0::validateAndGetResourceByType(): resource id %d is out of range", id);
		return false;
	}

	switch (type) {
	case kResourceBitmap:
		return ensureBitmapResourceLoaded(id);
	case kResourceBlob:
	case kResourceScript:
		return loadOrGetResourceData(id);
	case kResourceWave:
		return ensureWaveResourceLoaded(id);
	case kResourceGroup:
		return ensureGroupResourceLoaded(id);
	case kResourceSprite:
		return ensureSpriteResourceLoaded(id);
	case kResourceTransPalette:
		return loadTransPaletteResource(id);
	default:
		return false;
	}
}

bool TopgunEngine_v1_0::isResourceObjectLoaded(uint16 id) {
	uint16 type = resourceLibrary()->resGetType(_file, id);

	if (type == kResourceBitmap || type == kResourceBlob || type == kResourceScript)
		return _records[id].data;

	if (type == kResourceWave || type == kResourceSprite)
		return _records[id].handle != 0;

	return false;
}

void TopgunEngine_v1_0::decrementResourceRefCountAndMaybeDestroy(uint16 bitmapId) {
	if (!_records || bitmapId >= _numResources) {
		warning("TopgunEngine_v1_0::decrementResourceRefCountAndMaybeDestroy(): resource id %d is out of range", bitmapId);
		return;
	}

	if (_records[bitmapId].refCount != 0)
		_records[bitmapId].refCount--;

	if (_records[bitmapId].refCount == 0)
		destroyResourceObjectByType(bitmapId, kResourceBitmap);
}

void TopgunEngine_v1_0::refreshGamePalette(byte *buffer) {
	if (display()->hasPalette())
		display()->getSystemPaletteEntries(buffer, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);
	else
		memset(buffer, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE);

	resourceLibrary()->resLoadPalette(_file, buffer + TG_LOADER_NUM_SYSTEM_COLORS * TG_DISPLAY_PALETTE_ENTRY_SIZE);
	spriteLibrary()->spriteGetPalette(buffer + TG_LOADER_LIBRARY_COLORS_FIRST * TG_DISPLAY_PALETTE_ENTRY_SIZE, TG_LOADER_LIBRARY_COLORS_FIRST, TG_LOADER_NUM_LIBRARY_COLORS);
	spriteLibrary()->spriteSetPalette(buffer);
	spriteLibrary()->spriteFadePalette(0, 0, TG_LOADER_NUM_SYSTEM_COLORS, TG_LOADER_NUM_FILE_COLORS);
	spriteLibrary()->spriteRealizePalette();
}

bool TopgunEngine_v1_0::openResourceFileAndInit(const Common::String &name) {
	_file = resourceLibrary()->resOpenFile(name);
	if (_file == 0)
		return false;

	_numResources = resourceLibrary()->resCountResources(_file);
	_records = new ResourceRecord[_numResources];
	_numTimers = 0;
	return true;
}

void TopgunEngine_v1_2::freeBitmapBlobIfUnreferenced(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "freeBitmapBlobIfUnreferenced"))
		return;

	ResourceRecord &record = _records[index];

	if (record.data && record.refCount == 0) {
		if (_version < kVersionV1_2_2_2 && id == _backgroundId) {
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
	if (_dragSpriteId == id)
		_dragSpriteId = 0;

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroySpriteResourceObject"))
		return true;

	ResourceRecord &record = _records[index];
	uint16 sprite = record.handle;

	if (_freeingAll && sprite != 0 && _sceneSprite == sprite)
		return releaseSceneSpriteRecord(id);

	if (sprite != 0)
		spriteLib()->spriteDestroy(sprite, !_freeingAll, false);

	record.handle = 0;

	bool hadObject = (record.object != 0);

	if (hadObject) {
		const byte *data = record.data;
		record.object = 0;

		if (!data) {
			warning("TopgunEngine_v1_2::destroySpriteResourceObject(): the sprite object has no data");
		} else {
			uint16 shift = 0;
			if (_version >= kVersionV1_2_6_1)
				shift = TG_HOST_V1_2_6_1_SPRITE_DATA_SHIFT;

			uint16 count = READ_LE_UINT16(data + shift + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_SPRITE_DATA_MEMBER_COUNT : TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT));

			uint16 membersOffset = shift + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_SPRITE_DATA_MEMBERS : TG_HOST_V1_2_SPRITE_DATA_MEMBERS);
			if (_version >= kVersionV1_2_9)
				membersOffset = TG_HOST_V1_2_9_SPRITE_DATA_MEMBERS;

			for (uint16 i = 0; i < count; i++) {
				uint16 member = READ_LE_UINT16(data + membersOffset + 2 * i);
				uint16 memberIndex = operandValue(member);
				if (!isRecordIndexValid(memberIndex, "destroySpriteResourceObject"))
					continue;

				uint16 type = resourceLib()->resGetType(_file, memberIndex);
				bool isText;
				bool isReferenceCounted;

				if (_version >= kVersionV1_2_9) {
					isText = (type == kResV1_2_9Text);
					isReferenceCounted = (type == kResV1_2_9Bitmap || type == kResV1_2_9Movie || type == kResV1_2_9Text || type == kResV1_2_9Cell);
				} else if (_version >= kVersionV1_2_5) {
					isText = (type == kResV1_2_5Text);
					isReferenceCounted = (type == kResV1_2_3Bitmap || type == kResV1_2_5Movie || type == kResV1_2_5Text || type == kResV1_2_5BitmapGroup);
				} else if (_version >= kVersionV1_2_2_2) {
					isText = (type == kResV1_2_3Text);
					isReferenceCounted = (type == kResV1_2_3Bitmap || type == kResV1_2_3Movie || type == kResV1_2_3Text || type == kResV1_2_3BitmapGroup);
				} else if (_version >= kVersionV1_2_2) {
					isText = (type == kResV1_2_2Text);
					isReferenceCounted = (type == kResV1_2_2Bitmap || type == kResV1_2_2Movie || type == kResV1_2_2Text || type == kResV1_2_2BitmapGroup);
				} else if (_version >= kVersionV1_2_1) {
					isText = (type == kResV1_2_1Text);
					isReferenceCounted = (type == kResV1_2_1Bitmap || type == kResV1_2_1Movie || type == kResV1_2_1Text || type == kResV1_2_1BitmapGroup);
				} else {
					isText = (type == kResV1_2Text);
					isReferenceCounted = (type == kResV1_2Bitmap || type == kResV1_2Movie || type == kResV1_2Text);
				}

				if (isText)
					unregisterTextObjectSpriteUse(_records[memberIndex].object, sprite);

				if (isReferenceCounted && _records[memberIndex].refCount != 0)
					_records[memberIndex].refCount--;

				destroyResourceObjectByType(member, type);
			}
		}
	}

	if (sprite == _sceneSprite && (_version >= kVersionV1_2_2_2 || hadObject))
		_sceneSprite = 0;

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
	uint16 first = (_version >= kVersionV1_2_1) ? (uint16)(TG_HOST_V1_2_1_PALETTE_LAST_COLOR - _maxTransColors) : TG_HOST_V1_2_TRANS_PALETTE_FIRST;

	spriteLib()->spriteChangePalette(first, (uint16)(record.size >> 2), record.data, true);
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

	uint16 shift = 0;
	if (_version >= kVersionV1_2_6_1)
		shift = TG_HOST_V1_2_6_1_SPRITE_DATA_SHIFT;

	if (_version >= kVersionV1_2_2_2)
		WRITE_LE_UINT16(data + shift + TG_HOST_V1_2_3_SPRITE_DATA_MEMBER_COUNT, READ_LE_UINT16(data + shift + TG_HOST_V1_2_3_SPRITE_DATA_ORIGINAL_COUNT));

	uint16 level = READ_LE_UINT16(data + shift + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_SPRITE_DATA_LEVEL : TG_HOST_V1_2_SPRITE_DATA_LEVEL));
	if (_version >= kVersionV1_2_2_2)
		level = operandValue(level);

	uint16 sprite = spriteLib()->spriteCreate((int16)level, CelReference());
	if (sprite == 0)
		return false;

	spriteLib()->spriteSetId(sprite, (uint16)(index - ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS)));

	bool deferBitmaps;

	if (_version >= kVersionV1_2_2_2) {
		if (_version >= kVersionV1_2_2_3)
			static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteSetPickMode(sprite, data[shift + TG_HOST_V1_2_3_SPRITE_DATA_WHOLE_BOX], data[shift + TG_HOST_V1_2_3_SPRITE_DATA_PICK_INVISIBLE]);
		else if (data[shift + TG_HOST_V1_2_3_SPRITE_DATA_WHOLE_BOX] != 0)
			spriteLib()->spriteSetPickMode(sprite, 1);

		if (_version >= kVersionV1_2_9)
			static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteSetScrollable(sprite, data[TG_HOST_V1_2_9_SPRITE_DATA_SCROLLABLE]);

		if (data[shift + TG_HOST_V1_2_3_SPRITE_DATA_PALETTE_FLAG] != 0) {
			static_cast<TopgunEngine_v1_2_3 *>(this)->applySpritePaletteExtension(data);
			_sceneSprite = sprite;
		}

		deferBitmaps = (data[shift + TG_HOST_V1_2_3_SPRITE_DATA_DEFER_FLAG] != 0);
	} else {
		if (data[TG_HOST_V1_2_SPRITE_DATA_PICK_FLAG] != 0)
			spriteLib()->spriteSetPickMode(sprite, 1);

		deferBitmaps = (data[TG_HOST_V1_2_SPRITE_DATA_DEFER_FLAG] != 0 && (_version >= kVersionV1_2_1 || _deferBitmapLoading));
	}

	uint16 membersOffset = shift + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_SPRITE_DATA_MEMBERS : TG_HOST_V1_2_SPRITE_DATA_MEMBERS);
	if (_version >= kVersionV1_2_9)
		membersOffset = TG_HOST_V1_2_9_SPRITE_DATA_MEMBERS;

	uint16 memberCountOffset = shift + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_SPRITE_DATA_MEMBER_COUNT : TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT);

	if (!attachResourceListToSprite(sprite, data + membersOffset, READ_LE_UINT16(data + memberCountOffset), deferBitmaps)) {
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
	uint16 midiHandle = 0;
	uint16 index = 0;

	while (index < count) {
		uint16 member = READ_LE_UINT16(idList + 2 * index);
		uint16 memberIndex = operandValue(member);
		uint16 type = resourceLib()->resGetType(_file, memberIndex);
		bool isBitmap;
		bool isText;
		bool isBitmapGroup = false;
		bool isMovie;
		bool isWave;
		bool isMidi = false;
		bool isTransPalette = false;

		if (_version >= kVersionV1_2_9) {
			isBitmap = (type == kResV1_2_9Bitmap);
			isText = (type == kResV1_2_9Text);
			isBitmapGroup = (type == kResV1_2_9Cell);
			isMovie = (type == kResV1_2_9Movie);
			isWave = (type == kResV1_2_9Wave);
			isMidi = (type == kResV1_2_9Midi);
		} else if (_version >= kVersionV1_2_5) {
			isBitmap = (type == kResV1_2_3Bitmap);
			isText = (type == kResV1_2_5Text);
			isBitmapGroup = (type == kResV1_2_5BitmapGroup);
			isMovie = (type == kResV1_2_5Movie);
			isWave = (type == kResV1_2_5Wave);
			isMidi = (type == kResV1_2_3Midi);
		} else if (_version >= kVersionV1_2_2_2) {
			isBitmap = (type == kResV1_2_3Bitmap);
			isText = (type == kResV1_2_3Text);
			isBitmapGroup = (type == kResV1_2_3BitmapGroup);
			isMovie = (type == kResV1_2_3Movie);
			isWave = (type == kResV1_2_3Wave);
			isMidi = (type == kResV1_2_3Midi);
		} else if (_version >= kVersionV1_2_2) {
			isBitmap = (type == kResV1_2_2Bitmap);
			isText = (type == kResV1_2_2Text);
			isBitmapGroup = (type == kResV1_2_2BitmapGroup);
			isMovie = (type == kResV1_2_2Movie);
			isWave = (type == kResV1_2_2Wave);
			isTransPalette = (type == kResV1_2_2Palette);
		} else if (_version >= kVersionV1_2_1) {
			isBitmap = (type == kResV1_2_1Bitmap);
			isText = (type == kResV1_2_1Text);
			isBitmapGroup = (type == kResV1_2_1BitmapGroup);
			isMovie = (type == kResV1_2_1Movie);
			isWave = (type == kResV1_2_1Wave);
			isTransPalette = (type == kResV1_2_1Palette);
		} else {
			isBitmap = (type == kResV1_2Bitmap);
			isText = (type == kResV1_2Text);
			isMovie = (type == kResV1_2Movie);
			isWave = (type == kResV1_2Wave);
			isTransPalette = (type == kResV1_2TransPalette);
		}

		if (!(deferBitmaps && (isBitmap || isBitmapGroup))) {
			if (!validateAndGetResourceByType(member, type))
				break;
		}

		if (!isRecordIndexValid(memberIndex, "attachResourceListToSprite"))
			break;

		ResourceRecord &record = _records[memberIndex];

		if (isTransPalette) {
			_sceneSprite = spriteHandle;
		} else if (isBitmap || isText || isBitmapGroup) {
			if (!record.data) {
				frames[numFrames] = CelReference(nullptr, member);
			} else {
				frames[numFrames] = CelReference(record.data, READ_LE_UINT16(record.data + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID)));
				record.refCount++;
			}

			numFrames++;

			if (numFrames >= ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_MAX_FRAME_BATCH : TG_HOST_V1_2_MAX_FRAME_BATCH)) {
				if (!spriteLib()->spriteAppendBitmaps(spriteHandle, frames, numFrames))
					break;

				numFrames = 0;
			}

			if (isText) {
				if (!registerTextObjectSpriteUse(record.object, spriteHandle))
					break;
			}
		} else if (isMovie) {
			record.refCount++;
		} else if (isWave) {
			waveHandle = record.handle;
		} else if (isMidi) {
			midiHandle = record.handle;
		}

		index++;
	}

	if (index < count)
		return false;

	if (numFrames != 0) {
		if (!spriteLib()->spriteAppendBitmaps(spriteHandle, frames, numFrames))
			return false;
	}

	if (spriteHandle == _sceneSprite) {
		_sceneSpriteWave = waveHandle;

		if (_version >= kVersionV1_2_3)
			_sceneSpriteMidi = midiHandle;
	}

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

	if (_version >= kVersionV1_2_2) {
		switch (type) {
		case kResV1_2_2Bitmap:
			freeBitmapBlobIfUnreferenced(id);
			break;
		case kResV1_2_2Midi:
			destroyMidiResourceObject(id);
			break;
		case kResV1_2_2Wave:
			destroyWaveResourceObject(id);
			break;
		case kResV1_2_2Movie:
			destroyMovieResourceObject(id);
			break;
		case kResV1_2_2BitmapGroup:
			destroyBitmapGroupResourceObject(id);
			break;
		case kResV1_2_2MixedGroup:
			result = destroyGroupResourceObject(id);
			break;
		case kResV1_2_2Sprite:
			result = destroySpriteResourceObject(id);
			break;
		case kResV1_2_2Text:
			destroyTextObjectResource(id);
			break;
		default:
			break;
		}

		return result;
	}

	if (_version >= kVersionV1_2_1) {
		switch (type) {
		case kResV1_2_1Bitmap:
			freeBitmapBlobIfUnreferenced(id);
			break;
		case kResV1_2_1Wave:
			destroyWaveResourceObject(id);
			break;
		case kResV1_2_1BitmapGroup:
			destroyBitmapGroupResourceObject(id);
			break;
		case kResV1_2_1MixedGroup:
			result = destroyGroupResourceObject(id);
			break;
		case kResV1_2_1Movie:
			destroyMovieResourceObject(id);
			break;
		case kResV1_2_1Sprite:
			result = destroySpriteResourceObject(id);
			break;
		case kResV1_2_1Text:
			destroyTextObjectResource(id);
			break;
		default:
			break;
		}

		return result;
	}

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

	for (uint16 index = 0; index < _numResources; index++) {
		destroyResourceObjectByType((uint16)(index - ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS)), resourceLib()->resGetType(_file, index));

		if (_version >= kVersionV1_2_2_2)
			static_cast<ResourceLibrary_v1_2_3 *>(resourceLib())->resDeleteResource(_file, index);
	}

	_freeingAll = false;

	if (_version >= kVersionV1_2_1) {
		_numMovies = 0;
		_numCollisionPairs = 0;
	}

	if (_version < kVersionV1_2_2_2)
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

	if (_version >= kVersionV1_2_3 && release && _sceneSpriteMidi != 0)
		static_cast<AudioLibrary_v1_2_3 *>(audioLib())->audioDestroyMidi(_sceneSpriteMidi);

	_sceneSprite = 0;
	_sceneSpriteWave = 0;
	_sceneSpriteMidi = 0;
	_releaseOnPalette = false;
}

bool TopgunEngine_v1_2::validateResourceType(uint16 id, uint16 type) {
	if (resourceLib()->resGetType(_file, operandValue(id)) != type) {
		fatalError(108, "Invalid resource.");
		return false;
	}

	return true;
}

bool TopgunEngine_v1_2::validateAndGetResourceByType(uint16 id, uint16 type) {
	if (_version >= kVersionV1_2_2) {
		if (!validateResourceType(id, type))
			return false;

		switch (type) {
		case kResV1_2_2Bitmap:
			return ensureBitmapResourceLoaded(id);
		case kResV1_2_2Midi:
			return ensureMidiResourceLoaded(id);
		case kResV1_2_2Wave:
			return ensureWaveResourceLoaded(id);
		case kResV1_2_2OpList:
		case kResV1_2_2SpriteQueueData:
		case kResV1_2_2Script:
			return loadOrGetResourceData(id);
		case kResV1_2_2BitmapGroup:
			return ensureBitmapGroupResourceLoaded(id);
		case kResV1_2_2MixedGroup:
			return ensureGroupResourceLoaded(id);
		case kResV1_2_2Movie:
			return ensureMovieResourceLoaded(id);
		case kResV1_2_2Palette:
			return loadTransPaletteResource(id);
		case kResV1_2_2Sprite:
			return ensureSpriteResourceLoaded(id);
		case kResV1_2_2Text:
			return ensureTextObjectResourceLoaded(id);
		default:
			return false;
		}
	}

	if (_version >= kVersionV1_2_1) {
		if (!validateResourceType(id, type))
			return false;

		switch (type) {
		case kResV1_2_1Bitmap:
			return ensureBitmapResourceLoaded(id);
		case kResV1_2_1Wave:
			return ensureWaveResourceLoaded(id);
		case kResV1_2_1OpList:
		case kResV1_2_1SpriteQueueData:
		case kResV1_2_1Script:
			return loadOrGetResourceData(id);
		case kResV1_2_1BitmapGroup:
			return ensureBitmapGroupResourceLoaded(id);
		case kResV1_2_1MixedGroup:
			return ensureGroupResourceLoaded(id);
		case kResV1_2_1Movie:
			return ensureMovieResourceLoaded(id);
		case kResV1_2_1Palette:
			return loadTransPaletteResource(id);
		case kResV1_2_1Sprite:
			return ensureSpriteResourceLoaded(id);
		case kResV1_2_1Text:
			return ensureTextObjectResourceLoaded(id);
		default:
			return false;
		}
	}

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

	if (_version >= kVersionV1_2_2) {
		switch (resourceLib()->resGetType(_file, index)) {
		case kResV1_2_2Bitmap:
		case kResV1_2_2Movie:
		case kResV1_2_2BitmapGroup:
		case kResV1_2_2OpList:
		case kResV1_2_2SpriteQueueData:
		case kResV1_2_2Script:
			return record.data;
		case kResV1_2_2Midi:
		case kResV1_2_2Wave:
		case kResV1_2_2Sprite:
			return record.handle != 0;
		case kResV1_2_2Text:
			return record.object != 0;
		default:
			return false;
		}
	}

	if (_version >= kVersionV1_2_1) {
		switch (resourceLib()->resGetType(_file, index)) {
		case kResV1_2_1Bitmap:
		case kResV1_2_1BitmapGroup:
		case kResV1_2_1Movie:
		case kResV1_2_1OpList:
		case kResV1_2_1SpriteQueueData:
		case kResV1_2_1Script:
			return record.data;
		case kResV1_2_1Wave:
		case kResV1_2_1Sprite:
			return record.handle != 0;
		case kResV1_2_1Text:
			return record.object != 0;
		default:
			return false;
		}
	}

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

	if (_version >= kVersionV1_2_9) {
		if (type >= kResV1_2_9Movie && type <= kResV1_2_9Tile)
			return resourceLib()->resLoadResource(_file, index, sizeOut);
	} else if (_version >= kVersionV1_2_5) {
		if (type >= kResV1_2_5Movie && type <= kResV1_2_5Text)
			return resourceLib()->resLoadResource(_file, index, sizeOut);
	} else if (_version >= kVersionV1_2_2_2) {
		if (type >= kResV1_2_3Movie && type <= kResV1_2_3Text)
			return resourceLib()->resLoadResource(_file, index, sizeOut);
	} else if (_version >= kVersionV1_2_2) {
		if (type >= kResV1_2_2OpList && type <= kResV1_2_2Text)
			return resourceLib()->resLoadResource(_file, index, sizeOut);
	} else if (_version >= kVersionV1_2_1) {
		if (type >= kResV1_2_1OpList && type <= kResV1_2_1Text)
			return resourceLib()->resLoadResource(_file, index, sizeOut);
	} else if (type >= kResV1_2SpriteQueue && type <= kResV1_2Sprite) {
		return resourceLib()->resLoadResource(_file, index, sizeOut);
	}

	if (_cdAudioPlaying && _version >= kVersionV1_2_5 && static_cast<TopgunEngine_v1_2_3 *>(this)->_debugMode) {
		fatalError(114, "Attempted load while cd was playing.");
		return nullptr;
	}

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	uint16 previousSendMessages = spriteLib()->spriteSetSendMsg(0);
	resourceLoadProgressCallback();

	bool previousCallback = resourceLib()->resSetCallback(_file, true, TG_HOST_PROGRESS_INTERVAL);
	bool savedLoading = _loading;
	_loading = true;
	uint16 savedId = _loadingResourceId;
	_loadingResourceId = id;

	byte *block = resourceLib()->resLoadResource(_file, index, sizeOut);

	spriteLib()->spriteSetSendMsg((_version >= kVersionV1_2_2_2) ? previousSendMessages : 1);
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
		if (_version < kVersionV1_2_2_2)
			warning("TopgunEngine_v1_2::ensureResourceDataLoaded(): resource %d cannot be loaded", index);

		return false;
	}

	record.data = data;

	if (_version < kVersionV1_2_2_2)
		WRITE_LE_UINT16(data + TG_HOST_V1_2_SPRITE_DATA_MEMBER_COUNT, (uint16)((size - TG_HOST_V1_2_SPRITE_DATA_MEMBER_BASE) >> 1));

	return true;
}

void TopgunEngine_v1_2::destroyBitmapGroupResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyBitmapGroupResourceObject"))
		return;

	ResourceRecord &record = _records[index];

	if (!record.data || record.refCount != 0)
		return;

	if (record.object != 0) {
		uint16 memberIndex = operandValue(record.object);
		if (isRecordIndexValid(memberIndex, "destroyBitmapGroupResourceObject"))
			_records[memberIndex].refCount--;

		freeBitmapBlobIfUnreferenced(record.object);
		record.object = 0;
	}

	utilsLibrary()->tgFree(record.data);
	record.data = nullptr;
}

void TopgunEngine_v1_2::destroyMidiResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyMidiResourceObject"))
		return;

	ResourceRecord &record = _records[index];

	if (_freeingAll && record.handle == _sceneSpriteMidi)
		return;

	if (record.handle != 0)
		audioLib()->audioDestroyMidi(record.handle);

	if (record.handle == _sceneSpriteMidi)
		_sceneSpriteMidi = 0;

	record.handle = 0;
}

bool TopgunEngine_v1_2::ensureBitmapGroupResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureBitmapGroupResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.data)
		return true;

	byte *list = loadResourceWithProcessingPause(id, nullptr);
	if (!list)
		return false;

	uint16 member = READ_LE_UINT16(list);

	if (!ensureBitmapResourceLoaded(member))
		return false;

	uint16 memberIndex = operandValue(member);
	if (!isRecordIndexValid(memberIndex, "ensureBitmapGroupResourceLoaded"))
		return false;

	ResourceRecord &memberRecord = _records[memberIndex];
	const byte *bitmap = memberRecord.data;

	// The pixels are not reachable through a pointer stored in the header, so the object is a private copy of the whole bitmap
	uint16 flagsOffset = TG_RES_V1_2_3_BITMAP_FLAGS_BYTE_OFFSET;
	if (_version >= kVersionV1_2_5)
		flagsOffset = TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET;

	uint32 size;
	if (_version >= kVersionV1_2_2_2 && (bitmap[flagsOffset] & TG_RES_V1_2_3_BITMAP_COMPRESSED_MASK) != 0)
		size = resourceLib()->resGetSize(_file, memberIndex);
	else
		size = READ_LE_UINT16(bitmap) + (uint32)READ_LE_UINT16(bitmap + TG_RES_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(bitmap + TG_RES_V1_2_BITMAP_HEIGHT_OFFSET);

	if (size < TG_HOST_V1_2_3_BITMAP_HEADER_SIZE) {
		warning("TopgunEngine_v1_2::ensureBitmapGroupResourceLoaded(): bitmap %d is smaller than its header", memberIndex);
		return false;
	}

	byte *object = utilsLibrary()->tgMalloc(size, 0);
	if (!object)
		return false;

	record.data = object;
	record.refCount = 0;
	record.object = member;
	memberRecord.refCount++;

	memcpy(object, bitmap, size);

	if (_version >= kVersionV1_2_9) {
		object[TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET] = (object[TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET] & TG_HOST_V1_2_9_BITMAP_FLAGS_KEPT_MASK) | (list[TG_HOST_V1_2_9_CELL_FLAGS] & TG_HOST_V1_2_9_CELL_FLAGS_MASK);
		WRITE_LE_UINT16(object + TG_HOST_V1_2_3_BITMAP_HOT_X, READ_LE_UINT16(list + TG_HOST_V1_2_9_CELL_X));
		WRITE_LE_UINT16(object + TG_HOST_V1_2_3_BITMAP_HOT_Y, READ_LE_UINT16(list + TG_HOST_V1_2_9_CELL_Y));
		WRITE_LE_UINT16(object + TG_HOST_V1_2_3_BITMAP_RESOURCE_ID, (uint16)(index - TG_HOST_V1_2_3_LITERAL_BIAS));
		return true;
	}

	WRITE_LE_UINT16(object + TG_HOST_V1_2_3_BITMAP_HOT_X, READ_LE_UINT16(list + TG_HOST_V1_2_3_GROUP_X));
	WRITE_LE_UINT16(object + TG_HOST_V1_2_3_BITMAP_HOT_Y, READ_LE_UINT16(list + TG_HOST_V1_2_3_GROUP_Y));
	WRITE_LE_UINT16(object + TG_HOST_V1_2_3_BITMAP_RESOURCE_ID, id);
	return true;
}

bool TopgunEngine_v1_2::ensureMidiResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureMidiResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.handle == 0) {
		if (_cdAudioPlaying)
			_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

		record.handle = resourceLib()->resCreateMidiEvent(_file, index);
	}

	return true;
}

void TopgunEngine_v1_2_3::applySpritePaletteExtension(const byte *header) {
	uint16 shift = 0;
	uint16 entriesShift = 0;

	if (_version >= kVersionV1_2_9) {
		shift = TG_HOST_V1_2_6_1_SPRITE_DATA_SHIFT;
		entriesShift = TG_HOST_V1_2_9_SPRITE_DATA_PALETTE_ENTRIES_SHIFT;
	} else if (_version >= kVersionV1_2_6_1) {
		shift = TG_HOST_V1_2_6_1_SPRITE_DATA_SHIFT;
		entriesShift = TG_HOST_V1_2_6_1_SPRITE_DATA_PALETTE_ENTRIES_SHIFT;
	}

	const byte *entries = header + entriesShift + TG_HOST_V1_2_3_SPRITE_DATA_PALETTE_ENTRIES + 2 * READ_LE_UINT16(header + shift + TG_HOST_V1_2_3_SPRITE_DATA_MEMBER_COUNT);

	destroySceneMainObjects(_releaseOnPalette);

	// The entries sit just below the static colours, after the transparent ones of the file
	spriteLib()->spriteChangePalette(TG_HOST_V1_2_3_PALETTE_EXTENSION_END - _maxTransColors, READ_LE_UINT16(header + shift + TG_HOST_V1_2_3_SPRITE_DATA_PALETTE_COUNT), entries, true, false);
}

bool TopgunEngine_v1_2_3::ensureTableResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureTableResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.data)
		return true;

	uint32 size = 0;
	byte *block = loadResourceWithProcessingPause(id, &size);
	if (!block)
		return false;

	WRITE_LE_UINT16(block + TG_HOST_V1_2_9_TABLE_RESOURCE_ID, (uint16)(index - TG_HOST_V1_2_3_LITERAL_BIAS));

	record.data = block;
	record.size = size;
	return true;
}

void TopgunEngine_v1_2_3::destroyTableResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyTableResourceObject"))
		return;

	ResourceRecord &record = _records[index];

	if (!record.data)
		return;

	if (READ_LE_UINT16(record.data + TG_RES_V1_2_9_TABLE_OWNED_OFFSET) == 0)
		return;

	utilsLibrary()->tgFree(record.data);
	record.data = nullptr;
}

bool TopgunEngine_v1_2_3::loadBitmapListToPointers(uint16 count, const byte *list, byte **cells) {
	uint16 loaded = 0;

	while (loaded < count) {
		uint16 id = READ_LE_UINT16(list + 2 * loaded);

		if (id != 0) {
			uint16 index = operandValue(id);
			if (!isRecordIndexValid(index, "loadBitmapListToPointers"))
				break;

			uint16 type = resourceLib()->resGetType(_file, index);
			if (type != kResV1_2_9Bitmap && type != kResV1_2_9Cell)
				break;

			if (!validateAndGetResourceByType(id, type))
				break;

			ResourceRecord &record = _records[index];
			record.refCount++;
			cells[loaded] = record.data;
		}

		loaded++;
	}

	return loaded >= count;
}

void TopgunEngine_v1_2_3::releaseTileResourceCells(uint16 count, const byte *list) {
	for (uint16 i = 0; i < count; i++) {
		uint16 id = READ_LE_UINT16(list + 2 * i);
		if (id == 0)
			continue;

		uint16 index = operandValue(id);
		if (!isRecordIndexValid(index, "releaseTileResourceCells"))
			continue;

		uint16 type = resourceLib()->resGetType(_file, index);
		ResourceRecord &record = _records[index];

		if (record.refCount != 0)
			record.refCount--;

		destroyResourceObjectByType(id, type);
	}
}

bool TopgunEngine_v1_2_3::ensureTileResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureTileResourceLoaded"))
		return false;

	ResourceRecord &record = _records[index];

	if (record.data)
		return true;

	uint32 size = 0;
	byte *data = loadResourceWithProcessingPause(id, &size);
	if (!data)
		return false;

	if (size < TG_HOST_V1_2_9_TILE_HEADER_SIZE) {
		warning("TopgunEngine_v1_2_3::ensureTileResourceLoaded(): resource %d is smaller than its header", index);
		utilsLibrary()->tgFree(data);
		return false;
	}

	record.data = data;

	uint16 count = (uint16)((size - TG_HOST_V1_2_9_TILE_HEADER_SIZE) >> 1);

	TileBackground *tile = new TileBackground();
	tile->cells = reinterpret_cast<byte **>(utilsLibrary()->tgMalloc(count * sizeof(byte *), 1));
	if (!tile->cells) {
		delete tile;
		destroyTileResourceObject(id);
		return false;
	}

	tile->tileWidth = READ_LE_UINT16(data + TG_HOST_V1_2_9_TILE_WIDTH);
	tile->tileHeight = READ_LE_UINT16(data + TG_HOST_V1_2_9_TILE_HEIGHT);
	tile->columns = READ_LE_UINT16(data + TG_HOST_V1_2_9_TILE_COLUMNS);
	tile->rows = READ_LE_UINT16(data + TG_HOST_V1_2_9_TILE_ROWS);
	tile->layout = READ_LE_UINT16(data + TG_HOST_V1_2_9_TILE_LAYOUT);
	tile->id = (uint16)(index - TG_HOST_V1_2_3_LITERAL_BIAS);
	tile->cellCount = count;

	_tileBackgrounds[index] = tile;

	if (!loadBitmapListToPointers(count, data + TG_HOST_V1_2_9_TILE_HEADER_SIZE, tile->cells)) {
		destroyTileResourceObject(id);
		return false;
	}

	return true;
}

void TopgunEngine_v1_2_3::destroyTileResourceObject(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyTileResourceObject"))
		return;

	ResourceRecord &record = _records[index];

	if (!record.data)
		return;

	TileBackground *tile = nullptr;
	if (_tileBackgrounds.contains(index))
		tile = _tileBackgrounds[index];

	uint16 count = 0;
	if (tile)
		count = tile->cellCount;

	if (count != 0)
		releaseTileResourceCells(count, record.data + TG_HOST_V1_2_9_TILE_HEADER_SIZE);

	if (tile == _tileMap) {
		static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteSetTileBackground(nullptr, nullptr);
		_tileMap = nullptr;
	}

	if (tile) {
		utilsLibrary()->tgFree(reinterpret_cast<byte *>(tile->cells));
		delete tile;
		_tileBackgrounds.erase(index);
	}

	// The loaded resource is released here, the original keeps it allocated after clearing the record
	utilsLibrary()->tgFree(record.data);
	record.data = nullptr;
}

bool TopgunEngine_v1_2_3::destroyResourceObjectByType(uint16 id, uint16 type) {
	bool result = true;

	// The later numbering is the earlier one shifted by two from the wave on; the two numbers it leaves free are not types
	if (_version >= kVersionV1_2_9) {
		if (type == kResV1_2_9Table) {
			destroyTableResourceObject(id);
			return true;
		}

		if (type == kResV1_2_9Tile) {
			destroyTileResourceObject(id);
			return true;
		}

		if (type == kResV1_2_9Wave)
			type -= 2;
		else if (type >= kResV1_2_9Movie && type <= kResV1_2_9Text)
			type -= 3;
		else if (type > kResV1_2_3Midi)
			type = 0;
	} else if (_version >= kVersionV1_2_5) {
		if (type >= kResV1_2_5Wave)
			type -= 2;
		else if (type > kResV1_2_3Midi)
			type = 0;
	}

	switch (type) {
	case kResV1_2_3Bitmap:
		freeBitmapBlobIfUnreferenced(id);
		break;
	case kResV1_2_3Midi:
		destroyMidiResourceObject(id);
		break;
	case kResV1_2_3Wave:
		destroyWaveResourceObject(id);
		break;
	case kResV1_2_3Movie:
		destroyMovieResourceObject(id);
		break;
	case kResV1_2_3BitmapGroup:
		destroyBitmapGroupResourceObject(id);
		break;
	case kResV1_2_3MixedGroup:
		result = destroyGroupResourceObject(id);
		break;
	case kResV1_2_3Sprite:
		result = destroySpriteResourceObject(id);
		break;
	case kResV1_2_3Text:
		destroyTextObjectResource(id);
		break;
	default:
		break;
	}

	return result;
}

bool TopgunEngine_v1_2_3::validateAndGetResourceByType(uint16 id, uint16 type) {
	if (!validateResourceType(id, type))
		return false;

	if (_version >= kVersionV1_2_9) {
		if (type == kResV1_2_9Table)
			return ensureTableResourceLoaded(id);

		if (type == kResV1_2_9Tile)
			return ensureTileResourceLoaded(id);

		if (type == kResV1_2_9Wave)
			type -= 2;
		else if (type >= kResV1_2_9Movie && type <= kResV1_2_9Text)
			type -= 3;
		else if (type > kResV1_2_3Midi)
			type = 0;
	} else if (_version >= kVersionV1_2_5) {
		if (type >= kResV1_2_5Wave)
			type -= 2;
		else if (type > kResV1_2_3Midi)
			type = 0;
	}

	switch (type) {
	case kResV1_2_3Bitmap:
		return ensureBitmapResourceLoaded(id);
	case kResV1_2_3Midi:
		return ensureMidiResourceLoaded(id);
	case kResV1_2_3Wave:
		return ensureWaveResourceLoaded(id);
	case kResV1_2_3Movie:
		return ensureMovieResourceLoaded(id);
	case kResV1_2_3OpList:
	case kResV1_2_3Palette:
	case kResV1_2_3SpriteQueueData:
	case kResV1_2_3Script:
		return loadOrGetResourceData(id);
	case kResV1_2_3BitmapGroup:
		return ensureBitmapGroupResourceLoaded(id);
	case kResV1_2_3MixedGroup:
		return ensureGroupResourceLoaded(id);
	case kResV1_2_3Sprite:
		return ensureSpriteResourceLoaded(id);
	case kResV1_2_3Text:
		return ensureTextObjectResourceLoaded(id);
	default:
		return false;
	}
}

bool TopgunEngine_v1_2::isResourceObjectLoaded(uint16 id, uint16 type) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "isResourceObjectLoaded"))
		return false;

	if (!validateResourceType(id, type))
		return false;

	const ResourceRecord &record = _records[index];

	if (_version < kVersionV1_2_2_2) {
		switch (type) {
		case kResV1_2_2Bitmap:
		case kResV1_2_2Movie:
		case kResV1_2_2BitmapGroup:
		case kResV1_2_2OpList:
		case kResV1_2_2SpriteQueueData:
		case kResV1_2_2Script:
			return record.data;
		case kResV1_2_2Midi:
		case kResV1_2_2Wave:
		case kResV1_2_2Sprite:
			return record.handle != 0;
		case kResV1_2_2Text:
			return record.object != 0;
		default:
			return false;
		}
	}

	if (_version >= kVersionV1_2_9) {
		if (type == kResV1_2_9Table || type == kResV1_2_9Tile)
			return record.data;

		if (type == kResV1_2_9Wave)
			type -= 2;
		else if (type >= kResV1_2_9Movie && type <= kResV1_2_9Text)
			type -= 3;
		else if (type > kResV1_2_3Midi)
			type = 0;
	} else if (_version >= kVersionV1_2_5) {
		if (type >= kResV1_2_5Wave)
			type -= 2;
		else if (type > kResV1_2_3Midi)
			type = 0;
	}

	switch (type) {
	case kResV1_2_3Bitmap:
	case kResV1_2_3Movie:
	case kResV1_2_3BitmapGroup:
	case kResV1_2_3OpList:
	case kResV1_2_3Palette:
	case kResV1_2_3SpriteQueueData:
	case kResV1_2_3Script:
		return record.data;
	case kResV1_2_3Midi:
	case kResV1_2_3Wave:
	case kResV1_2_3Sprite:
		return record.handle != 0;
	case kResV1_2_3Text:
		return record.object != 0;
	default:
		return false;
	}
}

} // End of namespace Topgun
