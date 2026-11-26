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

#include "common/debug.h"
#include "common/endian.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool TopgunEngine_v1_2::o_callScript(const byte *record) {
	if (_version >= kVersionV1_2_2) {
		uint16 script = READ_LE_UINT16(record + 2);
		byte frameSlots = record[4];
		byte count = record[5];

		if (_version >= kVersionV1_2_3)
			script = (uint16)(operandValue(script) - TG_HOST_V1_2_3_LITERAL_BIAS);

		if (count != 0) {
			int32 destination = (int32)_argumentFrame - frameSlots;

			for (byte i = 0; i < count; i++) {
				uint16 value = operandValue(READ_LE_UINT16(record + TG_SCRIPT_V1_2_2_CALL_HEADER_SIZE + 2 * i));

				if (destination - i < 0) {
					warning("TopgunEngine_v1_2::o_callScript(): argument slot %d is out of range", i);
					continue;
				}

				_variableTable[destination - i] = value;
			}
		}

		_argumentFrame -= frameSlots;
		bool result = runScriptResource(script);
		_argumentFrame += frameSlots;

		return result;
	}

	if (_version >= kVersionV1_2_1) {
		uint16 saved[TG_SCRIPT_V1_2_1_ARGUMENT_FRAME_SIZE];

		memcpy(saved, &_variableTable[TG_SCRIPT_V1_2_1_ARGUMENT_FRAME_VARIABLE], sizeof(saved));

		for (uint16 i = 0; i < record[4]; i++)
			_variableTable[TG_SCRIPT_V1_2_1_ARGUMENT_FRAME_VARIABLE + i] = operandValue(READ_LE_UINT16(record + TG_SCRIPT_V1_2_CALL_HEADER_SIZE + 2 * i));

		bool result = runScriptResource(READ_LE_UINT16(record + 2));

		memcpy(&_variableTable[TG_SCRIPT_V1_2_1_ARGUMENT_FRAME_VARIABLE], saved, sizeof(saved));
		return result;
	}

	if (record[4] == 0)
		return runScriptResource(READ_LE_UINT16(record + 2));

	uint16 savedCount = _scriptArgCount;

	if (!pushScriptArguments(record[4], record + TG_SCRIPT_V1_2_CALL_HEADER_SIZE))
		return false;

	bool result = runScriptResource(READ_LE_UINT16(record + 2));

	_scriptArgCount = savedCount;
	if (savedCount != 0)
		popScriptArguments();

	return result;
}

bool TopgunEngine_v1_2::o_setSpriteQueue(uint16 sprite, uint16 data, bool flag) {
	if (data == 0 && !isResourceObjectLoaded(sprite))
		return true;

	if (!validateAndGetResourceByType(sprite, (_version >= kVersionV1_2_2) ? kResV1_2_2Sprite : kResV1_2Sprite))
		return false;

	uint16 index = operandValue(sprite);
	if (!isRecordIndexValid(index, "o_setSpriteQueue"))
		return false;

	uint16 handle = _records[index].handle;

	if (handle != 0 && flag)
		spriteLib()->spriteHide(handle, true);

	if (data == 0) {
		spriteLib()->spriteEmptyQueue(handle);
		return true;
	}

	uint16 queueType = kResV1_2SpriteQueue;
	if (_version >= kVersionV1_2_2)
		queueType = kResV1_2_2SpriteQueueData;
	else if (_version >= kVersionV1_2_1)
		queueType = kResV1_2_1SpriteQueueData;
	if (!validateAndGetResourceByType(data, queueType))
		return false;

	uint16 dataIndex = operandValue(data);
	if (!isRecordIndexValid(dataIndex, "o_setSpriteQueue"))
		return false;

	spriteLib()->spriteSetQueue(handle, _records[dataIndex].data, (uint16)(_records[dataIndex].size & 0xFFFF));
	return true;
}

bool TopgunEngine_v1_2::o_playWave(uint16 waveId, const byte *params) {
	if (validateAndGetResourceByType(waveId, (_version >= kVersionV1_2_1) ? kResV1_2_1Wave : kResV1_2Wave)) {
		uint16 index = operandValue(waveId);

		if (isRecordIndexValid(index, "o_playWave"))
			audioLib()->audioPlayWave(_records[index].handle, params);
	}

	return true;
}

void TopgunEngine_v1_2::o_playCDTrack(uint16 track) {
	_cdAudioPlaying = ((byte)audioLib()->audioPlayCdTrack((byte)track) != 0);
}

void TopgunEngine_v1_2::o_stopCD() {
	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);
}

void TopgunEngine_v1_2::o_stopWave(uint16 waveId, const byte *block) {
	if (waveId == 0) {
		audioLib()->audioStopAllWaves(block);
		return;
	}

	uint16 index = operandValue(waveId);
	if (!isRecordIndexValid(index, "o_stopWave"))
		return;

	uint16 handle = _records[index].handle;
	if (handle != 0)
		audioLib()->audioStopWave(handle, block);
}

