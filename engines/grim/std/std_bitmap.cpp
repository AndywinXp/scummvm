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
#include "engines/grim/std/std_gif.h"
#include "engines/grim/std/std_lbm.h"
#include "engines/grim/std/std_pcx.h"
#include "engines/grim/std/std_pic.h"
#include "engines/grim/std/std_sys.h"

namespace Grim {

static const uint ditherTable[16] = {
	0x001F, 0xC01F, 0x301F, 0xF01F,
	0x801F, 0x401F, 0xB01F, 0x701F,
	0x201F, 0xE01F, 0x101F, 0xD01F,
	0xA01F, 0x601F, 0x901F, 0x501F
};

static bool isValidShift(int shift) {
	return shift >= 0 && shift < 32;
}

StdBitmap::StdBitmap(StdDisplay *stdDisplay, StdFile *stdFile, SysStruct *pHS) : _stdDisplay(stdDisplay), _stdFile(stdFile), _pHS(pHS),
		_stdLbm(nullptr), _stdPcx(nullptr), _stdGif(nullptr), _stdPic(nullptr), _stdBmp(nullptr), _ditherCounter(0) {
}

StdBitmapStruct *StdBitmap::zbmLoad(const char *path, int bVideoMemory, int bNoZ) {
	StdBitmapStruct *pBitmap = new StdBitmapStruct();

	if (readData(path, pBitmap, bVideoMemory, bNoZ))
		return pBitmap;

	delete pBitmap;

	return nullptr;
}

int StdBitmap::readData(const char *fileName, StdBitmapStruct *pBitmap, int bVideoMemory, int bNoZ) {
	Common::SeekableReadStream *pFile = _pHS->openFile(fileName);

	if (!pFile)
		return 0;

	int result = loadBitmapData(pFile, pBitmap, bVideoMemory, bNoZ);

	strncpy(pBitmap->aName, _stdFile->fromPath(fileName), 31);
	pBitmap->aName[31] = 0;
	delete pFile;

	return result;
}

int StdBitmap::loadBitmapData(Common::SeekableReadStream *pFile, StdBitmapStruct *pBitmap, int bVideoMemory, int bNoZ) {
	StdBitmapFileHeader hdr;

	pFile->read(hdr.magic, 4);
	hdr.version = pFile->readSint32LE();
	hdr.encoding = pFile->readSint32LE();
	hdr.flags = pFile->readUint32LE();
	hdr.numFrames = pFile->readSint32LE();
	hdr.xPos = pFile->readSint32LE();
	hdr.yPos = pFile->readSint32LE();
	hdr.colorKey = pFile->readSint32LE();
	readColorFormat(pFile, &hdr.format);
	pFile->read(hdr.pad, 40);

	if (pFile->eos()) {
		warning("StdBitmap::loadBitmapData(): file shorter than the bitmap header");
		return 0;
	}

	if (memcmp(hdr.magic, "BM  ", 4))
		return 0;

	if (hdr.version != 70)
		return 0;

	if ((int64)hdr.numFrames * 8 > pFile->size() - pFile->pos()) {
		warning("StdBitmap::loadBitmapData(): frame count larger than the file");
		return 0;
	}

	if (!prepareBitmaps(pBitmap, hdr.numFrames, nullptr, hdr.encoding, hdr.flags, &hdr.format))
		return 0;

	pBitmap->xPos = hdr.xPos;
	pBitmap->colorKey = hdr.colorKey;
	pBitmap->yPos = hdr.yPos;

	StdVBufferTexFmt format;
	memset(&format, 0, sizeof(format));

	for (uint index = 0; index < pBitmap->numFrames; index++) {
		int frameWidth = pFile->readSint32LE();
		uint frameHeight = pFile->readUint32LE();

		if (frameWidth < 0 || (int)frameHeight < 0 || (int64)frameWidth * (int64)frameHeight * (int64)((uint)pBitmap->format.bpp >> 3) > 0x7FFFFFFF) {
			warning("StdBitmap::loadBitmapData(): invalid frame size");
			return 0;
		}

		format.height = frameHeight;
		format.width = frameWidth;
		format.format = pBitmap->format;

		StdVBuffer *pFrame = _stdDisplay->vBufferNew(&format, bVideoMemory, bNoZ, nullptr);

		if (!pFrame)
			return 0;

		addFrame(pBitmap, index, pFrame);
		_stdDisplay->vBufferLock(pFrame);

		if (pBitmap->encoding == 0) {
			byte *pRow = pFrame->pPixels;
			uint rowBytes = pFrame->tex.width * ((uint)pFrame->tex.format.bpp >> 3);

			for (uint row = 0; row < frameHeight; row++) {
				pFile->read(pRow, rowBytes);
				pRow += pFrame->tex.pitchInBytes;
			}
		} else if (pBitmap->encoding == 1) {
			byte *pOut = pFrame->pPixels;

			if (pFrame->tex.sizeInBytes) {
				do {
					byte value = pFile->readByte();

					if ((value & 0xC0) == 0xC0) {
						int count = value & 0x3F;
						byte fill = pFile->readByte();

						if (count > pFrame->tex.sizeInBytes - (int)(pOut - pFrame->pPixels)) {
							warning("StdBitmap::loadBitmapData(): run goes past the end of the frame");
							count = pFrame->tex.sizeInBytes - (int)(pOut - pFrame->pPixels);
						}

						if (count) {
							memset(pOut, fill, count);
							pOut += count;
						}
					} else {
						*pOut++ = value;
					}
				} while ((uint)(pOut - pFrame->pPixels) < (uint)pFrame->tex.sizeInBytes);
			}
		} else if (pBitmap->encoding == 3) {
			uint compressedSize = pFile->readUint32LE();

			if (compressedSize < (uint)pFrame->tex.sizeInBytes) {
				byte *pCompressed = new byte[compressedSize];

				pFile->read(pCompressed, compressedSize);
				pFrame->type = 2;
				pFrame->tex.sizeInBytes = compressedSize;
				delete[] pFrame->pPixels;
				pFrame->pPixels = pCompressed;
			}
		}

		_stdDisplay->vBufferUnlock(pFrame);

		if (pBitmap->flags & 1)
			_stdDisplay->vBufferSetColorKey(pFrame, pBitmap->colorKey);
	}

	if (pBitmap->flags & 2) {
		pBitmap->pPalette = new byte[768];
		pFile->read(pBitmap->pPalette, 768);
	}

	return 1;
}

void StdBitmap::unloadBitmap(StdBitmapStruct *&pBitmap) {
	freeEntry(pBitmap);
	delete pBitmap;
	pBitmap = nullptr;
}

void StdBitmap::freeEntry(StdBitmapStruct *pBitmap) {
	if (pBitmap->apFrames) {
		for (uint i = 0; i < pBitmap->numFrames; i++) {
			if (pBitmap->apFrames[i])
				_stdDisplay->vBufferFree(pBitmap->apFrames[i]);
		}

		delete[] pBitmap->apFrames;
		pBitmap->apFrames = nullptr;
	}

	if (pBitmap->pPalette) {
		delete[] pBitmap->pPalette;
		pBitmap->pPalette = nullptr;
	}
}

int StdBitmap::appendToFile(Common::WriteStream *pStream, StdBitmapStruct *pBitmap) {
	StdBitmapFileHeader hdr;

	memset(&hdr, 0, sizeof(hdr));
	strncpy(hdr.magic, "BM  ", 4);
	hdr.version = 70;
	hdr.encoding = pBitmap->encoding;
	hdr.flags = pBitmap->flags;
	hdr.numFrames = pBitmap->numFrames;
	hdr.xPos = pBitmap->xPos;
	hdr.yPos = pBitmap->yPos;
	hdr.colorKey = pBitmap->colorKey;
	hdr.format = pBitmap->format;

	pStream->write(hdr.magic, 4);
	pStream->writeSint32LE(hdr.version);
	pStream->writeSint32LE(hdr.encoding);
	pStream->writeUint32LE(hdr.flags);
	pStream->writeSint32LE(hdr.numFrames);
	pStream->writeSint32LE(hdr.xPos);
	pStream->writeSint32LE(hdr.yPos);
	pStream->writeSint32LE(hdr.colorKey);
	writeColorFormat(pStream, &hdr.format);
	pStream->write(hdr.pad, 40);

	if (pStream->err())
		return 0;

	for (uint i = 0; i < pBitmap->numFrames; i++) {
		StdVBuffer *pFrame = pBitmap->apFrames[i];
		int width = pFrame->tex.width;
		uint height = pFrame->tex.height;

		pStream->writeSint32LE(width);
		pStream->writeUint32LE(height);

		if (pStream->err())
			return 0;

		if (hdr.encoding == 0) {
			_stdDisplay->vBufferLock(pFrame);

			byte *pPixels = pFrame->pPixels;
			uint rowBytes = ((uint)pFrame->tex.format.bpp >> 3) * width;

			for (uint row = 0; row < height; row++) {
				if (pStream->write(pPixels, rowBytes) != rowBytes)
					return 0;

				pPixels += pFrame->tex.pitchInBytes;
			}

			_stdDisplay->vBufferUnlock(pFrame);
		} else if (hdr.encoding == 1) {
			byte *pPixels = pFrame->pPixels;

			for (uint y = 0; y < (uint)pFrame->tex.height; y++) {
				uint rowWidth = pFrame->tex.width;
				byte *pRow = pPixels + y * rowWidth;
				uint x = 0;

				while (x < rowWidth) {
					byte value = pRow[x];
					byte run = 1;

					while (run < 63 && x + run < rowWidth && pRow[x + run] == value)
						run++;

					if (run > 1 || value > 0xBF)
						pStream->writeByte(run | 0xC0);

					pStream->writeByte(value);
					x += run;
				}
			}
		} else if (hdr.encoding == 3) {
			_stdDisplay->vBufferLock(pFrame);

			uint sizeInBytes = pFrame->tex.sizeInBytes;

			pStream->writeUint32LE(sizeInBytes);
			pStream->write(pFrame->pPixels, sizeInBytes);
			_stdDisplay->vBufferUnlock(pFrame);
		}
	}

	if (pBitmap->flags & 2) {
		if (pStream->write(pBitmap->pPalette, 768) != 768)
			return 0;
	}

	return 1;
}

int StdBitmap::write(const char *path, StdBitmapStruct *pBitmap) {
	Common::DumpFile file;

	if (!file.open(path))
		return 0;

	if (appendToFile(&file, pBitmap)) {
		file.close();
		return 1;
	}

	file.close();

	return 0;
}

void StdBitmap::convertColorFormat(StdColorFormat *pFormat, StdBitmapStruct *pBitmap) {
	if (!memcmp(pFormat, &pBitmap->format, sizeof(StdColorFormat)))
		return;

	if (!pBitmap->format.colorMode && !pFormat->colorMode)
		return;

	for (int i = 0; i < (int)pBitmap->numFrames; i++) {
		if (pBitmap->apFrames[i]->type != 2) {
			pBitmap->apFrames[i] = _stdDisplay->vBufferConvertColorFormat(pFormat, pBitmap->apFrames[i]);

			if (!pBitmap->apFrames[i])
				error("Unable to allocate a new frame when converting image from 24 to 16bpp.");

			if (pBitmap->flags & 1)
				_stdDisplay->vBufferSetColorKey(pBitmap->apFrames[i], pBitmap->apFrames[i]->colorKey);
		}
	}

	if (pBitmap->flags & 1)
		pBitmap->colorKey = convertColor(pFormat, pBitmap->colorKey, &pBitmap->format);

	pBitmap->format = *pFormat;
}

StdBitmapStruct *StdBitmap::vBufferToBitmap(StdVBuffer *pSrc, int bVideoMemory, int bNoZ) {
	StdBitmapStruct *pBitmap = new StdBitmapStruct();

	pBitmap->encoding = 0;

	if (pSrc->pPaletteRGB)
		pBitmap->flags = 2;
	else
		pBitmap->flags = 0;

	pBitmap->numFrames = 1;
	pBitmap->curFrame = 0;
	pBitmap->xPos = 0;
	pBitmap->yPos = 0;
	pBitmap->apFrames = new StdVBuffer *[1];
	pBitmap->apFrames[0] = _stdDisplay->vBufferNew(&pSrc->tex, bVideoMemory, bNoZ, nullptr);

	if (!pBitmap->apFrames[0])
		return nullptr;

	_stdDisplay->vBufferCopy(pBitmap->apFrames[0], pSrc, 0, 0, nullptr, 0);
	_stdDisplay->vBufferUnlock(pSrc);
	pBitmap->format = pBitmap->apFrames[0]->tex.format;

	if (pBitmap->flags & 2) {
		pBitmap->pPalette = new byte[768];
		memcpy(pBitmap->pPalette, pSrc->pPaletteRGB, 768);
	} else {
		pBitmap->pPalette = nullptr;
	}

	return pBitmap;
}

int StdBitmap::prepareBitmaps(StdBitmapStruct *pBitmap, int numFrames, byte *pPalette, int encoding, uint flags, const StdColorFormat *pFormat) {
	memset(pBitmap, 0, sizeof(StdBitmapStruct));

	if (numFrames < 0) {
		warning("StdBitmap::prepareBitmaps(): negative frame count");
		return 0;
	}

	pBitmap->apFrames = new StdVBuffer *[numFrames]();
	pBitmap->flags = flags;
	pBitmap->encoding = encoding;
	pBitmap->format = *pFormat;
	pBitmap->numFrames = numFrames;
	pBitmap->pPalette = pPalette;

	return 1;
}

void StdBitmap::addFrame(StdBitmapStruct *pBitmap, int index, StdVBuffer *pFrame) {
	pBitmap->apFrames[index] = pFrame;
}

int StdBitmap::memUsage(StdBitmapStruct *pBitmap) {
	int size = 0;

	for (int i = 0; i < (int)pBitmap->numFrames; i++) {
		if (!pBitmap->apFrames[i]->bVideoMemory)
			size += pBitmap->apFrames[i]->tex.sizeInBytes;
	}

	return size + 124;
}

void StdBitmap::deCompress(StdBitmapStruct *pBitmap) {
	for (int i = 0; i < (int)pBitmap->numFrames; i++)
		_stdDisplay->vBufferDeCompress(pBitmap->apFrames[i]);

	pBitmap->encoding = 0;
}

void StdBitmap::applyGamma(byte *pDst, const byte *pSrc, int count, double gamma) {
	for (int i = 0; i < count; i++) {
		for (int component = 0; component < 3; component++) {
			double value = pow((double)*pSrc * 0.00392156862745098, gamma) * 255.0;

			if (value > 255.0)
				value = 255.0;

			if (value != value) {
				warning("StdBitmap::applyGamma(): gamma produced an invalid value");
				value = 0.0;
			}

			*pDst = (byte)(int)value;
			pSrc++;
			pDst++;
		}
	}
}

int StdBitmap::buildPaletteOpaque(const byte *pPalette, uint16 *pTable, const StdColorFormat *pFormat) {
	if (!isValidShift(pFormat->redLoss) || !isValidShift(pFormat->greenLoss) || !isValidShift(pFormat->blueLoss) ||
		!isValidShift(pFormat->redShift) || !isValidShift(pFormat->greenShift) || !isValidShift(pFormat->blueShift)) {
		warning("StdBitmap::buildPaletteOpaque(): invalid color format");
		return 0;
	}

	for (int i = 0; i < 256; i++) {
		uint red = pPalette[0] >> pFormat->redLoss;
		uint green = pPalette[1] >> pFormat->greenLoss;
		uint blue = pPalette[2] >> pFormat->blueLoss;

		pPalette += 3;
		pTable[i] = (uint16)((blue << pFormat->blueShift) | (green << pFormat->greenShift) | (red << pFormat->redShift));
	}

	return 1;
}

int StdBitmap::buildPaletteAlpha(const byte *pPalette, uint16 *pTable, const StdColorFormat *pFormat) {
	if (!isValidShift(pFormat->redLoss) || !isValidShift(pFormat->greenLoss) || !isValidShift(pFormat->blueLoss) ||
		!isValidShift(pFormat->redShift) || !isValidShift(pFormat->greenShift) || !isValidShift(pFormat->blueShift) ||
		!isValidShift(pFormat->alphaLoss) || !isValidShift(pFormat->alphaShift)) {
		warning("StdBitmap::buildPaletteAlpha(): invalid color format");
		return 0;
	}

	for (int i = 0; i < 256; i++) {
		uint red = pPalette[0] >> pFormat->redLoss;
		uint green = pPalette[1] >> pFormat->greenLoss;
		uint blue = pPalette[2] >> pFormat->blueLoss;
		byte alpha = 0;

		if (i != 0)
			alpha = 0xFF;

		pPalette += 3;
		pTable[i] = (uint16)((blue << pFormat->blueShift) | (green << pFormat->greenShift) | (red << pFormat->redShift) | (((uint)alpha >> pFormat->alphaLoss) << pFormat->alphaShift));
	}

	return 1;
}

int StdBitmap::buildPaletteColorKey(const byte *pPalette, uint16 *pTable, const StdColorFormat *pFormat, byte alpha) {
	if (!isValidShift(pFormat->redLoss) || !isValidShift(pFormat->greenLoss) || !isValidShift(pFormat->blueLoss) ||
		!isValidShift(pFormat->redShift) || !isValidShift(pFormat->greenShift) || !isValidShift(pFormat->blueShift) ||
		!isValidShift(pFormat->alphaLoss) || !isValidShift(pFormat->alphaShift)) {
		warning("StdBitmap::buildPaletteColorKey(): invalid color format");
		return 0;
	}

	for (int i = 0; i < 256; i++) {
		uint red = pPalette[0] >> pFormat->redLoss;
		uint green = pPalette[1] >> pFormat->greenLoss;
		uint blue = pPalette[2] >> pFormat->blueLoss;
		uint entryAlpha = 0;

		if (i != 0)
			entryAlpha = alpha;

		pPalette += 3;
		pTable[i] = (uint16)((blue << pFormat->blueShift) | (green << pFormat->greenShift) | (red << pFormat->redShift) | ((entryAlpha >> pFormat->alphaLoss) << pFormat->alphaShift));
	}

	return 1;
}

void StdBitmap::convertPixelRow(byte *pDst, StdColorFormat *pDstFormat, const byte *pSrc, StdColorFormat *pSrcFormat, int width) {
	_ditherCounter++;

	bool bRgb24To16 = pSrcFormat->bpp == 24 && pDstFormat->bpp == 16;
	bool bRgb16To16 = pSrcFormat->bpp == 16 && pDstFormat->bpp == 16;
	bool bRgb16To24 = pSrcFormat->bpp == 16 && pDstFormat->bpp == 24;

	if (!bRgb24To16 && !bRgb16To16 && !bRgb16To24)
		return;

	if (width <= 0) {
		warning("StdBitmap::convertPixelRow(): invalid width");
		return;
	}

	if (pSrcFormat->redBits < 1 || pSrcFormat->redBits > 30 || pSrcFormat->greenBits < 1 || pSrcFormat->greenBits > 30 || pSrcFormat->blueBits < 1 || pSrcFormat->blueBits > 30 ||
		pDstFormat->redBits < 0 || pDstFormat->redBits > 15 || pDstFormat->greenBits < 0 || pDstFormat->greenBits > 15 || pDstFormat->blueBits < 0 || pDstFormat->blueBits > 15) {
		warning("StdBitmap::convertPixelRow(): invalid color bits");
		return;
	}

	uint redScale = (int)((double)((1 << pDstFormat->redBits) - 1) * (1.0 / (double)((1 << pSrcFormat->redBits) - 1)) * 65536.0);
	uint greenScale = (int)((double)((1 << pDstFormat->greenBits) - 1) * (1.0 / (double)((1 << pSrcFormat->greenBits) - 1)) * 65536.0);
	uint blueScale = (int)((double)((1 << pDstFormat->blueBits) - 1) * (1.0 / (double)((1 << pSrcFormat->blueBits) - 1)) * 65536.0);
	int redShift = pDstFormat->redShift;
	int greenShift = pDstFormat->greenShift;
	const byte *pIn = pSrc;
	byte *pOut = pDst;
	int remaining = width;

	if (bRgb24To16) {
		if (!isValidShift(redShift) || !isValidShift(greenShift)) {
			warning("StdBitmap::convertPixelRow(): invalid color shift");
			return;
		}

		const uint *pDither = &ditherTable[(_ditherCounter & 3) * 4];

		do {
			uint dither = pDither[remaining & 3];
			uint red = (pIn[0] * redScale + dither) >> 16;
			uint green = (pIn[1] * greenScale + dither) >> 16;
			uint blue = (pIn[2] * blueScale + dither) >> 16;

			WRITE_LE_UINT16(pOut, (uint16)((red << redShift) | (green << greenShift) | blue));
			pIn += 3;
			pOut += 2;
			remaining--;
		} while (remaining);
	} else if (bRgb16To16) {
		if (!isValidShift(redShift) || !isValidShift(greenShift) || !isValidShift(pSrcFormat->redShift) || !isValidShift(pSrcFormat->greenShift)) {
			warning("StdBitmap::convertPixelRow(): invalid color shift");
			return;
		}

		uint redMask = 0xFFFFFFFF >> (32 - pSrcFormat->redBits);
		uint greenMask = 0xFFFFFFFF >> (32 - pSrcFormat->greenBits);
		uint blueMask = 0xFFFFFFFF >> (32 - pSrcFormat->blueBits);

		do {
			uint pixel = READ_LE_UINT16(pIn);
			uint red = (redScale * (redMask & (pixel >> pSrcFormat->redShift))) >> 16;
			uint green = (greenScale * (greenMask & (pixel >> pSrcFormat->greenShift))) >> 16;
			uint blue = (blueScale * (blueMask & pixel)) >> 16;

			WRITE_LE_UINT16(pOut, (uint16)((red << redShift) | (green << greenShift) | blue));
			pIn += 2;
			pOut += 2;
			remaining--;
		} while (remaining);
	} else {
		if (!isValidShift(pSrcFormat->redShift) || !isValidShift(pSrcFormat->greenShift) ||
			!isValidShift(pSrcFormat->redLoss) || !isValidShift(pSrcFormat->greenLoss) || !isValidShift(pSrcFormat->blueLoss)) {
			warning("StdBitmap::convertPixelRow(): invalid color shift");
			return;
		}

		uint redMask = 0xFFFFFFFF >> (32 - pSrcFormat->redBits);
		uint greenMask = 0xFFFFFFFF >> (32 - pSrcFormat->greenBits);
		uint blueMask = 0xFFFFFFFF >> (32 - pSrcFormat->blueBits);

		do {
			uint pixel = READ_LE_UINT16(pIn);
			byte red = (byte)(pixel >> pSrcFormat->redShift) & (byte)redMask;
			byte green = (byte)(pixel >> pSrcFormat->greenShift) & (byte)greenMask;
			byte blue = (byte)pixel & (byte)blueMask;

			pOut[0] = (byte)(red << pSrcFormat->redLoss);
			pOut[1] = (byte)(green << pSrcFormat->greenLoss);
			pOut[2] = (byte)(blue << pSrcFormat->blueLoss);
			pIn += 2;
			pOut += 3;
			remaining--;
		} while (remaining);
	}
}

int StdBitmap::convertColor(StdColorFormat *pDstFormat, int color, StdColorFormat *pSrcFormat) {
	byte srcColor[4];
	byte dstColor[4];

	WRITE_LE_UINT32(srcColor, color);
	memset(dstColor, 0, sizeof(dstColor));
	convertPixelRow(dstColor, pDstFormat, srcColor, pSrcFormat, 1);

	return READ_LE_INT32(dstColor);
}

StdBitmapStruct *StdBitmap::loadImage(Common::String &path, int bVideoMemory, int bNoZ) {
	StdBitmapStruct *pBitmap = loadLbm(path, bVideoMemory, bNoZ);

	if (pBitmap)
		deCompress(pBitmap);

	return pBitmap;
}

StdBitmapStruct *StdBitmap::loadLbm(Common::String &path, int bVideoMemory, int bNoZ) {
	// TODO: verify the supported games need the .lbm/.pcx/.gif/.pic/.bmp loaders
	Common::String extension;
	size_t dot = path.findLastOf('.');

	if (dot != Common::String::npos) {
		extension = path.substr(dot, 4);
	} else {
		path += ".lbm";
		extension = ".lbm";
	}

	if (extension.equalsIgnoreCase(".lbm"))
		return _stdLbm->load(path.c_str(), bVideoMemory, bNoZ);

	if (extension.equalsIgnoreCase(".pcx"))
		return _stdPcx->load(path.c_str(), bVideoMemory, bNoZ);

	if (extension.equalsIgnoreCase(".gif"))
		return _stdGif->load(path.c_str(), bVideoMemory, bNoZ);

	if (extension.equalsIgnoreCase(".bm") || extension.equalsIgnoreCase(".zbm"))
		return zbmLoad(path.c_str(), bVideoMemory, bNoZ);

	if (extension.equalsIgnoreCase(".pic"))
		return _stdPic->load(path.c_str(), bVideoMemory, bNoZ);

	if (extension.equalsIgnoreCase(".bmp"))
		return _stdBmp->load(path.c_str(), bVideoMemory, bNoZ);

	return nullptr;
}

int StdBitmap::loadLbm2(Common::String &path, StdBitmapStruct *pBitmap) {
	Common::String extension;
	const char *pExtension = _stdLbm->getExtension(path.c_str());

	if (!pExtension) {
		path += ".lbm";
		extension = ".lbm";
	} else {
		extension = Common::String(pExtension).substr(0, 4);
	}

	if (!extension.equalsIgnoreCase("bm") && !extension.equalsIgnoreCase("zbm"))
		deCompress(pBitmap);

	if (extension.equalsIgnoreCase("lbm"))
		return _stdLbm->write(path.c_str(), pBitmap) != 0;

	if (extension.equalsIgnoreCase("pcx"))
		return _stdPcx->write(path.c_str(), pBitmap) != 0;

	if (extension.equalsIgnoreCase("gif"))
		return _stdGif->write(path.c_str(), pBitmap) != 0;

	if (extension.equalsIgnoreCase("bm"))
		return write(path.c_str(), pBitmap) != 0;

	if (extension.equalsIgnoreCase("zbm")) {
		pBitmap->format.colorMode = 5;

		for (int i = 0; i < (int)pBitmap->numFrames; i++)
			pBitmap->apFrames[i]->tex.format.colorMode = 5;

		return write(path.c_str(), pBitmap) != 0;
	}

	return extension.equalsIgnoreCase("bmp") && _stdBmp->write(path.c_str(), pBitmap) != 0;
}

void StdBitmap::readColorFormat(Common::ReadStream *pStream, StdColorFormat *pFormat) {
	pFormat->colorMode = pStream->readSint32LE();
	pFormat->bpp = pStream->readSint32LE();
	pFormat->redBits = pStream->readSint32LE();
	pFormat->greenBits = pStream->readSint32LE();
	pFormat->blueBits = pStream->readSint32LE();
	pFormat->redShift = pStream->readSint32LE();
	pFormat->greenShift = pStream->readSint32LE();
	pFormat->blueShift = pStream->readSint32LE();
	pFormat->redLoss = pStream->readSint32LE();
	pFormat->greenLoss = pStream->readSint32LE();
	pFormat->blueLoss = pStream->readSint32LE();
	pFormat->alphaBits = pStream->readSint32LE();
	pFormat->alphaShift = pStream->readSint32LE();
	pFormat->alphaLoss = pStream->readSint32LE();
}

void StdBitmap::writeColorFormat(Common::WriteStream *pStream, const StdColorFormat *pFormat) {
	pStream->writeSint32LE(pFormat->colorMode);
	pStream->writeSint32LE(pFormat->bpp);
	pStream->writeSint32LE(pFormat->redBits);
	pStream->writeSint32LE(pFormat->greenBits);
	pStream->writeSint32LE(pFormat->blueBits);
	pStream->writeSint32LE(pFormat->redShift);
	pStream->writeSint32LE(pFormat->greenShift);
	pStream->writeSint32LE(pFormat->blueShift);
	pStream->writeSint32LE(pFormat->redLoss);
	pStream->writeSint32LE(pFormat->greenLoss);
	pStream->writeSint32LE(pFormat->blueLoss);
	pStream->writeSint32LE(pFormat->alphaBits);
	pStream->writeSint32LE(pFormat->alphaShift);
	pStream->writeSint32LE(pFormat->alphaLoss);
}

} // End of namespace Grim
