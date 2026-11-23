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

#include "topgun/topgun_v1_2_3.h"

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

bool TopgunEngine_v1_2_3::executeScriptOp(byte *ip, uint16 *advance) {
	bool result = true;
	uint16 size = 0;

	switch (READ_LE_UINT16(ip)) {
	case kOpV1_2_3CallScript:
		result = o_callScript(ip);
		size = (ip[5] + 3) * 2;
		break;

	case kOpV1_2_3LoadResource: {
		uint16 id = READ_LE_UINT16(ip + 2);

		result = validateAndGetResourceByType(id, resourceLib()->resGetType(_file, operandValue(id)));
		size = 4;
		break;
	}

	case kOpV1_2_3UnloadResource: {
		uint16 id = READ_LE_UINT16(ip + 2);

		result = destroyResourceObjectByType(id, resourceLib()->resGetType(_file, operandValue(id)));
		size = 4;
		break;
	}

	case kOpV1_2_3SetClickScript:
		result = o_setClickScript(ip);
		size = 18;

		if (_version >= kVersionV1_2_6)
			size = TG_SCRIPT_V1_2_6_SET_CLICK_SCRIPT_SIZE;

		break;

	case kOpV1_2_3SetSpriteQueue:
		result = o_setSpriteQueue(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4), ip[6] != 0);
		size = 8;
		break;

	case kOpV1_2_3SetBackground:
		o_setBackground(ip);
		size = 14;

		if (_version >= kVersionV1_2_6)
			size = TG_SCRIPT_V1_2_6_SET_BACKGROUND_SIZE;

		break;

	case kOpV1_2_3SetSpriteObjectProperties:
		result = o_setSpriteObjectProperties(ip);
		size = 10;
		break;

	case kOpV1_2_3PlayWave:
		result = o_playWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 28;
		break;

	case kOpV1_2_3PlayCDTrack:
		o_playCDTrack(ip[2]);
		size = 4;
		break;

	case kOpV1_2_3FadeIn:
	case kOpV1_2_3FadeOut:
		o_fadeInOut(ip);
		size = 4;
		break;

	case kOpV1_2_3BindKeyScript:
		o_bindKeyScript(ip);
		size = 12;
		break;

	case kOpV1_2_3Quit:
		if (_quitSceneName[0] != 0) {
			byte record[TG_SCRIPT_V1_2_5_GOTO_RECORD_SIZE];
			uint16 recordSize = TG_SCRIPT_V1_2_3_GOTO_RECORD_SIZE;
			uint16 releaseOffset = TG_SCRIPT_V1_2_3_GOTO_RELEASE_OFFSET;
			uint16 nameSize = TG_HOST_V1_2_3_SCENE_NAME_SIZE;

			if (_version >= kVersionV1_2_5) {
				recordSize = TG_SCRIPT_V1_2_5_GOTO_RECORD_SIZE;
				releaseOffset = TG_SCRIPT_V1_2_5_GOTO_RELEASE_OFFSET;
				nameSize = TG_HOST_V1_2_5_SCENE_NAME_SIZE;
			}

			memset(record, 0, recordSize);
			WRITE_LE_UINT16(record, kOpV1_2_3GotoScene);
			record[releaseOffset] = 1;
			Common::strlcpy((char *)record + TG_SCRIPT_V1_2_3_GOTO_NAME_OFFSET, _quitSceneName, nameSize);
			o_gotoScene(record);
		} else {
			postMessage(kMsgClose, 1, 0);
		}

		size = 2;
		break;

	case kOpV1_2_3StopCD:
		o_stopCD();
		size = 4;
		break;

	case kOpV1_2_3StopWave:
		o_stopWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 8;
		break;

	case kOpV1_2_3Arithmetic:
		o_arithmetic(ip);
		size = 24;
		break;

	case kOpV1_2_3ConditionalRunScript:
		result = o_conditionalRunScript(ip, size);
		break;

	case kOpV1_2_3GotoScene:
		o_gotoScene(ip);
		size = TG_SCRIPT_V1_2_3_GOTO_RECORD_SIZE;

		if (_version >= kVersionV1_2_5)
			size = TG_SCRIPT_V1_2_5_GOTO_RECORD_SIZE;

		break;

	case kOpV1_2_3SetScriptTimer:
		o_setScriptTimer(ip);
		size = 12;
		break;

	case kOpV1_2_3CancelScriptTimer:
		o_cancelScriptTimer(ip);
		size = 6;
		break;

	case kOpV1_2_3SetSceneEndScript:
		if (_postSceneEndEnabled)
			postMessage(kMsgV1_2SceneEnd, 0, 0);

		_sceneEndScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_3RandomRunScript:
		result = o_randomRunScript(ip);
		size = 18;
		break;

	case kOpV1_2_3RoundRobinRunScript:
		result = o_roundRobinRunScript(ip);
		size = 18;
		break;

	case kOpV1_2_3TransferSprite:
		o_transferSprite(ip);
		size = 12;
		break;

	case kOpV1_2_3StopSprite:
		o_stopSprite(ip);
		size = 6;
		break;

	case kOpV1_2_3SetSpriteQueueIfLoaded: {
		uint16 index = operandValue(READ_LE_UINT16(ip + 2));

		size = 6;

		if (isRecordIndexValid(index, "executeScriptOp") && _records[index].handle != 0)
			result = o_setSpriteQueue(index, operandValue(READ_LE_UINT16(ip + 4)), false);

		break;
	}

	case kOpV1_2_3DebugString:
		if (_debugMode)
			o_debugString(ip);

		size = 6;
		break;

	case kOpV1_2_3SetSpriteLevel:
		o_setSpriteLevel(ip);
		size = 6;
		break;

	case kOpV1_2_3SetActivationScript:
		_activationScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_3PostScript:
		enqueuePostedScriptRun((uint16)(operandValue(READ_LE_UINT16(ip + 2)) - TG_HOST_V1_2_3_LITERAL_BIAS),
			(uint16)(operandValue(READ_LE_UINT16(ip + 4)) - TG_HOST_V1_2_3_LITERAL_BIAS),
			(uint16)(operandValue(READ_LE_UINT16(ip + 6)) - TG_HOST_V1_2_3_LITERAL_BIAS));
		size = 8;
		break;

	case kOpV1_2_3CursorMode:
		o_setCursorMode(ip);
		size = 4;
		break;

	case kOpV1_2_3RunScriptIfLoaded: {
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

	case kOpV1_2_3AddCollisionPair:
		result = o_addCollisionPair(ip);
		size = 18;
		break;

	case kOpV1_2_3ListRunScript:
		o_listRunScript(ip);
		size = 10;
		break;

	case kOpV1_2_3SetPauseScript:
		_pauseScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_3MoveSprite:
		o_moveSprite(ip);
		size = 8;
		break;

	case kOpV1_2_3Jump:
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV1_2_3ConditionalSkip:
		size = o_conditionalSkip(ip);
		break;

	case kOpV1_2_3SetDisplayZoom:
		o_setDisplayZoom(ip);
		size = 8;
		break;

	case kOpV1_2_3GetSpritePosition:
		o_getSpritePosition(ip);
		size = 8;
		break;

	case kOpV1_2_3FormatTextObject:
		result = o_formatTextObject(ip);
		size = 8;
		break;

	case kOpV1_2_3SetSpriteClipBox:
		o_setSpriteRect(ip);
		size = 10;
		break;

	case kOpV1_2_3Switch:
		size = lookupSwitchOffset(ip);
		break;

	case kOpV1_2_3Stop:
		size = 0;
		break;

	case kOpV1_2_3Nop:
		size = 2;
		break;

	case kOpV1_2_3Random:
		o_random(ip);
		size = 8;
		break;

	case kOpV1_2_3GetKeyState:
		o_getKeyState(ip);
		size = 6;
		break;

	case kOpV1_2_3GetDate:
		o_getDate(ip);
		size = 10;
		break;

	case kOpV1_2_3GetAngle:
		o_getAngle(ip);
		size = 12;
		break;

	case kOpV1_2_3ClipLineToRect:
		o_clipLineToRect(ip);
		size = 20;
		break;

	case kOpV1_2_3SetWallpaper:
		warning("o_setWallpaper UNIMPLEMENTED");
		size = 8;
		break;

	case kOpV1_2_3SetMute:
		audioLib()->audioMute((int8)ip[2]);
		size = 4;
		break;

	case kOpV1_2_3BeginTextEdit:
		result = o_beginTextEdit(ip);
		size = 12;
		break;

	case kOpV1_2_3GetSpriteRect:
		o_getSpriteRect(ip);
		size = 12;
		break;

	case kOpV1_2_3FormatString:
		o_formatString(ip);
		size = 32;
		break;

	case kOpV1_2_3CompareStrings:
	case kOpV1_2_3CompareStringsIgnoreCase:
		o_compareStrings(ip);
		size = 8;
		break;

	case kOpV1_2_3IniRead:
	case kOpV1_2_3IniWrite:
		o_iniAccess(ip);
		size = 10;
		break;

	case kOpV1_2_3SetHoverRegion:
		o_setHoverRegion(ip);
		size = 16;

		if (_version >= kVersionV1_2_6)
			size = TG_SCRIPT_V1_2_6_SET_HOVER_REGION_SIZE;

		break;

	case kOpV1_2_3GetIntersect:
		o_getIntersect(ip);
		size = 22;
		break;

	case kOpV1_2_3SetSpritePosition:
		result = o_setSpritePosition(ip);
		size = 8;
		break;

	case kOpV1_2_3SetMouseEventScript:
		o_setMouseEventScript(ip);
		size = 4;
		break;

	case kOpV1_2_3Cosine:
	case kOpV1_2_3Sine:
		o_sineCosine(ip);
		size = 6;
		break;

	case kOpV1_2_3FlushIni:
		o_flushIni(ip);
		size = 4;
		break;

	case kOpV1_2_3IsSpriteBusy:
		o_isSpriteBusy(ip);
		size = 6;
		break;

	case kOpV1_2_3BackupIni:
		o_backupIni();
		size = 2;
		break;

	case kOpV1_2_3GetTime:
		o_getTime(ip);
		size = 10;
		break;

	case kOpV1_2_3SpriteCompositeToBackground:
		o_spriteCompositeToBackground(ip);
		size = 6;
		break;

	case kOpV1_2_3SetJoystickScript:
		o_setJoystickScript(ip);
		size = 6;
		break;

	case kOpV1_2_3SetCursorPosition:
		o_setCursorPosition(ip);
		size = 6;
		break;

	case kOpV1_2_3GetSpriteElementIds:
		o_getSpriteElementIds(ip);
		size = 12;

		if (_version >= kVersionV1_2_6)
			size = TG_SCRIPT_V1_2_6_GET_ELEMENT_IDS_SIZE;

		break;

	case kOpV1_2_3Expression:
		evaluateExpression(ip + 4);
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV1_2_3ConditionalJumpExpression:
		size = o_conditionalJumpExpr(ip);
		break;

	case kOpV1_2_3SwitchExpression:
		size = o_switchExpression(ip);
		break;

	case kOpV1_2_3ConditionalJumpExpressionCopy:
		size = o_conditionalJumpExprCopy(ip);
		break;

	case kOpV1_2_3PlayMidi:
		o_playMidi(ip);
		size = 10;
		break;

	case kOpV1_2_3StopMidi:
		o_stopMidi(ip);
		size = 4;
		break;

	case kOpV1_2_3SetDeviceGroupBinding:
		o_setDeviceGroupBinding(ip);
		size = 16;
		break;

	case kOpV1_2_3DeviceGroupButtonState:
		o_deviceGroupButtonState(ip);
		size = 10;
		break;

	case kOpV1_2_3SetDeviceGroupScript:
		o_setDeviceGroupScript(ip);
		size = 6;
		break;

	case kOpV1_2_3DeviceGroupLatch:
		o_deviceGroupLatch(ip);
		size = 8;
		break;

	case kOpV1_2_3SetDeviceGroupBindingB:
		o_setDeviceGroupBindingB(ip);
		size = 16;
		break;

	case kOpV1_2_3SetDeviceGroupFlag:
		o_setDeviceGroupFlag(ip);
		size = 6;
		break;

	case kOpV1_2_3GetDeviceGroupState:
		o_getDeviceGroupState(ip);
		size = 10;
		break;

	case kOpV1_2_3ReturnExpression:
		size = 0;
		_scriptResult = evaluateExpression(ip + 4);
		break;

	case kOpV1_2_3SetMidiParams:
		o_setMidiParams(ip);
		size = 14;
		break;

	case kOpV1_2_3SetVolume:
		o_setVolume(ip);
		size = 4;
		break;

	case kOpV1_2_3SetTextParam:
		o_setTextParam(ip);
		size = 6;
		break;

	case kOpV1_2_3LoadPaletteResource:
		o_loadPaletteResource(ip);
		size = 4;
		break;

	case kOpV1_2_3SetPaletteEntries:
		o_setPaletteEntries(ip);
		size = 12;
		break;

	case kOpV1_2_3SetDebugScriptId:
		_debugScriptId = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2_3SetQuitSceneName:
		if (_version >= kVersionV1_2_5) {
			Common::strlcpy(_quitSceneName, (const char *)ip + 2, TG_HOST_V1_2_5_SCENE_NAME_SIZE);
			size = TG_SCRIPT_V1_2_5_QUIT_SCENE_RECORD_SIZE;
		} else {
			Common::strlcpy(_quitSceneName, (const char *)ip + 2, TG_HOST_V1_2_3_SCENE_NAME_SIZE);
			size = 16;
		}

		break;

	default:
		if (_version >= kVersionV1_2_5 && READ_LE_UINT16(ip) == kOpV1_2_5SetReservedWord) {
			_reservedScriptWord = READ_LE_UINT16(ip + 2);
			size = TG_SCRIPT_V1_2_5_RESERVED_RECORD_SIZE;
		} else if (_version >= kVersionV1_2_5 && READ_LE_UINT16(ip) == kOpV1_2_5Reserved) {
			size = TG_SCRIPT_V1_2_5_RESERVED_RECORD_SIZE;
		} else {
			warning("TopgunEngine_v1_2_3::executeScriptOp(): unknown opcode %d", READ_LE_UINT16(ip));

			if (_version >= kVersionV1_2_5)
				fatalError(115, "Script command failed.");

			result = false;
			size = 0;
		}

		break;
	}

	if (advance)
		*advance = size;

	return result && _fatalCode == 0;
}

