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

#include "topgun/topgun_v2_0.h"

namespace Topgun {

bool TopgunEngine_v2_0::o_callScript(const byte *record) {
	byte copy[TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2 * TG_SCRIPT_V2_0_CALL_MAX_ARGUMENTS] = {};
	byte count = record[TG_SCRIPT_V2_0_CALL_COUNT];

	if (count > TG_SCRIPT_V2_0_CALL_MAX_ARGUMENTS) {
		warning("TopgunEngine_v2_0::o_callScript(): %d arguments do not fit in the record", count);
		count = TG_SCRIPT_V2_0_CALL_MAX_ARGUMENTS;
	}

	memcpy(copy, record, TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2 * count);
	copy[TG_SCRIPT_V2_0_CALL_COUNT] = count;

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[TG_SCRIPT_V2_0_CALL_SCRIPT_FLAG] & 1));

	copy[TG_SCRIPT_V2_0_CALL_MASK_BIT] = 1;
	WRITE_LE_UINT16(copy + TG_SCRIPT_V2_0_CALL_CLEARED_WORD, 0);

	for (byte i = 0; i < count; i++) {
		byte *argument = copy + TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2 * i;

		if ((copy[TG_SCRIPT_V2_0_CALL_MASK_BIT] & copy[TG_SCRIPT_V2_0_CALL_ARGUMENT_FLAGS]) != 0)
			WRITE_LE_UINT16(argument, resolveScriptOperand(READ_LE_UINT16(argument), 1));

		copy[TG_SCRIPT_V2_0_CALL_MASK_BIT] <<= 1;
	}

	return callScriptWithArguments(copy);
}

void TopgunEngine_v2_0::o2_setActivationScript(const byte *record) {
	_activationScript = resolveScriptOperand(READ_LE_UINT16(record + 2), record[4]);
}

void TopgunEngine_v2_0::o2_getAngle(const byte *record) {
	byte copy[14];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 4; i++)
		WRITE_LE_UINT16(copy + 4 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 4 + 2 * i), copy[12] & (1 << i)));

	o_getAngle(copy);
}

void TopgunEngine_v2_0::o2_stopSprite(const byte *record) {
	byte copy[6];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[5]));

	o_stopSprite(copy);
}

void TopgunEngine_v2_0::o2_setHoverRegion(const byte *record) {
	byte copy[22];
	memcpy(copy, record, sizeof(copy));

	resolveScriptRect(copy + 2, copy[21]);
	WRITE_LE_UINT16(copy + 10, resolveScriptOperand(READ_LE_UINT16(copy + 10), copy[20] & 0x04));
	WRITE_LE_UINT16(copy + 14, resolveScriptOperand(READ_LE_UINT16(copy + 14), copy[20] & 0x10));
	WRITE_LE_UINT16(copy + 12, resolveScriptOperand(READ_LE_UINT16(copy + 12), copy[20] & 0x08));
	WRITE_LE_UINT16(copy + 16, resolveScriptOperand(READ_LE_UINT16(copy + 16), copy[20] & 0x20));

	o_setHoverRegion(copy);
}

bool TopgunEngine_v2_0::o2_setClickScript(const byte *record) {
	byte copy[18];
	memcpy(copy, record, sizeof(copy));

	resolveScriptRect(copy + 8, copy[17]);
	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[16] & 0x08));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[16] & 0x10));
	WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[16] & 0x20));

	return o_setClickScript(copy);
}

void TopgunEngine_v2_0::o2_spriteCompositeToBackground(const byte *record) {
	byte copy[6];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[5]));

	o_spriteCompositeToBackground(copy);
}

bool TopgunEngine_v2_0::o2_addCollisionPair(const byte *record) {
	byte copy[18];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 5; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[17] & (1 << i)));

	return o_addCollisionPair(copy);
}

void TopgunEngine_v2_0::o2_sineCosine(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[6]));

	o_sineCosine(copy);
}

void TopgunEngine_v2_0::o2_setCursorPosition(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[6]));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[7]));

	o_setCursorPosition(copy);
}

void TopgunEngine_v2_0::o2_debugString(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[6]));

	o_debugString(copy);
}

bool TopgunEngine_v2_0::o2_setSpriteObjectProperties(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 3; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[9] & (1 << i)));

	return o_setSpriteObjectProperties(copy);
}

