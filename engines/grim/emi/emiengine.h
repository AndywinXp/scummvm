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

#ifndef GRIM_EMI_EMIENGINE_H
#define GRIM_EMI_EMIENGINE_H

#include "common/array.h"
#include "common/str.h"

#include "engines/grim/emi/lfile.h"
#include "engines/grim/grim.h"

namespace Graphics {
struct Surface;
}

namespace Grim {

class CDrawContext;
struct DisplayMode;
class KeyGenerator;
class LABitmap;
class Lua_V2;
class Polyglot_v2;
class TextEvent;
class ZFile;

class EmiEngine : public GrimEngine {
public:
	enum {
		kModeNone = 0,
		kModePaused = 1,
		kModeGame = 2,
		kModeMovie = 3,
		kModeOverworld = 4
	};

	enum {
		kMessageBoxTypeOk = 0,
		kMessageBoxOk = 1,
		kMessageBoxYesNoCancel = 3,
		kMessageBoxYesNo = 4,
		kMessageBoxCancel = 2,
		kMessageBoxYes = 6,
		kMessageBoxNo = 7
	};

	EmiEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language);
	~EmiEngine() override;

	int renderUpdate(CDrawContext *drawContext);
	int updateManual(CDrawContext *drawContext, bool draw, int manualMs);

	static int getMode();
	static void setMode(int newMode);
	static int getSavedMode();
	static void setSavedMode(int newMode);
	static int getCpuSpeed();
	static void setCpuSpeed(int newSpeed);
	static bool hasGammaControl();
	static void setGamma(float gamma);
	static bool setIsland(const char *island);
	static float getFrameTime();
	static bool isDumpingFrames();
	static void setSaveLoadIdle(bool idle);
	static Polyglot_v2 *getPolyglot();
	static void setPause(bool pause);
	static int getDiscNumber();
	static bool findDisc(int disc);
	static uint getTotalPhysMem();
	static int messageBox(const char *caption, const char *text, uint flags);
	static bool inputBox(const char *caption, const char *prompt, char *buffer, int bufferSize, const char *defaultText);
	static void saveScreenshot();
	static bool buildSpecScreenTextures(int index);
	static bool grabScreen();
	static bool makeThumbnailTexture(int index, bool fromScreen);
	static bool loadThumbnailTexture(int index, const char *fileName);
	static bool clearTextureSlot(int index);
	static LFile *openSaveFile(const char *fileName, const char *mode);
	static void closeSaveFile(LFile *file);
	static void beginFrameDump(const char *path);
	static void endFrameDump();
	static bool playMovie(const char *fileName, bool subtitles, bool flag6194, bool flag6195, int startFrame);
	static void setPendingMusicState(int musicState);
	static void showMessage(const char *text, const char *caption);
	static bool onCharacter(byte character);
	static bool callExitHook();
	static bool startupModules(CDrawContext *drawContext);
	static void shutdownModules();
	static bool openGameview(const char *bootParam);
	static void closeGameview();

	static Common::Array<Common::Keymap *> initKeymapsEmi(const char *target);
	static bool readSaveStrings(Common::SeekableReadStream *stream, Common::Array<Common::String> &strings);
	static Graphics::Surface *readSaveThumbnail(Common::SeekableReadStream *stream);
	static Common::String getSaveDescription(const Common::Array<Common::String> &strings, int slot);
	static bool isLegacySaveFile(const Common::String &fileName);

	const char *getUpdateFilename() override;
	Common::Error loadGameState(int slot) override;
	void syncSoundSettings() override;
	void applyGameSettings() override;

	void saveGame(CDrawContext *drawContext);
	void loadGame(CDrawContext *drawContext);

protected:
	Common::Error run() override;
	void pauseEngineIntern(bool pause) override;

private:
	enum {
		kNumStatsEvents = 5
	};

	void mountEmiPatchArchives();
	bool initialize();
	void shutdown();
	bool runFrame();
	void pumpEvents();
	void handleKeyPress(const Common::KeyState &key, bool repeat);
	void handleKeyRelease(const Common::KeyState &key);
	void handleDebugKeys(int virtualKey);
	void handleActivation(bool active);
	void pixelTest();
	float updateFrameTime(int manualMs);
	void drawStatsOverlay(CDrawContext *drawContext);
	void dumpFrame();
	void applyPendingGameSettings(int manualMs);

	static CDrawContext *createDrawContext();
	static bool openDrawContext(CDrawContext *drawContext, DisplayMode *mode);
	static void saveTga(const char *path);

	static int writeSaveData(void *data, int bytes);
	static int readSaveData(void *data, int bytes);
	static void halveBitmap(LABitmap *source, LABitmap *destination);
	static LABitmap *getScreenBitmap();
	static Common::String getSaveFileName(const char *path);
	static bool readSavedText(char *buffer, int bufferSize);

	// TODO audio and movie hooks until those modules are ported
	static void updateSounds(bool force);
	static bool updateMovie(float frameTime);
	static void startupMovies(CDrawContext *drawContext);
	static void releaseSubtitleFont();
	static void setMusicState(int musicState, int startMs);

	static bool _modulesStarted;
	static bool _gameviewOpen;
	static Lua_V2 *_lua;
	static KeyGenerator *_keyGenerator;
	static int _screenshotCount;
	static int _frameCount;
	static float _rawFrameTime;
	static double _lastStamp;
	static bool _firstFrame;
	static bool _backspaceHeld;
	static bool _controlHeld;
	static TextEvent *_statsEvents[kNumStatsEvents];
	static ZFile *_saveFile;
	static LABitmap *_screenBitmap;

	static Polyglot_v2 *_polyglot;
	static int _mode;
	static int _pausedMode;
	static int _discNumber;
	static int _frameDumpCount;
	static LString _frameDumpPath;
	static int _savedMode;
	static int _cpuSpeed;
	static LString _island;
	static float _frameTime;
	static bool _dumpingFrames;
	static bool _saveLoadIdle;
	static int _frontCopyCount;
	static int _pendingMusicState;
	static bool _gameSettingsPending;
};

} // End of namespace Grim

#endif
