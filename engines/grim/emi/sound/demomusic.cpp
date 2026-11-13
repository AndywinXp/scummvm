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

#include "common/archive.h"
#include "common/config-manager.h"
#include "common/fs.h"
#include "common/path.h"
#include "common/stream.h"
#include "common/system.h"

#include "engines/grim/emi/core/cpathingfileservices.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/sound/audiosystem.h"
#include "engines/grim/emi/sound/demomusic.h"
#include "engines/grim/emi/sound/miles_audio/driver.h"
#include "engines/grim/emi/sound/sounds.h"

namespace Grim {

static const int kLineSize = 1024;
static const int kGameDirectoryDepth = 4;
static const int kCenterPan = 64;
static const int kInfiniteLoopCount = 0;
static const float kCueFadeRate = 0.2f;
static const char *const kMusicMapName = "FullMonkeyMap.imt";

DemoMusicScript::DemoMusicScript(Sounds *sounds) {
	_sounds = sounds;
	_gameDirectory = nullptr;
	_loaded = false;
	_masterVolume = 1.0f;
	_lastUpdateMs = 0;
	_clockStarted = false;
}

DemoMusicScript::~DemoMusicScript() {
	closeMusicMap();
	clearSearchDirs();
}

EMIMilesDriver *DemoMusicScript::getMilesDriver() {
	AudioSystem *audioSystem = _sounds->getAudioSystem();

	if (!audioSystem)
		return nullptr;

	return audioSystem->getMilesDriver();
}

bool DemoMusicScript::isLoaded() const {
	return _loaded;
}

bool DemoMusicScript::loadMusicMap() {
	if (_loaded || !getMilesDriver())
		return false;

	pushSearchDir("Music\\");

	const char *cdPath = _sounds->getEngine()->getPathingFileServices()->getCdPath();

	if (cdPath) {
		Common::String directory = Common::String(cdPath) + "\\" + "Music\\";

		pushSearchDir(directory.c_str());
	}

	Common::SeekableReadStream *script = openAudioFile(kMusicMapName);

	if (!script) {
		clearSearchDirs();

		return false;
	}

	parseMusicMap(script);

	delete script;

	_loaded = true;

	return true;
}

void DemoMusicScript::parseMusicMap(Common::SeekableReadStream *script) {
	char line[kLineSize];
	char token[kLineSize];

	while (!script->eos()) {
		line[0] = 0;
		token[0] = 0;

		script->readLine(line, sizeof(line));
		sscanf(line, " %s ", token);

		if (strcmp(token, ".cueButton") == 0) {
			int cueId = 0;
			int cueX = 0;
			int cueY = 0;
			int cueSync = 0;
			Common::String cueName;

			sscanf(line, "%s id %d x %d y %d sync %d", token, &cueId, &cueX, &cueY, &cueSync);

			char *quote = strchr(line, '"');

			if (quote) {
				quote++;

				char *closingQuote = strchr(quote, '"');

				if (closingQuote) {
					*closingQuote = 0;
					cueName = quote;
				}
			}

			addCue(cueId, cueName.c_str(), cueX, cueY, cueSync);
		} else if (strcmp(token, ".playFile") == 0) {
			char *quote = strchr(line, '"');

			if (!quote)
				continue;

			quote++;

			char *closingQuote = strchr(quote, '"');

			if (!closingQuote)
				continue;

			*closingQuote = 0;

			addSlot(quote);
		}
	}
}

void DemoMusicScript::addCue(int id, const char *name, int x, int y, int sync) {
	Cue cue;

	cue.id = id;
	cue.name = name;
	cue.x = x;
	cue.y = y;
	cue.sync = sync;
	cue.fadeRate = kCueFadeRate;
	cue.slotCount = 0;

	for (int i = 0; i < kMaxSlots; i++) {
		cue.slots[i].stream = nullptr;
		cue.slots[i].state = 0;
		cue.slots[i].fadeVolume = 0.0f;
		cue.slots[i].paused = false;
	}

	_cues.push_back(cue);
}

void DemoMusicScript::addSlot(const char *fileName) {
	if (_cues.empty())
		return;

	Cue &cue = _cues.back();

	if (cue.slotCount >= kMaxSlots)
		return;

	cue.slots[cue.slotCount].fileName = fileName;
	cue.slotCount++;
}

void DemoMusicScript::closeMusicMap() {
	if (!_loaded)
		return;

	closeAllStreams();

	_cues.clear();
	_loaded = false;

	clearSearchDirs();
}

void DemoMusicScript::setMasterVolume(float volume) {
	float clamped = volume;

	if (!(volume >= 0.0f))
		clamped = 0.0f;
	else if (volume > 1.0f)
		clamped = 1.0f;

	EMIMilesDriver *driver = getMilesDriver();

	if (!driver)
		return;

	_masterVolume = clamped;

	// The fade is ignored: an open stream jumps to the master volume until the next fade step
	for (uint i = 0; i < _cues.size(); i++) {
		for (int j = 0; j < _cues[i].slotCount; j++) {
			Slot &slot = _cues[i].slots[j];

			if (slot.stream)
				driver->setStreamVolume(slot.stream, (int)((double)_masterVolume * 127.0));
		}
	}
}

void DemoMusicScript::applySlotVolume(Slot &slot) {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !slot.stream)
		return;

	driver->setStreamVolume(slot.stream, (int)((double)_masterVolume * (double)slot.fadeVolume * 127.0));
}

