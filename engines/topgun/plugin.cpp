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

#include "common/archive.h"
#include "common/endian.h"
#include "common/savefile.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "topgun/display.h"
#include "topgun/games/ace/ace1util.h"
#include "topgun/games/ace/dynalloc.h"
#include "topgun/games/ace/fileio.h"
#include "topgun/games/ace/pathfind.h"
#include "topgun/games/battle/battle.h"
#include "topgun/games/battledemo/battledemo.h"
#include "topgun/games/virgil/re.h"
#include "topgun/games/virgil/wordlist.h"
#include "topgun/libraries/resource/resource_v1_2_4.h"
#include "topgun/libraries/sprite/sprite_v1_2_4.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/plugin.h"
#include "topgun/topgun_v2_0.h"

namespace Topgun {

struct MmioFile {
	bool open;
	bool writable;
	bool modified;
	Common::String name;
	Common::Array<byte> data;
	uint32 position;

	MmioFile() : open(false), writable(false), modified(false), position(0) {}
};

class EnginePluginHost : public PluginHost {
public:
	EnginePluginHost(TopgunEngine_v1_2 *engine) : _engine(engine) {}
	~EnginePluginHost() override;

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
	uint16 callScript(uint16 script, uint16 argument1, uint16 argument2) override;
	void getSpriteFrameIds(uint16 sprite, uint16 *ids) override;
	uint16 spritesCollide(uint16 first, uint16 second, const Common::Rect *rect) override;
	void stopSprite(uint16 sprite) override;
	uint16 isSpriteVisible(uint16 sprite) override;
	void addCollisionPair(uint16 first, uint16 second, uint16 script, uint16 argument1, uint16 argument2, byte repeat) override;
	void getSpritePosition(uint16 sprite, Common::Point &position) override;
	void spriteSkipMessage(uint16 sprite, uint16 count) override;
	void spriteSwapOrder(uint16 first, uint16 second, uint16 onlyIfSecondAbove, uint16 refresh) override;
	void getSpriteInfo(uint16 sprite, SpriteInfo_v1_2_4 &info) override;
	void setSpriteSpeedMotion(uint16 sprite, uint16 speed, uint16 motion) override;
	char *getString(uint16 id) override;
	void setString(uint16 id, const char *text) override;
	uint16 setText(uint16 id, const char *text) override;
	int16 compareStringsIgnoreCase(const char *first, const char *second) override;
	uint32 getConstantStringRoom(uint16 id) override;
	byte *tgMalloc(uint32 size, uint16 zeroInit) override;
	byte *tgFree(byte *block) override;
	uint16 globalAlloc(uint32 size) override;
	byte *globalLock(uint16 handle, uint32 &size) override;
	void globalFree(uint16 handle) override;
	ResourceRecord *getResource(uint16 id) override;
	bool isPaletteDevice() override;
	void getPalette(byte *entries, uint16 first, uint16 count) override;
	void replaceSpriteBitmap(uint16 sprite, byte *oldBitmap, byte *newBitmap) override;
	byte *deviceBitmapToDibData(const Graphics::ManagedSurface *bitmap, bool remap, byte color) override;
	void mapBytes(byte *buffer, uint32 count, const byte *table) override;
	uint16 createFont(int16 height, const char *face) override;
	void deleteFont(uint16 font) override;
	const Graphics::Font *getFont(uint16 font) override;
	void copyStringVariable(uint16 index, char *destination) override;
	void setScriptResult(uint16 value) override;
	uint16 getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue) override;
	void writePrivateProfileString(const char *section, const char *key, const char *value) override;
	char *getVariableString(uint16 id) override;
	uint16 readAddress(uint16 address) override;
	void writeAddress(uint16 address, uint16 value) override;
	uint16 runMessage(uint16 script, const uint16 *arguments, uint16 count) override;
	uint16 getModulePath(char *destination, uint16 size) override;
	uint16 getErrorFileHandle() override;
	uint16 mmioOpen(const char *name, uint32 flags) override;
	uint16 mmioClose(uint16 handle) override;
	int32 mmioRead(uint16 handle, byte *buffer, int32 count) override;
	int32 mmioWrite(uint16 handle, const byte *buffer, int32 count) override;
	int32 mmioSeek(uint16 handle, int32 offset, int origin) override;
	void setMessageFilter(Plugin *plugin) override;
	void setNetworkMessageCallback(Plugin *plugin) override;
	void setScriptHook(Plugin *plugin) override;
	void repostMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) override;

