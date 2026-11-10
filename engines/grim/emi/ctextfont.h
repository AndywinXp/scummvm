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

#include "engines/grim/emi/ctexture.h"
#include "engines/grim/emi/labitmap.h"
#include "engines/grim/emi/lresource.h"

namespace Grim {

class CDrawContext;

struct Glyph {
	Glyph();

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
		kMaxGlyphs = 65536
	};

	CTextFont();
	~CTextFont() override;

	uint resSizeOfItem() override;

	CTexture *getTexture(CDrawContext *ctx, uint page);
	void buildTextures(CDrawContext *ctx);
	bool loadFile(const char *path);

	int lineHeight;
	int header52;
	int header56;
	int header60;
	int minHeight;
	int maxHeight;
	int firstChar;
	int lastChar;
	int numGlyphs;
	int blobSize;
	Common::Array<CTexture *> pages;
	uint16 *charIndex;
	Glyph *glyphs;
	byte *blob;
	LString fileName;

protected:
	bool resLoadItem(const char *itemName) override;
};

} // End of namespace Grim

#endif
