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

#include "engines/grim/emi/graphics/cdrawcontext.h"
#include "engines/grim/emi/text/ctextfont.h"
#include "engines/grim/emi/emiengine.h"
#include "engines/grim/emi/core/lconsoleservices.h"
#include "engines/grim/emi/files/lfile.h"
#include "engines/grim/emi/core/lmemoryservices.h"

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

Glyph::Glyph(EmiEngine *engine) : alpha(engine) {
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
	_lineHeight = 0;
	_header52 = 0;
	_header56 = 0;
	_header60 = 0;
	_minHeight = 0;
	_maxHeight = 0;
	_firstChar = 0;
	_lastChar = 0;
	_numGlyphs = 0;
	_blobSize = 0;
	_charIndex = nullptr;
	_glyphs = nullptr;
	_blob = nullptr;
}

CTextFont::~CTextFont() {
	CDrawContext *drawContext = _engine->getDrawContext();

	if (drawContext) {
		for (uint i = 0; i < _pages.size(); i++)
			drawContext->freeTexture(_pages[i]);
	}

	_pages.clear();

	if (_blob)
		_engine->getMemoryServices()->freeImpl(_blob);

	_blob = nullptr;

	if (_glyphs) {
		for (int i = 0; i < _numGlyphs; i++)
			_glyphs[i].~Glyph();

		_engine->getMemoryServices()->freeImpl(_glyphs);
	}

	_glyphs = nullptr;

	if (_charIndex)
		_engine->getMemoryServices()->freeImpl(_charIndex);

	_charIndex = nullptr;
}

uint CTextFont::resSizeOfItem() {
	uint size = sizeof(CTextFont) + _fileName.size() + _numGlyphs * (sizeof(Glyph) + sizeof(uint16));

	for (int i = 0; i < _numGlyphs; i++)
		size += _glyphs[i].alpha.getSizeBytes();

	size += _blobSize + _pages.size() * sizeof(CTexture *);

	for (uint i = 0; i < _pages.size(); i++) {
		if (_pages[i])
			size += _pages[i]->getSize();
	}

	return size;
}

bool CTextFont::resLoadItem(const char *itemName) {
	bool result = loadFile(itemName);

	if (result) {
		resSetName(itemName);
		_loaded = true;

		if (_engine->getDrawContext())
			buildTextures(_engine->getDrawContext());
		else
			warning("CTextFont::resLoadItem(): no draw context");
	}

	return result;
}

CTexture *CTextFont::getTexture(CDrawContext *ctx, uint page) {
	if (page >= _pages.size()) {
		warning("CTextFont::getTexture(): page %u out of range", page);

		return nullptr;
	}

	CTexture *result = _pages[page];

	if (!result) {
		buildTextures(ctx);

		if (page < _pages.size())
			return _pages[page];

		return nullptr;
	}

	return result;
}

