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
#include "common/util.h"

#include "topgun/games/battle/battle.h"
#include "topgun/libraries/resource/resource_v1_2_1.h"
#include "topgun/libraries/sprite/sprite_v1_2_1.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/plugin.h"
#include "topgun/topgun_v1_2_1.h"

namespace Topgun {

class EnginePluginHost : public PluginHost {
public:
	EnginePluginHost(TopgunEngine_v1_2_1 *engine) : _engine(engine) {}

	uint16 *variableTable() override;
	void setScriptTimer(uint16 timerId, uint32 delay, uint16 script) override;
	void cancelScriptTimer(uint16 timerId) override;
	uint16 joystickMessageToIndex(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) override;
	void setSpriteQueue(uint16 sprite, uint16 data, uint16 flag) override;
	void setSpritePosition(uint16 sprite, uint16 x, uint16 y) override;
	void getSpriteRect(uint16 sprite, Common::Rect &rect) override;
	void setSpriteLevel(uint16 sprite, uint16 level) override;
	void loadResource(uint16 id) override;
	void unloadResource(uint16 id) override;
	void transferSprite(uint16 source, uint16 destination, uint16 data, byte freeSource, byte flag) override;
	bool callScript(uint16 script, uint16 argument1, uint16 argument2) override;
	void getSpriteFrameIds(uint16 sprite, uint16 *ids) override;
	uint16 spritesCollide(uint16 first, uint16 second, const Common::Rect *rect) override;
	void stopSprite(uint16 sprite) override;
	uint16 isSpriteVisible(uint16 sprite) override;
	void addCollisionPair(uint16 first, uint16 second, uint16 script, uint16 argument1, uint16 argument2, byte repeat) override;
	void getSpritePosition(uint16 sprite, Common::Point &position) override;
	void spriteSkipMessage(uint16 sprite, uint16 count) override;
	void spriteSwapOrder(uint16 first, uint16 second, uint16 onlyIfSecondAbove, uint16 refresh) override;
	void getSpriteInfo(uint16 sprite, SpriteInfo_v1_2_1 &info) override;
	void setSpriteSpeedMotion(uint16 sprite, uint16 speed, uint16 motion) override;
	const char *getString(uint16 id) override;
	void copyStringVariable(uint16 index, char *destination) override;
	void setScriptResult(uint16 value) override;
	uint16 getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue) override;
	void writePrivateProfileString(const char *section, const char *key, const char *value) override;
	void setMessageFilter(Plugin *plugin) override;
	void setNetworkMessageCallback(Plugin *plugin) override;

private:
	TopgunEngine_v1_2_1 *_engine;
};

uint16 *EnginePluginHost::variableTable() {
	return _engine->_variableTable;
}

void EnginePluginHost::setScriptTimer(uint16 timerId, uint32 delay, uint16 script) {
	byte record[12];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1SetScriptTimer);
	WRITE_LE_UINT16(record + 2, timerId);
	WRITE_LE_UINT16(record + 4, script);
	WRITE_LE_UINT32(record + 6, delay);

	_engine->o_setScriptTimer(record);
}

void EnginePluginHost::cancelScriptTimer(uint16 timerId) {
	byte record[6];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1CancelScriptTimer);
	WRITE_LE_UINT16(record + 2, timerId);

	_engine->o_cancelScriptTimer(record);
}

uint16 EnginePluginHost::joystickMessageToIndex(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	Message joystickMessage;
	joystickMessage.id = message;
	joystickMessage.param = wParam;
	joystickMessage.data = ((uint32)lParamHigh << 16) | lParamLow;

	return _engine->joystickMessageToIndex(joystickMessage);
}

void EnginePluginHost::setSpriteQueue(uint16 sprite, uint16 data, uint16 flag) {
	_engine->o_setSpriteQueue(sprite, data, flag != 0);
}

