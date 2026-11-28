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

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v2_0.h"

namespace Topgun {

void TopgunEngine_v1_2::pushExpressionValue(uint16 value) {
	if (_expressionStackPointer >= TG_SCRIPT_V1_2_EXPRESSION_STACK_SIZE) {
		warning("TopgunEngine_v1_2::pushExpressionValue(): the expression stack is full");
		return;
	}

	_expressionStack[_expressionStackPointer] = value;
	_expressionStackPointer++;
}

uint16 TopgunEngine_v1_2::evaluateExpression(const byte *expression) {
	uint16 base = _expressionStackPointer;
	const byte *token = expression;

	while (*token != 0) {
		byte code = *token;
		token++;

		// The later versions leave token 12 unassigned and number every following token one higher
		if (_version >= kVersionV1_2_7_3 && _version != kVersionV1_2_8) {
			if (code == 12)
				continue;

			if (code > 12)
				code--;
		}

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
			if (_version >= kVersionV1_2_3_1)
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
			if (_version >= kVersionV1_2_3_1) {
				exprCallBuiltinFunction(token);
				token += 4;
			} else {
				if (READ_LE_UINT16(token) > TG_HOST_V1_2_VARIABLE_LAST_INDEX)
					exprCallScript(token);
				else
					evaluateBuiltinFunction(READ_LE_UINT16(token), READ_LE_UINT16(token + 4));

				token += 6;
			}

			break;

		case 13:
			if (_version >= kVersionV1_2_3_1) {
				exprCallScript(token);
				token += 4;
			}

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
			if (_version >= kVersionV1_2_3_1)
				pushExpressionValue(readAddress(_expressionStack[_expressionStackPointer - 1]));

			break;

		default:
			break;
		}
	}

	_expressionStackPointer = base;
	return _expressionStack[base];
}

void TopgunEngine_v1_2::exprCallBuiltinFunction(const byte *token) {
	uint16 count = READ_LE_UINT16(token + 2);
	uint16 code = _expressionStack[_expressionStackPointer - count - 1];

	uint16 lastBuiltinCode = TG_SCRIPT_V1_2_4_LAST_BUILTIN_CODE;
	if (_version >= kVersionV2_0)
		lastBuiltinCode = TG_SCRIPT_V2_0_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_12)
		lastBuiltinCode = TG_SCRIPT_V1_2_12_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_11)
		lastBuiltinCode = TG_SCRIPT_V1_2_11_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_10)
		lastBuiltinCode = TG_SCRIPT_V1_2_10_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_9)
		lastBuiltinCode = TG_SCRIPT_V1_2_9_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_7_3)
		lastBuiltinCode = TG_SCRIPT_V1_2_7_3_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_7_2)
		lastBuiltinCode = TG_SCRIPT_V1_2_7_2_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_7_1)
		lastBuiltinCode = TG_SCRIPT_V1_2_7_1_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_7)
		lastBuiltinCode = TG_SCRIPT_V1_2_7_LAST_BUILTIN_CODE;
	else if (_version >= kVersionV1_2_6)
		lastBuiltinCode = TG_SCRIPT_V1_2_6_LAST_BUILTIN_CODE;
	else if (_version == kVersionV1_2_3_1)
		lastBuiltinCode = TG_SCRIPT_V1_2_3_1_LAST_BUILTIN_CODE;
	else if (_version == kVersionV1_2_3_2)
		lastBuiltinCode = TG_SCRIPT_V1_2_3_2_LAST_BUILTIN_CODE;
	else if (_version == kVersionV1_2_3_3)
		lastBuiltinCode = TG_SCRIPT_V1_2_3_3_LAST_BUILTIN_CODE;
	else if (_version < kVersionV1_2_5)
		lastBuiltinCode--;

	if (code <= lastBuiltinCode)
		evaluateBuiltinFunction(code, count);
	else
		exprCallPluginFunction(code - lastBuiltinCode - 1, count);
}

bool TopgunEngine_v1_2::exprCallScript(const byte *token) {
	uint16 script = 0;
	uint16 frameSlots;
	uint16 count;

	if (_version >= kVersionV1_2_3_1) {
		frameSlots = READ_LE_UINT16(token);
		count = READ_LE_UINT16(token + 2);
	} else {
		script = READ_LE_UINT16(token);
		frameSlots = READ_LE_UINT16(token + 2);
		count = READ_LE_UINT16(token + 4);
	}

	for (uint16 slot = count; slot > 0; slot--) {
		int32 index = (int32)_argumentFrame - frameSlots - (slot - 1);

		_expressionStackPointer--;

		if (index < 0) {
			warning("TopgunEngine_v1_2::exprCallScript(): argument slot %d is out of range", slot - 1);
			continue;
		}

		_variableTable[index] = _expressionStack[_expressionStackPointer];
	}

	_argumentFrame -= frameSlots;

	uint16 savedResult = _scriptResult;
	_scriptResult = 0;

	if (_version >= kVersionV1_2_3_1) {
		uint16 bias = (_version >= kVersionV1_2_3_2) ? TG_HOST_V1_2_4_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;
		if (_version >= kVersionV2_0)
			bias = 0;

		_expressionStackPointer--;
		script = (uint16)(_expressionStack[_expressionStackPointer] - bias);
	}

	bool result = runScriptResource(script);

	pushExpressionValue(_scriptResult);
	_scriptResult = savedResult;
	_argumentFrame += frameSlots;

	return result;
}

void TopgunEngine_v1_2::evaluateBuiltinFunction(uint16 code, uint16 argc) {
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

		debug("%s", formatWindowsString(format, count ? &arguments[0] : nullptr, count).c_str());

		if (_version >= kVersionV1_2_3_1)
			_expressionStackPointer--;

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
			warning("TopgunEngine_v1_2::evaluateBuiltinFunction(): the range of built-in 46 holds every number");
			pick = random;
		} else {
			pick = random % range;
		}

		if (_version >= kVersionV1_2_3_1)
			_expressionStack[_expressionStackPointer - 1] = (uint16)(minimum + pick);
		else
			pushExpressionValue((uint16)(minimum + pick));

		break;
	}

	case 47: {
		uint16 key = _expressionStack[_expressionStackPointer - 1];

		_expressionStack[_expressionStackPointer - 1] = (key < TG_HOST_V1_2_KEY_STATE_SIZE && _keyStates[key]) ? 1 : 0;
		break;
	}

	case 49: {
		int16 deltaY = (int16)(_expressionStack[_expressionStackPointer - 1] - _expressionStack[_expressionStackPointer - 3]);

		if (_version >= kVersionV1_2_3_1) {
			int16 deltaX = (int16)(_expressionStack[_expressionStackPointer - 2] - _expressionStack[_expressionStackPointer - 4]);

			_expressionStackPointer -= 4;
			_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->getAngle(deltaX, deltaY);
			break;
		}

		// The original also runs the debug string case after pushing the angle, which pops the arguments a second time
		_expressionStackPointer -= 4;
		pushExpressionValue((uint16)utilsLibrary()->getAngle(0, deltaY));
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

		uint16 result = (uint16)compareStringsCustom(first, second, code != 58);

		if (_version >= kVersionV1_2_3_1)
			_expressionStack[_expressionStackPointer - 1] = result;
		else
			pushExpressionValue(result);

		break;
	}

	case 65: {
		int16 angle = (int16)_expressionStack[_expressionStackPointer - 1];

		if (_version >= kVersionV1_2_3_1)
			_expressionStackPointer--;

		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->cosine(angle);
		break;
	}

	case 66: {
		int16 angle = (int16)_expressionStack[_expressionStackPointer - 1];

		if (_version >= kVersionV1_2_3_1)
			_expressionStackPointer--;

		_expressionStack[_expressionStackPointer - 1] = (uint16)utilsLibrary()->sine(angle);
		break;
	}

	case 90: {
		_expressionStackPointer -= argc;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(sprite - TG_HOST_V1_2_LITERAL_BIAS), (_version >= kVersionV1_2_3_1) ? kResV1_2_4Sprite : kResV1_2_3Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		spriteLib()->spriteSendMessage(_records[sprite].handle, argc, &_expressionStack[_expressionStackPointer + 1]);

		if (_version >= kVersionV1_2_3_1)
			_expressionStackPointer--;

		break;
	}

	case 91: {
		if (_version < kVersionV1_2_3_1)
			break;

		_expressionStackPointer -= argc;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(sprite - TG_HOST_V1_2_LITERAL_BIAS), kResV1_2_4Sprite))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		static_cast<SpriteLibrary_v1_2_4 *>(spriteLib())->spritePostMessage(_records[sprite].handle, argc, &_expressionStack[_expressionStackPointer + 1]);
		_expressionStackPointer--;
		break;
	}

	default:
		break;
	}
}

