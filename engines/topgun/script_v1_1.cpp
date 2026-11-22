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

#include "common/config-manager.h"
#include "common/endian.h"
#include "common/textconsole.h"

#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/topgun_v1_1.h"

namespace Topgun {

bool TopgunEngine_v1_1::runScriptResource(uint16 scriptId) {
	if (_scriptsDisabled)
		return false;

	if (!validateAndGetResourceByType(scriptId, kResourceScript))
		return false;

	_scriptDepth++;

	if (_scriptDepth >= TG_HOST_V1_1_MAX_SCRIPT_DEPTH) {
		_shuttingDown = true;
		_scriptsDisabled = true;
		GUIErrorMessage("Stack overflow.");
		postMessage(kMsgClose, 0, 0);
		return false;
	}

	bool result = runScriptBytecodeLoop(scriptId);

	_scriptDepth--;
	return result;
}

bool TopgunEngine_v1_1::runScriptBytecodeLoop(uint16 scriptId) {
	const ResourceRecord &script = _records[scriptId];
	byte *ip = script.data;
	int16 left = (int16)(script.size & 0xFFFF);
	bool keepGoing = true;

	while (true) {
		uint16 size = 0;

		keepGoing = runOpcode(ip, size);
		if (!keepGoing)
			break;

		if (size == 0) {
			warning("TopgunEngine_v1_1::runScriptBytecodeLoop(): unknown opcode %d", READ_LE_UINT16(ip));
			break;
		}

		left -= size;
		if (left <= 0)
			break;

		ip += size;
	}

	return keepGoing;
}

bool TopgunEngine_v1_1::runOpcode(byte *ip, uint16 &size) {
	bool result = true;

	switch (READ_LE_UINT16(ip)) {
	case kOpCallScript:
		result = runScriptResource(READ_LE_UINT16(ip + 2));
		size = 4;
		break;

	case kOpLoadResource:
		result = validateAndGetResourceByType(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
		size = 6;
		break;

	case kOpUnloadResource:
		result = destroyResourceObjectByType(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
		size = 6;
		break;

	case kOpSetClickScript:
		o_setClickScript(ip);
		size = 14;
		break;

	case kOpSetSpriteQueue:
		result = o_setSpriteQueue(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
		size = 6;
		break;

	case kOpSetBackground: {
		uint16 backgroundId = READ_LE_UINT16(ip + 2);

		if ((_flagLoMem || _flagMode640) && READ_LE_UINT16(ip + 4) != 0)
			backgroundId = READ_LE_UINT16(ip + 4);

		setBackgroundFromBitmapResource(backgroundId);
		size = 6;
		break;
	}

	case kOpPlayWave:
		result = o_playWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 11;
		break;

	case kOpPlayCDTrack:
		o_playCDTrack(ip[2]);
		size = 4;
		break;

	case kOpFadeIn:
	case kOpFadeOut:
		o_fadeInOut(ip);
		size = 4;
		break;

	case kOpBindKeyScript:
		o_bindKeyScript(ip);
		size = 8;
		break;

	case kOpQuit:
		postMessage(kMsgClose, 0, 0);
		size = 2;
		break;

	case kOpStopCD:
		o_stopCD();
		size = 4;
		break;

	case kOpStopWave:
		o_stopWave(READ_LE_UINT16(ip + 2), ip + 4);
		size = 7;
		break;

	case kOpSetFlag:
		o_setFlag(ip);
		size = 9;
		break;

	case kOpTestFlag:
		result = o_testFlag(ip);
		size = 13;
		break;

	case kOpGotoScene:
		_gotoRecord.opcode = READ_LE_UINT16(ip);
		memcpy(_gotoRecord.name, ip + 2, sizeof(_gotoRecord.name));
		_gotoRecord.releaseSceneSprite = ip[15];

		if (ip[15] != 0)
			destroySceneMainObjects(_releaseOnPalette);

		postMessage(kMsgGotoScene, 0, 0);
		_keyScripts.clear();
		size = 16;
		break;

	case kOpSetScriptTimer:
		o_setScriptTimer(ip);
		size = 10;
		break;

	case kOpCancelScriptTimer:
		o_cancelScriptTimer(ip);
		size = 4;
		break;

	case kOpSetSceneEndScript:
		if (_gotoRecord.releaseSceneSprite != 0)
			postMessage(kMsgSceneEnd, 0, 0);

		_sceneEndScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpRandomRunScript:
		result = o_randomRunScript(ip);
		size = 17;
		break;

	case kOpRoundRobinRunScript:
		result = o_roundRobinRunScript(ip);
		size = 17;
		break;

	case kOpTransferSprite:
		o_transferSprite(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4), READ_LE_UINT16(ip + 6), ip[8] != 0);
		size = 9;
		break;

	case kOpBreakSpriteLoops: {
		uint16 id = READ_LE_UINT16(ip + 2);

		if (!_records || id >= _numResources)
			warning("TopgunEngine_v1_1::runOpcode(): resource id %d is out of range", id);
		else if (_records[id].handle != 0)
			spriteLibrary()->spriteBreakLoops(_records[id].handle);

		size = 4;
		break;
	}

	case kOpSetSpriteQueueIfLoaded: {
		uint16 id = READ_LE_UINT16(ip + 2);

		if (!_records || id >= _numResources)
			warning("TopgunEngine_v1_1::runOpcode(): resource id %d is out of range", id);
		else if (_records[id].handle != 0)
			result = o_setSpriteQueue(id, READ_LE_UINT16(ip + 4));

		size = 6;
		break;
	}

	case kOpDebugString:
		printDebugString(ip + 4, readFlag(READ_LE_UINT16(ip + 2)));
		size = 55;
		break;

	case kOpSetSpriteLevel:
		o_setSpriteLevel(READ_LE_UINT16(ip + 2), READ_LE_UINT16(ip + 4));
		size = 6;
		break;

	case kOpSetActivationScript:
		_activationScript = READ_LE_UINT16(ip + 2);
		size = 4;
		break;

	case kOpPostScript:
		postMessage(kMsgRunScript, READ_LE_UINT16(ip + 2), 0);
		size = 4;
		break;

	case kOpV1_1CursorMode:
		setCursorMode(ip[2]);
		size = 3;
		break;

	case kOpV1_1RunScriptIfLoaded:
		if (isResourceObjectLoaded(READ_LE_UINT16(ip + 2)) && READ_LE_UINT16(ip + 4) != 0)
			runScriptResource(READ_LE_UINT16(ip + 4));

		size = 6;
		break;

	case kOpV1_1SetSpriteCollision:
		result = o_setSpriteCollision(ip);
		size = 8;
		break;

	case kOpV1_1CallPluginFunction:
		warning("TopgunEngine_v1_1::runOpcode(): plug-in functions are not supported");
		size = 8;
		break;

	case kOpV1_1RunListEntry:
		result = o_runListEntry(ip);
		size = 10;
		break;

	case kOpV1_1IniVariable:
		o_iniVariable(ip);
		size = 26;
		break;

	default:
		size = 0;
		break;
	}

	return result;
}

uint16 *TopgunEngine_v1_1::flagSlot(uint16 flag) {
	uint16 index = flag & 0x7FFF;

	if ((flag & 0x8000) != 0) {
		if (index < TG_HOST_NUM_GLOBAL_FLAGS)
			return &_globalFlags[index];

		warning("TopgunEngine_v1_1::flagSlot(): global flag %d is out of range", index);
		return nullptr;
	}

	if (!_currentScene)
		return nullptr;

	if (index < TG_HOST_NUM_SCENE_FLAGS)
		return &_currentScene->flags[index];

	warning("TopgunEngine_v1_1::flagSlot(): scene flag %d is out of range", index);
	return nullptr;
}

uint16 TopgunEngine_v1_1::readFlag(uint16 flag) {
	uint16 *slot = flagSlot(flag);

	return slot ? *slot : 0;
}

void TopgunEngine_v1_1::o_playCDTrack(uint16 track) {
	_cdAudioPlaying = (audioLibrary()->audioCommand(kPlayCDTracksCommand, 0, (uint16)(((track & 0xFF) << 8) | track), nullptr) != 0);
}

void TopgunEngine_v1_1::o_setFlag(const byte *record) {
	uint16 *destination = flagSlot(READ_LE_UINT16(record + 2));
	if (!destination)
		return;

	uint16 value = READ_LE_UINT16(record + 6);

	if (record[8] != 0)
		value += readFlag(READ_LE_UINT16(record + 4));

	*destination = value;
}

bool TopgunEngine_v1_1::o_testFlag(const byte *record) {
	uint16 flag = READ_LE_UINT16(record + 2);

	if ((flag & 0x8000) == 0 && !_currentScene)
		return true;

	uint16 other = READ_LE_UINT16(record + 8);

	if (record[12] != 0)
		other = readFlag(other);

	uint16 value = readFlag(flag);
	byte condition = record[10];
	bool hit;

	if (condition & 0x01)
		hit = (other == value);
	else if (condition & 0x02)
		hit = (other < value);
	else if (condition & 0x04)
		hit = (other > value);
	else
		hit = (value != 0);

	if (condition & 0x80)
		hit = !hit;

	uint16 script;

	if (hit) {
		script = READ_LE_UINT16(record + 4);
	} else {
		if (record[11] == 0)
			return true;

		script = READ_LE_UINT16(record + 6);
	}

	if (script == 0xFFFF)
		return true;

	return runScriptResource(script);
}

bool TopgunEngine_v1_1::o_randomRunScript(byte *record) {
	const byte *list;
	uint16 count;

	if (record[16] != 0) {
		uint16 listId = READ_LE_UINT16(record + 4);

		if (!validateAndGetResourceByType(listId, kResourceIdList))
			return false;

		const byte *data = _records[listId].data;
		count = READ_LE_UINT16(data);
		list = data + 2;
	} else {
		count = record[2];
		list = record + 4;
	}

	if (count == 0) {
		warning("TopgunEngine_v1_1::o_randomRunScript(): the record has no scripts");
		return true;
	}

	uint16 pick = getRandom15Bit() % count;

	if (record[3] == pick) {
		pick++;

		if (pick >= count)
			pick = 0;
	}

	record[3] = (byte)pick;

	if (record[16] == 0 && pick >= TG_HOST_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_1::o_randomRunScript(): script index %d is out of range", pick);
		return true;
	}

	return runScriptResource(READ_LE_UINT16(list + 2 * pick));
}

bool TopgunEngine_v1_1::o_roundRobinRunScript(byte *record) {
	if (record[16] != 0) {
		uint16 listId = READ_LE_UINT16(record + 4);

		if (!validateAndGetResourceByType(listId, kResourceIdList))
			return false;

		const byte *data = _records[listId].data;
		uint16 count = READ_LE_UINT16(data);

		if (record[3] >= count) {
			warning("TopgunEngine_v1_1::o_roundRobinRunScript(): script index %d is out of range", record[3]);
			record[3] = 0;
		}

		bool result = runScriptResource(READ_LE_UINT16(data + 2 + 2 * record[3]));

		record[3]++;
		if (record[3] >= count)
			record[3] = 0;

		return result;
	}

	if (record[2] <= record[3])
		record[3] = 0;

	if (record[3] >= TG_HOST_NUM_SCRIPT_CHOICES) {
		warning("TopgunEngine_v1_1::o_roundRobinRunScript(): script index %d is out of range", record[3]);
		return false;
	}

	uint16 id = READ_LE_UINT16(record + 4 + 2 * record[3]);

	if (!validateAndGetResourceByType(id, kResourceScript))
		return false;

	byte *nested = _records[id].data;
	bool result = runScriptResource(id);

	if (READ_LE_UINT16(nested) != kOpRoundRobinRunScript || nested[2] == nested[3])
		record[3]++;

	return result;
}

bool TopgunEngine_v1_1::o_setSpriteCollision(const byte *record) {
	uint16 id = READ_LE_UINT16(record + 2);

	if (!validateAndGetResourceByType(id, kResourceSprite))
		return false;

	spriteLibrary()->spriteSetCollision(_records[id].handle, READ_LE_UINT16(record + 4), READ_LE_UINT16(record + 6));
	return true;
}

bool TopgunEngine_v1_1::o_runListEntry(const byte *record) {
	uint16 listId = READ_LE_UINT16(record + 2);

	if (!validateAndGetResourceByType(listId, kResourceIdList))
		return false;

	const byte *list = _records[listId].data;
	uint16 count = READ_LE_UINT16(list);
	uint16 flag = READ_LE_UINT16(record + 6);
	uint16 *variable = flagSlot(flag);
	uint16 index = variable ? *variable : 0;
	bool result = true;

	if (index < count)
		result = runScriptResource(READ_LE_UINT16(list + 2 + 2 * index));
	else if (record[8] != 0)
		result = runScriptResource(READ_LE_UINT16(record + 4));

	if (record[9] != 0) {
		index++;

		if (index >= count)
			index = 0;

		if (variable) {
			// This is the exact value the game stores in a global flag: the index ORed with 0x7FFF
			*variable = ((flag & 0x8000) != 0) ? (index | 0x7FFF) : index;
		}
	}

	return result;
}

void TopgunEngine_v1_1::o_iniVariable(const byte *record) {
	uint16 *variable = flagSlot(READ_LE_UINT16(record + 2));
	if (!variable)
		return;

	char text[22];
	memcpy(text, record + 5, sizeof(text) - 1);
	text[sizeof(text) - 1] = '\0';

	Common::String key(text);
	key.toLowercase();

	if (key.empty()) {
		warning("TopgunEngine_v1_1::o_iniVariable(): the key name is empty");
		return;
	}

	if (record[4] != 0) {
		*variable = (uint16)(ConfMan.hasKey(key) ? ConfMan.getInt(key) : 0);
	} else {
		ConfMan.setInt(key, (int16)*variable);
		ConfMan.flushToDisk();
	}
}

} // End of namespace Topgun
