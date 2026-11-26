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

#include "common/config-manager.h"
#include "common/endian.h"
#include "common/textconsole.h"

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

TopgunEngine_v1_2_3::TopgunEngine_v1_2_3(OSystem *syst, const ADGameDescription *gameDesc, EngineVersion version) :
	TopgunEngine_v1_2(syst, gameDesc, version) {
	setAudioLibrary(new AudioLibrary_v1_2_3(this));
	setResourceLibrary(new ResourceLibrary_v1_2_3(this));
	setSpriteLibrary(new SpriteLibrary_v1_2_3(this));

	if (_version >= kVersionV1_2_5) {
		_keyboardSetKeys[0][6] = 0x33;
		_keyboardSetKeys[0][7] = 0x34;
		_keyboardSetKeys[1][6] = 0xBD;
		_keyboardSetKeys[1][7] = 0xBB;

		_keyboardSetCodes[0][6] = 0x04;
		_keyboardSetCodes[0][7] = 0x08;
		_keyboardSetCodes[1][6] = 0x04;
		_keyboardSetCodes[1][7] = 0x08;
	}
}

TopgunEngine_v1_2_3::~TopgunEngine_v1_2_3() {
	unloadPluginLibrary();
	unloadAllPluginModules();
	freeUnloadedPluginModules();

	for (uint16 i = 0; i < _pluginModules.size(); i++) {
		delete _pluginModules[i]->plugin;
		delete _pluginModules[i];
	}

	_pluginModules.clear();

	delete _pluginHost;
	_pluginHost = nullptr;

	destroyAllMovieObjects();
	destroyAllTextObjects();

	delete spriteLibrary();
	setSpriteLibrary(nullptr);
	delete audioLibrary();
	setAudioLibrary(nullptr);
	delete resourceLibrary();
	setResourceLibrary(nullptr);
}

void TopgunEngine_v1_2_3::syncSoundSettings() {
	TopgunEngine_v1_2::syncSoundSettings();

	audioLib()->applyMidiDeviceVolume();
}

bool TopgunEngine_v1_2_3::postMessage(uint16 id, uint16 param, uint32 data) {
	if (id == kMsgV1_2MemoryError || id == kMsgV1_2_1IoError) {
		// The fatal quit messages are handled synchronously
		flushPostedScriptMessages();
		_fatalCode = id;
		_scriptsDisabled = true;
		postMessage(kMsgClose, 0, 0);
		return true;
	}

	return TopgunEngine_v1_2::postMessage(id, param, data);
}

uint16 TopgunEngine_v1_2_3::audioCreateWaveEvent(const byte *header, uint16 waveInfo) {
	SoundCreateInfo_v1_2 info;

	uint16 headerSize = TG_AUDIO_V1_2_3_CREATE_HEADER_SIZE;
	if (_version < kVersionV1_2_3)
		headerSize = TG_AUDIO_V1_2_2_2_CREATE_HEADER_SIZE;

	memcpy(info.header, header, headerSize);
	info.readProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE;
	info.rewindProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE;
	info.closeProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE;
	info.handle = waveInfo;

	return audioLib()->audioCreateWave(&info);
}

void TopgunEngine_v1_2_3::spriteMessageService(byte *record, uint16 spriteId) {
	if (_fatalCode != 0)
		return;

	_variableTable[TG_HOST_V1_2_3_VARIABLE_EVENT_PARAMETER] = operandValue(spriteId);

	uint16 message = READ_LE_UINT16(record);
	if (_version >= kVersionV1_2_6)
		message--;

	switch (message) {
	case 0x12:
		trimSpriteQueueBitmaps(READ_LE_UINT16(record + 2));
		break;

	case 0x13:
		postMessage(kMsgV1_2SceneEnd, 0, 0);
		break;

	case 0x14:
		executeScriptOp(record + 4, nullptr);
		break;

	case 0x15:
		runPostedScriptRecord(record);
		break;

	default:
		break;
	}
}

