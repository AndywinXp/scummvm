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

#ifndef GRIM_STD_STD_PCX_H
#define GRIM_STD_STD_PCX_H

#include "common/scummsys.h"

namespace Common {
class ReadStream;
class SeekableReadStream;
class WriteStream;
}

namespace Grim {

class StdBitmap;
class StdDisplay;
class StdFile;
class SysStruct;
struct StdBitmapStruct;

struct StdPcxHeader {
	byte manufacturer;
	byte version;
	byte encoding;
	byte bitsPerPixel;
	int16 xMin;
	int16 yMin;
	int16 xMax;
	int16 yMax;
	int16 hDpi;
	int16 vDpi;
	byte colorMap[48];
	byte reserved;
	byte planes;
	int16 bytesPerLine;
	int16 paletteInfo;
	byte filler[58];
};

class StdPcx {
public:
	StdPcx(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *hs);

	StdBitmapStruct *load(const char *path, int videoMemory, int noZ);
	int write(const char *path, StdBitmapStruct *bitmap);

private:
	StdBitmapStruct *loadFailed(Common::SeekableReadStream *file, StdBitmapStruct *bitmap);
	void readHeader(Common::ReadStream *stream, StdPcxHeader *header);
	void writeHeader(Common::WriteStream *stream, const StdPcxHeader *header);

	StdDisplay *_stdDisplay;
	StdFile *_stdFile;
	StdBitmap *_stdBitmap;
	SysStruct *_hs;
};

} // End of namespace Grim

#endif