static const byte kBuiltinCodesV2_0[] = {
	  0,   0,   0,  98,   0, 142, 121, 218, 219, 217, 220,  49, 118,  25,   0, 111,
	  0,   0, 124,   0,   0, 206, 125,   0, 134, 101,  65,  31,   0, 115,  27, 185,
	184, 188, 102,   0, 191, 233, 234, 196, 199, 126, 123,   0,   0, 227, 190, 187,
	177,   0, 235, 236, 195, 200,   0, 170, 171,  13,   0, 148,  10,  11,   0,  55,
	  0, 116,   0, 207, 161, 162, 139, 140,  62, 129, 163, 160, 144,  40, 133, 167,
	168, 202, 158,  70,   0, 157, 149, 164,   0,   0,   0,   0,   0, 224,   0, 107,
	  0,   0, 137,   0, 221,   0, 130, 110,   0, 105,  47,  12,   0,   0, 204, 150,
	  0,  28,   0, 114,  21,   2, 136, 135,   0, 122,   0, 210,  36, 189, 225, 226,
	180, 231, 232, 223, 183, 186, 194, 198, 201, 112,   0, 153, 108, 109,   9,   0,
	211,   0, 212, 213, 151,   0, 173, 104, 138,  46,   0, 174,   0, 155, 127,   0,
	181,   0,   0,   0,   0, 147, 152,   0, 175,   0, 179, 230, 117, 165,  95,   0,
	193,   0, 146,  97,  96, 192, 229,   0,  63,   5, 203, 178, 228, 166, 145, 169,
	  0, 159,   0,   0,   0, 205,   0, 182, 120,   0,  66, 197, 131,  91,  90, 132,
	222, 128,  14,  81,  15,  57,  58,  99, 100,   0,   0,   0,   0, 209,   0,   3,
	 50,  68, 154,   0,   0, 156,   0,   0
};