bool TopgunEngine_v1_2_3::rtStartup(bool runChecks) {
	if (_gameDescription->flags & ADGF_DEMO) {
		const char *title;
		const char *container;
		const char *folder;

		if (getGameId() == "coldblooded") {
			title = "Cold Blooded Sampler";
			container = "COLDDEMO.BIN";
			folder = "CBLOODED";
		} else if (getGameId() == "dominion") {
			title = "Dominion Sampler";
			container = "SCRNDEMO.BIN";
			folder = "DOMINION";
		} else if (getGameId() == "holygrail") {
			title = "Holy Grail Sampler";
			container = "GRLDEMO.BIN";
			folder = "GRAIL";
		} else if (getGameId() == "bestshot") {
			title = "TopGun Player";
			container = "BESTSHOT.BIN";
			folder = "BESTSHOT";
		} else if (getGameId() == "7thlevelsamplers") {
			title = "Battle Beast";
			container = "DEMO.BIN";
			folder = "DEMO";
		} else if (getGameId() == "arcadeamerica") {
			title = "Arcade America";
			container = "ARCADE.BIN";
			folder = "ARCADE";
		} else {
			title = "TopGun Player";
			container = "ACE_DEMO.BIN";
			folder = "ACE";
		}

		addDemoDirectory(folder);

		return runStartup(title, container);
	}

	if (getGameId() == "arcadeamerica")
		return runStartup("Arcade America", "ARCADE.BIN");

	if (getGameId() == "holygrail")
		return runStartup("Quest For The Holy Grail", "GRAIL.BIN");

	if (getGameId() == "tuneland")
		return runStartup("TuneLand", "TUNELAND.BIN");

	if (getGameId() == "aceventura")
		return runStartup("Ace Ventura", "ACE.BIN");

	if (getGameId() == "junglegames" && _version >= kVersionV1_2_8)
		return runStartup("Pumbaa And Timon's Jungle Arcade", "JUNGLE.BIN");

	if (getGameId() == "junglegames")
		return runStartup("Timon and Pumbaa's Jungle Games", "JUNGLE.BIN");

	if (getGameId() == "greatword")
		return runStartup("Howie's Great Word Adventure", "WORD.BIN");

	return runStartup("Battle Beast", "BEAST.BIN");
}

bool TopgunEngine_v1_2_3::runStartup(const Common::String &title, const Common::String &firstContainer) {
	if (!initApplication(title)) {
		warning("TopgunEngine_v1_2_3::runStartup(): cannot initialize the application '%s'", title.c_str());
		return false;
	}

	bool ok = checkDisplayCapabilities();

	if (ok)
		ok = applyLaunchModeSetup();

	if (ok)
		ok = loadInitialScene(firstContainer);

	if (!ok) {
		warning("TopgunEngine_v1_2_3::runStartup(): startup of '%s' failed", title.c_str());
		postMessage(kMsgClose, 0, 0);
	}

	return true;
}

bool TopgunEngine_v1_2_3::initApplication(const Common::String &title) {
	_appTitle = title;

	bool ok = registerViewClassAndCreateMainWindow();

	if (ok)
		ok = initEngineLibraries();

	if (ok) {
		_audioPresent = (byte)audioLib()->audioGetWaveDeviceInfo(nullptr);
		_screenBox = true;
		display()->setHasPalette(!(ConfMan.hasKey("high_bit_depth") && ConfMan.getBool("high_bit_depth")));
		_flagMode640 = (display()->width() <= TG_DISPLAY_DEFAULT_WIDTH);
		_altTab = ConfMan.hasKey("alttab") && ConfMan.getInt("alttab") != 0;
		if (_version >= kVersionV1_2_2_3)
			_debugMode = ((byte)getPrivateProfileInt(title.c_str(), "Debug", 0) != 0);

		_maxJoySticks = getPrivateProfileInt(title.c_str(), "MaxJoySticks", 2);
		_cursorMode = 1;
		utilsLibrary()->memSetNotify(1);
		joystickInit();
		_inputTimeout = getPrivateProfileInt("Input", "Timeout", 200);
	}

	return ok;
}

bool TopgunEngine_v1_2_3::registerViewClassAndCreateMainWindow() {
	_windowRect = Common::Rect(0, 0, display()->width(), display()->height());

	display()->setWindowOrigin(-1, -1);

	return loadCursorResources();
}

bool TopgunEngine_v1_2_3::initEngineLibraries() {
	if (audioLib()->audioInitLibrary(1, TG_AUDIO_V1_2_3_VERSION) == 0) {
		warning("TopgunEngine_v1_2_3::initEngineLibraries(): cannot initialize the audio library (version 0x%X)", TG_AUDIO_V1_2_3_VERSION);
		return false;
	}

	if (!spriteLib()->spriteInitLibrary()) {
		warning("TopgunEngine_v1_2_3::initEngineLibraries(): cannot initialize the sprite library");
		return false;
	}

	spriteLib()->spriteSetMessageProc(true);
	spriteLib()->spriteSetMovieProc(true);

	if (_version == kVersionV1_2_2_3)
		spriteLib()->spriteSetQueueDoneProc(false);

	spriteLib()->spriteSetStreamProc(true);
	spriteLib()->spriteSetServiceProc(true, 1);
	return true;
}

