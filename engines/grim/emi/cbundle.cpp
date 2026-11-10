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
#include "common/str.h"
#include "common/textconsole.h"

#include "engines/grim/emi/cbundle.h"
#include "engines/grim/emi/cpathingfileservices.h"
#include "engines/grim/emi/filefind.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

CBundleServices *gpBundleServices = nullptr;

Common::String CBundleServices::_currentIsland;

BundleLookup::BundleLookup() {
	file = nullptr;
	size = 0;
	offset = 0;
	inBundle = false;
	entry = nullptr;
	bundle = nullptr;
}

PBundleDirectoryEntry::PBundleDirectoryEntry() {
	reset();
}

void PBundleDirectoryEntry::reset() {
	hash = 0;
	name = nullptr;
	flags = 0;
	offset = 0;
	size = 0;
}

bool PBundleDirectoryEntry::isDirectoryFile() {
	return (flags & kBundleEntryDirectoryFile) != 0;
}

void PBundleDirectoryEntry::fillLookup(LFileServices *services, CBundle *bundle, BundleLookup *lookup) {
	lookup->name = name;
	lookup->path = Common::String::format("%s%s", bundle->name.c_str(), name);
	lookup->file = nullptr;

	if (services)
		lookup->size = services->getSize(lookup->path.c_str());
	else
		lookup->size = 0;

	lookup->bundle = bundle;
	lookup->offset = 0;
	lookup->inBundle = false;
	lookup->entry = this;
}

PBundleFileEntry::PBundleFileEntry() {
}

void PBundleFileEntry::fillLookup(LFileServices *services, CBundle *bundle, BundleLookup *lookup) {
	lookup->name = name;
	lookup->path = name;
	lookup->file = bundle->file;
	lookup->size = size;
	lookup->entry = this;
	lookup->offset = offset;
	lookup->inBundle = true;
	lookup->bundle = bundle;
}

CBundle::CBundle() {
	prev = nullptr;
	next = nullptr;
	reset();
}

CBundle::~CBundle() {
	close();
}

void CBundle::reset() {
	name.clear();
	file = nullptr;
	fileSize = 0;
	type = 0;
	numEntries = 0;
	nameTableSize = 0;
	entries.clear();
	nameTable = nullptr;
	hashIndex = nullptr;
}

void CBundle::close() {
	if (file)
		gpFileServices->close(file, true);

	for (uint i = 0; i < entries.size(); i++)
		delete entries[i];

	entries.clear();

	if (nameTable) {
		gpMemoryServices->free(nameTable);
		nameTable = nullptr;
	}

	delete hashIndex;
	hashIndex = nullptr;

	reset();
}

bool CBundle::openDirectory(const char *directory) {
	Common::String directoryName = directory;
	Common::Array<Common::String> fileNames;
	char fileName[260];
	uint bufferSize = 256;
	uint attributes;
	int nameBytes = 0;

	FileFind *finder = new FileFind("*.*", directoryName.c_str());

	while (finder->next(fileName, &bufferSize, &attributes)) {
		if ((attributes & kFileFindDirectory) == 0) {
			fileNames.push_back(fileName);
			nameBytes += strlen(fileName) + 1;
		}

		bufferSize = 256;
	}

	finder->close();
	delete finder;

	if (nameBytes < 1)
		nameBytes = 1;

	close();

	if (fileNames.empty()) {
		close();

		return numEntries != 0;
	}

	name = directoryName;
	type = kBundleTypeDirectory;
	numEntries = fileNames.size();

	nameTable = (char *)gpMemoryServices->malloc(nameBytes * sizeof(char));
	if (!nameTable) {
		close();

		return numEntries != 0;
	}

	nameTableSize = nameBytes;

	int offset = 0;

	for (uint i = 0; i < fileNames.size(); i++) {
		PBundleDirectoryEntry *entry = new PBundleDirectoryEntry();

		entry->name = &nameTable[offset];
		entry->hash = hashPath(fileNames[i].c_str(), &nameTable[offset]);
		entry->flags = kBundleEntryDirectoryFile;
		entries.push_back(entry);

		offset += fileNames[i].size() + 1;
	}

	buildHashIndex();

	return numEntries != 0;
}

