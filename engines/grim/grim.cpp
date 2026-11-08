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

#define FORBIDDEN_SYMBOL_EXCEPTION_fprintf
#define FORBIDDEN_SYMBOL_EXCEPTION_fgetc
#define FORBIDDEN_SYMBOL_EXCEPTION_stderr
#define FORBIDDEN_SYMBOL_EXCEPTION_stdin
#define FORBIDDEN_SYMBOL_EXCEPTION_FILE

#include "common/archive.h"
#include "common/debug-channels.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/config-manager.h"
#include "common/compression/stuffit.h"
#include "common/translation.h"
#include "common/endian.h"
#include "common/events.h"
#include "common/savefile.h"
#include "common/serializer.h"
#include "common/str.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/textconsole.h"

#include "backends/keymapper/action.h"
#include "backends/keymapper/keymap.h"
#include "backends/keymapper/standard-actions.h"

#include "graphics/cursorman.h"
#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/pixelformat.h"
#include "graphics/surface.h"
#include "graphics/renderer.h"

#if defined(USE_OPENGL_GAME) || defined(USE_OPENGL_SHADERS)
#include "graphics/opengl/context.h"
#endif

#include "gui/error.h"
#include "gui/gui-manager.h"
#include "gui/message.h"

#include "image/png.h"

#include "engines/engine.h"
#include "engines/util.h"

#include "engines/grim/md5check.h"
#include "engines/grim/md5checkdialog.h"
#include "engines/grim/debug.h"
#include "engines/grim/grim.h"
#include "engines/grim/lua.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/emi/poolsound.h"
#include "engines/grim/emi/layer.h"
#include "engines/grim/actor.h"
#include "engines/grim/movie/movie.h"
#include "engines/grim/savegame.h"
#include "engines/grim/registry.h"
#include "engines/grim/resource.h"
#include "engines/grim/legacy_savegames/legacy_savegames.h"
#include "engines/grim/localize.h"
#include "engines/grim/gfx_base.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/font.h"
#include "engines/grim/primitives.h"
#include "engines/grim/objectstate.h"
#include "engines/grim/set.h"
#include "engines/grim/sound.h"
#include "engines/grim/debugger.h"
#include "engines/grim/remastered/overlay.h"
#include "engines/grim/remastered/lua_remastered.h"
#include "engines/grim/remastered/commentary.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/emi/sound/emisound.h"
#include "engines/grim/lua/lua.h"
#include "engines/grim/stdcontrol.h"
#include "engines/grim/blasttext.h"
#include "engines/grim/chore/cos.h"
#include "engines/grim/normaltext.h"
#include "engines/grim/patchr.h"
#include "engines/grim/polyglot.h"
#include "engines/grim/renderdroid/rd.h"
#include "engines/grim/res.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcontrol.h"
#include "engines/grim/sputcostume.h"
#include "engines/grim/sputprimitive.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/sputtext.h"
#include "engines/grim/sputtime.h"
#include "engines/grim/std/std_3d.h"
#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_bmp.h"
#include "engines/grim/std/std_conffile.h"
#include "engines/grim/std/std_display.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_gif.h"
#include "engines/grim/std/std_lbm.h"
#include "engines/grim/std/std_math.h"
#include "engines/grim/std/std_pcx.h"
#include "engines/grim/std/std_pic.h"
#include "engines/grim/text.h"
#include "engines/grim/walkplane.h"
#include "engines/grim/wuregistry.h"

namespace Grim {

GrimEngine *g_grim = nullptr;
GfxBase *g_driver = nullptr;
int g_imuseState = -1;

GrimEngine::GrimEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language) :
		Engine(syst), _currSet(nullptr), _selectedActor(nullptr), _pauseStartTime(0), _language(0) {
	g_grim = this;

	setDebugger(new Debugger());
	_gameType = gameType;
	_gameFlags = gameFlags;
	_gamePlatform = platform;
	_gameLanguage = language;

	if (getGameType() == GType_GRIM && !useGrimeFlow())
		g_registry = new Registry();
	else
		g_registry = nullptr;

	g_resourceloader = nullptr;
	g_localizer = nullptr;
	g_movie = nullptr;
	g_imuse = nullptr;

	// Set default settings
	ConfMan.registerDefault("use_arb_shaders", true);

	_showFps = ConfMan.getBool("show_fps");

	_softRenderer = true;

	_mixer->setVolumeForSoundType(Audio::Mixer::kPlainSoundType, 192);
	_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, ConfMan.getInt("sfx_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, ConfMan.getInt("speech_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, ConfMan.getInt("music_volume"));

	_currSet = nullptr;
	_selectedActor = nullptr;
	_controlsEnabled = new bool[KEYCODE_EXTRA_LAST];
	_controlsState = new bool[KEYCODE_EXTRA_LAST];
	for (int i = 0; i < KEYCODE_EXTRA_LAST; i++) {
		_controlsEnabled[i] = false;
		_controlsState[i] = false;
	}
	_joyAxisPosition = new float[NUM_JOY_AXES];
	for (int i = 0; i < NUM_JOY_AXES; i++) {
		_joyAxisPosition[i] = 0;
	}
	_speechMode = TextAndVoice;
	_textSpeed = 7;
	_mode = _previousMode = NormalMode;
	_flipEnable = true;
	int speed = ConfMan.getInt("engine_speed");
	if (speed == 0) {
		_speedLimitMs = 0;
	} else if (speed < 0 || speed > 100) {
		_speedLimitMs = 1000 / 60;
		ConfMan.setInt("engine_speed", 1000 / _speedLimitMs);
	} else {
		_speedLimitMs = 1000 / speed;
	}
	_listFilesIter = nullptr;
	_savedState = nullptr;
	_fps[0] = 0;
	_iris = new Iris();
	_buildActiveActorsList = false;
	_justSaveLoaded = false;

	Color c(0, 0, 0);

	_printLineDefaults.setX(0);
	_printLineDefaults.setY(100);
	_printLineDefaults.setWidth(0);
	_printLineDefaults.setHeight(0);
	_printLineDefaults.setFGColor(c);
	_printLineDefaults.setFont(nullptr);
	_printLineDefaults.setJustify(TextObject::LJUSTIFY);

	_sayLineDefaults.setX(0);
	_sayLineDefaults.setY(100);
	_sayLineDefaults.setWidth(0);
	_sayLineDefaults.setHeight(0);
	_sayLineDefaults.setFGColor(c);
	_sayLineDefaults.setFont(nullptr);
	_sayLineDefaults.setJustify(TextObject::CENTER);

	_blastTextDefaults.setX(0);
	_blastTextDefaults.setY(200);
	_blastTextDefaults.setWidth(0);
	_blastTextDefaults.setHeight(0);
	_blastTextDefaults.setFGColor(c);
	_blastTextDefaults.setFont(nullptr);
	_blastTextDefaults.setJustify(TextObject::LJUSTIFY);

	const Common::FSNode gameDataDir(ConfMan.getPath("path"));
	SearchMan.addSubDirectoryMatching(gameDataDir, "movies"); // Add 'movies' subdirectory for the demo
	SearchMan.addSubDirectoryMatching(gameDataDir, "credits");
	SearchMan.addSubDirectoryMatching(gameDataDir, "widescreen");


	// Remastered:
	if (isRemastered()) {
		for (uint32 i = 0; i < kNumCutscenes; i++) {
			_cutsceneEnabled[i] = false;
		}
		for (uint32 i = 0; i < kNumConcepts; i++) {
			_conceptEnabled[i] = false;
		}

		_saveMeta1 = "";
		_saveMeta2 = 0;
		_saveMeta3 = "";
	}
	_commentary = nullptr;
}

GrimEngine::~GrimEngine() {
	destroyModules();

	delete[] _controlsEnabled;
	delete[] _controlsState;
	delete[] _joyAxisPosition;

	clearPools();

	delete LuaBase::instance();
	if (g_registry) {
		g_registry->save();
		delete g_registry;
		g_registry = nullptr;
	}
	delete g_movie;
	g_movie = nullptr;
	delete g_imuse;
	g_imuse = nullptr;
	delete g_emiSound;
	g_emiSound = nullptr;
	delete g_sound;
	g_sound = nullptr;
	delete g_localizer;
	g_localizer = nullptr;
	delete g_resourceloader;
	g_resourceloader = nullptr;
	delete g_driver;
	g_driver = nullptr;
	delete _iris;

	// Remastered:
	delete _commentary;

	ConfMan.flushToDisk();

	g_grim = nullptr;
}

void GrimEngine::clearPools() {
	Actor::getPool().deleteObjects();
	Set::getPool().deleteObjects();
	PrimitiveObject::getPool().deleteObjects();
	TextObject::getPool().deleteObjects();
	Bitmap::getPool().deleteObjects();
	BitmapFont::getPool().deleteObjects();
	FontTTF::getPool().deleteObjects();
	ObjectState::getPool().deleteObjects();

	_currSet = nullptr;
}

LuaBase *GrimEngine::createLua() {
	if (isRemastered()) {
		return new Lua_Remastered();
	} else {
		return new Lua_V1();
	}
}

GfxBase *GrimEngine::createRenderer(int screenW, int screenH) {
	Common::String rendererConfig = ConfMan.get("renderer");
	Graphics::RendererType desiredRendererType = Graphics::Renderer::parseTypeCode(rendererConfig);
	uint32 availableRendererTypes = Graphics::Renderer::getAvailableTypes();

	availableRendererTypes &=
#if defined(USE_OPENGL_GAME)
			Graphics::kRendererTypeOpenGL |
#endif
#if defined(USE_OPENGL_SHADERS)
			Graphics::kRendererTypeOpenGLShaders |
#endif
#if defined(USE_TINYGL)
			Graphics::kRendererTypeTinyGL |
#endif
			0;

	// For Grim Fandango, Korean fan translation can only use OpenGL renderer
	if (getGameType() == GType_GRIM && g_grim->getGameLanguage() == Common::KO_KOR) {
		availableRendererTypes &= ~Graphics::kRendererTypeOpenGLShaders;
		availableRendererTypes &= ~Graphics::kRendererTypeTinyGL;
	}

	// For Grim Fandango, OpenGL renderer without shaders is preferred if available
	if (desiredRendererType == Graphics::kRendererTypeDefault &&
		(availableRendererTypes & Graphics::kRendererTypeOpenGL) &&
	    getGameType() == GType_GRIM) {
		availableRendererTypes &= ~Graphics::kRendererTypeOpenGLShaders;
	}

	// Not supported yet.
	if (getLanguage() == Common::Language::ZH_CHN || getLanguage() == Common::Language::ZH_TWN
		|| getGameLanguage() == Common::Language::ZH_CHN || getGameLanguage() == Common::Language::ZH_TWN)
		availableRendererTypes &= ~Graphics::kRendererTypeOpenGLShaders;

	Graphics::RendererType matchingRendererType = Graphics::Renderer::getBestMatchingType(desiredRendererType, availableRendererTypes);

	_softRenderer = matchingRendererType == Graphics::kRendererTypeTinyGL;
	if (!_softRenderer) {
		initGraphics3d(screenW, screenH);
	} else {
		initGraphics(screenW, screenH, nullptr);
	}

	GfxBase *renderer = nullptr;
#if defined(USE_OPENGL_SHADERS)
	if (matchingRendererType == Graphics::kRendererTypeOpenGLShaders) {
		renderer = CreateGfxOpenGLShader();
	}
#endif
#if defined(USE_OPENGL_GAME)
	if (matchingRendererType == Graphics::kRendererTypeOpenGL) {
		renderer = CreateGfxOpenGL();
	}
#endif
#if defined(USE_TINYGL)
	if (matchingRendererType == Graphics::kRendererTypeTinyGL) {
		renderer = CreateGfxTinyGL();
	}
#endif

	if (!renderer) {
		/* We should never end up here, getBestMatchingRendererType would have failed before */
		error("Unable to create a renderer");
	}

	renderer->setupScreen(screenW, screenH);
	renderer->loadEmergFont();
	return renderer;
}

const char *GrimEngine::getUpdateFilename() {
	if (!(getGameFlags() & ADGF_DEMO))
		return "gfupd101.exe";
	else
		return nullptr;
}

Common::Error GrimEngine::run() {
	// Try to see if we have the EMI Mac installer present
	// Currently, this requires the data fork to be standalone
	if (getGameType() == GType_MONKEY4) {
		if (SearchMan.hasFile("Monkey Island 4 Installer")) {
			Common::Archive *archive = Common::createStuffItArchive("Monkey Island 4 Installer", true);

			if (archive)
				SearchMan.add("Monkey Island 4 Installer", archive, 0, true);
		}
		if (SearchMan.hasFile("EFMI Installer")) {
			Common::Archive *archive = Common::createStuffItArchive("EFMI Installer", true);

			if (archive)
				SearchMan.add("EFMI Installer", archive, 0, true);
		}
	}

	ConfMan.registerDefault("check_gamedata", true);
	if (ConfMan.getBool("check_gamedata") && !isRemastered()) {
		MD5CheckDialog d;
		if (!d.runModal()) {
			Common::U32String confirmString = Common::U32String::format(_(
			        "ScummVM found some problems with your game data files.\n"
			        "Running ScummVM nevertheless may cause game bugs or even crashes.\n"
			        "Do you still want to run %s?"),
			        GType_MONKEY4 == getGameType() ? "Escape From Monkey Island" : "Grim Fandango"
			        );
			GUI::MessageDialog msg(confirmString, _("Yes"), _("No"));
			if (msg.runModal() != GUI::kMessageOK) {
				return Common::kUserCanceled;
			}
		}

		ConfMan.setBool("check_gamedata", false);
		ConfMan.flushToDisk();
	}

	if (useGrimeFlow())
		return runGrime();

	g_resourceloader = new ResourceLoader();
	bool demo = getGameFlags() & ADGF_DEMO;
	if (getGameType() == GType_GRIM)
		g_movie = CreateSmushPlayer(demo);
	else if (getGameType() == GType_MONKEY4) {
		if (_gamePlatform == Common::kPlatformPS2)
			g_movie = CreateMpegPlayer();
		else
			g_movie = CreateBinkPlayer(demo);
	}
	if (getGameType() == GType_GRIM) {
		createImuse();
		g_emiSound = nullptr;
		if (g_grim->isRemastered()) {
			// This must happen here, since we need the resource loader set up.
			_commentary = new Commentary();
		}
	} else if (getGameType() == GType_MONKEY4) {
		g_emiSound = new EMISound(20);
		g_imuse = nullptr;
	}
	g_sound = new SoundPlayer();

	if (getGameType() == GType_GRIM && g_grim->isRemastered()) {
		g_driver = createRenderer(1600, 900);
	} else {
		g_driver = createRenderer(640, 480);
	}

	if (getGameType() == GType_MONKEY4 && getGameLanguage() == Common::Language::ZH_TWN) {
		_transcodeChineseToSimplified = ConfMan.hasKey("language") && (Common::parseLanguage(ConfMan.get("language")) == Common::Language::ZH_CHN);

		if (_transcodeChineseToSimplified) {
			FontTTF *f = new FontTTF();
			f->loadTTFFromArchive("NotoSansSC-Regular.otf", 1200);
			_overrideFont = f;
		} else {
			Common::File img, imgmap;
			if (img.open("font.tga") && imgmap.open("map.bin")) {
				BitmapFont *f = new BitmapFont();
				f->loadTGA("font.tga", &imgmap, &img);
				_overrideFont = f;
			}
		}
	}

	if (getGameType() == GType_MONKEY4 && SearchMan.hasFile("AMWI.m4b")) {
		// Play EMI Mac Aspyr logo
		playAspyrLogo();
	}

	Bitmap *splash_bm = nullptr;
	if (!(_gameFlags & ADGF_DEMO) && getGameType() == GType_GRIM)
		splash_bm = Bitmap::create("splash.bm");
	else if ((_gameFlags & ADGF_DEMO) && getGameType() == GType_MONKEY4)
		splash_bm = Bitmap::create("splash.til");
	else if (getGamePlatform() == Common::kPlatformPS2 && getGameType() == GType_MONKEY4)
		splash_bm = Bitmap::create("load.tga");

	g_driver->clearScreen();

	if (splash_bm != nullptr)
		splash_bm->draw();

	// This flipBuffer() may make the OpenGL renderer show garbage instead of the splash,
	// while the TinyGL renderer needs it.
	if (_softRenderer)
		g_driver->flipBuffer();

	LuaBase *lua = createLua();

	lua->registerOpcodes();
	lua->registerLua();

	// One version of the demo doesn't set the demo flag in scripts.
	if (getGameType() == GType_GRIM && _gameFlags & ADGF_DEMO) {
		lua->forceDemo();
	}

	//Initialize Localizer first. In system-script are already localizeable Strings
	g_localizer = new Localizer();
	lua->loadSystemScript();
	lua->boot();

	_savegameLoadRequest = false;
	_savegameSaveRequest = false;

	// Load game from specified slot, if any
	if (ConfMan.hasKey("save_slot")) {
		loadGameState(ConfMan.getInt("save_slot"));
	}

	g_grim->setMode(NormalMode);
	delete splash_bm;
	g_grim->mainLoop();

	return Common::kNoError;
}

