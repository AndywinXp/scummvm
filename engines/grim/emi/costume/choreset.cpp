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

#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"

#include "engines/grim/emi/costume/choreset.h"
#include "engines/grim/emi/costume/component.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"
#include "engines/grim/emi/resources/lresourcehandle.h"

namespace Grim {

static const int kNameBufferSize = 256;
static const int kTextTokenSize = 128;
static const int kTagSize = 4;
static const int kBinaryKeySize = 8;

static bool readInt(LFile *file, int *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_INT32(bytes);

	return true;
}

static bool readFloat(LFile *file, float *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_FLOAT32(bytes);

	return true;
}

static bool readName(LFile *file, char *buffer) {
	int length;

	if (!readInt(file, &length) || length < 1 || length > kNameBufferSize)
		return false;

	memset(buffer, 0, kNameBufferSize);

	if (file->read(buffer, length) != (uint)length)
		return false;

	buffer[kNameBufferSize - 1] = 0;

	return true;
}

static void writeInt(LFile *file, int value) {
	byte bytes[4];

	WRITE_LE_INT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

static void writeFloat(LFile *file, float value) {
	byte bytes[4];

	WRITE_LE_FLOAT32(bytes, value);
	file->write(bytes, sizeof(bytes));
}

void *ChoreKey::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void ChoreKey::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

ChoreTrack::ChoreTrack() {
	_id = -1;
	_parentId = -1;
}

ChoreTrack::~ChoreTrack() {
	Common::List<ChoreKey *>::iterator it;

	for (it = _keys.begin(); it != _keys.end(); ++it) {
		delete *it;
		*it = nullptr;
	}

	_keys.clear();
}

void *ChoreTrack::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void ChoreTrack::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int ChoreTrack::getSize() {
	return (int)(_resourceName.size() + 3 * sizeof(void *) * (_keys.size() + 3));
}

void ChoreTrack::addKey(ChoreKey *key) {
	if (key)
		_keys.push_back(key);
}

Chore::Chore(EmiEngine *engine) {
	_engine = engine;
	_length = 0.0f;
	_name = "<unnamed chore>";
}

Chore::~Chore() {
	for (uint i = 0; i < _tracks.size(); i++) {
		delete _tracks[i];
		_tracks[i] = nullptr;
	}

	_tracks.clear();
	_name.clear();
}

void *Chore::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void Chore::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

int Chore::getSize() {
	int size = (int)(sizeof(Chore) + _name.size() + _tracks.size() * sizeof(ChoreTrack *));

	for (uint i = 0; i < _tracks.size(); i++) {
		if (_tracks[i])
			size += _tracks[i]->getSize();
	}

	return size;
}

ChoreTrack *Chore::addTrack(const LString &trackResourceName) {
	ChoreTrack *track = new ChoreTrack();

	track->_resourceName = trackResourceName;
	_tracks.push_back(track);

	return track;
}

int Chore::findTrackIndex(int trackId) {
	if (trackId >= 0) {
		for (uint i = 0; i < _tracks.size(); i++) {
			if (_tracks[i]->_id == trackId)
				return i;
		}
	}

	return -1;
}

void Chore::lockComponents() {
	LResourceHandle<Component> handle;

	for (uint i = 0; i < _tracks.size(); i++) {
		ChoreTrack *track = _tracks[i];

		if (!track) {
			warning("Chore::lockComponents(): missing track %u", i);
			continue;
		}

		{
			LResourceHandle<Component> trackHandle(_engine, track->_resourceName);

			handle.setResource(trackHandle._resource);
		}

		if (handle.lock() && handle._typed)
			handle._typed->resLock();

		handle.unlock();
	}
}

void Chore::unlockComponents() {
	LResourceHandle<Component> handle;

	for (uint i = 0; i < _tracks.size(); i++) {
		ChoreTrack *track = _tracks[i];

		if (!track) {
			warning("Chore::unlockComponents(): missing track %u", i);
			continue;
		}

		{
			LResourceHandle<Component> trackHandle(_engine, track->_resourceName);

			handle.setResource(trackHandle._resource);
		}

		if (handle.lock() && handle._typed)
			handle._typed->resUnlock();

		handle.unlock();
	}
}

ChoreSet::ChoreSet() {
}

ChoreSet::~ChoreSet() {
	for (uint i = 0; i < _chores.size(); i++) {
		delete _chores[i];
		_chores[i] = nullptr;
	}

	_chores.clear();
}

uint ChoreSet::resSizeOfItem() {
	uint size = sizeof(ChoreSet) + _chores.size() * sizeof(Chore *);

	for (uint i = 0; i < _chores.size(); i++)
		size += _chores[i]->getSize();

	return size;
}

Chore *ChoreSet::findChore(const LString &choreName) {
	for (uint i = 0; i < _chores.size(); i++) {
		if (_chores[i] && _chores[i]->_name == choreName)
			return _chores[i];
	}

	return nullptr;
}

void ChoreSet::logChoreError(const char *format, const char *argument) {
	LString message = LString::format(format, argument);
	LConsoleServices *console = _engine->getConsoleServices();

	if (console)
		console->logError("%s", message.c_str());
	else
		warning("%s", message.c_str());
}

bool ChoreSet::resLoadItem(const char *itemName) {
	LString textPath(itemName);
	LString binaryPath = textPath + "b";
	bool binaryExists = _engine->getFileServices()->exists(binaryPath.c_str());
	bool textExists = _engine->getFileServices()->exists(textPath.c_str());

	if (binaryExists) {
		if (!textExists || _engine->getFileServices()->getModTime(binaryPath.c_str()) >= _engine->getFileServices()->getModTime(textPath.c_str())) {
			if (!loadBinary(binaryPath.c_str())) {
				logChoreError("Parse problem in file '%s'.\n", binaryPath.c_str());

				return false;
			}

			return true;
		}
	}

	if (loadText(itemName)) {
		saveBinary(binaryPath.c_str());

		return true;
	}

	logChoreError("Parse problem in file '%s'.\n", itemName);

	return false;
}

bool ChoreSet::loadText(const char *path) {
	char section[kTextTokenSize];
	char lineBuffer[kLineBufferSize];
	LFile *file = _engine->getFileServices()->open(path, "rt");

	if (!file || !file->isOpen()) {
		logChoreError("Error: Invalid load filename '%s'.\n", path);

		return false;
	}

	section[0] = 0;

	if (file->readLine(lineBuffer, kLineBufferSize)) {
		while (sscanf(lineBuffer, " section: %127s", section) != 1 || parseSection(file, section)) {
			if (!file->readLine(lineBuffer, kLineBufferSize)) {
				_engine->getFileServices()->close(file, true);

				return true;
			}
		}

		logChoreError("Load of file %s failed due to syntax errors.\n", path);
		_engine->getFileServices()->close(file, true);

		return false;
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

bool ChoreSet::parseSection(LFile *file, const char *sectionName) {
	if (scumm_stricmp("chores", sectionName) != 0) {
		logChoreError("Warning: Unknown Section '%s'.\n", sectionName);
	} else if (!parseChores(file)) {
		logChoreError("Error while parsing section %s.\n", sectionName);

		return false;
	}

	return true;
}

bool ChoreSet::parseChores(LFile *file) {
	Chore *currentChore = nullptr;
	ChoreTrack *currentTrack = nullptr;
	char section[kTextTokenSize];
	char tag[kTextTokenSize];
	char trackName[kTextTokenSize];
	char lineBuffer[kLineBufferSize];
	int trackId;
	int parentId;
	float timeValue;
	float keyValue;
	float length;

	section[0] = 0;

	if (!file->readLine(lineBuffer, kLineBufferSize))
		return true;

	while (true) {
		if (sscanf(lineBuffer, " section: %127s\n", section) != 0) {
			if (!parseSection(file, section))
				return false;
		} else if (sscanf(lineBuffer, " chore %127s %f", section, &length) == 2) {
			currentChore = new Chore(_engine);
			currentChore->_name = section;
			currentChore->_length = length;
			_chores.push_back(currentChore);
		} else {
			if (!currentChore) {
				logChoreError("Unrecognized line in choreset file: '%s'\n", lineBuffer);

				return false;
			}

			if (sscanf(lineBuffer, " track %d %d %127s %127s", &trackId, &parentId, tag, trackName) == 4) {
				tag[kTagSize] = 0;

				int tagLength = strlen(tag);

				if (tagLength < kTagSize)
					memset(&tag[tagLength], ' ', kTagSize - tagLength);

				LString resourceName = LString("!") + tag + trackName;

				currentTrack = currentChore->addTrack(resourceName);
				currentTrack->_id = trackId;
				currentTrack->_parentId = parentId;
			} else if (currentTrack && sscanf(lineBuffer, " %f %f", &timeValue, &keyValue) == 2) {
				ChoreKey *key = new ChoreKey();

				key->time = timeValue;
				key->value = keyValue;
				currentTrack->addKey(key);
			} else {
				logChoreError("Unrecognized line in choreset file: '%s'\n", lineBuffer);

				return false;
			}
		}

		if (!file->readLine(lineBuffer, kLineBufferSize))
			return true;
	}
}

bool ChoreSet::loadBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "rb");

	if (!file)
		return false;

	int choreCount;
	char buffer[kNameBufferSize];

	if (!readInt(file, &choreCount)) {
		warning("ChoreSet::loadBinary(): unexpected end of file");
		_engine->getFileServices()->close(file, true);

		return false;
	}

	for (int i = 0; i < choreCount; i++) {
		int trackCount;
		Chore *chore = new Chore(_engine);

		_chores.push_back(chore);

		if (!readName(file, buffer) || !readFloat(file, &chore->_length) || !readInt(file, &trackCount)) {
			warning("ChoreSet::loadBinary(): invalid chore");
			_engine->getFileServices()->close(file, true);

			return false;
		}

		chore->_name = buffer;

		for (int j = 0; j < trackCount; j++) {
			int keyCount;

			if (!readName(file, buffer)) {
				warning("ChoreSet::loadBinary(): invalid track name");
				_engine->getFileServices()->close(file, true);

				return false;
			}

			ChoreTrack *track = chore->addTrack(LString(buffer));

			if (!readInt(file, &track->_id) || !readInt(file, &track->_parentId) || !readInt(file, &keyCount)) {
				warning("ChoreSet::loadBinary(): unexpected end of file");
				_engine->getFileServices()->close(file, true);

				return false;
			}

			for (int k = 0; k < keyCount; k++) {
				byte raw[kBinaryKeySize];

				if (file->read(raw, sizeof(raw)) != sizeof(raw)) {
					warning("ChoreSet::loadBinary(): unexpected end of file");
					_engine->getFileServices()->close(file, true);

					return false;
				}

				ChoreKey *key = new ChoreKey();

				key->time = READ_LE_FLOAT32(raw);
				key->value = READ_LE_FLOAT32(raw + 4);
				track->addKey(key);
			}
		}
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

bool ChoreSet::saveBinary(const char *path) {
	LFile *file = _engine->getFileServices()->open(path, "wb");

	if (!file)
		return false;

	writeInt(file, (int)_chores.size());

	for (uint i = 0; i < _chores.size(); i++) {
		Chore *chore = _chores[i];

		writeInt(file, (int)chore->_name.size() + 1);
		file->write(chore->_name.c_str(), chore->_name.size() + 1);
		writeFloat(file, chore->_length);
		writeInt(file, (int)chore->_tracks.size());

		for (uint j = 0; j < chore->_tracks.size(); j++) {
			ChoreTrack *track = chore->_tracks[j];

			writeInt(file, (int)track->_resourceName.size() + 1);
			file->write(track->_resourceName.c_str(), track->_resourceName.size() + 1);
			writeInt(file, track->_id);
			writeInt(file, track->_parentId);
			writeInt(file, (int)track->_keys.size());

			Common::List<ChoreKey *>::iterator it;

			for (it = track->_keys.begin(); it != track->_keys.end(); ++it) {
				writeFloat(file, (*it)->time);
				writeFloat(file, (*it)->value);
			}
		}
	}

	_engine->getFileServices()->close(file, true);

	return true;
}

} // End of namespace Grim
