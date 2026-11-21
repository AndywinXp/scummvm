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

#ifndef TOPGUN_H
#define TOPGUN_H

#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/util.h"
#include "engines/engine.h"
#include "engines/savestate.h"
#include "graphics/screen.h"

#include "topgun/detection.h"

namespace Topgun {

class AudioLibrary;
class Console;
class Display;
class ResourceLibrary;
class SpriteLibrary;
class UtilsLibrary;

class TopgunEngine : public Engine {
	friend class AudioLibrary;
	friend class Console;
	friend class ResourceLibrary;
	friend class SpriteLibrary;
	friend class UtilsLibrary;

private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource{"Topgun"};
	Graphics::Screen *_screen = nullptr;
	UtilsLibrary *_utilsLibrary = nullptr;
	AudioLibrary *_audioLibrary = nullptr;
	ResourceLibrary *_resourceLibrary = nullptr;
	SpriteLibrary *_spriteLibrary = nullptr;
	Display *_display = nullptr;

protected:
	Common::Error run() override;

	// Services
	Graphics::Screen *screen() { return _screen; }
	UtilsLibrary *utilsLibrary() { return _utilsLibrary; }
	AudioLibrary *audioLibrary() { return _audioLibrary; }
	ResourceLibrary *resourceLibrary() { return _resourceLibrary; }
	SpriteLibrary *spriteLibrary() { return _spriteLibrary; }
	Display *display() { return _display; }

	// Main loop
	virtual bool rtStartup(bool runChecks) = 0;
	virtual uint16 mainLoop() = 0;
	virtual void requestClose() = 0;
	virtual void postMemoryError() = 0;
	virtual void resourceLoadProgressCallback() = 0;

	// Scripts
	virtual void spriteMessageService(byte *record) = 0;

public:
	TopgunEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~TopgunEngine() override;

	uint32 getFeatures() const;

	/**
	 * Returns the game Id
	 */
	Common::String getGameId() const;

	/**
	 * Gets a random number
	 */
	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	uint16 getRandom15Bit() {
		return _randomSource.getRandomNumber(0x7FFF);
	}

	bool hasFeature(EngineFeature f) const override {
		return
		    (f == kSupportsLoadingDuringRuntime) ||
		    (f == kSupportsSavingDuringRuntime) ||
		    (f == kSupportsReturnToLauncher);
	};

	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}

	/**
	 * Uses a serializer to allow implementing savegame
	 * loading and saving using a single method
	 */
	Common::Error syncGame(Common::Serializer &s);

	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override {
		Common::Serializer s(nullptr, stream);
		return syncGame(s);
	}
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override {
		Common::Serializer s(stream, nullptr);
		return syncGame(s);
	}
};

extern TopgunEngine *g_engine;
#define SHOULD_QUIT ::Topgun::g_engine->shouldQuit()

} // End of namespace Topgun

#endif // TOPGUN_H
