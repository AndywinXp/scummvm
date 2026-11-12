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

#include "engines/grim/emi/resources/cbundle.h"
#include "engines/grim/emi/core/cpathingfileservices.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/files/filefind.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/core/lmemoryservices.h"

namespace Grim {

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

void *PBundleDirectoryEntry::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void PBundleDirectoryEntry::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void PBundleDirectoryEntry::reset() {
	_hash = 0;
	_name = nullptr;
	_flags = 0;
	_offset = 0;
	_size = 0;
}

bool PBundleDirectoryEntry::isDirectoryFile() {
	return (_flags & kBundleEntryDirectoryFile) != 0;
}

void PBundleDirectoryEntry::fillLookup(LFileServices *services, CBundle *bundle, BundleLookup *lookup) {
	lookup->name = _name;
	lookup->path = Common::String::format("%s%s", bundle->_name.c_str(), _name);
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
	lookup->name = _name;
	lookup->path = _name;
	lookup->file = bundle->_file;
	lookup->size = _size;
	lookup->entry = this;
	lookup->offset = _offset;
	lookup->inBundle = true;
	lookup->bundle = bundle;
}

CBundle::CBundle(EmiEngine *engine) : _engine(engine) {
	_prev = nullptr;
	_next = nullptr;
	reset();
}

CBundle::~CBundle() {
	close();
}

void *CBundle::operator new(size_t size) {
	return LMemoryServices::getCurrent()->mallocImpl(size);
}

void CBundle::operator delete(void *block) {
	LMemoryServices::getCurrent()->freeImpl(block);
}

void CBundle::reset() {
	_name.clear();
	_file = nullptr;
	_fileSize = 0;
	_type = 0;
	_numEntries = 0;
	_nameTableSize = 0;
	_entries.clear();
	_nameTable = nullptr;
	_hashIndex = nullptr;
}

void CBundle::close() {
	if (_file)
		_engine->getFileServices()->close(_file, true);

	for (uint i = 0; i < _entries.size(); i++)
		delete _entries[i];

	_entries.clear();

	if (_nameTable) {
		_engine->getMemoryServices()->freeImpl(_nameTable);
		_nameTable = nullptr;
	}

	delete _hashIndex;
	_hashIndex = nullptr;

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

		return _numEntries != 0;
	}

	_name = directoryName;
	_type = kBundleTypeDirectory;
	_numEntries = fileNames.size();

	_nameTable = (char *)_engine->getMemoryServices()->mallocImpl(nameBytes * sizeof(char));
	if (!_nameTable) {
		close();

		return _numEntries != 0;
	}

	_nameTableSize = nameBytes;

	int offset = 0;

	for (uint i = 0; i < fileNames.size(); i++) {
		PBundleDirectoryEntry *entry = new PBundleDirectoryEntry();

		entry->_name = &_nameTable[offset];
		entry->_hash = hashPath(fileNames[i].c_str(), &_nameTable[offset]);
		entry->_flags = kBundleEntryDirectoryFile;
		_entries.push_back(entry);

		offset += fileNames[i].size() + 1;
	}

	buildHashIndex();

	return _numEntries != 0;
}

bool CBundle::openBundleFile(const char *fileName) {
	Common::String path = ".\\";
	path += fileName;

	LConsoleServices *console = _engine->getConsoleServices();
	LFile *bundleFile = _engine->getFileServices()->open(path.c_str(), "rb");
	if (!bundleFile) {
		if (_engine->getGamePlatform() == Common::kPlatformPS2 && console)
			console->logWarning("OpenBundle: Unable to open file <%s>.\n", fileName);

		return false;
	}

	_name = fileName;
	_file = bundleFile;
	_fileSize = _engine->getFileServices()->getSize(fileName);
	_type = kBundleTypeFile;
	_entries.clear();
	_nameTable = nullptr;
	_nameTableSize = 0;

	bool success = readHeader();

	if (success) {
		buildHashIndex();
	} else {
		if (console)
			console->logError("Unable to open bundle %s. It is not a valid bundle?\n", fileName);
		else
			warning("CBundle::openBundleFile(): unable to open bundle %s, it is not a valid bundle?", fileName);

		close();
	}

	return success;
}

