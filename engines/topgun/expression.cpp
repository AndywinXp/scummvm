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

#include "common/array.h"
#include "common/debug.h"
#include "common/rect.h"
#include "common/str.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_1.h"

namespace Topgun {

void TopgunEngine_v1_2_1::pushExpressionValue(uint16 value) {
	if (_expressionStackPointer >= TG_SCRIPT_V1_2_1_EXPRESSION_STACK_SIZE) {
		warning("TopgunEngine_v1_2_1::pushExpressionValue(): the expression stack is full");
		return;
	}

	_expressionStack[_expressionStackPointer] = value;
	_expressionStackPointer++;
}

uint16 TopgunEngine_v1_2_1::evaluateExpression(const byte *expression) {
	uint16 base = _expressionStackPointer;
	const byte *token = expression;

	while (*token != 0) {
		byte code = *token;
		token++;

		switch (code) {
		case 1:
			pushExpressionValue(READ_LE_UINT16(token));
			token += 2;
			break;

		case 2:
			pushExpressionValue(_variableTable[variableIndex(READ_LE_UINT16(token))]);
			token += 2;
			break;

		case 3:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = readAddress((uint16)(_expressionStack[_expressionStackPointer - 1] + 2 * _expressionStack[_expressionStackPointer]));
			break;

		case 4:
			pushExpressionValue(operandAddress(READ_LE_UINT16(token)));
			token += 2;
			break;

		case 5:
			_expressionStack[_expressionStackPointer - 1] = readAddress(_expressionStack[_expressionStackPointer - 1]);
			break;

		case 6:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (uint16)(_expressionStack[_expressionStackPointer] * 2 + _expressionStack[_expressionStackPointer - 1]);
			break;

		case 8:
			_expressionStackPointer--;
			writeAddress(_expressionStack[_expressionStackPointer - 1], _expressionStack[_expressionStackPointer]);
			_expressionStack[_expressionStackPointer - 1] = _expressionStack[_expressionStackPointer];
			break;

		case 12:
			exprCallBuiltinFunction(token);
			token += 4;
			break;

		case 13:
			exprCallScript(token);
			token += 4;
			break;

		case 14:
			_expressionStack[_expressionStackPointer - 1] = (uint16)-(int16)_expressionStack[_expressionStackPointer - 1];
			break;

		case 15:
			_expressionStack[_expressionStackPointer - 1] = (_expressionStack[_expressionStackPointer - 1] == 0) ? 1 : 0;
			break;

		case 16:
			_expressionStack[_expressionStackPointer - 1] = (uint16)~_expressionStack[_expressionStackPointer - 1];
			break;

		case 17:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (uint16)(_expressionStack[_expressionStackPointer - 1] + _expressionStack[_expressionStackPointer]);
			break;

		case 18:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (uint16)(_expressionStack[_expressionStackPointer - 1] - _expressionStack[_expressionStackPointer]);
			break;

		case 19:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (uint16)((int16)_expressionStack[_expressionStackPointer] * (int16)_expressionStack[_expressionStackPointer - 1]);
			break;

		case 20:
			_expressionStackPointer--;

			if (_expressionStack[_expressionStackPointer] == 0) {
				fatalError(111, "A divide by zero was attempted.");
				return 0;
			}

			_expressionStack[_expressionStackPointer - 1] = (uint16)((int16)_expressionStack[_expressionStackPointer - 1] / (int16)_expressionStack[_expressionStackPointer]);
			break;

		case 21:
			_expressionStackPointer--;

			if (_expressionStack[_expressionStackPointer] == 0) {
				fatalError(111, "A divide by zero was attempted.");
				return 0;
			}

			_expressionStack[_expressionStackPointer - 1] = (uint16)((int16)_expressionStack[_expressionStackPointer - 1] % (int16)_expressionStack[_expressionStackPointer]);
			break;

		case 22:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (_expressionStack[_expressionStackPointer - 1] == _expressionStack[_expressionStackPointer]) ? 1 : 0;
			break;

		case 23:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = ((int16)_expressionStack[_expressionStackPointer - 1] <= (int16)_expressionStack[_expressionStackPointer]) ? 1 : 0;
			break;

		case 24:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = ((int16)_expressionStack[_expressionStackPointer - 1] < (int16)_expressionStack[_expressionStackPointer]) ? 1 : 0;
			break;

		case 25:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = ((int16)_expressionStack[_expressionStackPointer - 1] >= (int16)_expressionStack[_expressionStackPointer]) ? 1 : 0;
			break;

		case 26:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = ((int16)_expressionStack[_expressionStackPointer - 1] > (int16)_expressionStack[_expressionStackPointer]) ? 1 : 0;
			break;

		case 27:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (_expressionStack[_expressionStackPointer - 1] != _expressionStack[_expressionStackPointer]) ? 1 : 0;
			break;

		case 28:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (_expressionStack[_expressionStackPointer - 1] != 0 && _expressionStack[_expressionStackPointer] != 0) ? 1 : 0;
			break;

		case 29:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] = (_expressionStack[_expressionStackPointer - 1] != 0 || _expressionStack[_expressionStackPointer] != 0) ? 1 : 0;
			break;

		case 30:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] &= _expressionStack[_expressionStackPointer];
			break;

		case 31:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] |= _expressionStack[_expressionStackPointer];
			break;

