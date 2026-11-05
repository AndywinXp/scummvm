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

#ifndef GRIM_STD_STD_BITMAP_H
#define GRIM_STD_STD_BITMAP_H

#include "common/scummsys.h"
#include "common/str.h"

#include "engines/grim/std/std_display.h"

namespace Common {
class ReadStream;
class SeekableReadStream;
class WriteStream;
}

namespace Grim {

class StdBmp;
class StdFile;
class StdGif;
class StdLbm;
class StdPcx;
class StdPic;
class SysStruct;

struct StdBitmapStruct {
	char aName[32];
	int encoding;
	uint flags;
	StdColorFormat format;
	byte *pPalette;
	uint numFrames;
	int unk_104;
	int xPos;
	int yPos;
	int colorKey;
	StdVBuffer **apFrames;
};

struct StdBitmapFileHeader {
	char magic[4];
	int version;
	int encoding;
	uint flags;
	int numFrames;
	int xPos;
	int yPos;
	int colorKey;
	StdColorFormat format;
	byte pad[40];
};

class StdBitmap {
public:
	StdBitmap(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *pHS);

	void setImageFormats(StdLbm *stdLbm, StdPcx *stdPcx, StdGif *stdGif, StdPic *stdPic, StdBmp *stdBmp) {
		_stdLbm = stdLbm;
		_stdPcx = stdPcx;
		_stdGif = stdGif;
		_stdPic = stdPic;
		_stdBmp = stdBmp;
	}

	StdBitmapStruct *zbmLoad(const char *path, int bVideoMemory, int bNoZ);
	int readData(const char *fileName, StdBitmapStruct *pBitmap, int bVideoMemory, int bNoZ);
	int loadBitmapData(Common::SeekableReadStream *pFile, StdBitmapStruct *pBitmap, int bVideoMemory, int bNoZ);
	void unloadBitmap(StdBitmapStruct *&pBitmap);
	void freeEntry(StdBitmapStruct *pBitmap);
	int appendToFile(Common::WriteStream *pStream, StdBitmapStruct *pBitmap);
	int write(const char *path, StdBitmapStruct *pBitmap);
	void convertColorFormat(StdColorFormat *pFormat, StdBitmapStruct *pBitmap);
	StdBitmapStruct *vBufferToBitmap(StdVBuffer *pSrc, int bVideoMemory, int bNoZ);
	int prepareBitmaps(StdBitmapStruct *pBitmap, int numFrames, byte *pPalette, int encoding, uint flags, const StdColorFormat *pFormat);
	void addFrame(StdBitmapStruct *pBitmap, int index, StdVBuffer *pFrame);
	int memUsage(StdBitmapStruct *pBitmap);
	void deCompress(StdBitmapStruct *pBitmap);
	void applyGamma(byte *pDst, const byte *pSrc, int count, double gamma);
	int buildPaletteOpaque(const byte *pPalette, uint16 *pTable, const StdColorFormat *pFormat);
	int buildPaletteAlpha(const byte *pPalette, uint16 *pTable, const StdColorFormat *pFormat);
	int buildPaletteColorKey(const byte *pPalette, uint16 *pTable, const StdColorFormat *pFormat, byte alpha);
	void convertPixelRow(byte *pDst, StdColorFormat *pDstFormat, const byte *pSrc, StdColorFormat *pSrcFormat, int width);
	int convertColor(StdColorFormat *pDstFormat, int color, StdColorFormat *pSrcFormat);

	StdBitmapStruct *loadImage(Common::String &path, int bVideoMemory, int bNoZ);
	StdBitmapStruct *loadLbm(Common::String &path, int bVideoMemory, int bNoZ);
	int loadLbm2(Common::String &path, StdBitmapStruct *pBitmap);

private:
	void readColorFormat(Common::ReadStream *pStream, StdColorFormat *pFormat);
	void writeColorFormat(Common::WriteStream *pStream, const StdColorFormat *pFormat);

	StdDisplay *_stdDisplay;
	StdFile *_stdFile;
	SysStruct *_pHS;
	StdLbm *_stdLbm;
	StdPcx *_stdPcx;
	StdGif *_stdGif;
	StdPic *_stdPic;
	StdBmp *_stdBmp;
	uint _ditherCounter;
};

} // End of namespace Grim

#endif
