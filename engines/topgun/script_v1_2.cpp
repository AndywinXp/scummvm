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
	0, 0, 4, 4, 17, 7, 10, 9, 26, 4,
	4, 4, 10, 2, 4, 8, 18, 12, 17, 12,
	4, 4, 17, 17, 12, 4, 6, 6, 6, 4,
	4, 3, 6, 13, 10, 5, 8, 4, 0, 8,
	8, 8, 10, 0, 2, 2, 8, 6, 10, 12,
	20, 0, 7, 3, 11, 12, 26, 8, 8, 10,
	10, 14, 22, 8, 4, 6, 6, 4, 6, 2,
	10, 4, 6, 6
};

bool TopgunEngine_v1_2::runScriptResource(uint16 scriptId) {
	if (_version >= kVersionV1_2_3) {
		if (static_cast<TopgunEngine_v1_2_3 *>(this)->_fatalCode != 0)
			return false;
	} else if (_scriptsDisabled) {
		return false;
	}

	uint16 scriptType = kResV1_2Script;
	if (_version >= kVersionV1_2_7)
		scriptType = kResV1_2_7Script;
	else if (_version >= kVersionV1_2_5)
		scriptType = kResV1_2_5Script;
	else if (_version >= kVersionV1_2_3)
		scriptType = kResV1_2_3Script;
	else if (_version >= kVersionV1_2_2)
		scriptType = kResV1_2_2Script;
	if (!validateAndGetResourceByType(scriptId, scriptType))
		return false;

	_scriptDepth++;

	if (_scriptDepth >= ((_version >= kVersionV1_2_3) ? TG_SCRIPT_V1_2_3_MAX_DEPTH : TG_SCRIPT_V1_2_MAX_DEPTH)) {
		if (_version >= kVersionV1_2_1)
			return fatalError(106, "Stack overflow.");

		return fatalError("Stack overflow.");
	}

	if (_version >= kVersionV1_2_5)
		static_cast<TopgunEngine_v1_2_3 *>(this)->_debugScriptId = 0;

	_currentScriptId = scriptId;
	bool result = runScriptBytecodeLoop(scriptId);

	_scriptDepth--;
	return result;
}

bool TopgunEngine_v1_2::runScriptBytecodeLoop(uint16 scriptId) {
	uint16 index = operandValue(scriptId);
	if (!isRecordIndexValid(index, "runScriptBytecodeLoop"))
		return false;

	byte *start = _records[index].data;
	if (!start) {
		warning("TopgunEngine_v1_2::runScriptBytecodeLoop(): the script %d has no data", index);
		return false;
	}

	byte *ip = start;
	int16 remaining = (int16)(_records[index].size & 0xFFFF);
	bool result;

	// The first operation runs even when the script is empty
	while (true) {
		uint16 advance = 0;
		result = executeScriptOp(ip, &advance);
		if (!result)
			break;

		if (advance == 0)
			break;

		remaining = (int16)(remaining - advance);
		if (remaining <= 0)
			break;

		ip += (int16)advance;

		if (ip < start || ip >= start + _records[index].size) {
			warning("TopgunEngine_v1_2::runScriptBytecodeLoop(): the script %d runs out of its data", index);
			return false;
		}
	}

	return result;
}

