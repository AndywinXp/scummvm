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

bool TopgunEngine_v2_0::executeScriptOp(byte *ip, uint16 *advance) {
	bool result = true;
	uint16 size = 0;

	switch (READ_LE_UINT16(ip)) {
	case kOpV2_0CallScript:
		result = o_callScript(ip);
		size = (ip[TG_SCRIPT_V2_0_CALL_COUNT] + 4) * 2;
		break;

	case kOpV2_0Nop:
		size = 2;
		break;

	case kOpV2_0SetActivationScript:
		o2_setActivationScript(ip);
		size = 6;
		break;

	case kOpV2_0GetAngle:
		o2_getAngle(ip);
		size = 14;
		break;

	case kOpV2_0StopSprite:
		o2_stopSprite(ip);
		size = 6;
		break;

	case kOpV2_0SetHoverRegion:
		o2_setHoverRegion(ip);
		size = 22;
		break;

	case kOpV2_0SetClickScript:
		result = o2_setClickScript(ip);
		size = 18;
		break;

	case kOpV2_0SetSpriteClipBox:
		o_setSpriteRect(ip);
		size = 12;
		break;

	case kOpV2_0SpriteCompositeToBackground:
		o2_spriteCompositeToBackground(ip);
		size = 6;
		break;

	case kOpV2_0AddCollisionPair:
		result = o2_addCollisionPair(ip);
		size = 18;
		break;

	case kOpV2_0Cosine:
	case kOpV2_0Sine:
		o2_sineCosine(ip);
		size = 8;
		break;

	case kOpV2_0CursorMode:
		o_setCursorMode(ip);
		size = 4;
		break;

	case kOpV2_0SetCursorPosition:
		o2_setCursorPosition(ip);
		size = 8;
		break;

	case kOpV2_0DebugString:
		if (_debugMode)
			o2_debugString(ip);

		size = 8;
		break;

	case kOpV2_0FlushIni:
		o_flushIni(ip);
		size = 4;
		break;

	case kOpV2_0SetSpriteObjectProperties:
		result = o2_setSpriteObjectProperties(ip);
		size = 10;
		break;

	case kOpV2_0BeginTextEdit:
		result = o2_beginTextEdit(ip);
		size = 12;
		break;

	case kOpV2_0GotoScene:
		o_gotoScene(ip);
		size = TG_SCRIPT_V1_2_6_GOTO_RECORD_SIZE;
		break;

	case kOpV2_0Quit:
		if (_quitSceneName[0] != 0) {
			byte record[TG_SCRIPT_V1_2_6_GOTO_RECORD_SIZE];

			memset(record, 0, sizeof(record));
			WRITE_LE_UINT16(record, kOpV2_0GotoScene);
			record[TG_SCRIPT_V1_2_6_GOTO_RELEASE_OFFSET] = 1;
			Common::strlcpy((char *)record + TG_SCRIPT_V1_2_4_GOTO_NAME_OFFSET, _quitSceneName, TG_HOST_V1_2_6_SCENE_NAME_SIZE);
			o_gotoScene(record);
		} else {
			postMessage(kMsgClose, 1, 0);
		}

		size = 2;
		break;

	case kOpV2_0SetQuitSceneName:
		Common::strlcpy(_quitSceneName, (const char *)ip + 2, TG_HOST_V1_2_6_SCENE_NAME_SIZE);
		size = TG_SCRIPT_V1_2_6_QUIT_SCENE_RECORD_SIZE;
		break;

	case kOpV2_0FadeIn:
	case kOpV2_0FadeOut:
		o_fadeInOut(ip);
		size = 4;
		break;

	case kOpV2_0SetDebugScriptId:
		_debugScriptId = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV2_0GetSpriteRect:
		o2_getSpriteRect(ip);
		size = 14;
		break;

	case kOpV2_0GetSpriteElementIds:
		o2_getSpriteElementIds(ip);
		size = 22;
		break;

	case kOpV2_0GetDate:
		o_getDate(ip);
		size = 10;
		break;

	case kOpV2_0GetIntersect:
		o2_getIntersect(ip);
		size = 24;
		break;

	case kOpV2_0GetSpritePosition:
		o2_getSpritePosition(ip);
		size = 10;
		break;

	case kOpV2_0GetTime:
		o_getTime(ip);
		size = 10;
		break;

	case kOpV2_0ConditionalRunScript:
		result = o2_conditionalRunScript(ip, size);
		break;

	case kOpV2_0ConditionalSkip:
		size = o2_conditionalSkip(ip);
		break;

	case kOpV2_0ConditionalJumpExpression:
		size = o_conditionalJumpExpr(ip);
		break;

	case kOpV2_0RunScriptIfLoaded:
		size = 8;

		if (!o2_runScriptIfLoaded(ip))
			size = 0;

		break;

	case kOpV2_0SetDeviceGroupBinding:
		o2_setDeviceGroupBinding(ip);
		size = 16;
		break;

	case kOpV2_0DeviceGroupButtonState:
		o2_deviceGroupButtonState(ip);
		size = 10;
		break;

	case kOpV2_0SetDeviceGroupScript:
		o2_setDeviceGroupScript(ip);
		size = 8;
		break;

	case kOpV2_0GetDeviceGroupState:
		o2_getDeviceGroupState(ip);
		size = 12;
		break;

	case kOpV2_0SetJoystickScript:
		o2_setJoystickScript(ip);
		size = 8;
		break;

	case kOpV2_0Jump:
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV2_0GetKeyState:
		o2_getKeyState(ip);
		size = 8;
		break;

	case kOpV2_0BindKeyScript:
		o2_bindKeyScript(ip);
		size = 8;
		break;

	case kOpV2_0CancelScriptTimer:
		o_cancelScriptTimer(ip);
		size = 6;
		break;

	case kOpV2_0SetSpriteLevel:
		o2_setSpriteLevel(ip);
		size = 8;
		break;

	case kOpV2_0SetReservedWord:
		_reservedScriptWord = READ_LE_UINT16(ip + 2);
		size = 6;
		break;

	case kOpV2_0SetSceneEndScript:
		o2_setSceneEndScript(ip);
		size = 6;
		break;

	case kOpV2_0LoadResource:
	case kOpV2_0UnloadResource:
		result = o2_loadUnloadResource(ip);
		size = 6;
		break;

	case kOpV2_0SetMouseEventScript:
		o2_setMouseEventScript(ip);
		size = 6;
		break;

	case kOpV2_0SetMute:
		audioLib()->audioMute((int8)ip[2]);
		size = 4;
		break;

	case kOpV2_0MoveSprite:
		o2_moveSprite(ip);
		size = 10;
		break;

	case kOpV2_0SetPauseScript:
		o2_setPauseScript(ip);
		size = 6;
		break;

	case kOpV2_0PlayCDTrack:
		o_playCDTrack(ip[2]);
		size = 4;
		break;

	case kOpV2_0PlayMidi:
		o2_playMidi(ip);
		size = 12;
		break;

	case kOpV2_0PlayWave:
		result = o2_playWave(ip);
		size = 28;
		break;

	case kOpV2_0PostScript:
		o2_postScript(ip);
		size = 10;
		break;

	case kOpV2_0Random:
		o2_random(ip);
		size = 10;
		break;

	case kOpV2_0RandomRunScript:
		result = o2_randomRunScript(ip);
		size = 18;
		break;

	case kOpV2_0IniRead:
	case kOpV2_0IniWrite:
		o_iniAccess(ip);
		size = 10;
		break;

	case kOpV2_0ReturnExpression:
		size = 0;
		_scriptResult = evaluateExpression(ip + 4);
		break;

	case kOpV2_0Stop:
		size = 0;
		break;

	case kOpV2_0BackupIni:
		o_backupIni();
		size = 2;
		break;

	case kOpV2_0SetBackground:
		o2_setBackground(ip);
		size = 16;
		break;

	case kOpV2_0RoundRobinRunScript:
		result = o2_roundRobinRunScript(ip);
		size = 18;
		break;

	case kOpV2_0Arithmetic:
		o2_arithmetic(ip);
		size = 24;
		break;

	case kOpV2_0Expression:
		evaluateExpression(ip + 4);
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV2_0SetPaletteEntries:
		o2_setPaletteEntries(ip);
		size = 12;
		break;

	case kOpV2_0LoadPaletteResource:
		o2_loadPaletteResource(ip);
		size = 6;
		break;

	case kOpV2_0SetWallpaper:
		warning("o_setWallpaper UNIMPLEMENTED");
		size = 8;
		break;

	case kOpV2_0SetSpritePosition:
		result = o2_setSpritePosition(ip);
		size = 10;
		break;

	case kOpV2_0SetSpriteQueue:
		result = o2_setSpriteQueue(ip);
		size = 8;
		break;

	case kOpV2_0FormatString:
		o2_formatString(ip);
		size = 32;
		break;

	case kOpV2_0FormatTextObject:
		result = o2_formatTextObject(ip);
		size = 10;
		break;

	case kOpV2_0SetTextParam:
		o2_setTextParam(ip);
		size = 8;
		break;

	case kOpV2_0SetScriptTimer:
		o2_setScriptTimer(ip);
		size = 12;
		break;

	case kOpV2_0SetVolume:
		o_setVolume(ip);
		size = 4;
		break;

	case kOpV2_0StopCD:
		o_stopCD();
		size = 4;
		break;

	case kOpV2_0StopMidi:
		o2_stopMidi(ip);
		size = 6;
		break;

	case kOpV2_0StopWave:
		o2_stopWave(ip);
		size = 10;
		break;

	case kOpV2_0CompareStrings:
	case kOpV2_0CompareStringsIgnoreCase:
		o_compareStrings(ip);
		size = 8;
		break;

	case kOpV2_0ListRunScript:
		o2_listRunScript(ip);
		size = 10;
		break;

	case kOpV2_0Switch:
		size = lookupSwitchOffset(ip);
		break;

	case kOpV2_0SwitchExpression:
		size = o_switchExpression(ip);
		break;

	case kOpV2_0TransferSprite:
		o2_transferSprite(ip);
		size = 10;
		break;

	case kOpV2_0ClipLineToRect:
		o2_clipLineToRect(ip);
		size = 22;
		break;

	case kOpV2_0IsSpriteBusy:
		o2_isSpriteBusy(ip);
		size = 8;
		break;

	case kOpV2_0ConditionalJumpExpressionCopy:
		size = o_conditionalJumpExprCopy(ip);
		break;

	case kOpV2_0SetDisplayZoom:
		o2_setDisplayZoom(ip);
		size = 10;
		break;

	default:
		warning("TopgunEngine_v2_0::executeScriptOp(): unknown opcode %d", READ_LE_UINT16(ip));
		fatalError(115, "Script command failed.");
		result = false;
		size = 0;
		break;
	}

	if (advance)
		*advance = size;

	return result && _fatalCode == 0;
}

