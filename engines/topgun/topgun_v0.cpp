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
#include "common/file.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/screen.h"

#include "topgun/display.h"
#include "topgun/libraries/audiolibrary.h"
#include "topgun/libraries/spritelibrary.h"
#include "topgun/libraries/utilslibrary.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

static const char *const kHeraldFileName = "herald.bin";

TopgunEngine_v0::TopgunEngine_v0(OSystem *syst, const ADGameDescription *gameDesc) :
	TopgunEngine(syst, gameDesc) {
	memset(_globalFlags, 0, sizeof(_globalFlags));
	memset(_tileOrder, 0, sizeof(_tileOrder));
}

TopgunEngine_v0::~TopgunEngine_v0() {
}

bool TopgunEngine_v0::rtStartup(bool runChecks) {
	setCursorImage(false);

	bool ok = initEngineLibraries();

	if (ok) {
		_screenBox = true;
		display()->setHasPalette(!(ConfMan.hasKey("high_bit_depth") && ConfMan.getBool("high_bit_depth")));
		_flagMode640 = (display()->width() <= Display::kDefaultWidth);

		if (runChecks) {
			ok = locateResourceFile();

			if (ok) {
				utilsLibrary()->memSetNotify();
				_appActive = true;
				invalidateScreen();
				handlePaintMessage();
				loadInitialScene(kHeraldFileName, false);
			}
		}
	}

	if (!ok) {
		Message message;
		message.id = kMsgClose;
		dispatchMessage(message);
	}

	return ok;
}

uint16 TopgunEngine_v0::mainLoop() {
	Message message;

	while (!shouldQuit()) {
		if (_inputBusy) {
			_inputBusy = false;
			applyCursorFromRegister();
			removeMessages(kMsgLButtonDown);
		}

		if (peekMessage(message)) {
			if (message.id == kMsgQuit)
				return message.param;

			dispatchMessage(message);
		}

		if (_appActive) {
			audioLibrary()->audioService();
			spriteLibrary()->spriteAnimate();

			if (_numTimers != 0)
				serviceScheduledMsgTimerQueue();
		}

		presentScreenAndWait();
	}

	handleAppShutdown(false);

	return 0;
}

void TopgunEngine_v0::presentScreenAndWait() {
	if (screen()->isDirty())
		screen()->update();

	g_system->delayMillis(kLoopDelay);
}

void TopgunEngine_v0::pauseEngineIntern(bool pause) {
	Engine::pauseEngineIntern(pause);

	postMessage(kMsgActivate, pause ? 0 : 1, 0);
}

void TopgunEngine_v0::requestClose() {
	postMessage(kMsgClose, 0, 0);
}

void TopgunEngine_v0::postMemoryError() {
	postMessage(kMsgMemoryError, 0, 0);
}

bool TopgunEngine_v0::postMessage(uint16 id, uint16 param, uint32 data) {
	if (_numMsgsPosted >= kMaxPostedMessages)
		return false;

	Message &message = _messages[_numMsgsPosted];
	message.id = id;
	message.param = param;
	message.data = data;
	message.shift = false;
	message.ctrl = false;
	_numMsgsPosted++;
	return true;
}

bool TopgunEngine_v0::peekMessage(Message &message) {
	if (_numMsgsPosted > 0) {
		message = _messages[0];
		_numMsgsPosted--;
		memmove(&_messages[0], &_messages[1], _numMsgsPosted * sizeof(Message));
		return true;
	}

	if (!_shuttingDown && peekInputMessage(message))
		return true;

	if (_paintPending) {
		message.id = kMsgPaint;
		message.param = 0;
		message.data = 0;
		message.shift = false;
		message.ctrl = false;
		_paintPending = false;
		return true;
	}

	if (_quit) {
		message.id = kMsgQuit;
		message.param = _quitCode;
		message.data = 0;
		message.shift = false;
		message.ctrl = false;
		_quit = false;
		return true;
	}

	return false;
}

void TopgunEngine_v0::removeMessages(uint16 id) {
	uint16 kept = 0;

	for (uint16 i = 0; i < _numMsgsPosted; i++) {
		if (id != 0 && _messages[i].id != id) {
			_messages[kept] = _messages[i];
			kept++;
		}
	}

	_numMsgsPosted = kept;

	discardPendingInput(id);
}

void TopgunEngine_v0::clearMessages() {
	removeMessages(0);
	_paintPending = false;
}

void TopgunEngine_v0::invalidateScreen() {
	_paintPending = true;
}

void TopgunEngine_v0::validateScreen() {
	_paintPending = false;
}

void TopgunEngine_v0::postQuitMessage(uint16 exitCode) {
	_quit = true;
	_quitCode = exitCode;
}