void TopgunEngine_v1_2_4::evaluateBuiltinFunction(uint16 code, uint16 argc) {
	if (_version >= kVersionV2_0) {
		if (code >= ARRAYSIZE(kBuiltinCodesV2_0))
			return;

		code = kBuiltinCodesV2_0[code];
	}

	uint16 spriteType = kResV1_2_4Sprite;
	if (_version >= kVersionV1_2_12)
		spriteType = kResV1_2_12Sprite;
	else if (_version >= kVersionV1_2_6)
		spriteType = kResV1_2_6Sprite;

	uint16 literalBias = TG_HOST_V1_2_4_LITERAL_BIAS;
	if (_version >= kVersionV2_0)
		literalBias = 0;

	// The built-ins from 157 on exist only in the last version
	if (code > 156 && _version < kVersionV1_2_12)
		return;

	switch (code) {
	case 2:
	case 3: {
		if (_version < kVersionV1_2_10)
			break;

		_expressionStackPointer--;
		uint16 id = _expressionStack[_expressionStackPointer];
		uint16 type = resourceLib()->resGetType(_file, id);
		bool result;

		if (code == 2)
			result = validateAndGetResourceByType((uint16)(id - literalBias), type);
		else
			result = destroyResourceObjectByType((uint16)(id - literalBias), type);

		_expressionStack[_expressionStackPointer - 1] = result ? 1 : 0;
		break;
	}

	case 5: {
		if (_version < kVersionV1_2_10)
			break;

		uint16 flag = 0;

		if (argc > 2) {
			_expressionStackPointer--;
			flag = _expressionStack[_expressionStackPointer];
		}

		_expressionStackPointer--;
		uint16 data = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];
		bool result = o_setSpriteQueue((uint16)(sprite - literalBias), (uint16)(data - literalBias), flag != 0);

		_expressionStack[_expressionStackPointer - 1] = result ? 1 : 0;
		break;
	}

	case 9:
		if (_version < kVersionV1_2_12)
			break;

		_expressionStackPointer--;
		o_playCDTrack((byte)_expressionStack[_expressionStackPointer]);
		break;

	case 10:
	case 11: {
		if (_version < kVersionV1_2_10)
			break;

		byte record[4];
		uint16 operation = code;

		if (_version >= kVersionV2_0)
			operation = (code == 10) ? kOpV2_0FadeIn : kOpV2_0FadeOut;

		_expressionStackPointer--;
		WRITE_LE_UINT16(record, operation);
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_fadeInOut(record);
		break;
	}

	case 12: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[12];

		memset(record, 0, sizeof(record));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_bindKeyScript(record);
		break;
	}

	case 13: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[2];
		uint16 operation = kOpV1_2_4Quit;

		if (_version >= kVersionV2_0)
			operation = kOpV2_0Quit;

		WRITE_LE_UINT16(record, operation);
		executeScriptOp(record, nullptr);
		break;
	}

	case 14:
		if (_version >= kVersionV1_2_12)
			o_stopCD();

		break;

	case 15: {
		if (_version < kVersionV1_2_12)
			break;

		byte block[4];

		_expressionStackPointer--;
		WRITE_LE_UINT32(block, (uint32)(int32)(int16)_expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		o_stopWave((uint16)(_expressionStack[_expressionStackPointer] - literalBias), block);
		break;
	}

	case 21: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[6];
		uint16 operation = kOpV1_2_4SetSceneEndScript;

		if (_version >= kVersionV2_0)
			operation = kOpV2_0SetSceneEndScript;

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, operation);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		executeScriptOp(record, nullptr);
		break;
	}

	case 25: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[6];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4StopSprite);
		_expressionStackPointer--;
		record[4] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_stopSprite(record);
		break;
	}

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

		if (_debugMode || _version == kVersionV1_2_3_2)
			debug("%s", text.c_str());

		break;
	}

	case 28: {
		if (_version < kVersionV1_2_12)
			break;

		_expressionStackPointer--;
		uint16 level = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		o_setSpriteLevel((uint16)(_expressionStack[_expressionStackPointer] - literalBias), level);
		break;
	}

	case 31: {
		if (_version < kVersionV1_2_12)
			break;

		_expressionStackPointer--;
		setCursorMode(_expressionStack[_expressionStackPointer]);
		break;
	}

	case 36: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[8];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4MoveSprite);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_moveSprite(record);
		break;
	}

	case 40: {
		if (_version < kVersionV1_2_12)
			break;

		_expressionStackPointer--;
		uint16 yAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 xAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateResourceType((uint16)(sprite - literalBias), spriteType))
			break;

		// The results are stored through a null segment
		if (xAddress != 0 || yAddress != 0)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 40 cannot store the position");

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
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): the range of built-in 46 holds every number");
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

	case 50:
		if (_version < kVersionV1_2_12)
			break;

		// The two results are stored through a null segment
		_expressionStackPointer -= 9;
		warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 50 cannot store the exit point");
		break;

	case 55: {
		if (_version < kVersionV1_2_12)
			break;

		_expressionStackPointer -= 4;
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateResourceType((uint16)(sprite - literalBias), spriteType))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		// The bounds are stored through a null segment
		if (_records[sprite].handle != 0)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 55 cannot store the bounds");

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

		_expressionStack[_expressionStackPointer - 1] = (uint16)compareStringsCustom(first, second, code != 58);
		break;
	}

	case 62:
		if (_version < kVersionV1_2_12)
			break;

		// The intersection is stored through a null segment
		_expressionStackPointer -= 10;
		warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 62 cannot store the intersection");
		break;

	case 63: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[8];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetSpritePosition);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_setSpritePosition(record);
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
		if (_version < kVersionV1_2_4)
			break;

		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];
		uint16 result = 0;

		if (isResourceObjectLoaded((uint16)(sprite - literalBias), spriteType) && isRecordIndexValid(sprite, "evaluateBuiltinFunction")) {
			if (spriteLib()->spriteIsVisible(_records[sprite].handle) != 0)
				result = 1;
		}

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 70:
		if (_version < kVersionV1_2_12)
			break;

		// The time is stored through a null segment
		_expressionStackPointer -= 4;

		if (_expressionStack[_expressionStackPointer] != 0 || _expressionStack[_expressionStackPointer + 1] != 0 ||
				_expressionStack[_expressionStackPointer + 2] != 0 || _expressionStack[_expressionStackPointer + 3] != 0)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 70 cannot store the time");

		break;

	case 81: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[4];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4StopMidi);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_stopMidi(record);
		break;
	}

	case 90: {
		_expressionStackPointer -= argc;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(sprite - literalBias), spriteType))
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

		if (!validateAndGetResourceByType((uint16)(sprite - literalBias), spriteType))
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

	case 96: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[4];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4LoadPaletteResource);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_loadPaletteResource(record);
		break;
	}

	case 97: {
		if (_version < kVersionV1_2_12)
			break;

		byte record[12];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetPaletteEntries);
		_expressionStackPointer--;
		record[10] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		record[9] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		record[8] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, _expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);

		if (READ_LE_UINT16(record + 4) > 1) {
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 97 holds one entry, not %d", READ_LE_UINT16(record + 4));
			WRITE_LE_UINT16(record + 4, 1);
		}

		o_setPaletteEntries(record);
		break;
	}

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
				warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): the start of built-in 100 is negative");
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

		if (_version >= kVersionV1_2_4)
			_expressionStack[_expressionStackPointer - 1] = 0;

		break;

	case 105: {
		uint16 previous = _keyHookScript;

		_expressionStackPointer--;
		builtinSetKeyHookScript(_expressionStack[_expressionStackPointer]);

		if (_version >= kVersionV1_2_4)
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

		if (sprite != 0 && !isResourceObjectLoaded((uint16)(sprite - literalBias), spriteType))
			break;

		uint16 handle = 0;

		if (sprite != 0) {
			if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
				break;

			handle = _records[sprite].handle;
		}

		spriteLib()->spritePause(handle, value);

		if (_version >= kVersionV1_2_9)
			setMoviesPaused((byte)value != 0);

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

		if (_version >= kVersionV1_2_4)
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

		if (text) {
			bool inRange = (strlen(text) >= index);

			// The later version compares the length with the index as signed numbers
			if (_version >= kVersionV1_2_6) {
				inRange = ((int16)strlen(text) >= (int16)index);

				if (inRange && (int16)index < 0) {
					warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): the string index %d is negative", (int16)index);
					inRange = false;
				}
			}

			if (inRange)
				result = (uint16)(int16)(int8)text[index];
		}

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

		if (index >= TG_BUILTIN_V1_2_4_MAX_STRING_INDEX)
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
		if (_version < kVersionV1_2_4)
			break;

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
		int32 total = (int32)((uint32)TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE * (uint32)integerSum);
		total = (int32)((uint32)total + (uint32)(int32)(int16)readAddress(fractionAddress) + (uint32)addedFraction);

		writeAddress(integerAddress, (uint16)(total / TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE));
		writeAddress(fractionAddress, (uint16)(total % TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE));
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

		int32 first = (int32)((uint32)(TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE * (int32)(int16)readAddress(integerAddress)) + (uint32)(int32)(int16)readAddress(fractionAddress));
		int32 second = (int32)((uint32)(TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE * factorInteger) + (uint32)factorFraction);
		int32 product = (int32)((uint32)first * (uint32)second);

		writeAddress(integerAddress, (uint16)(product / TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE));
		writeAddress(fractionAddress, (uint16)(product % TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE));
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

		int32 divisor = (int32)((uint32)(TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE * divisorInteger) + (uint32)divisorFraction);
		int32 dividend = (int32)((uint32)(TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE * (int32)(int16)readAddress(integerAddress)) + (uint32)(int32)(int16)readAddress(fractionAddress));
		int32 quotient = 0;

		if (divisor == 0)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 123 divides by zero");
		else
			quotient = dividend / divisor;

		writeAddress(integerAddress, (uint16)(quotient / TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE));
		writeAddress(fractionAddress, (uint16)(quotient % TG_BUILTIN_V1_2_4_FIXED_POINT_SCALE));
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
		uint16 deltaY = 0;
		uint16 deltaX = 0;

		if (_version >= kVersionV1_2_10) {
			if (argc == 4) {
				_expressionStackPointer--;
				deltaY = _expressionStack[_expressionStackPointer];
			}

			if (argc >= 3) {
				_expressionStackPointer--;
				deltaX = _expressionStack[_expressionStackPointer];
			}
		}

		_expressionStackPointer--;
		uint16 second = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 first = _expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = builtinBlitBitmapOverlap(first, second, (int16)deltaX, (int16)deltaY);
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

		if (!isResourceObjectLoaded((uint16)(sprite - literalBias), spriteType)) {
			if (_version >= kVersionV2_0)
				_expressionStack[_expressionStackPointer - 1] = 0;

			break;
		}

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
		bool available;

		if (_version >= kVersionV2_0)
			available = validateAndGetResourceByType((uint16)(sprite - literalBias), spriteType);
		else
			available = isResourceObjectLoaded((uint16)(sprite - literalBias), spriteType);

		if (!available)
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

		if (isResourceObjectLoaded((uint16)(second - literalBias), spriteType) && isResourceObjectLoaded((uint16)(first - literalBias), spriteType)) {
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

		if (!isResourceObjectLoaded((uint16)(sprite - literalBias), spriteType))
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
	case 142: {
		bool setsActivateScript = (_version >= kVersionV1_2_5);

		// The later version moves the function from 141 to 142 and leaves 141 empty
		if (_version >= kVersionV1_2_6)
			setsActivateScript = (code == 142);

		if (setsActivateScript) {
			uint16 script = _expressionStack[_expressionStackPointer - 1];

			_activateScript = script ? (uint16)(script - literalBias) : 0;
		}

		break;
	}

	case 144:
		if (_version >= kVersionV1_2_7)
			_expressionStack[_expressionStackPointer - 1] = TG_SCRIPT_V1_2_7_WINDOWS_VERSION;

		break;

	case 145: {
		if (_version < kVersionV1_2_7_1)
			break;

		if (_version >= kVersionV1_2_12) {
			_expressionStackPointer--;
			int16 y = (int16)_expressionStack[_expressionStackPointer];
			_expressionStackPointer--;
			int16 x = (int16)_expressionStack[_expressionStackPointer];

			spriteLib()->spriteSetScrollPos(x, y);
			break;
		}

		uint16 script = _expressionStack[_expressionStackPointer - 1];

		_rubberBandScript = script ? (uint16)(script - TG_HOST_V1_2_4_LITERAL_BIAS) : 0;

		if (script == 0)
			cancelMouseCapture();

		break;
	}

	case 146: {
		if (_version < kVersionV1_2_7_2)
			break;

		_expressionStackPointer--;
		uint16 speed = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 movieId = _expressionStack[_expressionStackPointer];

		uint16 movieType = kResV1_2_6Movie;
		if (_version >= kVersionV1_2_12)
			movieType = kResV1_2_12Movie;

		if (!isResourceObjectLoaded((uint16)(movieId - literalBias), movieType))
			break;

		if (!isRecordIndexValid(movieId, "evaluateBuiltinFunction"))
			break;

		Movie_v1_2 *movie = movieFromHandle(_records[movieId].object);
		if (movie)
			movie->speed = speed;

		break;
	}

	case 147: {
		if (_version < kVersionV1_2_7_2)
			break;

		_expressionStackPointer--;
		uint16 seconds = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 script = _expressionStack[_expressionStackPointer];

		_idleScript = script ? (uint16)(script - literalBias) : 0;

		if (_idleScript != 0) {
			// The product is kept as a 16-bit number
			_idleDelay = (uint16)(seconds * TG_SCRIPT_V1_2_7_2_IDLE_SECONDS_TO_MS);
			_idleDeadline = getEngineMillis() + _idleDelay;
		}

		break;
	}

	case 148:
		if (_version < kVersionV1_2_7_2)
			break;

		if (_version < kVersionV1_2_8) {
			_expressionStackPointer--;
			const char *program = getStringFromId(_expressionStack[_expressionStackPointer]);

			if (program)
				warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): launching the program '%s' is not supported", program);

			break;
		}

		_expressionStackPointer -= 3;
		warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): extracting an icon from a program file is not supported");
		_expressionStack[_expressionStackPointer - 1] = 0;
		break;

	case 149: {
		if (_version < kVersionV1_2_7_2)
			break;

		if (_version < kVersionV1_2_8) {
			_expressionStackPointer -= 3;
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): extracting an icon from a program file is not supported");
			_expressionStack[_expressionStackPointer - 1] = 0;
			break;
		}

		_expressionStackPointer--;
		uint16 id = _expressionStack[_expressionStackPointer];

		if ((id & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
			break;

		char *text = getStringFromId(id);
		if (text)
			Common::strlcpy(text, "guest", TG_HOST_V1_2_STRING_SLOT_SIZE);

		break;
	}

	case 150: {
		if (_version < kVersionV1_2_7_2)
			break;

		if (_version < kVersionV1_2_8) {
			_expressionStackPointer--;
			break;
		}

		// A single argument is taken in version 1.2.8, whatever the count
		uint16 numArguments = 1;
		if (_version >= kVersionV1_2_9) {
			if (argc > 2)
				numArguments = 3;
			else if (argc > 1)
				numArguments = 2;
		}

		_expressionStackPointer -= numArguments;
		const char *file = getStringFromId(_expressionStack[_expressionStackPointer]);

		if (file)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): launching the program '%s' is not supported", file);

		_expressionStack[_expressionStackPointer - 1] = 0;
		break;
	}

	case 151:
		if (_version >= kVersionV1_2_9) {
			uint16 script = _expressionStack[_expressionStackPointer - 1];

			_waveStartedScript = script ? (uint16)(script - literalBias) : 0;
		} else if (_version == kVersionV1_2_8) {
			_expressionStackPointer--;

			if (_expressionStack[_expressionStackPointer] == 0)
				warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): hiding the main window is not supported");
		} else if (_version >= kVersionV1_2_7_3) {
			_expressionStackPointer--;
			uint16 id = _expressionStack[_expressionStackPointer];

			if ((id & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
				break;

			char *text = getStringFromId(id);
			if (text)
				text[0] = '\0';
		}

		break;

	case 152: {
		if (_version < kVersionV1_2_7_3)
			break;

		if (_version == kVersionV1_2_8) {
			_expressionStackPointer -= argc;

			uint16 result = 0;
			if (argc > 0)
				result = (_expressionStack[_expressionStackPointer] == 0) ? 1 : 0;
			else
				warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 152 has no argument");

			_expressionStack[_expressionStackPointer - 1] = result;
			break;
		}

		_expressionStackPointer--;
		int16 deltaY = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		int16 deltaX = (int16)_expressionStack[_expressionStackPointer];

		if (_version >= kVersionV1_2_12)
			spriteLib()->spriteScroll(deltaX, deltaY, true);
		else
			spriteLib()->spriteScrollScreen(deltaX, deltaY);

		break;
	}

	case 153:
		if (_version >= kVersionV1_2_9) {
			_expressionStackPointer--;
			audioLib()->audioPauseMidi(_expressionStack[_expressionStackPointer]);
		}

		break;

	case 154:
		if (_version >= kVersionV1_2_9)
			_wallpaperSelectionEnabled = ((byte)_expressionStack[_expressionStackPointer - 1] != 0);

		break;

	case 155: {
		if (_version < kVersionV1_2_10)
			break;

		_expressionStackPointer -= 3;
		uint16 id = _expressionStack[_expressionStackPointer + 2];

		// Constant strings are left alone
		if ((id & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
			break;

		char *text = getStringFromId(id);

		if (text)
			text[0] = '\0';

		break;
	}

	case 156:
		if (_version >= kVersionV1_2_10)
			_expressionStackPointer -= 3;

		break;

	case 157:
		_expressionStackPointer--;
		warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): posting a global atom to the main window is not supported");
		break;

	case 158:
	case 159: {
		uint16 numIndices = (uint16)(argc - 1);
		uint16 indices[TG_BUILTIN_V1_2_12_MAX_TABLE_DIMENSIONS] = {};
		uint16 value = 0;
		bool tooManyIndices = false;

		if (code == 159)
			numIndices--;

		for (int16 i = (int16)numIndices - 1; i >= 0; i--) {
			_expressionStackPointer--;

			if (i < TG_BUILTIN_V1_2_12_MAX_TABLE_DIMENSIONS)
				indices[i] = _expressionStack[_expressionStackPointer];
			else
				tooManyIndices = true;
		}

		if (code == 159) {
			_expressionStackPointer--;
			value = _expressionStack[_expressionStackPointer];
		}

		_expressionStackPointer--;
		uint16 table = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(table - literalBias), kResV1_2_12Table))
			break;

		if (!isRecordIndexValid(table, "evaluateBuiltinFunction"))
			break;

		const byte *object = _records[table].data;
		uint32 size = _records[table].size;
		uint16 result = 0;

		if (tooManyIndices)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): the table access has too many indices");
		else if (object && size >= TG_BUILTIN_V1_2_12_TABLE_HEADER_SIZE && READ_LE_UINT16(object + TG_BUILTIN_V1_2_12_TABLE_DIMENSIONS) == numIndices) {
			uint16 offset = 0;

			for (uint16 i = 0; i < numIndices; i++)
				offset = (uint16)(offset + (uint16)((uint16)(READ_LE_UINT16(object + TG_BUILTIN_V1_2_12_TABLE_STRIDES + 2 * i) * indices[i]) << 1));

			uint32 count = READ_LE_UINT16(object + TG_BUILTIN_V1_2_12_TABLE_COUNT);

			if (offset < 2 * count && TG_BUILTIN_V1_2_12_TABLE_HEADER_SIZE + (uint32)offset + 2 <= size) {
				byte *element = _records[table].data + TG_BUILTIN_V1_2_12_TABLE_HEADER_SIZE + offset;

				if (code == 158) {
					result = READ_LE_UINT16(element);
				} else {
					WRITE_LE_UINT16(element, value);
					result = 1;
				}
			}
		}

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 160:
	case 161:
	case 162: {
		uint16 dimension = 0;

		if (code != 160) {
			_expressionStackPointer--;
			dimension = _expressionStack[_expressionStackPointer];
		}

		_expressionStackPointer--;
		uint16 table = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(table - literalBias), kResV1_2_12Table))
			break;

		if (!isRecordIndexValid(table, "evaluateBuiltinFunction"))
			break;

		const byte *object = _records[table].data;
		uint32 size = _records[table].size;
		uint16 result = 0;

		if (code == 160) {
			if (object && size >= TG_BUILTIN_V1_2_12_TABLE_HEADER_SIZE)
				result = READ_LE_UINT16(object + TG_BUILTIN_V1_2_12_TABLE_DIMENSIONS);
		} else if (object && TG_BUILTIN_V1_2_12_TABLE_SIZES + 2 * (uint32)dimension + 2 <= size) {
			result = READ_LE_UINT16(object + TG_BUILTIN_V1_2_12_TABLE_SIZES + 2 * dimension);
		}

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 163: {
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];
		uint16 result = 0;

		if (validateAndGetResourceByType((uint16)(sprite - literalBias), spriteType) && isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			result = spriteLib()->spriteGetNumCels(_records[sprite].handle);

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 164: {
		_expressionStackPointer--;
		uint16 wave = _expressionStack[_expressionStackPointer];
		uint16 result = 0;

		if (validateAndGetResourceByType((uint16)(wave - literalBias), kResV1_2_12Wave) && isRecordIndexValid(wave, "evaluateBuiltinFunction"))
			result = (uint16)audioLib()->audioGetWaveSoundTime(_records[wave].handle);

		_expressionStack[_expressionStackPointer - 1] = result;
		break;
	}

	case 165:
	case 166:
	case 169: {
		Common::Rect rect;

		_expressionStackPointer--;
		rect.bottom = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		rect.right = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		rect.top = (int16)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		rect.left = (int16)_expressionStack[_expressionStackPointer];

		if (code == 166) {
			setScrollBox(rect);
			break;
		}

		if (code == 165) {
			spriteLib()->spriteSetClipBox(&rect);
			break;
		}

		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateAndGetResourceByType((uint16)(sprite - literalBias), spriteType))
			break;

		if (!isRecordIndexValid(sprite, "evaluateBuiltinFunction"))
			break;

		spriteLib()->spriteSetSpriteClipBox(_records[sprite].handle, &rect);
		break;
	}

	case 167: {
		_expressionStackPointer--;
		uint16 bottomAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 rightAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 topAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 leftAddress = _expressionStack[_expressionStackPointer];
		Common::Rect box = spriteLib()->spriteGetScrollBox();

		writeAddress(leftAddress, (uint16)box.left);
		writeAddress(topAddress, (uint16)box.top);
		writeAddress(rightAddress, (uint16)box.right);
		writeAddress(bottomAddress, (uint16)box.bottom);
		break;
	}

	case 168: {
		_expressionStackPointer--;
		uint16 yAddress = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 xAddress = _expressionStack[_expressionStackPointer];
		Common::Point position = spriteLib()->spriteGetScrollPos();

		writeAddress(xAddress, (uint16)position.x);
		writeAddress(yAddress, (uint16)position.y);
		break;
	}

	case 170:
	case 171: {
		byte record[TG_SCRIPT_V1_2_6_GOTO_RECORD_SIZE];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4GotoScene);
		_expressionStackPointer--;
		record[TG_SCRIPT_V1_2_6_GOTO_RELEASE_OFFSET] = (byte)_expressionStack[_expressionStackPointer];

		if (code == 170) {
			_expressionStackPointer--;
			const char *name = getStringFromId(_expressionStack[_expressionStackPointer]);

			if (name) {
				if (strlen(name) >= TG_HOST_V1_2_6_SCENE_NAME_SIZE)
					warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): the scene name '%s' is too long", name);

				Common::strlcpy((char *)record + TG_SCRIPT_V1_2_4_GOTO_NAME_OFFSET, name, TG_HOST_V1_2_6_SCENE_NAME_SIZE);
			}
		} else {
			record[TG_SCRIPT_V1_2_6_GOTO_BACK_OFFSET] = 1;
		}

		o_gotoScene(record);
		break;
	}

	case 173: {
		_expressionStackPointer--;
		uint16 argument2 = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 argument1 = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 script = _expressionStack[_expressionStackPointer];

		enqueuePostedScriptRun((uint16)(script - literalBias), (uint16)(argument1 - literalBias), (uint16)(argument2 - literalBias));
		break;
	}

	case 174:
	case 175: {
		byte record[18];
		uint16 listFlag = 0x10;
		uint16 scripts = 4;

		if (_version >= kVersionV2_0) {
			listFlag = 4;
			scripts = 6;
		}

		memset(record, 0, sizeof(record));
		record[listFlag] = 1;
		_expressionStackPointer--;
		uint16 list = _expressionStack[_expressionStackPointer];
		WRITE_LE_UINT16(record + scripts, (uint16)(list - literalBias));

		int32 entry = -1;

		for (uint32 i = 0; i < _scriptCursors.size() && entry < 0; i++) {
			if (_scriptCursors[i].id == list)
				entry = i;
		}

		if (code == 174) {
			WRITE_LE_UINT16(record, kOpV1_2_4RandomRunScript);

			if (entry >= 0)
				record[3] = (byte)_scriptCursors[entry].randomIndex;

			o_randomRunScript(record);
		} else {
			WRITE_LE_UINT16(record, kOpV1_2_4RoundRobinRunScript);

			if (entry >= 0)
				record[3] = (byte)_scriptCursors[entry].sequentialIndex;

			o_roundRobinRunScript(record);
		}

		entry = -1;

		for (uint32 i = 0; i < _scriptCursors.size() && entry < 0; i++) {
			if (_scriptCursors[i].id == list)
				entry = i;
		}

		if (entry < 0) {
			ScriptCursor_v1_2_12 cursor;

			cursor.id = list;
			_scriptCursors.push_back(cursor);
			entry = _scriptCursors.size() - 1;
		}

		if (code == 174)
			_scriptCursors[entry].randomIndex = record[3];
		else
			_scriptCursors[entry].sequentialIndex = record[3];

		break;
	}

	case 177: {
		byte record[10];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4DeviceGroupButtonState);
		_expressionStackPointer--;

		if (_expressionStack[_expressionStackPointer] != 0)
			record[7] = 1;
		else
			record[8] = 1;

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_deviceGroupButtonState(record);
		break;
	}

	case 178:
	case 179:
	case 192:
	case 228:
	case 229:
	case 230: {
		byte record[TG_SCRIPT_V1_2_7_SET_BACKGROUND_SIZE];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetBackground);

		if (code == 178 || code == 228 || code == 192 || code == 229)
			WRITE_LE_UINT16(record + 2, 1);

		// The last three arguments of the variants with a wipe describe the effect
		if (code == 228 || code == 229 || code == 230) {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 0x0E, _expressionStack[_expressionStackPointer]);
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 0x0C, _expressionStack[_expressionStackPointer]);
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 0x0A, _expressionStack[_expressionStackPointer]);
		}

		if (code == 178 || code == 228) {
			_expressionStackPointer--;
			byte blue = (byte)_expressionStack[_expressionStackPointer];
			_expressionStackPointer--;
			byte green = (byte)_expressionStack[_expressionStackPointer];
			_expressionStackPointer--;
			byte red = (byte)_expressionStack[_expressionStackPointer];
			byte color[TG_DISPLAY_PALETTE_ENTRY_SIZE] = { red, green, blue, 1 };
			int16 colorIndex = -1;

			for (uint16 i = 0; i < _filePaletteEntryCount && colorIndex < 0; i++) {
				if (memcmp(_palette + (TG_SCRIPT_V1_2_4_FIRST_PALETTE_ENTRY + i) * TG_DISPLAY_PALETTE_ENTRY_SIZE, color, sizeof(color)) == 0)
					colorIndex = (int16)(TG_SCRIPT_V1_2_4_FIRST_PALETTE_ENTRY + i);
			}

			if (colorIndex < 0 && (uint16)(TG_SCRIPT_V1_2_4_MAX_PALETTE_ENTRIES - _maxTransColors) > _filePaletteEntryCount) {
				colorIndex = (int16)(TG_SCRIPT_V1_2_4_FIRST_PALETTE_ENTRY + _filePaletteEntryCount);
				memcpy(_palette + colorIndex * TG_DISPLAY_PALETTE_ENTRY_SIZE, color, sizeof(color));
				_filePaletteEntryCount++;
			}

			if (colorIndex < 0) {
				uint16 nearest = TG_BUILTIN_V1_2_12_MAX_COLOR_DISTANCE;

				colorIndex = TG_BUILTIN_V1_2_12_NO_NEAREST_COLOR;

				for (uint16 i = 0; i < _filePaletteEntryCount; i++) {
					const byte *entry = _palette + (TG_SCRIPT_V1_2_4_FIRST_PALETTE_ENTRY + i) * TG_DISPLAY_PALETTE_ENTRY_SIZE;
					uint16 distance = (uint16)(ABS((int16)entry[2] - (int16)blue) + ABS((int16)entry[0] - (int16)red) + ABS((int16)entry[1] - (int16)green));

					if (distance < nearest) {
						nearest = distance;
						colorIndex = (int16)(TG_SCRIPT_V1_2_4_FIRST_PALETTE_ENTRY + i);
					}
				}
			}

			if (_version >= kVersionV2_0)
				record[9] = (byte)colorIndex;
			else
				WRITE_LE_UINT16(record + 8, (uint16)colorIndex);
		} else if (code == 192 || code == 229) {
			_expressionStackPointer--;

			if (_version >= kVersionV2_0)
				record[9] = (byte)_expressionStack[_expressionStackPointer];
			else
				WRITE_LE_UINT16(record + 8, _expressionStack[_expressionStackPointer]);
		} else {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			WRITE_LE_UINT16(record + 6, READ_LE_UINT16(record + 4));
		}

		o_setBackground(record);
		break;
	}

	case 180: {
		byte record[6];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetDeviceGroupScript);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_setDeviceGroupScript(record);
		break;
	}

	case 181: {
		byte record[12];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetPaletteEntries);
		record[6] = 1;
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, _expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_setPaletteEntries(record);
		break;
	}

	case 182:
		_expressionStackPointer -= 3;
		warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): setting the wallpaper is not supported");
		break;

	case 183:
	case 184:
	case 185: {
		byte record[TG_SCRIPT_V1_2_9_SET_HOVER_REGION_SIZE];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetHoverRegion);

		if (code == 185) {
			record[0x14] = 1;
		} else {
			if (code == 183) {
				_expressionStackPointer--;
				WRITE_LE_UINT16(record + 0x0C, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
				_expressionStackPointer--;
				WRITE_LE_UINT16(record + 0x0A, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			}

			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 8, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		}

		o_setHoverRegion(record);
		break;
	}

	case 186:
	case 187:
	case 188: {
		byte record[TG_SCRIPT_V1_2_7_1_SET_CLICK_SCRIPT_SIZE];
		uint16 rectOffset = 6;
		uint16 argumentOffset = 0x0E;

		if (_version >= kVersionV2_0) {
			rectOffset = 8;
			argumentOffset = 6;
		}

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetClickScript);

		if (code == 186) {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + argumentOffset, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		} else if (code == 187) {
			_expressionStackPointer--;
			bool enable = (_expressionStack[_expressionStackPointer] != 0);

			if (_version >= kVersionV2_0)
				record[0x10] |= enable ? TG_SCRIPT_V2_0_CLICK_ENABLE : TG_SCRIPT_V2_0_CLICK_DISABLE;
			else if (enable)
				record[0x11] = 1;
			else
				record[0x12] = 1;
		}

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + rectOffset + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + rectOffset + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + rectOffset + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + rectOffset, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_setClickScript(record);
		break;
	}

	case 189:
	case 190:
	case 191: {
		byte record[TG_SCRIPT_V1_2_7_1_SET_CLICK_SCRIPT_SIZE];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetClickScript);
		record[0x10] = 1;

		if (code == 189) {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		} else if (code == 190) {
			_expressionStackPointer--;
			bool enable = (_expressionStack[_expressionStackPointer] != 0);

			if (_version >= kVersionV2_0)
				record[0x10] |= enable ? TG_SCRIPT_V2_0_CLICK_ENABLE : TG_SCRIPT_V2_0_CLICK_DISABLE;
			else if (enable)
				record[0x11] = 1;
			else
				record[0x12] = 1;
		}

		o_setClickScript(record);
		break;
	}

	case 193: {
		byte record[10];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4DeviceGroupButtonState);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, _expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;

		if (_expressionStack[_expressionStackPointer] > 5)
			record[6] = 2;
		else
			record[6] = (byte)_expressionStack[_expressionStackPointer];

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_deviceGroupButtonState(record);
		break;
	}

	case 194:
	case 195:
	case 196: {
		byte record[TG_SCRIPT_V1_2_7_1_SET_CLICK_SCRIPT_SIZE];
		uint16 argumentOffset = 0x0E;

		if (_version >= kVersionV2_0)
			argumentOffset = 6;

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetClickScript);

		if (code == 194) {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + argumentOffset, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		} else if (code == 195) {
			_expressionStackPointer--;
			bool enable = (_expressionStack[_expressionStackPointer] != 0);

			if (_version >= kVersionV2_0)
				record[0x10] |= enable ? TG_SCRIPT_V2_0_CLICK_ENABLE : TG_SCRIPT_V2_0_CLICK_DISABLE;
			else if (enable)
				record[0x11] = 1;
			else
				record[0x12] = 1;
		}

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_setClickScript(record);
		break;
	}

	case 197: {
		byte record[6];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SpriteCompositeToBackground);
		_expressionStackPointer--;
		record[4] = (_expressionStack[_expressionStackPointer] != 0) ? 1 : 0;
		_expressionStackPointer--;

		if (_expressionStack[_expressionStackPointer] != 0)
			WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));

		o_spriteCompositeToBackground(record);
		break;
	}

	case 198: {
		byte record[18];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4AddCollisionPair);
		_expressionStackPointer--;
		record[0x10] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		record[0x0C] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 0x0A, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 8, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		record[0x0F] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		record[0x0E] = (byte)_expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_addCollisionPair(record);
		break;
	}

	case 199: {
		byte record[18];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4AddCollisionPair);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));

		if (READ_LE_UINT16(record + 2) == 0 && READ_LE_UINT16(record + 4) == 0)
			record[0x0D] = 1;

		o_addCollisionPair(record);
		break;
	}

	case 200:
	case 201: {
		byte record[10];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetSpriteObjectProperties);

		if (code == 200) {
			_expressionStackPointer--;
			record[8] = (byte)_expressionStack[_expressionStackPointer];
		} else {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		}

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_setSpriteObjectProperties(record);
		break;
	}

	case 202: {
		_expressionStackPointer -= 4;
		_expressionStackPointer--;
		uint16 sprite = _expressionStack[_expressionStackPointer];

		if (!validateResourceType((uint16)(sprite - literalBias), spriteType))
			break;

		// The bitmap identifiers are stored through a null segment
		warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 202 cannot store the bitmap identifiers");
		break;
	}

	case 203:
	case 227: {
		uint16 data = 0;
		bool flag = false;

		_expressionStackPointer--;

		if (code == 203) {
			if (_expressionStack[_expressionStackPointer] != 0)
				data = (uint16)(_expressionStack[_expressionStackPointer] - literalBias);
		} else {
			flag = (_expressionStack[_expressionStackPointer] != 0);
		}

		_expressionStackPointer--;
		bool result = o_setSpriteQueue((uint16)(_expressionStack[_expressionStackPointer] - literalBias), data, flag);

		_expressionStack[_expressionStackPointer - 1] = result ? 1 : 0;
		break;
	}

	case 204: {
		byte record[6];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4CancelScriptTimer);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		record[4] = (_expressionStack[_expressionStackPointer] == 0xFFFF) ? 1 : 0;
		o_cancelScriptTimer(record);
		break;
	}

	case 205: {
		byte record[12];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetScriptTimer);
		_expressionStackPointer--;
		record[0x0B] = (_expressionStack[_expressionStackPointer] != 0) ? 1 : 0;
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;

		if (_version >= kVersionV2_0)
			WRITE_LE_UINT32(record + 6, _expressionStack[_expressionStackPointer]);
		else
			WRITE_LE_UINT32(record + 6, (uint32)(int32)(int16)_expressionStack[_expressionStackPointer]);

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_setScriptTimer(record);
		break;
	}

	case 206: {
		_expressionStackPointer--;
		uint16 deltaY = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 deltaX = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 second = _expressionStack[_expressionStackPointer];
		_expressionStackPointer--;
		uint16 first = _expressionStack[_expressionStackPointer];

		_expressionStack[_expressionStackPointer - 1] = builtinBlitBitmapOverlap(first, second, (int16)deltaX, (int16)deltaY);
		break;
	}

	case 207:
		// The date is stored through a null segment
		_expressionStackPointer -= 4;

		if (_expressionStack[_expressionStackPointer] != 0 || _expressionStack[_expressionStackPointer + 1] != 0 ||
				_expressionStack[_expressionStackPointer + 2] != 0 || _expressionStack[_expressionStackPointer + 3] != 0)
			warning("TopgunEngine_v1_2_4::evaluateBuiltinFunction(): built-in 207 cannot store the date");

		break;

	case 209:
		audioLib()->audioMute(-1);
		break;

	case 210:
		_expressionStackPointer--;
		audioLib()->audioMute((_expressionStack[_expressionStackPointer] != 0) ? 1 : 0);
		break;

	case 211: {
		byte record[12];
		uint16 blockOffset = 4;

		if (_version >= kVersionV2_0)
			blockOffset = 6;

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4PlayMidi);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + blockOffset, _expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		bool flag = (_expressionStack[_expressionStackPointer] != 0);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + blockOffset + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));

		if (!flag && READ_LE_UINT16(record + blockOffset + 4) == 0)
			WRITE_LE_UINT16(record + blockOffset + 2, 0);
		else
			WRITE_LE_UINT16(record + blockOffset + 2, 1);

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_playMidi(record);
		break;
	}

	case 212: {
		byte parameters[TG_AUDIO_V1_2_4_PLAY_PARAMS_SIZE];

		memset(parameters, 0, sizeof(parameters));
		_expressionStackPointer--;
		bool notify = (_expressionStack[_expressionStackPointer] != 0);

		if (_version < kVersionV2_0)
			parameters[3] = notify ? 1 : 0;
		else if (notify)
			parameters[2] = TG_SCRIPT_V2_0_WAVE_NOTIFY;

		_expressionStackPointer--;
		o_playWave((uint16)(_expressionStack[_expressionStackPointer] - literalBias), parameters);
		break;
	}

	case 213: {
		byte parameters[TG_AUDIO_V1_2_4_PLAY_PARAMS_SIZE];
		uint16 volumeOffset = 5;
		uint16 fadeInOffset = 6;
		uint16 fadeOutOffset = 0x0A;
		uint16 minimumDelayOffset = 0x0E;
		uint16 maximumDelayOffset = 0x12;
		uint16 notifyScriptOffset = 0x16;

		if (_version >= kVersionV2_0) {
			volumeOffset = 3;
			fadeInOffset = 4;
			fadeOutOffset = 8;
			minimumDelayOffset = 0x0C;
			maximumDelayOffset = 0x10;
			notifyScriptOffset = 0x14;
		}

		memset(parameters, 0, sizeof(parameters));
		_expressionStackPointer--;
		int32 maximumDelay = (int16)_expressionStack[_expressionStackPointer];
		WRITE_LE_UINT32(parameters + maximumDelayOffset, (uint32)maximumDelay);
		_expressionStackPointer--;
		int32 minimumDelay = (int16)_expressionStack[_expressionStackPointer];
		WRITE_LE_UINT32(parameters + minimumDelayOffset, (uint32)minimumDelay);
		_expressionStackPointer--;
		uint16 loops = _expressionStack[_expressionStackPointer];

		if ((minimumDelay != 0 || maximumDelay != 0) && loops == 0)
			loops = 0xFFFF;

		WRITE_LE_UINT16(parameters, loops);
		_expressionStackPointer--;
		WRITE_LE_UINT32(parameters + fadeOutOffset, (uint32)(int32)(int16)_expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		WRITE_LE_UINT32(parameters + fadeInOffset, (uint32)(int32)(int16)_expressionStack[_expressionStackPointer]);
		_expressionStackPointer--;
		parameters[volumeOffset] = (byte)_expressionStack[_expressionStackPointer];

		if (parameters[volumeOffset] > TG_BUILTIN_V1_2_12_MAX_WAVE_VOLUME)
			parameters[volumeOffset] = TG_BUILTIN_V1_2_12_MAX_WAVE_VOLUME;

		_expressionStackPointer--;

		if (_version < kVersionV2_0)
			parameters[2] = (byte)_expressionStack[_expressionStackPointer];
		else if ((_expressionStack[_expressionStackPointer] & 1) != 0)
			parameters[2] |= TG_SCRIPT_V2_0_WAVE_HAS_VOLUME;

		_expressionStackPointer--;
		bool noFlush = (_expressionStack[_expressionStackPointer] != 0);

		if (_version < kVersionV2_0)
			parameters[4] = noFlush ? 1 : 0;
		else if (noFlush)
			parameters[2] |= TG_SCRIPT_V2_0_WAVE_NO_FLUSH;

		_expressionStackPointer--;
		bool flag = (_expressionStack[_expressionStackPointer] != 0);
		_expressionStackPointer--;
		WRITE_LE_UINT16(parameters + notifyScriptOffset, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));

		bool notify = (flag || READ_LE_UINT16(parameters + notifyScriptOffset) != 0);

		if (_version < kVersionV2_0)
			parameters[3] = notify ? 1 : 0;
		else if (notify)
			parameters[2] |= TG_SCRIPT_V2_0_WAVE_NOTIFY;

		_expressionStackPointer--;
		o_playWave((uint16)(_expressionStack[_expressionStackPointer] - literalBias), parameters);
		break;
	}

	case 214: {
		byte record[14];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetMidiParams);
		record[5] = 1;
		_expressionStackPointer--;
		uint16 denominator = _expressionStack[_expressionStackPointer];

		if (denominator == 0)
			denominator = 1;

		denominator -= TG_HOST_V1_2_4_LITERAL_BIAS;
		_expressionStackPointer--;
		uint16 numerator = _expressionStack[_expressionStackPointer];

		if (numerator == 0)
			denominator = 1;

		numerator -= TG_HOST_V1_2_4_LITERAL_BIAS;
		WRITE_LE_UINT16(record + 0x0A, denominator);
		WRITE_LE_UINT16(record + 0x0C, numerator);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - TG_HOST_V1_2_4_LITERAL_BIAS));
		o_setMidiParams(record);
		break;
	}

	case 215: {
		byte record[14];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetMidiParams);
		record[5] = 1;
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - TG_HOST_V1_2_4_LITERAL_BIAS));
		o_setMidiParams(record);
		break;
	}

	case 216: {
		byte record[16];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetMidiParams);
		record[4] = 1;
		_expressionStackPointer--;
		uint16 enable = (_expressionStack[_expressionStackPointer] != 0) ? 1 : 0;
		WRITE_LE_UINT16(record + 0x0E, enable);
		_expressionStackPointer--;
		uint16 channel = _expressionStack[_expressionStackPointer];

		if (channel > TG_BUILTIN_V1_2_12_MAX_MIDI_CHANNEL)
			channel = TG_BUILTIN_V1_2_12_MAX_MIDI_CHANNEL;

		if (channel != 0)
			channel--;

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - TG_HOST_V1_2_4_LITERAL_BIAS));
		WRITE_LE_UINT16(record + 6, (uint16)(1 << channel));
		WRITE_LE_UINT16(record + 8, (uint16)(enable << channel));
		o_setMidiParams(record);
		break;
	}

	case 217:
	case 218:
	case 219:
	case 220: {
		byte record[4];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetVolume);

		// The wave volume is reached only by the kind 3 that no built-in selects
		if (code == 218)
			record[3] = 1;
		else if (code == 219)
			record[3] = 2;

		_expressionStackPointer--;
		record[2] = ((int16)_expressionStack[_expressionStackPointer] >= 0) ? 1 : 0;
		o_setVolume(record);
		break;
	}

	case 221: {
		_expressionStackPointer--;
		uint16 id = (uint16)(_expressionStack[_expressionStackPointer] - literalBias);
		uint16 type = resourceLib()->resGetType(_file, operandValue(id));

		_expressionStack[_expressionStackPointer - 1] = isResourceObjectLoaded(id, type) ? 1 : 0;
		break;
	}

	case 222: {
		byte record[12];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4TransferSprite);
		_expressionStackPointer--;

		if (_version >= kVersionV2_0)
			record[8] = (_expressionStack[_expressionStackPointer] != 0) ? 1 : 0;
		else
			record[0x0A] = (byte)_expressionStack[_expressionStackPointer];

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 6, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		o_transferSprite(record);
		break;
	}

	case 223: {
		byte record[4];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4SetMouseEventScript);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_setMouseEventScript(record);
		break;
	}

	case 224: {
		byte record[10];

		memset(record, 0, sizeof(record));
		WRITE_LE_UINT16(record, kOpV1_2_4ListRunScript);
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		record[8] = (READ_LE_UINT16(record + 4) == 0xFFFF) ? 1 : 0;
		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		_expressionStackPointer--;
		bool advance = (_expressionStack[_expressionStackPointer] != 0);

		if (_version < kVersionV2_0)
			record[9] = advance ? 1 : 0;
		else if (advance)
			record[8] |= TG_SCRIPT_V2_0_LIST_ADVANCE;

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 6, _expressionStack[_expressionStackPointer]);
		o_listRunScript(record);
		break;
	}

	case 225: {
		_expressionStackPointer--;
		uint16 script = (uint16)(_expressionStack[_expressionStackPointer] - literalBias);

		if (script == 0) {
			_numClickRegions = 0;
		} else {
			for (uint16 i = 0; i < _numClickRegions; i++) {
				_clickRegions[i].script = script;
				_clickRegions[i].disabled = 0;
			}
		}

		break;
	}

	case 226: {
		_expressionStackPointer--;
		uint16 script = (uint16)(_expressionStack[_expressionStackPointer] - literalBias);

		if (!_records)
			break;

		for (uint16 i = 0; i < _numResources; i++) {
			if (resourceLib()->resGetType(_file, i) == spriteType && _records[i].data)
				WRITE_LE_UINT16(_records[i].data + TG_INPUT_V1_2_4_OBJECT_CLICK_SCRIPT, script);
		}

		break;
	}

	case 231:
	case 232:
	case 233:
	case 234:
	case 235:
	case 236: {
		byte record[12];

		memset(record, 0, sizeof(record));

		if (code == 231 || code == 232) {
			_expressionStackPointer--;
			WRITE_LE_UINT16(record + 4, (uint16)(_expressionStack[_expressionStackPointer] - literalBias));
		}

		if (code == 235 || code == 236) {
			_expressionStackPointer--;
			bool enable = (_expressionStack[_expressionStackPointer] != 0);

			if (_version >= kVersionV2_0)
				record[6] |= enable ? TG_SCRIPT_V2_0_KEY_ENABLE : TG_SCRIPT_V2_0_KEY_DISABLE;
			else if (enable)
				record[8] = 1;
			else
				record[9] = 1;
		}

		if (code == 232 || code == 234 || code == 236) {
			_expressionStackPointer--;

			if (_version < kVersionV2_0)
				record[6] = (byte)_expressionStack[_expressionStackPointer];
			else if ((_expressionStack[_expressionStackPointer] & 1) != 0)
				record[6] |= TG_SCRIPT_V2_0_KEY_CTRL;

			_expressionStackPointer--;

			if (_version < kVersionV2_0)
				record[7] = (byte)_expressionStack[_expressionStackPointer];
			else if ((_expressionStack[_expressionStackPointer] & 1) != 0)
				record[6] |= TG_SCRIPT_V2_0_KEY_SHIFT;
		}

		_expressionStackPointer--;
		WRITE_LE_UINT16(record + 2, _expressionStack[_expressionStackPointer]);
		o_bindKeyScript(record);
		break;
	}

	default:
		break;
	}
}

