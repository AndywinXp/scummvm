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

#include "engines/grim/emi/lfile.h"
#include "engines/grim/grim.h"

namespace Grim {

class CDrawContext;
class Polyglot_v2;

class EmiEngine : public GrimEngine {
public:
	enum {
		kModeNone = 0,
		kModeMovie = 3,
		kModeOverworld = 4
	};

	enum {
		kMessageBoxYesNoCancel = 3,
		kMessageBoxYesNo = 4,
		kMessageBoxCancel = 2,
		kMessageBoxYes = 6,
		kMessageBoxNo = 7
	};

	EmiEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language);
	~EmiEngine() override;

	int renderUpdate(CDrawContext *drawContext);

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
	static void clearKeyStates();
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

private:
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
};

} // End of namespace Grim

#endif
