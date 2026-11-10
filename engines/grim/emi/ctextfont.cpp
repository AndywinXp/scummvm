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
#include "common/util.h"

#include "engines/grim/emi/cdrawcontext.h"
#include "engines/grim/emi/ctextfont.h"
#include "engines/grim/emi/lconsoleservices.h"
#include "engines/grim/emi/lfile.h"
#include "engines/grim/emi/lmemoryservices.h"

namespace Grim {

static bool readInt(LFile *file, int *value) {
	byte bytes[4];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_INT32(bytes);

	return true;
}

static bool readByte(LFile *file, signed char *value) {
	byte data;

	if (file->read(&data, sizeof(data)) != sizeof(data))
		return false;

	*value = (signed char)data;

	return true;
}

static bool readWord(LFile *file, uint16 *value) {
	byte bytes[2];

	if (file->read(bytes, sizeof(bytes)) != sizeof(bytes))
		return false;

	*value = READ_LE_UINT16(bytes);

	return true;
}

Glyph::Glyph() {
	blobOffset = 0;
	advance = 0;
	offsetX = 0;
	offsetY = 0;
	reserved7 = 0;
	u0 = 0.0f;
	v0 = 0.0f;
	u1 = 0.0f;
	v1 = 0.0f;
	page = 0;
}

CTextFont::CTextFont() {
	lineHeight = 0;
	header52 = 0;
	header56 = 0;
	header60 = 0;
	minHeight = 0;
	maxHeight = 0;
	firstChar = 0;
	lastChar = 0;
	numGlyphs = 0;
	blobSize = 0;
	charIndex = nullptr;
	glyphs = nullptr;
	blob = nullptr;
}

CTextFont::~CTextFont() {
	if (CDrawContext::context) {
		for (uint i = 0; i < pages.size(); i++)
			CDrawContext::context->freeTexture(pages[i]);
	}

	pages.clear();

	if (blob)
		gpMemoryServices->free(blob);

	blob = nullptr;

	delete[] glyphs;
	glyphs = nullptr;

	if (charIndex)
		gpMemoryServices->free(charIndex);

	charIndex = nullptr;
}

uint CTextFont::resSizeOfItem() {
	uint size = sizeof(CTextFont) + fileName.size() + numGlyphs * (sizeof(Glyph) + sizeof(uint16));

	for (int i = 0; i < numGlyphs; i++)
		size += glyphs[i].alpha.getSizeBytes();

	size += blobSize + pages.size() * sizeof(CTexture *);

	for (uint i = 0; i < pages.size(); i++) {
		if (pages[i])
			size += pages[i]->getSize();
	}

	return size;
}

bool CTextFont::resLoadItem(const char *itemName) {
	bool result = loadFile(itemName);

	if (result) {
		resSetName(itemName);
		loaded = true;

		if (CDrawContext::context)
			buildTextures(CDrawContext::context);
		else
			warning("CTextFont::resLoadItem(): no draw context");
	}

	return result;
}

CTexture *CTextFont::getTexture(CDrawContext *ctx, uint page) {
	if (page >= pages.size()) {
		warning("CTextFont::getTexture(): page %u out of range", page);

		return nullptr;
	}

	CTexture *result = pages[page];

	if (!result) {
		buildTextures(ctx);

		if (page < pages.size())
			return pages[page];

		return nullptr;
	}

	return result;
}

bool CTextFont::loadFile(const char *path) {
	LFile *file = gpFileServices->open(path, "rb");

	if (!file)
		return false;

	fileName = path;

	if (!readInt(file, &numGlyphs) || !readInt(file, &blobSize) || !readInt(file, &lineHeight) ||
			!readInt(file, &header52) || !readInt(file, &header56) || !readInt(file, &header60) ||
			!readInt(file, &firstChar) || !readInt(file, &lastChar)) {
		warning("CTextFont::loadFile(): unexpected end of file");
		gpFileServices->close(file, true);

		return false;
	}

	numGlyphs = lastChar - firstChar + 1;

	if (numGlyphs <= 0 || numGlyphs > kMaxGlyphs || blobSize < 0) {
		warning("CTextFont::loadFile(): invalid header");
		numGlyphs = 0;
		gpFileServices->close(file, true);

		return false;
	}

	charIndex = (uint16 *)gpMemoryServices->malloc(numGlyphs * sizeof(uint16));

	if (!charIndex) {
		gpFileServices->close(file, true);

		return false;
	}

	for (int i = 0; i < numGlyphs; i++) {
		if (!readWord(file, &charIndex[i])) {
			warning("CTextFont::loadFile(): unexpected end of file");
			gpFileServices->close(file, true);

			return false;
		}
	}

	glyphs = new Glyph[numGlyphs];

	for (int i = 0; i < numGlyphs; i++) {
		Glyph &glyph = glyphs[i];
		int width;
		int height;

		if (!readInt(file, &glyph.blobOffset) || !readByte(file, &glyph.advance) || !readByte(file, &glyph.offsetX) ||
				!readByte(file, &glyph.offsetY) || !readByte(file, &glyph.reserved7) || !readInt(file, &width) ||
				!readInt(file, &height) || width < 0 || height < 0) {
			warning("CTextFont::loadFile(): invalid glyph");
			gpFileServices->close(file, true);

			return false;
		}

		glyph.alpha.create(width, height, LColorFormat::format2, nullptr);
		glyph.page = 0;
		glyph.v0 = 0.0f;
		glyph.u0 = 0.0f;
		glyph.v1 = 1.0f;
		glyph.u1 = 1.0f;

		minHeight = MIN<int>(minHeight, glyph.offsetY);
		maxHeight = MAX<int>(maxHeight, glyph.offsetY + height);
	}

	blob = (byte *)gpMemoryServices->malloc(blobSize * sizeof(byte));

	if (file->read(blob, blobSize) != (uint)blobSize) {
		warning("CTextFont::loadFile(): unexpected end of file");
		gpFileServices->close(file, true);

		return false;
	}

	gpFileServices->close(file, true);

	return true;
}

void CTextFont::buildTextures(CDrawContext *ctx) {
	int glyphsToPack = lastChar - firstChar + 1;

	if (glyphsToPack <= 0 || glyphsToPack > numGlyphs || !glyphs) {
		warning("CTextFont::buildTextures(): no glyphs");

		return;
	}

	LColorFormat format(9);
	LABitmap *bitmap = new LABitmap(kPageSize, kPageSize, format, nullptr, false);

	bitmap->bitmapName = fileName;

	Glyph *cursor = glyphs;
	int packed = 0;
	bool done = false;

	while (true) {
		Glyph *pageStart = cursor;
		uint pageHeight = 0;
		uint rowTop = 0;
		uint x = 0;

		while (true) {
			int glyphWidth = cursor->alpha.getWidth();
			int glyphHeight = cursor->alpha.getHeight();

			if (glyphWidth > kPageSize || glyphHeight > kPageSize) {
				warning("CTextFont::buildTextures(): glyph %d is too big", packed);
			} else {
				if (x + glyphWidth > kPageSize) {
					x = 0;
					rowTop = pageHeight;
				}

				if (rowTop + glyphHeight > pageHeight) {
					pageHeight = rowTop + glyphHeight;

					if (pageHeight > kPageSize)
						break;
				}

				if (cursor->blobOffset < 0 || (int64)cursor->blobOffset + (int64)glyphWidth * glyphHeight > blobSize) {
					warning("CTextFont::buildTextures(): glyph %d is outside the pixel data", packed);
				} else {
					cursor->alpha.setPixels(blob + cursor->blobOffset);

					const byte *source = (const byte *)cursor->alpha.getPixels();
					uint bitmapWidth = bitmap->getWidth();
					uint32 *destination = (uint32 *)bitmap->getPixels() + x + rowTop * bitmapWidth;

					for (int row = 0; row < glyphHeight; row++) {
						uint32 *pixel = destination;

						for (int column = 0; column < glyphWidth; column++) {
							if (source[column] == 0xFF)
								*pixel = 0xFFFFFFFF;
							else if (source[column] == 0x80)
								*pixel = 0xFF000000;
							else
								*pixel = 0;

							pixel++;
						}

						source += glyphWidth;
						destination += bitmapWidth;
					}
				}

				cursor->u0 = (float)x;
				cursor->v0 = (float)rowTop;
				cursor->u1 = (float)((double)glyphWidth + cursor->u0);
				cursor->v1 = (float)((double)glyphHeight + cursor->v0);
				x += glyphWidth;
			}

			cursor++;
			packed++;

			if (packed >= glyphsToPack) {
				done = true;

				break;
			}
		}

		for (uint height = bitmap->getHeight() >> 1; height > pageHeight; height >>= 1)
			bitmap->setHeight(height);

		CTexture *texture = ctx->createTexture(bitmap);

		if (texture) {
			texture->flags = (texture->flags & 0xFFFFFFFC) | 0x84;
			texture->applyFlags();
			pages.push_back(texture);

			float widthInverse = (float)(1.0 / (double)bitmap->getWidth());
			float heightInverse = (float)(1.0 / (double)bitmap->getHeight());

			for (Glyph *glyph = pageStart; glyph != cursor; glyph++) {
				glyph->page = pages.size() - 1;
				glyph->u0 = (float)((double)widthInverse * (double)glyph->u0);
				glyph->v0 = (float)((double)heightInverse * (double)glyph->v0);
				glyph->u1 = (float)((double)widthInverse * (double)glyph->u1);
				glyph->v1 = (float)((double)heightInverse * (double)glyph->v1);
			}
		} else if (gpConsoleServices) {
			gpConsoleServices->logError("Error allocating font texture!!\n");
		} else {
			warning("CTextFont::buildTextures(): error allocating font texture");
		}

		delete bitmap;
		bitmap = nullptr;

		if (done)
			break;

		bitmap = new LABitmap(kPageSize, kPageSize, format, nullptr, false);
		bitmap->bitmapName = fileName;
	}
}

} // End of namespace Grim