private:
	MmioFile *findFile(uint16 handle);

	TopgunEngine_v1_2 *_engine;
	Common::Array<MmioFile> _files;
	Common::Array<Common::Array<byte> *> _globalBlocks;
};

EnginePluginHost::~EnginePluginHost() {
	for (uint16 i = 0; i < _files.size(); i++) {
		if (_files[i].open)
			mmioClose(i + 1);
	}

	for (uint32 i = 0; i < _globalBlocks.size(); i++)
		delete _globalBlocks[i];
}

uint16 *EnginePluginHost::variableTable() {
	return _engine->_variableTable;
}

void EnginePluginHost::setScriptTimer(uint16 timerId, uint32 delay, uint16 script) {
	byte record[12];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_4SetScriptTimer);
	WRITE_LE_UINT16(record + 2, timerId);
	WRITE_LE_UINT16(record + 4, script);
	WRITE_LE_UINT32(record + 6, delay);

	_engine->o_setScriptTimer(record);
}

void EnginePluginHost::cancelScriptTimer(uint16 timerId) {
	byte record[6];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_4CancelScriptTimer);
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

	uint16 literalBias = (_engine->_version >= kVersionV1_2_3_2) ? TG_HOST_V1_2_4_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

	WRITE_LE_UINT16(record, kOpV1_2_4SetSpritePosition);
	WRITE_LE_UINT16(record + 2, sprite);
	WRITE_LE_UINT16(record + 4, (uint16)(x - literalBias));
	WRITE_LE_UINT16(record + 6, (uint16)(y - literalBias));

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
	uint16 spriteType = kResV1_2_4Sprite;
	if (_engine->_version >= kVersionV1_2_12)
		spriteType = kResV1_2_12Sprite;
	else if (_engine->_version >= kVersionV1_2_6)
		spriteType = kResV1_2_6Sprite;

	if (!_engine->isResourceObjectLoaded(sprite, spriteType))
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

	WRITE_LE_UINT16(record, kOpV1_2_4TransferSprite);
	WRITE_LE_UINT16(record + 2, source);
	WRITE_LE_UINT16(record + 4, destination);
	WRITE_LE_UINT16(record + 6, data);
	record[0x0A] = freeSource;
	record[0x0B] = flag;

	_engine->o_transferSprite(record);
}

uint16 EnginePluginHost::callScript(uint16 script, uint16 argument1, uint16 argument2) {
	if (_engine->_version >= kVersionV2_0) {
		byte call[TG_SCRIPT_V2_0_POSTED_RUN_SIZE];
		memset(call, 0, sizeof(call));

		WRITE_LE_UINT16(call + 2, script);
		call[TG_SCRIPT_V2_0_CALL_FRAME_SLOTS] = TG_SCRIPT_V2_0_HOST_FRAME_SLOTS;
		call[TG_SCRIPT_V2_0_CALL_COUNT] = 2;
		WRITE_LE_UINT16(call + TG_SCRIPT_V2_0_CALL_HEADER_SIZE, argument1);
		WRITE_LE_UINT16(call + TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2, argument2);

		return static_cast<TopgunEngine_v2_0 *>(_engine)->callScriptWithArguments(call);
	}

	byte record[18];
	memset(record, 0, sizeof(record));

	uint16 literalBias = (_engine->_version >= kVersionV1_2_3_2) ? TG_HOST_V1_2_4_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

	WRITE_LE_UINT16(record, kOpV1_2_4CallScript);
	WRITE_LE_UINT16(record + 2, script);
	record[5] = 2;
	WRITE_LE_UINT16(record + 6, (uint16)(argument1 - literalBias));
	WRITE_LE_UINT16(record + 8, (uint16)(argument2 - literalBias));

	bool result = _engine->o_callScript(record);

	// The demo hands back the value returned by the script
	if (_engine->_version < kVersionV1_2_3_2)
		return _engine->_scriptResult;

	return result;
}

