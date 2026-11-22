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

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

SceneEntry_v1_2 *TopgunEngine_v1_2::findOrCreateSceneEntry(const Common::String &name) {
	Common::String sceneName = Common::Path(name, '\\').baseName();

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
		if (length >= TG_HOST_SCENE_NAME_SIZE) {
			warning("TopgunEngine_v1_2::findOrCreateSceneEntry(): scene name is too long");
			length = TG_HOST_SCENE_NAME_SIZE - 1;
		}

		memcpy(entry.name, sceneName.c_str(), length);
		entry.name[length] = '\0';

		_sceneEntries.push_back(entry);
		resourceLib()->resEnumVariables(_file);
	}

	SceneEntry_v1_2 &current = _sceneEntries[_currentSceneIndex];
	memmove(_currentSceneName, current.name, sizeof(_currentSceneName));
	memmove(_variables, current.variables, sizeof(current.variables));
	spriteLib()->spriteSetVarTable(_variables);
	return &current;
}

bool TopgunEngine_v1_2::loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback) {
	for (uint16 i = 0; i < TG_HOST_V1_2_KEY_TABLE_SIZE; i++)
		_keyTable[i] = KeyEntry_v1_2();

	_hoverRegionCurrent = 0;
	_numHoverRegions = 0;
	_numMovies = 0;
	_numCollisionPairs = 0;
	_numClickRegions = 0;
	_scriptArgStackOffset = 0;
	_scriptArgCount = 0;
	_numScheduledTimers = 0;
	_activationScript = 0;

	if (!openResourceFileAndInit(name)) {
		fatalError(Common::String::format("Can't open file '%s'.", name.c_str()));
		return false;
	}

	if (!findOrCreateSceneEntry(name)) {
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
	runScriptResource(TG_HOST_V1_2_ENTRY_SCRIPT);
	_entryScriptRunning = false;
	resourceLib()->resSetCallback(_file, false, 0);
	return true;
}

void TopgunEngine_v1_2::switchToNextScene(const char *name) {
	notifyPythonizerWindow();

	if (_currentSceneIndex < _sceneEntries.size()) {
		SceneEntry_v1_2 &current = _sceneEntries[_currentSceneIndex];
		memmove(current.name, _currentSceneName, sizeof(current.name));
		memmove(current.variables, _variables, sizeof(current.variables));
	} else {
		warning("TopgunEngine_v1_2::switchToNextScene(): scene index %d is out of range", _currentSceneIndex);
	}

	_previousSceneIndex = _currentSceneIndex;

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	Common::String sceneName = Common::Path(name, '\\').baseName();

	spriteLib()->spriteSetTopMost(_sceneSprite);
	spriteLib()->spriteSetClipBox(nullptr);
	spriteLib()->spriteSetSendMsg(0);
	releaseCurrentBackgroundRecord();

	if (!display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(true);

	sceneTransitionOut(TG_HOST_V1_2_FADE_STEPS);
	resetResourcesAndTransientState();

	drainMessage(kMsgV1_2SoundFinished);
	drainMessage(kMsgV1_2GotoScene);
	drainMessage(kMsgV1_2RunScript);

	spriteLib()->spriteSetBackground(nullptr, false, 0, nullptr);
	spriteLib()->spriteResetLibrary();
	spriteLib()->spriteSetSendMsg(1);

	if (_backgroundBitmap)
		_backgroundBitmap = utilsLibrary()->tgFree(_backgroundBitmap);

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	if (_file != 0)
		resourceLib()->resCloseFile(_file);

	_file = 0;
	_releaseOnPalette = true;

	if (loadSceneContainerAndRunEntryScript(sceneName, true)) {
		if (_sceneSprite != 0)
			spriteLib()->spriteBreakLoops(_sceneSprite);

		if (_fadeIn)
			sceneTransitionIn(TG_HOST_V1_2_FADE_STEPS);
	}

	clearInputMessageQueues();

	if (_fadeIn && !display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(false);
}

bool TopgunEngine_v1_2::loadInitialScene(const Common::String &name) {
	notifyPythonizerWindow();

	_gotoRecord = GotoSceneRecord();

	for (uint16 i = 0; i < TG_HOST_V1_2_NUM_CLEARED_VARIABLES; i++)
		_variables[TG_HOST_V1_2_FIRST_CLEARED_VARIABLE + i] = 0;

	if (ConfMan.hasKey("mode640"))
		_flagMode640 = ((byte)ConfMan.getInt("mode640") != 0);

	if (ConfMan.hasKey("lomem"))
		_flagLoMem = ((byte)ConfMan.getInt("lomem") != 0);

	_variables[TG_HOST_V1_2_VARIABLE_LOMEM] = _flagLoMem ? 1 : 0;
	_variables[TG_HOST_V1_2_VARIABLE_SCREEN_WIDTH] = display()->width();
	_variables[TG_HOST_V1_2_VARIABLE_SCREEN_HEIGHT] = display()->height();
	_variables[TG_HOST_V1_2_VARIABLE_AUDIO_PRESENT] = _audioPresent;
	_variables[TG_HOST_V1_2_VARIABLE_LAUNCH_MODE] = _launchMode;
	_variables[TG_HOST_V1_2_VARIABLE_PROCESSOR_TYPE] = utilsLibrary()->getProcessorType();
	_variables[TG_HOST_V1_2_VARIABLE_OVER_256_COLORS] = _flagOver256Colors ? 1 : 0;

	Common::Point cursor = g_system->getEventManager()->getMousePos();
	_variables[TG_HOST_V1_2_VARIABLE_MOUSE_X] = (uint16)(cursor.x - display()->viewportOrigin().x);
	_variables[TG_HOST_V1_2_VARIABLE_MOUSE_Y] = (uint16)(cursor.y - display()->viewportOrigin().y);

	_postSceneEndEnabled = true;
	_backgroundColor = 0;
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
	_scriptsDisabled = false;
	_faded = true;
	_flagActivated = true;
	_appActive = true;
	_screenBox = true;

	Common::String sceneName = Common::Path(name, '\\').baseName();

	if (!display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(true);

	_startTick = getEngineMillis();

	loadSceneContainerAndRunEntryScript(sceneName, false);
	spriteLib()->spriteSetTopMost(_sceneSprite);

	if (_fadeIn) {
		sceneTransitionIn(TG_HOST_V1_2_FADE_STEPS);

		if (!display()->hasPalette())
			spriteLib()->spriteSetBkgTransparent(false);
	}

	return true;
}

void TopgunEngine_v1_2::cleanupCurrentSceneAndContainer() {
	if (_launchMode == 0)
		saveScreenSaverTimes();

	destroySceneMainObjects(_releaseOnPalette);
	resetResourcesAndTransientState();

	if (_cdAudioPlaying)
		_cdAudioPlaying = ((byte)audioLib()->audioStopCd() != 0);

	if (_buffer1)
		_buffer1 = utilsLibrary()->tgFree(_buffer1);

	if (_backgroundBitmap)
		_backgroundBitmap = utilsLibrary()->tgFree(_backgroundBitmap);

	for (uint16 i = 0; i < _sceneEntries.size(); i++) {
		if (_sceneEntries[i].strings)
			utilsLibrary()->tgFree(_sceneEntries[i].strings);
	}

	_sceneEntries.clear();

	if (_file != 0)
		resourceLib()->resCloseFile(_file);

	_file = 0;
	_appActive = false;

	// The Pythonizer (the desktop program the game can launch) is not restarted here if it was running when the game closed, since it is not supported
}

bool TopgunEngine_v1_2::openResourceFileAndInit(const Common::String &name) {
	Common::Path fullPath = _resourcePath.appendComponent(name);

	_file = resourceLib()->resOpenFile(fullPath.toString());
	if (_file == 0)
		return false;

	_numResources = resourceLib()->resCountResources(_file);
	_records = new ResourceRecord[_numResources];
	_deferBitmapLoading = false;

	byte title[TG_HOST_V1_2_CONTAINER_TITLE_SIZE];
	memset(title, 0, sizeof(title));
	resourceLib()->resGetTitle(_file, title, TG_HOST_V1_2_CONTAINER_TITLE_SIZE);

	_containerTitle.clear();
	if (title[0] != 0) {
		uint32 length = 0;
		while (length < sizeof(title) - 1 && title[1 + length] != 0)
			length++;

		_containerTitle = Common::String((const char *)(title + 1), length);
	}

	return true;
}

void TopgunEngine_v1_2::buildResourceNameOffsetTable() {
	_resourceNamesTable = resourceLib()->resLoadNameTable(_file, nullptr);
	if (!_resourceNamesTable)
		return;

	_resourceNameOffsets = utilsLibrary()->tgMalloc(_numResources * 2, 0);
	if (!_resourceNameOffsets) {
		utilsLibrary()->tgFree(_resourceNamesTable);
		_resourceNamesTable = nullptr;
		return;
	}

	uint16 offset = 0;
	const byte *name = _resourceNamesTable;

	for (uint16 i = 0; i < _numResources; i++) {
		WRITE_LE_UINT16(_resourceNameOffsets + 2 * i, offset);

		uint16 length = strlen((const char *)name) + 1;
		offset += length;
		name += length;
	}
}

void TopgunEngine_v1_2::releaseCurrentBackgroundRecord() {
	if (_backgroundId == 0)
		return;

	uint16 index = operandValue(_backgroundId);
	if (!isRecordIndexValid(index, "releaseCurrentBackgroundRecord"))
		return;

	if (_records[index].refCount == 0)
		_records[index].data = nullptr;

	_backgroundId = 0;
}

void TopgunEngine_v1_2::refreshGamePalette() {
	byte palette[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];

	if (display()->hasPalette())
		display()->getSystemPaletteEntries(palette, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);
	else
		memset(palette, 0, sizeof(palette));

	_filePaletteEntryCount = resourceLib()->resLoadPalette(_file, palette + TG_HOST_V1_2_NUM_SYSTEM_COLORS * TG_DISPLAY_PALETTE_ENTRY_SIZE);
	spriteLib()->spriteGetPalette(palette + TG_HOST_V1_2_TRANS_PALETTE_FIRST * TG_DISPLAY_PALETTE_ENTRY_SIZE, TG_HOST_V1_2_TRANS_PALETTE_FIRST, TG_HOST_V1_2_NUM_TRANS_PALETTE_COLORS);
	spriteLib()->spriteSetPalette(palette);

	if (!_noFade) {
		spriteLib()->spriteFadePalette(0, 0, TG_HOST_V1_2_NUM_SYSTEM_COLORS, TG_HOST_V1_2_NUM_FADE_COLORS);
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
