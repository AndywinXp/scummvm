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
#include "common/textconsole.h"
#include "graphics/cursorman.h"

#include "topgun/display.h"
#include "topgun/libraries/audiolibrary.h"
#include "topgun/libraries/resourcelibrary.h"
#include "topgun/libraries/spritelibrary.h"
#include "topgun/libraries/utilslibrary.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

SceneRecord *TopgunEngine_v0::findOrCreateSceneEntry(const Common::String &name) {
	for (uint16 i = 0; i < _numScenes; i++) {
		if (name.equalsIgnoreCase(_scenes[i].name))
			return &_scenes[i];
	}

	if (_numScenes >= kMaxScenes)
		return nullptr;

	SceneRecord &scene = _scenes[_numScenes];
	uint32 length = name.size();

	if (length >= kSceneNameSize) {
		warning("TopgunEngine_v0::findOrCreateSceneEntry(): scene name is too long");
		length = kSceneNameSize - 1;
	}

	memcpy(scene.name, name.c_str(), length);
	scene.name[length] = '\0';
	memset(scene.flags, 0, sizeof(scene.flags));
	_numScenes++;
	return &scene;
}

bool TopgunEngine_v0::loadSceneContainerAndRunEntryScript(const Common::String &name, bool withProgressCallback) {
	_currentScene = findOrCreateSceneEntry(name);
	_keyScripts.clear();
	_activationScript = 0;

	if (!openResourceFileAndInit(name)) {
		showFormattedErrorMessageBox(102, name.c_str());
		postMessage(kMsgClose, 0, 0);
		return false;
	}

	if (withProgressCallback)
		resourceLibrary()->resSetCallback(_file, true, kProgressInterval);

	byte palette[kNumPaletteEntries * kPaletteEntrySize];
	memset(palette, 0, sizeof(palette));
	refreshGamePalette(palette);

	runScriptResource(0);

	resourceLibrary()->resSetCallback(_file, false, 0);
	return true;
}

void TopgunEngine_v0::switchToNextScene(const char *name) {
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

bool TopgunEngine_v0::loadInitialScene(const Common::String &name) {
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

void TopgunEngine_v0::cleanupCurrentSceneAndContainer() {
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

} // End of namespace Topgun