bool CBundle::openBundleFile(const char *fileName) {
	Common::String path = ".\\";
	path += fileName;

	LFile *bundleFile = gpFileServices->open(path.c_str(), "rb");
	if (!bundleFile)
		return false;

	name = fileName;
	file = bundleFile;
	fileSize = gpFileServices->getSize(fileName);
	type = kBundleTypeFile;
	entries.clear();
	nameTable = nullptr;
	nameTableSize = 0;

	bool success = readHeader();

	if (success) {
		buildHashIndex();
	} else {
		if (gpConsoleServices)
			gpConsoleServices->logError("Unable to open bundle %s. It is not a valid bundle?\n", fileName);
		else
			warning("CBundle::openBundleFile(): unable to open bundle %s, it is not a valid bundle?", fileName);

		close();
	}

	return success;
}

bool CBundle::readHeader() {
	char magic[4];
	uint value;

	if (file->read(magic, 4) != 4 || strncmp(magic, "LABN", 4) != 0) {
		close();

		return false;
	}

	if (file->read(&value, 4) != 4 || FROM_LE_32(value) > 0x10000) {
		close();

		return false;
	}

	if (file->read(&value, 4) != 4) {
		close();

		return false;
	}

	uint numFiles = FROM_LE_32(value);

	if (file->read(&value, 4) != 4) {
		close();

		return false;
	}

	uint tableSize = FROM_LE_32(value);

	if (file->read(&value, 4) != 4) {
		close();

		return false;
	}

	int tableOffset = FROM_LE_32(value) - 81167;

	// Each entry takes 16 bytes of the file
	if (numFiles > (uint)fileSize / 16) {
		warning("CBundle::readHeader(): too many entries (%u)", numFiles);
		close();

		return false;
	}

	for (uint i = 0; i < numFiles; i++)
		entries.push_back(new PBundleFileEntry());

	nameTable = (char *)gpMemoryServices->malloc(tableSize * sizeof(char));
	if (!nameTable) {
		close();

		return false;
	}

	Common::Array<char *> names;

	for (uint i = 0; i < numFiles; i++) {
		PBundleDirectoryEntry *entry = entries[i];

		if (file->read(&value, 4) != 4) {
			close();

			return false;
		}

		uint nameOffset = FROM_LE_32(value);

		if (nameOffset >= tableSize) {
			warning("CBundle::readHeader(): name offset out of the name table");
			close();

			return false;
		}

		entry->name = &nameTable[nameOffset];
		names.push_back(&nameTable[nameOffset]);

		if (file->read(&value, 4) != 4) {
			close();

			return false;
		}

		entry->offset = FROM_LE_32(value);

		if (file->read(&value, 4) != 4) {
			close();

			return false;
		}

		entry->size = FROM_LE_32(value);

		if (file->read(&value, 4) != 4) {
			close();

			return false;
		}

		entry->flags = kBundleEntryBundleFile;
	}

	file->seek(tableOffset, 0);

	if (file->read(nameTable, tableSize) != tableSize) {
		close();

		return false;
	}

	for (uint i = 0; i < numFiles; i++) {
		if (!memchr(names[i], 0, nameTable + tableSize - names[i])) {
			warning("CBundle::readHeader(): name without terminator");
			close();

			return false;
		}

		decodeName(names[i]);
		entries[i]->hash = hashPath(names[i], names[i]);
	}

	return true;
}

void CBundle::buildHashIndex() {
	if (!hashIndex)
		hashIndex = new BundleHashIndex();

	for (uint i = 0; i < entries.size(); i++) {
		PBundleDirectoryEntry *entry = entries[i];

		if (!entry->hash || !entry->name)
			break;

		(*hashIndex)[entry->hash].push_back(entry);
	}
}

bool CBundle::findBundleEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup) {
	if (findHashedEntry(services, normalizedName, hash, lookup))
		return true;

	for (uint i = 0; i < entries.size(); i++) {
		PBundleDirectoryEntry *entry = entries[i];

		if (!entry->name)
			return false;

		if (scumm_stricmp(entry->name, normalizedName) == 0) {
			entry->fillLookup(services, this, lookup);

			return true;
		}
	}

	return false;
}

bool CBundle::findDirectoryEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup) {
	return findHashedEntry(services, normalizedName, hash, lookup);
}

