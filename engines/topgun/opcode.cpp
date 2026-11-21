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

#include "topgun/display.h"
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

enum {
	kBitmapIdOffset = 0x0E,
	kNumScriptChoices = 6
};

bool TopgunEngine_v0::o_setClickScript(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);

	if (id != 0) {
		if (!_records || id >= _numResources)
			warning("TopgunEngine_v0::o_setClickScript(): resource id %d is out of range", id);
		else
			_records[id].clickScript = READ_LE_UINT16(record + 4);
	} else {
		addOrUpdateClickRegion(record);
	}

	return true;
}

bool TopgunEngine_v0::o_setSpriteQueue(uint16 spriteId, uint16 dataId) {
	if (!validateAndGetResourceByType(spriteId, kResourceSprite))
		return false;

	if (!validateAndGetResourceByType(dataId, kResourceBlob))
		return false;

	spriteLibrary()->spriteSetQueue(_records[spriteId].handle, _records[dataId].data, (uint16)_records[dataId].size);
	return true;
}

bool TopgunEngine_v0::o_playWave(uint16 waveId, byte *params) {
	if (!validateAndGetResourceByType(waveId, kResourceWave))
		return false;

	byte stopBlock[3] = { 0x01, 0x40, 0x00 };

	audioLibrary()->audioCommand(kStopWavesCommand, 0, 0, stopBlock);
	audioLibrary()->audioCommand(kStartSoundCommand, _records[waveId].handle, 0, params);
	return true;
}

void TopgunEngine_v0::o_playCDTrack(uint16 track) {
	_cdAudioPlaying = true;

	if (!_inputBusy) {
		_inputBusy = true;
		applyCursorFromRegister();
	}

	audioLibrary()->audioCommand(kPlayCDTracksCommand, 0, (uint16)(((track & 0xFF) << 8) | track), nullptr);
}

void TopgunEngine_v0::o_fadeInOut(const byte *record) {
	byte *savedBuffer = _buffer2;

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(true);

	if (_backgroundId != 0)
		_buffer2 = _records[_backgroundId].data;

	if (READ_LE_UINT16(record) == kOpFadeIn)
		sceneTransitionIn();
	else
		sceneTransitionOut();

	_buffer2 = savedBuffer;

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(false);
}

void TopgunEngine_v0::o_bindKeyScript(const byte *record) {
	uint16 scriptKey = READ_LE_UINT16(record + 2);
	uint16 script = READ_LE_UINT16(record + 4);
	Common::KeyCode keyCodes[2];

	if (scriptKey >= 0x80) {
		warning("TopgunEngine_v0::o_bindKeyScript(): key %d is out of range", scriptKey);
		return;
	}

	uint16 numKeyCodes = scriptKeyToKeyCodes(scriptKey, keyCodes);

	for (uint16 i = 0; i < numKeyCodes; i++) {
		KeyScriptRecord &keyScript = _keyScripts[(int)keyCodes[i]];

		if (record[6] != 0)
			keyScript.ctrl = script;
		else if (record[7] != 0)
			keyScript.shift = script;
		else
			keyScript.plain = script;
	}
}

void TopgunEngine_v0::o_stopCD() {
	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;
}

bool TopgunEngine_v0::o_stopWave(uint16 waveId, byte *block) {
	if (waveId == 0) {
		audioLibrary()->audioCommand(kStopWavesCommand, 0, 0, block);
	} else if (!_records || waveId >= _numResources) {
		warning("TopgunEngine_v0::o_stopWave(): resource id %d is out of range", waveId);
	} else if (_records[waveId].handle != 0) {
		audioLibrary()->audioCommand(kStopSoundCommand, _records[waveId].handle, 0, block);
	}

	return true;
}

void TopgunEngine_v0::o_setScriptTimer(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);
	uint16 index = 0;

	while (index < _numTimers && _timers[index].id != id)
		index++;

	if (index >= kMaxTimers)
		return;

	if (index == _numTimers)
		_numTimers++;

	_timers[index].id = id;
	_timers[index].expiry = getEngineMillis() + READ_LE_UINT32(record + 6);
	_timers[index].script = READ_LE_UINT16(record + 4);
}

void TopgunEngine_v0::o_cancelScriptTimer(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);
	uint16 index = 0;

	while (index < _numTimers && _timers[index].id != id)
		index++;

	if (index != _numTimers)
		removeScheduledTimer(index);
}

