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

#ifndef GRIM_EMI_LFILE_H
#define GRIM_EMI_LFILE_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/stream.h"

namespace Grim {

typedef Common::String LString;

class LFile {
public:
	LFile();
	virtual ~LFile();

	virtual void reset();
	virtual uint read(void *buffer, uint size);
	virtual char *readLine(char *buffer, int maxCount);
	virtual uint write(const void *buffer, uint size);
	virtual int eof();
	virtual int tell();
	virtual int seek(int offset, int whence);
	virtual int writeFormat(const char *format, ...);
	virtual int unreadChar(int character);
	virtual uint loadAll();
	virtual const char *getName();
	virtual char *getString(char *buffer, int maxCount);

	bool isOpen() const;
	void closeStreams();

	Common::SeekableReadStream *readStream;
	Common::WriteStream *writeStream;
	LString name;
	LString fileName;
	char *memData;
	char *memCursor;
	uint memSize;
	byte openFlags;
};

class LFileServices {
public:
	LFileServices();
	virtual ~LFileServices();

	virtual void openInto(LFile *file, const char *path, const char *mode);
	virtual LFile *open(const char *path, const char *mode);
	virtual LFile *openExtended(const char *path, const char *mode, int unused1, int unused2);
	virtual void openIntoAndLoad(LFile *file, const char *path, const char *mode);
	virtual LFile *openAndLoad(const char *path, const char *mode);
	virtual LFile *openExtendedAndLoad(const char *path, const char *mode, int unused1, int unused2);
	virtual bool exists(const char *path);
	virtual int getModTime(const char *path);
	virtual void close(LFile *file, bool destroy);
	virtual int getSize(const char *path);
	virtual bool resolvePath(LString &outPath, const char *name);

	static void deleteAllServices();
	static int pushServices(LFileServices *services);
	static LFileServices *popServices();
	static uint getNumServices();
};

extern LFileServices *gpFileServices;

} // End of namespace Grim

#endif
