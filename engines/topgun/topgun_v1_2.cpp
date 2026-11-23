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
#include "common/file.h"
#include "common/path.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/translation.h"

#include "gui/message.h"

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

TopgunEngine_v1_2::TopgunEngine_v1_2(OSystem *syst, const ADGameDescription *gameDesc, int version) :
	TopgunEngine_v1_1(syst, gameDesc, version) {
	if (version < kVersionV1_2_3) {
		setAudioLibrary(new AudioLibrary_v1_2(this));
		setResourceLibrary(new ResourceLibrary_v1_2(this));
		setSpriteLibrary(new SpriteLibrary_v1_2(this));
	}

	memset(_variables, 0, sizeof(_variables));
	memset(_currentSceneName, 0, sizeof(_currentSceneName));
}

TopgunEngine_v1_2::~TopgunEngine_v1_2() {
	destroyCursorResources();
	destroyAllMovieObjects();
	destroyAllTextObjects();

	delete spriteLibrary();
	setSpriteLibrary(nullptr);
	delete audioLibrary();
	setAudioLibrary(nullptr);
	delete resourceLibrary();
	setResourceLibrary(nullptr);
}

bool TopgunEngine_v1_2::postMessage(uint16 id, uint16 param, uint32 data) {
	if (id == kMsgV1_2MemoryError || (_version >= kVersionV1_2_1 && id == kMsgV1_2_1IoError)) {
		// The fatal quit messages are handled synchronously
		flushPostedScriptMessages();

		if (_version >= kVersionV1_2_1)
			_fatalCode = id;

		_scriptsDisabled = true;
		postMessage(kMsgClose, 0, 0);

		if (_version < kVersionV1_2_1)
			_memoryError = true;

		return true;
	}

	return TopgunEngine_v1_1::postMessage(id, param, data);
}

void TopgunEngine_v1_2::resourceLoadProgressCallback() {
	spriteLib()->spriteAnimate();

	if (_loading && _cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);
}

uint16 TopgunEngine_v1_2::audioCreateWave(const byte *header, uint16 waveInfo) {
	SoundCreateInfo_v1_2 info;
	memcpy(info.header, header, TG_AUDIO_V1_2_CREATE_HEADER_SIZE);
	info.readProc = TG_AUDIO_V1_2_PROC_RESOURCE_WAVE;
	info.rewindProc = TG_AUDIO_V1_2_PROC_RESOURCE_WAVE;
	info.closeProc = TG_AUDIO_V1_2_PROC_RESOURCE_WAVE;
	info.handle = waveInfo;

	return audioLib()->audioCreateWave(&info);
}

uint32 TopgunEngine_v1_2::audioSourceRead(uint16 proc, uint16 handle, byte *destination, uint32 count) {
	if (proc == TG_AUDIO_V1_2_PROC_RESOURCE_WAVE)
		return (uint32)resourceLib()->waveRead(handle, destination, (int32)count);

	if (proc == TG_AUDIO_V1_2_PROC_MOVIE_AUDIO)
		return movieAudioFill(handle, destination, count);

	warning("TopgunEngine_v1_2::audioSourceRead(): unknown source procedure %d", proc);
	return 0;
}

uint32 TopgunEngine_v1_2::audioSourceRewind(uint16 proc, uint16 handle) {
	if (proc == TG_AUDIO_V1_2_PROC_RESOURCE_WAVE)
		return resourceLib()->waveRewind(handle);

	return 0;
}

void TopgunEngine_v1_2::audioSourceClose(uint16 proc, uint16 handle) {
	if (proc == TG_AUDIO_V1_2_PROC_RESOURCE_WAVE)
		resourceLib()->waveDone(handle);
}

void TopgunEngine_v1_2::resourceSendMessage(uint16 window, uint16 message) {
	postMessage(message, 0, 0);
}

bool TopgunEngine_v1_2::resourceEnumCallback(uint16 type, uint32 offset, uint32 size) {
	return true;
}

bool TopgunEngine_v1_2::resourceVariableCallback(uint16 id, uint16 value, const char *name) {
	if (_currentSceneIndex >= _sceneEntries.size() || id >= TG_HOST_V1_2_NUM_SCENE_VARIABLES) {
		warning("TopgunEngine_v1_2::resourceVariableCallback(): variable %d is out of range", id);
		return true;
	}

	_sceneEntries[_currentSceneIndex].variables[id] = value;
	return true;
}