uint16 TopgunEngine_v1_2_3::getScriptOpSize(const byte *ip) {
	uint16 opcode = READ_LE_UINT16(ip);
	uint16 lastOpcode = TG_SCRIPT_V1_2_3_LAST_OPCODE;

	if (_version >= kVersionV1_2_5)
		lastOpcode = TG_SCRIPT_V1_2_5_LAST_OPCODE;

	if ((uint16)(opcode - 1) > lastOpcode - 1)
		return 0;

	if (opcode == kOpV1_2_3CallScript)
		return (ip[5] + 3) * 2;

	if (opcode == kOpV1_2_3Expression)
		return READ_LE_UINT16(ip + 2);

	if (opcode == kOpV1_2_3ConditionalJumpExpression)
		return READ_LE_UINT16(ip + 4);

	if (_version >= kVersionV1_2_5) {
		if (opcode == kOpV1_2_3GotoScene)
			return TG_SCRIPT_V1_2_5_GOTO_RECORD_SIZE;

		if (opcode == kOpV1_2_3SetQuitSceneName)
			return TG_SCRIPT_V1_2_5_QUIT_SCENE_RECORD_SIZE;

		if (opcode == kOpV1_2_5SetReservedWord || opcode == kOpV1_2_5Reserved)
			return TG_SCRIPT_V1_2_5_RESERVED_RECORD_SIZE;

		if (opcode > TG_SCRIPT_V1_2_3_LAST_OPCODE)
			return 0;
	}

	if (_version >= kVersionV1_2_6) {
		if (opcode == kOpV1_2_3SetClickScript)
			return TG_SCRIPT_V1_2_6_SET_CLICK_SCRIPT_SIZE;

		if (opcode == kOpV1_2_3SetBackground)
			return TG_SCRIPT_V1_2_6_SET_BACKGROUND_SIZE;

		if (opcode == kOpV1_2_3SetHoverRegion)
			return TG_SCRIPT_V1_2_6_SET_HOVER_REGION_SIZE;

		if (opcode == kOpV1_2_3GetSpriteElementIds)
			return TG_SCRIPT_V1_2_6_GET_ELEMENT_IDS_SIZE;
	}

	return kScriptOpSizes[opcode];
}

