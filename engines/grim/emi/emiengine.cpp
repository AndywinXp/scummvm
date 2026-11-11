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

#include "engines/grim/emi/emiengine.h"

#include "common/str.h"

#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/dimregion.h"
#include "engines/grim/emi/movie.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/polyglot_v2.h"
#include "engines/grim/emi/primitivequeue_v2.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/textevent.h"
#include "engines/grim/inputdialog.h"

namespace Grim {

static const int kPrimitiveLayers = 2;

static ProfilerTimer renderUpdateTimer("sputRender_Update()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);

int EmiEngine::_mode = 0;
int EmiEngine::_pausedMode = 0;
int EmiEngine::_discNumber = 0;
int EmiEngine::_frameDumpCount = 0;
LString EmiEngine::_frameDumpPath;
int EmiEngine::_savedMode = 0;
int EmiEngine::_cpuSpeed = 1000;
LString EmiEngine::_island;
float EmiEngine::_frameTime = 0.0f;
bool EmiEngine::_dumpingFrames = false;
bool EmiEngine::_saveLoadIdle = true;
int EmiEngine::_frontCopyCount = 0;
int EmiEngine::_pendingMusicState = -1;
Polyglot_v2 *EmiEngine::_polyglot = nullptr;

EmiEngine::EmiEngine(OSystem *syst, uint32 gameFlags, GrimGameType gameType, Common::Platform platform, Common::Language language) :
		GrimEngine(syst, gameFlags, gameType, platform, language) {
	_polyglot = new Polyglot_v2();
}

EmiEngine::~EmiEngine() {
	delete _polyglot;
	_polyglot = nullptr;
}

Polyglot_v2 *EmiEngine::getPolyglot() {
	return _polyglot;
}

void EmiEngine::setPause(bool pause) {
	if (!_mode)
		return;

	if (pause) {
		if (_mode == 1)
			return;

		_pausedMode = _mode;
		_mode = 1;
		Sounds::pauseMusicStreams();
		Movie::pauseCurrent();
	} else if (_mode == 1) {
		_mode = _pausedMode;
		Sounds::resumeMusicStreams();
		Movie::resumeCurrent();
	}
}

int EmiEngine::getDiscNumber() {
	return _discNumber;
}

bool EmiEngine::findDisc(int disc) {
	if (disc != 1 && disc != 2)
		return false;

	// TODO the search of the CD is not ported, the disc is always found
	_discNumber = disc;

	return true;
}

uint EmiEngine::getTotalPhysMem() {
	// TODO the original asks the system, this is a constant
	return 0x40000000;
}

int EmiEngine::messageBox(const char *caption, const char *text, uint flags) {
	// TODO
	return kMessageBoxNo;
}

bool EmiEngine::inputBox(const char *caption, const char *prompt, char *buffer, int bufferSize, const char *defaultText) {
	Common::String message = caption;

	message += ": ";
	message += prompt;

	Common::String defaultString;

	if (defaultText)
		defaultString = defaultText;

	InputDialog dialog(message, defaultString);

	if (!dialog.runModal())
		return false;

	Common::strlcpy(buffer, dialog.getString().c_str(), bufferSize);

	return true;
}

void EmiEngine::clearKeyStates() {
	// TODO
}

void EmiEngine::saveScreenshot() {
	// TODO
}

bool EmiEngine::buildSpecScreenTextures(int index) {
	// TODO
	return false;
}

bool EmiEngine::grabScreen() {
	// TODO
	return false;
}

bool EmiEngine::makeThumbnailTexture(int index, bool fromScreen) {
	// TODO
	return false;
}

bool EmiEngine::loadThumbnailTexture(int index, const char *fileName) {
	// TODO
	return false;
}

bool EmiEngine::clearTextureSlot(int index) {
	// TODO
	return false;
}

LFile *EmiEngine::openSaveFile(const char *fileName, const char *mode) {
	// TODO
	return nullptr;
}

void EmiEngine::closeSaveFile(LFile *file) {
	// TODO
}

void EmiEngine::beginFrameDump(const char *path) {
	_frameDumpCount = 0;
	_frameDumpPath = path;
	_dumpingFrames = true;

	// TODO create the directory of the dump
}

void EmiEngine::endFrameDump() {
	_dumpingFrames = false;
}

bool EmiEngine::playMovie(const char *fileName, bool subtitles, bool flag6194, bool flag6195, int startFrame) {
	LString name = fileName;

	if (!strrchr(fileName, '.'))
		name += ".bik";

	LString resolvedName;

	if (!gpFileServices->resolvePath(resolvedName, name.c_str()))
		return false;

	if (!Movie::startMovie(resolvedName.c_str(), subtitles, flag6194, flag6195, startFrame))
		return false;

	_mode = kModeMovie;

	return true;
}

void EmiEngine::setPendingMusicState(int musicState) {
	_pendingMusicState = musicState;
}

int EmiEngine::getMode() {
	return _mode;
}

void EmiEngine::setMode(int newMode) {
	_mode = newMode;
}

int EmiEngine::getSavedMode() {
	return _savedMode;
}

void EmiEngine::setSavedMode(int newMode) {
	_savedMode = newMode;
}

int EmiEngine::getCpuSpeed() {
	return _cpuSpeed;
}

void EmiEngine::setCpuSpeed(int newSpeed) {
	_cpuSpeed = newSpeed;
}

bool EmiEngine::hasGammaControl() {
	return CDrawContext::context && CDrawContext::context->hasGammaControl();
}

void EmiEngine::setGamma(float gamma) {
	if (CDrawContext::context)
		CDrawContext::context->setGamma(gamma);
}

bool EmiEngine::setIsland(const char *island) {
	if (CBundleServices::isIslandBundleOpen(island))
		return true;

	if (!_island.empty()) {
		CBundleServices::closeIslandBundles(_island.c_str());

		ResourceManager *resourceManager = dynamic_cast<ResourceManager *>(gpResourceManager);

		if (resourceManager)
			resourceManager->purgeAll();
	}

	_island = island;

	if (_island.empty())
		return true;

	return CBundleServices::openIslandBundles(_island.c_str());
}

float EmiEngine::getFrameTime() {
	return _frameTime;
}

bool EmiEngine::isDumpingFrames() {
	return _dumpingFrames;
}

void EmiEngine::setSaveLoadIdle(bool idle) {
	_saveLoadIdle = idle;
}

int EmiEngine::renderUpdate(CDrawContext *drawContext) {
	renderUpdateTimer.start();

	int result = 0;

	if (getMode() == kModeNone) {
		renderUpdateTimer.stop();

		return 0;
	}

	if (getMode() == kModeMovie) {
		Movie::presentCurrent();

		renderUpdateTimer.stop();

		return 0;
	}

	Room_v2 *currentRoom = Room_v2::getCurrentRoom();

	if (!currentRoom) {
		renderUpdateTimer.stop();

		return 0;
	}

	Overworld *overworld = Overworld::getInstance();

	if (getMode() != kModeOverworld) {
		_frontCopyCount = 0;

		if (drawContext->beginScene()) {
			int roomResult = currentRoom->draw(drawContext);

			if (roomResult < 0) {
				drawContext->endScene();
			} else {
				PrimitiveQueue_v2::drawQueue(drawContext, kPrimitiveLayers);
				result = DimRegion::drawDimRegions(drawContext) + roomResult;

				if (overworld->enabled) {
					currentRoom->saveLightStates();
					currentRoom->setLightsActive(false);
					overworld->setLightsActive(true);
					result += overworld->draw(drawContext);
					overworld->setLightsActive(false);
					currentRoom->restoreLightStates();
				}

				result += TextEvent::update(drawContext, true);
				drawContext->endScene();
				drawContext->present(true);
			}
		}
	} else if (drawContext->beginScene()) {
		int copyCount = _frontCopyCount;

		_frontCopyCount++;

		if (copyCount < 1)
			drawContext->copyFrontToBack();

		if (overworld->enabled) {
			currentRoom->saveLightStates();
			currentRoom->setLightsActive(false);
			overworld->setLightsActive(true);
			result = overworld->draw(drawContext);
			overworld->setLightsActive(false);
			currentRoom->restoreLightStates();
		}

		result += TextEvent::update(drawContext, true);
		drawContext->endScene();
		drawContext->present(true);
	}

	renderUpdateTimer.stop();

	return result;
}

} // End of namespace Grim