Common::KeymapArray GrimEngine::initKeymapsGrim(const char *target) {
	using namespace Common;

	Keymap *engineKeyMap = new Keymap(Keymap::kKeymapTypeGame, "grim", "Grim Fandango");
	Action *act;

	act = new Action(kStandardActionMoveUp, _("Up"));
	act->setKeyEvent(KEYCODE_UP);
	act->addDefaultInputMapping("JOY_UP");
	engineKeyMap->addAction(act);

	act = new Action(kStandardActionMoveDown, _("Down"));
	act->setKeyEvent(KEYCODE_DOWN);
	act->addDefaultInputMapping("JOY_DOWN");
	engineKeyMap->addAction(act);

	act = new Action(kStandardActionMoveLeft, _("Left"));
	act->setKeyEvent(KEYCODE_LEFT);
	act->addDefaultInputMapping("JOY_LEFT");
	engineKeyMap->addAction(act);

	act = new Action(kStandardActionMoveRight, _("Right"));
	act->setKeyEvent(KEYCODE_RIGHT);
	act->addDefaultInputMapping("JOY_RIGHT");
	engineKeyMap->addAction(act);

	act = new Action("BRUN", _("Run"));
	act->setKeyEvent(KeyState(KEYCODE_LSHIFT));
	act->addDefaultInputMapping("JOY_RIGHT_SHOULDER");
	engineKeyMap->addAction(act);

	act = new Action("EXAM", _("Examine"));
	act->setKeyEvent(KeyState(KEYCODE_e, 'e'));
	act->addDefaultInputMapping("JOY_X");
	engineKeyMap->addAction(act);

	act = new Action("BUSE", _("Use / Talk"));
	act->setKeyEvent(KeyState(KEYCODE_u, 'u'));
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	act = new Action("PICK", _("Pick up / Put away"));
	act->setKeyEvent(KeyState(KEYCODE_p, 'p'));
	act->addDefaultInputMapping("JOY_B");
	engineKeyMap->addAction(act);

	act = new Action("INVT", _("Inventory"));
	act->setKeyEvent(KeyState(KEYCODE_i, 'i'));
	act->addDefaultInputMapping("JOY_Y");
	engineKeyMap->addAction(act);

	act = new Action("SKLI", _("Skip dialog lines"));
	act->setKeyEvent(KeyState(KEYCODE_PERIOD, '.'));
	act->addDefaultInputMapping("PERIOD");
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	// I18N: Skipping cutscene plaback
	act = new Action(kStandardActionSkip, _("Skip"));
	act->setKeyEvent(KeyState(KEYCODE_ESCAPE, ASCII_ESCAPE));
	act->addDefaultInputMapping("ESCAPE");
	act->addDefaultInputMapping("JOY_B");
	engineKeyMap->addAction(act);

	act = new Action("RETURN", _("Confirm"));
	act->setKeyEvent(KeyState(KEYCODE_RETURN, ASCII_RETURN));
	act->addDefaultInputMapping("RETURN");
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	act = new Action("GMNU", _("Menu"));
	act->setKeyEvent(KeyState(KEYCODE_F1));
	act->addDefaultInputMapping("JOY_GUIDE");
	engineKeyMap->addAction(act);

	act = new Action("PAUSE", _("Pause"));
	act->setKeyEvent(KeyState(KEYCODE_PAUSE));
	engineKeyMap->addAction(act);

	return Keymap::arrayOf(engineKeyMap);
}

Common::KeymapArray GrimEngine::initKeymapsEMI(const char *target) {
	using namespace Common;

	Keymap *engineKeyMap = new Keymap(Keymap::kKeymapTypeGame, "monkey4", "Escape from the Monkey Island");
	Action *act;

	act = new Action(kStandardActionMoveUp, _("Up"));
	act->setKeyEvent(KEYCODE_UP);
	act->addDefaultInputMapping("JOY_UP");
	engineKeyMap->addAction(act);

	act = new Action(kStandardActionMoveDown, _("Down"));
	act->setKeyEvent(KEYCODE_DOWN);
	act->addDefaultInputMapping("JOY_DOWN");
	engineKeyMap->addAction(act);

	act = new Action(kStandardActionMoveLeft, _("Left"));
	act->setKeyEvent(KEYCODE_LEFT);
	act->addDefaultInputMapping("JOY_LEFT");
	engineKeyMap->addAction(act);

	act = new Action(kStandardActionMoveRight, _("Right"));
	act->setKeyEvent(KEYCODE_RIGHT);
	act->addDefaultInputMapping("JOY_RIGHT");
	engineKeyMap->addAction(act);

	// I18N: Cycle means rotate through
	act = new Action("COUP", _("Cycle objects up"));
	act->setKeyEvent(KeyState(KEYCODE_PAGEUP));
	act->addDefaultInputMapping("JOY_LEFT_TRIGGER");
	engineKeyMap->addAction(act);

	// I18N: Cycle means rotate through
	act = new Action("CODW", _("Cycle objects down"));
	act->setKeyEvent(KeyState(KEYCODE_PAGEDOWN));
	act->addDefaultInputMapping("JOY_RIGHT_TRIGGER");
	engineKeyMap->addAction(act);

	// I18N: Run is a movement type
	act = new Action("BRUN", _("Run"));
	act->setKeyEvent(KeyState(KEYCODE_LSHIFT));
	act->addDefaultInputMapping("JOY_RIGHT_SHOULDER");
	engineKeyMap->addAction(act);

	act = new Action("QEXT", _("Quick room exit"));
	act->setKeyEvent(KeyState(KEYCODE_o, 'o'));
	act->addDefaultInputMapping("JOY_LEFT_SHOULDER");
	engineKeyMap->addAction(act);

	act = new Action("EXAM", _("Examine / Look"));
	act->setKeyEvent(KeyState(KEYCODE_e, 'e'));
	act->addDefaultInputMapping("JOY_X");
	engineKeyMap->addAction(act);

	act = new Action("BUSE", _("Use / Talk"));
	act->setKeyEvent(KeyState(KEYCODE_u, 'u'));
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	act = new Action("PICK", _("Pick up / Put away"));
	act->setKeyEvent(KeyState(KEYCODE_KP_PLUS, '+'));
	act->addDefaultInputMapping("JOY_B");
	engineKeyMap->addAction(act);

	act = new Action("INVT", _("Inventory"));
	act->setKeyEvent(KeyState(KEYCODE_INSERT, 'i'));
	act->addDefaultInputMapping("JOY_Y");
	engineKeyMap->addAction(act);

	act = new Action("SKLI", _("Skip dialog lines"));
	act->setKeyEvent(KeyState(KEYCODE_PERIOD, '.'));
	act->addDefaultInputMapping("PERIOD");
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	// I18N: Skipping cutscene playback
	act = new Action(kStandardActionSkip, _("Skip"));
	act->setKeyEvent(KeyState(KEYCODE_ESCAPE, ASCII_ESCAPE));
	act->addDefaultInputMapping("ESCAPE");
	act->addDefaultInputMapping("JOY_B");
	engineKeyMap->addAction(act);

	act = new Action("RETURN", _("Confirm"));
	act->setKeyEvent(KeyState(KEYCODE_RETURN, ASCII_RETURN));
	act->addDefaultInputMapping("RETURN");
	act->addDefaultInputMapping("JOY_A");
	engineKeyMap->addAction(act);

	act = new Action("GMNU", _("Menu"));
	act->setKeyEvent(KeyState(KEYCODE_F1, 0));
	act->addDefaultInputMapping("JOY_GUIDE");
	engineKeyMap->addAction(act);

	return Keymap::arrayOf(engineKeyMap);
}

void GrimEngine::playAspyrLogo() {
	// A trimmed down version of the code found in mainloop
	// for the purpose of playing the Aspyr-logo.
	// The reason for this, is that the logo needs a different
	// codec than all the other videos (which are Bink).
	// Code is provided to keep within the fps-limit, as well as to
	// allow for pressing ESC to skip the movie.
	MoviePlayer *defaultPlayer = g_movie;
	g_movie = CreateQuickTimePlayer();
	g_movie->play("AMWI.m4b", false, 0, 0);
	setMode(SmushMode);
	while (g_movie->isPlaying()) {
		_doFlip = true;
		uint32 startTime = g_system->getMillis();

		updateDisplayScene();
		doFlip();
		// Process events to allow the user to skip the logo.
		Common::Event event;
		while (g_system->getEventManager()->pollEvent(event)) {
			// Ignore everything but ESC when movies are playing
			Common::EventType type = event.type;
			if (type == Common::EVENT_KEYDOWN && event.kbd.keycode == Common::KEYCODE_ESCAPE) {
				g_movie->stop();
				break;
			}
		}

		uint32 endTime = g_system->getMillis();
		if (startTime > endTime)
			continue;
		uint32 diffTime = endTime - startTime;
		if (_speedLimitMs == 0)
			continue;
		if (diffTime < _speedLimitMs) {
			uint32 delayTime = _speedLimitMs - diffTime;
			g_system->delayMillis(delayTime);
		}
	}
	delete g_movie;
	setMode(NormalMode);
	g_movie = defaultPlayer;
}

Common::Error GrimEngine::loadGameState(int slot) {
	assert(slot >= 0);
	if (useGrimeFlow()) {
		loadLib(Common::String::format("grim%02d.gsv", slot).c_str());
		return Common::kNoError;
	}
	if (getGameType() == GType_MONKEY4) {
		if (getGamePlatform() == Common::kPlatformPS2) {
			_savegameFileName = Common::String::format("efmi%03d.ps2", slot);
		} else {
			_savegameFileName = Common::String::format("efmi%03d.gsv", slot);
		}
	} else {
		_savegameFileName = Common::String::format("grim%02d.gsv", slot);
	}
	_savegameLoadRequest = true;
	return Common::kNoError;
}

void GrimEngine::handlePause() {
	if (!LuaBase::instance()->callback("pauseHandler")) {
		error("handlePause: invalid handler");
	}
}

void GrimEngine::handleExit() {
	if (!LuaBase::instance()->callback("exitHandler")) {
		error("handleExit: invalid handler");
	}
}

void GrimEngine::handleUserPaint() {
	if (!LuaBase::instance()->callback("userPaintHandler")) {
		error("handleUserPaint: invalid handler");
	}
}

void GrimEngine::cameraChangeHandle(int prev, int next) {
	LuaObjects objects;
	objects.add(prev);
	objects.add(next);
	LuaBase::instance()->callback("camChangeHandler", objects);
}

void GrimEngine::cameraPostChangeHandle(int num) {
	LuaObjects objects;
	objects.add(num);
	LuaBase::instance()->callback("postCamChangeHandler", objects);
}

void GrimEngine::savegameCallback() {
	if (!LuaBase::instance()->callback("saveGameCallback")) {
		error("GrimEngine::savegameCallback: invalid handler");
	}
}

void GrimEngine::handleDebugLoadResource() {
	void *resource = nullptr;
	int c, i = 0;
	char buf[513];

	// Tool for debugging the loading of a particular resource without
	// having to actually make it all the way to it in the game
	fprintf(stderr, "Enter resource to load (extension specifies type): ");
	while (i < 512 && (c = fgetc(stdin)) != EOF && c != '\n')
		buf[i++] = c;

	buf[i] = '\0';
	if (strstr(buf, ".key"))
		resource = (void *)g_resourceloader->loadKeyframe(buf);
	else if (strstr(buf, ".zbm") || strstr(buf, ".bm"))
		resource = (void *)Bitmap::create(buf);
	else if (strstr(buf, ".cmp"))
		resource = (void *)g_resourceloader->loadColormap(buf);
	else if (strstr(buf, ".cos"))
		resource = (void *)g_resourceloader->loadCostume(buf, nullptr, nullptr);
	else if (strstr(buf, ".lip"))
		resource = (void *)g_resourceloader->loadLipSync(buf);
	else if (strstr(buf, ".snm"))
		resource = (void *)g_movie->play(buf, false, 0, 0);
	else if (strstr(buf, ".wav") || strstr(buf, ".imu")) {
		if (g_imuse)
			g_imuse->soundsLoadStartSound(buf, 127, IMUSE_GROUP_SFX);
		resource = (void *)1;
	} else if (strstr(buf, ".mat")) {
		CMap *cmap = g_resourceloader->loadColormap("item.cmp");
		warning("Default colormap applied to resources loaded in this fashion");
		// Default to repeating the texture as in GRIM
		resource = (void *)g_resourceloader->loadMaterial(buf, cmap, false);
	} else {
		warning("Resource type not understood");
	}
	if (!resource)
		warning("Requested resource (%s) not found", buf);
}

void GrimEngine::drawTextObjects() {
	for (TextObject *t : TextObject::getPool()) {
		t->draw();
	}
}

void GrimEngine::playIrisAnimation(Iris::Direction dir, int x, int y, int time) {
	_iris->play(dir, x, y, time);
}

void GrimEngine::luaUpdate() {
	if (_savegameLoadRequest || _savegameSaveRequest || _changeHardwareState)
		return;

	// Update timing information
	unsigned newStart = g_system->getMillis();
	if (newStart < _frameStart) {
		_frameStart = newStart;
		return;
	}
	_frameTime = newStart - _frameStart;
	_frameStart = newStart;

	if (_mode == PauseMode || _shortFrame) {
		_frameTime = 0;
	}

	LuaBase::instance()->update(_frameTime, _movieTime);

	if (_currSet && (_mode == NormalMode || _mode == SmushMode)) {
		// call updateTalk() before calling update(), since it may modify costumes state, and
		// the costumes are updated in update().
		for (Common::List<Actor *>::iterator i = _talkingActors.begin(); i != _talkingActors.end(); ++i) {
			Actor *a = *i;
			if (!a->updateTalk()) {
				i = _talkingActors.reverse_erase(i);
			}
		}

		// Update the actors. Do it here so that we are sure to react asap to any change
		// in the actors state caused by lua.
		buildActiveActorsList();
		for (Actor *a : _activeActors) {
			// Note that the actor need not be visible to update chores, for example:
			// when Manny has just brought Meche back he is offscreen several times
			// when he needs to perform certain chores
			a->update(_frameTime);
		}

		_iris->update(_frameTime);

		for (TextObject *t : TextObject::getPool()) {
			t->update();
		}
	}
}

void GrimEngine::updateDisplayScene() {
	_doFlip = true;

	if (_mode == SmushMode) {
		if (g_movie->isPlaying()) {
			_movieTime = g_movie->getMovieTime();
			if (g_movie->isUpdateNeeded()) {
				g_driver->prepareMovieFrame(g_movie->getDstSurface(), g_movie->getDstPalette());
				g_movie->clearUpdateNeeded();
			}
			int frame = g_movie->getFrame();
			if (frame >= 0) {
				if (frame != _prevSmushFrame) {
					_prevSmushFrame = g_movie->getFrame();
					g_driver->drawMovieFrame(g_movie->getX(), g_movie->getY());
					if (_showFps)
						g_driver->drawEmergString(550, 25, _fps, Color(255, 255, 255));
				} else
					_doFlip = false;
			} else
				g_driver->releaseMovieFrame();
		}
		// Draw Primitives
		_iris->draw();

		g_movie->drawMovieSubtitle();

	} else if (_mode == NormalMode || _mode == OverworldMode) {
		updateNormalMode();
	} else if (_mode == DrawMode) {
		updateDrawMode();
	}
}

