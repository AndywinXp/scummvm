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

#include "engines/grim/emi/choreset.h"
#include "engines/grim/emi/component.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"
#include "engines/grim/emi/lresourcehandle.h"

namespace Grim {

static const int kNameBufferSize = 256;
static const int kTextTokenSize = 128;
static const int kTagSize = 4;
static const int kBinaryKeySize = 8;

char ChoreSet::_lineBuffer[kLineBufferSize];

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

static void logChoreError(const char *format, const char *argument) {
	LString message = LString::format(format, argument);

	if (gpConsoleServices)
		gpConsoleServices->logError("%s", message.c_str());
	else
		warning("%s", message.c_str());
}

void *ChoreKey::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void ChoreKey::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

ChoreTrack::ChoreTrack() {
	id = -1;
	parentId = -1;
}

ChoreTrack::~ChoreTrack() {
	Common::List<ChoreKey *>::iterator it;

	for (it = keys.begin(); it != keys.end(); ++it) {
		delete *it;
		*it = nullptr;
	}

	keys.clear();
}

void *ChoreTrack::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void ChoreTrack::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

int ChoreTrack::getSize() {
	return (int)(resourceName.size() + 3 * sizeof(void *) * (keys.size() + 3));
}

void ChoreTrack::addKey(ChoreKey *key) {
	if (key)
		keys.push_back(key);
}

Chore::Chore() {
	length = 0.0f;
	name = "<unnamed chore>";
}

Chore::~Chore() {
	for (uint i = 0; i < tracks.size(); i++) {
		delete tracks[i];
		tracks[i] = nullptr;
	}

	tracks.clear();
	name.clear();
}

void *Chore::operator new(size_t size) {
	return gpMemoryServices->mallocImpl(size);
}

void Chore::operator delete(void *block) {
	gpMemoryServices->freeImpl(block);
}

int Chore::getSize() {
	int size = (int)(sizeof(Chore) + name.size() + tracks.size() * sizeof(ChoreTrack *));

	for (uint i = 0; i < tracks.size(); i++) {
		if (tracks[i])
			size += tracks[i]->getSize();
	}

	return size;
}

ChoreTrack *Chore::addTrack(const LString &trackResourceName) {
	ChoreTrack *track = new ChoreTrack();

	track->resourceName = trackResourceName;
	tracks.push_back(track);

	return track;
}

int Chore::findTrackIndex(int trackId) {
	if (trackId >= 0) {
		for (uint i = 0; i < tracks.size(); i++) {
			if (tracks[i]->id == trackId)
				return i;
		}
	}

	return -1;
}

void Chore::lockComponents() {
	LResourceHandle<Component> handle;

	for (uint i = 0; i < tracks.size(); i++) {
		ChoreTrack *track = tracks[i];

		if (!track) {
			warning("Chore::lockComponents(): missing track %u", i);
			continue;
		}

		{
			LResourceHandle<Component> trackHandle(track->resourceName);

			handle.setResource(trackHandle.resource);
		}

		if (handle.lock() && handle.typed)
			handle.typed->resLock();

		handle.unlock();
	}
}

void Chore::unlockComponents() {
	LResourceHandle<Component> handle;

	for (uint i = 0; i < tracks.size(); i++) {
		ChoreTrack *track = tracks[i];

		if (!track) {
			warning("Chore::unlockComponents(): missing track %u", i);
			continue;
		}

		{
			LResourceHandle<Component> trackHandle(track->resourceName);

			handle.setResource(trackHandle.resource);
		}

		if (handle.lock() && handle.typed)
			handle.typed->resUnlock();

		handle.unlock();
	}
}

ChoreSet::ChoreSet() {
}

ChoreSet::~ChoreSet() {
	for (uint i = 0; i < chores.size(); i++) {
		delete chores[i];
		chores[i] = nullptr;
	}

	chores.clear();
}

uint ChoreSet::resSizeOfItem() {
	uint size = sizeof(ChoreSet) + chores.size() * sizeof(Chore *);

	for (uint i = 0; i < chores.size(); i++)
		size += chores[i]->getSize();

	return size;
}

Chore *ChoreSet::findChore(const LString &choreName) {
	for (uint i = 0; i < chores.size(); i++) {
		if (chores[i] && chores[i]->name == choreName)
			return chores[i];
	}

	return nullptr;
}

bool ChoreSet::resLoadItem(const char *itemName) {
	LString textPath(itemName);
	LString binaryPath = textPath + "b";
	bool binaryExists = gpFileServices->exists(binaryPath.c_str());
	bool textExists = gpFileServices->exists(textPath.c_str());

	if (binaryExists) {
		if (!textExists || gpFileServices->getModTime(binaryPath.c_str()) >= gpFileServices->getModTime(textPath.c_str())) {
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
	LFile *file = gpFileServices->open(path, "rt");

	if (!file || !file->isOpen()) {
		logChoreError("Error: Invalid load filename '%s'.\n", path);

		return false;
	}

	section[0] = 0;

	if (file->readLine(_lineBuffer, kLineBufferSize)) {
		while (sscanf(_lineBuffer, " section: %127s", section) != 1 || parseSection(file, section)) {
			if (!file->readLine(_lineBuffer, kLineBufferSize)) {
				gpFileServices->close(file, true);

				return true;
			}
		}

		logChoreError("Load of file %s failed due to syntax errors.\n", path);
		gpFileServices->close(file, true);

		return false;
	}

	gpFileServices->close(file, true);

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
	int trackId;
	int parentId;
	float timeValue;
	float keyValue;
	float length;

	section[0] = 0;

	if (!file->readLine(_lineBuffer, kLineBufferSize))
		return true;

	while (true) {
		if (sscanf(_lineBuffer, " section: %127s\n", section) != 0) {
			if (!parseSection(file, section))
				return false;
		} else if (sscanf(_lineBuffer, " chore %127s %f", section, &length) == 2) {
			currentChore = new Chore();
			currentChore->name = section;
			currentChore->length = length;
			chores.push_back(currentChore);
		} else {
			if (!currentChore) {
				logChoreError("Unrecognized line in choreset file: '%s'\n", _lineBuffer);

				return false;
			}

			if (sscanf(_lineBuffer, " track %d %d %127s %127s", &trackId, &parentId, tag, trackName) == 4) {
				tag[kTagSize] = 0;

				int tagLength = strlen(tag);

				if (tagLength < kTagSize)
					memset(&tag[tagLength], ' ', kTagSize - tagLength);

				LString resourceName = LString("!") + tag + trackName;

				currentTrack = currentChore->addTrack(resourceName);
				currentTrack->id = trackId;
				currentTrack->parentId = parentId;
			} else if (currentTrack && sscanf(_lineBuffer, " %f %f", &timeValue, &keyValue) == 2) {
				ChoreKey *key = new ChoreKey();

				key->time = timeValue;
				key->value = keyValue;
				currentTrack->addKey(key);
			} else {
				logChoreError("Unrecognized line in choreset file: '%s'\n", _lineBuffer);

				return false;
			}
		}

		if (!file->readLine(_lineBuffer, kLineBufferSize))
			return true;
	}
}

bool ChoreSet::loadBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "rb");

	if (!file)
		return false;

	int choreCount;
	char buffer[kNameBufferSize];

	if (!readInt(file, &choreCount)) {
		warning("ChoreSet::loadBinary(): unexpected end of file");
		gpFileServices->close(file, true);

		return false;
	}

	for (int i = 0; i < choreCount; i++) {
		int trackCount;
		Chore *chore = new Chore();

		chores.push_back(chore);

		if (!readName(file, buffer) || !readFloat(file, &chore->length) || !readInt(file, &trackCount)) {
			warning("ChoreSet::loadBinary(): invalid chore");
			gpFileServices->close(file, true);

			return false;
		}

		chore->name = buffer;

		for (int j = 0; j < trackCount; j++) {
			int keyCount;

			if (!readName(file, buffer)) {
				warning("ChoreSet::loadBinary(): invalid track name");
				gpFileServices->close(file, true);

				return false;
			}

			ChoreTrack *track = chore->addTrack(LString(buffer));

			if (!readInt(file, &track->id) || !readInt(file, &track->parentId) || !readInt(file, &keyCount)) {
				warning("ChoreSet::loadBinary(): unexpected end of file");
				gpFileServices->close(file, true);

				return false;
			}

			for (int k = 0; k < keyCount; k++) {
				byte raw[kBinaryKeySize];

				if (file->read(raw, sizeof(raw)) != sizeof(raw)) {
					warning("ChoreSet::loadBinary(): unexpected end of file");
					gpFileServices->close(file, true);

					return false;
				}

				ChoreKey *key = new ChoreKey();

				key->time = READ_LE_FLOAT32(raw);
				key->value = READ_LE_FLOAT32(raw + 4);
				track->addKey(key);
			}
		}
	}

	gpFileServices->close(file, true);

	return true;
}

bool ChoreSet::saveBinary(const char *path) {
	LFile *file = gpFileServices->open(path, "wb");

	if (!file)
		return false;

	writeInt(file, (int)chores.size());

	for (uint i = 0; i < chores.size(); i++) {
		Chore *chore = chores[i];

		writeInt(file, (int)chore->name.size() + 1);
		file->write(chore->name.c_str(), chore->name.size() + 1);
		writeFloat(file, chore->length);
		writeInt(file, (int)chore->tracks.size());

		for (uint j = 0; j < chore->tracks.size(); j++) {
			ChoreTrack *track = chore->tracks[j];

			writeInt(file, (int)track->resourceName.size() + 1);
			file->write(track->resourceName.c_str(), track->resourceName.size() + 1);
			writeInt(file, track->id);
			writeInt(file, track->parentId);
			writeInt(file, (int)track->keys.size());

			Common::List<ChoreKey *>::iterator it;

			for (it = track->keys.begin(); it != track->keys.end(); ++it) {
				writeFloat(file, (*it)->time);
				writeFloat(file, (*it)->value);
			}
		}
	}

	gpFileServices->close(file, true);

	return true;
}

} // End of namespace Grim
