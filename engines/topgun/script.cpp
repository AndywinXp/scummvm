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
#include "common/system.h"
#include "common/textconsole.h"

#include "topgun/libraries/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/topgun_v1_0.h"

namespace Topgun {

#define TG_SCRIPT_DEBUG_STRING_SIZE 51

enum SpriteEvent {
	kEventBreakLoops = 16,
	kEventSceneEnd = 17,
	kEventLoadResource = 18,
	kEventSetLevel = 19,
	kEventPlayCDTrack = 20,
	kEventPlayWave = 21,
	kEventRunScript = 22,
	kEventSetQueue = 23,
	kEventStopCD = 24,
	kEventStopWave = 25,
	kEventTransferSprite = 26,
	kEventDestroyResource = 27,
	kEventTrimQueue = 28
};

void TopgunEngine_v1_0::serviceScheduledMsgTimerQueue() {
	uint16 index = 0;

	if (_numTimers == 0)
		return;

	do {
		uint32 now = getEngineMillis();

		if (_timers[index].expiry <= now) {
			uint16 script = _timers[index].script;
			removeScheduledTimer(index);

			if (_numTimers > index)
				index--;

			runScriptResource(script);
		}

		index++;
	} while (index < _numTimers);
}

void TopgunEngine_v1_0::removeScheduledTimer(uint16 index) {
	if (_numTimers == 0 || index >= _numTimers) {
		warning("TopgunEngine_v1_0::removeScheduledTimer(): timer index %d is out of range", index);
		return;
	}

	_numTimers--;

	uint16 count = _numTimers - index;
	if (count != 0)
		memmove(&_timers[index], &_timers[index + 1], count * sizeof(ScheduledTimerRecord));
}

bool TopgunEngine_v1_0::runScriptBytecodeLoop(uint16 scriptId) {
	if (_scriptDepth >= TG_HOST_MAX_SCRIPT_DEPTH) {
		warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): script nesting too deep");
		return true;
	}

	_scriptDepth++;

	bool keepGoing = true;
	const ResourceRecord &script = _records[scriptId];
	byte *ip = script.data;
	int16 left = (int16)(script.size & 0xFFFF);

	while (left > 0) {
		uint16 opcode = READ_LE_UINT16(ip);

		switch (opcode) {
		case kOpCallScript:
			keepGoing = runScriptResource(READ_LE_UINT16(ip + 2));
			ip += 4;
			left -= 4;
			break;

		case kOpLoadResource:
			keepGoing = validateAndGetResourceByType(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
			ip += 6;
			left -= 6;
			break;

		case kOpUnloadResource: {
			bool unloadsSelf = (READ_LE_UINT16(ip + 2) == scriptId);

			keepGoing = destroyResourceObjectByType(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));

			if (unloadsSelf) {
				left = 0;
			} else {
				ip += 6;
				left -= 6;
			}
			break;
		}

		case kOpSetClickScript:
			keepGoing = o_setClickScript(ip);
			ip += 14;
			left -= 14;
			break;

		case kOpSetSpriteQueue:
			keepGoing = o_setSpriteQueue(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
			ip += 6;
			left -= 6;
			break;

		case kOpSetBackground: {
			uint16 backgroundId = READ_LE_UINT16(ip + 2);

			if ((_flagLoMem || _flagMode640) && READ_LE_UINT16(ip + 4) != 0)
				backgroundId = READ_LE_UINT16(ip + 4);

			keepGoing = setBackgroundFromBitmapResource(backgroundId);
			ip += 6;
			left -= 6;
			break;
		}

		case kOpPlayWave:
			keepGoing = o_playWave(READ_LE_UINT16(ip + 2), ip + 4);
			ip += 11;
			left -= 11;
			break;

		case kOpPlayCDTrack:
			o_playCDTrack(ip[2]);
			ip += 4;
			left -= 4;
			break;

		case kOpFadeIn:
		case kOpFadeOut:
			o_fadeInOut(ip);
			ip += 4;
			left -= 4;
			break;

		case kOpBindKeyScript:
			o_bindKeyScript(ip);
			ip += 8;
			left -= 8;
			break;

		case kOpQuit:
			postMessage(kMsgClose, 0, 0);
			ip += 2;
			left -= 2;
			break;

		case kOpStopCD:
			o_stopCD();
			ip += 4;
			left -= 4;
			break;

		case kOpStopWave:
			keepGoing = o_stopWave(READ_LE_UINT16(ip + 2), ip + 4);
			ip += 7;
			left -= 7;
			break;

		case kOpSetFlag: {
			uint16 flag = READ_LE_UINT16(ip + 2);
			uint16 index = flag & 0x7FFF;

			if ((flag & 0x8000) != 0) {
				if (index < TG_HOST_NUM_GLOBAL_FLAGS)
					_globalFlags[index] = ip[4];
				else
					warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): global flag %d is out of range", index);
			} else if (_currentScene) {
				if (index < TG_HOST_NUM_SCENE_FLAGS)
					_currentScene->flags[index] = ip[4];
				else
					warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): scene flag %d is out of range", index);
			}

			ip += 5;
			left -= 5;
			break;
		}

		case kOpTestFlag: {
			uint16 flag = READ_LE_UINT16(ip + 2);
			uint16 index = flag & 0x7FFF;
			bool global = (flag & 0x8000) != 0;

			if (global || _currentScene) {
				uint16 value = 0;

				if (global) {
					if (index < TG_HOST_NUM_GLOBAL_FLAGS)
						value = _globalFlags[index];
					else
						warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): global flag %d is out of range", index);
				} else {
					if (index < TG_HOST_NUM_SCENE_FLAGS)
						value = _currentScene->flags[index];
					else
						warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): scene flag %d is out of range", index);
				}

				if (ip[9] != value)
					keepGoing = runScriptResource(READ_LE_UINT16(ip + 4));
				else if (ip[8] != 0)
					keepGoing = runScriptResource(READ_LE_UINT16(ip + 6));
			}

