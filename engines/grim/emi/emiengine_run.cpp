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
#include "common/config-manager.h"
#include "common/events.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "graphics/cursorman.h"
#include "graphics/renderer.h"

#include "video/qt_decoder.h"

#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/cgrimeconsole.h"
#include "engines/grim/emi/coglcontext.h"
#include "engines/grim/emi/cpathingfileservices.h"
#include "engines/grim/emi/ctinyglcontext.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/inputrecorder.h"
#include "engines/grim/emi/keygenerator.h"
#include "engines/grim/emi/labitmap.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/polyglot_v2.h"
#include "engines/grim/emi/zfile.h"

namespace Grim {

static const int kScreenWidth = 640;
static const int kScreenHeight = 480;

Common::Error EmiEngine::run() {
	if (!checkGameData("Escape from Monkey Island"))
		return Common::kUserCanceled;

	mountEmiPatchArchives();

	new CPathingFileServices();
	new LConsoleServices();

	_keyGenerator = new KeyGenerator();

	bool initialized = initialize();

	if (initialized) {
		if (ConfMan.hasKey("save_slot"))
			loadGameState(ConfMan.getInt("save_slot"));

		while (!shouldQuit()) {
			uint32 startTime = g_system->getMillis();

			if (!runFrame())
				break;

			pumpEvents();

			uint32 endTime = g_system->getMillis();

			if (endTime >= startTime && endTime - startTime < _speedLimitMs)
				g_system->delayMillis(_speedLimitMs - (endTime - startTime));
		}
	}

	shutdown();
	LConsoleServices::deleteAllConsoles();
	LFileServices::deleteAllServices();

	delete _keyGenerator;
	_keyGenerator = nullptr;

	if (!initialized)
		return Common::kUnknownError;

	return Common::kNoError;
}

CDrawContext *EmiEngine::createDrawContext() {
	uint32 availableTypes = Graphics::Renderer::getAvailableTypes();
	Graphics::RendererType desiredType = Graphics::Renderer::parseTypeCode(ConfMan.get("renderer"));

#if defined(USE_TINYGL)
	if (desiredType == Graphics::kRendererTypeTinyGL && (availableTypes & Graphics::kRendererTypeTinyGL))
		return new CTinyGLContext();
#endif

#if defined(USE_OPENGL_GAME)
	if (availableTypes & Graphics::kRendererTypeOpenGL)
		return new COGLContext();
#endif

#if defined(USE_TINYGL)
	if (availableTypes & Graphics::kRendererTypeTinyGL)
		return new CTinyGLContext();
#endif

	return nullptr;
}

bool EmiEngine::openDrawContext(CDrawContext *drawContext, DisplayMode *mode) {
	mode->bpp = 32;

	if (drawContext->open(mode))
		return true;

	mode->bpp = 16;

	return drawContext->open(mode);
}

bool EmiEngine::initialize() {
	_polyglot->configure("Install/messages.tab");

	if (!findDisc(1) && !findDisc(2)) {
		showMessage("/GOT_CD/Please make sure that the game is properly installed and that the CD is in your drive.",
				"/NO_FILES/Could not find files!");

		return false;
	}

	CDrawContext *drawContext = createDrawContext();

	if (!drawContext) {
		showMessage("/OTHER_APP/Please make sure you are not running any application that takes exclusive control of your hardware.",
				"/HARDWARE/Could not initialize your hardware!");

		return false;
	}

	CDrawContext::context = drawContext;
	drawContext->enumerateDevices();

	DisplayMode mode;

	mode.width = kScreenWidth;
	mode.height = kScreenHeight;
	mode.bpp = 32;
	mode.reserved12[0] = 2;
	mode.reserved12[1] = 16;
	mode.flags = 0;

	if (ConfMan.getBool("fullscreen"))
		mode.flags |= 1;

	bool opened = openDrawContext(drawContext, &mode);

#if defined(USE_TINYGL)
	if (!opened && dynamic_cast<CTinyGLContext *>(drawContext) == nullptr && (Graphics::Renderer::getAvailableTypes() & Graphics::kRendererTypeTinyGL)) {
		warning("EmiEngine::initialize(): the OpenGL context could not be opened, using the software renderer");

		delete drawContext;
		drawContext = new CTinyGLContext();
		CDrawContext::context = drawContext;
		drawContext->enumerateDevices();
		opened = openDrawContext(drawContext, &mode);
	}
#endif

	if (!opened) {
		showMessage("/OTHER_APP/Please make sure you are not running any application that takes exclusive control of your hardware.",
				"/HARDWARE/Could not initialize your hardware!");

		return false;
	}

	drawContext->setTextureFlag(false);
	CursorMan.showMouse(false);
	CGrimeConsole::setDebugLog(true);

	if (!startupModules(drawContext))
		return false;

	if (_showFps)
		Lua_V2::statsOverlayFlags |= Lua_V2::kStatsOverlayFlag;

	if (!CBundleServices::init(getGameLanguage() != Common::EN_ANY)) {
		showMessage("/NO_FILES/Could not find files!", "/ERROR/Error!");

		return false;
	}

	ZFile *defaultFile = ZFileServices::get()->open("default.tga", "rb");

	if (defaultFile) {
		LABitmap *defaultBitmap = new LABitmap();
		bool loaded = defaultBitmap->loadFile("default.tga");

		ZFileServices::get()->closeZFile(defaultFile);

		if (loaded)
			drawContext->defaultTexture = drawContext->createTexture(defaultBitmap);

		delete defaultBitmap;
	}

	LABitmap whiteBitmap;

	whiteBitmap.create(2, 2, LColorFormat(9), nullptr);

	uint32 *whitePixels = (uint32 *)whiteBitmap.pixels;

	if (whitePixels) {
		for (int i = 0; i < 4; i++)
			whitePixels[i] = 0xFFFFFFFF;
	}

	for (int i = 0; i < CDrawContext::kNumCachedTextures; i++)
		drawContext->setCachedTexture(i, &whiteBitmap);

	pixelTest();
	setShutdownCallback(shutdownCallback);
	_polyglot->shutdown();
	_polyglot->configure("script.tab");

	if (getGamePlatform() == Common::kPlatformMacintosh)
		playAspyrLogo(drawContext);

	return openGameview(nullptr);
}

void EmiEngine::playAspyrLogo(CDrawContext *drawContext) {
	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(Common::Path("AMWI.m4b"));

	if (!stream)
		return;

	Video::QuickTimeDecoder decoder;

	if (!decoder.loadStream(stream)) {
		warning("EmiEngine::playAspyrLogo(): cannot open AMWI.m4b");

		return;
	}

	Common::Event event;

	while (g_system->getEventManager()->pollEvent(event)) {
	}

	decoder.start();

	bool skipped = false;

	while (!shouldQuit() && !skipped && !decoder.endOfVideo()) {
		if (decoder.needsUpdate()) {
			const Graphics::Surface *frame = decoder.decodeNextFrame();

			if (frame && drawContext->beginScene()) {
				drawContext->drawMovieFrame(*frame);
				drawContext->endScene();
				drawContext->present(true);
			}
		}

		while (g_system->getEventManager()->pollEvent(event)) {
			switch (event.type) {
			case Common::EVENT_KEYDOWN:
			case Common::EVENT_LBUTTONDOWN:
			case Common::EVENT_RBUTTONDOWN:
			case Common::EVENT_MBUTTONDOWN:
			case Common::EVENT_JOYBUTTON_DOWN:
			case Common::EVENT_CUSTOM_ENGINE_ACTION_START:
				skipped = true;

				break;

			default:
				break;
			}
		}

		g_system->delayMillis(10);
	}

	decoder.stop();
}

void EmiEngine::shutdown() {
	closeGameview();
	shutdownModules();
	CGrimeConsole::setDebugLog(false);
	InputRecorder::destroyInputRecorder();

	delete CDrawContext::context;
	CDrawContext::context = nullptr;

	delete _screenBitmap;
	_screenBitmap = nullptr;
	_island.clear();

	_polyglot->shutdown();

	for (int i = 0; i < kNumStatsEvents; i++)
		_statsEvents[i] = nullptr;

	_firstFrame = true;
	_lastStamp = 0.0;
}

} // End of namespace Grim
