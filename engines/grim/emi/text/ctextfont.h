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

#ifndef GRIM_EMI_CTEXTFONT_H
#define GRIM_EMI_CTEXTFONT_H

#include "common/array.h"

#include "engines/grim/emi/graphics/ctexture.h"
#include "engines/grim/emi/images/labitmap.h"
#include "engines/grim/emi/resources/lresource.h"

namespace Grim {

class CDrawContext;
class EmiEngine;

struct Glyph {
	explicit Glyph(EmiEngine *engine);

	int blobOffset;
	signed char advance;
	signed char offsetX;
	signed char offsetY;
	signed char reserved7;
	LABitmap alpha;
	float u0;
	float v0;
	float u1;
	float v1;
	int page;
};

class CTextFont : public LResource {
public:
	enum {
		kPageSize = 256,
		kMaxGlyphs = 65536,
		kGlyphCellSize = 16,
		kGlyphRecordSize = 10
	};

	CTextFont();
	~CTextFont() override;

	uint resSizeOfItem() override;

	CTexture *getTexture(CDrawContext *ctx, uint page);
	void buildTextures(CDrawContext *ctx);
	bool loadFile(const char *path);

	static bool loadGlyphMap(EmiEngine *engine);
	static void makeGlyphAlpha(LABitmap *glyph);
	static void freeGlyphMap(EmiEngine *engine);

	int _lineHeight;
	int _header52;
	int _header56;
	int _header60;
	int _minHeight;
	int _maxHeight;
	int _firstChar;
	int _lastChar;
	int _numGlyphs;
	int _blobSize;
	Common::Array<CTexture *> _pages;
	uint16 *_charIndex;
	Glyph *_glyphs;
	byte *_blob;
	LString _fileName;

protected:
	bool resLoadItem(const char *itemName) override;
};

} // End of namespace Grim

#endif
