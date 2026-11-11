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

#include "common/fs.h"
#include "common/path.h"
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/actor.h"
#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/color.h"
#include "engines/grim/emi/ctgafile.h"
#include "engines/grim/emi/dimregion.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/inputrecorder.h"
#include "engines/grim/emi/labitmap.h"
#include "engines/grim/emi/layer.h"
#include "engines/grim/emi/lcolorformat.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/ltimeservices.h"
#include "engines/grim/emi/lua_v2.h"
#include "engines/grim/emi/movie.h"
#include "engines/grim/emi/overworld.h"
#include "engines/grim/emi/primitivequeue_v2.h"
#include "engines/grim/emi/profilertimer.h"
#include "engines/grim/emi/resourcemanager.h"
#include "engines/grim/emi/room_v2.h"
#include "engines/grim/emi/sounds.h"
#include "engines/grim/emi/sputcontrol_v2.h"
#include "engines/grim/emi/textevent.h"
#include "engines/grim/emi/zfile.h"

namespace Grim {

static const int kPrimitiveLayers = 2;
static const float kMaxFrameTime = 0.125f;
static const double kMillisecondsToSeconds = 0.001f;
static const int kStatsTextX = 10;
static const int kStatsTextFirstY = 20;
static const int kStatsTextRowHeight = 20;
static const byte kStatsTextJustify = 16;

static ProfilerTimer updateManualTimer("grimlib_UpdateManual()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);
static ProfilerTimer renderUpdateTimer("sputRender_Update()", ProfilerTimer::kTrackNesting | ProfilerTimer::kPrintOther);

bool EmiEngine::runFrame() {
	updateManual(CDrawContext::context, true, 0);

	return !_bQuit;
}

int EmiEngine::updateManual(CDrawContext *drawContext, bool draw, int manualMs) {
	updateManualTimer.start();
	updateSounds(false);

	if (_libSaveRequest)
		saveGame(drawContext);

	if (_libLoadRequest)
		loadGame(drawContext);

	// The disasm does not stop the timer here
	if (_mode == kModeNone)
		return 0;

	// Not in the disasm, the options changed from the ScummVM menu
	applyPendingGameSettings(manualMs);

	if (InputRecorder::instance && InputRecorder::instance->mode == InputRecorder::kModeReplay)
		InputRecorder::instance->replayKeys();
	else
		SputControl_v2::update();

	float frameTime = updateFrameTime(manualMs);

	ResourceManager::update(frameTime);
	updateSounds(false);

	if (_mode != kModePaused || manualMs != 0) {
		_frameCount++;
		_lua->update(frameTime);
		updateSounds(false);

		if (_mode == kModeMovie) {
			if (updateMovie(frameTime))
				_mode = kModeGame;
		} else if (_mode != kModeOverworld) {
			Actor::updateAllActors(frameTime);
			updateSounds(false);
		}
	}

	if (_pendingMusicState != -1) {
		setMusicState(_pendingMusicState, -1);
		_pendingMusicState = -1;
	}

	updateSounds(false);

	int result = 0;

	if (draw && (_mode != kModePaused || manualMs != 0))
		result = renderUpdate(drawContext);

	updateManualTimer.stop();
	drawStatsOverlay(drawContext);

	if (_dumpingFrames)
		dumpFrame();

	return result;
}

float EmiEngine::updateFrameTime(int manualMs) {
	LTimeStamp stamp;

	stamp.stamp();

	if (InputRecorder::instance && InputRecorder::instance->mode == InputRecorder::kModeReplay) {
		_frameTime = InputRecorder::instance->replayFrame();

		return _frameTime;
	}

	float elapsed;

	if (manualMs != 0)
		elapsed = (float)((double)(uint)manualMs * kMillisecondsToSeconds);
	else
		elapsed = (float)(stamp.seconds - _lastStamp);

	if (_firstFrame) {
		elapsed = 0.0f;
		_firstFrame = false;
	}

	_rawFrameTime = elapsed;

	if (elapsed > kMaxFrameTime)
		elapsed = kMaxFrameTime;

	if (_dumpingFrames)
		elapsed = kMaxFrameTime;

	_lastStamp = stamp.seconds;
	_frameTime = elapsed;

	if (InputRecorder::instance)
		InputRecorder::instance->recordFrame(elapsed);

	return elapsed;
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

void EmiEngine::pixelTest() {
	CDrawContext *drawContext = CDrawContext::context;

	if (!drawContext->beginScene())
		return;

	Layer::set16BitFix(false);

	Layer *layer = new Layer("pixeltest.til", 0, 0, -1);

	layer->draw(drawContext);
	drawContext->endScene();
	Layer::set16BitFix(true);

	LABitmap bitmap;

	drawContext->readBackBuffer(&bitmap);
	bitmap.convertFormat(LColorFormat(6));

	uint pixelCount = (uint)bitmap.sizeBytes / 3;
	const byte *pixels = (const byte *)bitmap.pixels;
	uint rowWidth = (uint)bitmap.width;

	if (pixelCount != 0 && (!pixels || rowWidth == 0)) {
		warning("EmiEngine::pixelTest(): the back buffer has no pixels");
		pixelCount = 0;
	}

	for (uint i = 0; i < pixelCount; i++) {
		byte expected = ((i + i / rowWidth) & 1) != 0 ? 0 : 255;

		if (pixels[i * 3] != expected) {
			drawContext->setPixelCenter(!drawContext->getPixelCenter());

			break;
		}
	}

	delete layer;
	drawContext->clear(true, false);
}

void EmiEngine::saveScreenshot() {
	Common::String path = Common::String::format("./%s%d.tga", "shot", _screenshotCount);

	_screenshotCount++;

	// TODO write the screenshot to the ScummVM screenshot path
	saveTga(path.c_str());
}

void EmiEngine::saveTga(const char *path) {
	CDrawContext *drawContext = CDrawContext::context;

	if (!drawContext) {
		warning("EmiEngine::saveTga(): no draw context");

		return;
	}

	LABitmap bitmap;

	drawContext->readBackBuffer(&bitmap);
	bitmap.convertFormat(LColorFormat(6));

	CTGAFile tgaFile;
	ZFile *file = ZFileServices::get()->open(path, "wb");

	if (!file) {
		warning("EmiEngine::saveTga(): could not create %s", path);

		return;
	}

	tgaFile.writeImage(file, &bitmap, nullptr);
	ZFileServices::get()->closeZFile(file);
}

void EmiEngine::dumpFrame() {
	Common::String path = Common::String::format("./%s/%s%02d.tga", _frameDumpPath.c_str(), _frameDumpPath.c_str(), _frameDumpCount);

	_frameDumpCount++;
	saveTga(path.c_str());
}

void EmiEngine::beginFrameDump(const char *path) {
	_frameDumpCount = 0;
	_frameDumpPath = path;
	_dumpingFrames = true;

	// TODO write the frames to the ScummVM screenshot path
	Common::FSNode(Common::Path(path, '/')).createDirectory();
	Sounds::suspendAudio();
}

void EmiEngine::endFrameDump() {
	_dumpingFrames = false;
	Sounds::resumeAudio();
}

void EmiEngine::drawStatsOverlay(CDrawContext *drawContext) {
	Room_v2 *currentRoom = Room_v2::getCurrentRoom();
	bool skipTexts = false;

	if (currentRoom && (Lua_V2::statsOverlayFlags & Lua_V2::kStatsOverlayFlag) != 0) {
		double framesPerSecond = 0.0;

		// The original could divide by zero here, let's avoid that...
		if (_rawFrameTime != 0.0f)
			framesPerSecond = (double)1.0f / (double)_rawFrameTime;

		Common::String framesText = Common::String::format("FPS: %6.2f", framesPerSecond);
		Common::String frameTimeText = Common::String::format("FT:  %6.2fms", (double)_rawFrameTime * (double)1000.0f);
		TextEvent *firstEvent = _statsEvents[0];

		if (firstEvent && (firstEvent->flags & 1) != 0) {
			firstEvent->text = framesText;
			firstEvent->geometryReady = false;
			_statsEvents[1]->text = frameTimeText;
			_statsEvents[1]->geometryReady = false;
		} else {
			for (int i = 0; i < kNumStatsEvents; i++) {
				TextEvent *event;

				if (i == 1)
					event = new TextEvent(frameTimeText);
				else
					event = new TextEvent(framesText);

				_statsEvents[i] = event;
				event->setFont("comicsans18.laf");
				event->x = kStatsTextX;
				event->y = kStatsTextFirstY + i * kStatsTextRowHeight;
				event->geometryReady = false;
				event->justify = kStatsTextJustify;
				event->geometryReady = false;
				event->color = Color::kWhite;
				event->geometryReady = false;
				event->activate(0.0f, 1);
			}
		}
	} else if (currentRoom && (Lua_V2::statsOverlayFlags & Lua_V2::kStatsOverlayFlag) == 0) {
		if (_statsEvents[0]) {
			for (int i = 0; i < kNumStatsEvents; i++) {
				_statsEvents[i]->flags &= ~1u;
				_statsEvents[i] = nullptr;
			}

			skipTexts = true;
		}
	}

	if (!skipTexts) {
		if (_statsEvents[2]) {
			_statsEvents[2]->text = Common::String::format("mem: %d, polys: %d", (int)gpMemoryServices->bytesAllocated, drawContext->trianglesDrawn);
			_statsEvents[2]->geometryReady = false;
		}

		// The D3D statistics do not exist here and the disasm leaves the last value uninitialized
		if (_statsEvents[3]) {
			_statsEvents[3]->text = Common::String::format("%s EV: %4d CR: %4d UT: %4d UV: %4d", " ", 0, 0, 0, 0);
			_statsEvents[3]->geometryReady = false;
		}

		if (_statsEvents[4]) {
			_statsEvents[4]->text = Common::String::format("RE: %4d (%10d) MN: %4d (%10d)", 0, 0, 0, 0);
			_statsEvents[4]->geometryReady = false;
		}
	}

	ProfilerTimer::endFrame();

	if (currentRoom && (Lua_V2::statsOverlayFlags & Lua_V2::kStatsProfilerFlag) != 0) {
		ProfilerTimer::printAllStats();
		Lua_V2::statsOverlayFlags &= ~(uint)Lua_V2::kStatsProfilerFlag;
	}
}

} // End of namespace Grim