void TopgunEngine_v0::dispatchMessage(const Message &message) {
	if (message.id == kMsgLButtonDown || message.id == kMsgRButtonDown) {
		handleMouseButtonDown((int16)(message.data & 0xFFFF), (int16)(message.data >> 16));
	} else if (message.id == kMsgActivate) {
		handleActivateMessage(message.param);
	} else if (message.id == kMsgKeyDown) {
		handleKeyDownMessage((Common::KeyCode)message.param, message.shift, message.ctrl);
	} else if (message.id == kMsgPaint) {
		handlePaintMessage();
	} else if (message.id == kMsgClose) {
		handleAppShutdown(false);
	} else if (message.id == kMsgSceneEnd) {
		handleSceneEndMessage();
	} else if (message.id == kMsgGotoScene) {
		switchToNextScene(_gotoRecord.name);
	} else if (message.id == kMsgRunScript) {
		if (message.param != 0)
			runScriptResource(message.param);
	} else if (message.id == kMsgMemoryError) {
		handleAppShutdown(true);
	}
}

void TopgunEngine_v0::resourceLoadProgressCallback() {
	spriteLibrary()->spriteAnimate();
	audioLibrary()->audioService();

	if (_active)
		advanceSceneFadeStep();

	if (_loading && _cdAudioPlaying) {
		audioLibrary()->audioCommand(0x73, 0, 0, nullptr);
		_cdAudioPlaying = false;
	}
}

void TopgunEngine_v0::clearInputMessageQueues() {
	removeMessages(kMsgLButtonDown);
	removeMessages(kMsgKeyDown);
}

void TopgunEngine_v0::showFormattedErrorMessageBox(uint16 stringId, const char *argument) {
	static const char *const kErrorNotEnoughMemory = "Not enough memory to run TuneLand. Please\nrefer to the trouble shooting instructions.";
	static const char *const kErrorNeed256Colors = "TuneLand requires 256 colors or more to run.";
	static const char *const kErrorCannotOpenFile = "Can't open file '%s'.";

	Common::String text;
	if (stringId == 100) {
		text = kErrorNotEnoughMemory;
	} else if (stringId == 101) {
		text = kErrorNeed256Colors;
	} else if (stringId == 102) {
		if (argument)
			text = Common::String::format("Can't open file '%s'.", argument);
		else
			text = kErrorCannotOpenFile;
	} else {
		warning("TopgunEngine_v0::showFormattedErrorMessageBox(): unknown string id %d", stringId);
	}

	GUIErrorMessage(text);
}

bool TopgunEngine_v0::initEngineLibraries() {
	if (!spriteLibrary()->spriteInitLibrary())
		return false;

	if (audioLibrary()->audioInitLibrary(0x100) == 0)
		return false;

	spriteLibrary()->spriteSetMessageProc();
	spriteLibrary()->spriteSetServiceProc();
	return true;
}

bool TopgunEngine_v0::locateResourceFile() {
	if (Common::File::exists(kHeraldFileName))
		return true;

	showFormattedErrorMessageBox(102, kHeraldFileName);
	return false;
}

void TopgunEngine_v0::handleActivateMessage(uint16 param) {
	bool active = (param != 0);

	if (_file != 0 && _appActive != active) {
		if (_appActive && !active)
			audioLibrary()->audioActivate(0);

		_globalFlags[6] = active ? 1 : 0;
		_appActive = active;

		if (active) {
			audioLibrary()->audioActivate(1);

			if (_activationScript != 0)
				runScriptResource(_activationScript);

			applyCursorFromRegister();
		}
	}
}

void TopgunEngine_v0::handleAppShutdown(bool memoryError) {
	_shuttingDown = true;
	_appActive = false;

	if (_file != 0)
		cleanupCurrentSceneAndContainer();

	spriteLibrary()->spriteExitLibrary();
	audioLibrary()->audioExitLibrary();

	if (memoryError) {
		fillBackground();
		showFormattedErrorMessageBox(100, nullptr);
	}

	clearMessages();
	postQuitMessage(0);
}

void TopgunEngine_v0::handlePaintMessage() {
	validateScreen();

	if (_screenBox)
		fillBackground();

	spriteLibrary()->spritePaint(0);
}

void TopgunEngine_v0::handleSceneEndMessage() {
	if (_sceneEndScript != 0) {
		runScriptResource(_sceneEndScript);
		spriteLibrary()->spriteAnimate();
		_sceneEndScript = 0;
	}

	destroySceneMainObjects();
}

void TopgunEngine_v0::fillBackground() {
	screen()->clear();
}

} // End of namespace Topgun