void EnginePluginHost::getSpriteFrameIds(uint16 sprite, uint16 *ids) {
	uint16 spriteType = kResV1_2_4Sprite;
	if (_engine->_version >= kVersionV1_2_12)
		spriteType = kResV1_2_12Sprite;
	else if (_engine->_version >= kVersionV1_2_6)
		spriteType = kResV1_2_6Sprite;

	if (!_engine->isResourceObjectLoaded(sprite, spriteType))
		return;

	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "getSpriteFrameIds"))
		return;

	SpriteInfo_v1_2_4 info;
	static_cast<SpriteLibrary_v1_2_4 *>(_engine->spriteLib())->spriteGetInfo(_engine->_records[index].handle, info);

	uint16 count = info.frameCount;
	if (count > TG_PLUGIN_V1_2_4_FRAME_IDS_SIZE) {
		warning("EnginePluginHost::getSpriteFrameIds(): the sprite has %d frames", count);
		count = TG_PLUGIN_V1_2_4_FRAME_IDS_SIZE;
	}

	for (uint16 i = 0; i < count; i++) {
		if (info.frames[i])
			ids[i] = READ_LE_UINT16(info.frames[i] + TG_HOST_V1_2_4_BITMAP_RESOURCE_ID);
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
		return static_cast<SpriteLibrary_v1_2_4 *>(_engine->spriteLib())->spriteHitDetectRect(firstHandle, secondHandle, *rect);

	return static_cast<SpriteLibrary_v1_2_4 *>(_engine->spriteLib())->spriteHitDetect(firstHandle, secondHandle, 0, 0, 0);
}

void EnginePluginHost::stopSprite(uint16 sprite) {
	byte record[6];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record, kOpV1_2_4StopSprite);
	WRITE_LE_UINT16(record + 2, sprite);

	if (_engine->_version >= kVersionV1_2_5)
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

	WRITE_LE_UINT16(record, kOpV1_2_4AddCollisionPair);
	WRITE_LE_UINT16(record + 2, first);
	WRITE_LE_UINT16(record + 4, second);
	uint16 literalBias = (_engine->_version >= kVersionV1_2_3_2) ? TG_HOST_V1_2_4_LITERAL_BIAS : TG_HOST_V1_2_LITERAL_BIAS;

	WRITE_LE_UINT16(record + 6, script);
	WRITE_LE_UINT16(record + 8, (uint16)(argument1 - literalBias));
	WRITE_LE_UINT16(record + 0x0A, (uint16)(argument2 - literalBias));
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
		static_cast<SpriteLibrary_v1_2_4 *>(_engine->spriteLib())->spriteSkipMessage(handle, count);
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

void EnginePluginHost::getSpriteInfo(uint16 sprite, SpriteInfo_v1_2_4 &info) {
	uint16 spriteType = kResV1_2_4Sprite;
	if (_engine->_version >= kVersionV1_2_12)
		spriteType = kResV1_2_12Sprite;
	else if (_engine->_version >= kVersionV1_2_6)
		spriteType = kResV1_2_6Sprite;

	if (!_engine->isResourceObjectLoaded(sprite, spriteType))
		return;

	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "getSpriteInfo"))
		return;

	static_cast<SpriteLibrary_v1_2_4 *>(_engine->spriteLib())->spriteGetInfo(_engine->_records[index].handle, info);
}

void EnginePluginHost::setSpriteSpeedMotion(uint16 sprite, uint16 speed, uint16 motion) {
	uint16 index = _engine->operandValue(sprite);
	if (!_engine->isRecordIndexValid(index, "setSpriteSpeedMotion"))
		return;

	uint16 handle = _engine->_records[index].handle;

	_engine->spriteLib()->spriteSetSpeed(handle, speed);
	_engine->spriteLib()->spriteSetMotion(handle, motion);
}

char *EnginePluginHost::getString(uint16 id) {
	return _engine->getStringFromId(id);
}

void EnginePluginHost::setString(uint16 id, const char *text) {
	if (id == 0 || _engine->_currentSceneIndex >= _engine->_sceneEntries.size())
		return;

	const SceneEntry_v1_2 &entry = _engine->_sceneEntries[_engine->_currentSceneIndex];
	if (entry.stringCount < id)
		return;

	Common::strlcpy((char *)entry.strings + (id - 1) * TG_HOST_V1_2_STRING_SLOT_SIZE, text, TG_PLUGIN_V1_2_4_STRING_COPY_SIZE);
}

uint16 EnginePluginHost::setText(uint16 id, const char *text) {
	if (!_engine->validateAndGetResourceByType(id, kResV1_2_12Text))
		return 0;

	uint16 index = _engine->operandValue(id);
	if (!_engine->isRecordIndexValid(index, "setText"))
		return 0;

	if (!text)
		return 0;

	TextObject_v1_2 *object = _engine->textObjectFromHandle(_engine->_records[index].object);
	if (!object) {
		warning("EnginePluginHost::setText(): the text object %d does not exist", _engine->_records[index].object);
		return 0;
	}

	if (strchr(text, '%'))
		warning("EnginePluginHost::setText(): the string '%s' is a format without arguments", text);

	Common::String formatted = TopgunEngine_v1_2::formatWindowsString(text, nullptr, 0);
	if (formatted.size() >= TG_TEXT_V1_2_TEXT_SIZE)
		warning("EnginePluginHost::setText(): the string '%s' does not fit in the text object", text);

	Common::strlcpy(object->text, formatted.c_str(), TG_TEXT_V1_2_TEXT_SIZE);

	return _engine->refreshTextObjectBitmap(object->handle) ? 1 : 0;
}

