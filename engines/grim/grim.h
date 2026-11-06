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

#include "engines/grim/textobject.h"
#include "engines/grim/iris.h"
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
struct StdRect;
struct StdVBuffer;

class Actor;
class SaveGame;
class Bitmap;
class Font;
class Color;
class ObjectState;
class Set;
class TextObject;
class PrimitiveObject;
class LuaBase;
class Commentary;
class GfxBase;

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
	kNumPerfCounters = 13
};

struct PerfCounter {
	const char *pName;
	double start;
	double elapsed;
};

class GrimEngine : public Engine, public SysStruct {

protected:
	// Engine APIs
	Common::Error run() override;

public:
	enum EngineMode {
		PauseMode = 1,
		NormalMode = 2,
		SmushMode = 3,
		DrawMode = 4,
		OverworldMode = 5
	};
	enum SpeechMode {
		TextOnly = 1,
		VoiceOnly = 2,
		TextAndVoice = 3
	};

	GrimEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language);
	virtual ~GrimEngine();

	void clearPools();

	uint32 getGameFlags() { return _gameFlags; }
	GrimGameType getGameType() { return _gameType; }
	Common::Language getGameLanguage() { return _gameLanguage; }
	Common::Platform getGamePlatform() { return _gamePlatform; }
	virtual const char *getUpdateFilename();
	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override { return true; }
	Common::Error loadGameState(int slot) override;
	bool isRemastered() const { return !!(_gameFlags & ADGF_REMASTERED); }

	void setMode(EngineMode mode);
	EngineMode getMode() { return _mode; }
	void setPreviousMode(EngineMode mode) { _previousMode = mode; }
	EngineMode getPreviousMode() { return _previousMode; }
	void setSpeechMode(SpeechMode mode) { _speechMode = mode; }
	SpeechMode getSpeechMode() { return _speechMode; }
	SaveGame *savedState() { return _savedState; }
	bool getJustSaveLoaded() { return _justSaveLoaded; }
	void setJustSaveLoaded(bool state) { _justSaveLoaded = state; }

	void handleDebugLoadResource();
	void luaUpdate();
	void updateDisplayScene();
	void doFlip();
	void setFlipEnable(bool state) { _flipEnable = state; }
	bool getFlipEnable() { return _flipEnable; }
	virtual void drawTextObjects();
	void playIrisAnimation(Iris::Direction dir, int x, int y, int time);

	void mainLoop();
	unsigned getFrameStart() const { return _frameStart; }
	unsigned getFrameTime() const { return _frameTime; }

	// perSecond should allow rates of zero, some actors will accelerate
	// up to their normal speed (such as the bone wagon) so handling
	// a walking rate of zero should happen in the default actor creation
	float getPerSecond(float rate) const;

	int getTextSpeed() { return _textSpeed; }
	void setTextSpeed(int speed);

	void enableControl(int num) { _controlsEnabled[num] = true; }
	void disableControl(int num) { _controlsEnabled[num] = false; }
	float getControlAxis(int num);
	bool getControlState(int num);
	void clearEventQueue();

	Set *findSet(const Common::String &name);
	void setSetLock(const char *name, bool lockStatus);
	Set *loadSet(const Common::String &name);
	void setSet(const char *name);
	void setSet(Set *scene);
	Set *getCurrSet() { return _currSet; }
	void makeCurrentSetup(int num);

	void flagRefreshShadowMask(bool flag) { _refreshShadowMask = flag; }
	bool getFlagRefreshShadowMask() { return _refreshShadowMask; }

	void setSelectedActor(Actor *a) { _selectedActor = a; }
	Actor *getSelectedActor() { return _selectedActor; }

	/**
	 * Tell the engine that an actor has been moved into/outside a set,
	 * and so that it should rebuild the list of active ones.
	 */
	virtual void invalidateActiveActorsList();
	virtual void invalidateTextObjectsSortOrder() {};
	/**
	 * Return a list of the currently active actors, i. e. the actors in the current set.
	 */
	const Common::List<Actor *> &getActiveActors() const { return _activeActors; }

	/**
	 * Add an actor to the list of actors that are talking
	 */
	void addTalkingActor(Actor *actor);
	inline const Common::List<Actor *> &getTalkingActors() const { return _talkingActors; }
	bool areActorsTalking() const;
	void immediatelyRemoveActor(Actor *actor);

	void drawMovieSubtitle();
	void setMovieSubtitle(TextObject *to);
	void setMovieSetup();

	int getLanguage() const { return _language; }
	void setLanguage(int langId) { _language = langId; }
	Common::String getLanguagePrefix() const;

	bool isConceptEnabled(uint32 number) const;
	void enableConcept(uint32 number);

	bool isCutsceneEnabled(uint32 number) const;
	void enableCutscene(uint32 number);

	Commentary *getCommentary() { return _commentary; }

	Graphics::RendererType getRendererType();

	// TODO: Refactor.
	void setSaveMetaData(const char*, int, const char*);

	void saveGame(const Common::String &file);
	void loadGame(const Common::String &file);

	void changeHardwareState();

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

	// grimlib module
	void overrideSelectedActorCostume(int hInstance);
	void restoreSelectedActorCostume();
	int setDisplay(StdVBuffer *pVBuffer, StdVBuffer *pZBuffer);
	int unsetDisplay();
	void makeCurrentSet(const char *name);
	void close();
	void setDebugToggleCallback(void *pFunc);
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

	// Engine APIs
	bool hasFeature(EngineFeature f) const override;

	static Common::Array<Common::Keymap *> initKeymapsGrim(const char *target);
	static Common::Array<Common::Keymap *> initKeymapsEMI(const char *target);

	Common::StringArray _listFiles;
	Common::StringArray::const_iterator _listFilesIter;

	TextObjectDefaults _sayLineDefaults, _printLineDefaults, _blastTextDefaults;

	void debugLua(const Common::String &str);