bool TopgunEngine_v1_2::executeScriptOp(byte *ip, uint16 *advance) {
	bool result = true;
	uint16 size = 0;
	uint16 opcode = READ_LE_UINT16(ip);

	// The operations after the last one of the oldest version are not recognised there
	if (_version < kVersionV1_2_1 && opcode >= kOpV1_2GetTime)
		opcode = 0;

	if (_version < kVersionV1_2_2 && opcode >= kOpV1_2GetSpriteElementIds)
		opcode = 0;

	switch (opcode) {
	case kOpV1_2CallScript:
		result = o_callScript(ip);

		if (_version >= kVersionV1_2_2)
			size = (ip[5] + 3) * 2;
		else
			size = TG_SCRIPT_V1_2_CALL_HEADER_SIZE + 2 * ip[4];

		break;

	case kOpV1_2LoadResource: {
		uint16 id = READ_LE_UINT16(ip + 2);

		result = validateAndGetResourceByType(id, resourceLib()->resGetType(_file, operandValue(id)));
		size = 4;
		break;
	}

	case kOpV1_2UnloadResource: {
		uint16 id = READ_LE_UINT16(ip + 2);

		result = destroyResourceObjectByType(id, resourceLib()->resGetType(_file, operandValue(id)));
		size = 4;
		break;
	}

	case kOpV1_2SetClickScript:
		result = o_setClickScript(ip);
		size = 17;
		break;

	case kOpV1_2SetSpriteQueue:
		result = o_setSpriteQueue(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4), ip[6] != 0);
		size = 7;
		break;

	case kOpV1_2SetBackground:
		o_setBackground(ip);
		size = 10;
		break;

	case kOpV1_2SetSpriteObjectProperties:
		result = o_setSpriteObjectProperties(ip);
		size = 9;
		break;

	case kOpV1_2PlayWave:
		result = o_playWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 26;
		break;

	case kOpV1_2PlayCDTrack:
		o_playCDTrack(ip[2]);
		size = 4;
		break;

	case kOpV1_2FadeIn:
	case kOpV1_2FadeOut:
		o_fadeInOut(ip);
		size = 4;
		break;

	case kOpV1_2BindKeyScript:
		o_bindKeyScript(ip);
		size = (_version >= kVersionV1_2_2) ? 11 : 10;
		break;

	case kOpV1_2Quit:
		postMessage(kMsgClose, 1, 0);
		size = 2;
		break;

	case kOpV1_2StopCD:
		o_stopCD();
		size = 4;
		break;

	case kOpV1_2StopWave:
		o_stopWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 8;
		break;

	case kOpV1_2Arithmetic:
		o_arithmetic(ip);
		size = 18;
		break;

	case kOpV1_2ConditionalRunScript:
		result = o_conditionalRunScript(ip, size);
		break;

	case kOpV1_2GotoScene:
		o_gotoScene(ip);
		size = 17;
		break;

	case kOpV1_2SetScriptTimer:
		o_setScriptTimer(ip);
		size = 12;
		break;

	case kOpV1_2CancelScriptTimer:
		o_cancelScriptTimer(ip);
		size = 4;
		break;

	case kOpV1_2SetSceneEndScript:
		if (_postSceneEndEnabled)
			postMessage(kMsgV1_2SceneEnd, 0, 0);

		_sceneEndScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2RandomRunScript:
		result = o_randomRunScript(ip);
		size = 17;
		break;

	case kOpV1_2RoundRobinRunScript:
		result = o_roundRobinRunScript(ip);
		size = 17;
		break;

	case kOpV1_2TransferSprite:
		o_transferSprite(ip);
		size = 12;
		break;

	case kOpV1_2StopSprite:
		o_stopSprite(ip);
		size = 4;
		break;

	case kOpV1_2SetSpriteQueueIfLoaded: {
		uint16 index = operandValue(READ_LE_UINT16(ip + 2));

		if (isRecordIndexValid(index, "executeScriptOp") && _records[index].handle != 0)
			result = o_setSpriteQueue(index, operandValue(READ_LE_UINT16(ip + 4)), false);

		size = 6;
		break;
	}

	case kOpV1_2DebugString:
		o_debugString(ip);
		size = 6;
		break;

	case kOpV1_2SetSpriteLevel:
		o_setSpriteLevel(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
		size = 6;
		break;

	case kOpV1_2SetActivationScript:
		_activationScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpV1_2PostScript:
		postMessage(kMsgV1_2RunScript, READ_LE_UINT16(ip + 2), 0);
		size = 4;
		break;

	case kOpV1_2CursorMode:
		setCursorMode(ip[2]);
		size = 3;
		break;

	case kOpV1_2RunScriptIfLoaded:
		size = 6;

		if (isResourceObjectLoaded(READ_LE_UINT16(ip + 2))) {
			if (READ_LE_UINT16(ip + 4) == 0)
				size = 0;
			else
				runScriptResource(READ_LE_UINT16(ip + 4));
		}
		break;

	case kOpV1_2AddCollisionPair:
		result = o_addCollisionPair(ip);
		size = 13;
		break;

	case kOpV1_2ListRunScript:
		o_listRunScript(ip);
		size = 10;
		break;

	case kOpV1_2LaunchProgram:
		if (_version >= kVersionV1_2_1) {
			_pauseScript = READ_LE_UINT16(ip + 2);
			size = 4;
			break;
		}

		if (ip[4] != 0)
			warning("o_launchProgram UNIMPLEMENTED");

		size = 5;
		break;

	case kOpV1_2MoveSprite:
		o_moveSprite(ip);
		size = 8;
		break;

	case kOpV1_2Jump:
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV1_2ConditionalSkip:
		size = o_conditionalSkip(ip);
		break;

	case kOpV1_2SetDisplayZoom:
		o_setDisplayZoom(ip);
		size = 8;
		break;

	case kOpV1_2GetSpriteInfo:
		o_getSpriteInfo(ip);
		size = 8;
		break;

	case kOpV1_2FormatTextObject:
		result = o_formatTextObject(ip);
		size = 8;
		break;

	case kOpV1_2SetSpriteRect:
		o_setSpriteRect(ip);
		size = 10;
		break;

	case kOpV1_2Switch:
		size = lookupSwitchOffset(ip);
		break;

	case kOpV1_2Stop:
		size = 0;
		break;

	case kOpV1_2Nop:
		size = 2;
		break;

	case kOpV1_2Random:
		o_random(ip);
		size = 8;
		break;

	case kOpV1_2GetKeyState:
		o_getKeyState(ip);
		size = 6;
		break;

	case kOpV1_2GetDate:
		o_getDate(ip);
		size = 10;
		break;

	case kOpV1_2GetAngle:
		o_getAngle(ip);
		size = 12;
		break;

	case kOpV1_2ClipLineToRect:
		o_clipLineToRect(ip);
		size = 20;
		break;

	case kOpV1_2SetWallpaper:
		warning("o_setWallpaper UNIMPLEMENTED");
		size = 7;
		break;

	case kOpV1_2SetMute:
		audioLib()->audioMute((int8)ip[2]);
		size = 3;
		break;

	case kOpV1_2BeginTextEdit:
		result = o_beginTextEdit(ip);
		size = 11;
		break;

	case kOpV1_2GetSpriteRect:
		o_getSpriteRect(ip);
		size = 12;
		break;

	case kOpV1_2FormatString:
		o_formatString(ip);
		size = 26;
		break;

	case kOpV1_2CompareStrings:
	case kOpV1_2CompareStringsIgnoreCase:
		o_compareStrings(ip);
		size = 8;
		break;

	case kOpV1_2IniRead:
	case kOpV1_2IniWrite:
		o_iniAccess(ip);
		size = 10;
		break;

	case kOpV1_2SetHoverRegion:
		o_setHoverRegion(ip);
		size = (_version >= kVersionV1_2_2) ? 15 : 14;
		break;

	case kOpV1_2GetIntersect:
		o_getIntersect(ip);
		size = 22;
		break;

	case kOpV1_2SetSpritePosition:
		result = o_setSpritePosition(ip);
		size = 8;
		break;

	case kOpV1_2SetMouseEventScript:
		_mouseEventScript = READ_LE_UINT16(ip + 2);
		// Monty Python takes the length left by the previous operation: the only occurrence ends its script, so 4 is equivalent
		size = 4;
		break;

	case kOpV1_2Cosine:
	case kOpV1_2Sine:
		o_sineCosine(ip);
		size = 6;
		break;

	case kOpV1_2FlushIni:
		o_flushIni(ip);
		size = 4;
		break;

	case kOpV1_2IsSpriteBusy:
		o_isSpriteBusy(ip);
		size = 6;
		break;

	case kOpV1_2BackupIni:
		o_backupIni();
		size = 2;
		break;

	case kOpV1_2GetTime:
		o_getTime(ip);
		size = 10;
		break;

	case kOpV1_2SpriteCompositeToBackground:
		o_spriteCompositeToBackground(ip);
		size = 4;
		break;

	case kOpV1_2SetJoystickScript:
		o_setJoystickScript(ip);
		size = 6;
		break;

	case kOpV1_2SetCursorPosition:
		o_setCursorPosition(ip);
		size = 6;
		break;

	case kOpV1_2GetSpriteElementIds:
		o_getSpriteElementIds(ip);
		size = 12;
		break;

	case kOpV1_2Expression:
		evaluateExpression(ip + 4);
		size = READ_LE_UINT16(ip + 2);
		break;

	case kOpV1_2ConditionalJumpExpression:
		size = o_conditionalJumpExpr(ip);
		break;

	case kOpV1_2SwitchExpression:
		size = o_switchExpression(ip);
		break;

	case kOpV1_2ConditionalJumpExpressionCopy:
		size = o_conditionalJumpExprCopy(ip);
		break;

	case kOpV1_2PlayMidi:
		o_playMidi(ip);
		size = 4;
		break;

	case kOpV1_2StopMidi:
		o_stopMidi(ip);
		size = 4;
		break;

	case kOpV1_2ReturnExpression:
		size = 0;
		_scriptResult = evaluateExpression(ip + 4);
		break;

	default:
		warning("TopgunEngine_v1_2::executeScriptOp(): unknown opcode %d", READ_LE_UINT16(ip));
		size = 0;
		break;
	}

	if (advance)
		*advance = size;

	return result && !_scriptsDisabled;
}