void TopgunEngine_v1_2::spriteServiceProc(uint16 word) {
	audioLib()->audioService();
}

byte *TopgunEngine_v1_2::spriteBitmapProvider(uint16 id, bool release) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "spriteBitmapProvider"))
		return nullptr;

	if (release) {
		_records[index].refCount--;
		freeBitmapBlobIfUnreferenced(id);
		return nullptr;
	}

	if (!ensureBitmapResourceLoaded(id))
		return nullptr;

	_records[index].refCount++;
	return _records[index].data;
}

bool TopgunEngine_v1_2::rtStartup(bool runChecks) {
	if (_version >= kVersionV1_2_1) {
		Common::String container = getGameId() + ".bin";

		if (getGameId() == "bestshot") {
			bool smallVersion = Common::File::exists(Common::Path("BESTACT\\bestshot.bin", '\\'));
			bool bigVersion = Common::File::exists(Common::Path("BESTBIG\\bestshot.bin", '\\'));

			if (smallVersion && bigVersion) {
				GUI::MessageDialog dialog(_("Two versions of the game were found. Which one do you want to play?"), "Minimum", "Full");
				container = (dialog.runModal() == GUI::kMessageOK) ? "BESTACT\\bestshot.bin" : "BESTBIG\\bestshot.bin";
			} else if (smallVersion) {
				container = "BESTACT\\bestshot.bin";
			} else if (bigVersion) {
				container = "BESTBIG\\bestshot.bin";
			}
		}

		return runStartup("Take Your Best Shot", container);
	}

	return runStartup("Python", "herald.bin");
}

bool TopgunEngine_v1_2::runStartup(const Common::String &title, const Common::String &firstContainer) {
	bool ok = initApplication(title);

	if (ok)
		ok = checkDisplayCapabilities();

	if (ok)
		ok = applyLaunchModeSetup();

	if (ok) {
		locateResourceFile(firstContainer);
		handlePaintMessage();
		loadInitialScene(Common::Path(firstContainer, '\\').getLastComponent().toString());
	} else {
		Message message;
		message.id = kMsgClose;
		dispatchMessage(message);
	}

	return ok;
}

bool TopgunEngine_v1_2::initApplication(const Common::String &title) {
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

		if (_version >= kVersionV1_2_1)
			_maxJoySticks = getPrivateProfileInt(title.c_str(), "MaxJoySticks", 2);

		_cursorMode = 1;
		utilsLibrary()->memSetNotify(1);

		if (_version >= kVersionV1_2_1)
			joystickInit();
	}

	return ok;
}

uint16 TopgunEngine_v1_2::getPrivateProfileInt(const char *section, const char *key, uint16 defaultValue) {
	loadIniFile();

	Common::String value;
	if (!_iniFile.getKey(key, section, value))
		return defaultValue;

	return parseIniInteger(value.c_str());
}

bool TopgunEngine_v1_2::registerViewClassAndCreateMainWindow() {
	_windowRect = Common::Rect(0, 0, display()->width(), display()->height());

	return loadCursorResources();
}

bool TopgunEngine_v1_2::initEngineLibraries() {
	if (audioLib()->audioInitLibrary(1, TG_AUDIO_V1_2_VERSION) == 0)
		return false;

	if (!spriteLib()->spriteInitLibrary())
		return false;

	spriteLib()->spriteSetMessageProc(true);
	spriteLib()->spriteSetMovieProc(true);
	spriteLib()->spriteSetStreamProc(true);
	spriteLib()->spriteSetServiceProc(true, 1);
	return true;
}

bool TopgunEngine_v1_2::checkDisplayCapabilities() {
	uint16 bitsPerPixel = display()->hasPalette() ? 8 : 32;
	uint16 planes = 1;
	uint16 colors = 1;

	for (uint16 bit = 0; bit < bitsPerPixel; bit++) {
		if (colors >= 0x200)
			break;

		colors <<= 1;
	}

	if (colors <= TG_HOST_V1_2_MIN_COLORS)
		colors *= planes;

	_flagOver256Colors = (colors > TG_HOST_V1_2_MIN_COLORS);

	bool ok = (colors >= TG_HOST_V1_2_MIN_COLORS);
	if (!ok)
		GUIErrorMessage("This program requires 256 colors or more to run.");

	return ok;
}

bool TopgunEngine_v1_2::applyLaunchModeSetup() {
	// The windowed, screen saver and preview launch modes (started by the Pythonizer or the screen saver module) are not supported
	notifyPythonizerWindow();
	return true;
}

