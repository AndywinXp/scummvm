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
#include "topgun/libraries/audio.h"
#include "topgun/libraries/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

enum {
	kMaxFrameBatch = 50,
	kNumSystemColors = 10,
	kNumFileColors = 200,
	kLibraryColorsFirst = kNumSystemColors + kNumFileColors,
	kNumLibraryColors = 36
};

void TopgunEngine_v0::freeBitmapBlobIfUnreferenced(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.data && record.refCount == 0) {
		if (_backgroundId == id) {
			if (_buffer2 != record.data)
				utilsLibrary()->tgFree(_buffer2);

			_buffer2 = record.data;
			_backgroundId = 0;
		} else {
			utilsLibrary()->tgFree(record.data);
		}

		record.data = nullptr;
	}
}

void TopgunEngine_v0::destroyDataResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.data)
		utilsLibrary()->tgFree(record.data);

	record.data = nullptr;
}

bool TopgunEngine_v0::destroyGroupResourceObject(uint16 id) {
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
		warning("TopgunEngine_v0::destroyGroupResourceObject(): the group list is freed through its original pointer");

	utilsLibrary()->tgFree(list);
	record.data = nullptr;
	return true;
}

bool TopgunEngine_v0::destroySpriteResourceObject(uint16 id) {
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

void TopgunEngine_v0::destroyWaveResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	if (_freeingAll && record.handle == _sceneSpriteWave)
		return;

	if (record.handle != 0)
		audioLibrary()->audioCommand(kDestroySoundCommand, record.handle, 0, nullptr);

	if (record.handle == _sceneSpriteWave)
		_sceneSpriteWave = 0;

	record.handle = 0;
}

bool TopgunEngine_v0::ensureBitmapResourceLoaded(uint16 id) {
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

bool TopgunEngine_v0::loadOrGetResourceData(uint16 id) {
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

bool TopgunEngine_v0::ensureGroupResourceLoaded(uint16 id) {
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
		warning("TopgunEngine_v0::ensureGroupResourceLoaded(): the group list is freed through its original pointer");

	utilsLibrary()->tgFree(list);
	return loaded == count;
}

bool TopgunEngine_v0::loadTransPaletteResource(uint16 id) {
	if (!loadOrGetResourceData(id))
		return false;

	ResourceRecord &record = _records[id];
	spriteLibrary()->spriteChangePalette(kLibraryColorsFirst, (uint16)(record.size >> 2), record.data);
	return true;
}

byte *TopgunEngine_v0::loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) {
	uint16 type = resourceLibrary()->resGetType(_file, id);
	if (type == kResourceScript)
		return resourceLibrary()->resLoadResource(_file, id, sizeOut);

	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	resourceLoadProgressCallback();

	bool previousCallback = resourceLibrary()->resSetCallback(_file, true, kProgressInterval);
	bool savedLoading = _loading;
	_loading = true;
	uint16 savedId = _loadingResourceId;
	_loadingResourceId = id;

	byte *block = resourceLibrary()->resLoadResource(_file, id, sizeOut);

	_loading = savedLoading;
	_loadingResourceId = savedId;
	resourceLibrary()->resSetCallback(_file, previousCallback, kProgressInterval);
	return block;
}

bool TopgunEngine_v0::ensureSpriteResourceLoaded(uint16 id) {
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

bool TopgunEngine_v0::attachResourceListToSprite(uint16 spriteHandle, const byte *idList, uint16 count) {
	byte *frames[kMaxFrameBatch];
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

			if (numFrames >= kMaxFrameBatch) {
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

bool TopgunEngine_v0::ensureWaveResourceLoaded(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.handle != 0)
		return true;

	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	record.handle = resourceLibrary()->resCreateWaveEvent(_file, id);
	return record.handle != 0;
}

bool TopgunEngine_v0::destroyResourceObjectByType(uint16 id, uint16 type) {
	if (!_records || id >= _numResources) {
		warning("TopgunEngine_v0::destroyResourceObjectByType(): resource id %d is out of range", id);
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

void TopgunEngine_v0::resetResourcesAndTransientState() {
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

void TopgunEngine_v0::destroySceneMainObjects() {
	if (_sceneSprite != 0)
		_sceneSprite = spriteLibrary()->spriteDestroy(_sceneSprite, true, true);

	if (_sceneSpriteWave != 0)
		audioLibrary()->audioCommand(kDestroySoundCommand, _sceneSpriteWave, 0, nullptr);

	_sceneSpriteWave = 0;
}

bool TopgunEngine_v0::validateAndGetResourceByType(uint16 id, uint16 type) {
	if (!_records || id >= _numResources) {
		warning("TopgunEngine_v0::validateAndGetResourceByType(): resource id %d is out of range", id);
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

bool TopgunEngine_v0::isResourceObjectLoaded(uint16 id) {
	uint16 type = resourceLibrary()->resGetType(_file, id);

	if (type == kResourceBitmap || type == kResourceBlob || type == kResourceScript)
		return _records[id].data;

	if (type == kResourceWave || type == kResourceSprite)
		return _records[id].handle != 0;

	return false;
}

void TopgunEngine_v0::decrementResourceRefCountAndMaybeDestroy(uint16 bitmapId) {
	if (!_records || bitmapId >= _numResources) {
		warning("TopgunEngine_v0::decrementResourceRefCountAndMaybeDestroy(): resource id %d is out of range", bitmapId);
		return;
	}

	if (_records[bitmapId].refCount != 0)
		_records[bitmapId].refCount--;

	if (_records[bitmapId].refCount == 0)
		destroyResourceObjectByType(bitmapId, kResourceBitmap);
}

void TopgunEngine_v0::refreshGamePalette(byte *buffer) {
	if (display()->hasPalette())
		display()->getSystemPaletteEntries(buffer, 0, kNumPaletteEntries);
	else
		memset(buffer, 0, kNumPaletteEntries * kPaletteEntrySize);

	resourceLibrary()->resLoadPalette(_file, buffer + kNumSystemColors * kPaletteEntrySize);
	spriteLibrary()->spriteGetPalette(buffer + kLibraryColorsFirst * kPaletteEntrySize, kLibraryColorsFirst, kNumLibraryColors);
	spriteLibrary()->spriteSetPalette(buffer);
	spriteLibrary()->spriteFadePalette(0, 0, kNumSystemColors, kNumFileColors);
	spriteLibrary()->spriteRealizePalette();
}

bool TopgunEngine_v0::openResourceFileAndInit(const Common::String &name) {
	_file = resourceLibrary()->resOpenFile(name);
	if (_file == 0)
		return false;

	_numResources = resourceLibrary()->resCountResources(_file);
	_records = new ResourceRecord[_numResources];
	_numTimers = 0;
	return true;
}

} // End of namespace Topgun
