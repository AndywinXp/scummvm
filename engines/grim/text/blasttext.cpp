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
#include "common/stream.h"
#include "common/textconsole.h"

#include "engines/grim/text/blasttext.h"
#include "engines/grim/text/text.h"

namespace Grim {

static BlastTextGlyph *findGlyph(BlastTextFont *pFont, int c) {
	if (c > pFont->lastChar)
		return nullptr;

	if (c < pFont->firstChar) {
		warning("BlastText: character %d is below the first character of the font", c);
		return nullptr;
	}

	return &pFont->aGlyphs[pFont->aCharMap[c - pFont->firstChar]];
}

BlastText::BlastText(Text *pText) : _pText(pText) {
	memset(&_surface, 0, sizeof(_surface));
	memset(_aCharMap, 0, sizeof(_aCharMap));
}

int BlastText::load(const char *fileName, BlastTextFont *&pFont) {
	int result = 1;

	if (!_pText->_pHS) {
		warning("BlastText::load(): the text system is not initialized");
		return result;
	}

	Common::SeekableReadStream *pFile = _pText->_pHS->openFile(fileName);

	if (pFile) {
		result = read(pFile, pFont);
		delete pFile;
		pFile = nullptr;
	}

	return result;
}

int BlastText::initFont(BlastTextFont *&pFont) {
	const byte *pData = g_systemFontData;
	int numGlyphs = READ_LE_UINT16(pData);
	const byte aMap[2] = { 0, 255 };

	BlastTextFont *pNewFont = new BlastTextFont();

	pNewFont->lineHeight = pData[2];
	pNewFont->baseLine = pData[2] - 1;
	pNewFont->reserved12 = 0;
	pNewFont->reserved16 = 1;
	pNewFont->firstChar = 0;
	pNewFont->lastChar = numGlyphs;

	int numPixels = 0;

	for (int i = 0; i < numGlyphs; i++) {
		uint32 offset = READ_LE_UINT32(pData + 4 + 4 * i);

		if (offset)
			numPixels += pData[offset] * pData[offset + 1];
	}

	pNewFont->aCharMap = new uint16[numGlyphs + 1];

	for (int i = 0; i < numGlyphs; i++)
		pNewFont->aCharMap[i] = i;

	pNewFont->aCharMap[numGlyphs] = 0;
	pNewFont->aGlyphs = new BlastTextGlyph[numGlyphs]();
	pNewFont->pPixels = new byte[numPixels]();

	int pixelOffset = 0;

	for (int i = 0; i < numGlyphs; i++) {
		uint32 offset = READ_LE_UINT32(pData + 4 + 4 * i);

		if (offset) {
			BlastTextGlyph *pGlyph = &pNewFont->aGlyphs[i];
			signed char xOffset = pData[offset + 2];

			pGlyph->width = pData[offset];
			pGlyph->height = pData[offset + 1];
			pGlyph->xOffset = xOffset;
			pGlyph->advance = (signed char)(xOffset + (signed char)pGlyph->width);
			pGlyph->reserved20 = 1;
			pGlyph->yOffset = (signed char)((signed char)pData[offset + 3] - (signed char)pNewFont->baseLine);
			pGlyph->dataOffset = pixelOffset;
			pGlyph->pPixels = pNewFont->pPixels + pixelOffset;

			if (pNewFont->minY > pGlyph->yOffset)
				pNewFont->minY = pGlyph->yOffset;

			uint bottom = pGlyph->height + (uint)(int)pGlyph->yOffset;

			if (pNewFont->maxY > bottom)
				bottom = pNewFont->maxY;

			pNewFont->maxY = bottom;

			initFontHelper(pData + 4 + offset, pGlyph, 1, aMap);

			pixelOffset += pGlyph->height * pGlyph->width;
		}
	}

	pFont = pNewFont;

	return 0;
}

void BlastText::initFontHelper(const byte *pSource, BlastTextGlyph *pGlyph, int bitsPerPixel, const byte *pMap) {
	byte *pDest = pGlyph->pPixels;
	int bitPosition = -1;
	int bits = 0;

	for (uint row = 0; row < pGlyph->height; row++) {
		for (uint column = 0; column < pGlyph->width; column++) {
			if (bitPosition < 0) {
				bits = *pSource++;
				bitPosition = 8 - bitsPerPixel;
			}

			*pDest++ = pMap[(255 >> (8 - bitsPerPixel)) & (bits >> bitPosition)];
			bitPosition -= bitsPerPixel;
		}
	}
}

int BlastText::read(Common::SeekableReadStream *pFile, BlastTextFont *&pFont) {
	pFile->readSint32LE();
	int dataSize = pFile->readSint32LE();

	BlastTextFont *pNewFont = new BlastTextFont();

	pNewFont->lineHeight = pFile->readSint32LE();
	pNewFont->baseLine = pFile->readSint32LE();
	pNewFont->reserved12 = pFile->readSint32LE();
	pNewFont->reserved16 = pFile->readSint32LE();
	pNewFont->firstChar = pFile->readSint32LE();
	pNewFont->lastChar = pFile->readSint32LE();

	int numGlyphs = pNewFont->lastChar - pNewFont->firstChar + 1;

	if (numGlyphs < 0 || numGlyphs > 65536 || dataSize < 0 || dataSize > 0x1000000 || (int64)numGlyphs * 18 > pFile->size() - pFile->pos()) {
		warning("BlastText::read(): invalid font header");
		delete pNewFont;
		pNewFont = nullptr;

		return 1;
	}

	pNewFont->aCharMap = new uint16[numGlyphs + 1];

	for (int i = 0; i < numGlyphs; i++)
		pNewFont->aCharMap[i] = pFile->readUint16LE();

	pNewFont->aCharMap[numGlyphs] = 0;
	pNewFont->aGlyphs = new BlastTextGlyph[numGlyphs]();

	for (int i = 0; i < numGlyphs; i++) {
		BlastTextGlyph *pGlyph = &pNewFont->aGlyphs[i];

		pGlyph->dataOffset = pFile->readSint32LE();
		pGlyph->advance = pFile->readSByte();
		pGlyph->xOffset = pFile->readSByte();
		pGlyph->yOffset = pFile->readSByte();
		pGlyph->pad = pFile->readByte();
		pGlyph->width = pFile->readUint32LE();
		pGlyph->height = pFile->readUint32LE();
		pGlyph->reserved20 = 1;

		if (pNewFont->minY > pGlyph->yOffset)
			pNewFont->minY = pGlyph->yOffset;

		uint bottom = pGlyph->height + (uint)(int)pGlyph->yOffset;

		if (pNewFont->maxY > bottom)
			bottom = pNewFont->maxY;

		pNewFont->maxY = bottom;
	}

	pNewFont->pPixels = new byte[dataSize]();
	pFile->read(pNewFont->pPixels, dataSize);

	pFont = pNewFont;

	return 0;
}

const char *BlastText::measureString(TextParams *pParams, BlastTextFont *pFont, const char *pText, int *pWidth, int *pCount, int bAdvance) {
	StdRect *pClip = pParams->pClip;

	if (!pClip) {
		warning("BlastText::measureString(): no clip rectangle");
		return nullptr;
	}

	int flags = pParams->flags;
	int leftSpace = pParams->x - pClip->x;
	int rightSpace = pClip->x + pClip->width - pParams->x;
	int available;

	if (flags & 1) {
		if (leftSpace < rightSpace)
			rightSpace = leftSpace;

		available = rightSpace * 2;
	} else if (flags & 2) {
		available = leftSpace;
	} else {
		available = rightSpace;
	}

	const char *pSavedText = pText;
	int spaceWidth = 0;
	int spaceCount = 0;
	int wordWidth = 0;
	int wordChars = 0;
	int lineWidth = 0;
	int lineChars = 0;
	int c = _pText->getNextChar(&pText);

	if (c != 0) {
		for (;;) {
			if ((c == 32 || c == 254) && (flags & 4)) {
				int total = lineWidth + spaceWidth + wordWidth;

				if (total > available) {
					if (lineChars) {
						pText = pSavedText;
					} else {
						lineWidth += spaceWidth + wordWidth;
						lineChars = spaceCount + wordChars;
					}

					break;
				}

				lineWidth = total;
				lineChars += spaceCount + wordChars;
				spaceWidth = charWidth(pParams, pFont, c);
				wordWidth = 0;
				wordChars = 0;
				spaceCount = 1;
				pSavedText = pText;
				c = _pText->getNextChar(&pText);

				if (c == 0) {
					lineWidth += spaceWidth;
					lineChars += spaceCount;
					break;
				}

				while (c == 32 || c == 254) {
					spaceWidth += charWidth(pParams, pFont, c);
					spaceCount++;
					pSavedText = pText;
					c = _pText->getNextChar(&pText);

					if (c == 0)
						break;
				}

				if (c == 0) {
					lineWidth += spaceWidth;
					lineChars += spaceCount;
					break;
				}
			}

			if (c == 13 || c == 10) {
				int total = lineWidth + spaceWidth + wordWidth;

				if (total > available && (flags & 4)) {
					pText = pSavedText;
				} else {
					lineWidth = total;
					lineChars += spaceCount + wordChars;

					if (c == 13 && *pText == 10)
						pText++;
				}

				break;
			}

			wordWidth += charWidth(pParams, pFont, c);
			wordChars++;
			c = _pText->getNextChar(&pText);

			if (c == 0)
				break;
		}
	}

	const char *pNext;

	if (c != 0) {
		pNext = pText;
	} else {
		spaceWidth += wordWidth;

		int total = lineWidth + spaceWidth;

		if (total > available && (flags & 4)) {
			if (lineChars) {
				pNext = pSavedText;
			} else {
				lineWidth = total;
				lineChars = spaceCount + wordChars;
				pNext = nullptr;
			}
		} else {
			lineWidth = total;
			lineChars += spaceCount + wordChars;
			pNext = nullptr;
		}
	}

	if (bAdvance) {
		int x = pParams->x;

		if (pNext)
			pParams->y += (pParams->scaleY * (uint)pFont->lineHeight + 0xFFFF) >> 16;
		else if (flags & 1)
			pParams->x = x + lineWidth / 2;
		else if (flags & 2)
			pParams->x = x - lineWidth;
		else
			pParams->x = x + lineWidth;
	}

	if (pCount)
		*pCount = lineChars;

	if (pWidth)
		*pWidth = lineWidth;

	return pNext;
}

uint BlastText::charWidth(const TextParams *pParams, BlastTextFont *pFont, int c) {
	BlastTextGlyph *pGlyph = findGlyph(pFont, c);

	if (!pGlyph)
		return 0;

	uint width = (uint)(int)pGlyph->advance;

	if (width <= pGlyph->width)
		width = pGlyph->width;

	return (width * pParams->scaleX + 0xFFFF) >> 16;
}

uint BlastText::charPositionHelper(const TextParams *pParams, BlastTextFont *pFont, byte c) {
	BlastTextGlyph *pGlyph = findGlyph(pFont, c);

	if (!pGlyph)
		return 0;

	return (pParams->scaleX * (uint)(int)pGlyph->advance + 0xFFFF) >> 16;
}

void BlastText::drawString(TextParams *pParams, BlastTextFont *pFont, const char *pText) {
	int previousChar = 0;
	int savedFlags = pParams->flags;

	pParams->flags = savedFlags & ~8;
	_aCharMap[0] = (byte)pParams->bgColor;
	_aCharMap[128] = (byte)pParams->fxColor;
	_aCharMap[255] = (byte)pParams->fgColor;

	if (pText) {
		int bCenter = savedFlags & 1;
		const char *pNext;

		do {
			int width;
			int count;

			pNext = measureString(pParams, pFont, pText, &width, &count, 0);

			int x = pParams->x;
			int y = pParams->y;

			if (bCenter)
				pParams->x = x - width / 2;
			else if (savedFlags & 2)
				pParams->x = x - width;

			for (int i = 0; i < count; i++) {
				int c = _pText->getNextChar(&pText);

				if (previousChar != 10 || c != 13)
					drawChars(pParams, pFont, c);

				previousChar = c;
			}

			if (pNext) {
				pParams->x = x;
				pParams->y = y + ((pFont->lineHeight * pParams->scaleY + 0xFFFF) >> 16);
			}

			pText = pNext;
		} while (pNext);
	}

	pParams->flags = savedFlags;
}

void BlastText::drawChars(TextParams *pParams, BlastTextFont *pFont, int c) {
	int mode = 0;
	uint scaleX = pParams->scaleX;
	uint scaleY = pParams->scaleY;
	int penY = pParams->y;
	StdRect *pClip = pParams->pClip;
	int penX = pParams->x;

	if (!pClip) {
		warning("BlastText::drawChars(): no clip rectangle");
		return;
	}

	BlastTextGlyph *pGlyph = findGlyph(pFont, c);

	if (!pGlyph)
		return;

	pGlyph->pPixels = pFont->pPixels + pGlyph->dataOffset;

	int x;
	int y;
	int width;
	int height;

	if (scaleX == 0x10000 && scaleY == 0x10000) {
		width = pGlyph->width;
		height = pGlyph->height;
		x = pGlyph->xOffset + penX;
		y = pGlyph->yOffset + penY;
	} else {
		x = (int)((scaleX * (uint)(int)pGlyph->xOffset + 0xFFFF) >> 16) + penX;
		width = (int)((scaleX * pGlyph->width + 0xFFFF) >> 16);
		height = (int)((scaleY * pGlyph->height + 0xFFFF) >> 16);
		mode = 1;
		y = (int)((scaleY * (uint)(int)pGlyph->yOffset + 0xFFFF) >> 16) + penY;
	}

	if (!(x >= pClip->x && y >= pClip->y && x + width < pClip->x + pClip->width && y + height < pClip->y + pClip->height))
		mode |= 2;

	if (pParams->flags & 8)
		mode |= 4;

	if (!pParams->pSurface || !pParams->pSurface->pPixels) {
		warning("BlastText::drawChars(): no surface");
	} else {
		switch (mode) {
		case 0:
			drawCharsCase0(pGlyph, pFont, pParams);
			break;
		case 2:
			drawCharsCase1(pGlyph, pFont, pParams);
			break;
		case 4:
			drawCharsCase4(pGlyph, pFont, pParams);
			break;
		case 6:
			drawCharsCase6(pGlyph, pFont, pParams);
			break;
		default:
			break;
		}
	}

	pParams->x = (int)((scaleX * (uint)(int)pGlyph->advance + 0xFFFF) >> 16) + pParams->x;
}

void BlastText::drawCharsCase0(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams) {
	uint height = pGlyph->height;
	int width = pGlyph->width;
	const byte *pSource = pGlyph->pPixels;
	BlastTextSurface *pSurface = pParams->pSurface;
	int bytesPerPixel = pSurface->bytesPerPixel;
	int pitch = pSurface->pitch;
	byte *pRow = pSurface->pPixels + pitch * (pParams->y + pGlyph->yOffset) + bytesPerPixel * (pParams->x + pGlyph->xOffset);
	const byte *pMap = pParams->pColorMap;

	if (!pMap)
		pMap = _aCharMap;

	const byte *pForeground = pMap + bytesPerPixel * pParams->fgColor;
	const byte *pEffect = pMap + bytesPerPixel * pParams->fxColor;

	for (uint row = 0; row < height; row++) {
		byte *pDest = pRow;

		if (width > 0) {
			for (int column = 0; column < width; column++) {
				int value = *pSource++;

				if (value == 0) {
					pDest += bytesPerPixel;
				} else if (value == 128) {
					WRITE_UINT16(pDest, READ_UINT16(pEffect));
					pDest += bytesPerPixel;
				} else if (value == 255) {
					WRITE_UINT16(pDest, READ_UINT16(pForeground));
					pDest += bytesPerPixel;
				}
			}
		}

		pRow += pitch;
	}
}

void BlastText::drawCharsCase4(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams) {
	int height = pGlyph->height;
	int width = pGlyph->width;
	const byte *pSource = pGlyph->pPixels;
	BlastTextSurface *pSurface = pParams->pSurface;
	int pitch = pSurface->pitch;
	int topPadding = pGlyph->yOffset - pFont->minY;
	byte *pDest = pSurface->pPixels + pitch * (pParams->y + pFont->minY) + pGlyph->xOffset + pParams->x;
	const byte *pMap = pParams->pColorMap;

	if (!pMap)
		pMap = _aCharMap;

	for (int row = 0; row < topPadding; row++) {
		byte *pCell = pDest;

		for (int column = 0; column < width; column++)
			*pCell++ = *pMap;

		pDest += pitch;
	}

	int bottomPadding = (int)(pFont->maxY - pGlyph->height - (uint)(int)pGlyph->yOffset);

	for (int row = 0; row < height; row++) {
		byte *pCell = pDest;

		for (int column = 0; column < width; column++)
			*pCell++ = pMap[*pSource++];

		pDest += pitch;
	}

	for (int row = 0; row < bottomPadding; row++) {
		byte *pCell = pDest;

		for (int column = 0; column < width; column++)
			*pCell++ = *pMap;

		pDest += pitch;
	}
}

void BlastText::drawCharsCase1(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams) {
	int xOffset = pGlyph->xOffset;
	int yOffset = pGlyph->yOffset;
	int glyphWidth = pGlyph->width;
	BlastTextSurface *pSurface = pParams->pSurface;
	int bytesPerPixel = pSurface->bytesPerPixel;
	int pitch = pSurface->pitch;
	StdRect *pClip = pParams->pClip;
	int startY = yOffset + pParams->y;
	int startX = xOffset + pParams->x;

	int left = startX;

	if (left <= pClip->x)
		left = pClip->x;

	left -= startX;

	int top = startY;

	if (top <= pClip->y)
		top = pClip->y;

	top -= startY;

	int rightLimit = startX + glyphWidth;

	if (rightLimit >= pClip->x + pClip->width)
		rightLimit = pClip->x + pClip->width;

	rightLimit -= startX;

	int glyphHeight = pGlyph->height;
	int bottomLimit = pClip->y + pClip->height;

	if (startY + glyphHeight < bottomLimit)
		bottomLimit = startY + glyphHeight;

	bottomLimit -= startY;

	if (left < rightLimit && top < bottomLimit) {
		const byte *pSourceRow = pGlyph->pPixels + top * glyphWidth + left;
		byte *pDestRow = pSurface->pPixels + (left + startX) * bytesPerPixel + pitch * startY;
		const byte *pMap = pParams->pColorMap;

		if (!pMap)
			pMap = _aCharMap;

		const byte *pForeground = pMap + pParams->fgColor * bytesPerPixel;
		const byte *pEffect = pMap + pParams->fxColor * bytesPerPixel;
		int visibleWidth = rightLimit - left;
		int rows = bottomLimit - top;
		int y = startY;

		for (int row = 0; row < rows; row++) {
			if (y >= pClip->y && y < pClip->y + pClip->height && visibleWidth > 0) {
				const byte *pSource = pSourceRow;
				byte *pDest = pDestRow;

				for (int column = 0; column < visibleWidth; column++) {
					int value = *pSource++;

					if (value == 0) {
						pDest += bytesPerPixel;
					} else if (value == 128) {
						WRITE_UINT16(pDest, READ_UINT16(pEffect));
						pDest += bytesPerPixel;
					} else if (value == 255) {
						WRITE_UINT16(pDest, READ_UINT16(pForeground));
						pDest += bytesPerPixel;
					}
				}
			}

			pDestRow += pitch;
			pSourceRow += glyphWidth;
			y++;
		}
	}
}

void BlastText::drawCharsCase6(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams) {
	int glyphWidth = pGlyph->width;
	int glyphHeight = pGlyph->height;
	BlastTextSurface *pSurface = pParams->pSurface;
	StdRect *pClip = pParams->pClip;
	int pitch = pSurface->pitch;
	int startX = pParams->x + pGlyph->xOffset;
	int topPadding = pGlyph->yOffset - pFont->minY;
	int startY = pFont->minY + pParams->y;

	int left = startX;

	if (left <= pClip->x)
		left = pClip->x;

	left -= startX;

	int top = startY;

	if (top <= pClip->y)
		top = pClip->y;

	top -= startY;

	int rightLimit = startX + glyphWidth;

	if (rightLimit >= pClip->x + pClip->width)
		rightLimit = pClip->x + pClip->width;

	rightLimit -= startX;

	int bottomLimit = pClip->y + pClip->height;

	if (startY + glyphHeight < bottomLimit)
		bottomLimit = startY + glyphHeight;

	bottomLimit -= startY;

	if (left < rightLimit && top < bottomLimit) {
		const byte *pSourceRow = pGlyph->pPixels + left;
		byte *pDest = pSurface->pPixels + pitch * startY + left + startX;
		const byte *pMap = pParams->pColorMap;

		if (!pMap)
			pMap = _aCharMap;

		int visibleWidth = rightLimit - left;
		int y = startY;

		for (int row = 0; row < topPadding; row++) {
			if (y >= pClip->y && y < pClip->y + pClip->height && visibleWidth > 0) {
				byte *pCell = pDest;

				for (int column = 0; column < visibleWidth; column++)
					*pCell++ = *pMap;
			}

			pDest += pitch;
			y++;
		}

		int bottomPadding = (int)(pFont->maxY - (uint)glyphHeight - (uint)(int)pGlyph->yOffset);

		for (int row = 0; row < glyphHeight; row++) {
			if (y >= pClip->y && y < pClip->y + pClip->height && visibleWidth > 0) {
				const byte *pSource = pSourceRow;
				byte *pCell = pDest;

				for (int column = 0; column < visibleWidth; column++)
					*pCell++ = pMap[*pSource++];
			}

			pSourceRow += glyphWidth;
			pDest += pitch;
			y++;
		}

		for (int row = 0; row < bottomPadding; row++) {
			if (y >= pClip->y && y < pClip->y + pClip->height && visibleWidth > 0) {
				byte *pCell = pDest;

				for (int column = 0; column < visibleWidth; column++)
					*pCell++ = *pMap;
			}

			pDest += pitch;
			y++;
		}
	}
}

} // End of namespace Grim