void TopgunEngine_v1_2::notifyPythonizerWindow() {
	// The original asks the Pythonizer window whether it is running, so that the game can restart it when it closes: the Pythonizer is not supported
}

bool TopgunEngine_v1_2::locateResourceFile(const Common::String &name) {
	Common::Path path(name, '\\');
	bool found = Common::File::exists(path);

	if (found)
		_resourcePath = path.getParent();
	else
		GUIErrorMessage(Common::String::format("Can't open file '%s'.", name.c_str()));

	return found;
}

uint16 TopgunEngine_v1_2::mainLoop() {
	Message message;

	while (!shouldQuit()) {
		if (peekMessage(message)) {
			if (message.id == kMsgQuit)
				return message.param;

			dispatchMessage(message);
		}

		if (_appActive && !_gamePaused) {
			spriteLib()->spriteAnimate();

			if (_numScheduledTimers != 0)
				serviceScheduledMsgTimerQueue();

			if (_numCollisionPairs != 0)
				serviceCollisionPairs();

			if (_numMovies != 0)
				serviceMovies();

			while (peekPostedMessage(kMsgV1_2RunScript, message))
				dispatchMessage(message);
		}

		presentScreenAndWait();
	}

	handleAppShutdown();

	return 0;
}

bool TopgunEngine_v1_2::peekPostedMessage(uint16 id, Message &message) {
	for (uint16 i = 0; i < _numMsgsPosted; i++) {
		if (_messages[i].id != id)
			continue;

		message = _messages[i];
		_numMsgsPosted--;
		memmove(&_messages[i], &_messages[i + 1], (_numMsgsPosted - i) * sizeof(Message));
		return true;
	}

	return false;
}

void TopgunEngine_v1_2::dispatchMessage(const Message &message) {
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
		handleKeyDownMessage((Common::KeyCode)message.param, message.shift, message.ctrl);
		break;

	case kMsgV1_2Char:
		if (_textEditObjectId != 0)
			handleCharMessage(message.param);
		break;

	case kMsgV1_2_1Joystick1Move:
	case kMsgV1_2_1Joystick2Move:
	case kMsgV1_2_1Joystick1ButtonDown:
	case kMsgV1_2_1Joystick2ButtonDown:
	case kMsgV1_2_1Joystick1ButtonUp:
	case kMsgV1_2_1Joystick2ButtonUp:
		if (_version >= kVersionV1_2_1)
			handleJoystickMessage(message);
		break;

	case kMsgV1_2SysKeyDown:
		if (_version >= kVersionV1_2_1 && message.param == Common::KEYCODE_F10)
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
		switchToNextScene(_gotoRecord.name);
		break;

	case kMsgV1_2RunScript:
		if (message.param != 0)
			handleRunScriptMessage(message.param, (uint16)(message.data & 0xFFFF), (uint16)(message.data >> 16));
		break;

	case kMsgV1_2SoundFinished:
		if ((message.data & 0xFFFF) != 0)
			runScriptResource((uint16)(message.data & 0xFFFF));
		else
			destroyResourceObjectByType(message.param, (_version >= kVersionV1_2_1) ? kResV1_2_1Wave : kResV1_2Wave);
		break;

	case kMsgV1_2Mute:
		audioLib()->audioMute((int16)message.param);
		break;

	case kMsgV1_2CompanionWindow:
		// The Pythonizer registers its window here to be told when the game closes: the Pythonizer is not supported
		break;

	default:
		break;
	}
}

void TopgunEngine_v1_2::handlePaintMessage() {
	validateScreen();

	if (_screenBox)
		fillBackground();

	spriteLib()->spritePaint(0);
}

void TopgunEngine_v1_2::fillBackground() {
	if (_backgroundColor == 0)
		display()->clearScreen();
	else if (display()->hasPalette())
		display()->fillScreen(_solidColorEntry);
	else
		display()->fillScreen(_backgroundColor);
}

