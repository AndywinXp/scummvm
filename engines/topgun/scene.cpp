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
#include "common/path.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "graphics/cursorman.h"

#include "topgun/display.h"
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v2_0.h"

namespace Topgun {

SceneRecord *TopgunEngine_v1_0::findOrCreateSceneEntry(const Common::String &name) {
	for (uint16 i = 0; i < _numScenes; i++) {
		if (name.equalsIgnoreCase(_scenes[i].name))
			return &_scenes[i];
	}

	if (_numScenes >= TG_HOST_MAX_SCENES)
		return nullptr;

	SceneRecord &scene = _scenes[_numScenes];
	uint32 length = name.size();

	if (length >= TG_HOST_SCENE_NAME_SIZE) {
		warning("TopgunEngine_v1_0::findOrCreateSceneEntry(): scene name is too long");
		length = TG_HOST_SCENE_NAME_SIZE - 1;
	}

	memcpy(scene.name, name.c_str(), length);
	scene.name[length] = '\0';
	memset(scene.flags, 0, sizeof(scene.flags));
	_numScenes++;
	return &scene;
}

bool TopgunEngine_v1_0::loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback) {
	_currentScene = findOrCreateSceneEntry(name);
	_keyScripts.clear();
	_activationScript = 0;

	if (!openResourceFileAndInit(name)) {
		GUIErrorMessage(Common::String::format("Can't open file '%s'.", name.c_str()));
		postMessage(kMsgClose, 0, 0);
		return false;
	}

	if (withProgressCallback)
		resourceLibrary()->resSetCallback(_file, true, TG_HOST_PROGRESS_INTERVAL);

	byte palette[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];
	memset(palette, 0, sizeof(palette));
	refreshGamePalette(palette);

	runScriptResource(0);

	resourceLibrary()->resSetCallback(_file, false, 0);
	return true;
}

void TopgunEngine_v1_0::switchToNextScene(const char *name) {
	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	clearBackground();
	spriteLibrary()->spriteSetTopMost(_sceneSprite);

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(true);

	sceneTransitionOut();
	resetResourcesAndTransientState();
	spriteLibrary()->spriteSetBackground(nullptr, nullptr);

	if (_buffer2)
		_buffer2 = utilsLibrary()->tgFree(_buffer2);

	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	if (_file != 0)
		resourceLibrary()->resCloseFile(_file);

	_file = 0;

	loadSceneContainerAndRunEntryScript(name, true);

	if (_file == 0) {
		cleanupCurrentSceneAndContainer();
	} else {
		if (_sceneSprite != 0)
			spriteLibrary()->spriteBreakLoops(_sceneSprite);

		sceneTransitionIn();
	}

	clearInputMessageQueues();

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(false);
}

bool TopgunEngine_v1_0::loadInitialScene(const Common::String &name) {
	_gotoRecord = GotoSceneRecord();
	memset(_globalFlags, 0, sizeof(_globalFlags));

	_globalFlags[8] = (byte)(ConfMan.hasKey("howie") ? ConfMan.getInt("howie") : 1);

	if (ConfMan.hasKey("mode640"))
		_flagMode640 = ((byte)ConfMan.getInt("mode640") != 0);

	if (ConfMan.hasKey("lomem"))
		_flagLoMem = ((byte)ConfMan.getInt("lomem") != 0);

	_globalFlags[0] = _flagLoMem ? 1 : 0;
	_gotoRecord.releaseSceneSprite = 1;
	_numScenes = 0;
	_sceneEndScript = 0;
	_faded = true;
	_appActive = true;
	_screenBox = true;
	_cursorEnabled = true;

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(true);

	loadSceneContainerAndRunEntryScript(name, false);
	sceneTransitionIn();

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(false);

	return true;
}

void TopgunEngine_v1_0::cleanupCurrentSceneAndContainer() {
	resetResourcesAndTransientState();

	if (_cdAudioPlaying)
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);

	_cdAudioPlaying = false;

	if (_buffer1)
		_buffer1 = utilsLibrary()->tgFree(_buffer1);

	if (_file != 0)
		resourceLibrary()->resCloseFile(_file);

	_file = 0;
	destroySceneMainObjects();

	if (!_cursorEnabled)
		CursorMan.showMouse(true);

	_cursorEnabled = true;
}