bool CBundle::findHashedEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup) {
	if (!hashIndex || !hashIndex->contains(hash))
		return false;

	Common::Array<PBundleDirectoryEntry *> &bucket = (*hashIndex)[hash];

	for (uint i = 0; i < bucket.size(); i++) {
		PBundleDirectoryEntry *entry = bucket[i];

		if (entry->name && scumm_stricmp(entry->name, normalizedName) == 0) {
			entry->fillLookup(services, this, lookup);

			return true;
		}
	}

	return false;
}

LBundleFile::LBundleFile() {
	reset();
}

LBundleFile::~LBundleFile() {
	// The stream of a member of a bundle belongs to the bundle
	if (!unbuffered)
		readStream = nullptr;
}

void LBundleFile::reset() {
	LFile::reset();

	filePos = 0;
	bufPos = 0;
	bufAvail = 0;
	memberSize = 0;
	baseOffset = 0;
	atEnd = false;
	unbuffered = false;
	bufCapacity = 0;
	openMode[0] = 0;
	cache[0] = 0;
}

uint LBundleFile::read(void *dest, uint count) {
	if (!isOpen())
		return 0;

	if (unbuffered || memData)
		return LFile::read(dest, count);

	if (count < bufCapacity && count > bufAvail)
		fillBuffer();

	if (count > bufAvail) {
		uint available = bufAvail;

		if (available)
			memcpy(dest, &cache[bufPos], available);

		filePos += available;
		bufPos += available;
		bufAvail = 0;

		uint readBytes = readAt((byte *)dest + available, filePos, count - available);

		filePos += readBytes;
		atEnd = filePos >= memberSize;

		return available + readBytes;
	}

	memcpy(dest, &cache[bufPos], count);

	filePos += count;
	bufPos += count;
	bufAvail -= count;
	atEnd = filePos >= memberSize;

	return count;
}

char *LBundleFile::readLine(char *dest, int maxCount) {
	char *out = dest;
	char character = 0;
	int consumed = 0;

	if (!isOpen())
		return nullptr;

	if (unbuffered || memData)
		return LFile::readLine(dest, maxCount);

	if (maxCount <= 0) {
		warning("LBundleFile::readLine(): invalid buffer size");

		return nullptr;
	}

	int limit = maxCount;

	if (bufAvail < (uint)maxCount)
		fillBuffer();

	if (bufAvail < (uint)maxCount)
		limit = bufAvail + 1;

	int remaining = limit - 1;
	const byte *src = &cache[bufPos];
	bool full = remaining == 0;

	if (remaining > 0) {
		do {
			character = *src++;
			consumed++;

			if (character == 10)
				break;

			if (character == 13)
				break;

			*out++ = character;
			remaining--;
		} while (remaining > 0);

		full = remaining == 0;
	}

	if (!full)
		*out++ = 10;

	*out = 0;

	if (character == 13) {
		if (bufPos + consumed >= kBundleFileBufferSize)
			warning("LBundleFile::readLine(): line ends at the end of the buffer");
		else if (*src == 10)
			consumed++;
	}

	filePos += consumed;
	bufPos += consumed;
	bufAvail -= consumed;

	if (filePos < memberSize) {
		atEnd = false;

		return dest;
	}

	atEnd = true;

	return nullptr;
}

int LBundleFile::eof() {
	if (!isOpen())
		return 1;

	if (unbuffered || memData)
		return LFile::eof();

	return atEnd;
}

int LBundleFile::tell() {
	if (!isOpen())
		return -1;

	if (unbuffered || memData)
		return LFile::tell();

	return filePos;
}

int LBundleFile::seek(int offset, int whence) {
	int position;

	if (!isOpen())
		return 0;

	if (unbuffered || memData)
		return LFile::seek(offset, whence);

	if (whence == 0)
		position = offset;
	else if (whence == 1)
		position = filePos + offset;
	else if (whence == 2)
		position = memberSize + offset;
	else
		return 0;

	int bufferStart = filePos - bufPos;
	int bufferEnd = filePos + bufAvail;

	if (position < bufferStart || position >= bufferEnd - 1) {
		bufAvail = 0;
		bufPos = 0;
		filePos = position;

		return 1;
	}

	filePos = position;
	bufPos = position - bufferStart;
	bufAvail = bufferEnd - position;

	return 1;
}

