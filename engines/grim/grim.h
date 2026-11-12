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

#ifndef GRIM_ENGINE_H
#define GRIM_ENGINE_H

#include "engines/engine.h"
#include "engines/advancedDetector.h"

#include "common/str-array.h"
#include "common/hashmap.h"
#include "common/events.h"
#include "common/mutex.h"

#include "graphics/renderer.h"

#include "engines/grim/detection.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_sys.h"

namespace Common {
class SeekableReadStream;
class WriteStream;
}

namespace Grim {

class BlastText;
class IMuse;
class NormalText;
class Polyglot;
class RenderDroid;
class Res;
class StdBitmap;
class StdBmp;
class StdConffile;
class StdControl;
class StdFile;
class StdGif;
class StdLbm;
class Std3D;
class StdMath;
class StdPcx;
class StdPic;
class Text;
class WuRegistry;
class SputActorModule;
class SputControl;
class SputCostume;
class SputPrimitive;
class SputRender;
class SputResource;
class SputRoom;
class SputSmush;
class SputText;
class SputTime;
class StdDisplay;
class SysStruct;
class WalkSystem;
struct Std3DDevice;
struct StdRect;
struct StdVBuffer;

class LuaBase;
class Lua_V1;
class LegacyImporter;

struct ControlDescriptor {
	const char *name;
	int key;
};

enum SputMainModule {
	kSputMainModuleFileUtil,
	kSputMainModuleMemory,
	kSputMainModuleRoom,
	kSputMainModuleTime,
	kSputMainModuleLua,
	kSputMainModuleCostume,
	kSputMainModuleActor,
	kSputMainModuleResource,
	kSputMainModuleControl,
	kSputMainModuleSounds,
	kSputMainModuleRender,
	kSputMainModuleText,
	kSputMainModulePrimitive,
	kSputMainModuleSmush,
	kNumSputMainModules
};

enum {
	kNumBaseModules = 8,
	kNumMainModules = 6,
	kNumPerfCounters = 13,
	kPhysicalRam = 0x7FFFFFFF
};

struct PerfCounter {
	const char *pName;
	double start;
	double elapsed;
};

struct GrimVideoDevice {
	uint numModes;
	StdDisplayMode *aModes;
	uint num3DDevices;
	Std3DDevice *a3DDevices;
};

class GrimEngine : public Engine, public SysStruct {
	friend class Lua_V1;
	friend class LuaBase;
	friend class LegacyImporter;

protected:
	// Engine APIs
	Common::Error run() override;

public:
	GrimEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language);
	virtual ~GrimEngine();

	uint32 getGameFlags() { return _gameFlags; }
	GrimGameType getGameType() { return _gameType; }
	Common::Language getGameLanguage() { return _gameLanguage; }
	Common::Platform getGamePlatform() { return _gamePlatform; }
	virtual const char *getUpdateFilename();
	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override { return true; }
	Common::Error loadGameState(int slot) override;
	bool isRemastered() const { return !!(_gameFlags & ADGF_REMASTERED); }
	bool useRemasterFixes() const { return _bRemasterFixes != 0; }

	void clearEventQueue();

	// sputMain module
	int sputInitialize(SysStruct *pHS);
	int sputTerminate();
	int sputStartup();
	int sputShutdown();
	int callRegisterModules(SysStruct *pHS, int firstModule, int count);
	int runInitTable(int firstModule, int count);
	int callTableFunc(int firstModule, int count);
	int callRelease(int firstModule, int count);
	int bundleDofile();
	int startEngine(int saveGameResume, const char *bootParam);
	int loop(int bRender, int manualTicks);
	void timedPageFlip();
	void setPerfCounter(uint slot);
	int queryPerfCounterDiff(uint slot);
	void save();
	int zwrite(const void *buffer, int size);
	void restore();
	int zread(void *buffer, int size);
	StdVBuffer *getSaveGameImage(const char *fileName);
	void setPrevMode(int mode);
	int getPrevMode() const;
	void imuseSave();
	void imuseRestore();
	PerfCounter *getPerfCounters() { return _aPerfCounters; }
	IMuse *getImuse() const { return _imuse; }
	SputCostume *getSputCostume() const { return _pSputCostume; }
	SputResource *getSputResource() const { return _pSputResource; }