bool CTextFont::loadFile(const char *path) {
	LFileServices *fileServices = _engine->getFileServices();
	LFile *file = fileServices->open(path, "rb");

	if (!file)
		return false;

	_fileName = path;

	if (!readInt(file, &_numGlyphs) || !readInt(file, &_blobSize) || !readInt(file, &_lineHeight) ||
			!readInt(file, &_header52) || !readInt(file, &_header56) || !readInt(file, &_header60) ||
			!readInt(file, &_firstChar) || !readInt(file, &_lastChar)) {
		warning("CTextFont::loadFile(): unexpected end of file");
		fileServices->close(file, true);

		return false;
	}

	_numGlyphs = _lastChar - _firstChar + 1;

	if (_numGlyphs <= 0 || _numGlyphs > kMaxGlyphs || _blobSize < 0) {
		warning("CTextFont::loadFile(): invalid header");
		_numGlyphs = 0;
		fileServices->close(file, true);

		return false;
	}

	_charIndex = (uint16 *)_engine->getMemoryServices()->mallocImpl(_numGlyphs * sizeof(uint16));

	if (!_charIndex) {
		fileServices->close(file, true);

		return false;
	}

	for (int i = 0; i < _numGlyphs; i++) {
		if (!readWord(file, &_charIndex[i])) {
			warning("CTextFont::loadFile(): unexpected end of file");
			fileServices->close(file, true);

			return false;
		}
	}

	_glyphs = (Glyph *)_engine->getMemoryServices()->mallocImpl(_numGlyphs * sizeof(Glyph));

	if (!_glyphs) {
		fileServices->close(file, true);

		return false;
	}

	for (int i = 0; i < _numGlyphs; i++)
		new (&_glyphs[i]) Glyph(_engine);

	for (int i = 0; i < _numGlyphs; i++) {
		Glyph &glyph = _glyphs[i];
		int width;
		int height;

		if (!readInt(file, &glyph.blobOffset) || !readByte(file, &glyph.advance) || !readByte(file, &glyph.offsetX) ||
				!readByte(file, &glyph.offsetY) || !readByte(file, &glyph.reserved7) || !readInt(file, &width) ||
				!readInt(file, &height) || width < 0 || height < 0) {
			warning("CTextFont::loadFile(): invalid glyph");
			fileServices->close(file, true);

			return false;
		}

		glyph.alpha.create(width, height, LColorFormat::kFormat2, nullptr);
		glyph.page = 0;
		glyph.v0 = 0.0f;
		glyph.u0 = 0.0f;
		glyph.v1 = 1.0f;
		glyph.u1 = 1.0f;

		_minHeight = MIN<int>(_minHeight, glyph.offsetY);
		_maxHeight = MAX<int>(_maxHeight, glyph.offsetY + height);
	}

	_blob = (byte *)_engine->getMemoryServices()->mallocImpl(_blobSize * sizeof(byte));

	if (file->read(_blob, _blobSize) != (uint)_blobSize) {
		warning("CTextFont::loadFile(): unexpected end of file");
		fileServices->close(file, true);

		return false;
	}

	fileServices->close(file, true);

	return true;
}

void CTextFont::buildTextures(CDrawContext *ctx) {
	int glyphsToPack = _lastChar - _firstChar + 1;

	if (glyphsToPack <= 0 || glyphsToPack > _numGlyphs || !_glyphs) {
		warning("CTextFont::buildTextures(): no glyphs");

		return;
	}

	LColorFormat format(9);
	LABitmap *bitmap = new LABitmap(_engine, kPageSize, kPageSize, format, nullptr, false);

	bitmap->_bitmapName = _fileName;

	Glyph *cursor = _glyphs;
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

				if (cursor->blobOffset < 0 || (int64)cursor->blobOffset + (int64)glyphWidth * glyphHeight > _blobSize) {
					warning("CTextFont::buildTextures(): glyph %d is outside the pixel data", packed);
				} else {
					cursor->alpha.setPixels(_blob + cursor->blobOffset);

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
			texture->_flags = (texture->_flags & 0xFFFFFFFC) | 0x84;
			texture->applyFlags();
			_pages.push_back(texture);

			float widthInverse = (float)(1.0 / (double)bitmap->getWidth());
			float heightInverse = (float)(1.0 / (double)bitmap->getHeight());

			for (Glyph *glyph = pageStart; glyph != cursor; glyph++) {
				glyph->page = _pages.size() - 1;
				glyph->u0 = (float)((double)widthInverse * (double)glyph->u0);
				glyph->v0 = (float)((double)heightInverse * (double)glyph->v0);
				glyph->u1 = (float)((double)widthInverse * (double)glyph->u1);
				glyph->v1 = (float)((double)heightInverse * (double)glyph->v1);
			}
		} else if (_engine->getConsoleServices()) {
			_engine->getConsoleServices()->logError("Error allocating font texture!!\n");
		} else {
			warning("CTextFont::buildTextures(): error allocating font texture");
		}

		delete bitmap;
		bitmap = nullptr;

		if (done)
			break;

		bitmap = new LABitmap(_engine, kPageSize, kPageSize, format, nullptr, false);
		bitmap->_bitmapName = _fileName;
	}
}

} // End of namespace Grim