void EnginePluginHost::setSpritePosition(uint16 sprite, uint16 x, uint16 y) {
	byte record[8];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1SetSpritePosition);
	WRITE_LE_UINT16(record + 2, sprite);
	WRITE_LE_UINT16(record + 4, (uint16)(x - TG_HOST_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 6, (uint16)(y - TG_HOST_V1_2_1_LITERAL_BIAS));

	_engine->o_setSpritePosition(record);
}

void EnginePluginHost::getSpriteRect(uint16 sprite, Common::Rect &rect) {
	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "getSpriteRect"))
		return;

	uint16 handle = _engine->_records[index].handle;
	if (handle != 0)
		_engine->spriteLib()->spriteGetBounds(handle, rect);
}

void EnginePluginHost::setSpriteLevel(uint16 sprite, uint16 level) {
	if (!_engine->isResourceObjectLoaded(sprite, kResV1_2_1Sprite))
		return;

	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "setSpriteLevel"))
		return;

	_engine->spriteLib()->spriteSetLevel(_engine->_records[index].handle, (int16)level);
}

void EnginePluginHost::loadResource(uint16 id) {
	_engine->validateAndGetResourceByType(id, _engine->resourceLib()->resGetType(_engine->_file, _engine->operandValue(id)));
}

void EnginePluginHost::unloadResource(uint16 id) {
	_engine->destroyResourceObjectByType(id, _engine->resourceLib()->resGetType(_engine->_file, _engine->operandValue(id)));
}

void EnginePluginHost::transferSprite(uint16 source, uint16 destination, uint16 data, byte freeSource, byte flag) {
	byte record[12];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1TransferSprite);
	WRITE_LE_UINT16(record + 2, source);
	WRITE_LE_UINT16(record + 4, destination);
	WRITE_LE_UINT16(record + 6, data);
	record[0x0A] = freeSource;
	record[0x0B] = flag;

	_engine->o_transferSprite(record);
}

bool EnginePluginHost::callScript(uint16 script, uint16 argument1, uint16 argument2) {
	byte record[18];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1CallScript);
	WRITE_LE_UINT16(record + 2, script);
	record[5] = 2;
	WRITE_LE_UINT16(record + 6, (uint16)(argument1 - TG_HOST_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 8, (uint16)(argument2 - TG_HOST_V1_2_1_LITERAL_BIAS));

	return _engine->o_callScript(record);
}

void EnginePluginHost::getSpriteFrameIds(uint16 sprite, uint16 *ids) {
	if (!_engine->isResourceObjectLoaded(sprite, kResV1_2_1Sprite))
		return;

	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "getSpriteFrameIds"))
		return;

	SpriteInfo_v1_2_1 info;
	_engine->spriteLib()->spriteGetInfo(_engine->_records[index].handle, info);

	uint16 count = info.frameCount;
	if (count > TG_PLUGIN_V1_2_1_FRAME_IDS_SIZE) {
		warning("EnginePluginHost::getSpriteFrameIds(): the sprite has %d frames", count);
		count = TG_PLUGIN_V1_2_1_FRAME_IDS_SIZE;
	}

	for (uint16 i = 0; i < count; i++) {
		if (info.frames[i])
			ids[i] = READ_LE_UINT16(info.frames[i] + TG_HOST_V1_2_1_BITMAP_RESOURCE_ID);
	}
}

uint16 EnginePluginHost::spritesCollide(uint16 first, uint16 second, const Common::Rect *rect) {
	uint16 firstIndex = _engine->operandValue(first);
	if (!_engine->isRecordIndexValid(firstIndex, "spritesCollide"))
		return 0;

	uint16 firstHandle = _engine->_records[firstIndex].handle;
	if (firstHandle == 0)
		return 0;

	uint16 secondIndex = _engine->operandValue(second);
	if (!_engine->isRecordIndexValid(secondIndex, "spritesCollide"))
		return 0;

	uint16 secondHandle = _engine->_records[secondIndex].handle;
	if (secondHandle == 0)
		return 0;

	if (rect)
		return _engine->spriteLib()->spriteHitDetectRect(firstHandle, secondHandle, *rect);

	return _engine->spriteLib()->spriteHitDetect(firstHandle, secondHandle, 0, 0, 0);
}