int16 EnginePluginHost::compareStringsIgnoreCase(const char *first, const char *second) {
	return TopgunEngine_v1_2::compareStringsCustom(first, second, false);
}

uint32 EnginePluginHost::getConstantStringRoom(uint16 id) {
	return _engine->resourceLib()->resGetConstStrRoom(_engine->_file, id & TG_HOST_V1_2_STRING_ID_MASK);
}

byte *EnginePluginHost::tgMalloc(uint32 size, uint16 zeroInit) {
	return _engine->utilsLibrary()->tgMalloc(size, zeroInit);
}

byte *EnginePluginHost::tgFree(byte *block) {
	return _engine->utilsLibrary()->tgFree(block);
}

uint16 EnginePluginHost::globalAlloc(uint32 size) {
	Common::Array<byte> *block = new Common::Array<byte>();
	block->resize(size);

	for (uint16 i = 0; i < _globalBlocks.size(); i++) {
		if (!_globalBlocks[i]) {
			_globalBlocks[i] = block;
			return i + 1;
		}
	}

	_globalBlocks.push_back(block);
	return _globalBlocks.size();
}

byte *EnginePluginHost::globalLock(uint16 handle, uint32 &size) {
	if (handle == 0 || handle > _globalBlocks.size() || !_globalBlocks[handle - 1]) {
		warning("EnginePluginHost::globalLock(): the memory handle %d does not exist", handle);
		size = 0;
		return nullptr;
	}

	size = _globalBlocks[handle - 1]->size();
	return _globalBlocks[handle - 1]->data();
}

void EnginePluginHost::globalFree(uint16 handle) {
	if (handle == 0 || handle > _globalBlocks.size())
		return;

	delete _globalBlocks[handle - 1];
	_globalBlocks[handle - 1] = nullptr;
}

ResourceRecord *EnginePluginHost::getResource(uint16 id) {
	if (!_engine->isRecordIndexValid(id, "getResource"))
		return nullptr;

	return &_engine->_records[id];
}

bool EnginePluginHost::isPaletteDevice() {
	return _engine->display()->hasPalette();
}

void EnginePluginHost::getPalette(byte *entries, uint16 first, uint16 count) {
	_engine->spriteLib()->spriteGetPalette(entries, first, count);
}

void EnginePluginHost::replaceSpriteBitmap(uint16 sprite, byte *oldBitmap, byte *newBitmap) {
	uint16 bitmapResourceIdOffset = (_engine->_version >= kVersionV1_2_3_2) ? TG_HOST_V1_2_4_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID;

	CelReference oldCel(oldBitmap, READ_LE_UINT16(oldBitmap + bitmapResourceIdOffset));
	CelReference newCel(newBitmap, READ_LE_UINT16(newBitmap + bitmapResourceIdOffset));

	_engine->spriteLib()->spriteReplaceBitmap(sprite, oldCel, newCel);
}

byte *EnginePluginHost::deviceBitmapToDibData(const Graphics::ManagedSurface *bitmap, bool remap, byte color) {
	return _engine->createDIBFromBitmapHandle(bitmap, remap, color);
}

void EnginePluginHost::mapBytes(byte *buffer, uint32 count, const byte *table) {
	_engine->utilsLibrary()->mapBytes(buffer, count, table);
}

uint16 EnginePluginHost::createFont(int16 height, const char *face) {
	return _engine->createTextObjectFont(height, face);
}

void EnginePluginHost::deleteFont(uint16 font) {
	_engine->deleteTextObjectFont(font);
}

const Graphics::Font *EnginePluginHost::getFont(uint16 font) {
	for (uint32 i = 0; i < _engine->_textFonts.size(); i++) {
		if (_engine->_textFonts[i].handle == font)
			return _engine->_textFonts[i].font;
	}

	return nullptr;
}

