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

#ifndef GRIM_STD_STD_GIF_H
#define GRIM_STD_STD_GIF_H

#include "common/scummsys.h"

namespace Common {
class SeekableReadStream;
class WriteStream;
}

namespace Grim {

class StdBitmap;
class StdDisplay;
class StdFile;
class SysStruct;
struct StdBitmapStruct;

class StdGif {
public:
	StdGif(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *hs);

	StdBitmapStruct *load(const char *path, int videoMemory, int noZ);
	int write(const char *path, StdBitmapStruct *bitmap);
	int lzwInitDecoder(int minCodeSize);
	int lzwDecodeByte(Common::SeekableReadStream *file);
	int lzwGetCode(Common::SeekableReadStream *file);
	int readBlockByte(Common::SeekableReadStream *file);
	int lzwCompress(Common::WriteStream *stream, int minCodeSize);
	void lzwEncodeCode(Common::WriteStream *stream, int code);
	int lzwCompressRun(Common::WriteStream *stream, byte pixel);
	void lzwInitEncoder(Common::WriteStream *stream);
	int lzwFinish(Common::WriteStream *stream);
	uint32 writeBlock(Common::WriteStream *stream, int count);

private:
	StdBitmapStruct *loadFailed(Common::SeekableReadStream *file, StdBitmapStruct *bitmap);

	StdDisplay *_stdDisplay;
	StdFile *_stdFile;
	StdBitmap *_stdBitmap;
	SysStruct *_hs;

	int _oldCode;
	int _initCodeSize;
	int _firstFree;
	int _nextFree;
	int _clearCode;
	int _endCode;
	int _curCodeSize;
	int _firstChar;
	int _fresh;
	int _bitOffset;
	int _codeMask;
	byte _bitBuffer;
	int _blockPos;
	int _blockSize;
	byte _blockBuf[256];
	byte *_stack;
	byte _stackArray[5004];
	uint16 _prefix[4096];
	byte _suffix[4096];

	int _encOutCount;
	int _encPrefix;
	int _encHash;
	int _encFreeEnt;
	int _encHashShift;
	int _hashTable[5003];
	uint16 _hashCode[5003];
};

} // End of namespace Grim

#endif
