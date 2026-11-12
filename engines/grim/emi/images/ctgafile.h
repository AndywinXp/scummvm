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

#ifndef GRIM_EMI_CTGAFILE_H
#define GRIM_EMI_CTGAFILE_H

#include "common/scummsys.h"

#include "engines/grim/emi/files/lfile.h"

namespace Grim {

class EmiEngine;
class LBitmap;

class CTGAFile {
public:
	explicit CTGAFile(EmiEngine *engine);

	virtual bool readImage(LFile *file, LBitmap *image, int unused);
	virtual int readPalette(LFile *file, int *palette);
	virtual int writeImage(LFile *file, LBitmap *image, const byte *palette);

	void init();
	int readFooter(LFile *file);
	int readHeader(LFile *file);
	bool readPixels16(LFile *file);
	bool readPixels24(LFile *file);
	bool readPixels32(LFile *file);

	uint16 extensionSize;
	char authorName[41];
	char authorComments[324];
	uint16 stampMonth;
	uint16 stampDay;
	uint16 stampYear;
	uint16 stampHour;
	uint16 stampMinute;
	uint16 stampSecond;
	char jobName[41];
	uint16 jobHours;
	uint16 jobMinutes;
	uint16 jobSeconds;
	char softwareId[41];
	uint16 softwareVersion;
	byte softwareLetter;
	byte keyColor[4];
	uint16 pixelAspectNum;
	uint16 pixelAspectDen;
	uint16 gammaNum;
	uint16 gammaDen;
	uint32 colorCorrectionOffset;
	uint32 thumbnailOffset;
	uint32 scanLineOffset;
	byte attributesType;

	LBitmap *bitmap;
	byte idLength;
	byte colorMapType;
	byte imageType;
	uint16 colorMapFirst;
	uint16 colorMapLength;
	byte colorMapDepth;
	uint16 xOrigin;
	uint16 yOrigin;
	uint16 width;
	uint16 height;
	byte pixelDepth;
	byte descriptor;

	uint32 extensionOffset;
	char signature[16];
	uint16 footerTail;
	bool flipHorizontal;
	bool flipVertical;
	bool hasGamma;
	float gamma;
	float pixelAspect;

private:
	EmiEngine *_engine;

	void flipImage(int bytesPerPixel);
};

} // End of namespace Grim

#endif