int LBundleFile::writeFormat(const char *format, ...) {
	if (!isOpen() || !unbuffered)
		return 0;

	va_list args;
	va_start(args, format);
	LString text = LString::vformat(format, args);
	va_end(args);

	return LFile::write(text.c_str(), text.size());
}

uint LBundleFile::loadAll() {
	if (memData)
		return 0;

	if (unbuffered)
		return LFile::loadAll();

	uint allocSize = memberSize + 1;

	char *data = (char *)gpMemoryServices->malloc(allocSize * sizeof(char));
	if (!data) {
		warning("LBundleFile::loadAll(): cannot allocate the buffer");

		return 0;
	}

	memSize = readAt(data, 0, memberSize);
	memData = data;
	memCursor = data;
	data[allocSize - 1] = 0;

	return memSize;
}

const char *LBundleFile::getName() {
	return name.c_str();
}

uint LBundleFile::writeChecked(const void *src, uint count) {
	if (isOpen() && unbuffered)
		return LFile::write(src, count);

	return 0;
}

void LBundleFile::fillBuffer() {
	uint capacity = bufCapacity;
	byte *dest = cache;

	if (bufAvail) {
		memmove(cache, &cache[bufPos], bufAvail);
		capacity -= bufAvail;
		dest += bufAvail;
	}

	uint readBytes = readAt(dest, bufAvail + filePos, capacity);

	bufPos = 0;
	bufAvail += readBytes;
}

uint LBundleFile::readAt(void *dest, int position, uint count) {
	if (position < 0) {
		warning("LBundleFile::readAt(): negative position");

		return 0;
	}

	if (position >= memberSize)
		return 0;

	uint toRead = count;

	if ((int)(position + count) > memberSize)
		toRead = memberSize - position;

	LFile::seek(position + baseOffset, 0);

	return LFile::read(dest, toRead);
}

CBundleServices::CBundleServices(LFileServices *innerServices) {
	numBundles = 0;
	head = nullptr;
	tail = nullptr;
	logFile = nullptr;
	inner = innerServices;

	if (!gpBundleServices)
		gpBundleServices = this;
}

CBundleServices::~CBundleServices() {
	while (numBundles) {
		CBundle *bundle = head;
		CBundle *nextBundle = bundle->next;

		head = nextBundle;

		if (nextBundle)
			nextBundle->prev = nullptr;
		else
			tail = nullptr;

		bundle->prev = nullptr;
		bundle->next = nullptr;
		numBundles--;

		delete bundle;
	}

	closeLog();

	if (gpBundleServices == this)
		gpBundleServices = nullptr;
}

LFile *CBundleServices::open(const char *path, const char *mode) {
	uint index = 0;

	while (index < kBundleMaxOpenFiles) {
		if (!files[index].isOpen())
			break;

		index++;
	}

	if (index == kBundleMaxOpenFiles) {
		if (gpConsoleServices)
			gpConsoleServices->logError("Cannot open %s, too many files open!\n", path);
		else
			warning("CBundleServices::open(): cannot open %s, too many files open!", path);

		return nullptr;
	}

	LBundleFile *slot = &files[index];

	slot->reset();
	slot->bufCapacity = kBundleFileBufferSize;

	if (strlen(mode) >= sizeof(slot->openMode))
		warning("CBundleServices::open(): mode too long");

	Common::strlcpy(slot->openMode, mode, sizeof(slot->openMode));

	if (*mode == 'r' || *mode == 'R') {
		BundleLookup result;

		if (findFile(path, &result)) {
			if (result.inBundle) {
				slot->readStream = result.file->readStream;
				slot->unbuffered = false;
				slot->fileName = path;
				slot->name.clear();
			} else {
				inner->openInto(slot, result.path.c_str(), mode);
				slot->name = result.path;
				slot->unbuffered = true;

				int separator = (int)slot->name.findLastOf('\\');
				if (separator > 0)
					slot->name = slot->name.substr(0, separator + 1);

				if ((int)(result.path.size() - result.name.size()) < 0)
					slot->name.clear();
			}

			slot->memberSize = result.size;
			slot->baseOffset = result.offset;

			if (slot->name.size()) {
				if (logFile)
					logFile->writeFormat("%s%s\n", slot->name.c_str(), path);
			} else if (logFile) {
				logFile->writeFormat("%s\n", path);
			}

			return slot;
		}
	}

	inner->openInto(slot, path, mode);

	if (!slot->isOpen()) {
		CPathingFileServices::suspendCdPath();
		inner->openInto(slot, path, mode);
		CPathingFileServices::resumeCdPath();

		if (!slot->isOpen())
			return nullptr;
	}

	slot->unbuffered = true;

	if (*mode == 'w') {
		CBundle *bundle = head;

		while (bundle) {
			if (slot->name == bundle->name) {
				Common::String directory = bundle->name;

				bundle->openDirectory(directory.c_str());

				break;
			}

			bundle = bundle->next;
		}
	}

	return slot;
}