			ip += 10;
			left -= 10;
			break;
		}

		case kOpGotoScene:
			_gotoRecord.opcode = READ_LE_UINT16(ip);
			memcpy(_gotoRecord.name, ip + 2, sizeof(_gotoRecord.name));
			_gotoRecord.releaseSceneSprite = ip[15];

			if (ip[15] != 0) {
				_sceneSprite = 0;
				_sceneSpriteWave = 0;
			}

			postMessage(kMsgGotoScene, 0, 0);
			_keyScripts.clear();
			left = 0;
			break;

		case kOpSetScriptTimer:
			o_setScriptTimer(ip);
			ip += 10;
			left -= 10;
			break;

		case kOpCancelScriptTimer:
			o_cancelScriptTimer(ip);
			ip += 4;
			left -= 4;
			break;

		case kOpSetSceneEndScript:
			if (_gotoRecord.releaseSceneSprite != 0)
				postMessage(kMsgSceneEnd, 0, 0);

			_sceneEndScript = READ_LE_UINT16(ip + 2);
			ip += 4;
			left -= 4;
			break;

		case kOpRandomRunScript:
			keepGoing = o_randomRunScript(ip);
			ip += 16;
			left -= 16;
			break;

		case kOpRoundRobinRunScript:
			keepGoing = o_roundRobinRunScript(ip);
			ip += 16;
			left -= 16;
			break;

		case kOpTransferSprite:
			o_transferSprite(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4), READ_LE_UINT16(ip + 6), ip[8] != 0);
			ip += 9;
			left -= 9;
			break;

		case kOpBreakSpriteLoops: {
			uint16 id = READ_LE_UINT16(ip + 2);

			if (!_records || id >= _numResources)
				warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): resource id %d is out of range", id);
			else if (_records[id].handle != 0)
				spriteLibrary()->spriteBreakLoops(_records[id].handle);

			ip += 4;
			left -= 4;
			break;
		}

		case kOpSetSpriteQueueIfLoaded: {
			uint16 id = READ_LE_UINT16(ip + 2);

			if (!_records || id >= _numResources)
				warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): resource id %d is out of range", id);
			else if (_records[id].handle != 0)
				keepGoing = o_setSpriteQueue(id, READ_LE_UINT16(ip + 4));

			ip += 6;
			left -= 6;
			break;
		}

		case kOpDebugString: {
			uint16 flag = READ_LE_UINT16(ip + 2);
			uint16 index = flag & 0x7FFF;
			uint16 value = 0;

			if ((flag & 0x8000) != 0) {
				if (index < TG_HOST_NUM_GLOBAL_FLAGS)
					value = _globalFlags[index];
				else
					warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): global flag %d is out of range", index);
			} else {
				if (_currentScene && index < TG_HOST_NUM_SCENE_FLAGS)
					value = _currentScene->flags[index];
				else
					warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): scene flag %d cannot be read", index);
			}

			printDebugString(ip + 4, value);

			ip += 55;
			left -= 55;
			break;
		}

		case kOpSetSpriteLevel:
			o_setSpriteLevel(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
			ip += 6;
			left -= 6;
			break;

		case kOpSetActivationScript:
			_activationScript = READ_LE_UINT16(ip + 2);
			ip += 4;
			left -= 4;
			break;

		case kOpPostScript:
			postMessage(kMsgRunScript, READ_LE_UINT16(ip + 2), 0);
			ip += 4;
			left -= 4;
			break;

		case kOpCursorOff:
			_cursorEnabled = false;
			applyCursorFromRegister();
			ip += 2;
			left -= 2;
			break;

		case kOpCursorOn:
			_cursorEnabled = true;
			applyCursorFromRegister();
			ip += 2;
			left -= 2;
			break;

		case kOpRunScriptIfLoaded:
			if (isResourceObjectLoaded(READ_LE_UINT16(ip + 2)) && READ_LE_UINT16(ip + 4) != 0)
				runScriptResource(READ_LE_UINT16(ip + 4));

			ip += 6;
			left -= 6;
			break;

		default:
			warning("TopgunEngine_v1_0::runScriptBytecodeLoop(): unknown opcode %d", opcode);
			left = 0;
			break;
		}

		if (!keepGoing)
			break;
	}

	_scriptDepth--;
	return true;
}