void TopgunEngine_v1_2::o_arithmetic(const byte *record) {
	uint16 destination = READ_LE_UINT16(record + 2);
	int32 accumulator = (int16)operandValue(destination);
	uint16 count = READ_LE_UINT16(record + 4);

	for (uint16 i = 0; i < count; i++)
		accumulator = applyArithmeticOperand(accumulator, record + TG_SCRIPT_V1_2_ARITHMETIC_OPERANDS_OFFSET + i * TG_SCRIPT_V1_2_ARITHMETIC_OPERAND_SIZE);

	writeVariable(destination, (uint16)accumulator);
}

bool TopgunEngine_v1_2::o_conditionalRunScript(const byte *record, uint16 &size) {
	uint16 operandA = operandValue(READ_LE_UINT16(record + 7));
	uint16 operandB = operandValue(READ_LE_UINT16(record + 9));
	bool hit = evaluateCondition(record[0x0B], operandA, operandB);
	uint16 script;

	size = TG_SCRIPT_V1_2_CONDITIONAL_SIZE;

	if (hit) {
		script = READ_LE_UINT16(record + 2);
	} else {
		if (record[6] == 0)
			return true;

		script = READ_LE_UINT16(record + 4);
	}

	if (script == 0) {
		size = 0;
		return true;
	}

	return runScriptResource(script);
}

uint16 TopgunEngine_v1_2::o_conditionalSkip(const byte *record) {
	uint16 operandA = operandValue(READ_LE_UINT16(record + 4));
	uint16 operandB = operandValue(READ_LE_UINT16(record + 6));

	if (evaluateCondition(record[8], operandA, operandB))
		return TG_SCRIPT_V1_2_SKIP_TAKEN_SIZE;

	return READ_LE_UINT16(record + 2);
}

void TopgunEngine_v1_2::o_gotoScene(const byte *record) {
	if (record[0x10] != 0 && _previousSceneIndex == _currentSceneIndex) {
		postMessage(kMsgClose, 1, 0);
		return;
	}

	_gotoRecord.opcode = READ_LE_UINT16(record);
	memcpy(_gotoRecord.name, record + 2, sizeof(_gotoRecord.name));
	_gotoRecord.releaseSceneSprite = record[0x0F];

	if (record[0x10] != 0) {
		if (_previousSceneIndex < _sceneEntries.size())
			memcpy(_gotoRecord.name, _sceneEntries[_previousSceneIndex].name, sizeof(_gotoRecord.name));
		else
			warning("TopgunEngine_v1_2::o_gotoScene(): scene index %d is out of range", _previousSceneIndex);
	}

	if (record[0x0F] != 0)
		destroySceneMainObjects(_releaseOnPalette);

	audioLib()->audioClearNotifications();
	postMessage(kMsgV1_2GotoScene, 0, 0);

	for (uint16 i = 0; i < TG_HOST_V1_2_KEY_TABLE_SIZE; i++)
		_keyTable[i] = KeyEntry_v1_2();

	_dragSpriteId = 0;
}

bool TopgunEngine_v1_2::o_randomRunScript(byte *record) {
	if (record[0x10] != 0) {
		uint16 listId = READ_LE_UINT16(record + 4);
		uint16 count = countScriptListEntries(listId);
		if (count == 0)
			return false;

		uint16 pick = getRandom15Bit() % count;

		if (record[3] == pick) {
			pick++;

			if (pick >= count)
				pick = 0;
		}

		record[3] = (byte)pick;
		return runScriptListEntry(listId, (int16)pick);
	}

	uint16 count = record[2];
	if (count == 0) {
		warning("TopgunEngine_v1_2::o_randomRunScript(): the record has no scripts");
		return true;
	}

	uint16 pick = getRandom15Bit() % count;

	if (record[3] == pick) {
		pick++;

		if (pick >= count)
			pick = 0;
	}

	record[3] = (byte)pick;

	if (pick >= TG_SCRIPT_V1_2_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_2::o_randomRunScript(): script index %d is out of range", pick);
		return true;
	}

	return runScriptResource(READ_LE_UINT16(record + 4 + 2 * pick));
}