SceneEntry_v1_2 *TopgunEngine_v1_2::findOrCreateSceneEntry(const Common::String &name) {
	Common::String sceneName = Common::Path(name, '\\').baseName();
	uint32 nameSize = TG_HOST_SCENE_NAME_SIZE;
	if (_version >= kVersionV1_2_6)
		nameSize = TG_HOST_V1_2_6_SCENE_NAME_SIZE;
	else if (_version >= kVersionV1_2_3_2)
		nameSize = TG_HOST_V1_2_4_SCENE_NAME_SIZE;

	_currentSceneIndex = 0;

	if (!_sceneEntries.empty()) {
		do {
			if (sceneName.equalsIgnoreCase(_sceneEntries[_currentSceneIndex].name))
				break;

			_currentSceneIndex++;
		} while (_currentSceneIndex < _sceneEntries.size());
	}

	if (_currentSceneIndex >= _sceneEntries.size()) {
		SceneEntry_v1_2 entry;

		entry.stringCount = resourceLib()->resCountStrings(_file);
		if (entry.stringCount != 0) {
			entry.strings = utilsLibrary()->tgMalloc(entry.stringCount * TG_HOST_V1_2_STRING_SLOT_SIZE, 1);
			if (!entry.strings)
				return nullptr;
		}

		uint32 length = sceneName.size();
		if (length >= nameSize) {
			warning("TopgunEngine_v1_2::findOrCreateSceneEntry(): scene name is too long");
			length = nameSize - 1;
		}

		memcpy(entry.name, sceneName.c_str(), length);
		entry.name[length] = '\0';

		_sceneEntries.push_back(entry);
		resourceLib()->resEnumVariables(_file);
	}

	SceneEntry_v1_2 &current = _sceneEntries[_currentSceneIndex];
	uint32 numSceneVariables = TG_HOST_V1_2_NUM_SCENE_VARIABLES;
	if (_version >= kVersionV1_2_3_2)
		numSceneVariables = TG_HOST_V1_2_4_NUM_SCENE_VARIABLES;
	else if (_version >= kVersionV1_2_3)
		numSceneVariables = TG_HOST_V1_2_3_NUM_SCENE_VARIABLES;
	uint32 variablesSize = numSceneVariables * sizeof(uint16);

	memmove(_currentSceneName, current.name, nameSize);
	memmove(_variableTable, current.variables, variablesSize);
	spriteLib()->spriteSetVarTable(_variableTable);
	return &current;
}

bool TopgunEngine_v1_2::loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback) {
	uint16 keyTableSize = (_version >= kVersionV1_2_4) ? TG_INPUT_V1_2_4_KEY_TABLE_SIZE : TG_HOST_V1_2_KEY_TABLE_SIZE;

	for (uint16 i = 0; i < keyTableSize; i++)
		_keyTable[i] = KeyEntry_v1_2();

	if (_version >= kVersionV1_2_3_2) {
		TopgunEngine_v1_2_4 *newer = static_cast<TopgunEngine_v1_2_4 *>(this);

		_mouseAsStick = false;

		if (_version >= kVersionV1_2_4)
			_timersPaused = false;

		_keyboardRouted = false;
		newer->_joystickDirectionFlag = false;
		newer->_joystickRouted = false;
		_hoverSpriteScript = 0;
		_hoverSprite = 0;
		newer->_driveType = 0;
		_keyHookScript = 0;
		_mouseEventScript = 0;
		newer->_numNativeTimers = 0;
		newer->_activateScript = 0;
		_queueDoneScript = 0;

		if (_version >= kVersionV1_2_7_2) {
			newer->_waveStartedScript = 0;
			newer->_idleScript = 0;
		}

		if (_version == kVersionV1_2_7_3 || _version >= kVersionV1_2_9)
			newer->_numPostedScriptRuns = 0;

		if (_version >= kVersionV1_2_12) {
			_scrollBoxWindowBox = false;
			newer->_scriptCursors.clear();
		}

		if (_version >= kVersionV1_2_3_3)
			clearJoystickScripts();

		newer->clearDeviceState();
	} else if (_version >= kVersionV1_2_3) {
		_mouseEventScript = 0;
		_numNativeTimers = 0;
	} else {
		_scriptArgStackOffset = 0;
		_scriptArgCount = 0;
	}

	if (_version >= kVersionV1_2_3)
		_argumentFrame = (_version >= kVersionV1_2_3_2) ? TG_HOST_V1_2_4_FIRST_FRAME_INDEX : TG_HOST_V1_2_3_FIRST_FRAME_INDEX;

	if (_version >= kVersionV2_0)
		_argumentFrame = TG_HOST_V2_0_FIRST_FRAME_INDEX;

	if (_version >= kVersionV1_2_1)
		_pauseScript = 0;

	_hoverRegionCurrent = 0;
	_numHoverRegions = 0;
	_numMovies = 0;
	_numCollisionPairs = 0;
	_numClickRegions = 0;
	_numScheduledTimers = 0;
	_activationScript = 0;

	if (!openResourceFileAndInit(name)) {
		if (_version >= kVersionV1_2_3_2) {
			if (_file != 0)
				resourceLib()->resCloseFile(_file);

			_file = 0;
			_numResources = 0;
		} else if (_version >= kVersionV1_2_1) {
			if (_file != 0)
				resourceLib()->resCloseFile(_file);

			_file = 0;
			fatalError(102, Common::String::format("Can't open file '%s'.", name.c_str()));
		} else {
			fatalError(Common::String::format("Can't open file '%s'.", name.c_str()));
		}

		return false;
	}

	if (!findOrCreateSceneEntry(name)) {
		if (_version >= kVersionV1_2_1)
			fatalError(100, "Not enough memory to run program. Please\nrefer to the trouble shooting instructions.");
		else
			fatalError("Not enough memory to run program. Please\nrefer to the trouble shooting instructions.");

		return false;
	}

	if (_debugNameTable)
		buildResourceNameOffsetTable();

	if (withProgressCallback)
		resourceLib()->resSetCallback(_file, true, TG_HOST_PROGRESS_INTERVAL);

	refreshGamePalette();
	_fadeIn = true;
	_entryScriptRunning = true;

	// The entry script is the immediate operand 0 in the later version
	uint16 entryScript = (_version >= kVersionV1_2_3_2) ? (uint16)(0 - TG_HOST_V1_2_4_LITERAL_BIAS) : TG_HOST_V1_2_ENTRY_SCRIPT;
	if (_version >= kVersionV2_0)
		entryScript = 0;

	runScriptResource(entryScript);

	if (_version >= kVersionV1_2_12)
		static_cast<TopgunEngine_v1_2_4 *>(this)->_scriptCursors.clear();

	_entryScriptRunning = false;
	resourceLib()->resSetCallback(_file, false, 0);
	return true;
}

