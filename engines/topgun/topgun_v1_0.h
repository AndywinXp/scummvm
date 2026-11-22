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

#ifndef TOPGUN_TOPGUN_V1_0_H
#define TOPGUN_TOPGUN_V1_0_H

#include "common/events.h"
#include "common/hashmap.h"
#include "common/keyboard.h"
#include "common/path.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "topgun/topgun.h"

#define TG_HOST_PROGRESS_INTERVAL     0x2000
#define TG_HOST_SCENE_NAME_SIZE       13
#define TG_HOST_NUM_SCENE_FLAGS       30
#define TG_HOST_MAX_SCENES            10
#define TG_HOST_NUM_GLOBAL_FLAGS      10
#define TG_HOST_MAX_HOTSPOTS          100
#define TG_HOST_MAX_TIMERS            20
#define TG_HOST_NUM_DISSOLVE_TILES    300
#define TG_HOST_LOOP_DELAY            4
#define TG_HOST_MAX_POSTED_MESSAGES   8
#define TG_HOST_MAX_SCRIPT_DEPTH      256
#define TG_HOST_NUM_SCRIPT_CHOICES    6
#define TG_HOST_FIRST_AUDIO_TRACK     2

namespace Topgun {

struct SpriteInfo;

enum AudioCommand {
	kPlayCDTracksCommand = 0x6F,
	kStopCDCommand = 0x73,
	kStopWavesCommand = 0xD2,
	kDestroySoundCommand = 0x12C,
	kStartSoundCommand = 0x131,
	kStopSoundCommand = 0x134
};

enum ScriptOpcode {
	kOpCallScript = 1,
	kOpLoadResource = 2,
	kOpUnloadResource = 3,
	kOpSetClickScript = 4,
	kOpSetSpriteQueue = 5,
	kOpSetBackground = 6,
	kOpPlayWave = 8,
	kOpPlayCDTrack = 9,
	kOpFadeIn = 10,
	kOpFadeOut = 11,
	kOpBindKeyScript = 12,
	kOpQuit = 13,
	kOpStopCD = 14,
	kOpStopWave = 15,
	kOpSetFlag = 16,
	kOpTestFlag = 17,
	kOpGotoScene = 18,
	kOpSetScriptTimer = 19,
	kOpCancelScriptTimer = 20,
	kOpSetSceneEndScript = 21,
	kOpRandomRunScript = 22,
	kOpRoundRobinRunScript = 23,
	kOpTransferSprite = 24,
	kOpBreakSpriteLoops = 25,
	kOpSetSpriteQueueIfLoaded = 26,
	kOpDebugString = 27,
	kOpSetSpriteLevel = 28,
	kOpSetActivationScript = 29,
	kOpPostScript = 30,
	kOpCursorOff = 31,
	kOpCursorOn = 32,
	kOpRunScriptIfLoaded = 33
};

struct Message {
	uint16 id;
	uint16 param;
	uint32 data;
	bool shift;
	bool ctrl;

	Message() : id(0), param(0), data(0), shift(false), ctrl(false) {}
};

struct KeyScriptRecord {
	uint16 unused;
	uint16 plain;
	uint16 shift;
	uint16 ctrl;

	KeyScriptRecord() : unused(0), plain(0), shift(0), ctrl(0) {}
};

typedef Common::HashMap<int, KeyScriptRecord> KeyScriptMap;

struct HotspotRecord {
	int16 left;
	int16 top;
	int16 right;
	int16 bottom;
	uint16 script;

	HotspotRecord() : left(0), top(0), right(0), bottom(0), script(0) {}
};

struct ScheduledTimerRecord {
	uint32 expiry;
	uint16 id;
	uint16 script;

	ScheduledTimerRecord() : expiry(0), id(0), script(0) {}
};

enum ResourceType {
	kResourceBitmap = 1,
	kResourceBlob = 2,
	kResourceWave = 3,
	kResourceGroup = 5,
	kResourceSprite = 6,
	kResourceScript = 7,
	kResourceTransPalette = 8,
	kResourcePlugin = 9,
	kResourceIdList = 10
};

struct ResourceRecord {
	byte *data;
	uint16 handle;
	uint16 clickScript;
	uint32 size;
	uint16 refCount;
	uint16 numMembers;
	byte *idList;
	uint16 object;

	ResourceRecord() : data(nullptr), handle(0), clickScript(0), size(0), refCount(0),
		numMembers(0), idList(nullptr), object(0) {}
};

struct SceneRecord {
	char name[TG_HOST_SCENE_NAME_SIZE];
	uint16 flags[TG_HOST_NUM_SCENE_FLAGS];