bool CBundle::readHeader() {
	char magic[4];
	uint value;

	if (_file->read(magic, 4) != 4 || strncmp(magic, "LABN", 4) != 0) {
		close();

		return false;
	}

	if (_file->read(&value, 4) != 4 || FROM_LE_32(value) > 0x10000) {
		close();

		return false;
	}

	if (_file->read(&value, 4) != 4) {
		close();

		return false;
	}

	uint numFiles = FROM_LE_32(value);

	if (_file->read(&value, 4) != 4) {
		close();

		return false;
	}

	uint tableSize = FROM_LE_32(value);

	if (_file->read(&value, 4) != 4) {
		close();

		return false;
	}

	int tableOffset = FROM_LE_32(value) - 81167;

	// Each entry takes 16 bytes of the file
	if (numFiles > (uint)_fileSize / 16) {
		warning("CBundle::readHeader(): too many entries (%u)", numFiles);
		close();

		return false;
	}

	for (uint i = 0; i < numFiles; i++)
		_entries.push_back(new PBundleFileEntry());

	_nameTable = (char *)_engine->getMemoryServices()->mallocImpl(tableSize * sizeof(char));
	if (!_nameTable) {
		close();

		return false;
	}

	Common::Array<char *> names;

	for (uint i = 0; i < numFiles; i++) {
		PBundleDirectoryEntry *entry = _entries[i];

		if (_file->read(&value, 4) != 4) {
			close();

			return false;
		}

		uint nameOffset = FROM_LE_32(value);

		if (nameOffset >= tableSize) {
			warning("CBundle::readHeader(): name offset out of the name table");
			close();

			return false;
		}

		entry->_name = &_nameTable[nameOffset];
		names.push_back(&_nameTable[nameOffset]);

		if (_file->read(&value, 4) != 4) {
			close();

			return false;
		}

		entry->_offset = FROM_LE_32(value);

		if (_file->read(&value, 4) != 4) {
			close();

			return false;
		}

		entry->_size = FROM_LE_32(value);

		if (_file->read(&value, 4) != 4) {
			close();

			return false;
		}

		entry->_flags = kBundleEntryBundleFile;
	}

	_file->seek(tableOffset, 0);

	if (_file->read(_nameTable, tableSize) != tableSize) {
		close();

		return false;
	}

	for (uint i = 0; i < numFiles; i++) {
		if (!memchr(names[i], 0, _nameTable + tableSize - names[i])) {
			warning("CBundle::readHeader(): name without terminator");
			close();

			return false;
		}

		decodeName(names[i]);
		_entries[i]->_hash = hashPath(names[i], names[i]);
	}

	return true;
}

void CBundle::buildHashIndex() {
	if (!_hashIndex)
		_hashIndex = new BundleHashIndex();

	for (uint i = 0; i < _entries.size(); i++) {
		PBundleDirectoryEntry *entry = _entries[i];

		if (!entry->_hash || !entry->_name)
			break;

		(*_hashIndex)[entry->_hash].push_back(entry);
	}
}