void EnginePluginHost::copyStringVariable(uint16 index, char *destination) {
	if (index == 0 || _engine->_currentSceneIndex >= _engine->_sceneEntries.size())
		return;

	const SceneEntry_v1_2 &entry = _engine->_sceneEntries[_engine->_currentSceneIndex];
	if (entry.stringCount < index)
		return;

	Common::strlcpy(destination, (const char *)entry.strings + (index - 1) * TG_HOST_V1_2_STRING_SLOT_SIZE, TG_PLUGIN_V1_2_4_STRING_COPY_SIZE);
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

char *EnginePluginHost::getVariableString(uint16 id) {
	if (id == 0 || (id & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
		return nullptr;

	return _engine->getStringFromId(id);
}

uint16 EnginePluginHost::readAddress(uint16 address) {
	return _engine->readAddress(address);
}

void EnginePluginHost::writeAddress(uint16 address, uint16 value) {
	_engine->writeAddress(address, value);
}

uint16 EnginePluginHost::runMessage(uint16 script, const uint16 *arguments, uint16 count) {
	byte record[TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2 * TG_PLUGIN_V1_2_12_RUN_MESSAGE_MAX_ARGUMENTS];
	memset(record, 0, sizeof(record));

	if (count > TG_PLUGIN_V1_2_12_RUN_MESSAGE_MAX_ARGUMENTS) {
		warning("EnginePluginHost::runMessage(): %d arguments are too many", count);
		count = TG_PLUGIN_V1_2_12_RUN_MESSAGE_MAX_ARGUMENTS;
	}

	if (_engine->_version >= kVersionV2_0) {
		WRITE_LE_UINT16(record + 2, script);
		record[TG_SCRIPT_V2_0_CALL_FRAME_SLOTS] = TG_SCRIPT_V2_0_HOST_FRAME_SLOTS;
		record[TG_SCRIPT_V2_0_CALL_COUNT] = (byte)count;

		for (uint16 i = 0; i < count; i++)
			WRITE_LE_UINT16(record + TG_SCRIPT_V2_0_CALL_HEADER_SIZE + 2 * i, arguments[i]);

		_engine->_scriptResult = 1;
		static_cast<TopgunEngine_v2_0 *>(_engine)->callScriptWithArguments(record);

		return _engine->_scriptResult;
	}

	WRITE_LE_UINT16(record, kOpV1_2_4CallScript);
	WRITE_LE_UINT16(record + 2, (uint16)(script - TG_HOST_V1_2_4_LITERAL_BIAS));
	record[4] = TG_PLUGIN_V1_2_12_RUN_MESSAGE_FRAME_OFFSET;
	record[5] = (byte)count;

	for (uint16 i = 0; i < count; i++)
		WRITE_LE_UINT16(record + TG_PLUGIN_V1_2_12_RUN_MESSAGE_HEADER_SIZE + 2 * i, (uint16)(arguments[i] - TG_HOST_V1_2_4_LITERAL_BIAS));

	_engine->_scriptResult = 1;
	_engine->o_callScript(record);

	return _engine->_scriptResult;
}

uint16 EnginePluginHost::getModulePath(char *destination, uint16 size) {
	if (size != 0)
		destination[0] = '\0';

	return 0;
}

uint16 EnginePluginHost::getErrorFileHandle() {
	return static_cast<TopgunEngine_v1_2_4 *>(_engine)->_reservedScriptWord;
}

MmioFile *EnginePluginHost::findFile(uint16 handle) {
	if (handle == 0 || handle > _files.size() || !_files[handle - 1].open)
		return nullptr;

	return &_files[handle - 1];
}

uint16 EnginePluginHost::mmioOpen(const char *name, uint32 flags) {
	Common::SaveFileManager *saveFileManager = g_system->getSavefileManager();
	Common::String fileName = Common::Path(name, '\\').getLastComponent().toString();

	if ((flags & kMmioDelete) != 0)
		return saveFileManager->removeSavefile(fileName) ? 1 : 0;

	if ((flags & kMmioExist) != 0)
		return (saveFileManager->exists(fileName) || SearchMan.hasFile(Common::Path(fileName))) ? 1 : 0;

	MmioFile file;
	file.open = true;
	file.name = fileName;
	file.writable = ((flags & 3) != kMmioRead);

	if ((flags & kMmioCreate) != 0) {
		file.modified = true;
	} else {
		Common::SeekableReadStream *stream = saveFileManager->openForLoading(fileName);
		if (!stream)
			stream = SearchMan.createReadStreamForMember(Common::Path(fileName));

		if (!stream)
			return 0;

		file.data.resize(stream->size());
		if (!file.data.empty())
			stream->read(file.data.data(), file.data.size());

		delete stream;
	}

	for (uint16 i = 0; i < _files.size(); i++) {
		if (!_files[i].open) {
			_files[i] = file;
			return i + 1;
		}
	}

	_files.push_back(file);
	return _files.size();
}

uint16 EnginePluginHost::mmioClose(uint16 handle) {
	MmioFile *file = findFile(handle);
	if (!file)
		return 1;

	bool saved = true;

	if (file->writable && file->modified) {
		Common::OutSaveFile *stream = g_system->getSavefileManager()->openForSaving(file->name, false);

		if (stream) {
			if (!file->data.empty())
				stream->write(file->data.data(), file->data.size());

			stream->finalize();
			delete stream;
		} else {
			warning("EnginePluginHost::mmioClose(): cannot write the file '%s'", file->name.c_str());
			saved = false;
		}
	}

	*file = MmioFile();
	return saved ? 0 : 1;
}

int32 EnginePluginHost::mmioRead(uint16 handle, byte *buffer, int32 count) {
	MmioFile *file = findFile(handle);
	if (!file)
		return -1;

	if (count <= 0 || file->position >= file->data.size())
		return 0;

	uint32 length = MIN<uint32>(count, file->data.size() - file->position);

	memcpy(buffer, file->data.data() + file->position, length);
	file->position += length;
	return length;
}

int32 EnginePluginHost::mmioWrite(uint16 handle, const byte *buffer, int32 count) {
	MmioFile *file = findFile(handle);
	if (!file || !file->writable)
		return -1;

	if (count <= 0)
		return 0;

	if (file->position + count > file->data.size())
		file->data.resize(file->position + count);

	memcpy(file->data.data() + file->position, buffer, count);
	file->position += count;
	file->modified = true;
	return count;
}

int32 EnginePluginHost::mmioSeek(uint16 handle, int32 offset, int origin) {
	MmioFile *file = findFile(handle);
	if (!file)
		return -1;

	int64 target;

	if (origin == SEEK_SET)
		target = offset;
	else if (origin == SEEK_CUR)
		target = (int64)file->position + offset;
	else if (origin == SEEK_END)
		target = (int64)file->data.size() + offset;
	else
		return -1;

	if (target < 0)
		return -1;

	file->position = (uint32)target;
	return (int32)file->position;
}

void EnginePluginHost::setMessageFilter(Plugin *plugin) {
	_engine->_messageFilter = plugin;
}

void EnginePluginHost::setNetworkMessageCallback(Plugin *plugin) {
	_engine->_windowMessageCallback = plugin;
}

void EnginePluginHost::setScriptHook(Plugin *plugin) {
	_engine->_scriptHook = plugin;
}

void EnginePluginHost::repostMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	_engine->postMessage(message, wParam, ((uint32)lParamHigh << 16) | lParamLow);
}

Plugin *createPlugin(const Common::String &name, PluginHost *host) {
	if (name.equalsIgnoreCase("BATTLE.DLL"))
		return new BattlePlugin(host);

	if (name.equalsIgnoreCase("BEAST.DLL"))
		return new BattleDemoPlugin(host);

	if (name.equalsIgnoreCase("ACE1UTIL.R16"))
		return new Ace1UtilPlugin();

	if (name.equalsIgnoreCase("FILEIO.R16"))
		return new FileIoPlugin(host);

	if (name.equalsIgnoreCase("DYNALLOC.R16"))
		return new DynAllocPlugin(host);

	if (name.equalsIgnoreCase("PATHFIND.R16"))
		return new PathFindPlugin(host);

	if (name.equalsIgnoreCase("RE.R16"))
		return new RePlugin(host, name);

	if (name.equalsIgnoreCase("WORDLIST.R16"))
		return new WordListPlugin(host, name);

	return nullptr;
}

bool TopgunEngine_v1_2::loadPluginLibrary(const Common::String &name) {
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

void TopgunEngine_v1_2::unloadPluginLibrary() {
	if (!_plugin)
		return;

	_plugin->free();

	delete _plugin;
	_plugin = nullptr;

	_messageFilter = nullptr;
	_windowMessageCallback = nullptr;
	_scriptHook = nullptr;
}

bool TopgunEngine_v1_2::loadPluginFunctionTable() {
	uint32 size = 0;
	uint32 librariesSize = 0;
	byte *libraries = nullptr;
	byte *names;

	_pluginFunctions.clear();

	ResourceLibrary_v1_2_4 *library = static_cast<ResourceLibrary_v1_2_4 *>(resourceLib());

	if (_version >= kVersionV1_2_12) {
		libraries = library->resLoadKeyResource(_file, TG_PLUGIN_V1_2_12_KEY_FUNCTION_LIBRARIES, &librariesSize);
		if (!libraries)
			return false;

		names = library->resLoadKeyResource(_file, TG_PLUGIN_V1_2_12_KEY_FUNCTION_NAMES, &size);
		if (!names) {
			utilsLibrary()->tgFree(libraries);
			return false;
		}
	} else {
		uint16 key = TG_RES_V1_2_4_KEY_PLUGIN_NAMES;
		if (_version < kVersionV1_2_3_2)
			key = TG_RES_V1_2_3_1_KEY_PLUGIN_NAMES;

		names = library->resLoadKeyResource(_file, key, &size);
		if (!names)
			return size == 0;
	}

	uint32 consumed = 0;
	uint16 index = 0;

	while (consumed < size) {
		uint32 length = 0;
		while (consumed + length < size && names[consumed + length] != 0)
			length++;

		PluginFunction_v1_2_4 entry;
		Plugin *plugin = _plugin;

		if (_version >= kVersionV1_2_12) {
			if (2 * (uint32)index + 2 > librariesSize) {
				warning("TopgunEngine_v1_2::loadPluginFunctionTable(): function %d has no library", index);
				break;
			}

			entry.library = READ_LE_UINT16(libraries + 2 * index);
			if (entry.library >= _pluginLibraries.size()) {
				warning("TopgunEngine_v1_2::loadPluginFunctionTable(): library %d does not exist", entry.library);
				break;
			}

			plugin = _pluginLibraries[entry.library]->plugin;
		}

		Common::String name((const char *)names + consumed, length);

		entry.function = plugin->findFunction(name);
		if (entry.function < 0) {
			warning("TopgunEngine_v1_2::loadPluginFunctionTable(): the function '%s' does not exist", name.c_str());
			break;
		}

		_pluginFunctions.push_back(entry);
		consumed += length + 1;
		index++;
	}

	utilsLibrary()->tgFree(names);

	if (libraries)
		utilsLibrary()->tgFree(libraries);

	return consumed >= size;
}

void TopgunEngine_v1_2::exprCallPluginFunction(uint16 function, uint16 count) {
	_expressionStackPointer -= count;

	Plugin *plugin = _plugin;

	if (_version >= kVersionV1_2_12 && function < _pluginFunctions.size())
		plugin = _pluginLibraries[_pluginFunctions[function].library]->plugin;

	if (!plugin || function >= _pluginFunctions.size()) {
		warning("TopgunEngine_v1_2::exprCallPluginFunction(): plug-in function %d does not exist", function);
		_expressionStack[_expressionStackPointer - 1] = 0;
		return;
	}

	_expressionStack[_expressionStackPointer - 1] = plugin->callFunction(_pluginFunctions[function].function, &_expressionStack[_expressionStackPointer], count);
}

bool TopgunEngine_v1_2::callMessageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) {
	return _messageFilter && _messageFilter->messageFilter(message, wParam, lParamLow, lParamHigh) != 0;
}

bool TopgunEngine_v1_2::callScriptHook(uint16 script) {
	if (_argumentFrame >= TG_HOST_V1_2_3_1_HOOK_ARGUMENTS - 1)
		return _scriptHook->scriptHook(script, &_variableTable[_argumentFrame]);

	warning("TopgunEngine_v1_2::callScriptHook(): the argument frame %d is too close to the first variable", _argumentFrame);

	uint16 arguments[TG_HOST_V1_2_3_1_HOOK_ARGUMENTS] = {};
	for (uint16 i = 0; i <= _argumentFrame; i++)
		arguments[TG_HOST_V1_2_3_1_HOOK_ARGUMENTS - 1 - i] = _variableTable[_argumentFrame - i];

	return _scriptHook->scriptHook(script, &arguments[TG_HOST_V1_2_3_1_HOOK_ARGUMENTS - 1]);
}

bool TopgunEngine_v1_2_4::loadAndRegisterLibraries() {
	uint32 size = 0;
	byte *names = resourceLib()->resLoadKeyResource(_file, TG_PLUGIN_V1_2_12_KEY_LIBRARY_NAMES, &size);

	if (!names)
		return size == 0;

	bool ok = true;
	uint32 consumed = 0;

	while (ok && consumed < size) {
		uint32 length = 0;
		while (consumed + length < size && names[consumed + length] != 0)
			length++;

		Common::String fileName = Common::String((const char *)names + consumed, length) + ".r16";

		PluginModule_v1_2_12 *module = loadPluginModule(fileName);
		if (!module) {
			fatalError(102, Common::String::format("Can't open file '%s'.", fileName.c_str()));
			ok = false;
		} else if (!registerExternalLibrary(module)) {
			freePluginModule(module);
			fatalError(102, Common::String::format("Can't open file '%s'.", fileName.c_str()));
			ok = false;
		}

		consumed += length + 1;
	}

	if (ok && !loadPluginFunctionTable()) {
		fatalError(100, "Not enough memory to run program. Please\nrefer to the trouble shooting instructions.");
		ok = false;
	}

	utilsLibrary()->tgFree(names);

	return ok;
}

PluginModule_v1_2_12 *TopgunEngine_v1_2_4::loadPluginModule(const Common::String &fileName) {
	for (uint16 i = 0; i < _pluginModules.size(); i++) {
		if (fileName.equalsIgnoreCase(_pluginModules[i]->name)) {
			_pluginModules[i]->refCount++;
			return _pluginModules[i];
		}
	}

	if (!_pluginHost)
		_pluginHost = new EnginePluginHost(this);

	Plugin *plugin = createPlugin(fileName, _pluginHost);
	if (!plugin)
		return nullptr;

	PluginModule_v1_2_12 *module = new PluginModule_v1_2_12();
	module->name = fileName;
	module->plugin = plugin;
	module->refCount = 1;

	_pluginModules.push_back(module);
	return module;
}

void TopgunEngine_v1_2_4::freePluginModule(PluginModule_v1_2_12 *module) {
	module->refCount--;

	if (module->refCount != 0)
		return;

	for (uint16 i = 0; i < _pluginModules.size(); i++) {
		if (_pluginModules[i] == module) {
			_pluginModules.remove_at(i);
			break;
		}
	}

	delete module->plugin;
	delete module;
}

bool TopgunEngine_v1_2_4::registerExternalLibrary(PluginModule_v1_2_12 *module) {
	module->plugin->clearCallbacks();

	if (!module->plugin->init()) {
		warning("TopgunEngine_v1_2_4::registerExternalLibrary(): the library '%s' does not initialize", module->name.c_str());
		return false;
	}

	if (module->plugin->hasTickCallback())
		_pluginTickProcCount++;

	_pluginLibraries.push_back(module);
	return true;
}

void TopgunEngine_v1_2_4::unloadAllPluginModules() {
	if (_pluginLibraries.empty())
		return;

	for (int i = (int)_pluginLibraries.size() - 1; i >= 0; i--)
		_pluginLibraries[i]->plugin->free();

	_pluginFunctions.clear();
	_pluginsPendingFree = _pluginLibraries;
	_pluginLibraries.clear();
	_pluginTickProcCount = 0;

	postMessage(kMsgV1_2_12FreePluginModules, 0, 0);
}

void TopgunEngine_v1_2_4::freeUnloadedPluginModules() {
	for (int i = (int)_pluginsPendingFree.size() - 1; i >= 0; i--)
		freePluginModule(_pluginsPendingFree[i]);

	_pluginsPendingFree.clear();
}

void TopgunEngine_v1_2_4::notifyPluginsFocusChanged(bool active) {
	for (int i = (int)_pluginLibraries.size() - 1; i >= 0; i--) {
		if (_pluginLibraries[i]->plugin->hasFocusCallback())
			_pluginLibraries[i]->plugin->focusChanged(active);
	}
}

void TopgunEngine_v1_2_4::notifyServicePause(bool paused) {
	for (int i = (int)_pluginLibraries.size() - 1; i >= 0; i--) {
		if (_pluginLibraries[i]->plugin->hasPauseCallback())
			_pluginLibraries[i]->plugin->pauseChanged(paused);
	}
}

void TopgunEngine_v1_2_4::notifyPluginsLifecycleEvent(uint16 mode, const char *name) {
	for (int i = (int)_pluginLibraries.size() - 1; i >= 0; i--) {
		if (_pluginLibraries[i]->plugin->hasSceneCallback())
			_pluginLibraries[i]->plugin->sceneEvent(mode, name);
	}
}

void TopgunEngine_v1_2_4::runRegisteredServiceCallbacks() {
	uint16 remaining = _pluginTickProcCount;

	for (int i = (int)_pluginLibraries.size() - 1; i >= 0 && remaining != 0; i--) {
		if (_pluginLibraries[i]->plugin->hasTickCallback()) {
			_pluginLibraries[i]->plugin->tick();
			remaining--;
		}
	}
}

} // End of namespace Topgun