void TopgunEngine_v1_2::switchToNextScene(const char *name) {
	uint32 nameSize = TG_HOST_SCENE_NAME_SIZE;
	if (_version >= kVersionV1_2_6)
		nameSize = TG_HOST_V1_2_6_SCENE_NAME_SIZE;
	else if (_version >= kVersionV1_2_3_2)
		nameSize = TG_HOST_V1_2_4_SCENE_NAME_SIZE;

	uint32 numSceneVariables = TG_HOST_V1_2_NUM_SCENE_VARIABLES;
	if (_version >= kVersionV1_2_3_2)
		numSceneVariables = TG_HOST_V1_2_4_NUM_SCENE_VARIABLES;
	else if (_version >= kVersionV1_2_3)
		numSceneVariables = TG_HOST_V1_2_3_NUM_SCENE_VARIABLES;
	uint32 variablesSize = numSceneVariables * sizeof(uint16);

	if (_version >= kVersionV1_2_12)
		static_cast<TopgunEngine_v1_2_4 *>(this)->notifyPluginsLifecycleEvent(kPluginSceneSwitching, name);

	if (_version < kVersionV1_2_3_2)
		notifyPythonizerWindow();

	if (_currentSceneIndex < _sceneEntries.size()) {
		SceneEntry_v1_2 &current = _sceneEntries[_currentSceneIndex];
		memmove(current.name, _currentSceneName, nameSize);
		memmove(current.variables, _variableTable, variablesSize);
	} else {
		warning("TopgunEngine_v1_2::switchToNextScene(): scene index %d is out of range", _currentSceneIndex);
	}

	_previousSceneIndex = _currentSceneIndex;

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	if (_version >= kVersionV1_2_3_1 && _version < kVersionV1_2_12)
		unloadPluginLibrary();

	Common::String sceneName = Common::Path(name, '\\').baseName();

	spriteLib()->spriteSetTopMost(_sceneSprite);
	spriteLib()->spriteSetClipBox(nullptr);

	if (_version >= kVersionV1_2_12)
		static_cast<SpriteLibrary_v1_2_4 *>(spriteLib())->spriteSetScrollBox(nullptr);

	spriteLib()->spriteSetSendMsg(0);

	if (_version < kVersionV1_2_3_2)
		releaseCurrentBackgroundRecord();

	if (!display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(true);

	sceneTransitionOut(TG_HOST_V1_2_FADE_STEPS);
	resetResourcesAndTransientState();

	drainMessage(kMsgV1_2SoundFinished);
	drainMessage(kMsgV1_2GotoScene);

	if (_version < kVersionV1_2_3_2)
		drainMessage(kMsgV1_2RunScript);

	spriteLib()->spriteSetBackground(nullptr, false, 0, nullptr);
	spriteLib()->spriteResetLibrary();

	if (_version >= kVersionV1_2_3_3)
		static_cast<AudioLibrary_v1_2_4 *>(audioLib())->audioResetLibrary();

	spriteLib()->spriteSetSendMsg(1);

	if (_backgroundBitmap)
		_backgroundBitmap = utilsLibrary()->tgFree(_backgroundBitmap);

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	if (_file != 0)
		resourceLib()->resCloseFile(_file);

	if (_version >= kVersionV1_2_12)
		static_cast<TopgunEngine_v1_2_4 *>(this)->unloadAllPluginModules();

	_file = 0;
	_releaseOnPalette = true;

	if (_version >= kVersionV1_2_12)
		fillBackground();

	if (loadSceneContainerAndRunEntryScript(sceneName, true)) {
		if (_sceneSprite != 0) {
			if (_version >= kVersionV1_2_4)
				static_cast<SpriteLibrary_v1_2_4 *>(spriteLib())->spriteBreakLoops(_sceneSprite, 1);
			else
				spriteLib()->spriteBreakLoops(_sceneSprite);
		}

		if (_fadeIn)
			sceneTransitionIn(TG_HOST_V1_2_FADE_STEPS);
	}

	clearInputMessageQueues();

	if (_fadeIn && !display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(false);
}

bool TopgunEngine_v1_2::loadInitialScene(const Common::String &name) {
	if (_version >= kVersionV1_2_3_2) {
		TopgunEngine_v1_2_4 *newer = static_cast<TopgunEngine_v1_2_4 *>(this);

		if (!locateResourceFile(name))
			return false;

		memset(newer->_quitSceneName, 0, sizeof(newer->_quitSceneName));
		memset(newer->_gotoSceneName, 0, sizeof(newer->_gotoSceneName));
		clearJoystickScripts();
		newer->clearDeviceState();
	} else {
		notifyPythonizerWindow();

		_gotoRecord = GotoSceneRecord();

		if (_version >= kVersionV1_2_1)
			clearJoystickScripts();

		if (_version >= kVersionV1_2_3)
			clearDeviceState();
	}

	uint16 firstClearedVariable = TG_HOST_V1_2_FIRST_CLEARED_VARIABLE;
	if (_version >= kVersionV1_2_3_2)
		firstClearedVariable = TG_HOST_V1_2_4_FIRST_SYSTEM_VARIABLE;
	else if (_version >= kVersionV1_2_3)
		firstClearedVariable = TG_HOST_V1_2_3_FIRST_SYSTEM_VARIABLE;
	uint16 numClearedVariables = TG_HOST_V1_2_NUM_CLEARED_VARIABLES;

	if (_version >= kVersionV1_2_3_2)
		numClearedVariables = TG_HOST_V1_2_4_NUM_SYSTEM_VARIABLES;
	else if (_version >= kVersionV1_2_3)
		numClearedVariables = TG_HOST_V1_2_3_NUM_SYSTEM_VARIABLES;
	else if (_version >= kVersionV1_2_2)
		numClearedVariables = TG_HOST_V1_2_2_NUM_CLEARED_VARIABLES;
	else if (_version >= kVersionV1_2_1)
		numClearedVariables = TG_HOST_V1_2_1_NUM_CLEARED_VARIABLES;

	for (uint16 i = 0; i < numClearedVariables; i++)
		writeVariable(firstClearedVariable + i, 0);

	if (ConfMan.hasKey("mode640"))
		_flagMode640 = ((byte)ConfMan.getInt("mode640") != 0);

	if (ConfMan.hasKey("lomem"))
		_flagLoMem = ((byte)ConfMan.getInt("lomem") != 0);

	if (_version >= kVersionV1_2_3_2) {
		writeVariable(TG_HOST_V1_2_4_VARIABLE_LOMEM, _flagLoMem ? 1 : 0);
		writeVariable(TG_HOST_V1_2_4_VARIABLE_SCREEN_WIDTH, display()->width());
		writeVariable(TG_HOST_V1_2_4_VARIABLE_SCREEN_HEIGHT, display()->height());
		writeVariable(TG_HOST_V1_2_4_VARIABLE_AUDIO_PRESENT, _audioPresent);
		writeVariable(TG_HOST_V1_2_4_VARIABLE_LAUNCH_MODE, _launchMode);
		writeVariable(TG_HOST_V1_2_4_VARIABLE_PROCESSOR_TYPE, utilsLibrary()->getProcessorType());
		writeVariable(TG_HOST_V1_2_4_VARIABLE_OVER_256_COLORS, _flagOver256Colors ? 1 : 0);
	} else if (_version >= kVersionV1_2_3) {
		writeVariable(TG_HOST_V1_2_3_VARIABLE_LOMEM, _flagLoMem ? 1 : 0);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_SCREEN_WIDTH, display()->width());
		writeVariable(TG_HOST_V1_2_3_VARIABLE_SCREEN_HEIGHT, display()->height());
		writeVariable(TG_HOST_V1_2_3_VARIABLE_AUDIO_PRESENT, _audioPresent);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_LAUNCH_MODE, _launchMode);
		writeVariable(TG_HOST_V1_2_3_VARIABLE_PROCESSOR_TYPE, utilsLibrary()->getProcessorType());
		writeVariable(TG_HOST_V1_2_3_VARIABLE_OVER_256_COLORS, _flagOver256Colors ? 1 : 0);
	} else {
		writeVariable(TG_HOST_V1_2_VARIABLE_LOMEM, _flagLoMem ? 1 : 0);
		writeVariable(TG_HOST_V1_2_VARIABLE_SCREEN_WIDTH, display()->width());
		writeVariable(TG_HOST_V1_2_VARIABLE_SCREEN_HEIGHT, display()->height());
		writeVariable(TG_HOST_V1_2_VARIABLE_AUDIO_PRESENT, _audioPresent);
		writeVariable(TG_HOST_V1_2_VARIABLE_LAUNCH_MODE, _launchMode);
		writeVariable(TG_HOST_V1_2_VARIABLE_PROCESSOR_TYPE, utilsLibrary()->getProcessorType());
		writeVariable(TG_HOST_V1_2_VARIABLE_OVER_256_COLORS, _flagOver256Colors ? 1 : 0);
	}

	if (_version >= kVersionV1_2_1) {
		if (_version >= kVersionV1_2_3_2) {
			writeVariable(TG_HOST_V1_2_4_VARIABLE_SCREEN_SAVER, _screenSaverOption);
			writeVariable(TG_HOST_V1_2_4_VARIABLE_JOYSTICKS, _numJoysticks);
		} else if (_version >= kVersionV1_2_3) {
			writeVariable(TG_HOST_V1_2_3_VARIABLE_SCREEN_SAVER, _screenSaverOption);
			writeVariable(TG_HOST_V1_2_3_VARIABLE_JOYSTICKS, _numJoysticks);
		} else {
			writeVariable(TG_HOST_V1_2_1_VARIABLE_SCREEN_SAVER, _screenSaverOption);
			writeVariable(TG_HOST_V1_2_1_VARIABLE_JOYSTICKS, _numJoysticks);
		}
	}

	Common::Point cursor = g_system->getEventManager()->getMousePos();
	if (_version >= kVersionV1_2_3_2) {
		writeVariable(TG_HOST_V1_2_4_VARIABLE_MOUSE_X, (uint16)(cursor.x - display()->viewportOrigin().x));
		writeVariable(TG_HOST_V1_2_4_VARIABLE_MOUSE_Y, (uint16)(cursor.y - display()->viewportOrigin().y));
	} else if (_version >= kVersionV1_2_3) {
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_X, (uint16)(cursor.x - display()->viewportOrigin().x));
		writeVariable(TG_HOST_V1_2_3_VARIABLE_MOUSE_Y, (uint16)(cursor.y - display()->viewportOrigin().y));
	} else {
		writeVariable(TG_HOST_V1_2_VARIABLE_MOUSE_X, (uint16)(cursor.x - display()->viewportOrigin().x));
		writeVariable(TG_HOST_V1_2_VARIABLE_MOUSE_Y, (uint16)(cursor.y - display()->viewportOrigin().y));
	}

	if (_version >= kVersionV1_2_3)
		_expressionStackPointer = 0;

	if (_version >= kVersionV1_2_3_2) {
		TopgunEngine_v1_2_4 *newer = static_cast<TopgunEngine_v1_2_4 *>(this);

		_joystickButtonFlag = false;
		_joystickDirectionFlag = false;
		newer->_numPostedScriptRuns = 0;
		newer->_activateState = true;
		newer->_messageFilter = nullptr;
		newer->_windowMessageCallback = nullptr;
	}

	if (_version >= kVersionV1_2_1) {
		_maxTransColors = 0;
		_fatalCode = 0;
	}

	_postSceneEndEnabled = true;
	_backgroundColor = 0;
	_solidBackground = false;
	_displayZoomMode = 0;
	_mouseEventScript = 0;
	_scriptArgStackOffset = 0;
	_scriptArgCount = 0;
	_scriptDepth = 0;
	_currentScriptId = 0;
	_previousSceneIndex = 0;
	_currentSceneIndex = 0;
	_filePaletteEntryCount = 0;
	_sceneEntries.clear();
	_stepCount = 0;
	_step = 0;
	_sceneEndScript = 0;
	_backgroundId = 0;
	_textEditObjectId = 0;
	_dragSpriteId = 0;
	_cursorMode = 1;
	_displayZoomActive = false;
	_rightButtonDown = false;
	_leftButtonDown = false;
	_loading = false;
	_active = false;
	_cdAudioPlaying = false;
	_hoverSpriteScript = 0;
	_hoverSprite = 0;
	_maxFadeColors = 0;
	_scriptsDisabled = false;
	_faded = true;
	_flagActivated = true;
	_appActive = true;
	_screenBox = true;

	Common::String sceneName = Common::Path(name, '\\').baseName();

	if (!display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(true);

	_startTick = getEngineMillis();

	if (_version >= kVersionV1_2_12)
		fillBackground();

	bool loaded = loadSceneContainerAndRunEntryScript(sceneName, false);

	if (_version >= kVersionV1_2_12) {
		if (!loaded)
			return false;

		static_cast<TopgunEngine_v1_2_4 *>(this)->notifyPluginsLifecycleEvent(kPluginSceneLoaded, sceneName.c_str());
	}

	spriteLib()->spriteSetTopMost(_sceneSprite);

	if (_version >= kVersionV1_2_3_2)
		handlePaintMessage();

	if (_fadeIn) {
		sceneTransitionIn(TG_HOST_V1_2_FADE_STEPS);

		if (!display()->hasPalette())
			spriteLib()->spriteSetBkgTransparent(false);
	}

	return true;
}