uint16 TopgunEngine_v2_0::getScriptOpSize(const byte *ip) {
	switch (READ_LE_UINT16(ip)) {
	case kOpV2_0CallScript:
		return (ip[TG_SCRIPT_V2_0_CALL_COUNT] + 4) * 2;

	case kOpV2_0Nop:
	case kOpV2_0Quit:
	case kOpV2_0Stop:
	case kOpV2_0BackupIni:
		return 2;

	case kOpV2_0CursorMode:
	case kOpV2_0FlushIni:
	case kOpV2_0FadeIn:
	case kOpV2_0FadeOut:
	case kOpV2_0SetDebugScriptId:
	case kOpV2_0Jump:
	case kOpV2_0SetMute:
	case kOpV2_0PlayCDTrack:
	case kOpV2_0SetVolume:
	case kOpV2_0StopCD:
		return 4;

	case kOpV2_0SetActivationScript:
	case kOpV2_0StopSprite:
	case kOpV2_0SpriteCompositeToBackground:
	case kOpV2_0CancelScriptTimer:
	case kOpV2_0SetReservedWord:
	case kOpV2_0SetSceneEndScript:
	case kOpV2_0LoadResource:
	case kOpV2_0SetMouseEventScript:
	case kOpV2_0SetPauseScript:
	case kOpV2_0LoadPaletteResource:
	case kOpV2_0StopMidi:
	case kOpV2_0UnloadResource:
		return 6;

	case kOpV2_0Cosine:
	case kOpV2_0SetCursorPosition:
	case kOpV2_0DebugString:
	case kOpV2_0RunScriptIfLoaded:
	case kOpV2_0SetDeviceGroupScript:
	case kOpV2_0SetJoystickScript:
	case kOpV2_0GetKeyState:
	case kOpV2_0BindKeyScript:
	case kOpV2_0SetSpriteLevel:
	case kOpV2_0SetWallpaper:
	case kOpV2_0SetSpriteQueue:
	case kOpV2_0SetTextParam:
	case kOpV2_0Sine:
	case kOpV2_0CompareStrings:
	case kOpV2_0CompareStringsIgnoreCase:
	case kOpV2_0IsSpriteBusy:
		return 8;

	case kOpV2_0SetSpriteObjectProperties:
	case kOpV2_0GetDate:
	case kOpV2_0GetSpritePosition:
	case kOpV2_0GetTime:
	case kOpV2_0DeviceGroupButtonState:
	case kOpV2_0MoveSprite:
	case kOpV2_0PostScript:
	case kOpV2_0Random:
	case kOpV2_0IniRead:
	case kOpV2_0SetSpritePosition:
	case kOpV2_0FormatTextObject:
	case kOpV2_0StopWave:
	case kOpV2_0ListRunScript:
	case kOpV2_0TransferSprite:
	case kOpV2_0IniWrite:
	case kOpV2_0SetDisplayZoom:
		return 10;

	case kOpV2_0SetSpriteClipBox:
	case kOpV2_0BeginTextEdit:
	case kOpV2_0GetDeviceGroupState:
	case kOpV2_0PlayMidi:
	case kOpV2_0SetPaletteEntries:
	case kOpV2_0SetScriptTimer:
		return 12;

	case kOpV2_0GetAngle:
	case kOpV2_0GetSpriteRect:
	case kOpV2_0ConditionalRunScript:
		return 14;

	case kOpV2_0SetDeviceGroupBinding:
	case kOpV2_0SetBackground:
		return 16;

	case kOpV2_0SetClickScript:
	case kOpV2_0AddCollisionPair:
	case kOpV2_0SetQuitSceneName:
	case kOpV2_0RandomRunScript:
	case kOpV2_0RoundRobinRunScript:
		return 18;

	case kOpV2_0GotoScene:
		return TG_SCRIPT_V1_2_6_GOTO_RECORD_SIZE;

	case kOpV2_0SetHoverRegion:
	case kOpV2_0GetSpriteElementIds:
	case kOpV2_0ClipLineToRect:
		return 22;

	case kOpV2_0GetIntersect:
	case kOpV2_0Arithmetic:
		return 24;

	case kOpV2_0PlayWave:
		return 28;

	case kOpV2_0FormatString:
		return 32;

	case kOpV2_0ConditionalJumpExpression:
		return READ_LE_UINT16(ip + 4);

	case kOpV2_0Expression:
		return READ_LE_UINT16(ip + 2);

	default:
		return 0;
	}
}