	// grimlib module
	void overrideSelectedActorCostume(int hInstance);
	void restoreSelectedActorCostume();
	int setDisplay(StdVBuffer *pVBuffer, StdVBuffer *pZBuffer);
	int unsetDisplay();
	void makeCurrentSet(const char *name);
	void close();
	void setDebugToggleCallback(void (*pFunc)(int));
	void setPaletteCallback(int (*pFunc)(byte *, int));
	void setShutdownCallback(int (*pFunc)(int));
	void setPageFlipCallback(int (*pFunc)());
	void using3DHardware(int b3D);
	int addResourcePath(const char *path);
	int findResources();
	void markDirtyRect(const StdRect *pRect, int page);
	void makeCurrentSetupLib(uint setup);
	void nextSetup();
	void previousSetup();
	int confirmClose();
	void requestShutdown(int arg);
	void setPause(int bPause);
	int doPauseCallback();
	int characterCallback(int character);
	int setImuseState(int state);
	int playMovie(const char *name, int bFullscreen, int bLooping, int x, int y);
	int userPaint();
	void saveLib(const char *fileName);
	void loadLib(const char *fileName);
	int justLoaded();
	int getLibMode() const { return _libMode; }
	void setLibMode(int mode) { _libMode = mode; }
	int getDebugKeys() const { return _bDebugKeys; }
	void setDebugKeys(int bDebugKeys) { _bDebugKeys = bDebugKeys; }
	const StdColorFormat *getColorInfo() const { return &_colorInfoCopy; }

	// grim module
	Common::SeekableReadStream *openFile(const char *path) override;
	Common::SeekableReadStream *openStreamFile(const Common::String &fname);

	// Engine APIs
	bool hasFeature(EngineFeature f) const override;

	static Common::Array<Common::Keymap *> initKeymapsGrim(const char *target);

	Common::StringArray _listFiles;
	Common::StringArray::const_iterator _listFilesIter;

	void debugLua(const Common::String &str);

protected:
	void pauseEngineIntern(bool pause) override;

	Common::Mutex _imuseMutex;
	unsigned _speedLimitMs;
	bool _showFps;

	uint32 _gameFlags;
	GrimGameType _gameType;
	Common::Platform _gamePlatform;
	Common::Language _gameLanguage;

	// sputMain and grimlib modules
	int initializeModule(int module, SysStruct *pHS);
	int startupModule(int module);
	void shutdownModule(int module);
	void terminateModule(int module);
	void soundsSave(Common::WriteStream *pStream);
	void soundsRestore(Common::SeekableReadStream *pStream, bool legacyLayout = false);
	void restorePrologue();
	void restoreEpilogue();
	void restoreLegacy();

	// grim module
	bool checkGameData(const char *gameName);
	Common::Error runGrime();
	void mountPatchArchives();
	void createImuse();
	void createModules();
	void destroyModules();
	int grimStartup();
	void grimMain();
	void textOut(const char *text, int x, int y);
	void grimShutdown();
	void fatalError(const char *text, const char *caption);
	uint getHardwareBackends() const;
	int isBackendUsable(int backend);
	int hasUsableHardwareDevice();
	int chooseHardwareBackend();
	int initDisplay();
	void freeVideoDevices();
	int initDisplayAndDevice();
	int selectFallbackDisplay(int failedDevice, int bWant3D);
	int loadDisplaySettings(int *pDevice, int *pMode, int *p3DDevice);
	int chooseDefaultDisplay(int bPreferWindowed, int bWant3D, int *pDevice, int *pMode, int *p3DDevice);
	int selectDisplayMode(int deviceIndex, int modeIndex, int threeDIndex);
	int select3DDevice(int index);
	void shutdown3D();
	void shutdownDisplay();
	int getCurVideoDeviceIndex();
	int getCur3DDeviceIndex();
	int isHardware3D();
	void loadSplash();
	void pumpEvents();
	void handleFocus(int bHasFocus);
	void handleKey(Common::EventType type, const Common::KeyState &key, bool bRepeat);
	void handleKeyDebug(const Common::KeyState &key);
	void addKeyboardEvent(uint offset, int data);
	void addMouseButtonEvent(uint offset, int data);
	static int shutdownCallback(int arg);
	static int paletteCallback(byte *pPalette, int alpha);
	static int pageFlipCallback();