uint16 TopgunEngine_v1_2_4::builtinBlitBitmapOverlap(uint16 first, uint16 second, int16 deltaX, int16 deltaY) {
	uint16 literalBias = TG_HOST_V1_2_4_LITERAL_BIAS;
	if (_version >= kVersionV2_0)
		literalBias = 0;

	if (!validateAndGetResourceByType((uint16)(first - literalBias), kResV1_2_4Bitmap))
		return 0;

	uint16 textType = kResV1_2_4Text;
	if (_version >= kVersionV1_2_12)
		textType = kResV1_2_12Text;
	else if (_version >= kVersionV1_2_6)
		textType = kResV1_2_6Text;

	uint16 type = resourceLib()->resGetType(_file, second);
	if (type != kResV1_2_4Bitmap && type != textType)
		return 0;

	if (!validateAndGetResourceByType((uint16)(second - literalBias), type))
		return 0;

	if (!isRecordIndexValid(first, "builtinBlitBitmapOverlap") || !isRecordIndexValid(second, "builtinBlitBitmapOverlap"))
		return 0;

	byte *firstBitmap = _records[first].data;
	byte *secondBitmap = _records[second].data;
	Common::Rect firstBox;
	Common::Rect secondBox;

	spriteLib()->spriteDibGetBox(firstBitmap, firstBox);
	spriteLib()->spriteDibGetBox(secondBitmap, secondBox);
	secondBox.translate(deltaX, deltaY);

	if (!firstBox.intersects(secondBox))
		return 0;

	Common::Rect overlap = firstBox.findIntersectingRect(secondBox);

	spriteLib()->spriteDibMerge(firstBitmap, secondBitmap, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		(int16)(overlap.left - firstBox.left), (int16)(overlap.top - firstBox.top),
		(int16)(overlap.left - secondBox.left), (int16)(overlap.top - secondBox.top));
	return 1;
}

