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

#include "common/textconsole.h"

#include "engines/grim/grim.h"
#include "engines/grim/imuse/imuse_engine.h"
#include "engines/grim/sputactor.h"
#include "engines/grim/sputcontrol.h"
#include "engines/grim/sputrender.h"
#include "engines/grim/sputresource.h"
#include "engines/grim/sputroom.h"
#include "engines/grim/sputsmush.h"
#include "engines/grim/walkplane.h"

namespace Grim {

void GrimEngine::overrideSelectedActorCostume(int hInstance) {
	SputActor *pActor = _pSputActor->getSelectedActor();

	if (pActor) {
		// TODO: sputLua_DoCostumeOverrideCallback(pActor, 1)
		_pSputActor->pushCostume(pActor, nullptr, hInstance);
	}
}

void GrimEngine::restoreSelectedActorCostume() {
	SputActor *pActor = _pSputActor->getSelectedActor();

	if (pActor) {
		// TODO: sputLua_DoCostumeOverrideCallback(pActor, 0)
		_pSputActor->popCostume(pActor, 0);
	}
}

int GrimEngine::setDisplay(StdVBuffer *pVBuffer, StdVBuffer *pZBuffer) {
	int numDrawBuffers = 1; // TODO: stdDisplay_numDrawBuffers

	_pSputRender->pageCountIs(numDrawBuffers);
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

void GrimEngine::setDebugToggleCallback(void *pFunc) {
	// TODO: sputLua_SetDebugToggleCallback(pFunc)
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
		// TODO: sputLua_CameraChange(oldSetup, newSetup)
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
	int result = 0; // TODO: sputLua_DoExitCallback()

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
	int result = 0; // TODO: sputLua_DoPauseCallback()

	return result;
}

int GrimEngine::characterCallback() {
	int result = 0; // TODO: sputLua_DoCharacterCallback()

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
		// TODO: sputLua_DoUserPaintCallback()
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

} // End of namespace Grim