bool TopgunEngine_v1_2::o_roundRobinRunScript(byte *record) {
	if (record[0x10] != 0) {
		uint16 listId = READ_LE_UINT16(record + 4);
		uint16 count = countScriptListEntries(listId);
		if (count == 0)
			return false;

		uint16 current = record[3];

		record[3]++;
		if (record[3] >= count)
			record[3] = 0;

		return runScriptListEntry(listId, (int16)current);
	}

	if (record[2] <= record[3])
		record[3] = 0;

	if (record[3] >= TG_SCRIPT_V1_2_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_2::o_roundRobinRunScript(): script index %d is out of range", record[3]);
		return false;
	}

	uint16 id = READ_LE_UINT16(record + 4 + 2 * record[3]);

	if (!validateAndGetResourceByType(id, (_version >= kVersionV1_2_2) ? kResV1_2_2Script : kResV1_2Script))
		return false;

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "o_roundRobinRunScript"))
		return false;

	const byte *nested = _records[index].data;

	if (READ_LE_UINT16(nested) != TG_SCRIPT_V1_2_ROUND_ROBIN_NESTED_OPCODE || nested[2] == nested[3])
		record[3]++;

	return runScriptResource(id);
}

void TopgunEngine_v1_2::o_stopSprite(const byte *record) {
	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_stopSprite"))
		return;

	uint16 sprite = _records[index].handle;
	if (sprite != 0)
		spriteLib()->spriteBreakLoops(sprite);
}

void TopgunEngine_v1_2::o_debugString(const byte *record) {
	const char *text = getStringFromId(READ_LE_UINT16(record + 4));
	if (!text)
		return;

	FormatArgument_v1_2 argument;
	argument.value = operandValue(READ_LE_UINT16(record + 2));

	debug("%s", formatWindowsString(text, &argument, 1).c_str());
}

void TopgunEngine_v1_2::o_setSpriteLevel(uint16 spriteId, uint16 level) {
	if (_version >= kVersionV1_2_1) {
		if (!isResourceObjectLoaded(spriteId))
			return;

		uint16 loadedIndex = operandValue(spriteId);
		if (!isRecordIndexValid(loadedIndex, "o_setSpriteLevel"))
			return;

		spriteLib()->spriteSetLevel(_records[loadedIndex].handle, (int16)level);
		return;
	}

	if (!validateAndGetResourceByType(spriteId, kResV1_2Sprite))
		return;

	uint16 index = operandValue(spriteId);
	if (!isRecordIndexValid(index, "o_setSpriteLevel"))
		return;

	spriteLib()->spriteSetTopMost(_records[index].handle);
}

bool TopgunEngine_v1_2::o_listRunScript(const byte *record) {
	uint16 listId = READ_LE_UINT16(record + 2);
	uint16 count = countScriptListEntries(listId);
	if (count == 0)
		return false;

	uint16 indexVariable = READ_LE_UINT16(record + 6);
	uint16 index = readVariable(indexVariable);
	bool result;

	if (index >= count) {
		if (record[8] != 0)
			result = runScriptResource(READ_LE_UINT16(record + 4));
		else
			result = true;
	} else {
		result = runScriptListEntry(listId, (int16)index);
	}

	if (record[9] != 0) {
		index++;

		if (index >= count)
			index = 0;

		writeVariable(indexVariable, index);
	}

	return result;
}

void TopgunEngine_v1_2::o_moveSprite(const byte *record) {
	if (!isResourceObjectLoaded(READ_LE_UINT16(record + 2)))
		return;

	int16 deltaX = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 deltaY = (int16)operandValue(READ_LE_UINT16(record + 6));
	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_moveSprite"))
		return;

	spriteLib()->spriteOffset(_records[index].handle, deltaX, deltaY);
}

void TopgunEngine_v1_2::o_setDisplayZoom(const byte *record) {
	_displayZoomMode = operandValue(READ_LE_UINT16(record + 2));

	if ((int16)_displayZoomMode < 0 || (int16)_displayZoomMode > TG_SCRIPT_V1_2_MAX_ZOOM_MODE)
		_displayZoomMode = 0;

	_displayZoomX = operandValue(READ_LE_UINT16(record + 4));
	_displayZoomY = operandValue(READ_LE_UINT16(record + 6));

	_displayZoomActive = (_displayZoomMode != 0 && !(_displayZoomMode == 1 && _displayZoomX == 0 && _displayZoomY == 0));

	if (spriteLib()->spriteSetZoom(_displayZoomMode, (int16)_displayZoomX, (int16)_displayZoomY)) {
		if (_screenBox)
			fillBackground();

		spriteLib()->spritePaint(0);
	}
}

void TopgunEngine_v1_2::o_getSpriteInfo(const byte *record) {
	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	uint16 sprite = 0;

	if (isRecordIndexValid(index, "o_getSpriteInfo"))
		sprite = _records[index].handle;

	int16 x = 0;
	int16 y = 0;

	if (sprite != 0)
		spriteLib()->spriteGetPosition(sprite, x, y);

	writeVariable(READ_LE_UINT16(record + 4), (uint16)x);
	writeVariable(READ_LE_UINT16(record + 6), (uint16)y);
}

