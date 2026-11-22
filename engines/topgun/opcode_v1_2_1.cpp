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
#include "topgun/topgun_v1_2_1.h"

namespace Topgun {

bool TopgunEngine_v1_2_1::o_setSpriteQueue(uint16 sprite, uint16 data, bool flag) {
	if (data == 0 && !isResourceObjectLoaded(sprite, kResV1_2_1Sprite))
		return true;

	if (!validateAndGetResourceByType(sprite, kResV1_2_1Sprite))
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

	if (!validateAndGetResourceByType(data, kResV1_2_1SpriteQueueData))
		return false;

	uint16 dataIndex = operandValue(data);
	if (!isRecordIndexValid(dataIndex, "o_setSpriteQueue"))
		return false;

	spriteLib()->spriteSetQueue(handle, _records[dataIndex].data, (uint16)(_records[dataIndex].size & 0xFFFF));
	return true;
}

bool TopgunEngine_v1_2_1::o_playWave(uint16 waveId, const byte *params) {
	byte parameters[TG_AUDIO_V1_2_1_PLAY_PARAMS_SIZE];
	memcpy(parameters, params, sizeof(parameters));

	uint16 notifyValue = operandValue(READ_LE_UINT16(params + TG_AUDIO_V1_2_1_PLAY_OFFSET_NOTIFY_PARAM));
	uint16 notifyScript = 0;

	if (notifyValue != 0)
		notifyScript = (uint16)(notifyValue - TG_HOST_V1_2_1_LITERAL_BIAS);

	validateAndGetResourceByType(waveId, kResV1_2_1Wave);

	if (isResourceObjectLoaded(waveId, kResV1_2_1Wave)) {
		uint16 index = operandValue(waveId);

		if (isRecordIndexValid(index, "o_playWave"))
			audioLib()->audioPlayWave(_records[index].handle, parameters);
	} else if (params[TG_AUDIO_V1_2_1_PLAY_OFFSET_NOTIFY] != 0 && notifyValue != 0) {
		enqueuePostedScriptRun(notifyScript, waveId, 0);
	}

	return true;
}

void TopgunEngine_v1_2_1::o_stopWave(uint16 waveId, const byte *block) {
	if (waveId == 0) {
		audioLib()->audioStopWave(0, block);
		return;
	}

	if (!isResourceObjectLoaded(waveId, kResV1_2_1Wave))
		return;

	uint16 index = operandValue(waveId);
	if (!isRecordIndexValid(index, "o_stopWave"))
		return;

	audioLib()->audioStopWave(_records[index].handle, block);
}

void TopgunEngine_v1_2_1::o_arithmetic(const byte *record) {
	uint16 destination = READ_LE_UINT16(record + 2);
	int32 accumulator = (int16)operandValue(destination);
	uint16 count = READ_LE_UINT16(record + 4);

	for (uint16 i = 0; i < count; i++)
		accumulator = applyArithmeticOperand(accumulator, record + TG_SCRIPT_V1_2_1_ARITHMETIC_OPERANDS_OFFSET + i * TG_SCRIPT_V1_2_1_ARITHMETIC_OPERAND_SIZE);

	writeVariable(destination, (uint16)accumulator);
}

int32 TopgunEngine_v1_2_1::applyArithmeticOperand(int32 accumulator, const byte *operand) {
	int32 value = (int16)operandValue(READ_LE_UINT16(operand));

	if (operand[TG_SCRIPT_V1_2_1_ARITHMETIC_NEGATE] != 0)
		value = (int32)((uint32)0 - (uint32)value);

	switch (operand[TG_SCRIPT_V1_2_1_ARITHMETIC_OPERATION]) {
	case 0:
		return value;

	case 1:
		return (int32)((uint32)accumulator + (uint32)value);

	case 2:
		return (int32)((uint32)accumulator - (uint32)value);

	case 3:
		return (int32)((uint32)accumulator * (uint32)value);

	case 5:
		return accumulator | value;

	case 6:
		return accumulator & value;

	case 7:
		if (value == 0) {
			warning("TopgunEngine_v1_2_1::applyArithmeticOperand(): division by zero");
			return 0;
		}

		return (int32)((int64)accumulator % (int64)value);

	case 4: {
		if (value == 0)
			return 0;

		int32 half = divideSigned32(value, 2);
		if (value < 0)
			half = (int32)((uint32)0 - (uint32)half);

		return divideSigned32((int32)((uint32)accumulator + (uint32)half), value);
	}

	default:
		return 0;
	}
}

bool TopgunEngine_v1_2_1::o_conditionalRunScript(const byte *record, uint16 &size) {
	uint16 operandA = operandValue(READ_LE_UINT16(record + 8));
	uint16 operandB = operandValue(READ_LE_UINT16(record + 10));
	uint16 hit = evaluateCondition(record[12], operandA, operandB);
	uint16 script;

	size = TG_SCRIPT_V1_2_1_CONDITIONAL_SIZE;

	if (hit != 0) {
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

uint16 TopgunEngine_v1_2_1::evaluateCondition(byte condition, uint16 operandA, uint16 operandB) {
	uint16 result;

	if ((condition & TG_SCRIPT_V1_2_1_CONDITION_EQUAL) != 0) {
		result = (operandB == operandA) ? 1 : 0;
	} else if ((condition & TG_SCRIPT_V1_2_1_CONDITION_LESS) != 0) {
		result = ((int16)operandB < (int16)operandA) ? 1 : 0;
	} else if ((condition & TG_SCRIPT_V1_2_1_CONDITION_GREATER) != 0) {
		result = ((int16)operandB > (int16)operandA) ? 1 : 0;
	} else if ((condition & TG_SCRIPT_V1_2_1_CONDITION_OR) != 0) {
		result = operandB | operandA;
	} else if ((condition & TG_SCRIPT_V1_2_1_CONDITION_AND) != 0) {
		result = operandB & operandA;
	} else if ((condition & TG_SCRIPT_V1_2_1_CONDITION_MODULO) != 0) {
		if (operandB == 0) {
			warning("TopgunEngine_v1_2_1::evaluateCondition(): division by zero");
			result = 0;
		} else {
			result = (uint16)((int16)operandA % (int16)operandB);
		}
	} else {
		result = (operandA != 0) ? 1 : 0;
	}

	if ((condition & TG_SCRIPT_V1_2_1_CONDITION_NEGATE) != 0)
		result = (result == 0) ? 1 : 0;

	return result;
}

bool TopgunEngine_v1_2_1::o_randomRunScript(byte *record) {
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
		warning("TopgunEngine_v1_2_1::o_randomRunScript(): the record has no scripts");
		return true;
	}

	uint16 pick = getRandom15Bit() % count;

	if (record[3] == pick) {
		pick++;

		if (pick >= count)
			pick = 0;
	}

	record[3] = (byte)pick;

	if (pick >= TG_SCRIPT_V1_2_1_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_2_1::o_randomRunScript(): script index %d is out of range", pick);
		return true;
	}

	return runScriptResource(READ_LE_UINT16(record + 4 + 2 * pick));
}

bool TopgunEngine_v1_2_1::o_roundRobinRunScript(byte *record) {
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

	if (record[3] >= TG_SCRIPT_V1_2_1_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_2_1::o_roundRobinRunScript(): script index %d is out of range", record[3]);
		return false;
	}

	uint16 id = READ_LE_UINT16(record + 4 + 2 * record[3]);

	if (!validateAndGetResourceByType(id, kResV1_2_1Script))
		return false;

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "o_roundRobinRunScript"))
		return false;