uint16 TopgunEngine_v1_2::getScriptOpSize(const byte *ip) {
	uint16 opcode = READ_LE_UINT16(ip);

	if (_version >= kVersionV1_2_2) {
		switch (opcode) {
		case kOpV1_2CallScript:
			return (ip[5] + 3) * 2;

		case kOpV1_2BindKeyScript:
			return 11;

		case kOpV1_2SetHoverRegion:
			return 15;

		case kOpV1_2GetSpriteElementIds:
			return 12;

		case kOpV1_2Expression:
			return READ_LE_UINT16(ip + 2);

		case kOpV1_2ConditionalJumpExpression:
			return READ_LE_UINT16(ip + 4);

		case kOpV1_2PlayMidi:
		case kOpV1_2StopMidi:
			return 4;

		case kOpV1_2SetDeviceGroupBinding:
		case kOpV1_2SetDeviceGroupBindingB:
			return 15;

		case kOpV1_2DeviceGroupButtonState:
			return 7;

		case kOpV1_2SetDeviceGroupScript:
		case kOpV1_2SetDeviceGroupFlag:
			return 6;

		case kOpV1_2DeviceGroupLatch:
			return 8;

		case kOpV1_2GetDeviceGroupState:
			return 10;

		case kOpV1_2SwitchExpression:
		case kOpV1_2ConditionalJumpExpressionCopy:
		case kOpV1_2ReturnExpression:
			return 0;

		default:
			break;
		}
	}

	if (opcode == kOpV1_2CallScript)
		return 5 + 2 * ip[4];

	if (opcode >= sizeof(kScriptOpSizes) || (_version < kVersionV1_2_1 && opcode >= kOpV1_2GetTime))
		return 0;

	if (_version >= kVersionV1_2_1 && opcode == kOpV1_2SetPauseScript)
		return 4;

	return kScriptOpSizes[opcode];
}