bool TopgunEngine_v0::o_randomRunScript(const byte *record) {
	uint16 numScripts = record[2];

	if (numScripts == 0) {
		warning("TopgunEngine_v0::o_randomRunScript(): the record has no scripts");
		return true;
	}

	uint16 pick = getRandom15Bit() % numScripts;

	if (record[3] == pick) {
		pick++;

		if (numScripts <= pick)
			pick = 0;
	}

	if (pick >= kNumScriptChoices) {
		warning("TopgunEngine_v0::o_randomRunScript(): script index %d is out of range", pick);
		return true;
	}

	return runScriptResource(READ_LE_UINT16(record + 4 + 2 * pick));
}

bool TopgunEngine_v0::o_roundRobinRunScript(byte *record) {
	if (record[2] <= record[3])
		record[3] = 0;

	if (record[3] >= kNumScriptChoices) {
		warning("TopgunEngine_v0::o_roundRobinRunScript(): script index %d is out of range", record[3]);
		return false;
	}

	uint16 id = READ_LE_UINT16(record + 4 + 2 * record[3]);

	if (!validateAndGetResourceByType(id, kResourceScript))
		return false;

	byte *nested = _records[id].data;
	bool result = runScriptBytecodeLoop(id);

	if (READ_LE_UINT16(nested) != kOpRoundRobinRunScript || nested[2] == nested[3])
		record[3]++;

	return result;
}

bool TopgunEngine_v0::o_transferSprite(uint16 sourceSpriteId, uint16 destinationSpriteId, uint16 dataId, bool freeSource) {
	if (!_records || sourceSpriteId >= _numResources) {
		warning("TopgunEngine_v0::o_transferSprite(): resource id %d is out of range", sourceSpriteId);
		return true;
	}

	if (_records[sourceSpriteId].handle == 0)
		return true;

	bool result = transferSpriteContents(sourceSpriteId, destinationSpriteId, freeSource);

	if (freeSource && _records[sourceSpriteId].handle != 0)
		destroyResourceObjectByType(sourceSpriteId, kResourceSprite);

	if (result)
		result = o_setSpriteQueue(destinationSpriteId, dataId);

	return result;
}

void TopgunEngine_v0::o_setSpriteLevel(uint16 spriteId, uint16 level) {
	if (validateAndGetResourceByType(spriteId, kResourceSprite))
		spriteLibrary()->spriteSetTopMost(_records[spriteId].handle);
}

bool TopgunEngine_v0::transferSpriteContents(uint16 sourceSpriteId, uint16 destinationSpriteId, bool freeSource) {
	if (!_records || sourceSpriteId >= _numResources || destinationSpriteId >= _numResources) {
		warning("TopgunEngine_v0::transferSpriteContents(): resource id %d or %d is out of range", sourceSpriteId, destinationSpriteId);
		return false;
	}

	SpriteInfo sourceInfo;
	SpriteInfo destinationInfo;
	uint16 sourceSprite = _records[sourceSpriteId].handle;

	spriteLibrary()->spriteGetInfo(sourceSprite, sourceInfo);
	spriteLibrary()->spriteEmptyQueue(sourceSprite);
	spriteLibrary()->spriteClearUpdate(sourceSprite);

	if (freeSource) {
		spriteLibrary()->spriteEnable(sourceSprite, 0);

		for (uint16 i = 0; i < sourceInfo.frameCount; i++) {
			if (!sourceInfo.frames[i]) {
				warning("TopgunEngine_v0::transferSpriteContents(): source frame %d is missing", i);
				continue;
			}

			uint16 bitmapId = READ_LE_UINT16(sourceInfo.frames[i] + kBitmapIdOffset);

			if (bitmapId >= _numResources) {
				warning("TopgunEngine_v0::transferSpriteContents(): bitmap id %d is out of range", bitmapId);
				continue;
			}

			_records[bitmapId].refCount++;
		}

		releaseSourceSpriteFrameRefs(sourceSpriteId);
	}

	bool destinationLoaded = (_records[destinationSpriteId].handle != 0);

	if (!destinationLoaded) {
		if (!validateAndGetResourceByType(destinationSpriteId, kResourceSprite))
			return false;
	}

	uint16 destinationSprite = _records[destinationSpriteId].handle;
	spriteLibrary()->spriteGetInfo(destinationSprite, destinationInfo);

	bool transferred = false;

	if (!destinationLoaded || destinationInfo.visible == 0) {
		if (appendQueueBitmapsToSprite(destinationSpriteId, sourceInfo)) {
			spriteLibrary()->spriteTransfer(sourceSprite, destinationSprite);
			transferred = true;
		}
	}

	if (!transferred && !freeSource)
		spriteLibrary()->spriteHide(sourceSprite, true);

	spriteLibrary()->spriteSwapOrder(sourceSprite, destinationSprite, true, false);

	if (freeSource)
		destroySourceSpriteAfterTransfer(sourceSpriteId, sourceInfo);

	return true;
}

