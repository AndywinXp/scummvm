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

#ifndef GRIM_EMI_CPCXFILE_H
#define GRIM_EMI_CPCXFILE_H

#include "common/scummsys.h"

#include "engines/grim/emi/lfile.h"

namespace Grim {

class LBitmap;

struct PcxHeader {
	byte id;
	byte version;
	byte encoding;
	byte bitsPerPixel;
	uint16 xMin;
	uint16 yMin;
	uint16 xMax;
	uint16 yMax;
	uint16 hDpi;
	uint16 vDpi;
	byte palette16[48];
	byte reserved64;
	byte numPlanes;
	uint16 bytesPerLine;
	uint16 paletteType;
	uint16 screenWidth;
	uint16 screenHeight;
	byte filler[54];
};

struct PcxReader {
	byte *base;
	byte *cursor;
	byte *end;
	uint size;
	bool overrun;
};

class CPCXFile {
public:
	CPCXFile();

	virtual bool readImage(LFile *file, LBitmap *image, int unused);
	virtual int readPalette(LFile *file, int *palette);
	virtual int writeImage(LFile *file, LBitmap *image, const int *palette);

	LBitmap *bitmap;
};

bool ioPcxReadHeader(PcxReader *reader, PcxHeader *header);
int ioPcxWriteHeader(LFile *file, const PcxHeader *header);
int ioPcxEncodeLine(const byte *source, byte *dest, int length);
int ioPcxDecodeLine(PcxReader *reader, byte *dest, int length);
void ioPcxLoadFile(LFile *file, PcxReader *reader);

} // End of namespace Grim

#endif