bool TopgunEngine_v1_2::pushScriptArguments(uint16 count, const byte *arguments) {
	if (_scriptArgCount != 0) {
		if (TG_SCRIPT_V1_2_ARGUMENT_STACK_VARIABLE + (uint32)_scriptArgStackOffset + _scriptArgCount > TG_HOST_V1_2_NUM_VARIABLES) {
			warning("TopgunEngine_v1_2::pushScriptArguments(): the argument stack is beyond the variables");
			return false;
		}

		memmove(&_variableTable[TG_SCRIPT_V1_2_ARGUMENT_STACK_VARIABLE + _scriptArgStackOffset], &_variableTable[TG_SCRIPT_V1_2_ARGUMENT_FRAME_VARIABLE], _scriptArgCount * sizeof(uint16));
		_scriptArgStackOffset += _scriptArgCount;

		if (_scriptArgStackOffset > TG_SCRIPT_V1_2_ARGUMENT_STACK_SIZE)
			return false;
	}

	_scriptArgCount = count;

	for (uint16 i = 0; i < count; i++)
		_variableTable[TG_SCRIPT_V1_2_ARGUMENT_FRAME_VARIABLE + i] = operandValue(READ_LE_UINT16(arguments + 2 * i));

	return true;
}

void TopgunEngine_v1_2::popScriptArguments() {
	if (_scriptArgStackOffset == 0)
		return;

	if (_scriptArgCount > _scriptArgStackOffset) {
		warning("TopgunEngine_v1_2::popScriptArguments(): the argument frame is bigger than the stack");
		return;
	}

	_scriptArgStackOffset -= _scriptArgCount;
	memmove(&_variableTable[TG_SCRIPT_V1_2_ARGUMENT_FRAME_VARIABLE], &_variableTable[TG_SCRIPT_V1_2_ARGUMENT_STACK_VARIABLE + _scriptArgStackOffset], _scriptArgCount * sizeof(uint16));
}