bool CBundle::findBundleEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup) {
	if (findHashedEntry(services, normalizedName, hash, lookup))
		return true;

	for (uint i = 0; i < _entries.size(); i++) {
		PBundleDirectoryEntry *entry = _entries[i];

		if (!entry->_name)
			return false;

		if (scumm_stricmp(entry->_name, normalizedName) == 0) {
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
	if (!_hashIndex || !_hashIndex->contains(hash))
		return false;

	Common::Array<PBundleDirectoryEntry *> &bucket = (*_hashIndex)[hash];

	for (uint i = 0; i < bucket.size(); i++) {
		PBundleDirectoryEntry *entry = bucket[i];

		if (entry->_name && scumm_stricmp(entry->_name, normalizedName) == 0) {
			entry->fillLookup(services, this, lookup);

			return true;
		}
	}

	return false;
}

LBundleFile::LBundleFile(EmiEngine *engine) : LFile(engine) {
	reset();
}

LBundleFile::~LBundleFile() {
	// The stream of a member of a bundle belongs to the bundle
	if (!_unbuffered)
		_readStream = nullptr;
}

void LBundleFile::reset() {
	LFile::reset();

	_filePos = 0;
	_bufPos = 0;
	_bufAvail = 0;
	_memberSize = 0;
	_baseOffset = 0;
	_atEnd = false;
	_unbuffered = false;
	_bufCapacity = 0;
	_openMode[0] = 0;
	_cache[0] = 0;
}

uint LBundleFile::read(void *dest, uint count) {
	if (!isOpen())
		return 0;

	if (_unbuffered || _memData)
		return LFile::read(dest, count);

	if (count < _bufCapacity && count > _bufAvail)
		fillBuffer();

	if (count > _bufAvail) {
		uint available = _bufAvail;

		if (available)
			memcpy(dest, &_cache[_bufPos], available);

		_filePos += available;
		_bufPos += available;
		_bufAvail = 0;

		uint readBytes = readAt((byte *)dest + available, _filePos, count - available);

		_filePos += readBytes;
		_atEnd = _filePos >= _memberSize;

		return available + readBytes;
	}

	memcpy(dest, &_cache[_bufPos], count);

	_filePos += count;
	_bufPos += count;
	_bufAvail -= count;
	_atEnd = _filePos >= _memberSize;

	return count;
}

char *LBundleFile::readLine(char *dest, int maxCount) {
	char *out = dest;
	char character = 0;
	int consumed = 0;

	if (!isOpen())
		return nullptr;

	if (_unbuffered || _memData)
		return LFile::readLine(dest, maxCount);

	if (maxCount <= 0) {
		warning("LBundleFile::readLine(): invalid buffer size");

		return nullptr;
	}

	int limit = maxCount;

	if (_bufAvail < (uint)maxCount)
		fillBuffer();

	if (_bufAvail < (uint)maxCount)
		limit = _bufAvail + 1;

	int remaining = limit - 1;
	const byte *src = &_cache[_bufPos];
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
		if (_bufPos + consumed >= kBundleFileBufferSize)
			warning("LBundleFile::readLine(): line ends at the end of the buffer");
		else if (*src == 10)
			consumed++;
	}

	_filePos += consumed;
	_bufPos += consumed;
	_bufAvail -= consumed;

	if (_filePos < _memberSize) {
		_atEnd = false;

		return dest;
	}

	_atEnd = true;

	return nullptr;
}

int LBundleFile::eof() {
	if (!isOpen())
		return 1;

	if (_unbuffered || _memData)
		return LFile::eof();

	return _atEnd;
}

int LBundleFile::tell() {
	if (!isOpen())
		return -1;

	if (_unbuffered || _memData)
		return LFile::tell();

	return _filePos;
}

int LBundleFile::seek(int offset, int whence) {
	int position;

	if (!isOpen())
		return 0;

	if (_unbuffered || _memData)
		return LFile::seek(offset, whence);

	if (whence == 0)
		position = offset;
	else if (whence == 1)
		position = _filePos + offset;
	else if (whence == 2)
		position = _memberSize + offset;
	else
		return 0;

	int bufferStart = _filePos - _bufPos;
	int bufferEnd = _filePos + _bufAvail;

	if (position < bufferStart || position >= bufferEnd - 1) {
		_bufAvail = 0;
		_bufPos = 0;
		_filePos = position;

		return 1;
	}

	_filePos = position;
	_bufPos = position - bufferStart;
	_bufAvail = bufferEnd - position;

	return 1;
}

int LBundleFile::writeFormat(const char *format, ...) {
	if (!isOpen() || !_unbuffered)
		return 0;

	va_list args;
	va_start(args, format);
	LString text = LString::vformat(format, args);
	va_end(args);

	return LFile::write(text.c_str(), text.size());
}

uint LBundleFile::loadAll() {
	if (_memData)
		return 0;

	if (_unbuffered)
		return LFile::loadAll();

	uint allocSize = _memberSize + 1;

	char *data = (char *)_engine->getMemoryServices()->mallocImpl(allocSize * sizeof(char));
	if (!data) {
		warning("LBundleFile::loadAll(): cannot allocate the buffer");

		return 0;
	}

	_memSize = readAt(data, 0, _memberSize);
	_memData = data;
	_memCursor = data;
	data[allocSize - 1] = 0;

	return _memSize;
}

const char *LBundleFile::getName() {
	return _name.c_str();
}

uint LBundleFile::writeChecked(const void *src, uint count) {
	if (isOpen() && _unbuffered)
		return LFile::write(src, count);

	return 0;
}

void LBundleFile::fillBuffer() {
	uint capacity = _bufCapacity;
	byte *dest = _cache;

	if (_bufAvail) {
		memmove(_cache, &_cache[_bufPos], _bufAvail);
		capacity -= _bufAvail;
		dest += _bufAvail;
	}

	uint readBytes = readAt(dest, _bufAvail + _filePos, capacity);

	_bufPos = 0;
	_bufAvail += readBytes;
}

uint LBundleFile::readAt(void *dest, int position, uint count) {
	if (position < 0) {
		warning("LBundleFile::readAt(): negative position");

		return 0;
	}

	if (position >= _memberSize)
		return 0;

	uint toRead = count;

	if ((int)(position + count) > _memberSize)
		toRead = _memberSize - position;

	LFile::seek(position + _baseOffset, 0);

	return LFile::read(dest, toRead);
}

CBundleServices::CBundleServices(EmiEngine *engine, LFileServices *innerServices) : LFileServices(engine), _currentIsland("unknown") {
	_numBundles = 0;
	_head = nullptr;
	_tail = nullptr;
	_logFile = nullptr;
	_inner = innerServices;

	for (uint i = 0; i < kBundleMaxOpenFiles; i++)
		_files[i] = new LBundleFile(engine);

	if (!_engine->getBundleServices())
		_engine->setBundleServices(this);
}

CBundleServices::~CBundleServices() {
	while (_numBundles) {
		CBundle *bundle = _head;
		CBundle *nextBundle = bundle->_next;

		_head = nextBundle;

		if (nextBundle)
			nextBundle->_prev = nullptr;
		else
			_tail = nullptr;

		bundle->_prev = nullptr;
		bundle->_next = nullptr;
		_numBundles--;

		delete bundle;
	}

	closeLog();

	for (uint i = 0; i < kBundleMaxOpenFiles; i++) {
		delete _files[i];
		_files[i] = nullptr;
	}

	if (_engine->getBundleServices() == this)
		_engine->setBundleServices(nullptr);
}

LFile *CBundleServices::open(const char *path, const char *mode) {
	uint index = 0;

	while (index < kBundleMaxOpenFiles) {
		if (!_files[index]->isOpen())
			break;

		index++;
	}

	if (index == kBundleMaxOpenFiles) {
		LConsoleServices *console = _engine->getConsoleServices();

		if (console)
			console->logError("Cannot open %s, too many files open!\n", path);
		else
			warning("CBundleServices::open(): cannot open %s, too many files open!", path);

		return nullptr;
	}

	LBundleFile *slot = _files[index];

	slot->reset();
	slot->_bufCapacity = kBundleFileBufferSize;

	if (strlen(mode) >= sizeof(slot->_openMode))
		warning("CBundleServices::open(): mode too long");

	Common::strlcpy(slot->_openMode, mode, sizeof(slot->_openMode));

	if (*mode == 'r' || *mode == 'R') {
		BundleLookup result;

		if (findFile(path, &result)) {
			if (result.inBundle) {
				slot->_readStream = result.file->_readStream;
				slot->_unbuffered = false;
				slot->_fileName = path;
				slot->_name.clear();
			} else {
				_inner->openInto(slot, result.path.c_str(), mode);
				slot->_name = result.path;
				slot->_unbuffered = true;

				int separator = (int)slot->_name.findLastOf('\\');
				if (separator > 0)
					slot->_name = slot->_name.substr(0, separator + 1);

				if ((int)(result.path.size() - result.name.size()) < 0)
					slot->_name.clear();
			}

			slot->_memberSize = result.size;
			slot->_baseOffset = result.offset;

			if (slot->_name.size()) {
				if (_logFile)
					_logFile->writeFormat("%s%s\n", slot->_name.c_str(), path);
			} else if (_logFile) {
				_logFile->writeFormat("%s\n", path);
			}

			return slot;
		}
	}

	_inner->openInto(slot, path, mode);

	if (!slot->isOpen()) {
		_engine->getPathingFileServices()->suspendCdPath();
		_inner->openInto(slot, path, mode);
		_engine->getPathingFileServices()->resumeCdPath();

		if (!slot->isOpen())
			return nullptr;
	}

	slot->_unbuffered = true;

	if (*mode == 'w') {
		CBundle *bundle = _head;

		while (bundle) {
			if (slot->_name == bundle->_name) {
				Common::String directory = bundle->_name;

				bundle->openDirectory(directory.c_str());

				break;
			}

			bundle = bundle->_next;
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

	return _inner->exists(path);
}

int CBundleServices::getModTime(const char *path) {
	LString pathString = path;

	if (lookup(pathString))
		return 0;

	return _inner->getModTime(path);
}

void CBundleServices::close(LFile *file, bool destroy) {
	LBundleFile *slot = nullptr;

	for (uint i = 0; i < kBundleMaxOpenFiles; i++) {
		if (file == _files[i]) {
			slot = _files[i];

			break;
		}
	}

	if (!slot) {
		_inner->close(file, destroy);

		return;
	}

	if (!slot->isOpen())
		return;

	if (slot->_unbuffered)
		_inner->close(slot, false);

	if (slot->_memData) {
		_engine->getMemoryServices()->freeImpl(slot->_memData);
		slot->_memData = nullptr;
		slot->_memCursor = nullptr;
		slot->_memSize = 0;
	}

	slot->reset();
}

int CBundleServices::getSize(const char *path) {
	BundleLookup result;

	if (findFile(path, &result))
		return result.size;

	return _inner->getSize(path);
}

bool CBundleServices::resolvePath(LString &outPath, const char *name) {
	BundleLookup result;

	if (findFile(name, &result)) {
		outPath = name;

		return true;
	}

	return _inner->resolvePath(outPath, name);
}

CBundle *CBundleServices::openBundle(const char *name) {
	CBundle *bundle = findBundle(name);
	if (bundle)
		return bundle;

	bundle = newBundle();

	bool success;

	if (!*name || name[strlen(name) - 1] == '\\') {
		success = bundle->openDirectory(name);

		if (_logFile)
			_logFile->writeFormat("Opening Directory: %s\t-", name);
	} else {
		success = bundle->openBundleFile(name);

		if (_logFile)
			_logFile->writeFormat("Opening Bundle: %s\t-", name);
	}

	if (success) {
		if (_logFile)
			_logFile->writeFormat("SUCCESS\n");

		return bundle;
	}

	bundle->close();
	removeBundle(bundle);

	if (_logFile)
		_logFile->writeFormat("FAILED\n");

	return nullptr;
}

void CBundleServices::closeBundle(const char *name) {
	CBundle *bundle = findBundle(name);
	if (!bundle)
		return;

	if (bundle->_type & kBundleTypeFile) {
		close(bundle->_file, true);

		// The disasm closes the file twice here, let's avoid that...
		bundle->_file = nullptr;
	}

	removeBundle(bundle);
}

bool CBundleServices::moveBundleToFront(const char *name) {
	CBundle *bundle = findBundle(name);
	if (!bundle)
		return false;

	bool linked = true;

	if (bundle == _head) {
		CBundle *nextBundle = _head->_next;

		_head = nextBundle;

		if (nextBundle)
			nextBundle->_prev = nullptr;
		else
			_tail = nullptr;

		bundle->_prev = nullptr;
		bundle->_next = nullptr;
	} else if (bundle == _tail) {
		CBundle *prevBundle = _tail->_prev;

		_tail = prevBundle;

		if (!prevBundle)
			_head = nullptr;
		else
			prevBundle->_next = nullptr;

		bundle->_prev = nullptr;
		bundle->_next = nullptr;
	} else {
		CBundle *nextBundle = bundle->_next;

		if (!nextBundle && !bundle->_prev) {
			linked = false;
		} else {
			nextBundle->_prev = bundle->_prev;
			bundle->_prev->_next = bundle->_next;
		}
	}

	if (linked) {
		_numBundles--;
		bundle->_prev = nullptr;
		bundle->_next = nullptr;
	}

	if (_head)
		_head->_prev = bundle;

	bundle->_prev = nullptr;
	bundle->_next = _head;

	CBundle *oldTail = _tail;

	_head = bundle;

	if (!oldTail)
		_tail = bundle;

	_numBundles++;

	return true;
}

bool CBundleServices::lookup(const LString &path) {
	BundleLookup result;

	return findFile(path.c_str(), &result);
}

bool CBundleServices::findFile(const char *path, BundleLookup *result) {
	char normalized[260];
	bool found = false;

	if (!_numBundles)
		return false;

	if (strlen(path) >= sizeof(normalized)) {
		warning("CBundleServices::findFile(): path too long");

		return false;
	}

	uint hash = CBundle::hashPath(path, normalized);
	CBundle *bundle = _head;

	while (bundle && !found) {
		if (bundle->_type & kBundleTypeFile)
			found = bundle->findBundleEntry(_inner, normalized, hash, result);
		else
			found = bundle->findDirectoryEntry(_inner, normalized, hash, result);

		bundle = bundle->_next;
	}

	return found;
}

CBundle *CBundleServices::findBundle(const char *name) {
	CBundle *bundle = _head;

	while (bundle) {
		if (bundle->_name == name)
			return bundle;

		bundle = bundle->_next;
	}

	return nullptr;
}

CBundle *CBundleServices::newBundle() {
	CBundle *bundle = new CBundle(_engine);

	if (_head)
		_head->_prev = bundle;

	bundle->_prev = nullptr;
	bundle->_next = _head;

	CBundle *oldTail = _tail;

	_head = bundle;

	if (!oldTail)
		_tail = bundle;

	_numBundles++;

	return bundle;
}

void CBundleServices::removeBundle(CBundle *bundle) {
	bool linked = true;

	if (bundle == _head) {
		CBundle *nextBundle = _head->_next;

		_head = nextBundle;

		if (nextBundle)
			nextBundle->_prev = nullptr;
		else
			_tail = nullptr;

		bundle->_prev = nullptr;
		bundle->_next = nullptr;
	} else if (bundle == _tail) {
		CBundle *prevBundle = _tail->_prev;

		_tail = prevBundle;

		if (!prevBundle)
			_head = nullptr;
		else
			prevBundle->_next = nullptr;

		bundle->_prev = nullptr;
		bundle->_next = nullptr;
	} else {
		CBundle *nextBundle = bundle->_next;

		if (!nextBundle && !bundle->_prev) {
			linked = false;
		} else {
			nextBundle->_prev = bundle->_prev;
			bundle->_prev->_next = bundle->_next;
		}
	}

	if (linked) {
		_numBundles--;
		bundle->_prev = nullptr;
		bundle->_next = nullptr;
	}

	bundle->close();
	delete bundle;
}

void CBundleServices::closeLog() {
	if (_logFile)
		_inner->close(_logFile, true);

	_logFile = nullptr;
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
	while (_numBundles) {
		CBundle *bundle = _head;
		CBundle *nextBundle = bundle->_next;

		_head = nextBundle;

		if (nextBundle)
			nextBundle->_prev = nullptr;
		else
			_tail = nullptr;

		bundle->_prev = nullptr;
		bundle->_next = nullptr;
		_numBundles--;

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

bool CBundleServices::init(EmiEngine *engine, bool isInternational) {
	if (engine->getBundleServices())
		return true;

	CBundleServices *services = new CBundleServices(engine, engine->getFileServices());
	bool localOpened = services->openLocalBundles();

	services->openCommonBundles();

	// The English disasm refuses i9n.m4b, the international one does not check it
	if (!localOpened || (!isInternational && services->isI9nBundleOpen())) {
		delete services;

		return false;
	}

	engine->pushFileServices(services);

	return true;
}

bool CBundleServices::isI9nBundleOpen() {
	return findBundle("i9n.m4b") != nullptr;
}

bool CBundleServices::openIslandBundles(const char *island) {
	bool failed = false;
	bool isPs2 = _engine->getGamePlatform() == Common::kPlatformPS2;

	if (scumm_stricmp(island, "melee") == 0) {
		bool musicFailed = isPs2 && openBundle("music1.m4b") == nullptr;
		bool voiceFailed = openBundle("voiceMel.m4b") == nullptr;
		CBundle *art = openBundle("artMel.m4b");

		failed = art == nullptr || voiceFailed || musicFailed;
	} else if (scumm_stricmp(island, "lucre") == 0) {
		bool musicFailed = isPs2 && openBundle("music1.m4b") == nullptr;
		bool voiceFailed = openBundle("voiceLuc.m4b") == nullptr;
		CBundle *art = openBundle("artLuc.m4b");

		failed = art == nullptr || voiceFailed || musicFailed;
	} else if (scumm_stricmp(island, "jambalaya") == 0) {
		bool musicFailed = isPs2 && openBundle("music2.m4b") == nullptr;
		bool voiceFailed = openBundle("voiceJam.m4b") == nullptr;
		CBundle *art = openBundle("artJam.m4b");

		failed = art == nullptr || voiceFailed || musicFailed;
	} else if (scumm_stricmp(island, "monkey") == 0) {
		bool musicFailed = isPs2 && openBundle("music2.m4b") == nullptr;
		bool voiceFailed = openBundle("voiceMon.m4b") == nullptr;
		CBundle *art = openBundle("artMon.m4b");

		failed = art == nullptr || voiceFailed || musicFailed;
	}

	if (failed)
		return false;

	moveBundleToFront("i9n.m4b");
	moveBundleToFront("patch.m4b");
	_currentIsland = island;

	return true;
}

void CBundleServices::closeIslandBundles(const char *island) {
	if (_engine->getGamePlatform() == Common::kPlatformPS2) {
		if (scumm_stricmp(island, "melee") == 0 || scumm_stricmp(island, "lucre") == 0)
			closeBundle("music1.m4b");
		else if (scumm_stricmp(island, "jambalaya") == 0 || scumm_stricmp(island, "monkey") == 0)
			closeBundle("music2.m4b");
	}

	if (scumm_stricmp(island, "melee") == 0) {
		closeBundle("voiceMel.m4b");
		closeBundle("artMel.m4b");
	} else if (scumm_stricmp(island, "lucre") == 0) {
		closeBundle("voiceLuc.m4b");
		closeBundle("artLuc.m4b");
	} else if (scumm_stricmp(island, "jambalaya") == 0) {
		closeBundle("voiceJam.m4b");
		closeBundle("artJam.m4b");
	} else if (scumm_stricmp(island, "monkey") == 0) {
		closeBundle("voiceMon.m4b");
		closeBundle("artMon.m4b");
	}

	_currentIsland = "unknown";
}

bool CBundleServices::isIslandBundleOpen(const char *island) {
	if (scumm_stricmp(island, "melee") == 0)
		return findBundle("artMel.m4b") != nullptr;

	if (scumm_stricmp(island, "lucre") == 0)
		return findBundle("artLuc.m4b") != nullptr;

	if (scumm_stricmp(island, "jambalaya") == 0)
		return findBundle("artJam.m4b") != nullptr;

	if (scumm_stricmp(island, "monkey") == 0)
		return findBundle("artMon.m4b") != nullptr;

	return findBundle(island) != nullptr;
}

bool CBundleServices::openSetBundle(const char *roomName) {
	if (_currentIsland == "unknown")
		return false;

	// Island folder names are cut to 8 characters (jambalay)
	Common::String path = _currentIsland.substr(0, 8);

	path.toUppercase();
	path += "\\";
	path += roomName;

	size_t dot = path.findLastOf('.');

	if (dot != Common::String::npos) {
		path.erase(dot);
		path += ".M4B";
	}

	if (!_currentSetBundle.empty()) {
		closeBundle(_currentSetBundle.c_str());
		_currentSetBundle.clear();
	}

	if (!openBundle(path.c_str()))
		return false;

	moveBundleToFront("i9n.m4b");
	moveBundleToFront("patch.m4b");
	_currentSetBundle = path;

	return true;
}

bool CBundleServices::setDiscBundles(int disc) {
	if (disc)
		return openCommonBundles();

	return resetBundles();
}

} // End of namespace Grim