		case 32:
			_expressionStackPointer--;
			_expressionStack[_expressionStackPointer - 1] ^= _expressionStack[_expressionStackPointer];
			break;

		case 33: {
			uint16 address = _expressionStack[_expressionStackPointer - 1];

			writeAddress(address, (uint16)(readAddress(address) + 1));
			_expressionStack[_expressionStackPointer - 1] = readAddress(address);
			break;
		}

		case 34: {
			uint16 address = _expressionStack[_expressionStackPointer - 1];

			_expressionStack[_expressionStackPointer - 1] = readAddress(address);
			writeAddress(address, (uint16)(readAddress(address) + 1));
			break;
		}

		case 35: {
			uint16 address = _expressionStack[_expressionStackPointer - 1];

			writeAddress(address, (uint16)(readAddress(address) - 1));
			_expressionStack[_expressionStackPointer - 1] = readAddress(address);
			break;
		}

		case 36: {
			uint16 address = _expressionStack[_expressionStackPointer - 1];

			_expressionStack[_expressionStackPointer - 1] = readAddress(address);
			writeAddress(address, (uint16)(readAddress(address) - 1));
			break;
		}

		case 37:
			if (_expressionStack[_expressionStackPointer - 1] == 0) {
				token += 2;
			} else {
				token += (int16)READ_LE_UINT16(token);
				pushExpressionValue(1);
			}
			break;

		case 38:
			if (_expressionStack[_expressionStackPointer - 1] != 0) {
				token += 2;
			} else {
				token += (int16)READ_LE_UINT16(token);
				pushExpressionValue(0);
			}
			break;

		case 39:
			pushExpressionValue(readAddress(_expressionStack[_expressionStackPointer - 1]));
			break;

		default:
			break;
		}
	}

	_expressionStackPointer = base;
	return _expressionStack[base];
}

void TopgunEngine_v1_2_1::exprCallBuiltinFunction(const byte *token) {
	uint16 count = READ_LE_UINT16(token + 2);
	uint16 code = _expressionStack[_expressionStackPointer - count - 1];

	uint16 lastBuiltinCode = TG_SCRIPT_V1_2_1_LAST_BUILTIN_CODE;
	if (_version < kVersionV1_2_2)
		lastBuiltinCode--;

	if (code <= lastBuiltinCode)
		evaluateBuiltinFunction(code, count);
	else
		warning("exprCallPluginFunction UNIMPLEMENTED");
}

bool TopgunEngine_v1_2_1::exprCallScript(const byte *token) {
	uint16 frameSlots = READ_LE_UINT16(token);
	uint16 count = READ_LE_UINT16(token + 2);

	for (uint16 slot = count; slot > 0; slot--) {
		int32 index = (int32)_argumentFrame - frameSlots - (slot - 1);

		_expressionStackPointer--;

		if (index < 0) {
			warning("TopgunEngine_v1_2_1::exprCallScript(): argument slot %d is out of range", slot - 1);
			continue;
		}

		_variableTable[index] = _expressionStack[_expressionStackPointer];
	}

	_argumentFrame -= frameSlots;

	uint16 savedResult = _scriptResult;
	_scriptResult = 0;

	_expressionStackPointer--;
	bool result = runScriptResource((uint16)(_expressionStack[_expressionStackPointer] - TG_HOST_V1_2_1_LITERAL_BIAS));

	pushExpressionValue(_scriptResult);
	_scriptResult = savedResult;
	_argumentFrame += frameSlots;

	return result;
}