	StdMath *_pStdMath = nullptr;
	StdFile *_pStdFile = nullptr;
	StdConffile *_pStdConffile = nullptr;
	StdBitmap *_pStdBitmap = nullptr;
	StdLbm *_pStdLbm = nullptr;
	StdPcx *_pStdPcx = nullptr;
	StdGif *_pStdGif = nullptr;
	StdPic *_pStdPic = nullptr;
	StdBmp *_pStdBmp = nullptr;
	RenderDroid *_pRd = nullptr;
	StdControl *_pStdControl = nullptr;
	WuRegistry *_pWuRegistry = nullptr;
	Polyglot *_pPolyglot = nullptr;
	Text *_pText = nullptr;
	BlastText *_pBlastText = nullptr;
	NormalText *_pNormalText = nullptr;
	StdVBuffer *_pBackBuffer = nullptr;
	StdVBuffer *_pZBuffer = nullptr;
	StdColorFormat _colorInfoCopy = { 1, 16, 5, 6, 5, 11, 5, 0, 3, 2, 3, 0, 0, 0 };
	int _numDrawBuffers = 1;
	int _bHasFocus = 1;
	int _bFirstFrame = 0;
	int _bQuit = 0;
	int _bDisplayOpen = 0;
	int _bShowDebugOverlay = 0;
	int _bBackspaceHeld = 0;
	int _bEnginePaused = 0;
	int _saveGameToResume = 0;
	int _grimFrameCount = 0;
	int _grimLastFpsFrame = 0;
	uint32 _grimLastFpsTime = 0;
	double _grimFps = 0.0;
	int _screenshotCounter = 0;
	int _zbufShotCounter = 0;
	int _bEnumerateVideo = 0;
	int _bShow = 0;
	int _bUseHardware = 0;
	uint _hardwareBackends = 0;
	int _preferredBackend = kStdDisplayBackendNone;
	int _bRemasterFixes = 0;
	int _pendingDevice = 0;
	int _pendingMode = 0;
	int _pending3DDevice = -1;
	int _bDisplayChangePending = 0;
	Std3D *_pStd3D = nullptr;
	GrimVideoDevice _aVideoDevices[kStdDisplayMaxDevices] = {};
	StdDisplayMode _videoDeviceConfig = {};

	Res *_pRes = nullptr;
	StdDisplay *_pStdDisplay = nullptr;
	SputResource *_pSputResource = nullptr;
	SputRoom *_pSputRoom = nullptr;
	SputTime *_pSputTime = nullptr;
	SputCostume *_pSputCostume = nullptr;
	SputActorModule *_pSputActor = nullptr;
	SputControl *_pSputControl = nullptr;
	SputRender *_pSputRender = nullptr;
	SputText *_pSputText = nullptr;
	SputPrimitive *_pSputPrimitive = nullptr;
	SputSmush *_pSputSmush = nullptr;
	Lua_V1 *_pLua = nullptr;
	WalkSystem *_pWalkSystem = nullptr;
	StdVBuffer *_pMainGfxBuffer = nullptr;

	SysStruct *_pSputHS = nullptr;
	int _bSputInitialized = 0;
	int _bSputStarted = 0;
	int _loopCount = 0;
	int _bRenderPending = 0;
	int _bRunLua = 1;
	int _sputPrevMode = 0;
	PerfCounter _aPerfCounters[kNumPerfCounters] = {
		{ "Grimlib", 0.0, 0.0 }, { "Controls", 0.0, 0.0 }, { "Lua", 0.0, 0.0 }, { "Actors", 0.0, 0.0 }, { "Rects", 0.0, 0.0 },
		{ "2D", 0.0, 0.0 }, { "3D", 0.0, 0.0 }, { "Rasterize", 0.0, 0.0 }, { "Display", 0.0, 0.0 }, { "ISO", 0.0, 0.0 },
		{ "Sound", 0.0, 0.0 }, { "Shadows", 0.0, 0.0 }, { "WAITICON", 0.0, 0.0 }
	};
	Common::WriteStream *_pSaveWriteStream = nullptr;
	Common::SeekableReadStream *_pSaveReadStream = nullptr;