void TopgunEngine_v1_2::o_setSpriteRect(const byte *record) {
	Common::Rect rect;

	rect.left = (int16)operandValue(READ_LE_UINT16(record + 2));
	rect.top = (int16)operandValue(READ_LE_UINT16(record + 4));
	rect.right = (int16)operandValue(READ_LE_UINT16(record + 6));
	rect.bottom = (int16)operandValue(READ_LE_UINT16(record + 8));

	spriteLib()->spriteSetClipBox(&rect);
}

void TopgunEngine_v1_2::o_random(const byte *record) {
	int16 low = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 high = (int16)operandValue(READ_LE_UINT16(record + 6));

	if (high < low) {
		int16 swap = low;
		low = high;
		high = swap;
	}

	uint16 random = getRandom15Bit();
	uint16 range = (uint16)(high - low + 1);
	uint16 pick;

	if (range == 0) {
		warning("TopgunEngine_v1_2::o_random(): the range holds every number");
		pick = random;
	} else {
		pick = random % range;

		if (_lastRandom == pick)
			pick = (uint16)(pick + 1) % range;
	}

	_lastRandom = pick;
	writeVariable(READ_LE_UINT16(record + 2), (uint16)(pick + low));
}

void TopgunEngine_v1_2::o_getKeyState(const byte *record) {
	uint16 key = READ_LE_UINT16(record + 4);
	bool down = (key < TG_HOST_V1_2_KEY_STATE_SIZE && _keyStates[key]);

	writeVariable(READ_LE_UINT16(record + 2), down ? 1 : 0);
}

void TopgunEngine_v1_2::o_getDate(const byte *record) {
	TimeDate date;
	g_system->getTimeAndDate(date);

	if (READ_LE_UINT16(record + 8) != 0)
		writeVariable(READ_LE_UINT16(record + 8), (uint16)(date.tm_year + 1900));

	if (READ_LE_UINT16(record + 6) != 0)
		writeVariable(READ_LE_UINT16(record + 6), (uint16)date.tm_mday);

	if (READ_LE_UINT16(record + 4) != 0)
		writeVariable(READ_LE_UINT16(record + 4), (uint16)(date.tm_mon + 1));

	if (READ_LE_UINT16(record + 2) != 0)
		writeVariable(READ_LE_UINT16(record + 2), (uint16)date.tm_wday);
}

bool TopgunEngine_v1_2::o_beginTextEdit(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);

	if (_version >= kVersionV1_2_1) {
		uint16 type = kResV1_2_1Text;
		if (_version >= kVersionV1_2_9)
			type = kResV1_2_9Text;
		else if (_version >= kVersionV1_2_5)
			type = kResV1_2_5Text;
		else if (_version >= kVersionV1_2_3)
			type = kResV1_2_3Text;
		else if (_version >= kVersionV1_2_2)
			type = TG_TEXT_V1_2_2_RESOURCE_TYPE;

		// An id of zero only ends the edit that is active
		if (id != 0 && !validateAndGetResourceByType(id, type))
			return false;

		if (_textEditObjectId != 0)
			commitTextEdit(0);

		if (id != 0) {
			if (!beginTextEdit(record))
				return false;

			refreshTextObjectBitmap(getTextObjectFromId(_textEditObjectId));
		}

		return true;
	}

	if (!validateAndGetResourceByType(id, kResV1_2Text))
		return false;

	if (_textEditObjectId != 0)
		commitTextEdit(_textEditObjectId);

	return beginTextEdit(record);
}

void TopgunEngine_v1_2::o_getSpriteRect(const byte *record) {
	if (_version >= kVersionV1_2_9) {
		if (!validateResourceType(READ_LE_UINT16(record + 2), kResV1_2_9Sprite))
			return;
	} else if (_version >= kVersionV1_2_5 && !validateResourceType(READ_LE_UINT16(record + 2), kResV1_2_5Sprite)) {
		return;
	}

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_getSpriteRect"))
		return;

	uint16 sprite = _records[index].handle;
	if (sprite == 0)
		return;

	Common::Rect bounds;
	spriteLib()->spriteGetBounds(sprite, bounds);

	writeVariable(READ_LE_UINT16(record + 4), (uint16)bounds.left);
	writeVariable(READ_LE_UINT16(record + 6), (uint16)bounds.top);
	writeVariable(READ_LE_UINT16(record + 8), (uint16)bounds.right);
	writeVariable(READ_LE_UINT16(record + 10), (uint16)bounds.bottom);
}