void GrimEngine::updateNormalMode() {
	if (!_currSet || !_flipEnable)
		return;

	g_driver->clearScreen();

	drawNormalMode();

	_iris->draw();
	drawTextObjects();
}

void GrimEngine::updateDrawMode() {
	_doFlip = false;
	_prevSmushFrame = 0;
	_movieTime = 0;
}

void GrimEngine::drawNormalMode() {
	_prevSmushFrame = 0;
	_movieTime = 0;

	_currSet->drawBackground();

	// Draw underlying scene components
	// Background objects are drawn underneath everything except the background
	// There are a bunch of these, especially in the tube-switcher room
	_currSet->drawBitmaps(ObjectState::OBJSTATE_BACKGROUND);

	// State objects are drawn on top of other things, such as the flag
	// on Manny's message tube
	_currSet->drawBitmaps(ObjectState::OBJSTATE_STATE);

	// Play SMUSH Animations
	// This should occur on top of all underlying scene objects,
	// a good example is the tube switcher room where some state objects
	// need to render underneath the animation or you can't see what's going on
	// This should not occur on top of everything though or Manny gets covered
	// up when he's next to Glottis's service room
	if (g_movie->isPlaying() && _movieSetup == _currSet->getCurrSetup()->_name) {
		_movieTime = g_movie->getMovieTime();
		if (g_movie->isUpdateNeeded()) {
			g_driver->prepareMovieFrame(g_movie->getDstSurface(), g_movie->getDstPalette());
			g_movie->clearUpdateNeeded();
		}
		if (g_movie->getFrame() >= 0)
			g_driver->drawMovieFrame(g_movie->getX(), g_movie->getY());
		else
			g_driver->releaseMovieFrame();
	}

	// Underlay objects must be drawn on top of movies
	// Otherwise the lighthouse door will always be open as the underlay for
	// the closed door will be overdrawn by a movie used as background image.
	_currSet->drawBitmaps(ObjectState::OBJSTATE_UNDERLAY);

	// Draw Primitives
	for (PrimitiveObject *p : PrimitiveObject::getPool()) {
		p->draw();
	}

	for (Overlay *p : Overlay::getPool()) {
		p->draw();
	}

	_currSet->setupCamera();

	g_driver->set3DMode();

	if (_setupChanged) {
		cameraPostChangeHandle(_currSet->getSetup());
		_setupChanged = false;
	}

	// Draw actors
	buildActiveActorsList();
	for (Actor *a : _activeActors) {
		if (a->isVisible())
			a->draw();
	}

	flagRefreshShadowMask(false);

	// Draw overlying scene components
	// The overlay objects should be drawn on top of everything else,
	// including 3D objects such as Manny and the message tube
	_currSet->drawBitmaps(ObjectState::OBJSTATE_OVERLAY);
}

void GrimEngine::doFlip() {
	_frameCounter++;
	// When possible, flip the buffer
	// This makes sure the screen is refreshed on a regular basis
	// The image is properly resized if needed and backend overlays are displayed
	if (!_doFlip || (_mode == PauseMode)) {
		g_driver->flipBuffer(true);
		return;
	}

	if (_showFps && _mode != DrawMode)
		g_driver->drawEmergString(550, 25, _fps, Color(255, 255, 255));

	if (_flipEnable)
		g_driver->flipBuffer();

	if (_showFps && _mode != DrawMode) {
		unsigned int currentTime = g_system->getMillis();
		unsigned int delta = currentTime - _lastFrameTime;
		if (delta > 500) {
			snprintf(_fps, sizeof(_fps), "%7.2f", (double)(_frameCounter * 1000) / (double)delta);
			_frameCounter = 0;
			_lastFrameTime = currentTime;
		}
	}
}

void GrimEngine::mainLoop() {
	_movieTime = 0;
	_frameTime = 0;
	_frameStart = g_system->getMillis();
	_frameCounter = 0;
	_lastFrameTime = 0;
	_prevSmushFrame = 0;
	_refreshShadowMask = false;
	_shortFrame = false;
	bool resetShortFrame = false;
	_changeHardwareState = false;
	_setupChanged = true;

	for (;;) {
		uint32 startTime = g_system->getMillis();
		if (_shortFrame) {
			if (resetShortFrame) {
				_shortFrame = false;
			}
			resetShortFrame = !resetShortFrame;
		}

		if (shouldQuit())
			return;

		if (_savegameLoadRequest) {
			savegameRestore();
		}
		if (_savegameSaveRequest) {
			savegameSave();
		}

		// If the backend destroys the OpenGL context or the user switched to a different
		// renderer, the GFX driver needs to be recreated.
		if (_changeHardwareState) {
			_changeHardwareState = false;

			uint screenWidth = g_driver->getScreenWidth();
			uint screenHeight = g_driver->getScreenHeight();

			EngineMode mode = getMode();

			_savegameFileName = "";
			savegameSave();
			clearPools();

			delete g_driver;
			g_driver = createRenderer(screenWidth, screenHeight);
			savegameRestore();

			if (mode == DrawMode) {
				setMode(GrimEngine::NormalMode);
				updateDisplayScene();
				g_driver->storeDisplay();
				g_driver->dimScreen();
			}
			setMode(mode);
		}

		g_sound->flushTracks();

		// Process events
		Common::Event event;
		while (g_system->getEventManager()->pollEvent(event)) {
			// Handle any buttons, keys and joystick operations
			Common::EventType type = event.type;
			if (type == Common::EVENT_KEYDOWN || type == Common::EVENT_KEYUP) {
				if (type == Common::EVENT_KEYDOWN) {
					// Ignore everything but ESC when movies are playing
					// This matches the retail and demo versions of EMI
					// This also allows the PS2 version to skip movies
					if (_mode == SmushMode && g_grim->getGameType() == GType_MONKEY4) {
						if (event.kbd.keycode == Common::KEYCODE_ESCAPE) {
							g_movie->stop();
							break;
						}
						continue;
					}

					if (_mode != DrawMode && _mode != SmushMode && ((event.kbd.ascii == 'q') || (event.kbd.ascii == 'x' && (event.kbd.flags & Common::KBD_ALT)))) {
						handleExit();
						break;
					} else if (_mode != DrawMode && (event.kbd.keycode == Common::KEYCODE_PAUSE)) {
						handlePause();
						break;
					} else {
						handleChars(type, event.kbd);
					}
				}

				handleControls(type, event.kbd);

				if (getGameType() != GType_MONKEY4) {
					// Allow lua to react to the event.
					// Without this lua_update switching the entries in the menu is slow because
					// if the button is not kept pressed the KEYUP will arrive just after the KEYDOWN
					// and it will break the lua scripts that checks for the state of the button
					// with GetControlState()
					//
					// This call seems to be only necessary to handle Grim's menu correctly.
					// In EMI it would have the side-effect that luaUpdate() is sometimes called
					// in the same millisecond which causes getPerSecond() to return 0 for
					// any given rate which is not compatible with e.g. actor walking.

					// We do not want the scripts to update while a movie is playing in the PS2-version.
					if (!(getGamePlatform() == Common::kPlatformPS2 && _mode == SmushMode)) {
						luaUpdate();
					}
				}
			}
			if (type == Common::EVENT_JOYAXIS_MOTION)
				handleJoyAxis(event.joystick.axis, event.joystick.position);
			if (type == Common::EVENT_JOYBUTTON_DOWN || type == Common::EVENT_JOYBUTTON_UP)
				handleJoyButton(type, event.joystick.button);

			if (type == Common::EVENT_LBUTTONUP) {
				_cursorX = event.mouse.x;
				_cursorY = event.mouse.y;
				Common::KeyState k;
				k.keycode = (Common::KeyCode)KEYCODE_MOUSE_B1;
				handleControls(Common::EVENT_KEYUP, k);
			}
			if (type == Common::EVENT_LBUTTONDOWN) {
				_cursorX = event.mouse.x;
				_cursorY = event.mouse.y;
				Common::KeyState k;
				k.keycode = (Common::KeyCode)KEYCODE_MOUSE_B1;
				handleControls(Common::EVENT_KEYDOWN, k);
			}
			if (type == Common::EVENT_MOUSEMOVE) {
				_cursorX = event.mouse.x;
				_cursorY = event.mouse.y;
				handleMouseAxis(0, _cursorX);
				handleMouseAxis(1, _cursorY);
			}
			if (type == Common::EVENT_SCREEN_CHANGED) {
				handleUserPaint();
			}
		}

		if (_mode != PauseMode) {
			// Draw the display scene before doing the luaUpdate.
			// This give a large performance boost as OpenGL stores commands
			// in a queue on the gpu to be rendered later. When doFlip is
			// called the cpu must wait for the gpu to finish its queue.
			// Now, it will queue all the OpenGL commands and draw them on the
			// GPU while the CPU is busy updating the game world.
			updateDisplayScene();
		}

		doFlip();

		// We do not want the scripts to update while a movie is playing in the PS2-version.
		if (!(getGamePlatform() == Common::kPlatformPS2 && _mode == SmushMode)) {
			luaUpdate();
		}

		if (g_imuseState != -1) {
			g_sound->setMusicState(g_imuseState);
			g_imuseState = -1;
		}

		uint32 endTime = g_system->getMillis();
		if (startTime > endTime)
			continue;
		uint32 diffTime = endTime - startTime;
		if (diffTime < _speedLimitMs) {
			uint32 delayTime = _speedLimitMs - diffTime;
			g_system->delayMillis(delayTime);
		}
	}
}

void GrimEngine::changeHardwareState() {
	_changeHardwareState = true;
}

void GrimEngine::saveGame(const Common::String &file) {
	_savegameFileName = file;
	_savegameSaveRequest = true;
}

void GrimEngine::loadGame(const Common::String &file) {
	_savegameFileName = file;
	_savegameLoadRequest = true;
}

void GrimEngine::savegameRestore() {
	debug(2, "GrimEngine::savegameRestore() started.");
	_savegameLoadRequest = false;
	Common::String filename;
	if (_savegameFileName.size() == 0) {
		filename = "grim.sav";
	} else {
		filename = _savegameFileName;
	}
	_savedState = SaveGame::openForLoading(filename);
	if (!_savedState || !_savedState->isCompatible())
		return;
	if (g_imuse)
		g_imuse->imStopAllSounds();
	g_movie->stop();
	if (g_imuse)
		g_imuse->pause(true);
	g_movie->pause(true);
	if (g_registry)
		g_registry->save();

	_selectedActor = nullptr;
	delete _currSet;
	_currSet = nullptr;

	Bitmap::getPool().restoreObjects(_savedState);
	Debug::debug(Debug::Engine, "Bitmaps restored successfully.");

	BitmapFont::getPool().restoreObjects(_savedState);
	if (_savedState->saveMinorVersion() >= 28) {
		FontTTF::getPool().restoreObjects(_savedState);
	}
	Debug::debug(Debug::Engine, "Fonts restored successfully.");

	ObjectState::getPool().restoreObjects(_savedState);
	Debug::debug(Debug::Engine, "ObjectStates restored successfully.");

	Set::getPool().restoreObjects(_savedState);
	Debug::debug(Debug::Engine, "Sets restored successfully.");

	if (g_imuse)
		g_imuse->soundsFloodMusicBuffer();

	TextObject::getPool().restoreObjects(_savedState);
	Debug::debug(Debug::Engine, "TextObjects restored successfully.");

	PrimitiveObject::getPool().restoreObjects(_savedState);
	Debug::debug(Debug::Engine, "PrimitiveObjects restored successfully.");

	Actor::getPool().restoreObjects(_savedState);
	Debug::debug(Debug::Engine, "Actors restored successfully.");

	if (getGameType() == GType_MONKEY4) {
		PoolSound::getPool().restoreObjects(_savedState);
		Debug::debug(Debug::Engine, "Pool sounds saved successfully.");

		Layer::getPool().restoreObjects(_savedState);
		Debug::debug(Debug::Engine, "Layers restored successfully.");
	}

	restoreGRIM();
	Debug::debug(Debug::Engine, "Engine restored successfully.");

	g_driver->restoreState(_savedState);
	Debug::debug(Debug::Engine, "Renderer restored successfully.");

	g_sound->restoreState(_savedState);
	Debug::debug(Debug::Engine, "iMuse restored successfully.");

	g_movie->restoreState(_savedState);
	Debug::debug(Debug::Engine, "Movie restored successfully.");

	_iris->restoreState(_savedState);
	Debug::debug(Debug::Engine, "Iris restored successfully.");

	lua_Restore(_savedState);
	Debug::debug(Debug::Engine, "Lua restored successfully.");

	if (getGameType() == GType_GRIM && !(getGameFlags() & ADGF_DEMO) &&
		_savedState->saveMajorVersion() == 22 &&
		_savedState->saveMinorVersion() >= 7 &&
		_savedState->saveMinorVersion() <= 28) {
		// Since ResidualVM 0.2.0, a ResidualVM/ScummVM specific patch was provided broken.
		// We patch here the code to fix all saves containing this invalid code.
		// cf. bug #13139 and #14987
		lua_PatchGrimSave();
	}

	delete _savedState;

	_justSaveLoaded = true;

	//Re-read the values, since we may have been in some state that changed them when loading the savegame,
	//e.g. running a cutscene, which sets the sfx volume to 0.
	_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, ConfMan.getInt("sfx_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, ConfMan.getInt("speech_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, ConfMan.getInt("music_volume"));

	LuaBase::instance()->postRestoreHandle();
	if (g_imuse)
		g_imuse->pause(false);
	g_movie->pause(false);

	debug(2, "GrimEngine::savegameRestore() finished.");

	_shortFrame = true;
	clearEventQueue();
	invalidateActiveActorsList();
	buildActiveActorsList();

	_currSet->setupCamera();
	g_driver->set3DMode();
}

void GrimEngine::restoreGRIM() {
	_savedState->beginSection('GRIM');

	_mode = (EngineMode)_savedState->readLEUint32();
	_previousMode = (EngineMode)_savedState->readLEUint32();

	// Actor stuff
	int32 id = _savedState->readLESint32();
	if (id != 0) {
		_selectedActor = Actor::getPool().getObject(id);
	}

	//TextObject stuff
	_sayLineDefaults.setFGColor(_savedState->readColor());
	_sayLineDefaults.setFont(Font::load(_savedState));
	_sayLineDefaults.setHeight(_savedState->readLESint32());
	_sayLineDefaults.setJustify(_savedState->readLESint32());
	_sayLineDefaults.setWidth(_savedState->readLESint32());
	_sayLineDefaults.setX(_savedState->readLESint32());
	_sayLineDefaults.setY(_savedState->readLESint32());
	_sayLineDefaults.setDuration(_savedState->readLESint32());
	if (_savedState->saveMinorVersion() > 5) {
		_movieSubtitle = TextObject::getPool().getObject(_savedState->readLESint32());
	}

	// Set stuff
	_currSet = Set::getPool().getObject(_savedState->readLESint32());
	if (_savedState->saveMinorVersion() > 4) {
		_movieSetup = _savedState->readString();
	} else {
		_movieSetup = _currSet->getCurrSetup()->_name;
	}

	_savedState->endSection();
}

void GrimEngine::storeSaveGameMetadata(SaveGame *state) {
	if (!g_grim->isRemastered()) {
		return;
	}
	state->beginSection('META');
	state->writeString(_saveMeta1);
	state->writeLEUint32(_saveMeta2);
	state->writeString(_saveMeta3);
	state->endSection();
}

void GrimEngine::storeSaveGameImage(SaveGame *state) {
	const Graphics::PixelFormat image_format = Graphics::PixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
	int width = 250, height = 188;
	Bitmap *screenshot;

	debug(2, "GrimEngine::StoreSaveGameImage() started.");

	screenshot = g_driver->getScreenshot(width, height, true);
	state->beginSection('SIMG');
	if (screenshot) {
		int size = screenshot->getWidth() * screenshot->getHeight();
		screenshot->setActiveImage(0);
		screenshot->getBitmapData()->convertToColorFormat(image_format);
		const uint16 *data = (const uint16 *)screenshot->getData().getPixels();
		for (int l = 0; l < size; l++) {
			state->writeLEUint16(data[l]);
		}
	} else {
		error("Unable to store screenshot");
	}
	state->endSection();
	delete screenshot;
	debug(2, "GrimEngine::StoreSaveGameImage() finished.");
}