	const byte *nested = _records[index].data;

	if (READ_LE_UINT16(nested) != kOpV1_2_1RoundRobinRunScript || nested[2] == nested[3])
		record[3]++;

	return runScriptResource(id);
}

void TopgunEngine_v1_2_1::o_stopSprite(const byte *record) {
	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_stopSprite"))
		return;

	uint16 sprite = _records[index].handle;
	if (sprite != 0)
		spriteLib()->spriteBreakLoops(sprite, record[4]);
}

void TopgunEngine_v1_2_1::o_setSpriteLevel(const byte *record) {
	if (!isResourceObjectLoaded(READ_LE_UINT16(record + 2), kResV1_2_1Sprite))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_setSpriteLevel"))
		return;

	uint16 handle = _records[index].handle;
	uint16 level = operandValue(READ_LE_UINT16(record + 4));

	spriteLib()->spriteSetLevel(handle, (int16)level);
}

void TopgunEngine_v1_2_1::o_listRunScript(const byte *record) {
	uint16 listId = READ_LE_UINT16(record + 2);
	uint16 count = countScriptListEntries(listId);
	if (count == 0)
		return;

	uint16 indexVariable = READ_LE_UINT16(record + 6);
	uint16 index = readVariable(indexVariable);

	if (index >= count) {
		if (record[8] != 0)
			runScriptResource(READ_LE_UINT16(record + 4));
	} else {
		runScriptListEntry(listId, (int16)index);
	}

	if (record[9] != 0) {
		index++;

		if (index >= count)
			index = 0;

		writeVariable(indexVariable, index);
	}
}

