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

#ifndef GRIM_BLASTTEXT_H
#define GRIM_BLASTTEXT_H

#include "common/scummsys.h"

#include "engines/grim/std/std_display.h"

namespace Common {
class SeekableReadStream;
}

namespace Grim {

class Text;

extern const byte g_systemFontData[];

struct BlastTextSurface {
	byte *pPixels;
	int width;
	int height;
	int bytesPerPixel;
	int reserved16;
	int pitch;
};

struct BlastTextGlyph {
	int dataOffset;
	signed char advance;
	signed char xOffset;
	signed char yOffset;
	byte pad;
	byte *pPixels;
	uint width;
	uint height;
	int reserved20;
};

struct BlastTextFont {
	int lineHeight;
	int baseLine;
	int reserved12;
	int reserved16;
	int minY;
	uint maxY;
	int firstChar;
	int lastChar;
	uint16 *aCharMap;
	BlastTextGlyph *aGlyphs;
	byte *pPixels;
};

struct TextParams {
	BlastTextSurface *pSurface;
	StdRect *pClip;
	BlastTextFont *pFont;
	int x;
	int y;
	int fgColor;
	int bgColor;
	int fxColor;
	uint scaleX;
	uint scaleY;
	int flags;
	byte *pColorMap;
};

class BlastText {
public:
	BlastText(Text *pText);

	BlastTextSurface *getSurface() { return &_surface; }

	int load(const char *fileName, BlastTextFont *&pFont);
	int initFont(BlastTextFont *&pFont);
	void initFontHelper(const byte *pSource, BlastTextGlyph *pGlyph, int bitsPerPixel, const byte *pMap);
	int read(Common::SeekableReadStream *pFile, BlastTextFont *&pFont);
	const char *measureString(TextParams *pParams, BlastTextFont *pFont, const char *pText, int *pWidth, int *pCount, int bAdvance);
	uint charWidth(const TextParams *pParams, BlastTextFont *pFont, int c);
	uint charPositionHelper(const TextParams *pParams, BlastTextFont *pFont, byte c);
	void drawString(TextParams *pParams, BlastTextFont *pFont, const char *pText);
	void drawChars(TextParams *pParams, BlastTextFont *pFont, int c);
	void drawCharsCase0(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams);
	void drawCharsCase4(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams);
	void drawCharsCase1(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams);
	void drawCharsCase6(BlastTextGlyph *pGlyph, BlastTextFont *pFont, TextParams *pParams);

	friend class Text;

private:
	Text *_pText;
	BlastTextSurface _surface;
	byte _aCharMap[256];
};

} // End of namespace Grim

#endif