bool TopgunEngine_v1_2_3::runPostedScriptRecord(const byte *record) {
	byte local[TG_SCRIPT_V1_2_3_POSTED_RECORD_BUFFER_SIZE];
	uint16 count = READ_LE_UINT16(record + 4);

	WRITE_LE_UINT16(local + 2, READ_LE_UINT16(record + 2));
	local[4] = 0;
	local[5] = (byte)count;

	if (TG_SCRIPT_V1_2_3_CALL_HEADER_SIZE + 2 * (uint32)count > sizeof(local)) {
		warning("TopgunEngine_v1_2_3::runPostedScriptRecord(): %d arguments do not fit in the record", count);
		count = (sizeof(local) - TG_SCRIPT_V1_2_3_CALL_HEADER_SIZE) / 2;
		local[5] = (byte)count;
	}

	for (uint16 i = 0; i < count; i++)
		WRITE_LE_UINT16(local + TG_SCRIPT_V1_2_3_CALL_HEADER_SIZE + 2 * i, READ_LE_UINT16(record + TG_SCRIPT_V1_2_3_CALL_HEADER_SIZE + 2 * i));

	return o_callScript(local);
}

bool TopgunEngine_v1_2_3::enqueuePostedScriptRun(uint16 script, uint16 argument1, uint16 argument2) {
	if (_numPostedScriptRuns >= TG_SCRIPT_V1_2_3_MAX_POSTED_RUNS)
		return fatalError(112, "Too many 'run' messages attempted.");

	byte *record = _postedRuns + TG_SCRIPT_V1_2_3_POSTED_RUN_SIZE * _numPostedScriptRuns;

	memset(record, 0, TG_SCRIPT_V1_2_3_POSTED_RUN_SIZE);
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

void TopgunEngine_v1_2_3::servicePostedRunQueue() {
	uint16 index = 0;

	if (_numPostedScriptRuns != 0) {
		do {
			o_callScript(_postedRuns + TG_SCRIPT_V1_2_3_POSTED_RUN_SIZE * index);
			index++;
		} while (index < _numPostedScriptRuns);
	}

	_numPostedScriptRuns = 0;
}

void TopgunEngine_v1_2_3::o_setBackground(const byte *record) {
	if (READ_LE_UINT16(record + 10) != 0) {
		if (setBackgroundWithWipeEffect(record))
			return;
	}

	setBackgroundByMode(record);
}

void TopgunEngine_v1_2_3::o_gotoScene(const byte *record) {
	uint16 releaseOffset = TG_SCRIPT_V1_2_3_GOTO_RELEASE_OFFSET;
	uint16 backOffset = TG_SCRIPT_V1_2_3_GOTO_BACK_OFFSET;
	uint16 nameSize = TG_HOST_V1_2_3_SCENE_NAME_SIZE;

	if (_version >= kVersionV1_2_5) {
		releaseOffset = TG_SCRIPT_V1_2_5_GOTO_RELEASE_OFFSET;
		backOffset = TG_SCRIPT_V1_2_5_GOTO_BACK_OFFSET;
		nameSize = TG_HOST_V1_2_5_SCENE_NAME_SIZE;
	}

	if (record[backOffset] != 0 && _previousSceneIndex == _currentSceneIndex) {
		postMessage(kMsgClose, 1, 0);
		return;
	}

	memcpy(_gotoSceneName, record + TG_SCRIPT_V1_2_3_GOTO_NAME_OFFSET, nameSize);

	if (record[backOffset] != 0) {
		if (_previousSceneIndex < _sceneEntries.size())
			memcpy(_gotoSceneName, _sceneEntries[_previousSceneIndex].name, nameSize);
		else
			warning("TopgunEngine_v1_2_3::o_gotoScene(): scene index %d is out of range", _previousSceneIndex);
	}

	if (record[releaseOffset] != 0)
		destroySceneMainObjects(_releaseOnPalette);

	audioLib()->audioClearNotifications();
	postMessage(kMsgV1_2GotoScene, 0, 0);

	for (uint16 i = 0; i < TG_INPUT_V1_2_3_KEY_TABLE_SIZE; i++)
		_keyTable[i] = KeyEntry_v1_2();

	if (_version >= kVersionV1_2_6)
		_activationScript = 0;

	_dragSpriteId = 0;
}

} // End of namespace Topgun
