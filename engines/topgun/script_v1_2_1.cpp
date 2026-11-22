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

static const byte kScriptOpSizes[] = {
	0, 0, 4, 4, 18, 8, 14, 10, 28, 4,
	4, 4, 12, 2, 4, 8, 24, 14, 18, 12,
	6, 4, 18, 18, 12, 6, 6, 6, 6, 4,
	8, 4, 6, 18, 10, 4, 8, 4, 0, 8,
	8, 8, 10, 0, 2, 2, 8, 6, 10, 12,
	20, 0, 8, 4, 12, 12, 32, 8, 8, 10,
	10, 16, 22, 8, 4, 6, 6, 4, 6, 2,
	10, 6, 6, 6, 0, 12, 0, 0, 0, 0,
	10, 4, 16, 10, 6, 8, 16, 6, 10, 0,
	0, 0, 14, 4, 6, 0, 4, 12, 0, 0,
	0, 0, 0, 0, 0, 0, 4, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 16
};

bool TopgunEngine_v1_2_1::executeScriptOp(byte *ip, uint16 *advance) {
	bool result = true;
	uint16 size = 0;

	switch (READ_LE_UINT16(ip)) {
	case kOpV1_2_1CallScript:
		result = o_callScript(ip);
		size = (ip[5] + 3) * 2;
		break;

	case kOpV1_2_1LoadResource: {
		uint16 id = READ_LE_UINT16(ip + 2);

		result = validateAndGetResourceByType(id, resourceLib()->resGetType(_file, operandValue(id)));
		size = 4;
		break;
	}

	case kOpV1_2_1UnloadResource: {
		uint16 id = READ_LE_UINT16(ip + 2);

		result = destroyResourceObjectByType(id, resourceLib()->resGetType(_file, operandValue(id)));
		size = 4;
		break;
	}

	case kOpV1_2_1SetClickScript:
		result = o_setClickScript(ip);
		size = 18;
		break;

	case kOpV1_2_1SetSpriteQueue:
		result = o_setSpriteQueue(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4), ip[6] != 0);
		size = 8;
		break;

	case kOpV1_2_1SetBackground:
		o_setBackground(ip);
		size = 14;
		break;

	case kOpV1_2_1SetSpriteObjectProperties:
		result = o_setSpriteObjectProperties(ip);
		size = 10;
		break;

	case kOpV1_2_1PlayWave:
		result = o_playWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 28;
		break;

	case kOpV1_2_1PlayCDTrack:
		o_playCDTrack(ip[2]);
		size = 4;
		break;

	case kOpV1_2_1FadeIn:
	case kOpV1_2_1FadeOut:
		o_fadeInOut(ip);
		size = 4;
		break;

	case kOpV1_2_1BindKeyScript:
		o_bindKeyScript(ip);
		size = 12;
		break;

	case kOpV1_2_1Quit:
		if (_quitSceneName[0] != 0) {
			byte record[TG_SCRIPT_V1_2_1_GOTO_RECORD_SIZE];

			memset(record, 0, sizeof(record));
			WRITE_LE_UINT16(record, kOpV1_2_1GotoScene);
			record[TG_SCRIPT_V1_2_1_GOTO_RELEASE_OFFSET] = 1;
			Common::strlcpy((char *)record + TG_SCRIPT_V1_2_1_GOTO_NAME_OFFSET, _quitSceneName, TG_HOST_V1_2_1_SCENE_NAME_SIZE);
			o_gotoScene(record);
		} else {
			postMessage(kMsgClose, 1, 0);
		}

		size = 2;
		break;

	case kOpV1_2_1StopCD:
		o_stopCD();
		size = 4;
		break;

	case kOpV1_2_1StopWave:
		o_stopWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 8;
		break;

	case kOpV1_2_1Arithmetic:
		o_arithmetic(ip);
		size = 24;
		break;

	case kOpV1_2_1ConditionalRunScript:
		result = o_conditionalRunScript(ip, size);
		break;

	case kOpV1_2_1GotoScene:
		o_gotoScene(ip);
		size = 18;
		break;

	case kOpV1_2_1SetScriptTimer:
		o_setScriptTimer(ip);
		size = 12;
		break;

	case kOpV1_2_1CancelScriptTimer:
		o_cancelScriptTimer(ip);
		size = 6;
		break;

	case kOpV1_2_1SetSceneEndScript:
		if (_postSceneEndEnabled)
			postMessage(kMsgV1_2SceneEnd, 0, 0);

		_sceneEndScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_1RandomRunScript:
		result = o_randomRunScript(ip);
		size = 18;
		break;

	case kOpV1_2_1RoundRobinRunScript:
		result = o_roundRobinRunScript(ip);
		size = 18;
		break;

	case kOpV1_2_1TransferSprite:
		o_transferSprite(ip);
		size = 12;
		break;

	case kOpV1_2_1StopSprite:
		o_stopSprite(ip);
		size = 6;
		break;

	case kOpV1_2_1SetSpriteQueueIfLoaded: {
		uint16 index = operandValue(READ_LE_UINT16(ip + 2));

		size = 6;

		if (isRecordIndexValid(index, "executeScriptOp") && _records[index].handle != 0)
			result = o_setSpriteQueue(index, operandValue(READ_LE_UINT16(ip + 4)), false);

		break;
	}

	case kOpV1_2_1DebugString:
		if (_debugMode)
			o_debugString(ip);

		size = 6;
		break;

	case kOpV1_2_1SetSpriteLevel:
		o_setSpriteLevel(ip);
		size = 6;
		break;

	case kOpV1_2_1SetActivationScript:
		_activationScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_1PostScript:
		enqueuePostedScriptRun((uint16)(operandValue(READ_LE_UINT16(ip + 2)) - TG_HOST_V1_2_1_LITERAL_BIAS),
			(uint16)(operandValue(READ_LE_UINT16(ip + 4)) - TG_HOST_V1_2_1_LITERAL_BIAS),
			(uint16)(operandValue(READ_LE_UINT16(ip + 6)) - TG_HOST_V1_2_1_LITERAL_BIAS));
		size = 8;
		break;

	case kOpV1_2_1CursorMode:
		o_setCursorMode(ip);
		size = 4;
		break;

	case kOpV1_2_1RunScriptIfLoaded: {
		uint16 id = READ_LE_UINT16(ip + 2);

		size = 6;

		if (isResourceObjectLoaded(id, resourceLib()->resGetType(_file, operandValue(id)))) {
			if (READ_LE_UINT16(ip + 4) == 0)
				size = 0;
			else
				runScriptResource(READ_LE_UINT16(ip + 4));
		}

		break;
	}

	case kOpV1_2_1AddCollisionPair:
		result = o_addCollisionPair(ip);
		size = 18;
		break;

	case kOpV1_2_1ListRunScript:
		o_listRunScript(ip);
		size = 10;
		break;

	case kOpV1_2_1SetPauseScript:
		_pauseScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_1MoveSprite:
		o_moveSprite(ip);
		size = 8;
		break;

	case kOpV1_2_1Jump:
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV1_2_1ConditionalSkip:
		size = o_conditionalSkip(ip);
		break;

	case kOpV1_2_1SetDisplayZoom:
		o_setDisplayZoom(ip);
		size = 8;
		break;

	case kOpV1_2_1GetSpritePosition:
		o_getSpritePosition(ip);
		size = 8;
		break;

	case kOpV1_2_1FormatTextObject:
		result = o_formatTextObject(ip);
		size = 8;
		break;

	case kOpV1_2_1SetSpriteClipBox:
		o_setSpriteRect(ip);
		size = 10;
		break;

	case kOpV1_2_1Switch:
		size = o_switch(ip);
		break;

	case kOpV1_2_1Stop:
		size = 0;
		break;

	case kOpV1_2_1Nop:
		size = 2;
		break;

	case kOpV1_2_1Random:
		o_random(ip);
		size = 8;
		break;

	case kOpV1_2_1GetKeyState:
		o_getKeyState(ip);
		size = 6;
		break;

	case kOpV1_2_1GetDate:
		o_getDate(ip);
		size = 10;
		break;

	case kOpV1_2_1GetAngle:
		o_getAngle(ip);
		size = 12;
		break;

	case kOpV1_2_1ClipLineToRect:
		o_clipLineToRect(ip);
		size = 20;
		break;

	case kOpV1_2_1SetWallpaper:
		warning("o_setWallpaper UNIMPLEMENTED");
		size = 8;
		break;

	case kOpV1_2_1SetMute:
		audioLib()->audioMute((int8)ip[2]);
		size = 4;
		break;

	case kOpV1_2_1BeginTextEdit:
		result = o_beginTextEdit(ip);
		size = 12;
		break;

	case kOpV1_2_1GetSpriteRect:
		o_getSpriteRect(ip);
		size = 12;
		break;

	case kOpV1_2_1FormatString:
		o_formatString(ip);
		size = 32;
		break;

	case kOpV1_2_1CompareStrings:
	case kOpV1_2_1CompareStringsIgnoreCase:
		o_compareStrings(ip);
		size = 8;
		break;

	case kOpV1_2_1IniRead:
	case kOpV1_2_1IniWrite:
		o_iniAccess(ip);
		size = 10;
		break;

	case kOpV1_2_1SetHoverRegion:
		o_setHoverRegion(ip);
		size = 16;
		break;

	case kOpV1_2_1GetIntersect:
		o_getIntersect(ip);
		size = 22;
		break;

	case kOpV1_2_1SetSpritePosition:
		result = o_setSpritePosition(ip);
		size = 8;
		break;

	case kOpV1_2_1SetMouseEventScript:
		o_setMouseEventScript(ip);
		size = 4;
		break;

	case kOpV1_2_1Cosine:
	case kOpV1_2_1Sine:
		o_sineCosine(ip);
		size = 6;
		break;

	case kOpV1_2_1FlushIni:
		o_flushIni(ip);
		size = 4;
		break;

	case kOpV1_2_1IsSpriteBusy:
		o_isSpriteBusy(ip);
		size = 6;
		break;

	case kOpV1_2_1BackupIni:
		o_backupIni();
		size = 2;
		break;

	case kOpV1_2_1GetTime:
		o_getTime(ip);
		size = 10;
		break;

	case kOpV1_2_1SpriteCompositeToBackground:
		o_spriteCompositeToBackground(ip);
		size = 6;
		break;

	case kOpV1_2_1SetJoystickScript:
		o_setJoystickScript(ip);
		size = 6;
		break;

	case kOpV1_2_1SetCursorPosition:
		o_setCursorPosition(ip);
		size = 6;
		break;

	case kOpV1_2_1GetSpriteElementIds:
		o_getSpriteElementIds(ip);
		size = 12;
		break;

	case kOpV1_2_1Expression:
		evaluateExpression(ip + 4);
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV1_2_1ConditionalJumpExpression:
		size = o_conditionalJumpExpr(ip);
		break;

	case kOpV1_2_1SwitchExpression:
		size = o_switchExpression(ip);
		break;

	case kOpV1_2_1ConditionalJumpExpressionCopy:
		size = o_conditionalJumpExprCopy(ip);
		break;

	case kOpV1_2_1PlayMidi:
		o_playMidi(ip);
		size = 10;
		break;

	case kOpV1_2_1StopMidi:
		o_stopMidi(ip);
		size = 4;
		break;

	case kOpV1_2_1SetDeviceGroupBinding:
		o_setDeviceGroupBinding(ip);
		size = 16;
		break;

	case kOpV1_2_1DeviceGroupButtonState:
		o_deviceGroupButtonState(ip);
		size = 10;
		break;

	case kOpV1_2_1SetDeviceGroupScript:
		o_setDeviceGroupScript(ip);
		size = 6;
		break;

	case kOpV1_2_1DeviceGroupLatch:
		o_deviceGroupLatch(ip);
		size = 8;
		break;

	case kOpV1_2_1SetDeviceGroupBindingB:
		o_setDeviceGroupBindingB(ip);
		size = 16;
		break;

	case kOpV1_2_1SetDeviceGroupFlag:
		o_setDeviceGroupFlag(ip);
		size = 6;
		break;

	case kOpV1_2_1GetDeviceGroupState:
		o_getDeviceGroupState(ip);
		size = 10;
		break;

	case kOpV1_2_1ReturnExpression:
		size = 0;
		_scriptResult = evaluateExpression(ip + 4);
		break;

	case kOpV1_2_1SetMidiParams:
		o_setMidiParams(ip);
		size = 14;
		break;

	case kOpV1_2_1SetVolume:
		o_setVolume(ip);
		size = 4;
		break;

	case kOpV1_2_1SetTextParam:
		o_setTextParam(ip);
		size = 6;
		break;

	case kOpV1_2_1LoadPaletteResource:
		o_loadPaletteResource(ip);
		size = 4;
		break;

	case kOpV1_2_1SetPaletteEntries:
		o_setPaletteEntries(ip);
		size = 12;
		break;

	case kOpV1_2_1SetDebugScriptId:
		_debugScriptId = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_1SetQuitSceneName:
		Common::strlcpy(_quitSceneName, (const char *)ip + 2, TG_HOST_V1_2_1_SCENE_NAME_SIZE);
		size = 16;
		break;

	default:
		warning("TopgunEngine_v1_2_1::executeScriptOp(): unknown opcode %d", READ_LE_UINT16(ip));
		result = false;
		size = 0;
		break;
	}

	if (advance)
		*advance = size;

	return result && _fatalCode == 0;
}

