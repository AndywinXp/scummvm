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

#ifndef GRIM_STD_STD_DISPLAY_H
#define GRIM_STD_STD_DISPLAY_H

#include "common/scummsys.h"

namespace Grim {

class StdBitmap;

struct StdRect {
	int x;
	int y;
	int width;
	int height;
};

struct StdColorFormat {
	int colorMode;
	int bpp;
	int redBits;
	int greenBits;
	int blueBits;
	int redShift;
	int greenShift;
	int blueShift;
	int redLoss;
	int greenLoss;
	int blueLoss;
	int alphaBits;
	int alphaShift;
	int alphaLoss;
};

struct StdVBufferTexFmt {
	int width;
	int height;
	int sizeInBytes;
	int pitchInBytes;
	int pitchInPixels;
	StdColorFormat format;
};

struct StdVBuffer {
	int type;
	uint lockCount;
	int bVideoMemory;
	StdVBufferTexFmt tex;
	byte *pPaletteRGB;
	byte *pPixels;
	int colorKey;
	int allocSize;
};

class StdDisplay {
public:
	StdDisplay();

	void setStdBitmap(StdBitmap *pStdBitmap) { _pStdBitmap = pStdBitmap; }

	StdVBuffer *vBufferNew(StdVBufferTexFmt *pFormat, int bVideoMemory, int bVideoMemoryOnly, byte *pPalette);
	void vBufferFree(StdVBuffer *&pVBuffer);
	void vBufferFreeEntry(StdVBuffer *pVBuffer);
	int vBufferLock(StdVBuffer *pVBuffer);
	int vBufferUnlock(StdVBuffer *pVBuffer);
	void vBufferFill(StdVBuffer *pVBuffer, int color, StdRect *pRect);
	int vBufferCopy(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);
	int vBufferSetColorKey(StdVBuffer *pVBuffer, int colorKey);
	void vBufferDeCompress(StdVBuffer *pVBuffer);
	int vBufferDeCompressTo(StdVBuffer *pSrc, StdVBuffer *pDst);
	StdVBuffer *vBufferConvertColorFormat(StdColorFormat *pFormat, StdVBuffer *pVBuffer);
	int copyBltA(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey);
	int lzssDecompress(byte *pDst, byte *pSrc, int size, int outputSize);

private:
	void decompressImpl();
	int readFlagBit(const byte *&pIn, uint &flagBits, int &flagCount);

	StdBitmap *_pStdBitmap;
	byte *_inputBuffer;
	int _inputBufferIndex;
	int _inputBufferMax;
	byte *_outputBuffer;
	int _outputBufferIndex;
	int _outputBufferMax;
};

} // End of namespace Grim

#endif
