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

#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/dimregion.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/polyglot_v2.h"
#include "engines/grim/emi/primitivequeue_v2.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/textevent.h"

namespace Grim {

static const int kPrimitiveLayers = 2;

static ProfilerTimer renderUpdateTimer("sputRender_Update()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);

int EmiEngine::_mode = 0;
int EmiEngine::_savedMode = 0;
int EmiEngine::_cpuSpeed = 1000;
LString EmiEngine::_island;
float EmiEngine::_frameTime = 0.0f;
bool EmiEngine::_dumpingFrames = false;
bool EmiEngine::_saveLoadIdle = true;
int EmiEngine::_frontCopyCount = 0;
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
		// TODO

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