uint16 TopgunEngine_v1_2::countScriptListEntries(uint16 listId) {
	uint16 listType = kResV1_2OpList;
	if (_version >= kVersionV1_2_7)
		listType = kResV1_2_7OpList;
	else if (_version >= kVersionV1_2_5)
		listType = kResV1_2_5OpList;
	else if (_version >= kVersionV1_2_3)
		listType = kResV1_2_3OpList;
	else if (_version >= kVersionV1_2_2)
		listType = kResV1_2_2OpList;
	else if (_version >= kVersionV1_2_1)
		listType = kResV1_2_1OpList;
	if (!validateAndGetResourceByType(listId, listType))
		return 0;

	uint16 index = operandValue(listId);
	if (!isRecordIndexValid(index, "countScriptListEntries"))
		return 0;

	const byte *ip = _records[index].data;
	int16 remaining = (int16)(_records[index].size & 0xFFFF);
	uint16 count = 0;

	while (remaining > 0) {
		uint16 size = getScriptOpSize(ip);
		if (size == 0)
			return 0;

		ip += size;
		count++;
		remaining = (int16)(remaining - size);
	}

	return count;
}

bool TopgunEngine_v1_2::runScriptListEntry(uint16 listId, int16 index) {
	uint16 listType = kResV1_2OpList;
	if (_version >= kVersionV1_2_7)
		listType = kResV1_2_7OpList;
	else if (_version >= kVersionV1_2_5)
		listType = kResV1_2_5OpList;
	else if (_version >= kVersionV1_2_3)
		listType = kResV1_2_3OpList;
	else if (_version >= kVersionV1_2_2)
		listType = kResV1_2_2OpList;
	else if (_version >= kVersionV1_2_1)
		listType = kResV1_2_1OpList;
	if (!validateAndGetResourceByType(listId, listType))
		return false;

	uint16 recordIndex = operandValue(listId);
	if (!isRecordIndexValid(recordIndex, "runScriptListEntry"))
		return false;

	byte *ip = _records[recordIndex].data;
	int16 remaining = (int16)(_records[recordIndex].size & 0xFFFF);

	while (index > 0) {
		index--;

		if (remaining <= 0)
			return false;

		uint16 size = getScriptOpSize(ip);
		if (size == 0)
			return false;

		remaining = (int16)(remaining - size);
		ip += size;
	}

	if (remaining <= 0)
		return false;

	_currentScriptId = listId;
	_scriptDepth++;

	if (_scriptDepth >= ((_version >= kVersionV1_2_3) ? TG_SCRIPT_V1_2_3_MAX_DEPTH : TG_SCRIPT_V1_2_MAX_DEPTH)) {
		if (_version >= kVersionV1_2_1)
			return fatalError(106, "Stack overflow.");

		return fatalError("Stack overflow.");
	}

	bool result = executeScriptOp(ip, nullptr);

	_scriptDepth--;
	return result;
}

