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

#ifndef TOPGUN_PLUGIN_H
#define TOPGUN_PLUGIN_H

#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"

namespace Graphics {

class Font;
class ManagedSurface;

} // End of namespace Graphics

namespace Topgun {

struct ResourceRecord;
struct SpriteInfo_v1_2_4;

enum MmioFlag {
	kMmioRead = 0,
	kMmioWrite = 1,
	kMmioReadWrite = 2,
	kMmioDenyNone = 0x40,
	kMmioDelete = 0x200,
	kMmioCreate = 0x1000,
	kMmioExist = 0x4000
};

enum PluginSceneEvent {
	kPluginSceneLoaded = 1,
	kPluginSceneSwitching = 2,
	kPluginSceneClosed = 3
};

class PluginServices {
public:
	virtual ~PluginServices() {}

	virtual uint16 *variableTable() = 0;

	virtual void setScriptTimer(uint16 timerId, uint32 delay, uint16 script) = 0; // RT_001
	virtual void cancelScriptTimer(uint16 timerId) = 0; // RT_002
	virtual uint16 joystickMessageToIndex(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) = 0; // RT_005
	virtual void setSpriteQueue(uint16 sprite, uint16 data, uint16 flag) = 0; // RT_006
	virtual void setSpritePosition(uint16 sprite, uint16 x, uint16 y) = 0; // RT_007
	virtual void getSpriteRect(uint16 sprite, Common::Rect &rect) = 0; // RT_008
	virtual void setSpriteLevel(uint16 sprite, uint16 level) = 0; // RT_009
	virtual void loadResource(uint16 id) = 0; // RT_021
	virtual void unloadResource(uint16 id) = 0; // RT_022
	virtual void transferSprite(uint16 source, uint16 destination, uint16 data, byte freeSource, byte flag) = 0; // RT_023
	virtual uint16 callScript(uint16 script, uint16 argument1, uint16 argument2) = 0; // RT_024
	virtual void getSpriteFrameIds(uint16 sprite, uint16 *ids) = 0; // RT_025
	virtual uint16 spritesCollide(uint16 first, uint16 second, const Common::Rect *rect) = 0; // RT_027
	virtual void stopSprite(uint16 sprite) = 0; // RT_028
	virtual uint16 isSpriteVisible(uint16 sprite) = 0; // RT_029
	virtual void addCollisionPair(uint16 first, uint16 second, uint16 script, uint16 argument1, uint16 argument2, byte repeat) = 0; // RT_030
	virtual void getSpritePosition(uint16 sprite, Common::Point &position) = 0; // RT_031
	virtual void spriteSkipMessage(uint16 sprite, uint16 count) = 0; // RT_032
	virtual void spriteSwapOrder(uint16 first, uint16 second, uint16 onlyIfSecondAbove, uint16 refresh) = 0; // RT_033
	virtual void getSpriteInfo(uint16 sprite, SpriteInfo_v1_2_4 &info) = 0; // RT_034
	virtual void setSpriteSpeedMotion(uint16 sprite, uint16 speed, uint16 motion) = 0; // RT_035
	virtual char *getString(uint16 id) = 0; // RT_036
	virtual void setString(uint16 id, const char *text) = 0;
	virtual uint16 setText(uint16 id, const char *text) = 0;
	virtual int16 compareStringsIgnoreCase(const char *first, const char *second) = 0;
	virtual void copyStringVariable(uint16 index, char *destination) = 0; // RT_037
	virtual void setScriptResult(uint16 value) = 0; // RT_038
	virtual uint16 getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue) = 0;
	virtual void writePrivateProfileString(const char *section, const char *key, const char *value) = 0;

	virtual char *getVariableString(uint16 id) = 0;
	virtual uint16 readAddress(uint16 address) = 0;
	virtual void writeAddress(uint16 address, uint16 value) = 0;
	virtual uint16 runMessage(uint16 script, const uint16 *arguments, uint16 count) = 0;
	virtual uint16 getModulePath(char *destination, uint16 size) = 0;
	virtual uint16 getErrorFileHandle() = 0;

	virtual uint32 getConstantStringRoom(uint16 id) = 0;
	virtual byte *tgMalloc(uint32 size, uint16 zeroInit) = 0;
	virtual byte *tgFree(byte *block) = 0;
	virtual uint16 globalAlloc(uint32 size) = 0;
	virtual byte *globalLock(uint16 handle, uint32 &size) = 0;
	virtual void globalFree(uint16 handle) = 0;

	virtual ResourceRecord *getResource(uint16 id) = 0;
	virtual bool isPaletteDevice() = 0;
	virtual void getPalette(byte *entries, uint16 first, uint16 count) = 0;
	virtual void replaceSpriteBitmap(uint16 sprite, byte *oldBitmap, byte *newBitmap) = 0;
	virtual byte *deviceBitmapToDibData(const Graphics::ManagedSurface *bitmap, bool remap, byte color) = 0;
	virtual void mapBytes(byte *buffer, uint32 count, const byte *table) = 0;
	virtual uint16 createFont(int16 height, const char *face) = 0;
	virtual void deleteFont(uint16 font) = 0;
	virtual const Graphics::Font *getFont(uint16 font) = 0;

	virtual uint16 mmioOpen(const char *name, uint32 flags) = 0;
	virtual uint16 mmioClose(uint16 handle) = 0;
	virtual int32 mmioRead(uint16 handle, byte *buffer, int32 count) = 0;
	virtual int32 mmioWrite(uint16 handle, const byte *buffer, int32 count) = 0;
	virtual int32 mmioSeek(uint16 handle, int32 offset, int origin) = 0;
};

class Plugin;

class PluginHost : public PluginServices {
public:
	virtual void setMessageFilter(Plugin *plugin) = 0; // RT_039
	virtual void setNetworkMessageCallback(Plugin *plugin) = 0; // RT_040
	virtual void setScriptHook(Plugin *plugin) = 0;

	// Puts a message that a filter has consumed back in the queue
	virtual void repostMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) = 0;
};

class Plugin {
public:
	virtual ~Plugin() {}

	virtual bool init() { return true; }
	virtual void free() = 0;
	virtual int findFunction(const Common::String &name) const = 0;
	virtual uint16 callFunction(int function, const uint16 *arguments, uint16 count) = 0;
	virtual uint16 messageFilter(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) { return 0; }
	virtual uint32 windowMessage(uint16 message, uint16 wParam, uint16 lParamLow, uint16 lParamHigh) { return 0; }

	// Called before a script; returning true skips it. The arguments are stored at decreasing addresses
	virtual bool scriptHook(uint16 script, const uint16 *arguments) { return false; }

	virtual void tick() {}
	virtual void focusChanged(bool active) {}
	virtual void pauseChanged(bool paused) {}
	virtual void sceneEvent(uint16 mode, const char *name) {}

	void clearCallbacks() {
		_hasTickCallback = false;
		_hasFocusCallback = false;
		_hasPauseCallback = false;
		_hasSceneCallback = false;
	}

	bool hasTickCallback() const { return _hasTickCallback; }
	bool hasFocusCallback() const { return _hasFocusCallback; }
	bool hasPauseCallback() const { return _hasPauseCallback; }
	bool hasSceneCallback() const { return _hasSceneCallback; }

protected:
	bool _hasTickCallback = false;
	bool _hasFocusCallback = false;
	bool _hasPauseCallback = false;
	bool _hasSceneCallback = false;
};

Plugin *createPlugin(const Common::String &name, PluginHost *host);

} // End of namespace Topgun

#endif // TOPGUN_PLUGIN_H