void TopgunEngine_v1_2::cleanupCurrentSceneAndContainer() {
	if (_version >= kVersionV1_2_12) {
		static_cast<TopgunEngine_v1_2_4 *>(this)->notifyPluginsLifecycleEvent(kPluginSceneClosed, nullptr);
		static_cast<TopgunEngine_v1_2_4 *>(this)->unloadAllPluginModules();
	}

	if (_version >= kVersionV1_2_1)
		fillBackground();
	else if (_launchMode == 0)
		saveScreenSaverTimes();

	destroySceneMainObjects(_releaseOnPalette);
	resetResourcesAndTransientState();

	if (_version >= kVersionV1_2_3_2) {
		TopgunEngine_v1_2_4 *newer = static_cast<TopgunEngine_v1_2_4 *>(this);

		_numClickRegions = 0;
		_numCollisionPairs = 0;

		clearJoystickScripts();
		newer->clearDeviceState();
	} else if (_version >= kVersionV1_2_3) {
		clearJoystickScripts();
	}

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	if (_buffer1)
		_buffer1 = utilsLibrary()->tgFree(_buffer1);

	if (_backgroundBitmap)
		_backgroundBitmap = utilsLibrary()->tgFree(_backgroundBitmap);

	if (_version >= kVersionV1_2_3_1 && _version < kVersionV1_2_12)
		unloadPluginLibrary();

	for (uint16 i = 0; i < _sceneEntries.size(); i++) {
		if (_sceneEntries[i].strings)
			utilsLibrary()->tgFree(_sceneEntries[i].strings);
	}

	_sceneEntries.clear();

	if (_file != 0)
		resourceLib()->resCloseFile(_file);

	_file = 0;
	_appActive = false;

	if (_version >= kVersionV1_2_3) {
		_joystickRouted = false;
		_keyboardRouted = false;
	}

	if (_version >= kVersionV1_2_4)
		_mouseAsStick = false;

	if (_version >= kVersionV1_2_3_2) {
		static_cast<TopgunEngine_v1_2_4 *>(this)->_messageFilter = nullptr;
		static_cast<TopgunEngine_v1_2_4 *>(this)->_windowMessageCallback = nullptr;
	}

	// The Pythonizer (the desktop program the game can launch) is not restarted here if it was running when the game closed, since it is not supported
}