void TopgunEngine_v1_0::printDebugString(const byte *text, uint16 value) {
	char message[TG_SCRIPT_DEBUG_STRING_SIZE + 1];
	memcpy(message, text, TG_SCRIPT_DEBUG_STRING_SIZE);
	message[TG_SCRIPT_DEBUG_STRING_SIZE] = '\0';

	uint16 numConversions = 0;
	bool validFormat = true;
	const char *c = message;

	while (*c && validFormat) {
		if (*c == '%') {
			c++;

			if (*c == 'd' || *c == 'i' || *c == 'u' || *c == 'x' || *c == 'X' || *c == 'c')
				numConversions++;
			else if (*c != '%')
				validFormat = false;
		}

		if (*c)
			c++;
	}

	if (validFormat && numConversions <= 1)
		debug("%s", Common::String::format(message, (int)value).c_str());
	else
		debug("%s", message);
}

void TopgunEngine_v1_0::spriteMessageService(byte *record) {
	switch (READ_LE_UINT16(record)) {
	case kEventBreakLoops: {
		uint16 id = READ_LE_UINT16(record + 2);

		if (!_records || id >= _numResources)
			warning("TopgunEngine_v1_0::spriteMessageService(): resource id %d is out of range", id);
		else if (_records[id].handle != 0)
			spriteLibrary()->spriteBreakLoops(_records[id].handle);
		break;
	}

	case kEventSceneEnd:
		postMessage(kMsgSceneEnd, 0, 0);
		break;

	case kEventLoadResource:
		validateAndGetResourceByType(READ_LE_UINT16(record + 2), resourceLibrary()->resGetType(_file, READ_LE_UINT16(record + 2)));
		break;

	case kEventSetLevel:
		o_setSpriteLevel(READ_LE_UINT16(record + 2), READ_LE_UINT16(record + 4));
		break;

	case kEventPlayCDTrack:
		o_playCDTrack(record[2]);
		break;

	case kEventPlayWave:
		o_playWave(READ_LE_UINT16(record + 2), record + 4);
		break;

	case kEventRunScript:
		runScriptResource(READ_LE_UINT16(record + 2));
		break;

	case kEventSetQueue:
		o_setSpriteQueue(READ_LE_UINT16(record + 2), READ_LE_UINT16(record + 4));
		break;

	case kEventStopCD:
		o_stopCD();
		break;

	case kEventStopWave:
		o_stopWave(READ_LE_UINT16(record + 2), record + 4);
		break;

	case kEventTransferSprite:
		o_transferSprite(READ_LE_UINT16(record + 2), READ_LE_UINT16(record + 4), READ_LE_UINT16(record + 6), record[8] != 0);
		break;

	case kEventDestroyResource:
		destroyResourceObjectByType(READ_LE_UINT16(record + 2), resourceLibrary()->resGetType(_file, READ_LE_UINT16(record + 2)));
		break;

	case kEventTrimQueue:
		trimSpriteQueueBitmaps(READ_LE_UINT16(record + 2));
		break;

	default:
		break;
	}
}

bool TopgunEngine_v1_0::runScriptResource(uint16 scriptId) {
	if (!validateAndGetResourceByType(scriptId, kResourceScript))
		return false;

	return runScriptBytecodeLoop(scriptId);
}

} // End of namespace Topgun
