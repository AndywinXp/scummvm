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
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/resource.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils.h"
#include "topgun/topgun_v1_1.h"

namespace Topgun {

TopgunEngine_v1_1::TopgunEngine_v1_1(OSystem *syst, const ADGameDescription *gameDesc) :
	TopgunEngine_v1_0(syst, gameDesc, 1) {
}

TopgunEngine_v1_1::~TopgunEngine_v1_1() {
}

void TopgunEngine_v1_1::initCDAudio() {
}

uint16 TopgunEngine_v1_1::mainLoop() {
	Message message;

	while (!shouldQuit()) {
		if (peekMessage(message)) {
			if (message.id == kMsgQuit)
				return message.param;

			dispatchMessage(message);
		}

		if (_appActive) {
			spriteLibrary()->spriteAnimate();

			if (_numTimers != 0)
				serviceScheduledMsgTimerQueue();
		}

		presentScreenAndWait();
	}

	handleAppShutdown();

	return 0;
}

void TopgunEngine_v1_1::resourceLoadProgressCallback() {
	spriteLibrary()->spriteAnimate();

	if (_loading && _cdAudioPlaying) {
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);
		_cdAudioPlaying = false;
	}
}

void TopgunEngine_v1_1::handleActivateMessage(uint16 param) {
	bool active = (param != 0);

	if (!_altTab && _file != 0 && _appActive != active) {
		_globalFlags[1] = active ? 1 : 0;
		_appActive = active;
		audioLibrary()->audioActivate(active ? 1 : 0);

		if (active) {
			if (_activationScript != 0)
				runScriptResource(_activationScript);

			applyCursorFromRegister();
		}
	}
}

void TopgunEngine_v1_1::handleSceneEndMessage() {
	if (_sceneEndScript != 0) {
		runScriptResource(_sceneEndScript);
		spriteLibrary()->spriteAnimate();
		_sceneEndScript = 0;
	}

	destroySceneMainObjects(true);
}

bool TopgunEngine_v1_1::loadInitialScene(const Common::String &name) {
	_gotoRecord = GotoSceneRecord();
	memset(_globalFlags, 0, sizeof(_globalFlags));

	if (ConfMan.hasKey("mode640"))
		_flagMode640 = ((byte)ConfMan.getInt("mode640") != 0);

	if (ConfMan.hasKey("lomem"))
		_flagLoMem = ((byte)ConfMan.getInt("lomem") != 0);

	_altTab = ConfMan.hasKey("alttab") && ConfMan.getInt("alttab") != 0;

	_globalFlags[2] = _flagLoMem ? 1 : 0;
	_gotoRecord.releaseSceneSprite = 1;
	_numScenes = 0;
	_sceneEndScript = 0;
	_cursorMode = 1;
	_faded = true;
	_appActive = true;
	_screenBox = true;
	_scriptsDisabled = false;

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(true);

	loadSceneContainerAndRunEntryScript(name, false);
	sceneTransitionIn();

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(false);

	return true;
}

void TopgunEngine_v1_1::switchToNextScene(const char *name) {
	if (_cdAudioPlaying) {
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);
		_cdAudioPlaying = false;
	}

	clearBackground();
	spriteLibrary()->spriteSetTopMost(_sceneSprite);

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(true);

	sceneTransitionOut();
	resetResourcesAndTransientState();
	spriteLibrary()->spriteSetBackground(nullptr, nullptr);
	spriteLibrary()->spriteResetComposeBuffer();

	if (_buffer2)
		_buffer2 = utilsLibrary()->tgFree(_buffer2);

	if (_cdAudioPlaying) {
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);
		_cdAudioPlaying = false;
	}

	if (_file != 0)
		resourceLibrary()->resCloseFile(_file);

	_file = 0;
	_releaseOnPalette = true;

	if (loadSceneContainerAndRunEntryScript(name, true)) {
		if (_sceneSprite != 0)
			spriteLibrary()->spriteBreakLoops(_sceneSprite);

		sceneTransitionIn();
	}

	clearInputMessageQueues();

	if (!display()->hasPalette())
		spriteLibrary()->spriteSetBkgTransparent(false);
}