bool TopgunEngine_v1_2::openResourceFileAndInit(const Common::String &name) {
	Common::Path fullPath = _resourcePath.appendComponent(name);

	_file = resourceLib()->resOpenFile(fullPath.toString());
	if (_file == 0) {
		if (_version >= kVersionV1_2_3_2)
			static_cast<TopgunEngine_v1_2_4 *>(this)->fatalError(102, Common::String::format("Can't open file '%s'.", fullPath.toString().c_str()));

		return false;
	}

	if (_version >= kVersionV1_2_3_2) {
		TopgunEngine_v1_2_4 *newer = static_cast<TopgunEngine_v1_2_4 *>(this);
		ResourceLibrary_v1_2_4 *library = static_cast<ResourceLibrary_v1_2_4 *>(resourceLib());

		library->resSetNotify(_file, 1);
		_numResources = library->resCountResources(_file, &newer->_numFileResources);
	} else {
		if (_version >= kVersionV1_2_1)
			resourceLib()->resSetNotify(_file, 1);

		_numResources = resourceLib()->resCountResources(_file);
	}

	_records = new ResourceRecord[_numResources];

	if (_version < kVersionV1_2_1)
		_deferBitmapLoading = false;

	byte title[TG_HOST_V1_2_6_CONTAINER_TITLE_SIZE];
	uint16 titleSize = TG_HOST_V1_2_CONTAINER_TITLE_SIZE;
	if (_version >= kVersionV1_2_12)
		titleSize = TG_HOST_V1_2_12_CONTAINER_TITLE_SIZE;
	else if (_version >= kVersionV1_2_6)
		titleSize = TG_HOST_V1_2_6_CONTAINER_TITLE_SIZE;

	memset(title, 0, sizeof(title));
	resourceLib()->resGetTitle(_file, title, titleSize);

	_containerTitle.clear();
	if (title[0] != 0) {
		uint16 length = 0;
		while (length < titleSize - 1 && title[1 + length] != 0)
			length++;

		_containerTitle = Common::String((const char *)(title + 1), length);
	}

	if (_version >= kVersionV1_2_1 && _version < kVersionV1_2_3) {
		byte header[TG_HOST_V1_2_2_FILE_HEADER_SIZE];
		uint16 headerSize = TG_HOST_V1_2_1_FILE_HEADER_SIZE;

		if (_version == kVersionV1_2_2)
			headerSize = TG_HOST_V1_2_2_FILE_HEADER_SIZE;

		if (!resourceLib()->resGetHeaderRec(_file, header, headerSize))
			return false;

		_maxFadeColors = READ_LE_UINT16(header + TG_HOST_V1_2_1_HEADER_MAX_FADE_COLORS);
		_maxTransColors = READ_LE_UINT16(header + TG_HOST_V1_2_1_HEADER_MAX_TRANS_COLORS);
	}

	if (_version == kVersionV1_2_3) {
		_maxTransColors = resourceLib()->resGetMaxTransColors(_file);
		_maxFadeColors = resourceLib()->resGetMaxFadeColors(_file);
	}

	if (_version >= kVersionV1_2_3_1) {
		ResourceLibrary_v1_2_4 *library = static_cast<ResourceLibrary_v1_2_4 *>(resourceLib());

		_maxFadeColors = library->resGetMaxFadeColors(_file);
		_maxTransColors = library->resGetMaxTransColors(_file);

		if (_version >= kVersionV1_2_12) {
			if (!static_cast<TopgunEngine_v1_2_4 *>(this)->loadAndRegisterLibraries())
				return false;
		} else {
			char libraryName[TG_HOST_V1_2_6_LIBRARY_NAME_SIZE];
			uint16 libraryNameSize = TG_HOST_V1_2_4_LIBRARY_NAME_SIZE;
			if (_version >= kVersionV1_2_6)
				libraryNameSize = TG_HOST_V1_2_6_LIBRARY_NAME_SIZE;
			else if (_version == kVersionV1_2_3_1)
				libraryNameSize = TG_HOST_V1_2_3_1_LIBRARY_NAME_SIZE;

			library->resGetLibraryName(_file, libraryName, libraryNameSize);
			if (libraryName[0] != 0 && !loadPluginLibrary(libraryName)) {
				if (_version >= kVersionV1_2_3_2)
					static_cast<TopgunEngine_v1_2_4 *>(this)->fatalError(102, Common::String::format("Can't open file '%s'.", libraryName));

				return false;
			}
		}
	}

	return true;
}