uint16 TopgunEngine_v1_2_1::getScriptOpSize(const byte *ip) {
	uint16 opcode = READ_LE_UINT16(ip);

	if ((uint16)(opcode - 1) > TG_SCRIPT_V1_2_1_LAST_OPCODE - 1)
		return 0;

	if (opcode == kOpV1_2_1CallScript)
		return (ip[5] + 3) * 2;

	if (opcode == kOpV1_2_1Expression)
		return READ_LE_UINT16(ip + 2);

	if (opcode == kOpV1_2_1ConditionalJumpExpression)
		return READ_LE_UINT16(ip + 4);

	return kScriptOpSizes[opcode];
}

bool TopgunEngine_v1_2_1::o_callScript(const byte *record) {
	uint16 script = (uint16)(operandValue(READ_LE_UINT16(record + 2)) - TG_HOST_V1_2_1_LITERAL_BIAS);
	byte frameSlots = record[4];
	byte count = record[5];

	if (count != 0) {
		int32 destination = (int32)_argumentFrame - frameSlots;

		for (byte i = 0; i < count; i++) {
			uint16 value = operandValue(READ_LE_UINT16(record + TG_SCRIPT_V1_2_1_CALL_HEADER_SIZE + 2 * i));

			if (destination - i < 0) {
				warning("TopgunEngine_v1_2_1::o_callScript(): argument slot %d is out of range", i);
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

bool TopgunEngine_v1_2_1::runPostedScriptRecord(const byte *record) {
	byte local[TG_SCRIPT_V1_2_1_POSTED_RECORD_BUFFER_SIZE];
	uint16 count = READ_LE_UINT16(record + 4);

	WRITE_LE_UINT16(local + 2, READ_LE_UINT16(record + 2));
	local[4] = 0;
	local[5] = (byte)count;

	if (TG_SCRIPT_V1_2_1_CALL_HEADER_SIZE + 2 * (uint32)count > sizeof(local)) {
		warning("TopgunEngine_v1_2_1::runPostedScriptRecord(): %d arguments do not fit in the record", count);
		count = (sizeof(local) - TG_SCRIPT_V1_2_1_CALL_HEADER_SIZE) / 2;
		local[5] = (byte)count;
	}

	for (uint16 i = 0; i < count; i++)
		WRITE_LE_UINT16(local + TG_SCRIPT_V1_2_1_CALL_HEADER_SIZE + 2 * i, READ_LE_UINT16(record + TG_SCRIPT_V1_2_1_CALL_HEADER_SIZE + 2 * i));

	return o_callScript(local);
}

bool TopgunEngine_v1_2_1::enqueuePostedScriptRun(uint16 script, uint16 argument1, uint16 argument2) {
	if (_numPostedScriptRuns >= TG_SCRIPT_V1_2_1_MAX_POSTED_RUNS)
		return fatalError(112, "Too many 'run' messages attempted.");

	byte *record = _postedRuns + TG_SCRIPT_V1_2_1_POSTED_RUN_SIZE * _numPostedScriptRuns;

	memset(record, 0, TG_SCRIPT_V1_2_1_POSTED_RUN_SIZE);
	WRITE_LE_UINT16(record + 2, script);
	WRITE_LE_UINT16(record + 6, argument1);

	if (argument1 != 0)
		record[5]++;

	WRITE_LE_UINT16(record + 8, argument2);

	if (argument2 != 0)
		record[5]++;

	_numPostedScriptRuns++;
	return true;
}

void TopgunEngine_v1_2_1::servicePostedRunQueue() {
	uint16 index = 0;

	if (_numPostedScriptRuns != 0) {
		do {
			o_callScript(_postedRuns + TG_SCRIPT_V1_2_1_POSTED_RUN_SIZE * index);
			index++;
		} while (index < _numPostedScriptRuns);
	}

	_numPostedScriptRuns = 0;
}

uint16 TopgunEngine_v1_2_1::operandAddress(uint16 operand) const {
	return (uint16)(TG_SCRIPT_V1_2_1_TABLE_ADDRESS + 2 * variableIndex(operand));
}

uint16 TopgunEngine_v1_2_1::readAddress(uint16 address) const {
	uint16 index = (uint16)((address - TG_SCRIPT_V1_2_1_TABLE_ADDRESS) / 2);

	if (address < TG_SCRIPT_V1_2_1_TABLE_ADDRESS || (address & 1) != 0 || index >= TG_HOST_V1_2_1_TABLE_SIZE) {
		warning("TopgunEngine_v1_2_1::readAddress(): address 0x%X is not inside the variable table", address);
		return 0;
	}

	return _variableTable[index];
}

void TopgunEngine_v1_2_1::writeAddress(uint16 address, uint16 value) {
	uint16 index = (uint16)((address - TG_SCRIPT_V1_2_1_TABLE_ADDRESS) / 2);

	if (address < TG_SCRIPT_V1_2_1_TABLE_ADDRESS || (address & 1) != 0 || index >= TG_HOST_V1_2_1_TABLE_SIZE) {
		warning("TopgunEngine_v1_2_1::writeAddress(): address 0x%X is not inside the variable table", address);
		return;
	}

	_variableTable[index] = value;
}

void TopgunEngine_v1_2_1::o_setBackground(const byte *record) {
	if (READ_LE_UINT16(record + 10) != 0) {
		if (setBackgroundWithWipeEffect(record))
			return;
	}

	setBackgroundByMode(record);
}

void TopgunEngine_v1_2_1::o_gotoScene(const byte *record) {
	if (record[TG_SCRIPT_V1_2_1_GOTO_BACK_OFFSET] != 0 && _previousSceneIndex == _currentSceneIndex) {
		postMessage(kMsgClose, 1, 0);
		return;
	}

	memcpy(_gotoSceneName, record + TG_SCRIPT_V1_2_1_GOTO_NAME_OFFSET, TG_HOST_V1_2_1_SCENE_NAME_SIZE);

	if (record[TG_SCRIPT_V1_2_1_GOTO_BACK_OFFSET] != 0) {
		if (_previousSceneIndex < _sceneEntries.size())
			memcpy(_gotoSceneName, _sceneEntries[_previousSceneIndex].name, TG_HOST_V1_2_1_SCENE_NAME_SIZE);
		else
			warning("TopgunEngine_v1_2_1::o_gotoScene(): scene index %d is out of range", _previousSceneIndex);
	}

	if (record[TG_SCRIPT_V1_2_1_GOTO_RELEASE_OFFSET] != 0)
		destroySceneMainObjects(_releaseOnPalette);

	audioLib()->audioClearNotifications();
	postMessage(kMsgV1_2GotoScene, 0, 0);

	for (uint16 i = 0; i < TG_INPUT_V1_2_1_KEY_TABLE_SIZE; i++)
		_keyTable[i] = KeyEntry_v1_2();

	_dragSpriteId = 0;
}

uint16 TopgunEngine_v1_2_1::o_switch(const byte *record) {
	uint16 count = record[8];
	const byte *table = record + READ_LE_UINT16(record + 4);
	uint16 defaultAdvance = READ_LE_UINT16(record + 6);
	uint16 value = operandValue(READ_LE_UINT16(record + 2));
	uint16 index = 0;

	while (index < count) {
		if (READ_LE_UINT16(table + 2 + TG_SCRIPT_V1_2_1_SWITCH_ENTRY_SIZE * index) == value)
			break;

		index++;
	}

	if (count > index)
		return READ_LE_UINT16(table + TG_SCRIPT_V1_2_1_SWITCH_ENTRY_SIZE * index + 4);

	return defaultAdvance;
}

uint16 TopgunEngine_v1_2_1::o_switchExpression(const byte *record) {
	uint16 count = record[8];
	const byte *table = record + READ_LE_UINT16(record + 4);
	uint16 defaultAdvance = READ_LE_UINT16(record + 6);
	uint16 value = evaluateExpression(record + 10);
	uint16 index = 0;

	while (index < count) {
		if (READ_LE_UINT16(table + 2 + TG_SCRIPT_V1_2_1_SWITCH_ENTRY_SIZE * index) == value)
			break;

		index++;
	}

	if (count > index)
		return READ_LE_UINT16(table + TG_SCRIPT_V1_2_1_SWITCH_ENTRY_SIZE * index + 4);

	return defaultAdvance;
}

uint16 TopgunEngine_v1_2_1::o_conditionalJumpExpr(const byte *record) {
	if (evaluateExpression(record + 6) != 0)
		return READ_LE_UINT16(record + 4);

	return READ_LE_UINT16(record + 2);
}

uint16 TopgunEngine_v1_2_1::o_conditionalJumpExprCopy(const byte *record) {
	if (evaluateExpression(record + 6) != 0)
		return READ_LE_UINT16(record + 4);

	return READ_LE_UINT16(record + 2);
}

} // End of namespace Topgun
