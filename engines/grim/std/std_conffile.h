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

#ifndef GRIM_STD_STD_CONFFILE_H
#define GRIM_STD_STD_CONFFILE_H

#include "common/scummsys.h"

#include "engines/grim/std/std_sys.h"

namespace Grim {

class StdConffile {
public:
	explicit StdConffile(SysStruct *pHS);
	~StdConffile();

	int open(const char *path);
	int fOpen(const char *path);
	void close();
	int readLine();
	const char *getLine() const { return _aLine; }
	int getLineNumber() const { return _linenum; }

private:
	void pushState();
	void popState();
	char *getsFile(Common::SeekableReadStream *pFile, char *pBuffer, int size);

	SysStruct *_pHS;
	int _linenum;
	int _bOpen;
	Common::SeekableReadStream *_openFile;
	int _stackLevel;
	char _pFilename[128];
	char *_aLine;
	byte _entry[1028];

	char _aFilenameStack[20][128];
	char *_apBufferStack[20];
	int _linenumStack[20];
	byte _aLineStack[20][1028];
	Common::SeekableReadStream *_openFileStack[20];
};

} // End of namespace Grim

#endif