bool TopgunEngine_v0::appendQueueBitmapsToSprite(uint16 destinationSpriteId, const SpriteInfo &sourceInfo) {
	if (!_records || destinationSpriteId >= _numResources) {
		warning("TopgunEngine_v0::appendQueueBitmapsToSprite(): resource id %d is out of range", destinationSpriteId);
		return false;
	}

	trimSpriteQueueBitmaps(destinationSpriteId);

	ResourceRecord &record = _records[destinationSpriteId];
	uint16 newCount = record.numMembers + sourceInfo.frameCount;
	uint16 oldCount = newCount - sourceInfo.frameCount;

	byte *list = utilsLibrary()->tgRealloc(record.idList, (uint32)newCount * sizeof(uint16), 0);
	if (!list)
		return false;

	uint16 destinationSprite = record.handle;

	for (uint16 i = 0; i < sourceInfo.frameCount; i++) {
		byte *write = list + (oldCount + i) * sizeof(uint16);

		if (!sourceInfo.frames[i]) {
			warning("TopgunEngine_v0::appendQueueBitmapsToSprite(): source frame %d is missing", i);
			WRITE_LE_UINT16(write, 0);
			continue;
		}

		uint16 bitmapId = READ_LE_UINT16(sourceInfo.frames[i] + kBitmapIdOffset);
		WRITE_LE_UINT16(write, bitmapId);

		if (bitmapId >= _numResources)
			warning("TopgunEngine_v0::appendQueueBitmapsToSprite(): bitmap id %d is out of range", bitmapId);
		else
			_records[bitmapId].refCount++;

		if (destinationSprite == _sceneSprite)
			spriteLibrary()->spriteAppendBitmap(destinationSprite, sourceInfo.frames[i]);
	}

	record.numMembers = newCount;
	record.idList = list;

	if (destinationSprite != _sceneSprite)
		spriteLibrary()->spriteCallback(destinationSprite, true);

	return true;
}

void TopgunEngine_v0::releaseSourceSpriteFrameRefs(uint16 spriteId) {
	if (!_records || spriteId >= _numResources) {
		warning("TopgunEngine_v0::releaseSourceSpriteFrameRefs(): resource id %d is out of range", spriteId);
		return;
	}

	const ResourceRecord &record = _records[spriteId];

	if (record.handle == 0)
		return;

	for (uint16 i = 0; i < record.numMembers; i++) {
		uint16 member = READ_LE_UINT16(record.idList + i * sizeof(uint16));
		uint16 type = resourceLibrary()->resGetType(_file, member);

		if (type == kResourceBitmap)
			decrementResourceRefCountAndMaybeDestroy(member);
		else
			destroyResourceObjectByType(member, type);
	}
}

void TopgunEngine_v0::destroySourceSpriteAfterTransfer(uint16 sourceSpriteId, const SpriteInfo &sourceInfo) {
	if (!_records || sourceSpriteId >= _numResources) {
		warning("TopgunEngine_v0::destroySourceSpriteAfterTransfer(): resource id %d is out of range", sourceSpriteId);
		return;
	}

	ResourceRecord &record = _records[sourceSpriteId];

	spriteLibrary()->spriteDestroy(record.handle, true, false);
	utilsLibrary()->tgFree(record.idList);
	record.handle = 0;
	record.idList = nullptr;

	for (uint16 i = 0; i < sourceInfo.frameCount; i++) {
		if (!sourceInfo.frames[i]) {
			warning("TopgunEngine_v0::destroySourceSpriteAfterTransfer(): source frame %d is missing", i);
			continue;
		}

		decrementResourceRefCountAndMaybeDestroy(READ_LE_UINT16(sourceInfo.frames[i] + kBitmapIdOffset));
	}
}

void TopgunEngine_v0::trimSpriteQueueBitmaps(uint16 spriteId) {
	if (!_records || spriteId >= _numResources) {
		warning("TopgunEngine_v0::trimSpriteQueueBitmaps(): resource id %d is out of range", spriteId);
		return;
	}

	uint16 originalCount = (uint16)(resourceLibrary()->resGetSize(_file, spriteId) >> 1);
	ResourceRecord &record = _records[spriteId];

	if (record.numMembers <= originalCount)
		return;

	for (uint16 i = originalCount; i < record.numMembers; i++)
		decrementResourceRefCountAndMaybeDestroy(READ_LE_UINT16(record.idList + i * sizeof(uint16)));

	record.numMembers = originalCount;
}

} // End of namespace Topgun