uint16 TopgunEngine_v1_2_3::mainLoop() {
	Message message;

	while (!shouldQuit()) {
		if (peekMessage(message)) {
			if (message.id == kMsgQuit)
				return message.param;

			dispatchMessage(message);
		}

		if (_appActive && !_gamePaused) {
			spriteLib()->spriteAnimate();

			if (_version >= kVersionV1_2_9 && _pluginTickProcCount != 0)
				runRegisteredServiceCallbacks();

			if (_version >= kVersionV1_2_6_2 && _idleScript != 0)
				idleCheck();

			if (_numScheduledTimers != 0)
				serviceScheduledMsgTimerQueue();

			if (_numNativeTimers != 0)
				serviceNativeTimerQueue();

			if (_numCollisionPairs != 0)
				serviceCollisionPairs();

			if (_numMovies != 0)
				serviceMovies();

			if (_numPostedScriptRuns != 0)
				servicePostedRunQueue();
		}

		presentScreenAndWait();
	}

	handleAppShutdown();

	// The first call unloads the plug-ins and only posts the close message, which is not dispatched after the loop
	if (_version >= kVersionV1_2_9 && !_shuttingDown)
		handleAppShutdown();

	return 0;
}

void TopgunEngine_v1_2_3::dispatchMessage(const Message &message) {
	if (_version >= kVersionV1_2_6_2 && _idleScript != 0) {
		switch (message.id) {
		case kMsgKeyDown:
		case kMsgV1_2_3KeyUp:
		case kMsgV1_2Char:
		case kMsgV1_2SysKeyDown:
		case kMsgV1_2MouseMove:
		case kMsgLButtonDown:
		case kMsgRButtonDown:
		case kMsgV1_2LButtonUp:
		case kMsgV1_2RButtonUp:
			idleRearm();
			break;

		default:
			break;
		}
	}

	switch (message.id) {
	case kMsgPaint:
		handlePaintMessage();
		break;

	case kMsgClose:
		handleAppShutdown();
		break;

	case kMsgV1_2ActivateApp:
		if (_file != 0)
			handleActivateMessage(message.param);
		break;

	case kMsgKeyDown:
		if (_version >= kVersionV1_2_7 && _textEditObjectId != 0 && message.param == Common::KEYCODE_DELETE)
			handleDeleteKeyMessage();
		else
			handleKeyDownMessage((Common::KeyCode)message.param, message.shift, message.ctrl);

		break;

	case kMsgV1_2_3KeyUp:
		handleKeyUpMessage((Common::KeyCode)message.param);
		break;

	case kMsgV1_2_1Joystick1Move:
	case kMsgV1_2_1Joystick2Move:
	case kMsgV1_2_1Joystick1ButtonDown:
	case kMsgV1_2_1Joystick2ButtonDown:
	case kMsgV1_2_1Joystick1ButtonUp:
	case kMsgV1_2_1Joystick2ButtonUp:
		if (!_gamePaused)
			handleJoystickMessage(message);
		break;

	case kMsgV1_2Char:
		if (_textEditObjectId != 0)
			handleCharMessage(message.param);
		break;

	case kMsgV1_2SysKeyDown:
		if (message.param == Common::KEYCODE_F10)
			handleKeyDownMessage((Common::KeyCode)message.param, message.shift, message.ctrl);
		else
			handleSysKeyDownAltTab(message);
		break;

	case kMsgV1_2MouseMove:
		handleMouseMoveMessage(message);
		break;

	case kMsgLButtonDown:
	case kMsgRButtonDown:
		handleMouseButtonMessage(message);
		break;

	case kMsgV1_2LButtonUp:
	case kMsgV1_2RButtonUp:
		handleMouseButtonUpMessage(message);
		break;

	case kMsgV1_2SceneEnd:
		handleSceneEndMessage();
		break;

	case kMsgV1_2GotoScene:
		switchToNextScene(_gotoSceneName);
		break;

	case kMsgV1_2SoundFinished:
		if ((message.data & 0xFFFF) == 0) {
			destroyResourceObjectByType(message.param, resourceLib()->resGetType(_file, operandValue(message.param)));
		} else if (_version >= kVersionV1_2_3) {
			enqueuePostedScriptRun(message.data & 0xFFFF, message.param, 0);
		} else {
			byte record[18];

			memset(record, 0, sizeof(record));
			WRITE_LE_UINT16(record + 2, message.data & 0xFFFF);
			record[5] = 1;
			WRITE_LE_UINT16(record + 6, message.param);
			o_callScript(record);
		}

		break;

	case kMsgV1_2_9FreePluginModules:
		if (_version >= kVersionV1_2_9)
			freeUnloadedPluginModules();
		break;

	default: {
		uint16 firstPluginMessage = TG_HOST_V1_2_3_MESSAGE_FIRST_PLUGIN;
		uint16 lastPluginMessage = TG_HOST_V1_2_3_MESSAGE_LAST_PLUGIN;

		if (_version >= kVersionV1_2_7) {
			firstPluginMessage = TG_HOST_V1_2_7_MESSAGE_FIRST_PLUGIN;
			lastPluginMessage = TG_HOST_V1_2_7_MESSAGE_LAST_PLUGIN;
		}

		if (_version >= kVersionV1_2_2_3 && message.id >= firstPluginMessage && message.id <= lastPluginMessage && _windowMessageCallback)
			_windowMessageCallback->windowMessage(message.id, message.param, (uint16)(message.data & 0xFFFF), (uint16)(message.data >> 16));

		break;
	}
	}
}