void TopgunEngine_v1_2_1::o_moveSprite(const byte *record) {
	if (!isResourceObjectLoaded(READ_LE_UINT16(record + 2), kResV1_2_1Sprite))
		return;

	int16 deltaX = (int16)operandValue(READ_LE_UINT16(record + 4));
	int16 deltaY = (int16)operandValue(READ_LE_UINT16(record + 6));
	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_moveSprite"))
		return;

	spriteLib()->spriteOffset(_records[index].handle, deltaX, deltaY);
}

uint16 TopgunEngine_v1_2_1::o_conditionalSkip(const byte *record) {
	uint16 operandA = operandValue(READ_LE_UINT16(record + 4));
	uint16 operandB = operandValue(READ_LE_UINT16(record + 6));

	if (evaluateCondition(record[8], operandA, operandB) != 0)
		return TG_SCRIPT_V1_2_1_SKIP_TAKEN_SIZE;

	return READ_LE_UINT16(record + 2);
}

void TopgunEngine_v1_2_1::o_getSpritePosition(const byte *record) {
	if (!validateResourceType(READ_LE_UINT16(record + 2), kResV1_2_1Sprite))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_getSpritePosition"))
		return;

	uint16 sprite = _records[index].handle;
	int16 x = 0;
	int16 y = 0;

	if (sprite != 0)
		spriteLib()->spriteGetPosition(sprite, x, y);

	writeVariable(READ_LE_UINT16(record + 4), (uint16)x);
	writeVariable(READ_LE_UINT16(record + 6), (uint16)y);
}

bool TopgunEngine_v1_2_1::o_setSpritePosition(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);

	if (!validateAndGetResourceByType(id, kResV1_2_1Sprite))
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

void TopgunEngine_v1_2_1::o_getTime(const byte *record) {
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

void TopgunEngine_v1_2_1::o_spriteCompositeToBackground(const byte *record) {
	uint16 sprite = 0;

	if (READ_LE_UINT16(record + 2) != 0) {
		uint16 index = operandValue(READ_LE_UINT16(record + 2));
		if (!isRecordIndexValid(index, "o_spriteCompositeToBackground"))
			return;

		sprite = _records[index].handle;
		if (sprite == 0)
			return;
	}

	spriteLib()->spriteCompToBackground(sprite, record[4]);
}

void TopgunEngine_v1_2_1::o_getSpriteElementIds(const byte *record) {
	uint16 firstVariable = READ_LE_UINT16(record + 4);

	if (firstVariable >= TG_HOST_V1_2_1_TABLE_SIZE)
		warning("TopgunEngine_v1_2_1::o_getSpriteElementIds(): variable %d is out of range", firstVariable);
	else
		_variableTable[firstVariable] = 0;

	if (!isResourceObjectLoaded(READ_LE_UINT16(record + 2), kResV1_2_1Sprite))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_getSpriteElementIds"))
		return;

	SpriteInfo_v1_2_1 info;
	spriteLib()->spriteGetInfo(_records[index].handle, info);

	uint16 count = info.frameCount;
	if (count > TG_SCRIPT_V1_2_1_NUM_ELEMENT_VARIABLES) {
		warning("TopgunEngine_v1_2_1::o_getSpriteElementIds(): the sprite has %d elements", count);
		count = TG_SCRIPT_V1_2_1_NUM_ELEMENT_VARIABLES;
	}

	for (uint16 i = 0; i < count; i++) {
		uint16 variable = READ_LE_UINT16(record + 4 + 2 * i);

		if (variable != 0 && info.frames[i])
			writeVariable(variable, (uint16)(READ_LE_UINT16(info.frames[i] + TG_HOST_V1_2_1_BITMAP_RESOURCE_ID) + TG_HOST_V1_2_1_LITERAL_BIAS));
	}
}

void TopgunEngine_v1_2_1::o_playMidi(const byte *record) {
	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), kResV1_2_1Midi))
		return;

	byte parameters[TG_AUDIO_V1_2_1_MIDI_PLAY_PARAMS_SIZE];
	memcpy(parameters, record + 4, sizeof(parameters));

	uint16 notifyValue = operandValue(READ_LE_UINT16(record + 8));
	uint16 notifyScript = 0;

	if (notifyValue != 0)
		notifyScript = (uint16)(notifyValue - TG_HOST_V1_2_1_LITERAL_BIAS);

	WRITE_LE_UINT16(parameters + TG_AUDIO_V1_2_1_MIDI_PLAY_OFFSET_PARAM, notifyScript);

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_playMidi"))
		return;

	audioLib()->audioPlayMidi(_records[index].handle, parameters);
}