void GrimEngine::savegameSave() {
	debug(2, "GrimEngine::savegameSave() started.");
	_savegameSaveRequest = false;
	Common::String filename;
	if (_savegameFileName.size() == 0) {
		filename = "grim.sav";
	} else {
		filename = _savegameFileName;
	}
	if (getGameType() == GType_MONKEY4 && filename.contains('/')) {
		filename = Common::lastPathComponent(filename, '/');
	}
	_savedState = SaveGame::openForSaving(filename);
	if (!_savedState) {
		//TODO: Translate this!
		GUI::displayErrorDialog(_("Error: the game could not be saved."));
		return;
	}
	storeSaveGameMetadata(_savedState);

	storeSaveGameImage(_savedState);

	if (g_imuse)
		g_imuse->pause(true);
	g_movie->pause(true);

	savegameCallback();

	Bitmap::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "Bitmaps saved successfully.");

	BitmapFont::getPool().saveObjects(_savedState);
	FontTTF::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "Fonts saved successfully.");

	ObjectState::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "ObjectStates saved successfully.");

	Set::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "Sets saved successfully.");

	TextObject::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "TextObjects saved successfully.");

	PrimitiveObject::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "PrimitiveObjects saved successfully.");

	Actor::getPool().saveObjects(_savedState);
	Debug::debug(Debug::Engine, "Actors saved successfully.");

	if (getGameType() == GType_MONKEY4) {
		PoolSound::getPool().saveObjects(_savedState);
		Debug::debug(Debug::Engine, "Pool sounds saved successfully.");

		Layer::getPool().saveObjects(_savedState);
		Debug::debug(Debug::Engine, "Layers saved successfully.");
	}

	saveGRIM();
	Debug::debug(Debug::Engine, "Engine saved successfully.");

	g_driver->saveState(_savedState);
	Debug::debug(Debug::Engine, "Renderer saved successfully.");

	g_sound->saveState(_savedState);
	Debug::debug(Debug::Engine, "iMuse saved successfully.");

	g_movie->saveState(_savedState);
	Debug::debug(Debug::Engine, "Movie saved successfully.");

	_iris->saveState(_savedState);
	Debug::debug(Debug::Engine, "Iris saved successfully.");

	lua_Save(_savedState);

	delete _savedState;

	if (g_imuse)
		g_imuse->pause(false);
	g_movie->pause(false);
	debug(2, "GrimEngine::savegameSave() finished.");

	_shortFrame = true;
	clearEventQueue();
}

void GrimEngine::saveGRIM() {
	_savedState->beginSection('GRIM');

	_savedState->writeLEUint32((uint32)_mode);
	_savedState->writeLEUint32((uint32)_previousMode);

	//Actor stuff
	if (_selectedActor) {
		_savedState->writeLESint32(_selectedActor->getId());
	} else {
		_savedState->writeLESint32(0);
	}

	//TextObject stuff
	_savedState->writeColor(_sayLineDefaults.getFGColor());
	Font::save(_sayLineDefaults.getFont(), _savedState);
	_savedState->writeLESint32(_sayLineDefaults.getHeight());
	_savedState->writeLESint32(_sayLineDefaults.getJustify());
	_savedState->writeLESint32(_sayLineDefaults.getWidth());
	_savedState->writeLESint32(_sayLineDefaults.getX());
	_savedState->writeLESint32(_sayLineDefaults.getY());
	_savedState->writeLESint32(_sayLineDefaults.getDuration());
	_savedState->writeLESint32(_movieSubtitle ? _movieSubtitle->getId() : 0);

	//Set stuff
	_savedState->writeLESint32(_currSet->getId());
	_savedState->writeString(_movieSetup);

	_savedState->endSection();
}

Set *GrimEngine::findSet(const Common::String &name) {
	// Find scene object
	for (Set *s : Set::getPool()) {
		if (s->getName() == name)
			return s;
	}
	return nullptr;
}

void GrimEngine::setSetLock(const char *name, bool lockStatus) {
	Set *scene = findSet(name);

	if (!scene) {
		Debug::warning(Debug::Engine, "Set object '%s' not found in list", name);
		return;
	}
	// Change the locking status
	scene->_locked = lockStatus;
}

Set *GrimEngine::loadSet(const Common::String &name) {
	Set *s = findSet(name);

	if (!s) {
		Common::String filename(name);
		// EMI-scripts refer to their .setb files as .set
		if (g_grim->getGameType() == GType_MONKEY4) {
			filename += "b";
		}
		Common::SeekableReadStream *stream;
		stream = g_resourceloader->openNewStreamFile(filename.c_str());
		if (!stream)
			error("Could not find scene file %s", name.c_str());

		s = new Set(name, stream);
		delete stream;
	}

	return s;
}

void GrimEngine::setSet(const char *name) {
	setSet(loadSet(name));
}

void GrimEngine::setSet(Set *scene) {
	if (scene == _currSet)
		return;

	if (g_imuse)
		g_imuse->soundsFloodMusicBuffer();

	if (getGameType() == GType_MONKEY4) {
		for (PoolSound *s : PoolSound::getPool()) {
			s->stop();
		}
	}
	// Stop the actors. This fixes bug #289 (https://github.com/residualvm/residualvm/issues/289)
	// and it makes sense too, since when changing set the directions
	// and coords change too.
	for (Actor *a : Actor::getPool()) {
		a->stopWalking();
	}

	Set *lastSet = _currSet;
	_currSet = scene;
	_currSet->setSoundParameters();
	// should delete the old scene after setting the new one
	if (lastSet && !lastSet->_locked) {
		delete lastSet;
	}
	_shortFrame = true;
	_setupChanged = true;
	invalidateActiveActorsList();
}

void GrimEngine::makeCurrentSetup(int num) {
	int prevSetup = g_grim->getCurrSet()->getSetup();
	if (prevSetup != num) {
		getCurrSet()->setSetup(num);
		getCurrSet()->setSoundParameters();
		cameraChangeHandle(prevSetup, num);

		if (getGameType() == GType_GRIM)
			getCurrSet()->relocateSounds();

		_setupChanged = true;
	}
}

void GrimEngine::setTextSpeed(int speed) {
	if (speed < 1)
		_textSpeed = 1;
	if (speed > 10)
		_textSpeed = 10;
	_textSpeed = speed;
}

float GrimEngine::getControlAxis(int num) {
	int idx = num - KEYCODE_AXIS_JOY1_X;
	if (idx >= 0 && idx < NUM_JOY_AXES) {
		return _joyAxisPosition[idx];
	}
	return 0;
}

bool GrimEngine::getControlState(int num) {
	return _controlsState[num];
}

float GrimEngine::getPerSecond(float rate) const {
	return rate * _frameTime / 1000;
}

void GrimEngine::invalidateActiveActorsList() {
	_buildActiveActorsList = true;
}

void GrimEngine::immediatelyRemoveActor(Actor *actor) {
	_activeActors.remove(actor);
	_talkingActors.remove(actor);
}

void GrimEngine::buildActiveActorsList() {
	if (!_buildActiveActorsList) {
		return;
	}

	_activeActors.clear();
	for (Actor *a : Actor::getPool()) {
		if (((_mode == NormalMode || _mode == DrawMode) && a->isDrawableInSet(_currSet->getName())) || a->isInOverworld()) {
			_activeActors.push_back(a);
		}
	}
	_buildActiveActorsList = false;
}

void GrimEngine::addTalkingActor(Actor *a) {
	_talkingActors.push_back(a);
}

bool GrimEngine::areActorsTalking() const {
	//This takes into account that there may be actors which are still talking, but in the background.
	bool talking = false;
	for (Actor *a : _talkingActors) {
		if (a->isTalkingForeground()) {
			talking = true;
			break;
		}
	}
	return talking;
}

void GrimEngine::setMovieSubtitle(TextObject *to) {
	if (_movieSubtitle != to) {
		delete _movieSubtitle;
		_movieSubtitle = to;
	}
}

void GrimEngine::drawMovieSubtitle() {
	if (_movieSubtitle)
		_movieSubtitle->draw();
}

void GrimEngine::setMovieSetup() {
	_movieSetup = _currSet->getCurrSetup()->_name;
}

void GrimEngine::setMode(EngineMode mode) {
	_mode = mode;
	invalidateActiveActorsList();
}

void GrimEngine::clearEventQueue() {
	g_system->getEventManager()->purgeKeyboardEvents();
	g_system->getEventManager()->purgeMouseEvents();

	for (int i = 0; i < KEYCODE_EXTRA_LAST; ++i) {
		_controlsState[i] = false;
	}
}

bool GrimEngine::hasFeature(EngineFeature f) const {
	return
		(f == kSupportsReturnToLauncher) ||
		(f == kSupportsLoadingDuringRuntime);
}

void GrimEngine::pauseEngineIntern(bool pause) {
	if (useGrimeFlow()) {
		if (pause) {
			_bEnginePaused = _libMode != 1;
			setPause(1);
		} else if (_bEnginePaused) {
			_bEnginePaused = 0;
			setPause(0);

			if (_pStdControl)
				_pStdControl->flush();
		}

		return;
	}

	if (g_imuse)
		g_imuse->pause(pause);
	if (g_movie)
		g_movie->pause(pause);

	if (pause) {
		_pauseStartTime = _system->getMillis();
	} else {
		_frameStart += _system->getMillis() - _pauseStartTime;

		// This "clear event queue" call is added to clear any keys registered as pressed
		// when the pause occured and their KEYUP event was not handled by the engine
		// (because it was paused) but it was consumed externally and is no longer pending in the event queue.
		// The call also clears any pending (at the time of the pause) keyboard or mouse events.
		// It addresses bug #16667.
		clearEventQueue();
	}
}

void GrimEngine::debugLua(const Common::String &str) {
	lua_dostring(str.c_str());
}

Common::String GrimEngine::getLanguagePrefix() const {
	switch (getLanguage()) {
		case 0:
			return Common::String("en");
		case 1:
			return Common::String("de");
		case 2:
			return Common::String("es");
		case 3:
			return Common::String("fr");
		case 4:
			return Common::String("it");
		case 5:
			return Common::String("pt");
		default:
			error("Unknown language id %d", getLanguage());
	}
}

bool GrimEngine::isConceptEnabled(uint32 number) const {
	assert (number < kNumConcepts);
	return _conceptEnabled[number];
}

void GrimEngine::enableConcept(uint32 number) {
	assert (number < kNumConcepts);
	_conceptEnabled[number] = true;
}

bool GrimEngine::isCutsceneEnabled(uint32 number) const {
	assert (number < kNumCutscenes);
	return _cutsceneEnabled[number];
}

void GrimEngine::enableCutscene(uint32 number) {
	assert (number < kNumCutscenes);
	_cutsceneEnabled[number] = true;
}

Graphics::RendererType GrimEngine::getRendererType() {
	return g_driver->type;
}

void GrimEngine::setSaveMetaData(const char *meta1, int meta2, const char *meta3) {
	_saveMeta1 = meta1;
	_saveMeta2 = meta2;
	_saveMeta3 = meta3;
}

bool GrimEngine::useGrimeFlow() const {
	return _gameType == GType_GRIM && !(_gameFlags & ADGF_DEMO) && !isRemastered();
}

Common::Error GrimEngine::runGrime() {
	g_resourceloader = new ResourceLoader();

	createImuse();
	createModules();

	if (!grimStartup()) {
		grimShutdown();
		destroyModules();

		return Common::kUnknownError;
	}

	if (ConfMan.hasKey("save_slot"))
		loadGameState(ConfMan.getInt("save_slot"));

	while (!shouldQuit() && !_bQuit) {
		uint32 startTime = g_system->getMillis();

		pumpEvents();
		grimMain();

		uint32 endTime = g_system->getMillis();

		if (endTime >= startTime && endTime - startTime < _speedLimitMs)
			g_system->delayMillis(_speedLimitMs - (endTime - startTime));
	}

	grimShutdown();
	destroyModules();

	return Common::kNoError;
}

void GrimEngine::createImuse() {
	bool demo = getGameFlags() & ADGF_DEMO;

	// The remastered works at 48000Hz
	int baseSampleRate = isRemastered() ? IMUSE_REMASTERED_SAMPLERATE : IMUSE_BASE_SAMPLERATE;
	int sampleRate = baseSampleRate;
	ConfMan.registerDefault("imuse_sample_rate", baseSampleRate);
	if (ConfMan.hasKey("imuse_sample_rate", _targetName)) {
		// Only accept sample rates which are a multiple or submultiple of the base one
		int configSampleRate = ConfMan.getInt("imuse_sample_rate");
		if ((configSampleRate % (baseSampleRate / 2)) == 0 &&
			configSampleRate >= baseSampleRate / 2 &&
			configSampleRate <= baseSampleRate * 4) {
			sampleRate = configSampleRate;
		}
	}

	bool lowLatencyMode = false;
	ConfMan.registerDefault("imuse_low_latency_mode", false);
	if (ConfMan.hasKey("imuse_low_latency_mode", _targetName))
		lowLatencyMode = ConfMan.getBool("imuse_low_latency_mode");

	g_imuse = new IMuse(sampleRate, _mixer, &_imuseMutex, lowLatencyMode, demo, isRemastered());
}