void TopgunEngine_v1_2_3::fillBackground() {
	if (_backgroundColor == 0) {
		display()->clearScreen();
	} else if (display()->hasPalette()) {
		display()->fillScreen(_backgroundColor);
	} else if (_backgroundColor < TG_DISPLAY_NUM_PALETTE_ENTRIES) {
		const byte *entry = _palette + _backgroundColor * TG_DISPLAY_PALETTE_ENTRY_SIZE;
		display()->fillScreen(entry[0] | (entry[1] << 8) | (entry[2] << 16));
	} else {
		warning("TopgunEngine_v1_2_3::fillBackground(): palette index %d is out of range", _backgroundColor);
	}
}

void TopgunEngine_v1_2_3::handleAppShutdown() {
	if (_shuttingDown)
		return;

	if (_version >= kVersionV1_2_9 && !_pluginLibraries.empty()) {
		unloadAllPluginModules();
		postMessage(kMsgClose, 0, 0);
		return;
	}

	_shuttingDown = true;
	_flagActivated = false;
	_appActive = false;
	joystickRelease();

	if (_file != 0)
		cleanupCurrentSceneAndContainer();

	flushPostedScriptMessages();

	if (_version >= kVersionV1_2_9)
		freeUnloadedPluginModules();

	spriteLib()->spriteExitLibrary();
	audioLib()->audioExitLibrary();

	setCursorMode(1);
	if (!_cursorApplied)
		forceShowOrHideCursor(true);

	setCursorMode(1);

	if (_fatalCode == kMsgV1_2MemoryError || _fatalCode == kMsgV1_2_1IoError)
		showShutdownErrorBox();

	clearMessages();
	postQuitMessage(0);
}

void TopgunEngine_v1_2_3::handleActivateMessage(uint16 param) {
	if (_shuttingDown)
		return;

	bool active = (param != 0);

	bool notify = false;
	if (_activateState != active) {
		if (_version >= kVersionV1_2_6)
			notify = true;
		else if (_version >= kVersionV1_2_4 && _activateScript != 0)
			notify = true;
	}

	if (notify) {
		if (_version >= kVersionV1_2_9)
			notifyPluginsFocusChanged(active);

		if (_version >= kVersionV1_2_6) {
			releaseOrRestoreDeviceGroups(!active);

			if (_file != 0)
				audioLib()->audioActivate(active ? 1 : 0);
		}

		if (_activateScript != 0) {
			byte record[18];
			memset(record, 0, sizeof(record));

			WRITE_LE_UINT16(record + 2, _activateScript);
			record[5] = 1;
			WRITE_LE_UINT16(record + 6, (uint16)((active ? 1 : 0) - TG_HOST_V1_2_3_LITERAL_BIAS));
			o_callScript(record);
		}

		_activateState = active;

		if (_version >= kVersionV1_2_6_1 && _version <= kVersionV1_2_8 && !active)
			cancelMouseCapture();
	}

	if (!_altTab && _file != 0 && _flagActivated != active) {
		if (_version < kVersionV1_2_6)
			audioLib()->audioActivate(active ? 1 : 0);

		_flagActivated = active;

		if (active) {
			if (_activationScript != 0)
				runScriptResource(_activationScript);

			if (_savedCursorMode != 0)
				_cursorMode = (byte)_savedCursorMode;

			_savedCursorMode = 0;
			applyCursorFromRegister();

			if (!_joystickCaptured)
				joystickInit();
		} else {
			joystickRelease();
		}

		_appActive = (_flagActivated && _file != 0);
	}
}

void TopgunEngine_v1_2_3::flushPostedScriptMessages() {
	for (uint16 id = TG_HOST_V1_2_3_MESSAGE_FIRST_FLUSHED; id <= TG_HOST_V1_2_3_MESSAGE_LAST_FLUSHED; id++)
		drainMessage(id);

	drainMessage(kMsgV1_2SoundFinished);
	drainMessage(kMsgV1_2MemoryError);
	drainMessage(kMsgV1_2_1IoError);
}

} // End of namespace Topgun