void EnginePluginHost::stopSprite(uint16 sprite) {
	byte record[6];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1StopSprite);
	WRITE_LE_UINT16(record + 2, sprite);
	record[4] = 1;

	_engine->o_stopSprite(record);
}

uint16 EnginePluginHost::isSpriteVisible(uint16 sprite) {
	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "isSpriteVisible"))
		return 0;

	uint16 handle = _engine->_records[index].handle;
	if (handle == 0)
		return 0;

	return (_engine->spriteLib()->spriteIsVisible(handle) != 0) ? 1 : 0;
}

void EnginePluginHost::addCollisionPair(uint16 first, uint16 second, uint16 script, uint16 argument1, uint16 argument2, byte repeat) {
	byte record[18];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_1AddCollisionPair);
	WRITE_LE_UINT16(record + 2, first);
	WRITE_LE_UINT16(record + 4, second);
	WRITE_LE_UINT16(record + 6, script);
	WRITE_LE_UINT16(record + 8, (uint16)(argument1 - TG_HOST_V1_2_1_LITERAL_BIAS));
	WRITE_LE_UINT16(record + 0x0A, (uint16)(argument2 - TG_HOST_V1_2_1_LITERAL_BIAS));
	record[0x0C] = repeat;

	_engine->addCollisionPair(record);
}

void EnginePluginHost::getSpritePosition(uint16 sprite, Common::Point &position) {
	position = Common::Point();

	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "getSpritePosition"))
		return;

	uint16 handle = _engine->_records[index].handle;

	if (handle != 0) {
		int16 x = 0;
		int16 y = 0;

		if (_engine->spriteLib()->spriteGetPosition(handle, x, y)) {
			position.x = x;
			position.y = y;
		}
	}
}

void EnginePluginHost::spriteSkipMessage(uint16 sprite, uint16 count) {
	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "spriteSkipMessage"))
		return;

	uint16 handle = _engine->_records[index].handle;
	if (handle != 0)
		_engine->spriteLib()->spriteSkipMessage(handle, count);
}

void EnginePluginHost::spriteSwapOrder(uint16 first, uint16 second, uint16 onlyIfSecondAbove, uint16 refresh) {
	uint16 firstIndex = _engine->operandValue(first);
	if (!_engine->isRecordIndexValid(firstIndex, "spriteSwapOrder"))
		return;

	uint16 firstHandle = _engine->_records[firstIndex].handle;
	if (firstHandle == 0)
		return;

	uint16 secondIndex = _engine->operandValue(second);
	if (!_engine->isRecordIndexValid(secondIndex, "spriteSwapOrder"))
		return;

	uint16 secondHandle = _engine->_records[secondIndex].handle;
	if (secondHandle == 0)
		return;

	_engine->spriteLib()->spriteSwapOrder(firstHandle, secondHandle, onlyIfSecondAbove != 0, refresh != 0);
}

void EnginePluginHost::getSpriteInfo(uint16 sprite, SpriteInfo_v1_2_1 &info) {
	if (!_engine->isResourceObjectLoaded(sprite, kResV1_2_1Sprite))
		return;

	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "getSpriteInfo"))
		return;

	_engine->spriteLib()->spriteGetInfo(_engine->_records[index].handle, info);
}

void EnginePluginHost::setSpriteSpeedMotion(uint16 sprite, uint16 speed, uint16 motion) {
	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "setSpriteSpeedMotion"))
		return;

	uint16 handle = _engine->_records[index].handle;

	_engine->spriteLib()->spriteSetSpeed(handle, speed);
	_engine->spriteLib()->spriteSetMotion(handle, motion);
}

