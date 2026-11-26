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
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool TopgunEngine_v1_2_3::o_playWave(uint16 waveId, const byte *params) {
	if (_version < kVersionV1_2_3)
		return TopgunEngine_v1_2::o_playWave(waveId, params);

	byte parameters[TG_AUDIO_V1_2_3_PLAY_PARAMS_SIZE];
	memcpy(parameters, params, sizeof(parameters));

	uint16 notifyValue = operandValue(READ_LE_UINT16(params + TG_AUDIO_V1_2_2_2_PLAY_OFFSET_NOTIFY_PARAM));
	uint16 notifyScript = 0;

	if (notifyValue != 0)
		notifyScript = (uint16)(notifyValue - TG_HOST_V1_2_3_LITERAL_BIAS);

	uint16 waveType = kResV1_2_3Wave;
	if (_version >= kVersionV1_2_5)
		waveType = kResV1_2_5Wave;

	// The later version stops the operation when the wave cannot be validated
	if (!validateAndGetResourceByType(waveId, waveType) && _version >= kVersionV1_2_5)
		return false;

	if (isResourceObjectLoaded(waveId, waveType)) {
		uint16 index = operandValue(waveId);

		if (isRecordIndexValid(index, "o_playWave"))
			audioLib()->audioPlayWave(_records[index].handle, parameters);

		if (_version >= kVersionV1_2_7 && _waveStartedScript != 0)
			enqueuePostedScriptRun(_waveStartedScript, waveId, 0);
	} else if (params[TG_AUDIO_V1_2_2_2_PLAY_OFFSET_NOTIFY] != 0 && notifyValue != 0) {
		enqueuePostedScriptRun(notifyScript, waveId, 0);
	}

	return true;
}

void TopgunEngine_v1_2_3::o_arithmetic(const byte *record) {
	if (_version < kVersionV1_2_2_3) {
		TopgunEngine_v1_2::o_arithmetic(record);
		return;
	}

	uint16 destination = READ_LE_UINT16(record + 2);
	int32 accumulator = (int16)operandValue(destination);
	uint16 count = READ_LE_UINT16(record + 4);

	for (uint16 i = 0; i < count; i++)
		accumulator = applyArithmeticOperand(accumulator, record + TG_SCRIPT_V1_2_3_ARITHMETIC_OPERANDS_OFFSET + i * TG_SCRIPT_V1_2_3_ARITHMETIC_OPERAND_SIZE);

	writeVariable(destination, (uint16)accumulator);
}

int32 TopgunEngine_v1_2_3::applyArithmeticOperand(int32 accumulator, const byte *operand) {
	int32 value = (int16)operandValue(READ_LE_UINT16(operand));

	if (operand[TG_SCRIPT_V1_2_3_ARITHMETIC_NEGATE] != 0)
		value = (int32)((uint32)0 - (uint32)value);

	switch (operand[TG_SCRIPT_V1_2_3_ARITHMETIC_OPERATION]) {
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
			warning("TopgunEngine_v1_2_3::applyArithmeticOperand(): division by zero");
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

bool TopgunEngine_v1_2_3::o_conditionalRunScript(const byte *record, uint16 &size) {
	uint16 operandA = operandValue(READ_LE_UINT16(record + 8));
	uint16 operandB = operandValue(READ_LE_UINT16(record + 10));
	uint16 hit = evaluateCondition(record[12], operandA, operandB);
	uint16 script;

	size = TG_SCRIPT_V1_2_3_CONDITIONAL_SIZE;

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

uint16 TopgunEngine_v1_2_3::evaluateCondition(byte condition, uint16 operandA, uint16 operandB) {
	uint16 result;

	if ((condition & TG_SCRIPT_V1_2_3_CONDITION_EQUAL) != 0) {
		result = (operandB == operandA) ? 1 : 0;
	} else if ((condition & TG_SCRIPT_V1_2_3_CONDITION_LESS) != 0) {
		result = ((int16)operandB < (int16)operandA) ? 1 : 0;
	} else if ((condition & TG_SCRIPT_V1_2_3_CONDITION_GREATER) != 0) {
		result = ((int16)operandB > (int16)operandA) ? 1 : 0;
	} else if ((condition & TG_SCRIPT_V1_2_3_CONDITION_OR) != 0) {
		result = operandB | operandA;
	} else if ((condition & TG_SCRIPT_V1_2_3_CONDITION_AND) != 0) {
		result = operandB & operandA;
	} else if ((condition & TG_SCRIPT_V1_2_3_CONDITION_MODULO) != 0) {
		if (operandB == 0) {
			warning("TopgunEngine_v1_2_3::evaluateCondition(): division by zero");
			result = 0;
		} else {
			result = (uint16)((int16)operandA % (int16)operandB);
		}
	} else {
		result = (operandA != 0) ? 1 : 0;
	}

	if ((condition & TG_SCRIPT_V1_2_3_CONDITION_NEGATE) != 0)
		result = (result == 0) ? 1 : 0;

	return result;
}

bool TopgunEngine_v1_2_3::o_randomRunScript(byte *record) {
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
		warning("TopgunEngine_v1_2_3::o_randomRunScript(): the record has no scripts");
		return true;
	}

	uint16 pick = getRandom15Bit() % count;

	if (record[3] == pick) {
		pick++;

		if (pick >= count)
			pick = 0;
	}

	record[3] = (byte)pick;

	if (pick >= TG_SCRIPT_V1_2_3_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_2_3::o_randomRunScript(): script index %d is out of range", pick);
		return true;
	}

	return runScriptResource(READ_LE_UINT16(record + 4 + 2 * pick));
}

bool TopgunEngine_v1_2_3::o_roundRobinRunScript(byte *record) {
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

	if (record[3] >= TG_SCRIPT_V1_2_3_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_2_3::o_roundRobinRunScript(): script index %d is out of range", record[3]);
		return false;
	}

	uint16 id = READ_LE_UINT16(record + 4 + 2 * record[3]);

	uint16 scriptType = kResV1_2_3Script;
	if (_version >= kVersionV1_2_9)
		scriptType = kResV1_2_9Script;
	else if (_version >= kVersionV1_2_5)
		scriptType = kResV1_2_5Script;

	if (!validateAndGetResourceByType(id, scriptType))
		return false;

	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "o_roundRobinRunScript"))
		return false;

	const byte *nested = _records[index].data;

	if (READ_LE_UINT16(nested) != kOpV1_2_3RoundRobinRunScript || nested[2] == nested[3])
		record[3]++;

	return runScriptResource(id);
}