	SceneRecord() {
		memset(name, 0, sizeof(name));
		memset(flags, 0, sizeof(flags));
	}
};

struct GotoSceneRecord {
	uint16 opcode;
	char name[TG_HOST_SCENE_NAME_SIZE];
	byte releaseSceneSprite;

	GotoSceneRecord() : opcode(0), releaseSceneSprite(0) {
		memset(name, 0, sizeof(name));
	}
};

class TopgunEngine_v1_0 : public TopgunEngine {
public:
	TopgunEngine_v1_0(OSystem *syst, const ADGameDescription *gameDesc, int version = 0);
	~TopgunEngine_v1_0() override;

protected:
	// Main loop
	bool rtStartup(bool runChecks) override;
	bool runStartup(bool runChecks, const Common::String &firstContainer);
	virtual void initCDAudio();
	uint16 mainLoop() override;
	void presentScreenAndWait();
	void resourceLoadProgressCallback() override;
	void clearInputMessageQueues();

	// Startup
	virtual bool initEngineLibraries();
	bool locateResourceFile(const Common::String &name);

	// Messages
	void requestClose() override;
	bool postMessage(uint16 id, uint16 param, uint32 data) override;
	bool peekMessage(Message &message);
	void removeMessages(uint16 id);
	void clearMessages();
	void invalidateScreen();
	void validateScreen();
	void postQuitMessage(uint16 exitCode);
	virtual void dispatchMessage(const Message &message);
	virtual void handleActivateMessage(uint16 param);
	virtual void handleAppShutdown();
	virtual void handlePaintMessage();
	virtual void handleSceneEndMessage();
	virtual void fillBackground();

	// Input
	virtual bool peekInputMessage(Message &message);
	void discardPendingInput(uint16 id);
	virtual bool translate(const Common::Event &event, Message &message);
	uint16 scriptKeyToKeyCodes(uint16 scriptKey, Common::KeyCode *keyCodes) const;
	virtual void handleKeyDownMessage(Common::KeyCode keycode, bool shift, bool ctrl);
	void runKeyScript(Common::KeyCode keycode, bool shift, bool ctrl);
	virtual void handleMouseButtonDown(int16 x, int16 y);
	void runClickScriptAt(int16 x, int16 y);
	void addOrUpdateClickRegion(const byte *record);

	// Cursor
	virtual void applyCursorFromRegister();
	void setCursorImage(bool hourglass);

	// Volume
	virtual void adjustMasterVolumeByKey(Common::KeyCode keycode);
	static uint32 stepVolumeLevels5Bit(uint32 volume, bool up);

	// Timers
	virtual void serviceScheduledMsgTimerQueue();
	void removeScheduledTimer(uint16 index);

	// Scenes
	SceneRecord *findOrCreateSceneEntry(const Common::String &name);
	virtual bool loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback);
	virtual void switchToNextScene(const char *name);
	virtual bool loadInitialScene(const Common::String &name);
	virtual void cleanupCurrentSceneAndContainer();

