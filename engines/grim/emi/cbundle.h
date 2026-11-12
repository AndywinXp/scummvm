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

#ifndef GRIM_EMI_CBUNDLE_H
#define GRIM_EMI_CBUNDLE_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/str.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class CBundle;
class EmiEngine;
class PBundleDirectoryEntry;

enum {
	kBundleTypeDirectory = 1,
	kBundleTypeFile = 2
};

enum {
	kBundleEntryDirectoryFile = 4,
	kBundleEntryBundleFile = 8
};

enum {
	kBundleMaxOpenFiles = 32,
	kBundleFileBufferSize = 4096
};

struct BundleLookup {
	BundleLookup();

	LFile *file;
	uint size;
	uint offset;
	bool inBundle;
	Common::String name;
	Common::String path;
	PBundleDirectoryEntry *entry;
	CBundle *bundle;
};

class PBundleDirectoryEntry {
public:
	PBundleDirectoryEntry();
	virtual ~PBundleDirectoryEntry() {}

	void *operator new(size_t size);
	void operator delete(void *block);

	void reset();

	virtual bool isDirectoryFile();
	virtual void fillLookup(LFileServices *services, CBundle *bundle, BundleLookup *lookup);

	uint hash;
	const char *name;
	int flags;
	uint offset;
	uint size;
};

class PBundleFileEntry : public PBundleDirectoryEntry {
public:
	PBundleFileEntry();

	void fillLookup(LFileServices *services, CBundle *bundle, BundleLookup *lookup) override;
};

typedef Common::HashMap<uint, Common::Array<PBundleDirectoryEntry *> > BundleHashIndex;

class CBundle {
public:
	CBundle(EmiEngine *engine);
	~CBundle();

	void *operator new(size_t size);
	void operator delete(void *block);

	void reset();
	void close();
	bool openDirectory(const char *directory);
	bool openBundleFile(const char *fileName);
	bool readHeader();
	void buildHashIndex();
	bool findBundleEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup);
	bool findDirectoryEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup);
	bool findHashedEntry(LFileServices *services, const char *normalizedName, uint hash, BundleLookup *lookup);

	static uint hashPath(const char *path, char *normalized);
	static int decodeName(char *name);

	CBundle *prev;
	CBundle *next;
	Common::String name;
	LFile *file;
	int fileSize;
	int type;
	int numEntries;
	Common::Array<PBundleDirectoryEntry *> entries;
	char *nameTable;
	int nameTableSize;
	BundleHashIndex *hashIndex;

private:
	EmiEngine *_engine;
};

class LBundleFile : public LFile {
public:
	explicit LBundleFile(EmiEngine *engine);
	~LBundleFile() override;

	void reset() override;
	uint read(void *dest, uint count) override;
	char *readLine(char *dest, int maxCount) override;
	int eof() override;
	int tell() override;
	int seek(int offset, int whence) override;
	int writeFormat(const char *format, ...) override;
	uint loadAll() override;
	const char *getName() override;
	virtual uint writeChecked(const void *src, uint count);

	void fillBuffer();
	uint readAt(void *dest, int position, uint count);

	int filePos;
	int bufPos;
	uint bufAvail;
	int memberSize;
	int baseOffset;
	bool atEnd;
	bool unbuffered;
	uint bufCapacity;
	char openMode[8];
	byte cache[kBundleFileBufferSize];
};

class CBundleServices : public LFileServices {
public:
	CBundleServices(EmiEngine *engine, LFileServices *innerServices);
	~CBundleServices() override;

	LFile *open(const char *path, const char *mode) override;
	LFile *openExtended(const char *path, const char *mode, int unused1, int unused2) override;
	bool exists(const char *path) override;
	int getModTime(const char *path) override;
	void close(LFile *file, bool destroy) override;
	int getSize(const char *path) override;
	bool resolvePath(LString &outPath, const char *name) override;

	CBundle *openBundle(const char *name);
	void closeBundle(const char *name);
	bool moveBundleToFront(const char *name);
	bool lookup(const LString &path);
	bool findFile(const char *path, BundleLookup *result);
	CBundle *findBundle(const char *name);
	CBundle *newBundle();
	void removeBundle(CBundle *bundle);
	void closeLog();
	bool openLocalBundles();
	bool openCommonBundles();
	bool resetBundles();

	LBundleFile *files[kBundleMaxOpenFiles];
	LFileServices *inner;
	int numBundles;
	CBundle *head;
	CBundle *tail;
	LFile *logFile;

	static bool init(EmiEngine *engine, bool isInternational);

	bool isI9nBundleOpen();
	bool openIslandBundles(const char *island);
	void closeIslandBundles(const char *island);
	bool isIslandBundleOpen(const char *island);
	bool openSetBundle(const char *roomName);
	bool setDiscBundles(int disc);

private:
	Common::String _currentIsland;
	Common::String _currentSetBundle;
};

} // End of namespace Grim

#endif
