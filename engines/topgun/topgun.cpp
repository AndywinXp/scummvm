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

#include "topgun/topgun.h"
#include "topgun/console.h"
#include "topgun/detection.h"
#include "topgun/display.h"
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"

#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"

#include "engines/util.h"

namespace Topgun {

TopgunEngine *g_engine;

TopgunEngine::TopgunEngine(OSystem *syst, const ADGameDescription *gameDesc, int version) : Engine(syst),
	_gameDescription(gameDesc), _version(version) {
	g_engine = this;

	bool hiRes = ConfMan.hasKey("screen_800x600") && ConfMan.getBool("screen_800x600");

	_utilsLibrary = new UtilsLibrary(this);

	// The engines from version 2 on create their own libraries
	if (_version < kVersionV1_2) {
		_audioLibrary = new AudioLibrary(this);
		_resourceLibrary = new ResourceLibrary(this);
		_spriteLibrary = new SpriteLibrary(this);
	}

	if (hiRes)
		_display = new Display(TG_DISPLAY_LARGE_WIDTH, TG_DISPLAY_LARGE_HEIGHT);
	else
		_display = new Display(TG_DISPLAY_DEFAULT_WIDTH, TG_DISPLAY_DEFAULT_HEIGHT);
}

TopgunEngine::~TopgunEngine() {
	delete _display;
	_display = nullptr;
	delete _spriteLibrary;
	_spriteLibrary = nullptr;
	delete _resourceLibrary;
	_resourceLibrary = nullptr;
	delete _audioLibrary;
	_audioLibrary = nullptr;
	delete _utilsLibrary;
	_utilsLibrary = nullptr;
	delete _screen;
	_screen = nullptr;
}

void TopgunEngine::pauseEngineIntern(bool pause) {
	Engine::pauseEngineIntern(pause);

	if (pause)
		_pauseStartMillis = g_system->getMillis();
	else
		_pausedMillis += g_system->getMillis() - _pauseStartMillis;
}

uint32 TopgunEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String TopgunEngine::getGameId() const {
	return _gameDescription->gameId;
}

void TopgunEngine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);

	SearchMan.addDirectory("folders", gamePath, 0, 2, true);
}

Common::Error TopgunEngine::run() {
	bool highBitDepth = ConfMan.hasKey("high_bit_depth") && ConfMan.getBool("high_bit_depth");

	if (highBitDepth) {
		Graphics::PixelFormat format(4, 8, 8, 8, 8, 24, 16, 8, 0);
		initGraphics(_display->width(), _display->height(), &format);

		if (g_system->getScreenFormat() != format)
			return Common::kUnsupportedColorMode;

		_screen = new Graphics::Screen();
	} else {
		initGraphics(_display->width(), _display->height());
		_screen = new Graphics::Screen();
		_screen->clearPalette();
	}

	_display->setScreen(_screen);

	setDebugger(new Console());

	int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);

	if (rtStartup(true))
		mainLoop();

	return Common::kNoError;
}

Common::Error TopgunEngine::syncGame(Common::Serializer &s) {
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

} // End of namespace Topgun