void GrimEngine::createModules() {
	_pImuse = g_imuse;

	_pStdMath = new StdMath();
	_pStdFile = new StdFile();
	_pStdDisplay = new StdDisplay();
	_pStdConffile = new StdConffile(this);
	_pStdBitmap = new StdBitmap(_pStdDisplay, _pStdFile, this);
	_pStdDisplay->setStdBitmap(_pStdBitmap);

#if defined(USE_OPENGL_SHADERS)
	_pStd3D = new Std3D(_pStdDisplay, _pStdBitmap);
#endif

	_pStdDisplay->setStd3D(_pStd3D);
	_pStdLbm = new StdLbm(_pStdDisplay, _pStdFile, _pStdBitmap, this);
	_pStdPcx = new StdPcx(_pStdDisplay, _pStdFile, _pStdBitmap, this);
	_pStdGif = new StdGif(_pStdDisplay, _pStdFile, _pStdBitmap, this);
	_pStdPic = new StdPic(_pStdDisplay, _pStdFile, this);
	_pStdBmp = new StdBmp(_pStdDisplay, _pStdFile, this);
	_pStdBitmap->setImageFormats(_pStdLbm, _pStdPcx, _pStdGif, _pStdPic, _pStdBmp);

	_pRd = new RenderDroid(_pStdMath, _pStdDisplay, _pStdConffile, _pStdFile);
	_pRd->setStd3D(_pStd3D);
	_pWalkSystem = new WalkSystem(_pRd, _pStdMath, _pStdConffile);

	_pRes = new Res();
	_pSputResource = new SputResource(_pRes, _pRd, _pStdDisplay, _pStdBitmap);
	_pStdControl = new StdControl();

	StdControlInput *pControlInput = _pStdControl->getInput();
	StdControlJoyCaps *pJoyCaps = &pControlInput->joyCaps[0];

	pControlInput->numJoysticks = 1;
	pJoyCaps->bAvailable = 1;
	pJoyCaps->flags = 0x0F;
	pJoyCaps->maxButtons = 10;

	for (int axis = 0; axis < 6; axis++) {
		pJoyCaps->axisMin[axis] = -32768;
		pJoyCaps->axisMax[axis] = 32767;
	}

	pControlInput->joyState[0].flags = 0x3C;

	_pWuRegistry = new WuRegistry();
	_pPolyglot = new Polyglot();
	_pSputTime = new SputTime();
	_pSputPrimitive = new SputPrimitive(_pRd);
	_pSputRoom = new SputRoom(_pRd, _pRes, _pSputResource, _pStdDisplay, _pStdBitmap, _pStdLbm, _pStdConffile, _pWalkSystem);
	_pSputControl = new SputControl(_pRd, _pSputRoom, _pStdControl);

	_pText = new Text();
	_pBlastText = new BlastText(_pText);
	_pText->setBlastText(_pBlastText);
	_pNormalText = new NormalText();
	_pSputText = new SputText(_pRes, _pRd, _pNormalText, _pText, _pBlastText, _pSputTime, _pPolyglot);

	_pMainGfxBuffer = new StdVBuffer();
	_pBackBuffer = new StdVBuffer();
	_pStdDisplay->setBuffers(_pMainGfxBuffer, _pBackBuffer);
	_pSputRender = new SputRender(_pRd, _pStdDisplay, _pStdBitmap, _pRes, _pSputResource, _pSputPrimitive, _pWalkSystem, _pSputRoom,
			_pBackBuffer, _pMainGfxBuffer, _pSputTime, _pSputControl);
	_pSputCostume = new SputCostume(_pRes, _pRd, _pSputResource, _pSputRender, _pSputRoom, _pStdLbm, _pWuRegistry);
	_pSputActor = new SputActorModule(_pRes, _pRd, _pStdMath, _pStdDisplay, _pWalkSystem, _pSputResource, _pSputRoom, _pSputRender,
			_pSputCostume, _pSputText, _pSputTime);
	_pSputSmush = new SputSmush(_pRes, _pStdDisplay, _pSputRender, _pMainGfxBuffer, _pWuRegistry);
	_pLua = new Lua_V1();

	_pRes->setSputRender(_pSputRender);
	_pSputResource->setSputRender(_pSputRender);
	_pSputResource->setSputRoom(_pSputRoom);
	_pSputResource->setCos(_pSputCostume->getCos());
	_pSputResource->setText(_pText);
	_pSputResource->setBlastText(_pBlastText);
	_pSputResource->setWuRegistry(_pWuRegistry);
	_pSputPrimitive->setSputRender(_pSputRender);
	_pSputPrimitive->setLua(_pLua);
	_pSputRoom->setSputRender(_pSputRender);
	_pSputRoom->setSputActor(_pSputActor);
	_pSputRoom->setGrimEngine(this);
	_pSputText->setSputRender(_pSputRender);
	_pSputText->setSputRoom(_pSputRoom);
	_pSputText->setGrimEngine(this);
	_pSputText->setLua(_pLua);
	_pSputRender->setSputActor(_pSputActor);
	_pSputRender->setSputText(_pSputText);
	_pSputRender->setBlastText(_pBlastText);
	_pSputRender->setSputSmush(_pSputSmush);
	_pSputRender->setStdControl(_pStdControl);
	_pSputRender->setStd3D(_pStd3D);
	_pSputRender->setGrimEngine(this);
	_pSputRender->setLua(_pLua);
	_pSputCostume->setSputActor(_pSputActor);
	_pSputCostume->setGrimEngine(this);
	_pSputCostume->setLua(_pLua);
	_pSputActor->setGrimEngine(this);
	_pSputActor->setLua(_pLua);
	_pSputActor->setStd3D(_pStd3D);
	_pSputControl->setGrimEngine(this);
}

void GrimEngine::destroyModules() {
	if (_bDisplayOpen)
		shutdownDisplay();

	freeVideoDevices();

	delete _pLua;
	_pLua = nullptr;
	delete _pSputSmush;
	_pSputSmush = nullptr;
	delete _pSputActor;
	_pSputActor = nullptr;
	delete _pSputCostume;
	_pSputCostume = nullptr;
	delete _pSputRender;
	_pSputRender = nullptr;
	delete _pBackBuffer;
	_pBackBuffer = nullptr;
	delete _pMainGfxBuffer;
	_pMainGfxBuffer = nullptr;
	delete _pSputText;
	_pSputText = nullptr;
	delete _pNormalText;
	_pNormalText = nullptr;
	delete _pBlastText;
	_pBlastText = nullptr;
	delete _pText;
	_pText = nullptr;
	delete _pSputControl;
	_pSputControl = nullptr;
	delete _pSputRoom;
	_pSputRoom = nullptr;
	delete _pSputPrimitive;
	_pSputPrimitive = nullptr;
	delete _pSputTime;
	_pSputTime = nullptr;
	delete _pPolyglot;
	_pPolyglot = nullptr;
	delete _pWuRegistry;
	_pWuRegistry = nullptr;
	delete _pStdControl;
	_pStdControl = nullptr;
	delete _pSputResource;
	_pSputResource = nullptr;
	delete _pRes;
	_pRes = nullptr;
	delete _pWalkSystem;
	_pWalkSystem = nullptr;
	delete _pRd;
	_pRd = nullptr;

#if defined(USE_OPENGL_SHADERS)
	delete _pStd3D;
#endif

	_pStd3D = nullptr;
	delete _pStdBmp;
	_pStdBmp = nullptr;
	delete _pStdPic;
	_pStdPic = nullptr;
	delete _pStdGif;
	_pStdGif = nullptr;
	delete _pStdPcx;
	_pStdPcx = nullptr;
	delete _pStdLbm;
	_pStdLbm = nullptr;
	delete _pStdBitmap;
	_pStdBitmap = nullptr;
	delete _pStdConffile;
	_pStdConffile = nullptr;
	delete _pStdDisplay;
	_pStdDisplay = nullptr;
	delete _pStdFile;
	_pStdFile = nullptr;
	delete _pStdMath;
	_pStdMath = nullptr;
	_pImuse = nullptr;
}

bool GrimEngine::wantsHardwareRenderer() const {
#if defined(USE_OPENGL_SHADERS)
	if (!(Graphics::Renderer::getAvailableTypes() & Graphics::kRendererTypeOpenGLShaders))
		return false;

	Graphics::RendererType desiredRendererType = Graphics::Renderer::parseTypeCode(ConfMan.get("renderer"));

	return desiredRendererType == Graphics::kRendererTypeDefault || desiredRendererType == Graphics::kRendererTypeOpenGLShaders;
#else
	return false;
#endif
}

int GrimEngine::grimStartup() {
	_bUseHardware = wantsHardwareRenderer() ? 1 : 0;

	_pStdDisplay->startup();
	_pPolyglot->initialize(this);
	_pPolyglot->startup();
	_pPolyglot->configure("grim.tab");
	_pWuRegistry->startup("1.0");

	_pRd->startup(this);

	if (!_pRd->open(0))
		return 0;

	if (!_pStdDisplay->enumerateDevices())
		return 0;

	initDisplay();

	if (_bEnumerateVideo && _bShow)
		_pWuRegistry->saveInt("ExperimentalDisplay", 1);

	if (!initDisplayAndDevice())
		return 0;

	if (sputInitialize(this))
		return 0;

	if (sputStartup()) {
		fatalError(_pPolyglot->getTranslation("/engn004/Please make sure you are not running any other DirectX apps that take exclusive control of your hardware."),
				_pPolyglot->getTranslation("/engn005/Grim Fandango could not initialize your hardware!"));
		return 0;
	}

	_pLua->registerHardwareOpcodes();
	setShutdownCallback(shutdownCallback);
	setPaletteCallback(paletteCallback);
	setPageFlipCallback(pageFlipCallback);
	findResources();
	loadSplash();

	if (bundleDofile()) {
		fatalError(_pPolyglot->getTranslation("/engn006/Please make sure that the game is properly installed and that the CD is in your drive."),
				_pPolyglot->getTranslation("/engn007/Grim Fandango could not find files!"));
		return 0;
	}

	if (startEngine(_saveGameToResume, nullptr))
		return 0;

	return 1;
}

void GrimEngine::grimMain() {
	if (_bQuit)
		return;

	if (_bDisplayChangePending) {
		int mode = _libMode;

		if (!selectDisplayMode(_pendingDevice, _pendingMode, _pending3DDevice)) {
			warning("GrimEngine::grimMain(): could not change the display, going back to the software display");

			int device;
			int displayMode;
			int threeDDevice;

			_bUseHardware = 0;
			chooseDefaultDisplay(1, 0, &device, &displayMode, &threeDDevice);
			selectDisplayMode(device, displayMode, threeDDevice);
		}

		_libMode = 2;
		_pSputRender->dim(1, 2);
		_libMode = mode;
		_bDisplayChangePending = 0;
	}

	loop(_bActive, _bFirstFrame);
	_bFirstFrame = 0;
	_grimFrameCount++;

	uint32 curTime = g_system->getMillis();
	uint32 elapsed = curTime - _grimLastFpsTime;

	if (elapsed > 1000) {
		_grimLastFpsTime = curTime;

		int numFrames = _grimFrameCount - _grimLastFpsFrame;

		_grimLastFpsFrame = _grimFrameCount;
		_grimFps = (double)numFrames * 1000.0 / (double)elapsed;
	}

	if (_showFps) {
		Common::String text = Common::String::format("%02.3f", _grimFps);
		textOut(text.c_str(), 4, 4);

		StdRect rect = { 0, 0, 80, 30 };

		markDirtyRect(&rect, 0);
	}

	if (_bShowDebugOverlay) {
		int y = 30;

		for (int i = 0; i < kNumPerfCounters; i++) {
			Common::String text = Common::String::format("%s %02.3f", _aPerfCounters[i].pName, _aPerfCounters[i].elapsed);
			textOut(text.c_str(), 4, y);

			StdRect rect = { 4, y, 190, 20 };

			markDirtyRect(&rect, 0);
			_aPerfCounters[i].elapsed = 0.0;
			y += 20;
		}
	}

	if (_bActive)
		timedPageFlip();
}

void GrimEngine::textOut(const char *text, int x, int y) {
	if (!_bDisplayOpen || !_pBackBuffer)
		return;

	const Graphics::Font *pFont = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);

	if (!pFont) {
		warning("GrimEngine::textOut(): no built-in font available");
		return;
	}

	bool bVideoMemory = _pBackBuffer->type == 1;

	if (bVideoMemory && !_pStdDisplay->vBufferLock(_pBackBuffer)) {
		warning("GrimEngine::textOut(): could not lock the back buffer");
		return;
	}

	if (_pBackBuffer->pPixels) {
		Graphics::PixelFormat pixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);
		Graphics::Surface surface;

		surface.init(_pBackBuffer->tex.width, _pBackBuffer->tex.height, _pBackBuffer->tex.pitchInBytes, _pBackBuffer->pPixels, pixelFormat);

		uint32 color = surface.format.RGBToColor(255, 255, 0);

		pFont->drawString(&surface, Common::String(text), x, y, surface.w - x, color);
	}

	if (bVideoMemory)
		_pStdDisplay->vBufferUnlock(_pBackBuffer);
}

void GrimEngine::grimShutdown() {
	freeVideoDevices();
	close();

	if (_bSputStarted)
		sputShutdown();

	sputTerminate();

	_pRd->close();
	_pRd->shutdown();

	shutdownDisplay();
	_pStdDisplay->restoreDisplayMode();

	_pWuRegistry->shutdown();
	_pPolyglot->shutdown();
	_pStdDisplay->shutdown();
}

int GrimEngine::shutdownCallback(int arg) {
	g_grim->_bQuit = 1;

	return 0;
}

Common::SeekableReadStream *GrimEngine::openFile(const char *path) {
	Common::String name(path);

	name.replace('\\', '/');
	name.toLowercase();

	Common::String baseName = Common::lastPathComponent(name, '/');
	Common::Path fullPath(name, '/');
	Common::Path basePath(baseName, Common::Path::kNoSeparator);
	Common::SeekableReadStream *pStream = nullptr;

	if (SearchMan.hasFile(fullPath)) {
		pStream = SearchMan.createReadStreamForMember(fullPath);
		pStream = wrapPatchedFile(pStream, fullPath);
	} else if (SearchMan.hasFile(basePath)) {
		pStream = SearchMan.createReadStreamForMember(basePath);
		pStream = wrapPatchedFile(pStream, basePath);
	}

	return pStream;
}

int GrimEngine::sputInitialize(SysStruct *pHS) {
	_pSputHS = pHS;

	int result = callRegisterModules(pHS, 0, kNumBaseModules);

	if (!result)
		result = callRegisterModules(pHS, kNumBaseModules, kNumMainModules);

	_bSputInitialized = result == 0;

	return result;
}

int GrimEngine::sputTerminate() {
	_pSputHS = nullptr;

	if (_bSputInitialized) {
		callRelease(kNumBaseModules, kNumMainModules);
		callRelease(0, kNumBaseModules);
		_bSputInitialized = 0;
	}

	return 0;
}

int GrimEngine::sputStartup() {
	if (_bSputStarted)
		return 1;

	int result = runInitTable(0, kNumBaseModules);

	if (!result) {
		int physicalRam = kPhysicalRam;

		if (physicalRam > 0x1000000) {
			int memoryLimit = _pRes->getMemoryLimit();

			if (memoryLimit >= (int)(physicalRam - 0x800000))
				memoryLimit = physicalRam - 0x800000;

			_pSputResource->setMemoryUsage(memoryLimit, 0);
		} else {
			_pSputResource->setMemoryUsage(10485760, 0);
		}

		result = runInitTable(kNumBaseModules, kNumMainModules);
	}

	_bSputStarted = result == 0;

	return result;
}

int GrimEngine::sputShutdown() {
	callTableFunc(kNumBaseModules, kNumMainModules);

	int result = callTableFunc(0, kNumBaseModules);

	_bSputStarted = 0;

	return result;
}

int GrimEngine::callRegisterModules(SysStruct *pHS, int firstModule, int count) {
	for (int i = 0; i < count; i++) {
		if (initializeModule(firstModule + i, pHS)) {
			for (int j = i - 1; j >= 0; j--)
				terminateModule(firstModule + j);

			return 1;
		}
	}

	return 0;
}

int GrimEngine::runInitTable(int firstModule, int count) {
	for (int i = 0; i < count; i++) {
		if (startupModule(firstModule + i)) {
			for (int j = i - 1; j >= 0; j--)
				shutdownModule(firstModule + j);

			return 1;
		}
	}

	return 0;
}

int GrimEngine::callTableFunc(int firstModule, int count) {
	for (int i = count - 1; i >= 0; i--)
		shutdownModule(firstModule + i);

	return 0;
}

int GrimEngine::callRelease(int firstModule, int count) {
	for (int i = count - 1; i >= 0; i--)
		terminateModule(firstModule + i);

	return 0;
}

int GrimEngine::initializeModule(int module, SysStruct *pHS) {
	switch (module) {
	case kSputMainModuleRoom:
		return _pSputRoom->initialize(pHS);
	case kSputMainModuleTime:
		return _pSputTime->initialize(pHS);
	case kSputMainModuleLua:
		return _pLua->initialize(pHS);
	case kSputMainModuleCostume:
		return _pSputCostume->initialize(pHS);
	case kSputMainModuleActor:
		return _pSputActor->initialize(pHS);
	case kSputMainModuleResource:
		return _pSputResource->initialize(pHS);
	case kSputMainModuleControl:
		return _pSputControl->initialize(pHS);
	case kSputMainModuleSounds:
		return 0;
	case kSputMainModuleRender:
		return _pSputRender->initialize(pHS);
	case kSputMainModuleText:
		return _pSputText->initialize(pHS);
	case kSputMainModulePrimitive:
		return _pSputPrimitive->initialize(pHS);
	case kSputMainModuleSmush:
		return _pSputSmush->initialize(pHS);
	default:
		return 0;
	}
}

int GrimEngine::startupModule(int module) {
	switch (module) {
	case kSputMainModuleRoom:
		return _pSputRoom->startup();
	case kSputMainModuleTime:
		return _pSputTime->startup();
	case kSputMainModuleLua:
		return _pLua->startup();
	case kSputMainModuleCostume:
		return _pSputCostume->startup();
	case kSputMainModuleActor:
		return _pSputActor->startup();
	case kSputMainModuleResource:
		return _pSputResource->startup();
	case kSputMainModuleControl:
		return _pSputControl->startup();
	case kSputMainModuleSounds:
		return 0;
	case kSputMainModuleRender:
		return _pSputRender->startup();
	case kSputMainModuleText:
		return _pSputText->startup();
	case kSputMainModulePrimitive:
		return _pSputPrimitive->startup();
	case kSputMainModuleSmush:
		return _pSputSmush->startup();
	default:
		return 0;
	}
}

