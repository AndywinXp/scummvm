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

#include "common/endian.h"
#include "common/file.h"
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_bmp.h"
#include "engines/grim/std/std_file.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

StdBmp::StdBmp(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *pHS) : _stdDisplay(stdDisplay), _stdFile(stdFile), _pHS(pHS) {
}

StdBitmapStruct *StdBmp::load(const char *path, int bVideoMemory, int bNoZ) {
	StdBitmapStruct *pBitmap = new StdBitmapStruct();

	if (loadEntry(path, pBitmap, bVideoMemory, bNoZ))
		return pBitmap;

	return nullptr;
}

int StdBmp::loadEntry(const char *path, StdBitmapStruct *pBitmap, int bVideoMemory, int bNoZ) {
	byte *pPalette = nullptr;
	Common::SeekableReadStream *pFile = _pHS->openFile(path);

	if (!pFile)
		return 0;

	byte fileHeader[14];
	byte infoHeader[40];

	memset(fileHeader, 0, sizeof(fileHeader));
	memset(infoHeader, 0, sizeof(infoHeader));

	uint32 numRead = pFile->read(fileHeader, 14);

	if (pFile->read(infoHeader, 40) + numRead != 54)
		return 0;

	uint16 bfType = READ_LE_UINT16(fileHeader);
	uint32 biSize = READ_LE_UINT32(infoHeader);
	int32 biWidth = READ_LE_INT32(infoHeader + 4);
	int32 biHeight = READ_LE_INT32(infoHeader + 8);
	uint16 biBitCount = READ_LE_UINT16(infoHeader + 14);

	if (bfType != 19778)
		return 0;

	if (biSize != 40)
		return 0;

	if (biBitCount < 8) {
		uint32 paletteSize = 4 * (1 << biBitCount);

		pPalette = new byte[paletteSize + 44];

		if (pFile->read(pPalette, paletteSize) != paletteSize)
			error("Unable to read the palette.  Your file is corrupted.");
	}

	memset(pBitmap, 0, sizeof(StdBitmapStruct));
	strncpy(pBitmap->aName, _stdFile->fromPath(path), 31);
	pBitmap->aName[31] = 0;
	pBitmap->encoding = 0;
	pBitmap->flags = 0;
	pBitmap->pPalette = pPalette;
	pBitmap->numFrames = 1;
	pBitmap->curFrame = 0;

	switch (biBitCount) {
	case 8:
		pBitmap->flags = 2;
		pBitmap->format.bpp = biBitCount;
		pBitmap->format.colorMode = 0;
		break;
	case 16:
		error("16 bit-per-pixel BMP not yet supported.");
		break;
	case 24:
		pBitmap->format.colorMode = 1;
		pBitmap->format.bpp = 24;
		pBitmap->format.redBits = 8;
		pBitmap->format.greenBits = 8;
		pBitmap->format.blueBits = 8;
		pBitmap->format.redShift = 16;
		pBitmap->format.greenShift = 8;
		pBitmap->format.blueShift = 0;
		break;
	case 32:
		error("32bit-per-pixel BMP not yet supported.");
		break;
	default:
		error("This BMP file uses a bit-depth which I do not support yet.");
		break;
	}

	pBitmap->apFrames = new StdVBuffer *[pBitmap->numFrames]();

	if (biWidth < 0 || biHeight == (int32)0x80000000) {
		warning("StdBmp::loadEntry(): invalid image size");
		return 0;
	}

	StdVBufferTexFmt format;

	memset(&format, 0, sizeof(format));
	format.format = pBitmap->format;
	format.width = biWidth;

	if (biHeight < 0)
		format.height = -biHeight;
	else
		format.height = biHeight;

	if ((int64)format.width * (int64)format.height * (int64)(pBitmap->format.bpp >> 3) > 0x7FFFFFFF) {
		warning("StdBmp::loadEntry(): invalid image size");
		return 0;
	}

	pBitmap->apFrames[0] = _stdDisplay->vBufferNew(&format, bVideoMemory, bNoZ, pPalette);

	if (!pBitmap->apFrames[0])
		error("Unable to allocate memory.");

	if (!_stdDisplay->vBufferLock(pBitmap->apFrames[0]))
		error("Unable to lock VBuffer memory.");

	StdVBuffer *pFrame = pBitmap->apFrames[0];
	int height = pFrame->tex.height;
	byte *pDstRow = pFrame->pPixels;
	int pitchInBytes = pFrame->tex.pitchInBytes;
	byte *pPixels = pDstRow;

	for (int row = 0; row < height; row++) {
		if (pFile->read(pPixels, pFrame->tex.pitchInBytes) != (uint32)pFrame->tex.pitchInBytes)
			error("Unable to read all the data from your BMP file.  It may be corrupted.");

		pFile->seek(((pitchInBytes + 3) & ~3) - pitchInBytes, SEEK_CUR);

		if (pFrame->tex.format.bpp == 24) {
			byte *pPixel = pPixels;

			for (int x = 0; x < pFrame->tex.width; x++) {
				byte swap = pPixel[0];

				pPixel[0] = pPixel[2];
				pPixel[2] = swap;
				pPixel += 3;
			}
		}

		pPixels += pitchInBytes;
	}

	if (biHeight > 0) {
		for (int row = 0; row < height / 2; row++) {
			byte *pTop = pDstRow + row * pitchInBytes;
			byte *pBottom = pDstRow + (height - 1 - row) * pitchInBytes;

			for (int x = 0; x < pitchInBytes; x++) {
				byte swap = pTop[x];

				pTop[x] = pBottom[x];
				pBottom[x] = swap;
			}
		}
	}

	_stdDisplay->vBufferUnlock(pBitmap->apFrames[0]);
	delete pFile;

	return 1;
}