void TopgunEngine_v1_2::o_formatString(const byte *record) {
	if ((READ_LE_UINT16(record + 2) & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
		return;

	char *destination = getStringFromId(READ_LE_UINT16(record + 2));
	char *format = getStringFromId(READ_LE_UINT16(record + 4));
	if (!destination || !format)
		return;

	uint16 count = READ_LE_UINT16(record + 6);

	if (count == 0) {
		Common::strlcpy(destination, format, TG_SCRIPT_V1_2_STRING_COPY_SIZE);
		return;
	}

	if (count > TG_SCRIPT_V1_2_FORMAT_MAX_ARGUMENTS) {
		warning("TopgunEngine_v1_2::o_formatString(): %d arguments do not fit in the operation", count);
		count = TG_SCRIPT_V1_2_FORMAT_MAX_ARGUMENTS;
	}

	FormatArgument_v1_2 arguments[TG_SCRIPT_V1_2_FORMAT_MAX_ARGUMENTS];

	for (uint16 i = 0; i < count; i++) {
		uint16 argumentSize = (_version >= kVersionV1_2_3) ? TG_STRING_V1_2_3_FORMAT_ARGUMENT_SIZE : TG_SCRIPT_V1_2_FORMAT_ARGUMENT_SIZE;
		const byte *entry = record + TG_SCRIPT_V1_2_FORMAT_ARGUMENTS_OFFSET + argumentSize * i;
		uint16 word = READ_LE_UINT16(entry);

		if (entry[2] != 0) {
			arguments[i].value = operandValue(word);
		} else {
			arguments[i].text = getStringFromId(word);

			if (!arguments[i].text)
				arguments[i].text = "<NULL>";
		}
	}

	Common::String text = formatWindowsString(format, arguments, count);
	Common::strlcpy(destination, text.c_str(), TG_SCRIPT_V1_2_STRING_COPY_SIZE);
}

void TopgunEngine_v1_2::o_compareStrings(const byte *record) {
	const char *first = getStringFromId(READ_LE_UINT16(record + 4));
	const char *second = getStringFromId(READ_LE_UINT16(record + 6));
	if (!first || !second)
		return;

	int comparison;

	if (READ_LE_UINT16(record) == kOpV1_2CompareStringsIgnoreCase)
		comparison = scumm_stricmp(first, second);
	else
		comparison = strcmp(first, second);

	uint16 result = 0;
	if (comparison < 0)
		result = (uint16)-1;
	else if (comparison > 0)
		result = 1;

	writeVariable(READ_LE_UINT16(record + 2), result);
}

bool TopgunEngine_v1_2::o_setSpritePosition(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);

	if (!validateAndGetResourceByType(id, (_version >= kVersionV1_2_2) ? kResV1_2_2Sprite : kResV1_2Sprite))
		return false;

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "o_setSpritePosition"))
		return false;

	uint16 sprite = _records[index].handle;

	if (sprite != 0) {
		int16 x = (int16)operandValue(READ_LE_UINT16(record + 4));
		int16 y = (int16)operandValue(READ_LE_UINT16(record + 6));

		spriteLib()->spriteSetPosition(sprite, x, y, 0);
	}

	return true;
}

void TopgunEngine_v1_2::o_isSpriteBusy(const byte *record) {
	uint16 index = operandValue(READ_LE_UINT16(record + 4));
	uint16 busy = 0;

	if (isRecordIndexValid(index, "o_isSpriteBusy") && _records[index].handle != 0 && spriteLib()->spriteIsVisible(_records[index].handle) != 0)
		busy = 1;

	writeVariable(READ_LE_UINT16(record + 2), busy);
}

void TopgunEngine_v1_2::o_getSpriteElementIds(const byte *record) {
	uint16 firstVariable = READ_LE_UINT16(record + 4);
	uint16 spriteType = kResV1_2_2Sprite;
	if (_version >= kVersionV1_2_9)
		spriteType = kResV1_2_9Sprite;
	else if (_version >= kVersionV1_2_5)
		spriteType = kResV1_2_5Sprite;
	else if (_version >= kVersionV1_2_3)
		spriteType = kResV1_2_3Sprite;

	uint16 bias = (_version >= kVersionV1_2_3) ? TG_HOST_V1_2_3_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

	if (_version >= kVersionV1_2_5 && !validateResourceType(READ_LE_UINT16(record + 2), spriteType))
		return;

	if (firstVariable >= TG_HOST_V1_2_TABLE_SIZE)
		warning("TopgunEngine_v1_2::o_getSpriteElementIds(): variable %d is out of range", firstVariable);
	else
		_variableTable[firstVariable] = 0;

	if (!isResourceObjectLoaded(READ_LE_UINT16(record + 2), spriteType))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_getSpriteElementIds"))
		return;

	SpriteInfo_v1_2 info;
	spriteLib()->spriteGetInfo(_records[index].handle, info);

	uint16 count = info.frameCount;
	uint16 maxCount = TG_SCRIPT_V1_2_NUM_ELEMENT_VARIABLES;
	if (_version >= kVersionV1_2_7)
		maxCount = TG_SCRIPT_V1_2_7_NUM_ELEMENT_VARIABLES;

	if (count > maxCount) {
		warning("TopgunEngine_v1_2::o_getSpriteElementIds(): the sprite has %d elements", count);
		count = maxCount;
	}

	for (uint16 i = 0; i < count; i++) {
		uint16 variable = READ_LE_UINT16(record + 4 + 2 * i);

		if (variable != 0 && info.frames[i])
			writeVariable(variable, (uint16)(READ_LE_UINT16(info.frames[i] + TG_HOST_V1_2_BITMAP_RESOURCE_ID) + bias));
	}
}