LFile *CBundleServices::openExtended(const char *path, const char *mode, int unused1, int unused2) {
	return open(path, mode);
}

bool CBundleServices::exists(const char *path) {
	LString pathString = path;

	if (lookup(pathString))
		return true;

	return inner->exists(path);
}

int CBundleServices::getModTime(const char *path) {
	LString pathString = path;

	if (lookup(pathString))
		return 0;

	return inner->getModTime(path);
}

void CBundleServices::close(LFile *file, bool destroy) {
	LBundleFile *slot = nullptr;

	for (uint i = 0; i < kBundleMaxOpenFiles; i++) {
		if (file == &files[i]) {
			slot = &files[i];

			break;
		}
	}

	if (!slot) {
		inner->close(file, destroy);

		return;
	}

	if (!slot->isOpen())
		return;

	if (slot->unbuffered)
		inner->close(slot, false);

	if (slot->memData) {
		gpMemoryServices->free(slot->memData);
		slot->memData = nullptr;
		slot->memCursor = nullptr;
		slot->memSize = 0;
	}

	slot->reset();
}

int CBundleServices::getSize(const char *path) {
	BundleLookup result;

	if (findFile(path, &result))
		return result.size;

	return inner->getSize(path);
}

bool CBundleServices::resolvePath(LString &outPath, const char *name) {
	BundleLookup result;

	if (findFile(name, &result)) {
		outPath = name;

		return true;
	}

	return inner->resolvePath(outPath, name);
}

CBundle *CBundleServices::openBundle(const char *name) {
	CBundle *bundle = findBundle(name);
	if (bundle)
		return bundle;

	bundle = newBundle();

	bool success;

	if (!*name || name[strlen(name) - 1] == '\\') {
		success = bundle->openDirectory(name);

		if (logFile)
			logFile->writeFormat("Opening Directory: %s\t-", name);
	} else {
		success = bundle->openBundleFile(name);

		if (logFile)
			logFile->writeFormat("Opening Bundle: %s\t-", name);
	}

	if (success) {
		if (logFile)
			logFile->writeFormat("SUCCESS\n");

		return bundle;
	}

	bundle->close();
	removeBundle(bundle);

	if (logFile)
		logFile->writeFormat("FAILED\n");

	return nullptr;
}

void CBundleServices::closeBundle(const char *name) {
	CBundle *bundle = findBundle(name);
	if (!bundle)
		return;

	if (bundle->type & kBundleTypeFile) {
		close(bundle->file, true);

		// The disasm closes the file twice here, let's avoid that...
		bundle->file = nullptr;
	}

	removeBundle(bundle);
}

bool CBundleServices::moveBundleToFront(const char *name) {
	CBundle *bundle = findBundle(name);
	if (!bundle)
		return false;

	bool linked = true;

	if (bundle == head) {
		CBundle *nextBundle = head->next;

		head = nextBundle;

		if (nextBundle)
			nextBundle->prev = nullptr;
		else
			tail = nullptr;

		bundle->prev = nullptr;
		bundle->next = nullptr;
	} else if (bundle == tail) {
		CBundle *prevBundle = tail->prev;

		tail = prevBundle;

		if (!prevBundle)
			head = nullptr;
		else
			prevBundle->next = nullptr;

		bundle->prev = nullptr;
		bundle->next = nullptr;
	} else {
		CBundle *nextBundle = bundle->next;

		if (!nextBundle && !bundle->prev) {
			linked = false;
		} else {
			nextBundle->prev = bundle->prev;
			bundle->prev->next = bundle->next;
		}
	}

	if (linked) {
		numBundles--;
		bundle->prev = nullptr;
		bundle->next = nullptr;
	}

	if (head)
		head->prev = bundle;

	bundle->prev = nullptr;
	bundle->next = head;

	CBundle *oldTail = tail;

	head = bundle;

	if (!oldTail)
		tail = bundle;

	numBundles++;

	return true;
}