void TopgunEngine_v1_2::buildResourceNameOffsetTable() {
	_resourceNamesTable = resourceLib()->resLoadNameTable(_file, nullptr);
	if (!_resourceNamesTable)
		return;

	uint16 entrySize = (_version >= kVersionV1_2_3_2) ? 4 : 2;

	_resourceNameOffsets = utilsLibrary()->tgMalloc(_numResources * entrySize, 0);
	if (!_resourceNameOffsets) {
		utilsLibrary()->tgFree(_resourceNamesTable);
		_resourceNamesTable = nullptr;
		return;
	}

	uint32 offset = 0;
	const byte *name = _resourceNamesTable;

	for (uint16 i = 0; i < _numResources; i++) {
		uint16 length;

		if (_version >= kVersionV1_2_3_2) {
			WRITE_LE_UINT32(_resourceNameOffsets + 4 * i, offset);
			length = utilsLibrary()->hugeStrLen((const char *)name) + 1;
		} else {
			WRITE_LE_UINT16(_resourceNameOffsets + 2 * i, (uint16)offset);
			length = strlen((const char *)name) + 1;
		}

		offset += length;
		name += length;
	}
}

void TopgunEngine_v1_2::releaseCurrentBackgroundRecord() {
	if (_backgroundId == 0)
		return;

	uint16 index = (_version >= kVersionV1_2_3_1) ? _backgroundId : operandValue(_backgroundId);
	if (!isRecordIndexValid(index, "releaseCurrentBackgroundRecord"))
		return;

	if (_records[index].refCount == 0)
		_records[index].data = nullptr;

	_backgroundId = 0;
}