bool TopgunEngine_v1_2::evaluateCondition(byte condition, uint16 operandA, uint16 operandB) {
	bool hit;

	if ((condition & TG_SCRIPT_V1_2_CONDITION_EQUAL) != 0)
		hit = (operandB == operandA);
	else if ((condition & TG_SCRIPT_V1_2_CONDITION_LESS) != 0)
		hit = ((int16)operandB < (int16)operandA);
	else if ((condition & TG_SCRIPT_V1_2_CONDITION_GREATER) != 0)
		hit = ((int16)operandB > (int16)operandA);
	else if (_version >= kVersionV1_2_1 && (condition & TG_SCRIPT_V1_2_1_CONDITION_OR) != 0)
		hit = ((operandB | operandA) != 0);
	else if (_version >= kVersionV1_2_1 && (condition & TG_SCRIPT_V1_2_1_CONDITION_AND) != 0)
		hit = ((operandB & operandA) != 0);
	else if (_version >= kVersionV1_2_1 && (condition & TG_SCRIPT_V1_2_1_CONDITION_MODULO) != 0) {
		if (operandB == 0) {
			warning("TopgunEngine_v1_2::evaluateCondition(): division by zero");
			hit = false;
		} else {
			hit = ((int16)operandA % (int16)operandB) != 0;
		}
	} else
		hit = (operandA != 0);

	if ((condition & TG_SCRIPT_V1_2_CONDITION_NEGATE) != 0)
		hit = !hit;

	return hit;
}

int32 TopgunEngine_v1_2::divideSigned32(int32 dividend, int32 divisor) {
	if (divisor == 0) {
		warning("TopgunEngine_v1_2::divideSigned32(): division by zero");
		return 0;
	}

	bool negative = ((dividend < 0) != (divisor < 0));
	uint32 magnitudeDividend = (dividend < 0) ? (uint32)0 - (uint32)dividend : (uint32)dividend;
	uint32 magnitudeDivisor = (divisor < 0) ? (uint32)0 - (uint32)divisor : (uint32)divisor;
	uint32 quotient = magnitudeDividend / magnitudeDivisor;

	return (int32)(negative ? (uint32)0 - quotient : quotient);
}

int32 TopgunEngine_v1_2::applyArithmeticOperand(int32 accumulator, const byte *operand) {
	int32 value = (int16)operandValue(READ_LE_UINT16(operand));
	byte flags = operand[2];

	if ((flags & TG_SCRIPT_V1_2_ARITHMETIC_NEGATE_FLAG) != 0)
		value = (int32)((uint32)0 - (uint32)value);

	switch (flags & TG_SCRIPT_V1_2_ARITHMETIC_OPERATION_MASK) {
	case 0:
		return value;

	case 1:
		return (int32)((uint32)accumulator + (uint32)value);

	case 2:
		return (int32)((uint32)accumulator - (uint32)value);

	case 3:
		return (int32)((uint32)accumulator * (uint32)value);

	case 4: {
		if (value == 0)
			return 0;

		int32 half = divideSigned32(value, 2);
		if (value < 0)
			half = (int32)((uint32)0 - (uint32)half);

		return divideSigned32((int32)((uint32)accumulator + (uint32)half), value);
	}

	case 5:
		if (_version >= kVersionV1_2_1)
			return accumulator | value;

		return 0;

	case 6:
		if (_version >= kVersionV1_2_1)
			return accumulator & value;

		return 0;

	case 7:
		if (_version >= kVersionV1_2_1) {
			if (value == 0) {
				warning("TopgunEngine_v1_2::applyArithmeticOperand(): division by zero");
				return 0;
			}

			return (int32)((int64)accumulator % (int64)value);
		}

		return 0;

	default:
		return 0;
	}
}