void TopgunEngine_v1_2::o_playMidi(const byte *record) {
	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), (_version >= kVersionV1_2_3) ? kResV1_2_3Midi : kResV1_2_2Midi))
		return;

	byte parameters[TG_AUDIO_V1_2_3_MIDI_PLAY_PARAMS_SIZE];

	if (_version >= kVersionV1_2_3) {
		memcpy(parameters, record + 4, sizeof(parameters));

		uint16 notifyValue = operandValue(READ_LE_UINT16(record + 8));
		uint16 notifyScript = 0;

		if (notifyValue != 0)
			notifyScript = (uint16)(notifyValue - TG_HOST_V1_2_3_LITERAL_BIAS);

		WRITE_LE_UINT16(parameters + TG_AUDIO_V1_2_3_MIDI_PLAY_OFFSET_PARAM, notifyScript);
	}

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_playMidi"))
		return;

	audioLib()->audioPlayMidi(_records[index].handle, (_version >= kVersionV1_2_3) ? parameters : nullptr);
}

void TopgunEngine_v1_2::o_stopMidi(const byte *record) {
	uint16 handle = 0;

	if (_version >= kVersionV1_2_3) {
		if (READ_LE_UINT16(record + 2) != 0) {
			uint16 index = operandValue(READ_LE_UINT16(record + 2));
			if (!isRecordIndexValid(index, "o_stopMidi"))
				return;

			handle = _records[index].handle;
		}

		audioLib()->audioStopMidi(handle);
		return;
	}

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_stopMidi"))
		return;

	handle = _records[index].handle;
	if (handle != 0)
		audioLib()->audioStopMidi(handle);
}

Common::String TopgunEngine_v1_2::formatWindowsString(const char *format, const FormatArgument_v1_2 *arguments, uint16 count) {
	Common::String result;
	FormatArgument_v1_2 missing;
	uint16 next = 0;
	const char *c = format;

	while (*c != '\0' && result.size() < TG_SCRIPT_V1_2_FORMAT_BUFFER_SIZE - 1) {
		if (*c != '%') {
			result += *c;
			c++;
			continue;
		}

		c++;
		if (*c == '\0')
			break;

		bool leftAlign = false;
		bool zeroPad = false;
		bool prefix = false;

		while (*c == '-' || *c == '0' || *c == '#') {
			if (*c == '-')
				leftAlign = true;
			else if (*c == '0')
				zeroPad = true;
			else
				prefix = true;

			c++;
		}

		int width = 0;

		if (*c == '*') {
			const FormatArgument_v1_2 &argument = (next < count) ? arguments[next] : missing;

			next++;
			width = (int16)argument.value;
			c++;

			if (width < 0) {
				leftAlign = true;
				width = -width;
			}
		} else {
			while (*c >= '0' && *c <= '9') {
				width = width * 10 + (*c - '0');
				c++;
			}
		}

		int precision = -1;

		if (*c == '.') {
			c++;
			precision = 0;

			if (*c == '*') {
				const FormatArgument_v1_2 &argument = (next < count) ? arguments[next] : missing;

				next++;
				precision = (int16)argument.value;
				c++;
			} else {
				while (*c >= '0' && *c <= '9') {
					precision = precision * 10 + (*c - '0');
					c++;
				}
			}
		}

		bool isLong = false;

		if (*c == 'l' || *c == 'L') {
			isLong = true;
			c++;
		} else if (*c == 'h') {
			c++;
		}

		char type = *c;
		if (type == '\0')
			break;

		c++;

		Common::String body;
		Common::String sign;

		if (type == 'd' || type == 'i' || type == 'u' || type == 'x' || type == 'X') {
			const FormatArgument_v1_2 &first = (next < count) ? arguments[next] : missing;
			uint32 number = first.value;

			next++;

			if (isLong) {
				const FormatArgument_v1_2 &second = (next < count) ? arguments[next] : missing;

				number |= (uint32)second.value << 16;
				next++;
			}

			if (type == 'd' || type == 'i') {
				int32 signedNumber = isLong ? (int32)number : (int16)number;

				if (signedNumber < 0) {
					sign = "-";
					number = (uint32)0 - (uint32)signedNumber;
				} else {
					number = (uint32)signedNumber;
				}
			}

			if (type == 'x')
				body = Common::String::format("%x", number);
			else if (type == 'X')
				body = Common::String::format("%X", number);
			else
				body = Common::String::format("%u", number);

			if (prefix && (type == 'x' || type == 'X') && number != 0)
				sign = (type == 'x') ? "0x" : "0X";

			while (precision >= 0 && (int)body.size() < precision)
				body = "0" + body;
		} else if (type == 'c') {
			const FormatArgument_v1_2 &argument = (next < count) ? arguments[next] : missing;

			next++;
			body = Common::String((char)argument.value);
		} else if (type == 's') {
			const FormatArgument_v1_2 &argument = (next < count) ? arguments[next] : missing;

			next++;
			body = argument.text ? argument.text : "";

			if (precision >= 0 && (int)body.size() > precision)
				body = Common::String(body.c_str(), precision);
		} else {
			body = Common::String(type);
		}

		int length = (int)(sign.size() + body.size());

		if (length < width) {
			int padding = width - length;

			if (leftAlign) {
				result += sign + body + Common::String((size_t)padding, ' ');
			} else if (zeroPad && precision < 0 && type != 's' && type != 'c') {
				result += sign + Common::String((size_t)padding, '0') + body;
			} else {
				result += Common::String((size_t)padding, ' ') + sign + body;
			}
		} else {
			result += sign + body;
		}
	}

	return result;
}

