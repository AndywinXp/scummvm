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
#include "common/textconsole.h"

#include "engines/grim/std/std_bitmap.h"
#include "engines/grim/std/std_display.h"

namespace Grim {

StdDisplay::StdDisplay() : _pStdBitmap(nullptr), _inputBuffer(nullptr), _inputBufferIndex(0), _inputBufferMax(0), _outputBuffer(nullptr), _outputBufferIndex(0), _outputBufferMax(0) {
}

StdVBuffer *StdDisplay::vBufferNew(StdVBufferTexFmt *pFormat, int bVideoMemory, int bVideoMemoryOnly, byte *pPalette) {
	if (((uint)pFormat->format.bpp >> 3) == 0) {
		warning("StdDisplay::vBufferNew(): unsupported bits per pixel");
		return nullptr;
	}

	StdVBuffer *pBuffer = new StdVBuffer;

	pBuffer->pPaletteRGB = pPalette;
	pBuffer->pPixels = nullptr;
	pBuffer->lockCount = 0;
	pBuffer->tex = *pFormat;

	int width = pBuffer->tex.width;
	uint bytesPerPixel = (uint)pBuffer->tex.format.bpp >> 3;

	pBuffer->tex.pitchInBytes = width * bytesPerPixel;
	pBuffer->colorKey = 0;
	pBuffer->tex.pitchInPixels = width * bytesPerPixel / bytesPerPixel;

	int sizeInBytes = pBuffer->tex.height * width * bytesPerPixel;
	pBuffer->tex.sizeInBytes = sizeInBytes;

	pBuffer->type = 0;
	pBuffer->bVideoMemory = 0;
	pBuffer->pPixels = new byte[sizeInBytes];
	pBuffer->allocSize = sizeInBytes;
	pBuffer->lockCount = 1;

	return pBuffer;
}

void StdDisplay::vBufferFree(StdVBuffer *&pVBuffer) {
	if (pVBuffer) {
		vBufferFreeEntry(pVBuffer);
		delete pVBuffer;
		pVBuffer = nullptr;
	}
}

void StdDisplay::vBufferFreeEntry(StdVBuffer *pVBuffer) {
	if (pVBuffer->type == 0 || pVBuffer->type == 2) {
		if (pVBuffer->pPixels) {
			delete[] pVBuffer->pPixels;
			pVBuffer->pPixels = nullptr;
		}
	}
}

int StdDisplay::vBufferLock(StdVBuffer *pVBuffer) {
	if (pVBuffer->type == 0)
		pVBuffer->lockCount++;

	return 1;
}

int StdDisplay::vBufferUnlock(StdVBuffer *pVBuffer) {
	if (pVBuffer->type == 0 && pVBuffer->lockCount)
		pVBuffer->lockCount--;

	return 1;
}

void StdDisplay::vBufferFill(StdVBuffer *pVBuffer, int color, StdRect *pRect) {
	if (pVBuffer->type != 0)
		return;

	switch (pVBuffer->tex.format.bpp) {
	case 8:
		if (pRect) {
			byte *row = pVBuffer->pPixels + pRect->y * pVBuffer->tex.pitchInPixels + pRect->x;

			for (int i = 0; i < pRect->height; i++) {
				memset(row, color, pRect->width);
				row += pVBuffer->tex.pitchInBytes;
			}
		} else {
			memset(pVBuffer->pPixels, color, pVBuffer->tex.sizeInBytes);
		}
		break;
	case 16:
		if (pRect) {
			byte *row = pVBuffer->pPixels + 2 * pRect->x + 2 * pRect->y * pVBuffer->tex.pitchInPixels;

			for (int i = 0; i < pRect->height; i++) {
				uint16 *pixel = (uint16 *)row;

				for (int j = 0; j < pRect->width; j++)
					pixel[j] = (uint16)color;

				row += pVBuffer->tex.pitchInBytes;
			}
		} else {
			uint16 *pixel = (uint16 *)pVBuffer->pPixels;

			for (int i = 0; i < pVBuffer->tex.sizeInBytes >> 1; i++)
				pixel[i] = (uint16)color;
		}
		break;
	case 32:
		if (pRect) {
			byte *row = pVBuffer->pPixels + 4 * pRect->x + 4 * pRect->y * pVBuffer->tex.pitchInPixels;

			for (int i = 0; i < pRect->height; i++) {
				uint32 *pixel = (uint32 *)row;

				for (int j = 0; j < pRect->width; j++)
					pixel[j] = (uint32)color;

				row += pVBuffer->tex.pitchInBytes;
			}
		} else {
			uint32 *pixel = (uint32 *)pVBuffer->pPixels;

			for (int i = 0; i < pVBuffer->tex.sizeInBytes >> 2; i++)
				pixel[i] = (uint32)color;
		}
		break;
	default:
		break;
	}
}

int StdDisplay::vBufferCopy(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey) {
	if (pSrc->type != 0 || pDst->type != 0)
		return 0;

	copyBltA(pDst, pSrc, x, y, pRect, bColorKey);

	return 1;
}

int StdDisplay::vBufferSetColorKey(StdVBuffer *pVBuffer, int colorKey) {
	if (pVBuffer->type == 0)
		pVBuffer->colorKey = colorKey;

	return 1;
}

void StdDisplay::vBufferDeCompress(StdVBuffer *pVBuffer) {
	if (pVBuffer->type != 2)
		return;

	byte *pPixels = new byte[pVBuffer->tex.height * pVBuffer->tex.pitchInBytes];

	int size = lzssDecompress(pPixels, pVBuffer->pPixels, pVBuffer->tex.sizeInBytes, pVBuffer->tex.height * pVBuffer->tex.pitchInBytes);
	delete[] pVBuffer->pPixels;
	pVBuffer->tex.sizeInBytes = size;
	pVBuffer->type = 0;
	pVBuffer->allocSize = pVBuffer->tex.height * pVBuffer->tex.pitchInBytes;
	pVBuffer->pPixels = pPixels;
}

int StdDisplay::vBufferDeCompressTo(StdVBuffer *pSrc, StdVBuffer *pDst) {
	vBufferLock(pDst);
	lzssDecompress(pDst->pPixels, pSrc->pPixels, pSrc->tex.sizeInBytes, pDst->allocSize);

	return vBufferUnlock(pDst);
}

StdVBuffer *StdDisplay::vBufferConvertColorFormat(StdColorFormat *pFormat, StdVBuffer *pVBuffer) {
	if (!memcmp(pFormat, &pVBuffer->tex.format, sizeof(StdColorFormat)))
		return pVBuffer;

	if (!pVBuffer->tex.format.colorMode && !pFormat->colorMode)
		return pVBuffer;

	if (pVBuffer->type != 0) {
		warning("StdDisplay::vBufferConvertColorFormat(): buffer is not a memory buffer");
		return pVBuffer;
	}

	StdVBuffer *pResult;

	if (pVBuffer->tex.format.bpp == pFormat->bpp) {
		pResult = pVBuffer;
	} else {
		StdVBufferTexFmt texFormat = pVBuffer->tex;

		texFormat.format = *pFormat;
		pResult = vBufferNew(&texFormat, 0, 0, nullptr);

		if (!pResult)
			return pResult;
	}

	vBufferLock(pVBuffer);
	vBufferLock(pResult);

	for (int i = 0; i < pResult->tex.height; i++)
		_pStdBitmap->convertPixelRow(&pResult->pPixels[pResult->tex.pitchInBytes * i], pFormat, &pVBuffer->pPixels[pVBuffer->tex.pitchInBytes * i], &pVBuffer->tex.format, pResult->tex.width);

	vBufferUnlock(pVBuffer);
	vBufferUnlock(pResult);

	pResult->colorKey = _pStdBitmap->convertColor(pFormat, pVBuffer->colorKey, &pVBuffer->tex.format);
	pResult->tex.format = *pFormat;

	if (pResult != pVBuffer)
		vBufferFree(pVBuffer);

	return pResult;
}

int StdDisplay::copyBltA(StdVBuffer *pDst, StdVBuffer *pSrc, int x, int y, StdRect *pRect, char bColorKey) {
	int srcX;
	int srcY;
	int width;
	int height;

	if (pRect) {
		srcX = pRect->x;
		srcY = pRect->y;
		width = pRect->width;
		height = pRect->height;
	} else {
		width = pSrc->tex.width;
		height = pSrc->tex.height;
		srcX = 0;
		srcY = 0;
	}

	if (x >= pDst->tex.width)
		return 0;

	int endX = x + width;
	if (!endX)
		return 0;

	uint firstRow = y;
	if (y >= pDst->tex.height || !(height + y))
		return 0;

	uint lastRow = height + y - 1;
	uint lastColumn = endX - 1;
	int srcStartX = srcX;

	if (y < 0) {
		srcY -= y;
		firstRow = 0;
	}

	if (lastRow >= (uint)pDst->tex.height)
		lastRow = pDst->tex.height - 1;

	if (x < 0) {
		srcStartX = srcX - x;
		x = 0;
	}

	if (lastColumn >= (uint)pDst->tex.width)
		lastColumn = pDst->tex.width - 1;

	int copyWidth = lastColumn - x + 1;
	if (!copyWidth)
		return 1;

	uint rowBytes = copyWidth * ((uint)pSrc->tex.format.bpp >> 3);
	uint bppCase = pSrc->tex.format.bpp - 8;
	if (bppCase > 0x18)
		return 1;

	if (firstRow <= lastRow) {
		int numRows = lastRow - firstRow + 1;

		if (srcStartX < 0 || srcY < 0 || srcStartX + copyWidth > pSrc->tex.width || srcY + numRows > pSrc->tex.height) {
			warning("StdDisplay::copyBltA(): source rectangle outside the source buffer");
			return 0;
		}
	}

	switch (pSrc->tex.format.bpp) {
	case 8: {
		byte *src = &pSrc->pPixels[pSrc->tex.pitchInPixels * srcY + srcStartX];
		byte *dst = &pDst->pPixels[pDst->tex.pitchInPixels * firstRow + x];

		if (firstRow > lastRow)
			return 1;

		int rows = lastRow - firstRow + 1;

		if ((bColorKey & 1) == 0) {
			do {
				memcpy(dst, src, rowBytes);
				dst += pDst->tex.pitchInPixels;
				src += pSrc->tex.pitchInPixels;
				rows--;
			} while (rows);
		} else {
			int srcSkip = pSrc->tex.pitchInPixels - copyWidth;
			int dstSkip = pDst->tex.pitchInPixels - copyWidth;

			do {
				if ((uint)x <= lastColumn) {
					int count = copyWidth;

					do {
						if (*src != (byte)pSrc->colorKey)
							*dst = *src;
						src++;
						dst++;
						count--;
					} while (count);
				}
				dst += dstSkip;
				src += srcSkip;
				rows--;
			} while (rows);
		}
		return 1;
	}
	case 16: {
		byte *src = &pSrc->pPixels[2 * srcStartX + 2 * pSrc->tex.pitchInPixels * srcY];
		byte *dst = &pDst->pPixels[2 * x + 2 * pDst->tex.pitchInPixels * firstRow];

		if (firstRow > lastRow)
			return 1;

		int rows = lastRow - firstRow + 1;

		if ((bColorKey & 1) == 0) {
			do {
				memcpy(dst, src, rowBytes);
				src += 2 * pSrc->tex.pitchInPixels;
				dst += 2 * pDst->tex.pitchInPixels;
				rows--;
			} while (rows);
		} else {
			int srcSkip = pSrc->tex.pitchInPixels - copyWidth;
			int dstSkip = pDst->tex.pitchInPixels - copyWidth;

			do {
				if ((uint)x <= lastColumn) {
					int count = copyWidth;

					do {
						if (*(uint16 *)src != (uint16)pSrc->colorKey)
							*(uint16 *)dst = *(uint16 *)src;
						src += 2;
						dst += 2;
						count--;
					} while (count);
				}
				rows--;
				dst += 2 * dstSkip;
				src += 2 * srcSkip;
			} while (rows);
		}
		return 1;
	}
	case 24: {
		byte *src = &pSrc->pPixels[3 * srcStartX + pSrc->tex.pitchInBytes * srcY];
		byte *dst = &pDst->pPixels[3 * x + pDst->tex.pitchInBytes * firstRow];

		if (firstRow > lastRow)
			return 1;

		int rows = lastRow - firstRow + 1;

		do {
			memcpy(dst, src, rowBytes);
			dst += pDst->tex.pitchInBytes;
			rows--;
			src += pSrc->tex.pitchInBytes;
		} while (rows);
		return 1;
	}
	case 32: {
		byte *src = &pSrc->pPixels[4 * srcStartX + 4 * pSrc->tex.pitchInPixels * srcY];
		byte *dst = &pDst->pPixels[4 * x + 4 * pDst->tex.pitchInPixels * firstRow];

		if (firstRow > lastRow)
			return 1;

		int rows = lastRow - firstRow + 1;

		if ((bColorKey & 1) == 0) {
			do {
				memcpy(dst, src, rowBytes);
				src += 4 * pSrc->tex.pitchInPixels;
				rows--;
				dst += 4 * pDst->tex.pitchInPixels;
			} while (rows);
		} else {
			int srcSkip = pSrc->tex.pitchInPixels - copyWidth;
			int dstSkip = pDst->tex.pitchInPixels - copyWidth;

			do {
				if ((uint)x <= lastColumn) {
					int count = copyWidth;

					do {
						if (*(uint32 *)src != (uint32)pSrc->colorKey)
							*(uint32 *)dst = *(uint32 *)src;
						src += 4;
						dst += 4;
						count--;
					} while (count);
				}
				rows--;
				dst += 4 * dstSkip;
				src += 4 * srcSkip;
			} while (rows);
		}
		return 1;
	}
	default:
		return 1;
	}
}

int StdDisplay::lzssDecompress(byte *pDst, byte *pSrc, int size, int outputSize) {
	byte *pPadded = new byte[size + 16];

	memcpy(pPadded, pSrc, size);
	memset(pPadded + size, 0, 16);

	_inputBuffer = pPadded;
	_inputBufferIndex = 0;
	_outputBufferIndex = 0;
	_inputBufferMax = size;
	_outputBuffer = pDst;
	_outputBufferMax = outputSize;
	decompressImpl();

	delete[] pPadded;
	_inputBuffer = nullptr;

	return _outputBufferIndex;
}

int StdDisplay::readFlagBit(const byte *&pIn, uint &flagBits, int &flagCount) {
	int bit = flagBits & 1;

	flagBits >>= 1;
	if (!--flagCount) {
		flagCount = 16;
		flagBits = READ_LE_UINT16(pIn);
		pIn += 2;
	}

	return bit;
}

void StdDisplay::decompressImpl() {
	byte *pOut = _outputBuffer;
	byte *pOutEnd = _outputBuffer + _outputBufferMax;
	const byte *pInLimit = _inputBuffer + _inputBufferMax + 8;
	uint flagBits = READ_LE_UINT16(_inputBuffer);
	const byte *pIn = _inputBuffer + 2;
	int flagCount = 16;

	while (true) {
		bool bLiteral = true;

		while (bLiteral) {
			if (pIn > pInLimit) {
				warning("StdDisplay::decompressImpl(): compressed data ends unexpectedly");
				_outputBufferIndex = pOut - _outputBuffer;
				_inputBufferIndex = pIn - _inputBuffer;
				return;
			}

			bLiteral = readFlagBit(pIn, flagBits, flagCount) == 1;

			if (bLiteral) {
				if (pOut >= pOutEnd) {
					warning("StdDisplay::decompressImpl(): decompressed data does not fit the buffer");
					_outputBufferIndex = pOut - _outputBuffer;
					_inputBufferIndex = pIn - _inputBuffer;
					return;
				}

				*pOut++ = *pIn++;
			}
		}

		int copyLen;
		int offset;

		if (readFlagBit(pIn, flagBits, flagCount) == 0) {
			int high = readFlagBit(pIn, flagBits, flagCount);
			int low = readFlagBit(pIn, flagBits, flagCount);

			copyLen = (low | 2 * high) + 3;
			offset = *pIn++ | 0xFFFFFF00;
		} else {
			int first = *pIn++;
			int second = *pIn++;

			offset = (16 * ((second & 0xF0) - 256)) | first;
			copyLen = (second & 0xF) + 3;
			if ((second & 0xF) == 0) {
				copyLen = *pIn++ + 1;
				if (copyLen == 1)
					break;
			}
		}

		const byte *pCopy = pOut + offset;

		if (pCopy < _outputBuffer || pOut + copyLen > pOutEnd || pIn > pInLimit) {
			warning("StdDisplay::decompressImpl(): invalid back reference");
			break;
		}

		for (int i = 0; i < copyLen; i++)
			*pOut++ = *pCopy++;
	}

	_outputBufferIndex = pOut - _outputBuffer;
	_inputBufferIndex = pIn - _inputBuffer;
}

} // End of namespace Grim