bool TopgunEngine_v2_0::o2_beginTextEdit(const byte *record) {
	byte copy[TG_TEXT_V1_2_4_EDIT_RECORD_SIZE];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[11] & 0x02));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[11] & 0x04));

	return o_beginTextEdit(copy);
}

void TopgunEngine_v2_0::o2_getSpriteRect(const byte *record) {
	byte copy[14];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));

	o_getSpriteRect(copy);
}

void TopgunEngine_v2_0::o2_getSpriteElementIds(const byte *record) {
	byte copy[22];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));

	o_getSpriteElementIds(copy);
}

void TopgunEngine_v2_0::o2_getIntersect(const byte *record) {
	byte copy[24];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 8; i++)
		WRITE_LE_UINT16(copy + 6 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 6 + 2 * i), copy[22] & (1 << i)));

	o_getIntersect(copy);
}

void TopgunEngine_v2_0::o2_getSpritePosition(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));

	o_getSpritePosition(copy);
}

bool TopgunEngine_v2_0::o2_conditionalRunScript(const byte *record, uint16 &size) {
	byte copy[TG_SCRIPT_V1_2_4_CONDITIONAL_SIZE];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[7] & 0x01));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[7] & 0x02));
	WRITE_LE_UINT16(copy + 8, resolveScriptOperand(READ_LE_UINT16(copy + 8), copy[13] & 0x01));
	WRITE_LE_UINT16(copy + 10, resolveScriptOperand(READ_LE_UINT16(copy + 10), copy[13] & 0x02));

	return o_conditionalRunScript(copy, size);
}

uint16 TopgunEngine_v2_0::o2_conditionalSkip(const byte *record) {
	byte copy[TG_SCRIPT_V1_2_4_SKIP_TAKEN_SIZE];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[9] & 0x01));
	WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[9] & 0x02));

	return o_conditionalSkip(copy);
}

bool TopgunEngine_v2_0::o2_runScriptIfLoaded(const byte *record) {
	uint16 id = resolveScriptOperand(READ_LE_UINT16(record + 2), record[6]);
	uint16 script = resolveScriptOperand(READ_LE_UINT16(record + 4), record[7]);

	if (isResourceObjectLoaded(id, resourceLib()->resGetType(_file, id))) {
		if (script == 0)
			return false;

		runScriptResource(script);
	}

	return true;
}

void TopgunEngine_v2_0::o2_setDeviceGroupBinding(const byte *record) {
	byte copy[16];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 3; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[15] & (1 << i)));

	o_setDeviceGroupBinding(copy);
}

void TopgunEngine_v2_0::o2_deviceGroupButtonState(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[9] & 0x01));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[9] & 0x02));

	o_deviceGroupButtonState(copy);
}

void TopgunEngine_v2_0::o2_setDeviceGroupScript(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[6]));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[7]));

	o_setDeviceGroupScript(copy);
}

void TopgunEngine_v2_0::o2_getDeviceGroupState(const byte *record) {
	byte copy[12];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[10]));

	o_getDeviceGroupState(copy);
}

void TopgunEngine_v2_0::o2_setJoystickScript(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[6]));

	o_setJoystickScript(copy);
}

void TopgunEngine_v2_0::o2_getKeyState(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[6]));

	o_getKeyState(copy);
}

void TopgunEngine_v2_0::o2_bindKeyScript(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[6] & 0x02));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[6] & 0x01));

	o_bindKeyScript(copy);
}

void TopgunEngine_v2_0::o2_setSpriteLevel(const byte *record) {
	uint16 sprite = resolveScriptOperand(READ_LE_UINT16(record + 2), record[6]);
	uint16 level = resolveScriptOperand(READ_LE_UINT16(record + 4), record[7]);

	o_setSpriteLevel(sprite, level);
}

void TopgunEngine_v2_0::o2_setSceneEndScript(const byte *record) {
	if (_postSceneEndEnabled)
		postMessage(kMsgV1_2SceneEnd, 0, 0);

	_sceneEndScript = resolveScriptOperand(READ_LE_UINT16(record + 2), record[4]);
}

bool TopgunEngine_v2_0::o2_loadUnloadResource(const byte *record) {
	uint16 id = resolveScriptOperand(READ_LE_UINT16(record + 2), record[4]);
	uint16 type = resourceLib()->resGetType(_file, id);

	if (READ_LE_UINT16(record) == kOpV2_0LoadResource)
		return validateAndGetResourceByType(id, type);

	return destroyResourceObjectByType(id, type);
}

