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

	uint16 _extensionSize;
	char _authorName[41];
	char _authorComments[324];
	uint16 _stampMonth;
	uint16 _stampDay;
	uint16 _stampYear;
	uint16 _stampHour;
	uint16 _stampMinute;
	uint16 _stampSecond;
	char _jobName[41];
	uint16 _jobHours;
	uint16 _jobMinutes;
	uint16 _jobSeconds;
	char _softwareId[41];
	uint16 _softwareVersion;
	byte _softwareLetter;
	byte _keyColor[4];
	uint16 _pixelAspectNum;
	uint16 _pixelAspectDen;
	uint16 _gammaNum;
	uint16 _gammaDen;
	uint32 _colorCorrectionOffset;
	uint32 _thumbnailOffset;
	uint32 _scanLineOffset;
	byte _attributesType;

	LBitmap *_bitmap;
	byte _idLength;
	byte _colorMapType;
	byte _imageType;
	uint16 _colorMapFirst;
	uint16 _colorMapLength;
	byte _colorMapDepth;
	uint16 _xOrigin;
	uint16 _yOrigin;
	uint16 _width;
	uint16 _height;
	byte _pixelDepth;
	byte _descriptor;

	uint32 _extensionOffset;
	char _signature[16];
	uint16 _footerTail;
	bool _flipHorizontal;
	bool _flipVertical;
	bool _hasGamma;
	float _gamma;
	float _pixelAspect;

private:
	EmiEngine *_engine;

	void flipImage(int bytesPerPixel);
};

} // End of namespace Grim

#endif