void TopgunEngine_v1_2::refreshGamePalette() {
	byte localPalette[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];
	byte *palette = localPalette;
	uint16 firstTransColor = TG_HOST_V1_2_TRANS_PALETTE_FIRST;
	uint16 numTransColors = TG_HOST_V1_2_NUM_TRANS_PALETTE_COLORS;
	uint16 fadeColors = TG_HOST_V1_2_NUM_FADE_COLORS;
	uint32 clearedSize = sizeof(localPalette);

	if (_version >= kVersionV1_2_1) {
		if (_maxTransColors > TG_HOST_V1_2_1_PALETTE_LAST_COLOR) {
			warning("TopgunEngine_v1_2::refreshGamePalette(): %d transparent colors are too many", _maxTransColors);
			return;
		}

		// The transparent colors are the last ones before the highest 10 system colors
		firstTransColor = TG_HOST_V1_2_1_PALETTE_LAST_COLOR - _maxTransColors;
		numTransColors = _maxTransColors;
		_solidColorEntry = (byte)(firstTransColor - 1);

		if (_version >= kVersionV1_2_3_2) {
			palette = static_cast<TopgunEngine_v1_2_4 *>(this)->_palette;
			fadeColors = _maxFadeColors;
			clearedSize = TG_DISPLAY_NUM_PALETTE_ENTRIES;
		} else {
			fadeColors = TG_HOST_V1_2_1_FADE_LAST_COLOR - _maxTransColors;
		}
	}

	if (display()->hasPalette())
		display()->getSystemPaletteEntries(palette, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);
	else
		memset(palette, 0, clearedSize);

	_filePaletteEntryCount = resourceLib()->resLoadPalette(_file, palette + TG_HOST_V1_2_NUM_SYSTEM_COLORS * TG_DISPLAY_PALETTE_ENTRY_SIZE);
	spriteLib()->spriteGetPalette(palette + firstTransColor * TG_DISPLAY_PALETTE_ENTRY_SIZE, firstTransColor, numTransColors);
	spriteLib()->spriteSetPalette(palette);

	if (_version >= kVersionV1_2_3_2 || !_noFade) {
		spriteLib()->spriteFadePalette(0, 0, TG_HOST_V1_2_NUM_SYSTEM_COLORS, fadeColors);
		spriteLib()->spriteRealizePalette();
	}
}

char *TopgunEngine_v1_2::getStringFromId(uint16 id) {
	if (id == 0)
		return nullptr;

	if ((id & TG_HOST_V1_2_STRING_ID_CONSTANT) != 0)
		return const_cast<char *>(resourceLib()->resGetConstStr(_file, id & TG_HOST_V1_2_STRING_ID_MASK));

	if (_currentSceneIndex >= _sceneEntries.size()) {
		warning("TopgunEngine_v1_2::getStringFromId(): scene index %d is out of range", _currentSceneIndex);
		return nullptr;
	}

	uint16 slot = (id & TG_HOST_V1_2_STRING_ID_MASK) - 1;
	const SceneEntry_v1_2 &entry = _sceneEntries[_currentSceneIndex];
	if (entry.stringCount <= slot)
		return nullptr;

	return (char *)entry.strings + slot * TG_HOST_V1_2_STRING_SLOT_SIZE;
}

} // End of namespace Topgun