void TopgunEngine_v2_0::o2_setMouseEventScript(const byte *record) {
	byte copy[6];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));

	o_setMouseEventScript(copy);
}

void TopgunEngine_v2_0::o2_moveSprite(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 3; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[8] & (1 << i)));

	o_moveSprite(copy);
}

void TopgunEngine_v2_0::o2_setPauseScript(const byte *record) {
	_pauseScript = resolveScriptOperand(READ_LE_UINT16(record + 2), record[4]);
}

void TopgunEngine_v2_0::o2_playMidi(const byte *record) {
	byte copy[12];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));
	WRITE_LE_UINT16(copy + 10, resolveScriptOperand(READ_LE_UINT16(copy + 10), copy[9]));

	o_playMidi(copy);
}

bool TopgunEngine_v2_0::o2_playWave(const byte *record) {
	byte copy[28];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));
	WRITE_LE_UINT16(copy + 26, resolveScriptOperand(READ_LE_UINT16(copy + 26), copy[8] & 0x08));

	return o_playWave(READ_LE_UINT16(copy + 2), copy + 6);
}

void TopgunEngine_v2_0::o2_postScript(const byte *record) {
	uint16 script = resolveScriptOperand(READ_LE_UINT16(record + 2), record[8] & 0x01);
	uint16 argument1 = resolveScriptOperand(READ_LE_UINT16(record + 4), record[8] & 0x02);
	uint16 argument2 = resolveScriptOperand(READ_LE_UINT16(record + 6), record[8] & 0x04);

	enqueuePostedScriptRun(script, argument1, argument2);
}

void TopgunEngine_v2_0::o2_random(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[8]));
	WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[9]));

	o_random(copy);
}

bool TopgunEngine_v2_0::o2_randomRunScript(byte *record) {
	byte copy[18];
	memcpy(copy, record, sizeof(copy));

	if (copy[4] != 0) {
		WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[5]));
	} else {
		for (uint16 i = 0; i < TG_SCRIPT_V1_2_4_NUM_SCRIPT_CHOICES; i++)
			WRITE_LE_UINT16(copy + 6 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 6 + 2 * i), copy[5] & (1 << i)));
	}

	bool result = o_randomRunScript(copy);

	record[3] = copy[3];

	return result;
}

void TopgunEngine_v2_0::o2_setBackground(const byte *record) {
	byte copy[TG_SCRIPT_V1_2_7_SET_BACKGROUND_SIZE];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[8] & 0x01));
	WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[8] & 0x02));

	o_setBackground(copy);
}

bool TopgunEngine_v2_0::o2_roundRobinRunScript(byte *record) {
	byte copy[18];
	memcpy(copy, record, sizeof(copy));

	if (copy[4] != 0) {
		WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[5]));
	} else {
		for (uint16 i = 0; i < TG_SCRIPT_V1_2_4_NUM_SCRIPT_CHOICES; i++)
			WRITE_LE_UINT16(copy + 6 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 6 + 2 * i), copy[5] & (1 << i)));
	}

	bool result = o_roundRobinRunScript(copy);

	record[3] = copy[3];

	return result;
}

void TopgunEngine_v2_0::o2_arithmetic(const byte *record) {
	byte copy[24];
	memcpy(copy, record, sizeof(copy));

	uint16 count = READ_LE_UINT16(copy + 4);

	if (count > TG_SCRIPT_V2_0_ARITHMETIC_MAX_OPERANDS) {
		warning("TopgunEngine_v2_0::o2_arithmetic(): %d operands do not fit in the operation", count);
		count = TG_SCRIPT_V2_0_ARITHMETIC_MAX_OPERANDS;
		WRITE_LE_UINT16(copy + 4, count);
	}

	for (uint16 i = 0; i < count; i++) {
		byte *operand = copy + TG_SCRIPT_V1_2_4_ARITHMETIC_OPERANDS_OFFSET + i * TG_SCRIPT_V1_2_4_ARITHMETIC_OPERAND_SIZE;

		WRITE_LE_UINT16(operand, resolveScriptOperand(READ_LE_UINT16(operand), operand[TG_SCRIPT_V2_0_ARITHMETIC_VARIABLE]));
	}

	o_arithmetic(copy);
}

