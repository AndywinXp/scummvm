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

TopgunEngine_v1_2_3::TopgunEngine_v1_2_3(OSystem *syst, const ADGameDescription *gameDesc) :
	TopgunEngine_v1_2(syst, gameDesc, (Common::String(gameDesc->gameId) == "battlebeast") ? kVersionV1_2_4 : kVersionV1_2_3) {
	setAudioLibrary(new AudioLibrary_v1_2_3(this));
	setResourceLibrary(new ResourceLibrary_v1_2_3(this));
	setSpriteLibrary(new SpriteLibrary_v1_2_3(this));
}

TopgunEngine_v1_2_3::~TopgunEngine_v1_2_3() {
	unloadPluginLibrary();
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

uint16 TopgunEngine_v1_2_3::variableIndex(uint16 operand) const {
	if (operand < TG_HOST_V1_2_3_FIRST_ARGUMENT_SLOT)
		return operand;

	int32 index = (int32)_argumentFrame - (operand - TG_HOST_V1_2_3_FIRST_ARGUMENT_SLOT);
	if (index < 0) {
		warning("TopgunEngine_v1_2_3::variableIndex(): argument slot %d is out of range", operand);
		return 0;
	}

	return (uint16)index;
}

uint16 TopgunEngine_v1_2_3::operandValue(uint16 operand) const {
	if (operand > TG_HOST_V1_2_3_LAST_OPERAND_INDEX)
		return (uint16)(operand + TG_HOST_V1_2_3_LITERAL_BIAS);

	return _variableTable[variableIndex(operand)];
}

uint16 TopgunEngine_v1_2_3::readVariable(uint16 index) const {
	if (index > TG_HOST_V1_2_3_LAST_OPERAND_INDEX) {
		warning("TopgunEngine_v1_2_3::readVariable(): variable %d is out of range", index);
		return 0;
	}

	return _variableTable[variableIndex(index)];
}

void TopgunEngine_v1_2_3::writeVariable(uint16 index, uint16 value) {
	if (index > TG_HOST_V1_2_3_LAST_OPERAND_INDEX) {
		warning("TopgunEngine_v1_2_3::writeVariable(): variable %d is out of range", index);
		return;
	}

	_variableTable[variableIndex(index)] = value;
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
	memcpy(info.header, header, TG_AUDIO_V1_2_3_CREATE_HEADER_SIZE);
	info.readProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE;
	info.rewindProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE;
	info.closeProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE;
	info.handle = waveInfo;

	return audioLib()->audioCreateWave(&info);
}

uint16 TopgunEngine_v1_2_3::audioCreateMidiEvent(const byte *header, uint16 midiInfo) {
	MidiCreateInfo_v1_2_3 info;
	memcpy(info.header, header, TG_AUDIO_V1_2_3_MIDI_HEADER_SIZE);
	info.readProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_MIDI;
	info.closeProc = TG_AUDIO_V1_2_3_PROC_RESOURCE_MIDI;
	info.handle = midiInfo;

	return audioLib()->audioCreateMidi(&info);
}

uint32 TopgunEngine_v1_2_3::audioSourceRead(uint16 proc, uint16 handle, byte *destination, uint32 count) {
	if (proc == TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE || proc == TG_AUDIO_V1_2_3_PROC_RESOURCE_MIDI)
		return (uint32)resourceLib()->waveRead(handle, destination, (int32)count);

	if (proc == TG_AUDIO_V1_2_3_PROC_MOVIE_AUDIO)
		return movieAudioFill(handle, destination, count);

	warning("TopgunEngine_v1_2_3::audioSourceRead(): unknown source procedure %d", proc);
	return 0;
}

uint32 TopgunEngine_v1_2_3::audioSourceRewind(uint16 proc, uint16 handle) {
	if (proc == TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE)
		return resourceLib()->waveRewind(handle);

	return 0;
}

void TopgunEngine_v1_2_3::audioSourceClose(uint16 proc, uint16 handle) {
	if (proc == TG_AUDIO_V1_2_3_PROC_RESOURCE_WAVE || proc == TG_AUDIO_V1_2_3_PROC_RESOURCE_MIDI)
		resourceLib()->waveDone(handle);
}

bool TopgunEngine_v1_2_3::resourceVariablePairCallback(uint16 id, uint16 value) {
	if (_currentSceneIndex >= _sceneEntries.size() || id >= TG_HOST_V1_2_3_NUM_SCENE_VARIABLES) {
		warning("TopgunEngine_v1_2_3::resourceVariablePairCallback(): variable %d is out of range", id);
		return true;
	}

	_sceneEntries[_currentSceneIndex].variables[id] = value;
	return true;
}

void TopgunEngine_v1_2_3::spriteMessageService(byte *record, uint16 spriteId) {
	if (_fatalCode != 0)
		return;

	_variableTable[TG_HOST_V1_2_3_VARIABLE_EVENT_PARAMETER] = operandValue(spriteId);

	switch (READ_LE_UINT16(record)) {
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
	if (getGameId() == "junglegames")
		return runStartup("Timon and Pumbaa's Jungle Games", "JUNGLE.BIN");

	if (getGameId() == "greatword")
		return runStartup("Howie's Great Word Adventure", "WORD.BIN");

	return runStartup("Battle Beast", "BEAST.BIN");
}

bool TopgunEngine_v1_2_3::runStartup(const Common::String &title, const Common::String &firstContainer) {
	if (!initApplication(title))
		return false;

	bool ok = checkDisplayCapabilities();

	if (ok)
		ok = applyLaunchModeSetup();

	if (ok)
		ok = loadInitialScene(firstContainer);

	if (!ok)
		postMessage(kMsgClose, 0, 0);

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
	if (audioLib()->audioInitLibrary(1, TG_AUDIO_V1_2_3_VERSION) == 0)
		return false;

	if (!spriteLib()->spriteInitLibrary())
		return false;

	spriteLib()->spriteSetMessageProc(true);
	spriteLib()->spriteSetMovieProc(true);
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

	return 0;
}

void TopgunEngine_v1_2_3::dispatchMessage(const Message &message) {
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
		if ((message.data & 0xFFFF) != 0)
			enqueuePostedScriptRun(message.data & 0xFFFF, message.param, 0);
		else
			destroyResourceObjectByType(message.param, resourceLib()->resGetType(_file, operandValue(message.param)));
		break;

	default:
		if (message.id >= TG_HOST_V1_2_3_MESSAGE_FIRST_PLUGIN && message.id <= TG_HOST_V1_2_3_MESSAGE_LAST_PLUGIN && _windowMessageCallback)
			_windowMessageCallback->windowMessage(message.id, message.param, (uint16)(message.data & 0xFFFF), (uint16)(message.data >> 16));

		break;
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

	_shuttingDown = true;
	_flagActivated = false;
	_appActive = false;
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

	if (_fatalCode == kMsgV1_2MemoryError || _fatalCode == kMsgV1_2_1IoError)
		showShutdownErrorBox();

	clearMessages();
	postQuitMessage(0);
}

void TopgunEngine_v1_2_3::handleActivateMessage(uint16 param) {
	if (_shuttingDown)
		return;

	bool active = (param != 0);

	if (_version >= kVersionV1_2_4 && _activateScript != 0 && _activateState != active) {
		byte record[18];
		memset(record, 0, sizeof(record));

		WRITE_LE_UINT16(record + 2, _activateScript);
		record[5] = 1;
		WRITE_LE_UINT16(record + 6, (uint16)((active ? 1 : 0) - TG_HOST_V1_2_3_LITERAL_BIAS));
		o_callScript(record);

		_activateState = active;
	}

	if (!_altTab && _file != 0 && _flagActivated != active) {
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
