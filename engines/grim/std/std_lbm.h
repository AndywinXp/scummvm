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

#ifndef GRIM_STD_STD_LBM_H
#define GRIM_STD_STD_LBM_H

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

struct StdLbmBmhd {
	uint16 width;
	uint16 height;
	int16 xOrigin;
	int16 yOrigin;
	byte planes;
	byte masking;
	byte compression;
	byte pad;
	uint16 transparentColor;
	byte xAspect;
	byte yAspect;
	uint16 pageWidth;
	uint16 pageHeight;
};

class StdLbm {
public:
	StdLbm(StdDisplay *stdDisplay, StdFile *stdFile, StdBitmap *stdBitmap, SysStruct *pHS);

	StdBitmapStruct *load(const char *path, int bVideoMemory, int bNoZ);
	int write(const char *path, StdBitmapStruct *pBitmap);
	uint32 packRow(Common::WriteStream *pStream, const byte *pPixels, int width, byte *pBuffer);
	void packByte(byte value);
	uint swapLong(uint value);
	void swapBMHD(StdLbmBmhd *pBmhd);
	uint16 swapShort(uint16 value);
	const char *stripPath(const char *path);
	const char *getExtension(const char *path);
	char *stripExtension(char *path);
	char *copyBaseName(char *dst, int size, const char *path);

private:
	StdBitmapStruct *loadFailed(Common::SeekableReadStream *pFile, StdBitmapStruct *pBitmap);
	void readBmhd(Common::ReadStream *pStream, StdLbmBmhd *pBmhd);
	void writeBmhd(Common::WriteStream *pStream, const StdLbmBmhd *pBmhd);

	StdDisplay *_stdDisplay;
	StdFile *_stdFile;
	StdBitmap *_stdBitmap;
	SysStruct *_pHS;

	byte _packLastByte;
	int _packSize;
	byte *_packRunPtr;
	byte *_packOut;
	int _packState;
	int _packRepeat;
};

} // End of namespace Grim

#endif