void TopgunEngine_v2_0::o2_setPaletteEntries(const byte *record) {
	byte copy[12];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[7] & 0x01));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[7] & 0x02));

	if (copy[6] == 0 && READ_LE_UINT16(copy + 4) > 1) {
		warning("TopgunEngine_v2_0::o2_setPaletteEntries(): the operation holds one entry, not %d", READ_LE_UINT16(copy + 4));
		WRITE_LE_UINT16(copy + 4, 1);
	}

	o_setPaletteEntries(copy);
}

void TopgunEngine_v2_0::o2_loadPaletteResource(const byte *record) {
	byte copy[6];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));

	o_loadPaletteResource(copy);
}

bool TopgunEngine_v2_0::o2_setSpritePosition(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 3; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[8] & (1 << i)));

	return o_setSpritePosition(copy);
}

bool TopgunEngine_v2_0::o2_setSpriteQueue(const byte *record) {
	uint16 sprite = resolveScriptOperand(READ_LE_UINT16(record + 2), record[7] & 0x01);
	uint16 queue = resolveScriptOperand(READ_LE_UINT16(record + 4), record[7] & 0x02);

	return o_setSpriteQueue(sprite, queue, record[6] != 0);
}

void TopgunEngine_v2_0::o2_formatString(const byte *record) {
	byte copy[32];
	memcpy(copy, record, sizeof(copy));

	uint16 count = READ_LE_UINT16(copy + 6);

	for (uint16 i = 0; i < count && i < TG_SCRIPT_V1_2_FORMAT_MAX_ARGUMENTS; i++) {
		byte *argument = copy + TG_SCRIPT_V1_2_FORMAT_ARGUMENTS_OFFSET + TG_STRING_V1_2_4_FORMAT_ARGUMENT_SIZE * i;

		if (argument[2] != 0)
			WRITE_LE_UINT16(argument, resolveScriptOperand(READ_LE_UINT16(argument), argument[3]));
	}

	o_formatString(copy);
}

bool TopgunEngine_v2_0::o2_formatTextObject(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[8]));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[9]));

	return o_formatTextObject(copy);
}

void TopgunEngine_v2_0::o2_setTextParam(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[6]));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[7]));

	o_setTextParam(copy);
}

void TopgunEngine_v2_0::o2_setScriptTimer(const byte *record) {
	byte copy[12];
	memcpy(copy, record, sizeof(copy));

	if ((copy[10] & 0x01) != 0)
		WRITE_LE_UINT16(copy + 6, resolveScriptOperand(READ_LE_UINT16(copy + 6), copy[10] & 0x04));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[10] & 0x02));

	o_setScriptTimer(copy);
}

void TopgunEngine_v2_0::o2_stopMidi(const byte *record) {
	byte copy[6];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[4]));

	o_stopMidi(copy);
}

void TopgunEngine_v2_0::o2_stopWave(const byte *record) {
	o_stopWave(resolveScriptOperand(READ_LE_UINT16(record + 2), record[4]), record + 6);
}

void TopgunEngine_v2_0::o2_listRunScript(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 2, resolveScriptOperand(READ_LE_UINT16(copy + 2), copy[8] & 0x04));
	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[8] & 0x08));

	o_listRunScript(copy);
}

void TopgunEngine_v2_0::o2_transferSprite(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 3; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[9] & (1 << i)));

	o_transferSprite(copy);
}

void TopgunEngine_v2_0::o2_clipLineToRect(const byte *record) {
	byte copy[22];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 7; i++)
		WRITE_LE_UINT16(copy + 6 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 6 + 2 * i), copy[20] & (1 << i)));

	o_clipLineToRect(copy);
}

void TopgunEngine_v2_0::o2_isSpriteBusy(const byte *record) {
	byte copy[8];
	memcpy(copy, record, sizeof(copy));

	WRITE_LE_UINT16(copy + 4, resolveScriptOperand(READ_LE_UINT16(copy + 4), copy[6]));

	o_isSpriteBusy(copy);
}

void TopgunEngine_v2_0::o2_setDisplayZoom(const byte *record) {
	byte copy[10];
	memcpy(copy, record, sizeof(copy));

	for (uint16 i = 0; i < 3; i++)
		WRITE_LE_UINT16(copy + 2 + 2 * i, resolveScriptOperand(READ_LE_UINT16(copy + 2 + 2 * i), copy[8] & (1 << i)));

	o_setDisplayZoom(copy);
}

} // End of namespace Topgun