int StdBmp::write(const char *path, StdBitmapStruct *pBitmap) {
	StdVBuffer *pFrame = pBitmap->apFrames[0];
	int paletteSize;

	if (pFrame->tex.format.colorMode) {
		if (pFrame->tex.format.bpp != 24)
			error("Only 24bpp supported");

		paletteSize = 0;
	} else {
		if (pFrame->tex.format.bpp < 1 || pFrame->tex.format.bpp > 8) {
			warning("StdBmp::write(): unsupported bits per pixel for a palettized image");
			return 0;
		}

		paletteSize = 4 * (1 << pFrame->tex.format.bpp);
	}

	uint32 imageSize = (uint32)pFrame->tex.format.bpp * (uint32)pFrame->tex.height * (uint32)pFrame->tex.width;
	byte fileHeader[14];
	byte infoHeader[40];

	imageSize >>= 3;
	WRITE_LE_UINT16(fileHeader, 19778);
	WRITE_LE_UINT32(fileHeader + 2, imageSize + paletteSize + 54);
	WRITE_LE_UINT16(fileHeader + 6, 0);
	WRITE_LE_UINT16(fileHeader + 8, 0);
	WRITE_LE_UINT32(fileHeader + 10, paletteSize + 54);
	memset(infoHeader, 0, sizeof(infoHeader));
	WRITE_LE_UINT32(infoHeader, 40);
	WRITE_LE_INT32(infoHeader + 4, pFrame->tex.width);
	WRITE_LE_INT32(infoHeader + 8, pFrame->tex.height);
	WRITE_LE_UINT16(infoHeader + 12, 1);
	WRITE_LE_UINT16(infoHeader + 14, pFrame->tex.format.bpp);
	WRITE_LE_UINT32(infoHeader + 16, 0);
	WRITE_LE_UINT32(infoHeader + 20, imageSize);
	WRITE_LE_UINT32(infoHeader + 24, 2834);
	WRITE_LE_UINT32(infoHeader + 28, 2834);

	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	bool bWritten = file.write(fileHeader, 14) == 14 && file.write(infoHeader, 40) == 40;

	if (bWritten && paletteSize > 0) {
		int copySize = paletteSize;

		if (!pBitmap->pPalette) {
			warning("StdBmp::write(): bitmap without a palette");
			copySize = 0;
		} else if (copySize > 768) {
			warning("StdBmp::write(): palette smaller than the file palette");
			copySize = 768;
		}

		if (copySize > 0)
			bWritten = file.write(pBitmap->pPalette, copySize) == (uint32)copySize;

		for (int i = copySize; bWritten && i < paletteSize; i++)
			file.writeByte(0);
	}

	if (bWritten) {
		int rowBytes = ((uint32)pFrame->tex.format.bpp * (uint32)pFrame->tex.width) >> 3;

		for (int y = pFrame->tex.height - 1; y >= 0; y--) {
			byte *pRow = pFrame->pPixels + pFrame->tex.pitchInBytes * y;

			if (pFrame->tex.format.bpp == 24) {
				byte *pPixel = pRow;

				for (int x = 0; x < pFrame->tex.width; x++) {
					byte swap = pPixel[0];

					pPixel[0] = pPixel[2];
					pPixel[2] = swap;
					pPixel += 3;
				}
			}

			if (file.write(pRow, rowBytes) != (uint32)rowBytes) {
				file.close();
				return 0;
			}
		}

		file.close();

		return 1;
	}

	file.close();

	return 0;
}

} // End of namespace Grim