bool TopgunEngine_v2_0::callScriptWithArguments(const byte *record) {
	byte frameSlots = record[TG_SCRIPT_V2_0_CALL_FRAME_SLOTS];
	byte count = record[TG_SCRIPT_V2_0_CALL_COUNT];
	int32 destination = (int32)_argumentFrame - frameSlots;

	for (byte i = 0; i < count; i++) {
		if (destination - i < 0) {
			warning("TopgunEngine_v2_0::callScriptWithArguments(): argument slot %d is out of range", i);
			break;
		}

		_variableTable[destination - i] = READ_LE_UINT16(record + TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2 * i);
	}

	_argumentFrame -= frameSlots;
	bool result = runScriptResource(READ_LE_UINT16(record + 2));
	_argumentFrame += frameSlots;

	return result;
}

uint16 TopgunEngine_v2_0::resolveScriptOperand(uint16 value, byte flag) const {
	if (flag == 0)
		return value;

	return readVariable(value);
}

void TopgunEngine_v2_0::resolveScriptRect(byte *rect, byte flags) const {
	for (uint16 i = 0; i < 4; i++)
		WRITE_LE_UINT16(rect + 2 * i, resolveScriptOperand(READ_LE_UINT16(rect + 2 * i), flags & (1 << i)));

	int16 left = (int16)READ_LE_UINT16(rect);
	int16 top = (int16)READ_LE_UINT16(rect + 2);
	int16 right = (int16)READ_LE_UINT16(rect + 4);
	int16 bottom = (int16)READ_LE_UINT16(rect + 6);

	if (right < left) {
		WRITE_LE_UINT16(rect, (uint16)right);
		WRITE_LE_UINT16(rect + 4, (uint16)left);
	}

	if (top > bottom) {
		WRITE_LE_UINT16(rect + 2, (uint16)bottom);
		WRITE_LE_UINT16(rect + 6, (uint16)top);
	}
}

void TopgunEngine_v2_0::resolveRecordOperandWord2(byte *record) const {
	WRITE_LE_UINT16(record + 2, resolveScriptOperand(READ_LE_UINT16(record + 2), record[7]));
}

} // End of namespace Topgun