void DemoMusicScript::startCue(int id) {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !_loaded)
		return;

	// The cue list of the exe is in reverse file order and the last open stream met provides the position
	int carry = 0;

	for (int i = (int)_cues.size() - 1; i >= 0; i--) {
		Cue &cue = _cues[i];

		if (cue.id == id)
			continue;

		for (int j = 0; j < cue.slotCount; j++) {
			Slot &slot = cue.slots[j];

			if (!slot.stream)
				continue;

			slot.state = 2;
			carry = driver->getStreamPosition(slot.stream);
		}
	}

	for (uint i = 0; i < _cues.size(); i++) {
		Cue &cue = _cues[i];

		if (cue.id != id)
			continue;

		for (int j = 0; j < cue.slotCount; j++) {
			Slot &slot = cue.slots[j];

			if (!slot.stream)
				openSlot(slot, carry);

			slot.state = 1;
		}
	}
}

void DemoMusicScript::openSlot(Slot &slot, int carry) {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver)
		return;

	Common::SeekableReadStream *file = openAudioFile(slot.fileName.c_str());

	// A missing file leaves the slot closed without any message
	if (!file)
		return;

	slot.stream = driver->openStream(file);

	if (!slot.stream)
		return;

	int startPosition = 0;

	if (slot.stream->dataSize > 0 && carry >= 0)
		startPosition = carry % slot.stream->dataSize;
	else if (carry != 0)
		warning("DemoMusicScript::openSlot(): cannot carry the music position %d into a stream of %d bytes", carry, slot.stream->dataSize);

	slot.fadeVolume = 0.0f;
	slot.paused = false;

	driver->setStreamLoopCount(slot.stream, kInfiniteLoopCount);
	driver->setStreamPosition(slot.stream, startPosition);
	driver->setStreamVolume(slot.stream, 0);
	driver->setStreamPan(slot.stream, kCenterPan);
	driver->startStream(slot.stream);
}

void DemoMusicScript::closeSlot(Slot &slot) {
	if (!slot.stream)
		return;

	EMIMilesDriver *driver = getMilesDriver();

	if (driver)
		driver->closeStream(slot.stream);

	slot.stream = nullptr;
	slot.state = 0;
	slot.fadeVolume = 0.0f;
	slot.paused = false;
}

void DemoMusicScript::updateStreams() {
	if (!getMilesDriver() || !_loaded)
		return;

	uint now = g_system->getMillis();

	// The first call only starts the clock
	if (!_clockStarted) {
		_clockStarted = true;
		_lastUpdateMs = now;

		return;
	}

	float elapsed = (float)(now - _lastUpdateMs) * 0.001f;

	_lastUpdateMs = now;

	for (uint i = 0; i < _cues.size(); i++) {
		Cue &cue = _cues[i];

		for (int j = 0; j < cue.slotCount; j++) {
			Slot &slot = cue.slots[j];

			if (!slot.stream || slot.state == 0)
				continue;

			float change = cue.fadeRate * elapsed;

			if (slot.state == 1)
				slot.fadeVolume += change;
			else
				slot.fadeVolume -= change;

			if (slot.fadeVolume < 0.0f) {
				closeSlot(slot);

				continue;
			}

			if (slot.fadeVolume > 1.0f) {
				slot.fadeVolume = 1.0f;
				slot.state = 0;
			}

			applySlotVolume(slot);
		}
	}
}

void DemoMusicScript::pauseStreams() {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !_loaded)
		return;

	for (uint i = 0; i < _cues.size(); i++) {
		for (int j = 0; j < _cues[i].slotCount; j++) {
			Slot &slot = _cues[i].slots[j];

			if (!slot.stream)
				continue;

			driver->pauseStream(slot.stream, true);
			slot.paused = true;
		}
	}
}

void DemoMusicScript::resumeStreams() {
	EMIMilesDriver *driver = getMilesDriver();

	if (!driver || !_loaded)
		return;

	for (uint i = 0; i < _cues.size(); i++) {
		for (int j = 0; j < _cues[i].slotCount; j++) {
			Slot &slot = _cues[i].slots[j];

			if (!slot.stream || !slot.paused)
				continue;

			driver->pauseStream(slot.stream, false);
			slot.paused = false;
		}
	}
}

void DemoMusicScript::closeAllStreams() {
	if (!_loaded)
		return;

	for (uint i = 0; i < _cues.size(); i++) {
		for (int j = 0; j < _cues[i].slotCount; j++)
			closeSlot(_cues[i].slots[j]);
	}
}

void DemoMusicScript::pushSearchDir(const char *directory) {
	Common::String slot = directory;

	if (slot.lastChar() != '\\')
		slot += '\\';

	_searchDirs.push_back(slot);
}

void DemoMusicScript::clearSearchDirs() {
	_searchDirs.clear();

	delete _gameDirectory;
	_gameDirectory = nullptr;
}

Common::SeekableReadStream *DemoMusicScript::openStream(const char *path) {
	// Like CPathingFileServices, the leading ".\" is not part of the lookup
	if (path[0] == '.' && path[1] == '\\')
		path += 2;

	Common::Path filePath(path, '\\');
	Common::SeekableReadStream *stream = SearchMan.createReadStreamForMember(filePath);

	if (stream)
		return stream;

	if (!_gameDirectory)
		_gameDirectory = new Common::FSDirectory(Common::FSNode(ConfMan.getPath("path")), kGameDirectoryDepth, false, true);

	return _gameDirectory->createReadStreamForMember(filePath);
}

Common::SeekableReadStream *DemoMusicScript::openAudioFile(const char *fileName) {
	Common::SeekableReadStream *stream = openStream(fileName);

	if (stream)
		return stream;

	for (uint i = 0; i < _searchDirs.size(); i++) {
		Common::String path = _searchDirs[i] + fileName;

		stream = openStream(path.c_str());

		if (stream)
			return stream;
	}

	return nullptr;
}

} // End of namespace Grim