void TopgunEngine_v1_2_1::o_stopMidi(const byte *record) {
	uint16 handle = 0;

	if (READ_LE_UINT16(record + 2) != 0) {
		uint16 index = operandValue(READ_LE_UINT16(record + 2));
		if (!isRecordIndexValid(index, "o_stopMidi"))
			return;

		handle = _records[index].handle;
	}

	audioLib()->audioStopMidi(handle);
}

void TopgunEngine_v1_2_1::o_setMidiParams(const byte *record) {
	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), kResV1_2_1Midi))
		return;

	byte parameters[TG_SCRIPT_V1_2_1_MIDI_PARAMETERS_SIZE];
	memcpy(parameters, record + 4, sizeof(parameters));

	WRITE_LE_UINT16(parameters + TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_NUMERATOR, operandValue(READ_LE_UINT16(parameters + TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_NUMERATOR)));
	WRITE_LE_UINT16(parameters + TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_DENOMINATOR, operandValue(READ_LE_UINT16(parameters + TG_AUDIO_V1_2_1_MIDI_SET_OFFSET_DENOMINATOR)));

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_setMidiParams"))
		return;

	audioLib()->audioSetMidi(_records[index].handle, parameters);
}

void TopgunEngine_v1_2_1::o_setVolume(const byte *record) {
	bool up = (record[2] != 0);

	if (record[3] == 0 || record[3] == 1)
		audioLib()->audioSetCdVolume(stepVolumeLevels5Bit(audioLib()->audioGetCdVolume(), up));

	if (record[3] == 0 || record[3] == 2)
		audioLib()->audioSetMidiVolume(stepVolumeLevels5Bit(audioLib()->audioGetMidiVolume(), up));

	if (record[3] == 0 || record[3] == 3)
		audioLib()->audioSetWaveVolume(stepVolumeLevels5Bit(audioLib()->audioGetWaveVolume(), up));
}

void TopgunEngine_v1_2_1::o_loadPaletteResource(const byte *record) {
	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), kResV1_2_1Palette))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_loadPaletteResource"))
		return;

	if (!_records[index].data || _records[index].size < TG_SCRIPT_V1_2_1_PALETTE_RESOURCE_SIZE) {
		warning("TopgunEngine_v1_2_1::o_loadPaletteResource(): the palette resource %d is too small", index);
		return;
	}

	byte *entries = _palette + TG_SCRIPT_V1_2_1_FIRST_PALETTE_ENTRY * TG_DISPLAY_PALETTE_ENTRY_SIZE;
	memmove(entries, _records[index].data, TG_SCRIPT_V1_2_1_PALETTE_RESOURCE_SIZE);

	spriteLib()->spriteChangePalette(TG_SCRIPT_V1_2_1_FIRST_PALETTE_ENTRY, TG_SCRIPT_V1_2_1_MAX_PALETTE_ENTRIES, entries, (_fadeIn && !_entryScriptRunning), false);
}

void TopgunEngine_v1_2_1::o_setPaletteEntries(const byte *record) {
	if (!display()->hasPalette())
		return;

	uint16 first = operandValue(READ_LE_UINT16(record + 2));
	if (first < TG_SCRIPT_V1_2_1_FIRST_PALETTE_ENTRY)
		return;

	uint16 count = operandValue(READ_LE_UINT16(record + 4));
	if (count > TG_SCRIPT_V1_2_1_MAX_PALETTE_ENTRIES)
		count = TG_SCRIPT_V1_2_1_MAX_PALETTE_ENTRIES;

	bool animate = (_fadeIn && !_entryScriptRunning);

	if (record[6] != 0) {
		if ((uint32)(first + count) * TG_DISPLAY_PALETTE_ENTRY_SIZE > TG_HOST_V1_2_1_PALETTE_SIZE) {
			warning("TopgunEngine_v1_2_1::o_setPaletteEntries(): the entries %d..%d are out of the palette", first, first + count);
			return;
		}

		spriteLib()->spriteChangePalette(first, count, _palette + first * TG_DISPLAY_PALETTE_ENTRY_SIZE, animate, false);
	} else {
		spriteLib()->spriteChangePalette(first, count, record + TG_SCRIPT_V1_2_1_PALETTE_ENTRIES_OFFSET, animate, true);
	}
}

} // End of namespace Topgun