uint16 TopgunEngine_v1_2_4::builtinCopyResource(uint16 id) {
	uint16 spriteType = kResV1_2_4Sprite;
	if (_version >= kVersionV1_2_12)
		spriteType = kResV1_2_12Sprite;
	else if (_version >= kVersionV1_2_6)
		spriteType = kResV1_2_6Sprite;

	uint16 type = resourceLib()->resGetType(_file, id);
	if (type != spriteType && (_version < kVersionV1_2_12 || type != kResV1_2_12Table))
		return 0;

	if (_version >= kVersionV2_0 && !validateAndGetResourceByType(id, type))
		return 0;

	uint16 copy = resourceLib()->resCopyResource(_file, id);

	if (copy != 0 && _version >= kVersionV1_2_4 && _version < kVersionV2_0)
		validateAndGetResourceByType((uint16)(copy - TG_HOST_V1_2_4_LITERAL_BIAS), type);

	return copy;
}

uint16 TopgunEngine_v1_2_4::builtinDeleteResource(uint16 id) {
	uint16 spriteType = kResV1_2_4Sprite;
	if (_version >= kVersionV1_2_12)
		spriteType = kResV1_2_12Sprite;
	else if (_version >= kVersionV1_2_6)
		spriteType = kResV1_2_6Sprite;

	uint16 type = resourceLib()->resGetType(_file, id);
	if (type != spriteType && (_version < kVersionV1_2_12 || type != kResV1_2_12Table))
		return 0;

	uint16 literalBias = TG_HOST_V1_2_4_LITERAL_BIAS;
	if (_version >= kVersionV2_0)
		literalBias = 0;

	if (!destroyResourceObjectByType((uint16)(id - literalBias), type))
		return 0;

	if (!resourceLib()->resDeleteResource(_file, id))
		return 0;

	if (!isRecordIndexValid(id, "builtinDeleteResource"))
		return 0;

	_records[id].data = nullptr;
	return 1;
}

} // End of namespace Topgun