uint16 TopgunEngine_v1_2::lookupSwitchOffset(const byte *record) {
	uint16 count = record[8];
	const byte *table = record + READ_LE_UINT16(record + 4);
	uint16 defaultAdvance = READ_LE_UINT16(record + 6);
	uint16 value = operandValue(READ_LE_UINT16(record + 2));
	uint16 entrySize = (_version >= kVersionV1_2_3) ? TG_SCRIPT_V1_2_3_SWITCH_ENTRY_SIZE : TG_SCRIPT_V1_2_2_SWITCH_ENTRY_SIZE;
	uint16 valueOffset = (_version >= kVersionV1_2_3) ? 2 : 0;
	uint16 index = 0;

	while (index < count) {
		if (READ_LE_UINT16(table + valueOffset + entrySize * index) == value)
			return READ_LE_UINT16(table + valueOffset + entrySize * index + 2);

		index++;
	}

	return defaultAdvance;
}

uint16 TopgunEngine_v1_2::o_switchExpression(const byte *record) {
	uint16 count = record[8];
	const byte *table = record + READ_LE_UINT16(record + 4);
	uint16 defaultAdvance = READ_LE_UINT16(record + 6);
	uint16 value = evaluateExpression(record + ((_version >= kVersionV1_2_3) ? 10 : 9));
	uint16 entrySize = (_version >= kVersionV1_2_3) ? TG_SCRIPT_V1_2_3_SWITCH_ENTRY_SIZE : TG_SCRIPT_V1_2_2_SWITCH_ENTRY_SIZE;
	uint16 valueOffset = (_version >= kVersionV1_2_3) ? 2 : 0;
	uint16 index = 0;

	while (index < count) {
		if (READ_LE_UINT16(table + valueOffset + entrySize * index) == value)
			return READ_LE_UINT16(table + valueOffset + entrySize * index + 2);

		index++;
	}

	return defaultAdvance;
}

uint16 TopgunEngine_v1_2::o_conditionalJumpExpr(const byte *record) {
	if (evaluateExpression(record + 6) != 0)
		return READ_LE_UINT16(record + 4);

	return READ_LE_UINT16(record + 2);
}

uint16 TopgunEngine_v1_2::o_conditionalJumpExprCopy(const byte *record) {
	if (evaluateExpression(record + 6) != 0)
		return READ_LE_UINT16(record + 4);

	return READ_LE_UINT16(record + 2);
}

void TopgunEngine_v1_2::spriteMessageService(byte *record) {
	if (_scriptsDisabled)
		return;

	switch (READ_LE_UINT16(record)) {
	case TG_SPR_V1_2_FRAME_NOTIFICATION:
		trimSpriteQueueBitmaps(READ_LE_UINT16(record + 2));
		break;

	case TG_SCRIPT_V1_2_HOST_EVENT_SCENE_END:
		postMessage(kMsgV1_2SceneEnd, 0, 0);
		break;

	case TG_SCRIPT_V1_2_HOST_EVENT_OPERATION:
		executeScriptOp(record + TG_SCRIPT_V1_2_HOST_EVENT_OPERATION_OFFSET, nullptr);
		break;

	default:
		break;
	}
}

} // End of namespace Topgun