	int _libMode = 0;
	int _libPauseOldMode = 0;
	int _libPauseSavedMode = 0;
	int _libPauseSavedOldMode = 0;
	int _bDebugKeys = 0;
	int _bSputOpen = 0;
	int _libSaveRequest = 0;
	int _libLoadRequest = 0;
	Common::String _libSaveGameFileName;
	int _libJustLoaded = 0;
	int _libImuseState = -1;
	int (*_pShutdownCallback)(int) = nullptr;
	int _libRenderOptions[8] = { 4, 3, 1, 1, 0, 0, 2, 0 };

private:
	IMuse *_imuse = nullptr;
};

extern GrimEngine *g_grim;

// Fake KEYCODE_* values for joystick and mouse events

enum {
	KEYCODE_JOY1_A = 512,
	KEYCODE_JOY1_B,
	KEYCODE_JOY1_X,
	KEYCODE_JOY1_Y,
	KEYCODE_JOY1_BACK,
	KEYCODE_JOY1_GUIDE,
	KEYCODE_JOY1_START,
	KEYCODE_JOY1_LEFTSTICK,
	KEYCODE_JOY1_RIGHTSTICK,
	KEYCODE_JOY1_L1,
	KEYCODE_JOY1_R1,
	KEYCODE_JOY1_HUP,
	KEYCODE_JOY1_HDOWN,
	KEYCODE_JOY1_HLEFT,
	KEYCODE_JOY1_HRIGHT,
	KEYCODE_JOY1_L2,
	KEYCODE_JOY1_R2,
	KEYCODE_JOY1_B14,
	KEYCODE_JOY1_B15,
	KEYCODE_JOY1_B16,
	KEYCODE_JOY1_B17,
	KEYCODE_JOY1_B18,
	KEYCODE_JOY1_B19,
	KEYCODE_JOY1_B20,
	KEYCODE_JOY2_A,
	KEYCODE_JOY2_B,
	KEYCODE_JOY2_X,
	KEYCODE_JOY2_Y,
	KEYCODE_JOY2_BACK,
	KEYCODE_JOY2_GUIDE,
	KEYCODE_JOY2_START,
	KEYCODE_JOY2_LEFTSTICK,
	KEYCODE_JOY2_RIGHTSTICK,
	KEYCODE_JOY2_L1,
	KEYCODE_JOY2_R1,
	KEYCODE_JOY2_HUP,
	KEYCODE_JOY2_HDOWN,
	KEYCODE_JOY2_HLEFT,
	KEYCODE_JOY2_HRIGHT,
	KEYCODE_JOY2_L2,
	KEYCODE_JOY2_R2,
	KEYCODE_MOUSE_B1,
	KEYCODE_MOUSE_B2,
	KEYCODE_MOUSE_B3,
	KEYCODE_MOUSE_B4,
	KEYCODE_AXIS_JOY1_X,
	KEYCODE_AXIS_JOY1_Y,
	KEYCODE_AXIS_JOY1_Z,
	KEYCODE_AXIS_JOY1_R,
	KEYCODE_AXIS_JOY1_U,
	KEYCODE_AXIS_JOY1_V,
	KEYCODE_AXIS_JOY2_X,
	KEYCODE_AXIS_JOY2_Y,
	KEYCODE_AXIS_JOY2_Z,
	KEYCODE_AXIS_JOY2_R,
	KEYCODE_AXIS_JOY2_U,
	KEYCODE_AXIS_JOY2_V,
	KEYCODE_AXIS_MOUSE_X,
	KEYCODE_AXIS_MOUSE_Y,
	KEYCODE_AXIS_MOUSE_Z,
	KEYCODE_EXTRA_LAST
};

#define NUM_JOY_AXES (KEYCODE_AXIS_JOY1_V - KEYCODE_AXIS_JOY1_X + 1)
#define NUM_JOY_BUTTONS (KEYCODE_JOY1_R2 - KEYCODE_JOY1_A + 1)

extern const ControlDescriptor controls[];

} // end of namespace Grim

#endif