void GrimEngine::shutdownModule(int module) {
	switch (module) {
	case kSputMainModuleRoom:
		_pSputRoom->shutdown();
		break;
	case kSputMainModuleTime:
		_pSputTime->shutdown();
		break;
	case kSputMainModuleLua:
		_pLua->shutdown();
		break;
	case kSputMainModuleCostume:
		_pSputCostume->shutdown();
		break;
	case kSputMainModuleActor:
		_pSputActor->shutdown();
		break;
	case kSputMainModuleResource:
		_pSputResource->shutdown();
		break;
	case kSputMainModuleControl:
		_pSputControl->shutdown();
		break;
	case kSputMainModuleSounds:
		break;
	case kSputMainModuleRender:
		_pSputRender->shutdown();
		break;
	case kSputMainModuleText:
		_pSputText->shutdown();
		break;
	case kSputMainModulePrimitive:
		_pSputPrimitive->shutdown();
		break;
	case kSputMainModuleSmush:
		_pSputSmush->shutdown();
		break;
	default:
		break;
	}
}

void GrimEngine::terminateModule(int module) {
	switch (module) {
	case kSputMainModuleRoom:
		_pSputRoom->terminate();
		break;
	case kSputMainModuleTime:
		_pSputTime->terminate();
		break;
	case kSputMainModuleLua:
		_pLua->terminate();
		break;
	case kSputMainModuleCostume:
		_pSputCostume->terminate();
		break;
	case kSputMainModuleActor:
		_pSputActor->terminate();
		break;
	case kSputMainModuleResource:
		_pSputResource->terminate();
		break;
	case kSputMainModuleControl:
		_pSputControl->terminate();
		break;
	case kSputMainModuleSounds:
		break;
	case kSputMainModuleRender:
		_pSputRender->terminate();
		break;
	case kSputMainModuleText:
		_pSputText->release();
		break;
	case kSputMainModulePrimitive:
		_pSputPrimitive->terminate();
		break;
	case kSputMainModuleSmush:
		_pSputSmush->terminate();
		break;
	default:
		break;
	}
}

int GrimEngine::bundleDofile() {
	int bFound = _pLua->findSystemFiles();

	return !bFound;
}

int GrimEngine::startEngine(int saveGameResume, const char *bootParam) {
	if (_bSputOpen)
		return 1;

	int bBooted = _pLua->boot(saveGameResume, bootParam);

	if (!bBooted)
		return 1;

	_libMode = 2;
	_pSputRender->open();

	if (!_pSputControl->open())
		return 1;

	_bSputOpen = 1;

	return 0;
}

int GrimEngine::loop(int bRender, int manualTicks) {
	if (_libSaveRequest)
		save();

	if (_libLoadRequest)
		restore();

	setPerfCounter(0);
	setPerfCounter(10);
	_pImuse->soundsPeriodic();
	queryPerfCounterDiff(10);

	int result = _libMode;

	if (_libMode) {
		setPerfCounter(1);
		_pSputControl->update(_pSputTime->getFrameTimeFlex());
		queryPerfCounterDiff(1);

		int mode = _libMode;

		if (mode != 1 || manualTicks) {
			_loopCount++;

			if (manualTicks)
				_pSputTime->advanceManual(manualTicks);
			else
				_pSputTime->advance();

			if (_bRunLua) {
				setPerfCounter(2);
				_pLua->update();
				queryPerfCounterDiff(2);
			}

			mode = _libMode;

			if (mode != 3 && mode != 4) {
				setPerfCounter(3);
				_pSputActor->updateActors(_pSputTime->getFrameTimeFlex());
				queryPerfCounterDiff(3);
				mode = _libMode;
			}
		}

		if (bRender && (mode != 1 || manualTicks)) {
			_pSputRender->update();
			_bRenderPending = 1;
		}

		queryPerfCounterDiff(0);

		result = _libImuseState;

		if (result != -1) {
			result = _pImuse->imSetState(result);
			_libImuseState = -1;
		}
	}

	return result;
}

void GrimEngine::timedPageFlip() {
	setPerfCounter(8);

	switch (_libMode) {
	case 3:
		if (!_pSputRender->queryDisplayState())
			_pSputRender->pageFlip();

		break;
	case 2:
		_pSputRender->pageFlip();
		break;
	case 1:
		if (_bRenderPending) {
			_pSputRender->pageFlip();
			_bRenderPending = 0;
		}

		break;
	default:
		break;
	}

	queryPerfCounterDiff(8);
}

void GrimEngine::setPerfCounter(uint slot) {
	if (slot >= (uint)kNumPerfCounters)
		error("GrimEngine::setPerfCounter(): slot %u out of range", slot);

	_aPerfCounters[slot].start = (double)g_system->getMillis();
}

int GrimEngine::queryPerfCounterDiff(uint slot) {
	if (slot >= (uint)kNumPerfCounters)
		error("GrimEngine::queryPerfCounterDiff(): slot %u out of range", slot);

	double elapsed = (double)g_system->getMillis() - _aPerfCounters[slot].start;

	_aPerfCounters[slot].elapsed = elapsed;

	return (int)elapsed;
}

void GrimEngine::setPrevMode(int mode) {
	_sputPrevMode = mode;
}

int GrimEngine::getPrevMode() const {
	return _sputPrevMode;
}

void GrimEngine::overrideSelectedActorCostume(int hInstance) {
	SputActor *pActor = _pSputActor->getSelectedActor();

	if (pActor) {
		_pLua->doCostumeOverrideCallback(pActor, 1);
		_pSputActor->pushCostume(pActor, nullptr, hInstance);
	}
}

void GrimEngine::restoreSelectedActorCostume() {
	SputActor *pActor = _pSputActor->getSelectedActor();

	if (pActor) {
		_pLua->doCostumeOverrideCallback(pActor, 0);
		_pSputActor->popCostume(pActor, 0);
	}
}

int GrimEngine::setDisplay(StdVBuffer *pVBuffer, StdVBuffer *pZBuffer) {
	_pSputRender->pageCountIs(_numDrawBuffers);
	_pSputRender->setDisplay(pVBuffer, pZBuffer, 0, 0, pVBuffer->tex.width - 1, pVBuffer->tex.height - 1, _libRenderOptions);

	return 0;
}

int GrimEngine::unsetDisplay() {
	_pSputRender->close();

	return 0;
}

void GrimEngine::makeCurrentSet(const char *name) {
	if (name) {
		Room *pRoom = static_cast<Room *>(_pSputResource->alloc(kSputTagSet, name));

		if (pRoom && pRoom->apColormaps && pRoom != _pSputRoom->getCurrentRoom()) {
			_pSputRoom->setCurrentRoom(pRoom);
			_pWalkSystem->setCollection(pRoom->name);
			_pSputRender->syncCamera(pRoom, pRoom->curSetup);
		}
	} else {
		_pSputRoom->setCurrentRoom(nullptr);
	}
}

void GrimEngine::close() {
	if (_bSputOpen) {
		_libMode = 0;
		_pSputControl->close();
		_pSputRender->close();
		_pSputRoom->setCurrentRoom(nullptr);
		_bSputOpen = 0;
	}
}

void GrimEngine::setDebugToggleCallback(void (*pFunc)(int)) {
	_pLua->setDebugToggleCallback(pFunc);
}

void GrimEngine::setPaletteCallback(int (*pFunc)(byte *, int)) {
	_pSputRender->setPaletteCallback(pFunc);
}

void GrimEngine::setShutdownCallback(int (*pFunc)(int)) {
	_pShutdownCallback = pFunc;
}

void GrimEngine::setPageFlipCallback(int (*pFunc)()) {
	_pSputRender->setPageFlipCallback(pFunc);
}

void GrimEngine::using3DHardware(int b3D) {
	_pSputRender->using3DHardware(b3D);
}

int GrimEngine::addResourcePath(const char *path) {
	return _pSputResource->addResourcePath(path);
}

int GrimEngine::findResources() {
	return _pSputResource->findResources();
}

void GrimEngine::markDirtyRect(const StdRect *pRect, int page) {
	_pSputRender->markDirtyRect(pRect, page);
}

void GrimEngine::makeCurrentSetupLib(uint setup) {
	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom) {
		warning("GrimEngine::makeCurrentSetupLib(): no current room");
		return;
	}

	uint numSetups = pRoom->numSetups;
	int newSetup = (int)setup;

	if (setup >= numSetups) {
		if (!numSetups) {
			warning("GrimEngine::makeCurrentSetupLib(): the room has no setups");
			return;
		}

		newSetup = (int)(setup % numSetups);
	}

	int oldSetup = pRoom->curSetup;

	if (newSetup != oldSetup) {
		pRoom->curSetup = newSetup;
		_pSputRoom->setSoundParameters(pRoom, 8.0f, 20, 127, 1.5f, 0.8f);
		_pLua->cameraChange(oldSetup, newSetup);
		_pSputRoom->relocateSounds();
	}
}

void GrimEngine::nextSetup() {
	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom || !pRoom->numSetups) {
		warning("GrimEngine::nextSetup(): no current room or no setups");
		return;
	}

	makeCurrentSetupLib((uint)(pRoom->curSetup + 1) % pRoom->numSetups);
}

void GrimEngine::previousSetup() {
	Room *pRoom = _pSputRoom->getCurrentRoom();

	if (!pRoom || !pRoom->numSetups) {
		warning("GrimEngine::previousSetup(): no current room or no setups");
		return;
	}

	makeCurrentSetupLib((uint)(pRoom->curSetup + pRoom->numSetups - 1) % pRoom->numSetups);
}

int GrimEngine::confirmClose() {
	int result = _pLua->doExitCallback();

	return result == 0;
}

void GrimEngine::requestShutdown(int arg) {
	if (_pShutdownCallback)
		_pShutdownCallback(arg);
}

void GrimEngine::setPause(int bPause) {
	if (!_libMode)
		return;

	if (bPause) {
		if (_libMode != 1) {
			_libPauseSavedMode = _libMode;
			_libPauseSavedOldMode = _libPauseOldMode;
			_libMode = 1;
			_pImuse->imPause();
			_pSputSmush->pause(1);
		}
	} else if (_libMode == 1) {
		_libMode = _libPauseSavedMode;
		_libPauseOldMode = _libPauseSavedOldMode;
		_pImuse->imResume();
		_pSputSmush->pause(0);
	}
}

int GrimEngine::doPauseCallback() {
	int result = _pLua->doPauseCallback();

	return result;
}

int GrimEngine::characterCallback(int character) {
	int result = _pLua->doCharacterCallback(character);

	return result;
}

int GrimEngine::setImuseState(int state) {
	_libImuseState = state;

	return state;
}

int GrimEngine::playMovie(const char *name, int bFullscreen, int bLooping, int x, int y) {
	if (bFullscreen)
		_pSputSmush->setTarget(_pMainGfxBuffer);
	else
		_pSputSmush->setTarget(_pSputRender->drawBuffer());

	if (_pSputSmush->startMovie(name, bLooping, x, y))
		return 0;

	if (bFullscreen) {
		_libMode = 3;
		return 1;
	}

	if (_libMode == 3)
		_libMode = 2;

	return 1;
}

int GrimEngine::userPaint() {
	int result = 0;

	if (_libMode == 4) {
		_pLua->doUserPaintCallback();
	}

	return result;
}

void GrimEngine::saveLib(const char *fileName) {
	_libSaveRequest = 1;
	_libSaveGameFileName = fileName ? fileName : "";
}

void GrimEngine::loadLib(const char *fileName) {
	_libLoadRequest = 1;
	_libSaveGameFileName = fileName ? fileName : "";
}

int GrimEngine::justLoaded() {
	int result = _libJustLoaded;

	_libJustLoaded = 0;

	return result;
}

static const StdColorFormat g_saveImageFormat = { 1, 16, 5, 6, 5, 11, 5, 0, 3, 2, 3, 0, 0, 0 };

static const int kSoundsSaveVersion = 30;

static int luaZwrite(void *buffer, int size) {
	return g_grim->zwrite(buffer, size);
}

static int luaZread(void *buffer, int size) {
	return g_grim->zread(buffer, size);
}

void GrimEngine::save() {
	_libSaveRequest = 0;
	_pSputRender->allowLoadImage(0);
	_pSputRender->drawProgressBar(0.1f);

	Common::String fileName = _libSaveGameFileName;

	if (fileName.empty())
		fileName = "grim.sav";

	_pSaveWriteStream = g_system->getSavefileManager()->openForSaving(fileName);

	if (!_pSaveWriteStream)
		return;

	_pLua->saveGameCallback();

	StdVBuffer *pShot = _pSputRender->screenShot(250, 188);
	uint32 imageSize = 0;

	if (pShot) {
		StdColorFormat format = g_saveImageFormat;
		StdVBuffer *pImage = _pStdDisplay->vBufferConvertColorFormat(&format, pShot);

		if (pImage) {
			imageSize = TO_LE_32((uint32)pImage->tex.sizeInBytes);
			zwrite(&imageSize, 4);
			zwrite(pImage->pPixels, pImage->tex.sizeInBytes);
			_pStdDisplay->vBufferFree(pImage);
		}
	} else {
		warning("GrimEngine::save(): no screenshot for the savegame");
	}

	if (!imageSize)
		zwrite(&imageSize, 4);

	_pSputRender->drawProgressBar(0.2f);
	_pSputCostume->getCos()->save(_pSaveWriteStream);
	_pSputRender->drawProgressBar(0.3f);
	_pRes->sysSave(_pSaveWriteStream);
	_pSputRender->drawProgressBar(0.4f);
	_pSputText->save(_pSaveWriteStream);
	_pSputRoom->save(_pSaveWriteStream);
	_pSputRender->drawProgressBar(0.5f);
	_pSputActor->save(_pSaveWriteStream);
	_pSputRender->save(_pSaveWriteStream);
	_pSputPrimitive->save(_pSaveWriteStream);
	_pSputSmush->save(_pSaveWriteStream);

	uint32 prevMode = TO_LE_32((uint32)_sputPrevMode);

	zwrite(&prevMode, 4);
	_pSputRender->drawProgressBar(0.75f);
	lua_Save(luaZwrite);

	_pSputRender->drawProgressBar(1.0f);
	soundsSave(_pSaveWriteStream);

	_pSaveWriteStream->finalize();
	delete _pSaveWriteStream;
	_pSaveWriteStream = nullptr;

	_pSputRender->allowLoadImage(1);
}

int GrimEngine::zwrite(const void *buffer, int size) {
	if (size && _pSaveWriteStream->write(buffer, size) != (uint32)size)
		error("GrimEngine::zwrite(): write error");

	return 0;
}

void GrimEngine::restore() {
	_libLoadRequest = 0;

	Common::String fileName = _libSaveGameFileName;

	if (fileName.empty())
		fileName = "grim.sav";

	_pSaveReadStream = g_system->getSavefileManager()->openForLoading(fileName);

	if (!_pSaveReadStream)
		return;

	if (LegacySaveFile::isLegacySave(_pSaveReadStream)) {
		restoreLegacy();
		return;
	}

	restorePrologue();

	uint32 blockSize = 0;

	zread(&blockSize, 4);
	_pSaveReadStream->seek((int)FROM_LE_32(blockSize), SEEK_CUR);

	uint32 imageSize = 0;

	zread(&imageSize, 4);
	_pSaveReadStream->seek((int)FROM_LE_32(imageSize), SEEK_CUR);

	_pSputRender->drawProgressBar(0.2f);
	_pSputCostume->getCos()->restore(_pSaveReadStream);
	_pSputRender->drawProgressBar(0.3f);
	_pRes->sysLoad(_pSaveReadStream);
	_pSputText->restore(_pSaveReadStream);
	_pSputRender->drawProgressBar(0.4f);
	_pSputRoom->restore(_pSaveReadStream);
	_pSputActor->restore(_pSaveReadStream);
	_pSputRender->restore(_pSaveReadStream);
	_pSputPrimitive->restore(_pSaveReadStream);
	_pSputSmush->restore(_pSaveReadStream);

	_libMode = 4;

	uint32 prevMode = 0;

	zread(&prevMode, 4);
	_sputPrevMode = (int)FROM_LE_32(prevMode);
	_pSputRender->drawProgressBar(0.75f);
	lua_Restore(luaZread);

	_pSputRender->drawProgressBar(1.0f);
	soundsRestore(_pSaveReadStream);
	restoreEpilogue();
}