void TopgunEngine_v1_1::cleanupCurrentSceneAndContainer() {
	destroySceneMainObjects(_releaseOnPalette);
	resetResourcesAndTransientState();

	if (_cdAudioPlaying) {
		audioLibrary()->audioCommand(kStopCDCommand, 0, 0, nullptr);
		_cdAudioPlaying = false;
	}

	if (_buffer1)
		_buffer1 = utilsLibrary()->tgFree(_buffer1);

	if (_file != 0)
		resourceLibrary()->resCloseFile(_file);

	_file = 0;
}

void TopgunEngine_v1_1::destroySceneMainObjects(bool release) {
	if (release && _sceneSprite != 0)
		spriteLibrary()->spriteDestroy(_sceneSprite, true, true);

	if (release && _sceneSpriteWave != 0)
		audioLibrary()->audioCommand(kDestroySoundCommand, _sceneSpriteWave, 0, nullptr);

	_sceneSprite = 0;
	_sceneSpriteWave = 0;
	_releaseOnPalette = false;
}

void TopgunEngine_v1_1::freeBitmapBlobIfUnreferenced(uint16 id) {
	ResourceRecord &record = _records[id];

	if (record.data && record.refCount == 0) {
		utilsLibrary()->tgFree(record.data);
		record.data = nullptr;
	}
}

bool TopgunEngine_v1_1::destroySpriteResourceObject(uint16 id) {
	ResourceRecord &record = _records[id];

	uint16 sprite = record.handle;
	if (_freeingAll && sprite != 0 && _sceneSprite == sprite) {
		utilsLibrary()->tgFree(record.idList);
		record.idList = nullptr;
		record.handle = 0;
		return true;
	}

	if (sprite != 0)
		spriteLibrary()->spriteDestroy(sprite, !_freeingAll, false);

	record.handle = 0;

	if (!record.idList)
		return true;

	byte *list = record.idList;
	record.idList = nullptr;

	uint16 count = record.numMembers;
	for (uint16 i = 0; i < count; i++) {
		uint16 member = READ_LE_UINT16(list + 2 * i);
		uint16 type = resourceLibrary()->resGetType(_file, member);

		if (type == kResourceBitmap && _records[member].refCount != 0)
			_records[member].refCount--;

		destroyResourceObjectByType(member, type);
	}

	if (_sceneSprite == sprite)
		_sceneSprite = 0;

	utilsLibrary()->tgFree(list);
	return true;
}

bool TopgunEngine_v1_1::loadTransPaletteResource(uint16 id) {
	destroySceneMainObjects(_releaseOnPalette);

	return TopgunEngine_v1_0::loadTransPaletteResource(id);
}

byte *TopgunEngine_v1_1::loadResourceWithProcessingPause(uint16 id, uint32 *sizeOut) {
	uint16 type = resourceLibrary()->resGetType(_file, id);
	if (type == kResourceScript || type == kResourceIdList)
		return resourceLibrary()->resLoadResource(_file, id, sizeOut);

	return TopgunEngine_v1_0::loadResourceWithProcessingPause(id, sizeOut);
}

bool TopgunEngine_v1_1::validateAndGetResourceByType(uint16 id, uint16 type) {
	if (type == kResourceIdList) {
		if (!_records || id >= _numResources) {
			warning("TopgunEngine_v1_1::validateAndGetResourceByType(): resource id %d is out of range", id);
			return false;
		}

		return loadOrGetResourceData(id);
	}

	if (type == kResourcePlugin) {
		warning("TopgunEngine_v1_1::validateAndGetResourceByType(): plug-in resources are not supported");
		return false;
	}

	return TopgunEngine_v1_0::validateAndGetResourceByType(id, type);
}

bool TopgunEngine_v1_1::isResourceObjectLoaded(uint16 id) {
	if (resourceLibrary()->resGetType(_file, id) == kResourceIdList)
		return _records[id].data;

	return TopgunEngine_v1_0::isResourceObjectLoaded(id);
}

} // End of namespace Topgun