bool CBundleServices::lookup(const LString &path) {
	BundleLookup result;

	return findFile(path.c_str(), &result);
}

bool CBundleServices::findFile(const char *path, BundleLookup *result) {
	char normalized[260];
	bool found = false;

	if (!numBundles)
		return false;

	if (strlen(path) >= sizeof(normalized)) {
		warning("CBundleServices::findFile(): path too long");

		return false;
	}

	uint hash = CBundle::hashPath(path, normalized);
	CBundle *bundle = head;

	while (bundle && !found) {
		if (bundle->type & kBundleTypeFile)
			found = bundle->findBundleEntry(inner, normalized, hash, result);
		else
			found = bundle->findDirectoryEntry(inner, normalized, hash, result);

		bundle = bundle->next;
	}

	return found;
}

CBundle *CBundleServices::findBundle(const char *name) {
	CBundle *bundle = head;

	while (bundle) {
		if (bundle->name == name)
			return bundle;

		bundle = bundle->next;
	}

	return nullptr;
}

CBundle *CBundleServices::newBundle() {
	CBundle *bundle = new CBundle();

	if (head)
		head->prev = bundle;

	bundle->prev = nullptr;
	bundle->next = head;

	CBundle *oldTail = tail;

	head = bundle;

	if (!oldTail)
		tail = bundle;

	numBundles++;

	return bundle;
}

void CBundleServices::removeBundle(CBundle *bundle) {
	bool linked = true;

	if (bundle == head) {
		CBundle *nextBundle = head->next;

		head = nextBundle;

		if (nextBundle)
			nextBundle->prev = nullptr;
		else
			tail = nullptr;

		bundle->prev = nullptr;
		bundle->next = nullptr;
	} else if (bundle == tail) {
		CBundle *prevBundle = tail->prev;

		tail = prevBundle;

		if (!prevBundle)
			head = nullptr;
		else
			prevBundle->next = nullptr;

		bundle->prev = nullptr;
		bundle->next = nullptr;
	} else {
		CBundle *nextBundle = bundle->next;

		if (!nextBundle && !bundle->prev) {
			linked = false;
		} else {
			nextBundle->prev = bundle->prev;
			bundle->prev->next = bundle->next;
		}
	}

	if (linked) {
		numBundles--;
		bundle->prev = nullptr;
		bundle->next = nullptr;
	}

	bundle->close();
	delete bundle;
}

void CBundleServices::closeLog() {
	if (logFile)
		inner->close(logFile, true);

	logFile = nullptr;
}

bool CBundleServices::openLocalBundles() {
	bool failed = openBundle("local.m4b") == nullptr;

	openBundle("patch.m4b");

	return !failed;
}

bool CBundleServices::openCommonBundles() {
	bool failed = openBundle("lip.m4b") == nullptr;

	failed = openBundle("voiceAll.m4b") == nullptr || failed;
	failed = openBundle("sfx.m4b") == nullptr || failed;
	failed = openBundle("artAll.m4b") == nullptr || failed;

	openBundle("i9n.m4b");
	moveBundleToFront("patch.m4b");

	return !failed;
}

bool CBundleServices::resetBundles() {
	while (numBundles) {
		CBundle *bundle = head;
		CBundle *nextBundle = bundle->next;

		head = nextBundle;

		if (nextBundle)
			nextBundle->prev = nullptr;
		else
			tail = nullptr;

		bundle->prev = nullptr;
		bundle->next = nullptr;
		numBundles--;

		delete bundle;
	}

	return openLocalBundles();
}

uint CBundle::hashPath(const char *path, char *normalized) {
	uint length = strlen(path);
	uint hash = (((length | ((length | (length << 8)) << 8)) << 8) | length);
	const char *in = path;
	char *out = normalized;

	int8 character = (int8)toupper((byte)*in++);

	while (character) {
		if (character == '/')
			character = '\\';

		*out++ = character;
		hash = (uint)(int)character ^ ((hash << 1) | (hash >> 31));
		character = (int8)toupper((byte)*in++);
	}

	*out = 0;

	return hash;
}