int32 TopgunEngine_v1_2::roundedDivideBy10000(int32 value) {
	if (value > 0)
		return divideSigned32(value + TG_SCRIPT_V1_2_ANGLE_ROUNDING, TG_SCRIPT_V1_2_ANGLE_RESULT_SCALE);

	return divideSigned32(value - TG_SCRIPT_V1_2_ANGLE_ROUNDING, TG_SCRIPT_V1_2_ANGLE_RESULT_SCALE);
}

Common::Point TopgunEngine_v1_2::computeRayRectIntersection(int16 angle, int16 x, int16 y, int16 left, int16 top, int16 right, int16 bottom) {
	if (angle == 0)
		return Common::Point(x, bottom);

	if (angle == TG_UTILS_ANGLE_QUARTER_TURN)
		return Common::Point(right, y);

	if (angle == TG_UTILS_ANGLE_HALF_TURN)
		return Common::Point(x, top);

	if (angle == TG_UTILS_ANGLE_THREE_QUARTER_TURN)
		return Common::Point(left, y);

	int16 quadrant = utilsLibrary()->getQuadrant(angle);
	int16 edgeX = (quadrant > 1) ? left : right;
	int16 edgeY = (quadrant == 1 || quadrant == 2) ? top : bottom;
	int16 deltaY = (int16)(edgeY - y);
	int16 deltaX = (int16)(edgeX - x);

	if (deltaX == 0 && deltaY == 0)
		return Common::Point(x, y);

	int16 cornerAngle = utilsLibrary()->getAngle(deltaX, deltaY);

	if (cornerAngle == angle)
		return Common::Point(edgeX, edgeY);

	bool verticalEdge = (cornerAngle < angle);
	int16 reducedAngle = (int16)(angle % TG_UTILS_ANGLE_QUARTER_TURN);
	int16 remainder = reducedAngle;

	if (quadrant == 1 || quadrant == 3) {
		verticalEdge = !verticalEdge;

		if (!verticalEdge)
			reducedAngle = (int16)(TG_UTILS_ANGLE_QUARTER_TURN - remainder);

		deltaY = (int16)-deltaY;
		deltaX = (int16)-deltaX;
	} else if (verticalEdge) {
		reducedAngle = (int16)(TG_UTILS_ANGLE_QUARTER_TURN - remainder);
	}

	int32 tangent = utilsLibrary()->tangent(reducedAngle);

	if (verticalEdge) {
		int32 offset = roundedDivideBy10000((int32)((uint32)(int32)deltaX * (uint32)tangent));

		return Common::Point(edgeX, (int16)(offset + y));
	}

	int32 offset = roundedDivideBy10000((int32)((uint32)(int32)deltaY * (uint32)tangent));

	return Common::Point((int16)(offset + x), edgeY);
}

void TopgunEngine_v1_2::o_getAngle(const byte *record) {
	int16 firstX = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 firstY = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 secondX = (int16)operandValue(READ_LE_UINT16(record + 8));
	int16 secondY = (int16)operandValue(READ_LE_UINT16(record + 10));

	writeVariable(READ_LE_UINT16(record + 2), (uint16)utilsLibrary()->getAngle((int16)(secondX - firstX), (int16)(secondY - firstY)));
}