const char *EnginePluginHost::getString(uint16 id) {
	return _engine->getStringFromId(id);
}

void EnginePluginHost::copyStringVariable(uint16 index, char *destination) {
	if (index == 0 || _engine->_currentSceneIndex >= _engine->_sceneEntries.size())
		return;

	const SceneEntry_v1_2 &entry = _engine->_sceneEntries[_engine->_currentSceneIndex];
	if (entry.stringCount < index)
		return;

	Common::strlcpy(destination, (const char *)entry.strings + (index - 1) * TG_HOST_V1_2_STRING_SLOT_SIZE, TG_PLUGIN_V1_2_1_STRING_COPY_SIZE);
}

void EnginePluginHost::setScriptResult(uint16 value) {
	_engine->_scriptResult = value;
}

uint16 EnginePluginHost::getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue) {
	return _engine->getPrivateProfileInt(section, key, defaultValue);
}

void EnginePluginHost::writePrivateProfileString(const char *section, const char *key, const char *value) {
	_engine->writePrivateProfileString(section, key, value);
}

void EnginePluginHost::setMessageFilter(Plugin *plugin) {
	_engine->_messageFilter = plugin;
}

void EnginePluginHost::setNetworkMessageCallback(Plugin *plugin) {
	_engine->_windowMessageCallback = plugin;
}

Plugin *createPlugin(const Common::String &name, PluginHost *host) {
	if (name.equalsIgnoreCase("BATTLE.DLL"))
		return new BattlePlugin(host);

	return nullptr;
}

bool TopgunEngine_v1_2_1::loadPluginLibrary(const Common::String &name) {
	if (_plugin)
		unloadPluginLibrary();

	if (!_pluginHost)
		_pluginHost = new EnginePluginHost(this);

	_plugin = createPlugin(name, _pluginHost);
	if (!_plugin)
		return false;

	if (!loadPluginFunctionTable()) {
		delete _plugin;
		_plugin = nullptr;
		return false;
	}

	return true;
}

void TopgunEngine_v1_2_1::unloadPluginLibrary() {
	if (!_plugin)
		return;

	_plugin->free();

	delete _plugin;
	_plugin = nullptr;

	_messageFilter = nullptr;
	_windowMessageCallback = nullptr;
}

bool TopgunEngine_v1_2_1::loadPluginFunctionTable() {
	uint32 size = 0;
	byte *names = resourceLib()->resLoadKeyResource(_file, TG_RES_V1_2_1_KEY_PLUGIN_NAMES, &size);

	_pluginFunctions.clear();

	if (!names)
		return size == 0;

	uint32 consumed = 0;

	while (consumed < size) {
		uint32 length = 0;
		while (consumed + length < size && names[consumed + length] != 0)
			length++;

		int function = _plugin->findFunction(Common::String((const char *)names + consumed, length));
		if (function < 0)
			break;

		_pluginFunctions.push_back(function);
		consumed += length + 1;
	}

	utilsLibrary()->tgFree(names);

	return consumed >= size;
}

void TopgunEngine_v1_2_1::exprCallPluginFunction(uint16 function, uint16 count) {
	_expressionStackPointer -= count;

	if (!_plugin || function >= _pluginFunctions.size()) {
		warning("TopgunEngine_v1_2_1::exprCallPluginFunction(): plug-in function %d does not exist", function);
		_expressionStack[_expressionStackPointer - 1] = 0;
		return;
	}

	_expressionStack[_expressionStackPointer - 1] = _plugin->callFunction(_pluginFunctions[function], &_expressionStack[_expressionStackPointer], count);
}

bool TopgunEngine_v1_2_1::callMessageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	return _messageFilter && _messageFilter->messageFilter(message, wParam, lParamLow, lParamHigh) != 0;
}

} // End of namespace Topgun