void GrimEngine::restorePrologue() {
	_pSputRender->allowLoadImage(0);
	_pSputRender->drawProgressBar(0.1f);
	_pImuse->imStopAllSounds();
	_pSputSmush->stopMovie();
	_pSputSmush->periodic();
	_pRes->freeAll();
	_pSputActor->clearShadowMaterial();
	_pSputResource->stopPurge(1);
}

void GrimEngine::restoreEpilogue() {
	_pSputResource->stopPurge(0);

	Room *pRoom = _pSputRoom->getCurrentRoom();
	int curSetup = 0;

	if (pRoom)
		curSetup = pRoom->curSetup;

	_pSputRender->useSetup(pRoom, curSetup);

	delete _pSaveReadStream;
	_pSaveReadStream = nullptr;

	_pLua->dofile("patch05.bin");
	_libJustLoaded = 1;
	_pSputRender->allowLoadImage(1);

	StdRect rect = { 0, 0, 640, 480 };

	_pSputRender->markAllPagesDirty(&rect, 1);
}

int GrimEngine::zread(void *buffer, int size) {
	if (size)
		_pSaveReadStream->read(buffer, size);

	return 0;
}

StdVBuffer *GrimEngine::getSaveGameImage(const char *fileName) {
	Common::SeekableReadStream *pFile = g_system->getSavefileManager()->openForLoading(fileName);

	if (!pFile)
		return nullptr;

	Common::Array<byte> legacyPixels;
	bool bLegacy = LegacySaveFile::isLegacySave(pFile);

	if (bLegacy) {
		if (!legacyReadThumbnail(pFile, legacyPixels)) {
			delete pFile;
			return nullptr;
		}
	} else {
		int blockSize = pFile->readSint32LE();

		pFile->seek(blockSize, SEEK_CUR);
	}

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.pitchInPixels = 2;
	format.width = 250;
	format.height = 188;
	format.pitchInBytes = 500;
	format.format = g_saveImageFormat;

	StdVBuffer *pImage = _pStdDisplay->vBufferNew(&format, 0, 0, nullptr);

	if (!pImage) {
		delete pFile;
		return nullptr;
	}

	int size = bLegacy ? (int)legacyPixels.size() : pFile->readSint32LE();

	if (size != pImage->tex.sizeInBytes) {
		warning("GrimEngine::getSaveGameImage(): unexpected image size %d", size);
		_pStdDisplay->vBufferFree(pImage);
		delete pFile;
		return nullptr;
	}

	if (bLegacy)
		memcpy(pImage->pPixels, legacyPixels.data(), size);
	else
		pFile->read(pImage->pPixels, size);

	delete pFile;

	return pImage;
}

void GrimEngine::imuseSave() {
	Common::OutSaveFile *pFile = g_system->getSavefileManager()->openForSaving("grim.tmp");

	if (!pFile)
		return;

	soundsSave(pFile);
	pFile->finalize();
	delete pFile;
}

void GrimEngine::imuseRestore() {
	Common::InSaveFile *pFile = g_system->getSavefileManager()->openForLoading("grim.tmp");

	if (!pFile)
		return;

	_pImuse->imStopAllSounds();
	soundsRestore(pFile);
	delete pFile;
	g_system->getSavefileManager()->removeSavefile("grim.tmp");
}

void GrimEngine::soundsSave(Common::WriteStream *pStream) {
	Common::Serializer ser(nullptr, pStream);

	ser.setVersion(kSoundsSaveVersion);
	_pImuse->soundsSaveLoad(ser);
}

void GrimEngine::soundsRestore(Common::SeekableReadStream *pStream, bool legacyLayout) {
	Common::Serializer ser(pStream, nullptr);

	ser.setVersion(kSoundsSaveVersion);
	_pImuse->soundsSaveLoad(ser, legacyLayout);
}

enum {
	kDisplayWidth = 640,
	kDisplayHeight = 480
};

void GrimEngine::fatalError(const char *text, const char *caption) {
	Common::String message = Common::String::format("%s\n\n%s", caption, text);

	GUI::displayErrorDialog(Common::U32String(message));
	shutdownDisplay();
	CursorMan.showMouse(true);
}

int GrimEngine::initDisplay() {
	memset(&_videoDeviceConfig, 0, sizeof(_videoDeviceConfig));
	_videoDeviceConfig.tex.width = kDisplayWidth;
	_videoDeviceConfig.tex.height = kDisplayHeight;
	_videoDeviceConfig.tex.format = _colorInfoCopy;

	freeVideoDevices();

	uint numDevices = _pStdDisplay->getNumDevices();

	for (uint i = 0; i < numDevices; i++) {
		GrimVideoDevice *pVideoDevice = &_aVideoDevices[i];
		uint numModes = 0;

		if (_pStdDisplay->enumerateModes(i))
			numModes = _pStdDisplay->getNumModes();

		pVideoDevice->numModes = numModes;
		pVideoDevice->aModes = new StdDisplayMode[numModes];
		memcpy(pVideoDevice->aModes, _pStdDisplay->getModes(), sizeof(StdDisplayMode) * numModes);
		pVideoDevice->num3DDevices = 0;
		pVideoDevice->a3DDevices = nullptr;

#if defined(USE_OPENGL_SHADERS)
		if (_bUseHardware && _pStdDisplay->getDevices()[i].b3D) {
			initGraphics3d(kDisplayWidth, kDisplayHeight);

			if (_pStd3D->enumerateDevices()) {
				uint numThreeDDevices = _pStd3D->getNumDevices();

				pVideoDevice->num3DDevices = numThreeDDevices;
				pVideoDevice->a3DDevices = new Std3DDevice[numThreeDDevices];
				memcpy(pVideoDevice->a3DDevices, _pStd3D->getDevices(), sizeof(Std3DDevice) * numThreeDDevices);
				_pStd3D->releaseDirect3D();
			}
		}
#endif

		_pStdDisplay->closeDisplay();
	}

	return numDevices;
}

void GrimEngine::freeVideoDevices() {
	for (int i = 0; i < kStdDisplayMaxDevices; i++) {
		GrimVideoDevice *pVideoDevice = &_aVideoDevices[i];

		delete[] pVideoDevice->a3DDevices;
		pVideoDevice->a3DDevices = nullptr;
		delete[] pVideoDevice->aModes;
		pVideoDevice->aModes = nullptr;
		pVideoDevice->numModes = 0;
		pVideoDevice->num3DDevices = 0;
	}
}

int GrimEngine::initDisplayAndDevice() {
	int device;
	int mode;
	int threeDDevice;

	if (!loadDisplaySettings(&device, &mode, &threeDDevice))
		chooseDefaultDisplay(_bShow && _bEnumerateVideo, _bUseHardware, &device, &mode, &threeDDevice);

	int result = selectDisplayMode(device, mode, threeDDevice);

	if (result && threeDDevice != -1 && !isHardware3D()) {
		warning("GrimEngine::initDisplayAndDevice(): the 3D device could not be started");
		result = 0;
	}

	if (!result && _bUseHardware) {
		warning("GrimEngine::initDisplayAndDevice(): the hardware display failed, going back to the software display");

		_bUseHardware = 0;
		chooseDefaultDisplay(1, 0, &device, &mode, &threeDDevice);
		result = selectDisplayMode(device, mode, threeDDevice);
	}

	return result;
}

int GrimEngine::loadDisplaySettings(int *pDevice, int *pMode, int *p3DDevice) {
	if (_pWuRegistry->getInt("ExperimentalDisplay", 1)) {
		debug("grimdemo_DefaultDisplay: Last switch was not successful.");
		return 0;
	}

	byte aZeroGuid[16];
	byte aLastDisplayGuid[16];
	byte aLast3DGuid[16];

	memset(aZeroGuid, 0, sizeof(aZeroGuid));
	memset(aLastDisplayGuid, 0, sizeof(aLastDisplayGuid));
	memset(aLast3DGuid, 0, sizeof(aLast3DGuid));

	if (!_pWuRegistry->getBinary("LastDisplayDevice", aLastDisplayGuid, sizeof(aLastDisplayGuid))) {
		debug("grimdemo_DefaultDisplay: Could not find matching device.");
		return 0;
	}

	_pWuRegistry->getBinary("Last3DDevice", aLast3DGuid, sizeof(aLast3DGuid));

	uint numDevices = _pStdDisplay->getNumDevices();
	StdDisplayDevice *aDevices = _pStdDisplay->getDevices();
	uint device = 0;

	while (device < numDevices) {
		if (!memcmp(aDevices[device].guid, aLastDisplayGuid, sizeof(aLastDisplayGuid)))
			break;

		device++;
	}

	if (device == numDevices || (aDevices[device].type == 1 && !_bUseHardware)) {
		debug("grimdemo_DefaultDisplay: Could not find matching device.");
		return 0;
	}

	GrimVideoDevice *pVideoDevice = &_aVideoDevices[device];

	*pDevice = device;
	*pMode = _pStdDisplay->findClosestMode(&_videoDeviceConfig, pVideoDevice->aModes, pVideoDevice->numModes);

	if (!memcmp(aLast3DGuid, aZeroGuid, sizeof(aZeroGuid))) {
		*p3DDevice = -1;
		return 1;
	}

	uint threeDDevice = 0;

	while (threeDDevice < pVideoDevice->num3DDevices) {
		if (!memcmp(pVideoDevice->a3DDevices[threeDDevice].guid, aLast3DGuid, sizeof(aLast3DGuid)))
			break;

		threeDDevice++;
	}

	if (threeDDevice == pVideoDevice->num3DDevices) {
		debug("grimdemo_DefaultDisplay: 3D device no longer present.");
		return 0;
	}

	*p3DDevice = threeDDevice;
	return 1;
}

int GrimEngine::chooseDefaultDisplay(int bPreferWindowed, int bWant3D, int *pDevice, int *pMode, int *p3DDevice) {
	StdDisplayDeviceFlags wanted;

	if (!_bUseHardware) {
		bPreferWindowed = 1;
		bWant3D = 0;
	}

	if (bPreferWindowed) {
		wanted.type = 0;
		wanted.bHasHardware = 0;
	} else {
		wanted.type = 1;
		wanted.bHasHardware = 1;
	}

	wanted.bHasGuid = 1;
	wanted.b3D = bWant3D;
	wanted.bPrimary = bWant3D == 0;

	int device = _pStdDisplay->findClosestDevice(&wanted);
	GrimVideoDevice *pVideoDevice = &_aVideoDevices[device];

	*pDevice = device;
	*pMode = _pStdDisplay->findClosestMode(&_videoDeviceConfig, pVideoDevice->aModes, pVideoDevice->numModes);
	*p3DDevice = -1;

#if defined(USE_OPENGL_SHADERS)
	if (pVideoDevice->num3DDevices && bWant3D) {
		Std3DDevice wantedDevice;

		memset(&wantedDevice, 0, sizeof(wantedDevice));
		wantedDevice.bHardware = 1;
		wantedDevice.bPerspectiveCorrect = 1;
		wantedDevice.bZBuffer = 1;
		wantedDevice.colorModelFlags = 2;

		int threeDDevice = _pStd3D->findClosestDevice(&wantedDevice, pVideoDevice->a3DDevices, pVideoDevice->num3DDevices);

		*p3DDevice = threeDDevice;

		if (!pVideoDevice->a3DDevices[threeDDevice].bHardware)
			*p3DDevice = -1;
	}
#endif

	return 1;
}

int GrimEngine::selectDisplayMode(int deviceIndex, int modeIndex, int threeDIndex) {
	shutdownDisplay();

	if (_bDisplayOpen)
		return 0;

	if (deviceIndex < 0 || (uint)deviceIndex >= _pStdDisplay->getNumDevices()) {
		warning("GrimEngine::selectDisplayMode(): invalid display device %d", deviceIndex);
		return 0;
	}

	StdDisplayDevice *pDevice = &_pStdDisplay->getDevices()[deviceIndex];

	if (pDevice->type == 1 && !_bUseHardware) {
		warning("GrimEngine::selectDisplayMode(): the hardware display is not enabled by the renderer setting");
		return 0;
	}

	_pWuRegistry->saveInt("ExperimentalDisplay", 1);

	if (!_pStdDisplay->enumerateModes(deviceIndex))
		return 0;

	StdVBufferTexFmt displayTex;

	if (pDevice->type == 1) {
		if (!_pStdDisplay->openDisplay(modeIndex, nullptr, 1)) {
			_pStdDisplay->closeDisplay();
			return 0;
		}

		displayTex = _pStdDisplay->getDisplayMode()->tex;
		_colorInfoCopy = displayTex.format;
		_numDrawBuffers = _pStdDisplay->getNumDrawBuffers();
	} else {
		Graphics::PixelFormat pixelFormat(2, 5, 6, 5, 0, 11, 5, 0, 0);

		initGraphics(kDisplayWidth, kDisplayHeight, &pixelFormat);

		if (g_system->getScreenFormat() != pixelFormat)
			error("GrimEngine::selectDisplayMode(): the screen does not support the RGB565 pixel format");

		memset(&displayTex, 0, sizeof(displayTex));
		displayTex.width = kDisplayWidth;
		displayTex.height = kDisplayHeight;
		displayTex.pitchInBytes = kDisplayWidth * 2;
		displayTex.pitchInPixels = kDisplayWidth;
		displayTex.sizeInBytes = kDisplayWidth * kDisplayHeight * 2;
		displayTex.format = _colorInfoCopy;

		_pMainGfxBuffer->type = 0;
		_pMainGfxBuffer->lockCount = 0;
		_pMainGfxBuffer->bVideoMemory = 0;
		_pMainGfxBuffer->tex = displayTex;
		_pMainGfxBuffer->pPaletteRGB = nullptr;
		_pMainGfxBuffer->pPixels = new byte[displayTex.sizeInBytes]();
		_pMainGfxBuffer->allocSize = displayTex.sizeInBytes;
		_pMainGfxBuffer->colorKey = 0;
		*_pBackBuffer = *_pMainGfxBuffer;
		_numDrawBuffers = 1;
	}

	_bDisplayOpen = 1;

	_pStdDisplay->vBufferFill(_pBackBuffer, 0, nullptr);
	pageFlipCallback();
	_pStdDisplay->vBufferFill(_pBackBuffer, 0, nullptr);
	_pRd->setColorInfo(_colorInfoCopy);

	StdVBuffer *pZBuffer = nullptr;

	if (select3DDevice(threeDIndex)) {
#if defined(USE_OPENGL_SHADERS)
		pZBuffer = _pStd3D->getZBuffer();
#endif
	} else {
		StdVBufferTexFmt zFormat = displayTex;

		zFormat.format.bpp = 16;
		pZBuffer = _pStdDisplay->vBufferNew(&zFormat, 0, 0, nullptr);
	}

	_pZBuffer = pZBuffer;
	setDisplay(_pBackBuffer, _pZBuffer);
	_pWuRegistry->saveBinary("LastDisplayDevice", pDevice->guid, sizeof(pDevice->guid));
	_bShow = pDevice->type == 0;

	return 1;
}

int GrimEngine::select3DDevice(int index) {
	shutdown3D();

#if defined(USE_OPENGL_SHADERS)
	if (index != -1 && _pStd3D->enumerateDevices()) {
		GrimVideoDevice *pVideoDevice = &_aVideoDevices[getCurVideoDeviceIndex()];

		if (index < 0 || (uint)index >= pVideoDevice->num3DDevices)
			warning("GrimEngine::select3DDevice(): invalid 3D device %d", index);

		if (_pStd3D->startup(index, 1)) {
			_pRd->setAcceleration(_pRd->getAcceleration() | 1);
			using3DHardware(1);
			_pWuRegistry->saveBinary("Last3DDevice", _pStd3D->getCurDevice()->guid, sizeof(_pStd3D->getCurDevice()->guid));

			return 1;
		}
	}
#endif

	byte aZeroGuid[16];

	memset(aZeroGuid, 0, sizeof(aZeroGuid));
	_pWuRegistry->saveBinary("Last3DDevice", aZeroGuid, sizeof(aZeroGuid));

	return 0;
}