void TopgunEngine_v1_2::o_clipLineToRect(const byte *record) {
	int16 left = (int16)operandValue(READ_LE_UINT16(record + 12));
	int16 right = (int16)operandValue(READ_LE_UINT16(record + 16));

	if (right < left) {
		int16 swap = left;
		left = right;
		right = swap;
	}

	int16 top = (int16)operandValue(READ_LE_UINT16(record + 14));
	int16 bottom = (int16)operandValue(READ_LE_UINT16(record + 18));

	if (bottom < top) {
		int16 swap = top;
		top = bottom;
		bottom = swap;
	}

	int16 x = (int16)operandValue(READ_LE_UINT16(record + 6));
	if (x < left)
		x = left;
	else if (x > right)
		x = right;

	int16 y = (int16)operandValue(READ_LE_UINT16(record + 8));
	if (y < top)
		y = top;
	else if (y > bottom)
		y = bottom;

	int16 angle = utilsLibrary()->normalizeAngle((int16)operandValue(READ_LE_UINT16(record + 10)));
	Common::Point end = computeRayRectIntersection(angle, x, y, left, top, right, bottom);

	if (end.x < left)
		end.x = left;
	else if (end.x > right)
		end.x = right;

	if (end.y < top)
		end.y = top;
	else if (end.y > bottom)
		end.y = bottom;

	writeVariable(READ_LE_UINT16(record + 2), (uint16)end.x);
	writeVariable(READ_LE_UINT16(record + 4), (uint16)end.y);
}

void TopgunEngine_v1_2::o_getIntersect(const byte *record) {
	int16 x1 = (int16)operandValue(READ_LE_UINT16(record + 6));
	int16 y1 = (int16)operandValue(READ_LE_UINT16(record + 8));
	int16 x2 = (int16)operandValue(READ_LE_UINT16(record + 10));
	int16 y2 = (int16)operandValue(READ_LE_UINT16(record + 12));
	int16 x3 = (int16)operandValue(READ_LE_UINT16(record + 14));
	int16 y3 = (int16)operandValue(READ_LE_UINT16(record + 16));
	int16 x4 = (int16)operandValue(READ_LE_UINT16(record + 18));
	int16 y4 = (int16)operandValue(READ_LE_UINT16(record + 20));
	Common::Point intersection;

	if (utilsLibrary()->getIntersect(&intersection, x1, y1, x2, y2, x3, y3, x4, y4) != 0) {
		writeVariable(READ_LE_UINT16(record + 2), (uint16)intersection.x);
		writeVariable(READ_LE_UINT16(record + 4), (uint16)intersection.y);
	}
}

void TopgunEngine_v1_2::o_sineCosine(const byte *record) {
	int16 angle = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 result;

	if (READ_LE_UINT16(record) == kOpV1_2Sine)
		result = utilsLibrary()->sine(angle);
	else
		result = utilsLibrary()->cosine(angle);

	writeVariable(READ_LE_UINT16(record + 2), (uint16)result);
}

void TopgunEngine_v1_2::o_getTime(const byte *record) {
	TimeDate time;
	g_system->getTimeAndDate(time);

	if (READ_LE_UINT16(record + 2) != 0)
		writeVariable(READ_LE_UINT16(record + 2), (uint16)time.tm_hour);

	if (READ_LE_UINT16(record + 4) != 0)
		writeVariable(READ_LE_UINT16(record + 4), (uint16)time.tm_min);

	if (READ_LE_UINT16(record + 6) != 0)
		writeVariable(READ_LE_UINT16(record + 6), (uint16)time.tm_sec);

	// TimeDate has no sub-second field: the hundredths come from the millisecond counter
	if (READ_LE_UINT16(record + 8) != 0)
		writeVariable(READ_LE_UINT16(record + 8), (uint16)((g_system->getMillis() / 10) % 100));
}

void TopgunEngine_v1_2::o_spriteCompositeToBackground(const byte *record) {
	uint16 sprite = 0;

	if (READ_LE_UINT16(record + 2) != 0) {
		uint16 index = operandValue(READ_LE_UINT16(record + 2));
		if (!isRecordIndexValid(index, "o_spriteCompositeToBackground"))
			return;

		sprite = _records[index].handle;
		if (sprite == 0)
			return;
	}

	spriteLib()->spriteCompToBackground(sprite, (_version >= kVersionV1_2_3) ? record[4] : 0);
}

void TopgunEngine_v1_2::o_setCursorPosition(const byte *record) {
	uint16 x = operandValue(READ_LE_UINT16(record + 2));
	uint16 y = operandValue(READ_LE_UINT16(record + 4));

	if (_version >= kVersionV1_2_3) {
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_X, x);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_Y, y);
	}

	g_system->warpMouse(display()->viewportOrigin().x + (int16)x, display()->viewportOrigin().y + (int16)y);
}

} // End of namespace Topgun
