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
	char name[32];
	int encoding;
	uint flags;
	StdColorFormat format;
	byte *palette;
	uint numFrames;
	int curFrame;
	int xPos;
	int yPos;
	int colorKey;
	StdVBuffer **frames;
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
	StdBitmap(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *hs);

	void setImageFormats(StdLbm *stdLbm, StdPcx *stdPcx, StdGif *stdGif, StdPic *stdPic, StdBmp *stdBmp) {
		_stdLbm = stdLbm;
		_stdPcx = stdPcx;
		_stdGif = stdGif;
		_stdPic = stdPic;
		_stdBmp = stdBmp;
	}

	StdBitmapStruct *zbmLoad(const char *path, int videoMemory, int noZ);
	int readData(const char *fileName, StdBitmapStruct *bitmap, int videoMemory, int noZ);
	int loadBitmapData(Common::SeekableReadStream *file, StdBitmapStruct *bitmap, int videoMemory, int noZ);
	void unloadBitmap(StdBitmapStruct *&bitmap);
	void freeEntry(StdBitmapStruct *bitmap);
	int appendToFile(Common::WriteStream *stream, StdBitmapStruct *bitmap);
	int write(const char *path, StdBitmapStruct *bitmap);
	void convertColorFormat(StdColorFormat *format, StdBitmapStruct *bitmap);
	StdBitmapStruct *vBufferToBitmap(StdVBuffer *src, int videoMemory, int noZ);
	int prepareBitmaps(StdBitmapStruct *bitmap, int numFrames, byte *palette, int encoding, uint flags, const StdColorFormat *format);
	void addFrame(StdBitmapStruct *bitmap, int index, StdVBuffer *frame);
	int memUsage(StdBitmapStruct *bitmap);
	void deCompress(StdBitmapStruct *bitmap);
	void applyGamma(byte *dst, const byte *src, int count, double gamma);
	int buildPaletteOpaque(const byte *palette, uint16 *table, const StdColorFormat *format);
	int buildPaletteAlpha(const byte *palette, uint16 *table, const StdColorFormat *format);
	int buildPaletteColorKey(const byte *palette, uint16 *table, const StdColorFormat *format, byte alpha);
	void convertPixelRow(byte *dst, StdColorFormat *dstFormat, const byte *src, StdColorFormat *srcFormat, int width);
	int convertColor(StdColorFormat *dstFormat, int color, StdColorFormat *srcFormat);

	StdBitmapStruct *loadImage(Common::String &path, int videoMemory, int noZ);
	StdBitmapStruct *loadLbm(Common::String &path, int videoMemory, int noZ);
	int loadLbm2(Common::String &path, StdBitmapStruct *bitmap);

private:
	void readColorFormat(Common::ReadStream *stream, StdColorFormat *format);
	void writeColorFormat(Common::WriteStream *stream, const StdColorFormat *format);

	StdDisplay *_stdDisplay;
	StdFile *_stdFile;
	SysStruct *_hs;
	StdLbm *_stdLbm;
	StdPcx *_stdPcx;
	StdGif *_stdGif;
	StdPic *_stdPic;
	StdBmp *_stdBmp;
	uint _ditherCounter;
};

} // End of namespace Grim

#endif