void TopgunEngine_v1_2_1::evaluateBuiltinFunction(uint16 code, uint16 argc) {
	switch (code) {
	case 27: {
		_expressionStackPointer -= argc;

		const char *format = getStringFromId(_expressionStack[_expressionStackPointer]);
		if (!format)
			break;

		uint16 count = (argc > 0) ? argc - 1 : 0;
		Common::Array<FormatArgument_v1_2> arguments;
		arguments.resize(count);

		for (uint16 i = 0; i < count; i++)
			arguments[i].value = _expressionStack[_expressionStackPointer + 1 + i];

		Common::String text = formatWindowsString(format, count ? &arguments[0] : nullptr, count);

		_expressionStackPointer--;

		if (_debugMode)
			debug("%s", text.c_str());

		break;
	}

	case 46: {
		_expressionStackPointer--;
		int16 minimum = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 maximum = (int16)_expressionStack[_expressionStackPointer];

		if (maximum < minimum) {
			int16 swap = maximum;
			maximum = minimum;
			minimum = swap;
		}

		uint16 random = getRandom15Bit();
		uint16 range = (uint16)(maximum - minimum + 1);
		uint16 pick;

		if (range == 0) {
			warning("TopgunEngine_v1_2_1::evaluateBuiltinFunction(): the range of built-in 46 holds every number");
			pick = random;
		} else {
			pick = random % range;
		}

		_expressionStack[_expressionStackPointer - 1] = (uint16)(minimum + pick);
		break;
	}

	case 47:
		_expressionStack[_expressionStackPointer - 1] = builtinGetKeyState(_expressionStack[_expressionStackPointer - 1]);
		break;

	case 49: {
		int16 deltaY = (int16)(_expressionStack[_expressionStackPointer - 1] - _expressionStack[_expressionStackPointer - 3]);
		int16 deltaX = (int16)(_expressionStack[_expressionStackPointer - 2] - _expressionStack[_expressionStackPointer - 4]);

		_expressionStackPointer -= 4;
		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->getAngle(deltaX, deltaY);
		break;
	}

	case 57:
	case 58: {
		_expressionStackPointer--;
		const char *second = getStringFromId(_expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		const char *first = getStringFromId(_expressionStack[_expressionStackPointer]);

		if (!first || !second)
			break;

		int comparison;

		if (code == 58)
			comparison = scumm_stricmp(first, second);
		else
			comparison = strcmp(first, second);

		uint16 result = 0;
		if (comparison < 0)
			result = (uint16)-1;
		else if (comparison > 0)
			result = 1;

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 65:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->cosine((int16)_expressionStack[_expressionStackPointer]);
		break;

	case 66:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->sine((int16)_expressionStack[_expressionStackPointer]);
		break;

	case 68: {
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];
		uint16 result = 0;

		if (isResourceObjectLoaded((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite) && isRecordIndexValid(sprite, "evaluateBuiltinFunction")) {
			if (spriteLib()->spriteIsVisible(_records[sprite].handle) != 0)
				result = 1;
		}

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 90: {
		_expressionStackPointer -= argc;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		spriteLib()->spriteSendMessage(_records[sprite].handle, argc, &_expressionStack[_expressionStackPointer + 1]);
		_expressionStackPointer--;
		break;
	}

	case 91: {
		_expressionStackPointer -= argc;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		spriteLib()->spritePostMessage(_records[sprite].handle, argc, &_expressionStack[_expressionStackPointer + 1]);
		_expressionStackPointer--;
		break;
	}

	case 95:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = builtinSetDraggedSpriteId(_expressionStack[_expressionStackPointer]);
		break;

	case 98: {
		_expressionStackPointer--;
		int16 value = (int16)_expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = (uint16)((value < 0) ? -value : value);
		break;
	}

	case 99: {
		_expressionStackPointer--;
		const char *text = getStringFromId(_expressionStack[_expressionStackPointer]);
		if (!text)
			break;

		_expressionStack[_expressionStackPointer - 1] = (uint16)strlen(text);
		break;
	}

	case 100: {
		_expressionStackPointer--;
		int16 start = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		const char *needle = getStringFromId(_expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		const char *text = getStringFromId(_expressionStack[_expressionStackPointer]);
		const char *found = nullptr;

		if (text && needle && (int)strlen(text) > start) {
			if (start < 0)
				warning("TopgunEngine_v1_2_1::evaluateBuiltinFunction(): the start of built-in 100 is negative");
			else
				found = strstr(text + start, needle);
		}

		if (found)
			_expressionStack[_expressionStackPointer - 1] = (uint16)(found - text);
		else
			_expressionStack[_expressionStackPointer - 1] = (uint16)-1;

		break;
	}

	case 101:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = builtinCopyResource(_expressionStack[_expressionStackPointer]);
		break;

	case 102:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = builtinDeleteResource(_expressionStack[_expressionStackPointer]);
		break;

	case 104:
		_expressionStackPointer--;
		warning("builtinPrintBitmapResource UNIMPLEMENTED");
		_expressionStack[_expressionStackPointer - 1] = 0;
		break;

	case 105: {
		uint16 previous = _keyHookScript;

		_expressionStackPointer--;
		builtinSetKeyHookScript(_expressionStack[_expressionStackPointer]);
		_expressionStack[_expressionStackPointer - 1] = operandValue(previous);
		break;
	}

	case 107: {
		_expressionStackPointer--;
		uint16 key = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 kind = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 set = (byte)_expressionStack[_expressionStackPointer];

		builtinSetKeyboardSetKey(set, kind, key);
		break;
	}

	case 108: {
		_expressionStackPointer--;
		uint16 value = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (sprite != 0 && !isResourceObjectLoaded((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
			break;

		uint16 handle = 0;

		if (sprite != 0) {
			if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
				break;

			handle = _records[sprite].handle;
		}

		spriteLib()->spritePause(handle, value);
		break;
	}

	case 109:
		_expressionStackPointer--;
		builtinPauseTimers(_expressionStack[_expressionStackPointer]);
		break;

	case 110:
		_expressionStackPointer--;
		builtinSetJoystickDistanceMode(_expressionStack[_expressionStackPointer]);
		break;

	case 111: {
		uint16 previous = _hoverSpriteScript;

		_expressionStackPointer--;
		builtinSetSpriteHoverScript(_expressionStack[_expressionStackPointer]);
		_expressionStack[_expressionStackPointer - 1] = operandValue(previous);
		break;
	}

	case 112:
		_expressionStackPointer--;
		audioLib()->audioPause(_expressionStack[_expressionStackPointer]);
		break;

	case 114: {
		_expressionStackPointer--;
		int16 y1 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x1 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 y0 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x0 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 total = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 step = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 yAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 xAddress = _expressionStack[_expressionStackPointer];

		Common::Point point = spriteLib()->spriteLinePoint(step, total, x0, y0, x1, y1);

		writeAddress(xAddress, (uint16)point.x);
		writeAddress(yAddress, (uint16)point.y);
		break;
	}

	case 115: {
		_expressionStackPointer--;
		int16 endY = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 endX = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 viaY = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 viaX = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 y0 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x0 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 steps = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 step = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 yAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 xAddress = _expressionStack[_expressionStackPointer];

		Common::Point point = spriteLib()->spriteCurvePoint(step, steps, x0, y0, viaX, viaY, endX, endY);

		writeAddress(xAddress, (uint16)point.x);
		writeAddress(yAddress, (uint16)point.y);
		break;
	}

	case 116: {
		_expressionStackPointer--;
		uint16 index = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		const char *text = getStringFromId(_expressionStack[_expressionStackPointer]);
		uint16 result = 0;

		if (text && strlen(text) >= index)
			result = (uint16)(int16)(int8)text[index];

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 117: {
		_expressionStackPointer--;
		uint16 character = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 index = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 id = _expressionStack[_expressionStackPointer];

		if ((id & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
			break;

		char *text = getStringFromId(id);
		if (!text)
			break;

		if (index >= TG_BUILTIN_V1_2_1_MAX_STRING_INDEX)
			break;

		text[index] = (char)character;
		break;
	}

	case 118: {
		_expressionStackPointer--;
		const char *text = getStringFromId(_expressionStack[_expressionStackPointer]);
		uint16 result = 0;

		if (text) {
			char copy[TG_HOST_V1_2_STRING_SLOT_SIZE];

			Common::strlcpy(copy, text, sizeof(copy));
			result = (uint16)atoi(copy);
		}

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 120: {
		_expressionStackPointer--;
		int16 deltaY = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 deltaX = (int16)_expressionStack[_expressionStackPointer];

		spriteLib()->spriteScreenShake(deltaX, deltaY);
		break;
	}

	case 121: {
		_expressionStackPointer--;
		uint16 fractionAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 integerAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int32 addedFraction = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int32 addedInteger = (int16)_expressionStack[_expressionStackPointer];

		int32 integerSum = addedInteger + (int16)readAddress(integerAddress);
		int32 total = (int32)((uint32)TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE * (uint32)integerSum);
		total = (int32)((uint32)total + (uint32)(int32)(int16)readAddress(fractionAddress) + (uint32)addedFraction);

		writeAddress(integerAddress, (uint16)(total / TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE));
		writeAddress(fractionAddress, (uint16)(total % TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE));
		break;
	}

	case 122: {
		_expressionStackPointer--;
		uint16 fractionAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 integerAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int32 factorFraction = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int32 factorInteger = (int16)_expressionStack[_expressionStackPointer];

		int32 first = (int32)((uint32)(TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE * (int32)(int16)readAddress(integerAddress)) + (uint32)(int32)(int16)readAddress(fractionAddress));
		int32 second = (int32)((uint32)(TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE * factorInteger) + (uint32)factorFraction);
		int32 product = (int32)((uint32)first * (uint32)second);

		writeAddress(integerAddress, (uint16)(product / TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE));
		writeAddress(fractionAddress, (uint16)(product % TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE));
		break;
	}

	case 123: {
		_expressionStackPointer--;
		uint16 fractionAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 integerAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int32 divisorFraction = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int32 divisorInteger = (int16)_expressionStack[_expressionStackPointer];

		int32 divisor = (int32)((uint32)(TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE * divisorInteger) + (uint32)divisorFraction);
		int32 dividend = (int32)((uint32)(TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE * (int32)(int16)readAddress(integerAddress)) + (uint32)(int32)(int16)readAddress(fractionAddress));
		int32 quotient = 0;

		if (divisor == 0)
			warning("TopgunEngine_v1_2_1::evaluateBuiltinFunction(): built-in 123 divides by zero");
		else
			quotient = dividend / divisor;

		writeAddress(integerAddress, (uint16)(quotient / TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE));
		writeAddress(fractionAddress, (uint16)(quotient % TG_BUILTIN_V1_2_1_FIXED_POINT_SCALE));
		break;
	}

	case 124: {
		_expressionStackPointer--;
		uint16 angle2Address = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 speed2Address = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 angle1Address = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 speed1Address = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 y2 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x2 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 y1 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x1 = (int16)_expressionStack[_expressionStackPointer];

		int16 speed1 = (int16)readAddress(speed1Address);
		int16 angle1 = (int16)readAddress(angle1Address);
		int16 speed2 = (int16)readAddress(speed2Address);
		int16 angle2 = (int16)readAddress(angle2Address);

		utilsLibrary()->collide(x1, y1, x2, y2, &speed1, &angle1, &speed2, &angle2);

		writeAddress(speed1Address, (uint16)speed1);
		writeAddress(angle1Address, (uint16)angle1);
		writeAddress(speed2Address, (uint16)speed2);
		writeAddress(angle2Address, (uint16)angle2);
		break;
	}

	case 125: {
		_expressionStackPointer--;
		uint16 second = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 first = _expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = builtinBlitBitmapOverlap(first, second);
		break;
	}

	case 126: {
		_expressionStackPointer--;
		int16 y2 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x2 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 y1 = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x1 = (int16)_expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->distance(x1, y1, x2, y2);
		break;
	}

	case 127: {
		_expressionStackPointer--;
		int16 axis = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 angle = (int16)_expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->reflectAngle(angle, axis);
		break;
	}

	case 128:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->squareRoot((int16)_expressionStack[_expressionStackPointer]);
		break;

	case 129:
		_expressionStack[_expressionStackPointer - 1] = builtinGetMouseEventScript();
		break;

	case 130:
		_expressionStackPointer--;
		builtinSetJoystickButtonOffsetMode(_expressionStack[_expressionStackPointer]);
		break;

	case 131: {
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!isResourceObjectLoaded((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		_expressionStack[_expressionStackPointer - 1] = spriteLib()->spriteGetExtraId(_records[sprite].handle);
		break;
	}

	case 132: {
		_expressionStackPointer--;
		uint16 value = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!isResourceObjectLoaded((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		spriteLib()->spriteSetExtraId(_records[sprite].handle, value);
		break;
	}

	case 133:
		_expressionStackPointer--;
		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->getQuadrant((int16)_expressionStack[_expressionStackPointer]);
		break;

	case 134: {
		_expressionStackPointer--;
		uint16 sameLevelOnly = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 allowInvisibleSecond = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 allowInvisibleFirst = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 second = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 first = _expressionStack[_expressionStackPointer];
		uint16 result = 0;

		if (isResourceObjectLoaded((uint16)(second - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite) && isResourceObjectLoaded((uint16)(first - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite)) {
			if (isRecordIndexValid(first, "evaluateBuiltinFunction") && isRecordIndexValid(second, "evaluateBuiltinFunction")) {
				if (spriteLib()->spriteHitDetect(_records[first].handle, _records[second].handle, allowInvisibleFirst, allowInvisibleSecond, sameLevelOnly) != 0)
					result = 1;
			}
		}

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 135: {
		_expressionStackPointer--;
		int16 second = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 first = (int16)_expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = (uint16)((first > second) ? second : first);
		break;
	}

	case 136: {
		_expressionStackPointer--;
		int16 second = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 first = (int16)_expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = (uint16)((first < second) ? second : first);
		break;
	}

	case 137: {
		_expressionStackPointer--;
		uint16 flag = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 group = _expressionStack[_expressionStackPointer];

		builtinReattachDeviceGroup(group, flag);
		break;
	}

	case 138: {
		_expressionStackPointer--;
		int16 y = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 x = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!isResourceObjectLoaded((uint16)(sprite - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		_expressionStack[_expressionStackPointer - 1] = spriteLib()->spriteIsPicked(_records[sprite].handle, x, y);
		break;
	}

	case 139:
		_expressionStack[_expressionStackPointer - 1] = _driveType;
		break;

	case 140:
		_expressionStack[_expressionStackPointer - 1] = _freeMemoryHigh;
		break;

	case 141:
		if (_version >= kVersionV1_2_2) {
			uint16 script = _expressionStack[_expressionStackPointer - 1];

			_activateScript = script ? (uint16)(script - TG_HOST_V1_2_1_LITERAL_BIAS) : 0;
		}

		break;

	default:
		break;
	}
}

uint16 TopgunEngine_v1_2_1::builtinBlitBitmapOverlap(uint16 first, uint16 second) {
	if (!validateAndGetResourceByType((uint16)(first - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Bitmap))
		return 0;

	uint16 type = resourceLib()->resGetType(_file, second);
	if (type != kResV1_2_1Bitmap && type != kResV1_2_1Text)
		return 0;

	if (!validateAndGetResourceByType((uint16)(second - TG_HOST_V1_2_1_LITERAL_BIAS), type))
		return 0;

	if (!isRecordIndexValid(first, "builtinBlitBitmapOverlap") || !isRecordIndexValid(second, "builtinBlitBitmapOverlap"))
		return 0;

	byte *firstBitmap = _records[first].data;
	byte *secondBitmap = _records[second].data;
	Common::Rect firstBox;
	Common::Rect secondBox;

	spriteLib()->spriteDibGetBox(firstBitmap, firstBox);
	spriteLib()->spriteDibGetBox(secondBitmap, secondBox);

	if (!firstBox.intersects(secondBox))
		return 0;

	Common::Rect overlap = firstBox.findIntersectingRect(secondBox);

	spriteLib()->spriteDibMerge(firstBitmap, secondBitmap, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		(int16)(overlap.left - firstBox.left), (int16)(overlap.top - firstBox.top),
		(int16)(overlap.left - secondBox.left), (int16)(overlap.top - secondBox.top));
	return 1;
}

uint16 TopgunEngine_v1_2_1::builtinCopyResource(uint16 id) {
	if (resourceLib()->resGetType(_file, id) != kResV1_2_1Sprite)
		return 0;

	uint16 copy = resourceLib()->resCopyResource(_file, id);

	if (copy != 0)
		validateAndGetResourceByType((uint16)(copy - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite);

	return copy;
}

uint16 TopgunEngine_v1_2_1::builtinDeleteResource(uint16 id) {
	if (resourceLib()->resGetType(_file, id) != kResV1_2_1Sprite)
		return 0;

	if (!destroyResourceObjectByType((uint16)(id - TG_HOST_V1_2_1_LITERAL_BIAS), kResV1_2_1Sprite))
		return 0;

	if (!resourceLib()->resDeleteResource(_file, id))
		return 0;

	if (!isRecordIndexValid(id, "builtinDeleteResource"))
		return 0;

	_records[id].data = nullptr;
	return 1;
}

} // End of namespace Topgun