void GrimEngine::shutdown3D() {
#if defined(USE_OPENGL_SHADERS)
	if (_pStd3D->isAvailable()) {
		_pStd3D->shutdown();
		_pStd3D->releaseDirect3D();
		_pRd->setAcceleration(_pRd->getAcceleration() & ~1);
		using3DHardware(0);
	}
#endif
}

void GrimEngine::shutdownDisplay() {
	if (!_bDisplayOpen)
		return;

	StdDisplayDevice *pDevice = _pStdDisplay->getCurDevice();
	bool bVideoDevice = pDevice && pDevice->type == 1;

	unsetDisplay();

	if (isHardware3D())
		shutdown3D();
	else
		_pStdDisplay->vBufferFree(_pZBuffer);

	_pZBuffer = nullptr;
	_pStdDisplay->releaseSurfaces();
	_pStdDisplay->closeDisplay();

	if (!bVideoDevice) {
		delete[] _pMainGfxBuffer->pPixels;
		_pMainGfxBuffer->pPixels = nullptr;
		_pBackBuffer->pPixels = nullptr;
	}

	_bDisplayOpen = 0;
	_pWuRegistry->saveInt("ExperimentalDisplay", 0);
}

int GrimEngine::getCurVideoDeviceIndex() {
	return _pStdDisplay->getCurDeviceIndex();
}

int GrimEngine::getCur3DDeviceIndex() {
#if defined(USE_OPENGL_SHADERS)
	if (isHardware3D())
		return _pStd3D->getCurDeviceIndex();
#endif

	return -1;
}

int GrimEngine::isHardware3D() {
	return _pSputRender->is3DHardware();
}

int GrimEngine::paletteCallback(byte *pPalette, int alpha) {
#if defined(USE_OPENGL_SHADERS)
	if (g_grim->isHardware3D())
		g_grim->_pStd3D->setCurrentPalette(pPalette, (byte)alpha);
#endif

	return 1;
}

void GrimEngine::loadSplash() {
	StdBitmapStruct *pBitmap = _pStdBitmap->zbmLoad("splash.bm", 0, 0);

	if (!pBitmap)
		return;

	StdVBuffer *pFrame = pBitmap->apFrames[0];

	if (!pFrame)
		return;

	for (int i = 0; i < pFrame->tex.height; i++)
		_pSputRender->colorCorrect((uint16 *)(pFrame->pPixels + pFrame->tex.pitchInBytes * i), pFrame->tex.width);

	_pStdDisplay->vBufferCopy(_pMainGfxBuffer, pFrame, 0, 0, nullptr, 0);

	if (_pBackBuffer->type == 1)
		_pStdDisplay->vBufferCopy(_pBackBuffer, pFrame, 0, 0, nullptr, 0);

	pageFlipCallback();
	_pStdDisplay->vBufferCopy(_pMainGfxBuffer, pFrame, 0, 0, nullptr, 0);
	_pStdBitmap->unloadBitmap(pBitmap);
}

int GrimEngine::pageFlipCallback() {
	StdDisplayDevice *pDevice = g_grim->_pStdDisplay->getCurDevice();

	if (g_grim->_bDisplayOpen && pDevice && pDevice->type == 1)
		return g_grim->_pStdDisplay->update();

	const StdVBuffer *pBuffer = g_grim->_pMainGfxBuffer;

	if (!pBuffer->pPixels) {
		warning("GrimEngine::pageFlipCallback(): the display is not open");
		return 0;
	}

	g_system->copyRectToScreen(pBuffer->pPixels, pBuffer->tex.pitchInBytes, 0, 0, pBuffer->tex.width, pBuffer->tex.height);
	g_system->updateScreen();

	return 1;
}

static int keyCodeToScanCode(Common::KeyCode keyCode) {
	switch (keyCode) {
	case Common::KEYCODE_ESCAPE:
		return 1;
	case Common::KEYCODE_1:
		return 2;
	case Common::KEYCODE_2:
		return 3;
	case Common::KEYCODE_3:
		return 4;
	case Common::KEYCODE_4:
		return 5;
	case Common::KEYCODE_5:
		return 6;
	case Common::KEYCODE_6:
		return 7;
	case Common::KEYCODE_7:
		return 8;
	case Common::KEYCODE_8:
		return 9;
	case Common::KEYCODE_9:
		return 10;
	case Common::KEYCODE_0:
		return 11;
	case Common::KEYCODE_MINUS:
		return 12;
	case Common::KEYCODE_EQUALS:
		return 13;
	case Common::KEYCODE_BACKSPACE:
		return 14;
	case Common::KEYCODE_TAB:
		return 15;
	case Common::KEYCODE_q:
		return 16;
	case Common::KEYCODE_w:
		return 17;
	case Common::KEYCODE_e:
		return 18;
	case Common::KEYCODE_r:
		return 19;
	case Common::KEYCODE_t:
		return 20;
	case Common::KEYCODE_y:
		return 21;
	case Common::KEYCODE_u:
		return 22;
	case Common::KEYCODE_i:
		return 23;
	case Common::KEYCODE_o:
		return 24;
	case Common::KEYCODE_p:
		return 25;
	case Common::KEYCODE_LEFTBRACKET:
		return 26;
	case Common::KEYCODE_RIGHTBRACKET:
		return 27;
	case Common::KEYCODE_RETURN:
		return 28;
	case Common::KEYCODE_LCTRL:
		return 29;
	case Common::KEYCODE_a:
		return 30;
	case Common::KEYCODE_s:
		return 31;
	case Common::KEYCODE_d:
		return 32;
	case Common::KEYCODE_f:
		return 33;
	case Common::KEYCODE_g:
		return 34;
	case Common::KEYCODE_h:
		return 35;
	case Common::KEYCODE_j:
		return 36;
	case Common::KEYCODE_k:
		return 37;
	case Common::KEYCODE_l:
		return 38;
	case Common::KEYCODE_SEMICOLON:
		return 39;
	case Common::KEYCODE_QUOTE:
		return 40;
	case Common::KEYCODE_BACKQUOTE:
		return 41;
	case Common::KEYCODE_LSHIFT:
		return 42;
	case Common::KEYCODE_BACKSLASH:
		return 43;
	case Common::KEYCODE_z:
		return 44;
	case Common::KEYCODE_x:
		return 45;
	case Common::KEYCODE_c:
		return 46;
	case Common::KEYCODE_v:
		return 47;
	case Common::KEYCODE_b:
		return 48;
	case Common::KEYCODE_n:
		return 49;
	case Common::KEYCODE_m:
		return 50;
	case Common::KEYCODE_COMMA:
		return 51;
	case Common::KEYCODE_PERIOD:
		return 52;
	case Common::KEYCODE_SLASH:
		return 53;
	case Common::KEYCODE_RSHIFT:
		return 54;
	case Common::KEYCODE_KP_MULTIPLY:
		return 55;
	case Common::KEYCODE_LALT:
		return 56;
	case Common::KEYCODE_SPACE:
		return 57;
	case Common::KEYCODE_CAPSLOCK:
		return 58;
	case Common::KEYCODE_F1:
		return 59;
	case Common::KEYCODE_F2:
		return 60;
	case Common::KEYCODE_F3:
		return 61;
	case Common::KEYCODE_F4:
		return 62;
	case Common::KEYCODE_F5:
		return 63;
	case Common::KEYCODE_F6:
		return 64;
	case Common::KEYCODE_F7:
		return 65;
	case Common::KEYCODE_F8:
		return 66;
	case Common::KEYCODE_F9:
		return 67;
	case Common::KEYCODE_F10:
		return 68;
	case Common::KEYCODE_NUMLOCK:
		return 69;
	case Common::KEYCODE_SCROLLOCK:
		return 70;
	case Common::KEYCODE_KP7:
		return 71;
	case Common::KEYCODE_KP8:
		return 72;
	case Common::KEYCODE_KP9:
		return 73;
	case Common::KEYCODE_KP_MINUS:
		return 74;
	case Common::KEYCODE_KP4:
		return 75;
	case Common::KEYCODE_KP5:
		return 76;
	case Common::KEYCODE_KP6:
		return 77;
	case Common::KEYCODE_KP_PLUS:
		return 78;
	case Common::KEYCODE_KP1:
		return 79;
	case Common::KEYCODE_KP2:
		return 80;
	case Common::KEYCODE_KP3:
		return 81;
	case Common::KEYCODE_KP0:
		return 82;
	case Common::KEYCODE_KP_PERIOD:
		return 83;
	case Common::KEYCODE_F11:
		return 87;
	case Common::KEYCODE_F12:
		return 88;
	case Common::KEYCODE_KP_EQUALS:
		return 141;
	case Common::KEYCODE_KP_ENTER:
		return 156;
	case Common::KEYCODE_RCTRL:
		return 157;
	case Common::KEYCODE_KP_DIVIDE:
		return 181;
	case Common::KEYCODE_SYSREQ:
	case Common::KEYCODE_PRINT:
		return 183;
	case Common::KEYCODE_RALT:
		return 184;
	case Common::KEYCODE_PAUSE:
		return 197;
	case Common::KEYCODE_HOME:
		return 199;
	case Common::KEYCODE_UP:
		return 200;
	case Common::KEYCODE_PAGEUP:
		return 201;
	case Common::KEYCODE_LEFT:
		return 203;
	case Common::KEYCODE_RIGHT:
		return 205;
	case Common::KEYCODE_END:
		return 207;
	case Common::KEYCODE_DOWN:
		return 208;
	case Common::KEYCODE_PAGEDOWN:
		return 209;
	case Common::KEYCODE_INSERT:
		return 210;
	case Common::KEYCODE_DELETE:
		return 211;
	case Common::KEYCODE_LSUPER:
	case Common::KEYCODE_LMETA:
		return 219;
	case Common::KEYCODE_RSUPER:
	case Common::KEYCODE_RMETA:
		return 220;
	case Common::KEYCODE_MENU:
		return 221;
	default:
		return 0;
	}
}

void GrimEngine::pumpEvents() {
	Common::EventManager *pEventMan = g_system->getEventManager();
	StdControlInput *pInput = _pStdControl->getInput();
	Common::Event event;

	while (pEventMan->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_QUIT:
		case Common::EVENT_RETURN_TO_LAUNCHER:
			if (!confirmClose()) {
				pEventMan->resetQuit();
				pEventMan->resetReturnToLauncher();
			}

			break;
		case Common::EVENT_FOCUS_GAINED:
			handleFocus(1);
			break;
		case Common::EVENT_FOCUS_LOST:
			handleFocus(0);
			break;
		case Common::EVENT_KEYDOWN:
			handleKey(event.type, event.kbd, event.kbdRepeat);
			break;
		case Common::EVENT_KEYUP:
			handleKey(event.type, event.kbd, false);
			break;
		case Common::EVENT_MOUSEMOVE:
			pInput->mouse.x += event.relMouse.x;
			pInput->mouse.y += event.relMouse.y;
			break;
		case Common::EVENT_WHEELUP:
			pInput->mouse.z += 120;
			break;
		case Common::EVENT_WHEELDOWN:
			pInput->mouse.z -= 120;
			break;
		case Common::EVENT_LBUTTONDOWN:
			addMouseButtonEvent(12, 0x80);
			break;
		case Common::EVENT_LBUTTONUP:
			addMouseButtonEvent(12, 0);
			break;
		case Common::EVENT_RBUTTONDOWN:
			addMouseButtonEvent(13, 0x80);
			break;
		case Common::EVENT_RBUTTONUP:
			addMouseButtonEvent(13, 0);
			break;
		case Common::EVENT_MBUTTONDOWN:
			addMouseButtonEvent(14, 0x80);
			break;
		case Common::EVENT_MBUTTONUP:
			addMouseButtonEvent(14, 0);
			break;
		case Common::EVENT_X1BUTTONDOWN:
			addMouseButtonEvent(15, 0x80);
			break;
		case Common::EVENT_X1BUTTONUP:
			addMouseButtonEvent(15, 0);
			break;
		case Common::EVENT_JOYAXIS_MOTION:
			if (event.joystick.axis < 6)
				pInput->joyState[0].axisPos[event.joystick.axis] = event.joystick.position;

			break;
		case Common::EVENT_JOYBUTTON_DOWN:
			if (event.joystick.button < 10)
				pInput->joyState[0].buttons |= 1 << event.joystick.button;

			break;
		case Common::EVENT_JOYBUTTON_UP:
			if (event.joystick.button < 10)
				pInput->joyState[0].buttons &= ~(1 << event.joystick.button);

			break;
		default:
			break;
		}
	}
}

void GrimEngine::handleFocus(int bActive) {
	if (bActive == _bActive)
		return;

	_bActive = bActive;
	_pStdControl->setActivation(bActive);

	if (bActive) {
		_pSputRender->forceRefresh();

		if (_pStdControl->isOpen())
			_pStdControl->flush();

		CursorMan.showMouse(true);
	}
}

void GrimEngine::handleKey(Common::EventType type, const Common::KeyState &key, bool bRepeat) {
	int scanCode = keyCodeToScanCode(key.keycode);

	if (type == Common::EVENT_KEYUP) {
		if (scanCode)
			addKeyboardEvent(scanCode, 0);

		if (key.keycode == Common::KEYCODE_BACKSPACE)
			_bBackspaceHeld = 0;

		return;
	}

	if (scanCode && !bRepeat)
		addKeyboardEvent(scanCode, 0x80);

	handleKeyDebug(key);

	if (key.ascii > 0 && key.ascii < 256 && key.ascii != 127)
		characterCallback(key.ascii);
}

void GrimEngine::handleKeyDebug(const Common::KeyState &key) {
	switch (key.keycode) {
	case Common::KEYCODE_BACKSPACE:
		if (_bDebugKeys)
			_bBackspaceHeld = 1;

		break;
	case Common::KEYCODE_PAUSE:
		doPauseCallback();
		break;
	case Common::KEYCODE_f:
		if (_bBackspaceHeld)
			_showFps = !_showFps;

		break;
	case Common::KEYCODE_p:
		if (_bBackspaceHeld)
			_bShowDebugOverlay = !_bShowDebugOverlay;

		break;
	case Common::KEYCODE_s:
		if (_bBackspaceHeld)
			_pSputRender->saveDrawBuffer(Common::String::format("shot%d.bmp", _screenshotCounter++).c_str());

		break;
	case Common::KEYCODE_t:
		if (_bBackspaceHeld) {
#if defined(USE_OPENGL_SHADERS)
			_pStd3D->purgeTextureCache();
#endif
		}

		break;
	case Common::KEYCODE_z:
		if (_bBackspaceHeld)
			_pSputRender->saveZBuffer(Common::String::format("shot%d.zbm", _zbufShotCounter++).c_str());

		break;
	case Common::KEYCODE_F12:
		if (_bBackspaceHeld)
			_pLua->dofile("grimpatch.bin");

		break;
	default:
		break;
	}
}

void GrimEngine::addKeyboardEvent(uint offset, int data) {
	StdControlInput *pInput = _pStdControl->getInput();

	pInput->keyboardState[offset] = (byte)data;

	if (pInput->numKeyboardEvents < 32) {
		StdControlDeviceEvent *pEvent = &pInput->keyboardEvents[pInput->numKeyboardEvents];

		pEvent->offset = offset;
		pEvent->data = data;
		pEvent->timeStamp = (int)g_system->getMillis();
		pInput->numKeyboardEvents++;
	} else {
		pInput->bKeyboardOverflow = 1;
	}
}

void GrimEngine::addMouseButtonEvent(uint offset, int data) {
	StdControlInput *pInput = _pStdControl->getInput();

	pInput->mouse.buttons[offset - 12] = (byte)data;

	if (pInput->numMouseEvents < 32) {
		StdControlDeviceEvent *pEvent = &pInput->mouseEvents[pInput->numMouseEvents];

		pEvent->offset = offset;
		pEvent->data = data;
		pEvent->timeStamp = (int)g_system->getMillis();
		pInput->numMouseEvents++;
	} else {
		pInput->bMouseOverflow = 1;
	}
}

} // end of namespace Grim