	// Resources
	virtual void freeBitmapBlobIfUnreferenced(uint16 id);
	void destroyDataResourceObject(uint16 id);
	virtual bool destroyGroupResourceObject(uint16 id);
	virtual bool destroySpriteResourceObject(uint16 id);
	virtual void destroyWaveResourceObject(uint16 id);
	virtual bool ensureBitmapResourceLoaded(uint16 id);
	virtual bool loadOrGetResourceData(uint16 id);
	virtual bool ensureGroupResourceLoaded(uint16 id);
	virtual bool loadTransPaletteResource(uint16 id);
	virtual byte *loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut);
	virtual bool ensureSpriteResourceLoaded(uint16 id);
	bool attachResourceListToSprite(uint16 spriteHandle, const byte *idList, uint16 count);
	virtual bool ensureWaveResourceLoaded(uint16 id);
	virtual bool destroyResourceObjectByType(uint16 id, uint16 type);
	virtual void resetResourcesAndTransientState();
	void destroySceneMainObjects();
	virtual bool validateAndGetResourceByType(uint16 id, uint16 type);
	virtual bool isResourceObjectLoaded(uint16 id);
	virtual void decrementResourceRefCountAndMaybeDestroy(uint16 bitmapId);
	void refreshGamePalette(byte *buffer);
	virtual bool openResourceFileAndInit(const Common::String &name);

	// Scripts
	virtual bool runScriptBytecodeLoop(uint16 scriptId);
	void spriteMessageService(byte *record) override;
	virtual bool runScriptResource(uint16 scriptId);
	void printDebugString(const byte *text, uint16 value);

	// Opcodes
	bool o_setClickScript(const byte *record); // 4
	bool o_setSpriteQueue(uint16 spriteId, uint16 dataId); // 5
	bool o_playWave(uint16 waveId, byte *params); // 8
	virtual void o_playCDTrack(uint16 track); // 9
	void o_fadeInOut(const byte *record); // 10, 11
	void o_bindKeyScript(const byte *record); // 12
	void o_stopCD(); // 14
	bool o_stopWave(uint16 waveId, byte *block); // 15
	void o_setScriptTimer(const byte *record); // 19
	void o_cancelScriptTimer(const byte *record); // 20
	virtual bool o_randomRunScript(byte *record); // 22
	virtual bool o_roundRobinRunScript(byte *record); // 23
	bool o_transferSprite(uint16 sourceSpriteId, uint16 destinationSpriteId, uint16 dataId, bool freeSource); // 24
	void o_setSpriteLevel(uint16 spriteId, uint16 level); // 28
	bool transferSpriteContents(uint16 sourceSpriteId, uint16 destinationSpriteId, bool freeSource);
	bool appendQueueBitmapsToSprite(uint16 destinationSpriteId, const SpriteInfo &sourceInfo);
	void releaseSourceSpriteFrameRefs(uint16 spriteId);
	void destroySourceSpriteAfterTransfer(uint16 sourceSpriteId, const SpriteInfo &sourceInfo);
	void trimSpriteQueueBitmaps(uint16 spriteId);

	// Backgrounds
	void syncBackgroundDimensions(uint16 bitmapId);
	virtual void commitBackgroundChangeAndRepaint(const Common::Rect &rect);
	void applyBackgroundToSpriteLib(uint16 bitmapId, Common::Rect *rectOut);
	virtual void clearBackground();
	bool setBackgroundFromBitmapResource(uint16 bitmapId);

	// Transitions
	virtual void copyAndPaintBackgroundRect(Common::Rect &rect);
	void initDissolve();
	void dissolveOneTile();
	void computeInsetRingRect(uint16 k, Common::Rect *out, uint16 n);
	virtual void paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner);
	void wipeEffectFrameRingsInsideOut();
	void wipeEffectFrameRingsOutsideIn();
	virtual void advanceSceneFadeStep();
	virtual void sceneTransitionIn();
	virtual void sceneTransitionOut();

	// Main loop
	bool _shuttingDown = false;

	// Messages
	Message _messages[TG_HOST_MAX_POSTED_MESSAGES];
	uint16 _numMsgsPosted = 0;
	bool _paintPending = false;
	bool _quit = false;
	uint16 _quitCode = 0;

	// Host state
	bool _flagMode640 = false;
	bool _flagLoMem = false;
	bool _appActive = false;
	bool _cursorEnabled = false;
	bool _inputBusy = false;
	bool _cdAudioPlaying = false;
	uint16 _globalFlags[TG_HOST_NUM_GLOBAL_FLAGS];
	KeyScriptMap _keyScripts;
	HotspotRecord _hotspots[TG_HOST_MAX_HOTSPOTS];
	uint16 _numHotspots = 0;
	ScheduledTimerRecord _timers[TG_HOST_MAX_TIMERS];
	uint16 _numTimers = 0;
	uint16 _sceneEndScript = 0;
	uint16 _activationScript = 0;
	uint16 _scriptDepth = 0;

	// Scenes
	GotoSceneRecord _gotoRecord;
	SceneRecord *_currentScene = nullptr;
	uint16 _numScenes = 0;
	SceneRecord _scenes[TG_HOST_MAX_SCENES];

	// Resources
	uint16 _file = 0;
	ResourceRecord *_records = nullptr;
	uint16 _numResources = 0;
	bool _freeingAll = false;
	uint16 _sceneSprite = 0;
	uint16 _sceneSpriteWave = 0;
	uint16 _loadingResourceId = 0;
	bool _loading = false;
	byte *_resourceNamesTable = nullptr;

	// Backgrounds
	uint16 _width = 0;
	uint16 _height = 0;
	uint16 _backgroundId = 0;
	byte *_buffer1 = nullptr;
	byte *_buffer2 = nullptr;
	bool _faded = false;
	bool _active = false;
	bool _fadeIn = false;
	uint16 _step = 0;
	uint16 _stepCount = 0;
	uint16 _tileSize = 0;
	uint16 _tileOrder[TG_HOST_NUM_DISSOLVE_TILES];
	bool _screenBox = false;
};

} // End of namespace Topgun

#endif // TOPGUN_TOPGUN_V1_0_H