void TopgunEngine_v1_2::handleAppShutdown() {
	// The Pythonizer (the desktop program the game can launch) is not told that the game is closing, since it is not supported
	_shuttingDown = true;
	_flagActivated = false;
	_appActive = false;

	if (_version >= kVersionV1_2_1)
		joystickRelease();

	if (_file != 0)
		cleanupCurrentSceneAndContainer();

	flushPostedScriptMessages();
	spriteLib()->spriteExitLibrary();
	audioLib()->audioExitLibrary();

	setCursorMode(1);
	if (!_cursorApplied)
		forceShowOrHideCursor(true);

	setCursorMode(1);

	if (_version >= kVersionV1_2_1) {
		if (_fatalCode != 0)
			showShutdownErrorBox();
	} else if (_memoryError) {
		fillBackground();
		GUIErrorMessage("Not enough memory to run program. Please\nrefer to the trouble shooting instructions.");
	}

	clearMessages();
	postQuitMessage(0);
}

void TopgunEngine_v1_2::showShutdownErrorBox() {
	fillBackground();

	if (_fatalCode == kMsgV1_2_1IoError)
		GUIErrorMessage("A file I/O error has occurred.");
	else
		GUIErrorMessage("Not enough memory to run program. Please\nrefer to the trouble shooting instructions.");
}

void TopgunEngine_v1_2::handleActivateMessage(uint16 param) {
	if (_shuttingDown)
		return;

	bool active = (param != 0);

	if (!_altTab && _file != 0 && _flagActivated != active) {
		if (_flagActivated && !active)
			audioLib()->audioActivate(0);

		_flagActivated = active;

		if (active) {
			audioLib()->audioActivate(1);

			if (_activationScript != 0)
				runScriptResource(_activationScript);

			if (_savedCursorMode != 0)
				_cursorMode = (byte)_savedCursorMode;

			_savedCursorMode = 0;
			applyCursorFromRegister();

			if (_version >= kVersionV1_2_1 && !_joystickCaptured)
				joystickInit();
		} else if (_version >= kVersionV1_2_1) {
			joystickRelease();
		}

		_appActive = (_flagActivated && _file != 0);
	}
}

void TopgunEngine_v1_2::handleSceneEndMessage() {
	bool hasSceneSprite = (_sceneSprite != 0);

	if (_sceneEndScript != 0) {
		runScriptResource(_sceneEndScript);
		spriteLib()->spriteAnimate();
		_sceneEndScript = 0;
	}

	if (hasSceneSprite)
		destroySceneMainObjects(true);
}

void TopgunEngine_v1_2::handleRunScriptMessage(uint16 script, uint16 argument1, uint16 argument2) {
	byte record[9];
	memset(record, 0, sizeof(record));

	WRITE_LE_UINT16(record + 2, script);

	WRITE_LE_UINT16(record + 5, argument1);
	if (argument1 != 0)
		record[4]++;

	WRITE_LE_UINT16(record + 7, argument2);
	if (argument2 != 0)
		record[4]++;

	o_callScript(record);
}

void TopgunEngine_v1_2::flushPostedScriptMessages() {
	for (uint16 id = kMsgV1_2SceneEnd; id <= kMsgV1_2SceneEnd + 3; id++)
		drainMessage(id);

	drainMessage(kMsgV1_2SoundFinished);
	drainMessage(kMsgV1_2MemoryError);

	if (_version >= kVersionV1_2_1)
		drainMessage(kMsgV1_2_1IoError);
}

void TopgunEngine_v1_2::drainMessage(uint16 id) {
	uint16 kept = 0;

	for (uint16 i = 0; i < _numMsgsPosted; i++) {
		if (_messages[i].id != id) {
			_messages[kept] = _messages[i];
			kept++;
		}
	}

	_numMsgsPosted = kept;
}

void TopgunEngine_v1_2::setCursorMode(byte mode) {
	if (_cursorMode == 3 || _cursorMode == 4)
		removeMessages(kMsgLButtonDown);

	_cursorMode = mode;

	if (_cursorApplied)
		applyCursorFromRegister();
}

bool TopgunEngine_v1_2::fatalError(const Common::String &message) {
	_shuttingDown = true;
	_scriptsDisabled = true;
	_appActive = false;

	GUIErrorMessage(message);
	flushPostedScriptMessages();
	postMessage(kMsgClose, 0, 0);
	return false;
}

bool TopgunEngine_v1_2::fatalError(uint16 code, const Common::String &message) {
	// The later original marks the shutdown as already started, so the close message does nothing and the window stays frozen; here the close runs the normal shutdown
	if (_version < kVersionV1_2_3)
		_shuttingDown = true;

	_fatalCode = code;
	_scriptsDisabled = true;
	_appActive = false;

	GUIErrorMessage(message);
	flushPostedScriptMessages();
	postMessage(kMsgClose, 0, 0);
	return false;
}

} // End of namespace Topgun