void TopgunEngine_v1_2_3::o_listRunScript(const byte *record) {
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

uint16 TopgunEngine_v1_2_3::o_conditionalSkip(const byte *record) {
	uint16 operandA = operandValue(READ_LE_UINT16(record + 4));
	uint16 operandB = operandValue(READ_LE_UINT16(record + 6));

	if (evaluateCondition(record[8], operandA, operandB) != 0)
		return TG_SCRIPT_V1_2_3_SKIP_TAKEN_SIZE;

	return READ_LE_UINT16(record + 2);
}

void TopgunEngine_v1_2_3::o_getSpritePosition(const byte *record) {
	uint16 spriteType = kResV1_2_3Sprite;
	if (_version >= kVersionV1_2_9)
		spriteType = kResV1_2_9Sprite;
	else if (_version >= kVersionV1_2_5)
		spriteType = kResV1_2_5Sprite;

	if (_version >= kVersionV1_2_3 && !validateResourceType(READ_LE_UINT16(record + 2), spriteType))
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

void TopgunEngine_v1_2_3::o_loadPaletteResource(const byte *record) {
	uint16 paletteType = kResV1_2_3Palette;
	if (_version >= kVersionV1_2_9)
		paletteType = kResV1_2_9Palette;
	else if (_version >= kVersionV1_2_5)
		paletteType = kResV1_2_5Palette;

	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), paletteType))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_loadPaletteResource"))
		return;

	if (!_records[index].data || _records[index].size < TG_SCRIPT_V1_2_3_PALETTE_RESOURCE_SIZE) {
		warning("TopgunEngine_v1_2_3::o_loadPaletteResource(): the palette resource %d is too small", index);
		return;
	}

	byte *entries = _palette + TG_SCRIPT_V1_2_3_FIRST_PALETTE_ENTRY * TG_DISPLAY_PALETTE_ENTRY_SIZE;
	memmove(entries, _records[index].data, TG_SCRIPT_V1_2_3_PALETTE_RESOURCE_SIZE);

	spriteLib()->spriteChangePalette(TG_SCRIPT_V1_2_3_FIRST_PALETTE_ENTRY, TG_SCRIPT_V1_2_3_MAX_PALETTE_ENTRIES, entries, (_fadeIn && !_entryScriptRunning), false);
}

void TopgunEngine_v1_2_3::o_setPaletteEntries(const byte *record) {
	if (!display()->hasPalette())
		return;

	uint16 first = operandValue(READ_LE_UINT16(record + 2));
	if (first < TG_SCRIPT_V1_2_3_FIRST_PALETTE_ENTRY)
		return;

	uint16 count = operandValue(READ_LE_UINT16(record + 4));
	if (count > TG_SCRIPT_V1_2_3_MAX_PALETTE_ENTRIES)
		count = TG_SCRIPT_V1_2_3_MAX_PALETTE_ENTRIES;

	bool animate = (_fadeIn && !_entryScriptRunning);

	if (record[6] != 0) {
		if ((uint32)(first + count) * TG_DISPLAY_PALETTE_ENTRY_SIZE > TG_HOST_V1_2_3_PALETTE_SIZE) {
			warning("TopgunEngine_v1_2_3::o_setPaletteEntries(): the entries %d..%d are out of the palette", first, first + count);
			return;
		}

		spriteLib()->spriteChangePalette(first, count, _palette + first * TG_DISPLAY_PALETTE_ENTRY_SIZE, animate, false);
	} else {
		spriteLib()->spriteChangePalette(first, count, record + TG_SCRIPT_V1_2_3_PALETTE_ENTRIES_OFFSET, animate, true);
	}
}

void TopgunEngine_v1_2_3::o_setQueueDoneScript(const byte *record) {
	_queueDoneScript = operandValue(READ_LE_UINT16(record + 2));
	spriteLib()->spriteSetQueueDoneProc(_queueDoneScript != 0);
}

} // End of namespace Topgun