int CBundle::decodeName(char *name) {
	int count = 0;
	int length = strlen(name);

	while (count < length) {
		name[count] ^= 0x96;
		count++;
	}

	return count;
}

bool CBundleServices::init() {
	if (gpBundleServices)
		return true;

	_currentIsland = "unknown";

	CBundleServices *services = new CBundleServices(gpFileServices);
	bool localOpened = services->openLocalBundles();

	services->openCommonBundles();

	if (!localOpened || isI9nBundleOpen()) {
		delete services;

		return false;
	}

	LFileServices::pushServices(services);

	return true;
}

bool CBundleServices::isI9nBundleOpen() {
	return gpBundleServices && gpBundleServices->findBundle("i9n.m4b") != nullptr;
}

bool CBundleServices::openIslandBundles(const char *island) {
	bool failed = false;

	if (!gpBundleServices)
		return false;

	if (scumm_stricmp(island, "melee") == 0) {
		bool voiceFailed = gpBundleServices->openBundle("voiceMel.m4b") == nullptr;
		CBundle *art = gpBundleServices->openBundle("artMel.m4b");

		failed = art == nullptr || voiceFailed;
	} else if (scumm_stricmp(island, "lucre") == 0) {
		bool voiceFailed = gpBundleServices->openBundle("voiceLuc.m4b") == nullptr;
		CBundle *art = gpBundleServices->openBundle("artLuc.m4b");

		failed = art == nullptr || voiceFailed;
	} else if (scumm_stricmp(island, "jambalaya") == 0) {
		bool voiceFailed = gpBundleServices->openBundle("voiceJam.m4b") == nullptr;
		CBundle *art = gpBundleServices->openBundle("artJam.m4b");

		failed = art == nullptr || voiceFailed;
	} else if (scumm_stricmp(island, "monkey") == 0) {
		bool voiceFailed = gpBundleServices->openBundle("voiceMon.m4b") == nullptr;
		CBundle *art = gpBundleServices->openBundle("artMon.m4b");

		failed = art == nullptr || voiceFailed;
	}

	if (failed)
		return false;

	gpBundleServices->moveBundleToFront("i9n.m4b");
	gpBundleServices->moveBundleToFront("patch.m4b");
	_currentIsland = island;

	return true;
}

void CBundleServices::closeIslandBundles(const char *island) {
	if (!gpBundleServices)
		return;

	if (scumm_stricmp(island, "melee") == 0) {
		gpBundleServices->closeBundle("voiceMel.m4b");
		gpBundleServices->closeBundle("artMel.m4b");
	} else if (scumm_stricmp(island, "lucre") == 0) {
		gpBundleServices->closeBundle("voiceLuc.m4b");
		gpBundleServices->closeBundle("artLuc.m4b");
	} else if (scumm_stricmp(island, "jambalaya") == 0) {
		gpBundleServices->closeBundle("voiceJam.m4b");
		gpBundleServices->closeBundle("artJam.m4b");
	} else if (scumm_stricmp(island, "monkey") == 0) {
		gpBundleServices->closeBundle("voiceMon.m4b");
		gpBundleServices->closeBundle("artMon.m4b");
	}

	_currentIsland = "unknown";
}

bool CBundleServices::isIslandBundleOpen(const char *island) {
	if (!gpBundleServices)
		return false;

	if (scumm_stricmp(island, "melee") == 0)
		return gpBundleServices->findBundle("artMel.m4b") != nullptr;

	if (scumm_stricmp(island, "lucre") == 0)
		return gpBundleServices->findBundle("artLuc.m4b") != nullptr;

	if (scumm_stricmp(island, "jambalaya") == 0)
		return gpBundleServices->findBundle("artJam.m4b") != nullptr;

	if (scumm_stricmp(island, "monkey") == 0)
		return gpBundleServices->findBundle("artMon.m4b") != nullptr;

	return gpBundleServices->findBundle(island) != nullptr;
}

bool CBundleServices::setDiscBundles(int disc) {
	if (!gpBundleServices)
		return true;

	if (disc)
		return gpBundleServices->openCommonBundles();

	return gpBundleServices->resetBundles();
}

} // End of namespace Grim