protected:
	void pauseEngineIntern(bool pause) override;

	void handleControls(Common::EventType type, const Common::KeyState &key);
	void handleChars(Common::EventType type, const Common::KeyState &key);
	void handleJoyAxis(byte axis, int16 position);
	void handleMouseAxis(byte axis, int16 position);
	void handleJoyButton(Common::EventType type, byte button);
	void handleExit();
	void handlePause();
	void handleUserPaint();
	void cameraChangeHandle(int prev, int next);
	void cameraPostChangeHandle(int num);
	void buildActiveActorsList();
	void savegameCallback();
	GfxBase *createRenderer(int screenW, int screenH);
	void playAspyrLogo();
	virtual LuaBase *createLua();
	virtual void updateNormalMode();
	virtual void updateDrawMode();
	virtual void drawNormalMode();

	void savegameSave();
	void saveGRIM();

	void savegameRestore();
	void restoreGRIM();

	virtual void storeSaveGameMetadata(SaveGame *state);
	virtual void storeSaveGameImage(SaveGame *savedState);

	bool _savegameLoadRequest = false;
	bool _savegameSaveRequest = false;
	Common::String _savegameFileName;
	SaveGame *_savedState;
	Common::Mutex _imuseMutex;
	bool _justSaveLoaded;

	Set *_currSet;
	EngineMode _mode, _previousMode;
	SpeechMode _speechMode;
	int _textSpeed;
	bool _flipEnable;
	char _fps[8];
	bool _doFlip;
	bool _refreshShadowMask = false;
	bool _shortFrame = false;
	bool _setupChanged = true;
	// This holds the name of the setup in which the movie must be drawed
	Common::String _movieSetup;

	unsigned _frameStart = 0, _frameTime = 0, _movieTime = 0;
	int _prevSmushFrame = 0;
	unsigned int _frameCounter = 0;
	unsigned int _lastFrameTime = 0;
	unsigned _speedLimitMs;
	bool _showFps;
	bool _softRenderer;

	bool *_controlsEnabled;
	bool *_controlsState;
	float *_joyAxisPosition;

	bool _changeHardwareState = false;

	Actor *_selectedActor;
	Iris *_iris;
	TextObject::Ptr _movieSubtitle;

	bool _buildActiveActorsList;
	Common::List<Actor *> _activeActors;
	Common::List<Actor *> _talkingActors;

	uint32 _gameFlags;
	GrimGameType _gameType;
	Common::Platform _gamePlatform;
	Common::Language _gameLanguage;
	uint32 _pauseStartTime;

	// Remastered;
	uint32 _language;
	static const uint32 kNumConcepts = 98;
	static const uint32 kNumCutscenes = 40;
	bool _cutsceneEnabled[kNumCutscenes]; // TODO, could probably use a different data structure
	bool _conceptEnabled[kNumConcepts];

	Common::String _saveMeta1;
	int _saveMeta2 = 0;
	Common::String _saveMeta3;

	Commentary *_commentary;

	// sputMain and grimlib modules
	int initializeModule(int module, SysStruct *pHS);
	int startupModule(int module);
	void shutdownModule(int module);
	void terminateModule(int module);
	void soundsSave(Common::WriteStream *pStream);
	void soundsRestore(Common::SeekableReadStream *pStream);

	// grim module
	Common::Error runGrime();
	bool useGrimeFlow() const;
	void createImuse();
	void createModules();
	void destroyModules();
	int grimStartup();
	void grimMain();
	void grimShutdown();
	void fatalError(const char *text, const char *caption);
	int initDisplayAndDevice();
	int selectDisplayMode();
	void shutdownDisplay();
	void loadSplash();
	void pumpEvents();
	void handleFocus(int bActive);
	void handleKey(Common::EventType type, const Common::KeyState &key, bool bRepeat);
	void handleKeyDebug(const Common::KeyState &key);
	void addKeyboardEvent(uint offset, int data);
	void addMouseButtonEvent(uint offset, int data);
	static int shutdownCallback(int arg);
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
	int _bActive = 1;
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
	WalkSystem *_pWalkSystem = nullptr;
	IMuse *_pImuse = nullptr;
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

public:
	int _cursorX = 0;
	int _cursorY = 0;
	bool _isUtf8 = false;
	bool _transcodeChineseToSimplified = false;
	Font *_overrideFont = nullptr;
};

extern GrimEngine *g_grim;

extern int g_imuseState;

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
