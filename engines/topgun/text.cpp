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

#include "common/archive.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/str.h"
#include "common/textconsole.h"
#include "common/ustr.h"

#include "graphics/font.h"
#include "graphics/fontman.h"
#include "graphics/managed_surface.h"
#ifdef USE_FREETYPE2
#include "graphics/fonts/ttf.h"
#endif

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

TextObject_v1_2::TextObject_v1_2() : handle(0), id(0), font(nullptr), fontOwned(false), fontHandle(0), color(0), leadByteBuffered(false), leadByte(0) {
	for (uint16 i = 0; i < TG_TEXT_V1_2_3_EDIT_RECORD_SIZE; i++)
		edit[i] = 0;

	for (uint16 i = 0; i < TG_TEXT_V1_2_1_HEADER_SIZE; i++)
		data[i] = 0;

	for (uint16 i = 0; i < TG_TEXT_V1_2_TEXT_SIZE; i++) {
		text[i] = 0;
		undo[i] = 0;
	}

	for (uint16 i = 0; i < TG_TEXT_V1_2_NUM_SPRITES; i++)
		sprites[i] = 0;
}

TextObject_v1_2::~TextObject_v1_2() {
	if (fontOwned)
		delete font;
}

bool TopgunEngine_v1_2::ensureTextObjectResourceLoaded(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "ensureTextObjectResourceLoaded"))
		return false;

	if (_records[index].object != 0)
		return true;

	byte *data = loadResourceWithProcessingPause(id, nullptr);
	if (!data)
		return false;

	uint16 handle = createTextObject(id, data);
	_records[index].object = handle;

	return handle != 0;
}

bool TopgunEngine_v1_2::destroyTextObjectResource(uint16 id) {
	uint16 index = operandValue(id);
	if (!isRecordIndexValid(index, "destroyTextObjectResource"))
		return true;

	ResourceRecord &record = _records[index];

	if (record.data && record.refCount == 0) {
		utilsLibrary()->tgFree(record.data);
		record.data = nullptr;

		record.object = destroyTextObject(record.object);
	}

	return true;
}

TextObject_v1_2 *TopgunEngine_v1_2::textObjectFromHandle(uint16 handle) {
	if (handle == 0 || handle > _textObjects.size())
		return nullptr;

	return _textObjects[handle - 1];
}

uint16 TopgunEngine_v1_2::createTextObject(uint16 id, const byte *data) {
	TextObject_v1_2 *object = new TextObject_v1_2();

	uint16 handle = 0;
	for (uint32 i = 0; i < _textObjects.size(); i++) {
		if (!_textObjects[i]) {
			_textObjects[i] = object;
			handle = (uint16)(i + 1);
			break;
		}
	}

	if (handle == 0) {
		_textObjects.push_back(object);
		handle = (uint16)_textObjects.size();
	}

	object->handle = handle;
	object->id = id;
	uint16 headerSize = TG_TEXT_V1_2_DATA_SIZE;
	if (_version >= kVersionV1_2_4)
		headerSize = TG_TEXT_V1_2_3_HEADER_SIZE;
	else if (_version >= kVersionV1_2_2_2)
		headerSize = TG_TEXT_V1_2_3_SHORT_HEADER_SIZE;
	else if (_version >= kVersionV1_2_1)
		headerSize = TG_TEXT_V1_2_1_HEADER_SIZE;

	memcpy(object->data, data, headerSize);

	if (_version >= kVersionV1_2_2_2)
		object->color = READ_LE_UINT16(object->data + TG_TEXT_V1_2_3_HEADER_COLOR);
	else if (_version >= kVersionV1_2_1)
		object->color = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_COLOR);

	const char *initialText = getStringFromId(READ_LE_UINT16(data + ((_version >= kVersionV1_2_2_2) ? TG_TEXT_V1_2_3_HEADER_TEXT : TG_TEXT_V1_2_DATA_TEXT)));
	if (initialText)
		Common::strlcpy(object->text, initialText, TG_TEXT_V1_2_TEXT_SIZE);

	if (_version >= kVersionV1_2_1)
		refreshTextObjectFont(object);
	else
		createTextObjectFont(object);

	if (!refreshTextObjectBitmap(handle))
		return destroyTextObject(handle);

	return handle;
}

uint16 TopgunEngine_v1_2::destroyTextObject(uint16 handle) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return 0;

	if (_version >= kVersionV1_2_1 && object->fontHandle != 0) {
		deleteTextObjectFont(object->fontHandle);
		object->fontHandle = 0;
	}

	uint16 index = operandValue(object->id);
	if (isRecordIndexValid(index, "destroyTextObject")) {
		if (_records[index].data) {
			// The record must not keep the freed block
			_records[index].data = utilsLibrary()->tgFree(_records[index].data);
		}
	}

	if (_textEditObjectId == object->id)
		_textEditObjectId = 0;

	_textObjects[handle - 1] = nullptr;
	delete object;

	return 0;
}

void TopgunEngine_v1_2::destroyAllTextObjects() {
	for (uint32 i = 0; i < _textObjects.size(); i++)
		delete _textObjects[i];

	_textObjects.clear();

	for (uint32 i = 0; i < _textFonts.size(); i++) {
		if (_textFonts[i].owned)
			delete _textFonts[i].font;
	}

	_textFonts.clear();
}

bool TopgunEngine_v1_2::registerTextObjectSpriteUse(uint16 object, uint16 sprite) {
	TextObject_v1_2 *textObject = textObjectFromHandle(object);
	if (!textObject) {
		warning("TopgunEngine_v1_2::registerTextObjectSpriteUse(): the text object %d does not exist", object);
		return false;
	}

	unregisterTextObjectSpriteUse(object, sprite);

	for (uint16 i = 0; i < TG_TEXT_V1_2_NUM_SPRITES; i++) {
		if (textObject->sprites[i] == 0) {
			textObject->sprites[i] = sprite;
			return true;
		}
	}

	return false;
}

void TopgunEngine_v1_2::unregisterTextObjectSpriteUse(uint16 object, uint16 sprite) {
	TextObject_v1_2 *textObject = textObjectFromHandle(object);
	if (!textObject) {
		if (object != 0)
			warning("TopgunEngine_v1_2::unregisterTextObjectSpriteUse(): the text object %d does not exist", object);

		return;
	}

	for (uint16 i = 0; i < TG_TEXT_V1_2_NUM_SPRITES; i++) {
		if (textObject->sprites[i] == sprite)
			textObject->sprites[i] = 0;
	}
}

void TopgunEngine_v1_2::loadGameFonts() {
	_gameFontsLoaded = true;

	Common::ArchiveMemberList members;
	SearchMan.listMatchingMembers(members, "*.ttf");

	for (Common::ArchiveMemberList::const_iterator it = members.begin(); it != members.end(); ++it) {
		Common::SeekableReadStream *stream = (*it)->createReadStream();
		if (!stream)
			continue;

		Common::String fullName;
		uint16 numTables = 0;

		if (stream->size() >= 12) {
			stream->skip(4);
			numTables = stream->readUint16BE();
			stream->skip(6);
		}

		uint32 nameOffset = 0;
		for (uint16 i = 0; i < numTables; i++) {
			uint32 tag = stream->readUint32BE();
			stream->skip(4);
			uint32 offset = stream->readUint32BE();
			stream->skip(4);

			if (tag == MKTAG('n', 'a', 'm', 'e')) {
				nameOffset = offset;
				break;
			}
		}

		if (nameOffset != 0 && stream->seek(nameOffset + 2)) {
			uint16 count = stream->readUint16BE();
			uint16 stringOffset = stream->readUint16BE();

			for (uint16 i = 0; i < count && fullName.empty(); i++) {
				uint16 platform = stream->readUint16BE();
				stream->skip(4);
				uint16 nameId = stream->readUint16BE();
				uint16 length = stream->readUint16BE();
				uint16 offset = stream->readUint16BE();

				if (nameId != TG_TEXT_V1_2_FONT_FULL_NAME || (platform != 1 && platform != 3))
					continue;

				int32 next = stream->pos();
				if (!stream->seek(nameOffset + stringOffset + offset))
					break;

				for (uint16 j = 0; j < length; j += (platform == 3) ? 2 : 1) {
					byte first = stream->readByte();
					byte character = (platform == 3) ? stream->readByte() : first;

					fullName += (char)character;
				}

				stream->seek(next);
			}
		}

		if (!fullName.empty())
			_gameFonts.setVal(fullName, (*it)->getPathInArchive());

		delete stream;
	}
}

Graphics::Font *TopgunEngine_v1_2::createGameFont(const Common::String &face, int size, bool cell) {
#ifdef USE_FREETYPE2
	if (!_gameFontsLoaded)
		loadGameFonts();

	if (!_gameFonts.contains(face))
		return nullptr;

	Common::File *file = new Common::File();
	if (!file->open(_gameFonts[face])) {
		delete file;
		return nullptr;
	}

	return Graphics::loadTTFFont(file, DisposeAfterUse::YES, size, cell ? Graphics::kTTFSizeModeCell : Graphics::kTTFSizeModeCharacter, 0, 0, Graphics::kTTFRenderModeMonochrome);
#else
	return nullptr;
#endif
}

void TopgunEngine_v1_2::createTextObjectFont(TextObject_v1_2 *object) {
	const char *face = getStringFromId(READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_FACE));

	if (object->fontOwned)
		delete object->font;

	object->font = nullptr;
	object->fontOwned = false;

	Common::String name;
	if (face)
		name = face;

	bool bold = name.hasSuffixIgnoreCase(" Bold");
	bool serif = name.hasPrefixIgnoreCase("Times");

	const char *file;
	if (serif)
		file = bold ? "LiberationSerif-Bold.ttf" : "LiberationSerif-Regular.ttf";
	else
		file = bold ? "LiberationSans-Bold.ttf" : "LiberationSans-Regular.ttf";

#ifdef USE_FREETYPE2
	// A positive height is the height of the cell, a negative one the height of the characters
	int16 height = (int16)operandValue(READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_HEIGHT));
	int size = (height != 0) ? ABS((int)height) : TG_TEXT_V1_2_DEFAULT_HEIGHT;
	Graphics::TTFSizeMode sizeMode = (height < 0) ? Graphics::kTTFSizeModeCharacter : Graphics::kTTFSizeModeCell;

	object->font = createGameFont(name, size, height >= 0);
	if (!object->font)
		object->font = Graphics::loadTTFFontFromArchive(file, size, sizeMode, 0, 0, Graphics::kTTFRenderModeMonochrome);

	if (object->font)
		object->fontOwned = true;
#endif

	if (!object->font) {
		warning("TopgunEngine_v1_2::createTextObjectFont(): the font %s is not available", file);
		object->font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	}
}

uint16 TopgunEngine_v1_2::createTextObjectFont(int16 height, const char *face) {
	for (uint32 i = 0; i < _textFonts.size(); i++) {
		if (_textFonts[i].height == height && scumm_stricmp(_textFonts[i].face, face) == 0) {
			_textFonts[i].refCount++;
			return _textFonts[i].handle;
		}
	}

	if (_textFonts.size() >= TG_TEXT_V1_2_3_MAX_FONTS)
		return 0;

	TextFont_v1_2 entry;

	Common::String name(face);

	bool bold = name.hasSuffixIgnoreCase(" Bold");
	bool serif = name.hasPrefixIgnoreCase("Times");

	const char *file;
	if (serif)
		file = bold ? "LiberationSerif-Bold.ttf" : "LiberationSerif-Regular.ttf";
	else
		file = bold ? "LiberationSans-Bold.ttf" : "LiberationSans-Regular.ttf";

#ifdef USE_FREETYPE2
	// A positive height is the height of the cell, a negative one the height of the characters
	int size = (height != 0) ? ABS((int)height) : TG_TEXT_V1_2_DEFAULT_HEIGHT;
	Graphics::TTFSizeMode sizeMode = (height < 0) ? Graphics::kTTFSizeModeCharacter : Graphics::kTTFSizeModeCell;

	entry.font = createGameFont(name, size, height >= 0);
	if (!entry.font)
		entry.font = Graphics::loadTTFFontFromArchive(file, size, sizeMode, 0, 0, Graphics::kTTFRenderModeMonochrome);

	if (entry.font)
		entry.owned = true;
#endif

	if (!entry.font) {
		warning("TopgunEngine_v1_2::createTextObjectFont(): the font %s is not available", file);
		entry.font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	}

	entry.handle = _nextTextFontHandle;
	_nextTextFontHandle++;
	if (_nextTextFontHandle == 0)
		_nextTextFontHandle = 1;

	entry.refCount = 1;
	entry.height = height;
	Common::strlcpy(entry.face, face, TG_TEXT_V1_2_3_FACE_SIZE);

	_textFonts.push_back(entry);
	return entry.handle;
}

void TopgunEngine_v1_2::deleteTextObjectFont(uint16 font) {
	for (uint32 i = 0; i < _textFonts.size(); i++) {
		if (_textFonts[i].handle != font)
			continue;

		_textFonts[i].refCount--;
		if (_textFonts[i].refCount != 0)
			return;

		if (_textFonts[i].owned)
			delete _textFonts[i].font;

		_textFonts.remove_at(i);
		return;
	}
}

void TopgunEngine_v1_2::refreshTextObjectFont(TextObject_v1_2 *object) {
	uint16 faceOffset = (_version >= kVersionV1_2_2_2) ? TG_TEXT_V1_2_3_HEADER_FACE : TG_TEXT_V1_2_DATA_FACE;
	uint16 heightOffset = (_version >= kVersionV1_2_2_2) ? TG_TEXT_V1_2_3_HEADER_HEIGHT : TG_TEXT_V1_2_DATA_HEIGHT;

	const char *face = getStringFromId(READ_LE_UINT16(object->data + faceOffset));
	if (!face)
		return;

	if (object->fontHandle != 0) {
		deleteTextObjectFont(object->fontHandle);
		object->fontHandle = 0;
	}

	int16 height = (int16)operandValue(READ_LE_UINT16(object->data + heightOffset));
	object->fontHandle = createTextObjectFont(height, face);
}

Graphics::ManagedSurface *TopgunEngine_v1_2::renderTextToBitmap(uint16 color, const char *text, const Graphics::Font *font) {
	Common::U32String string(text, Common::kWindows1252);

	int width = font->getStringWidth(string);
	int height = font->getFontHeight();

	// A surface without pixels cannot be created
	if (width <= 0 || height <= 0)
		return nullptr;

	Graphics::ManagedSurface *surface = new Graphics::ManagedSurface(width, height, Graphics::PixelFormat::createFormatCLUT8());

	surface->clear(0);

	uint32 textColor = display()->hasPalette() ? (color & 0xFF) : TG_TEXT_V1_2_SOLID_COLOR;

	if (_version >= kVersionV1_2_10 && display()->hasPalette()) {
		// Black turns into the transparent index, matched on the color of the entry without fades
		byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
		spriteLib()->spriteGetPalette(entry, (byte)color, 1);
		textColor = display()->getDeviceColorIndex((byte)color, entry);
	}

	font->drawString(surface, string, 0, 0, width, textColor, Graphics::kTextAlignLeft, 0, false, true);

	return surface;
}

Graphics::ManagedSurface *TopgunEngine_v1_2::renderTextToBitmap(const TextObject_v1_2 *object) {
	const Graphics::Font *font = nullptr;

	for (uint32 i = 0; i < _textFonts.size(); i++) {
		if (_textFonts[i].handle == object->fontHandle)
			font = _textFonts[i].font;
	}

	if (!font)
		font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);

	uint16 flags;
	int16 boxWidth;
	int16 boxHeight;

	if (_version >= kVersionV1_2_2_2) {
		flags = READ_LE_UINT16(object->data + TG_TEXT_V1_2_3_HEADER_FLAGS);
		if (_version < kVersionV1_2_4) {
			flags &= 0xFF;

			if (_version >= kVersionV1_2_3)
				flags |= TG_TEXT_V1_2_3_FLAG_NO_PREFIX;
		}

		boxWidth = (int16)READ_LE_UINT16(object->data + TG_TEXT_V1_2_3_HEADER_BOX_WIDTH);
		boxHeight = (int16)READ_LE_UINT16(object->data + TG_TEXT_V1_2_3_HEADER_BOX_HEIGHT);
	} else {
		flags = object->data[TG_TEXT_V1_2_1_HEADER_FLAGS];
		boxWidth = (int16)READ_LE_UINT16(object->data + TG_TEXT_V1_2_1_HEADER_BOX_WIDTH);
		boxHeight = (int16)READ_LE_UINT16(object->data + TG_TEXT_V1_2_1_HEADER_BOX_HEIGHT);
	}

	bool hasBox = (boxWidth != 0 && boxHeight != 0);

	if (hasBox)
		flags |= TG_TEXT_V1_2_3_FLAG_WORD_BREAK;

	const char *text = (object->text[0] != '\0') ? object->text : " ";

	Common::String visible;
	for (const char *c = text; *c != '\0'; c++) {
		// The underline of the prefix character is not drawn
		if (*c == '&' && (flags & TG_TEXT_V1_2_3_FLAG_NO_PREFIX) == 0) {
			c++;

			if (*c == '\0')
				break;
		}

		visible += *c;
	}

	Common::U32String string(visible.c_str(), Common::kWindows1252);
	Common::Array<Common::U32String> lines;

	if ((flags & TG_TEXT_V1_2_3_FLAG_SINGLE_LINE) != 0)
		lines.push_back(string);
	else
		font->wordWrapText(string, (flags & TG_TEXT_V1_2_3_FLAG_WORD_BREAK) != 0 ? boxWidth : TG_TEXT_V1_2_3_UNLIMITED_WIDTH, lines);

	int lineHeight = font->getFontHeight();
	int width = 0;

	for (uint32 i = 0; i < lines.size(); i++)
		width = MAX(width, font->getStringWidth(lines[i]));

	int height = (int)lines.size() * lineHeight;

	if (hasBox) {
		width = MIN<int>(boxWidth, width);
		height = MIN<int>(boxHeight, height);
	}

	int surfaceWidth = width + 1;
	int surfaceHeight = height + 1;

	Graphics::ManagedSurface *surface = new Graphics::ManagedSurface(surfaceWidth, surfaceHeight, Graphics::PixelFormat::createFormatCLUT8());

	surface->clear(0);

	uint32 textColor = display()->hasPalette() ? (object->color & 0xFF) : TG_TEXT_V1_2_SOLID_COLOR;

	if (_version >= kVersionV1_2_10 && display()->hasPalette()) {
		byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
		spriteLib()->spriteGetPalette(entry, (byte)object->color, 1);
		textColor = display()->getDeviceColorIndex((byte)object->color, entry);
	}

	int top = 0;

	if ((flags & TG_TEXT_V1_2_3_FLAG_SINGLE_LINE) != 0) {
		if ((flags & TG_TEXT_V1_2_3_FLAG_BOTTOM) != 0)
			top = surfaceHeight - lineHeight;
		else if ((flags & TG_TEXT_V1_2_3_FLAG_VERTICAL_CENTER) != 0)
			top = (surfaceHeight - lineHeight) / 2;
	}

	for (uint32 i = 0; i < lines.size(); i++) {
		int lineWidth = font->getStringWidth(lines[i]);
		int x = 0;

		if ((flags & TG_TEXT_V1_2_3_FLAG_RIGHT) != 0)
			x = surfaceWidth - lineWidth;
		else if ((flags & TG_TEXT_V1_2_3_FLAG_CENTER) != 0)
			x = (surfaceWidth - lineWidth) / 2;

		if (lineWidth > 0)
			font->drawString(surface, lines[i], x, top + (int)i * lineHeight, lineWidth, textColor, Graphics::kTextAlignLeft, 0, false, true);
	}

	return surface;
}

byte *TopgunEngine_v1_2::createDIBFromBitmapHandle(const Graphics::ManagedSurface *bitmap, bool remap, byte color) {
	uint16 width = (uint16)bitmap->w;
	uint16 height = (uint16)bitmap->h;

	byte *dib = spriteLib()->spriteDibCreate(width, height, 0);
	if (!dib)
		return nullptr;

	byte *pixels = dib + READ_LE_UINT16(dib);
	uint16 pitch = READ_LE_UINT16(dib + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);

	for (uint16 y = 0; y < height; y++) {
		byte *row = pixels + (uint32)(height - 1 - y) * pitch;

		memcpy(row, bitmap->getBasePtr(0, y), width);

		for (uint16 x = width; x < pitch; x++)
			row[x] = 0;
	}

	if (remap)
		warning("remapDIBToCurrentPalette UNIMPLEMENTED");

	return dib;
}

byte *TopgunEngine_v1_2::buildTextObjectBitmap(TextObject_v1_2 *object) {
	Graphics::ManagedSurface *surface;
	uint16 color;

	if (_version >= kVersionV1_2_1) {
		color = object->color;
		surface = renderTextToBitmap(object);
	} else {
		const char *text = (object->text[0] != '\0') ? object->text : " ";

		color = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_COLOR);
		surface = renderTextToBitmap(color, text, object->font);
	}

	byte *bitmap = nullptr;
	if (surface)
		bitmap = createDIBFromBitmapHandle(surface, false, 0);

	if (bitmap && !display()->hasPalette()) {
		byte table[256];
		for (uint16 i = 0; i < 256; i++)
			table[i] = 0;

		table[TG_TEXT_V1_2_SOLID_COLOR] = (byte)color;

		uint32 count = (uint32)READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
		utilsLibrary()->mapBytes(bitmap + READ_LE_UINT16(bitmap), count, table);
	}

	delete surface;
	return bitmap;
}

void TopgunEngine_v1_2::positionTextObject(byte *bitmap, const TextObject_v1_2 *object) {
	uint16 hotX;
	uint16 hotY;
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);

	if (_version >= kVersionV1_2_1) {
		uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
		byte flags;

		if (_version >= kVersionV1_2_2_2) {
			flags = object->data[TG_TEXT_V1_2_3_HEADER_FLAGS];
			hotX = READ_LE_UINT16(object->data + TG_TEXT_V1_2_3_HEADER_HOT_X);
			hotY = READ_LE_UINT16(object->data + TG_TEXT_V1_2_3_HEADER_HOT_Y);
		} else {
			flags = object->data[TG_TEXT_V1_2_1_HEADER_FLAGS];
			hotX = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_X);
			hotY = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_Y);
		}

		if ((flags & TG_TEXT_V1_2_3_FLAG_RIGHT) != 0)
			hotX = (uint16)(hotX - (width >> 1));
		else if ((flags & TG_TEXT_V1_2_3_FLAG_CENTER) == 0)
			hotX = (uint16)(hotX + ((uint16)(width - 1) >> 1));

		if ((flags & TG_TEXT_V1_2_3_FLAG_BOTTOM) != 0)
			hotY = (uint16)(hotY - (height >> 1));
		else if ((flags & TG_TEXT_V1_2_3_FLAG_VERTICAL_CENTER) == 0)
			hotY = (uint16)(hotY + ((uint16)(height - 1) >> 1));
	} else {
		byte alignment = object->data[TG_TEXT_V1_2_DATA_ALIGNMENT];

		hotX = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_X);
		hotY = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_Y);

		if (alignment == TG_TEXT_V1_2_ALIGN_CENTER)
			hotX = (uint16)(hotX + ((uint16)(width - 1) >> 1));
		else if (alignment == TG_TEXT_V1_2_ALIGN_RIGHT)
			hotX = (uint16)(hotX - (width >> 1));
	}

	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET, hotX);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET, hotY);
}

void TopgunEngine_v1_2::replaceTextBitmapInSprites(byte *oldBitmap, byte *newBitmap, const TextObject_v1_2 *object) {
	if (!oldBitmap)
		return;

	uint16 bitmapResourceIdOffset = (_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID;

	CelReference oldCel(oldBitmap, READ_LE_UINT16(oldBitmap + bitmapResourceIdOffset));
	CelReference newCel(newBitmap, READ_LE_UINT16(newBitmap + bitmapResourceIdOffset));

	for (uint16 i = 0; i < TG_TEXT_V1_2_NUM_SPRITES; i++) {
		if (object->sprites[i] != 0)
			spriteLib()->spriteReplaceBitmap(object->sprites[i], oldCel, newCel);
	}

	utilsLibrary()->tgFree(oldBitmap);
}

bool TopgunEngine_v1_2::refreshTextObjectBitmap(uint16 handle) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object) {
		warning("TopgunEngine_v1_2::refreshTextObjectBitmap(): the text object %d does not exist", handle);
		return false;
	}

	byte *bitmap = buildTextObjectBitmap(object);
	if (!bitmap)
		return false;

	WRITE_LE_UINT16(bitmap + ((_version >= kVersionV1_2_2_2) ? TG_HOST_V1_2_3_BITMAP_RESOURCE_ID : TG_HOST_V1_2_BITMAP_RESOURCE_ID), object->id);
	positionTextObject(bitmap, object);

	uint16 index = operandValue(object->id);
	if (!isRecordIndexValid(index, "refreshTextObjectBitmap")) {
		utilsLibrary()->tgFree(bitmap);
		return false;
	}

	byte *oldBitmap = _records[index].data;
	_records[index].data = bitmap;

	replaceTextBitmapInSprites(oldBitmap, bitmap, object);
	return true;
}

bool TopgunEngine_v1_2::beginTextEdit(const byte *record) {
	if (_version >= kVersionV1_2_6)
		_hasPendingChar = false;

	uint16 handle = getTextObjectFromId(READ_LE_UINT16(record + 2));
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

	memcpy(object->edit, record, (_version >= kVersionV1_2_2_2) ? TG_TEXT_V1_2_3_EDIT_RECORD_SIZE : TG_TEXT_V1_2_EDIT_RECORD_SIZE);

	if (object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] == 0)
		object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] = TG_TEXT_V1_2_DEFAULT_MAX_LENGTH;

	if (_version >= kVersionV1_2_2_2)
		Common::strlcpy(_textEditUndo, object->text, TG_TEXT_V1_2_TEXT_SIZE);
	else
		memcpy(object->undo, object->text, strlen(object->text) + 1);

	if (_version >= kVersionV1_2_1)
		textEditAppendCaret(handle);

	_textEditObjectId = READ_LE_UINT16(record + 2);
	return true;
}

// The argument is not used by the first version, the edit that is active is the one committed
void TopgunEngine_v1_2::commitTextEdit(uint16 cancel) {
	uint16 object = getTextObjectFromId(_textEditObjectId);

	if (_version >= kVersionV1_2_1) {
		if (object != 0)
			endTextEdit(object, cancel != 0);

		return;
	}

	if (object != 0) {
		if (endTextEdit(object, true))
			refreshTextObjectBitmap(object);
	}

	_textEditObjectId = 0;
}

void TopgunEngine_v1_2::textEditAppendCaret(uint16 handle) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return;

	uint16 length = (uint16)strlen(object->text);

	if (length + 1 >= TG_TEXT_V1_2_TEXT_SIZE) {
		warning("TopgunEngine_v1_2::textEditAppendCaret(): the text does not fit in the object");
		return;
	}

	object->text[length] = '_';
	object->text[length + 1] = '\0';
}

bool TopgunEngine_v1_2::endTextEdit(uint16 handle, bool cancel) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

	if (cancel) {
		if (_version >= kVersionV1_2_2_2)
			Common::strlcpy(object->text, _textEditUndo, TG_TEXT_V1_2_TEXT_SIZE);
		else
			memcpy(object->text, object->undo, strlen(object->undo) + 1);
	} else if (_version >= kVersionV1_2_1) {
		uint16 length = (uint16)strlen(object->text);

		if (length == 0)
			warning("TopgunEngine_v1_2::endTextEdit(): the text has no caret to remove");
		else
			object->text[length - 1] = '\0';
	}

	uint16 stringId = READ_LE_UINT16(object->edit + TG_TEXT_V1_2_EDIT_STRING);

	char *destination = nullptr;
	if ((stringId & TG_HOST_V1_2_STRING_ID_CONSTANT) == 0)
		destination = getStringFromId(stringId);

	if (destination)
		Common::strlcpy(destination, object->text, TG_HOST_V1_2_STRING_SLOT_SIZE);

	uint16 script = READ_LE_UINT16(object->edit + TG_TEXT_V1_2_EDIT_SCRIPT);
	if (script != 0)
		postRunScript(script, 0, 0);

	if (_version >= kVersionV1_2_1)
		refreshTextObjectBitmap(handle);

	_textEditObjectId = 0;
	return _version < kVersionV1_2_1;
}

bool TopgunEngine_v1_2::textEditBackspace(uint16 handle, bool clearAll) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

	uint16 length = (uint16)strlen(object->text);

	if (_version >= kVersionV1_2_7) {
		uint16 position = 0;

		if (!clearAll) {
			int16 caret = (int16)(length - 1);

			if (caret < 1) {
				warning("textEditBeep UNIMPLEMENTED");
				return false;
			}

			// The position of the character before the caret; a Shift-JIS character takes two bytes
			uint16 scan = 0;

			while (scan < (uint16)caret) {
				position = scan;

				byte current = (byte)object->text[scan];
				if (_version == kVersionV1_2_9 && _gameDescription->language == Common::JA_JPN &&
						((current >= 0x81 && current <= 0x9F) || (current >= 0xE0 && current <= 0xFC)) && object->text[scan + 1] != '\0')
					scan += 2;
				else
					scan++;
			}
		}

		object->text[position] = '\0';
		Common::strlcat(object->text, "_", TG_TEXT_V1_2_TEXT_SIZE);
		return true;
	}

	if (_version >= kVersionV1_2_1) {
		if ((int16)(length - 1) < 1) {
			if (_version >= kVersionV1_2_2_2)
				warning("textEditBeep UNIMPLEMENTED");

			return false;
		}

		object->text[length - 2] = '_';
		object->text[length - 1] = '\0';
		return true;
	}

	if (length < 1)
		return false;

	object->text[length - 1] = '\0';
	return true;
}

bool TopgunEngine_v1_2::textEditAppendChar(uint16 handle, byte character) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

	if (_version >= kVersionV1_2_7) {
		uint16 value = character;

		if (object->leadByteBuffered) {
			value = (uint16)((object->leadByte << 8) | character);
		} else if (_version == kVersionV1_2_9 && _gameDescription->language == Common::JA_JPN &&
				((character >= 0x81 && character <= 0x9F) || (character >= 0xE0 && character <= 0xFC))) {
			object->leadByte = character;
			object->leadByteBuffered = true;
			return false;
		}

		if (object->edit[TG_TEXT_V1_2_3_EDIT_NUMERIC] != 0 && (value < '0' || value > '9')) {
			warning("textEditBeep UNIMPLEMENTED");
			return false;
		}

		uint16 length = (uint16)strlen(object->text);

		if (length == 0) {
			warning("TopgunEngine_v1_2::textEditAppendChar(): the text has no caret");
			return false;
		}

		uint16 position = length - 1;

		if ((int16)object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] <= (int16)position) {
			warning("textEditBeep UNIMPLEMENTED");
			return false;
		}

		if (position + 4 > TG_TEXT_V1_2_TEXT_SIZE) {
			warning("TopgunEngine_v1_2::textEditAppendChar(): the text does not fit in the object");
			return false;
		}

		if (object->leadByteBuffered) {
			object->text[position] = (char)(value >> 8);
			position++;
			object->leadByteBuffered = false;
		}

		object->text[position] = (char)(value & 0xFF);
		object->text[position + 1] = '\0';
		Common::strlcat(object->text, "_", TG_TEXT_V1_2_TEXT_SIZE);
		return true;
	}

	if (_version >= kVersionV1_2_1) {
		if (_version >= kVersionV1_2_2_3 && object->edit[TG_TEXT_V1_2_3_EDIT_NUMERIC] != 0 && (character < '0' || character > '9')) {
			warning("textEditBeep UNIMPLEMENTED");
			return false;
		}

		uint16 length = (uint16)strlen(object->text);

		if (length == 0) {
			warning("TopgunEngine_v1_2::textEditAppendChar(): the text has no caret");
			return false;
		}

		if ((int16)object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] <= (int16)(length - 1)) {
			if (_version >= kVersionV1_2_2_2)
				warning("textEditBeep UNIMPLEMENTED");

			return false;
		}

		object->text[length - 1] = (char)character;
		textEditAppendCaret(handle);
		return true;
	}

	uint16 length = (uint16)strlen(object->text);

	if (object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] <= length)
		return false;

	if (length + 1 >= TG_TEXT_V1_2_TEXT_SIZE) {
		warning("TopgunEngine_v1_2::textEditAppendChar(): the text does not fit in the object");
		return false;
	}

	object->text[length] = (char)character;
	object->text[length + 1] = '\0';
	return true;
}

bool TopgunEngine_v1_2::o_formatTextObject(const byte *record) {
	uint16 type = kResV1_2Text;
	if (_version >= kVersionV1_2_10)
		type = kResV1_2_10Text;
	else if (_version >= kVersionV1_2_5)
		type = kResV1_2_5Text;
	else if (_version >= kVersionV1_2_2_1)
		type = kResV1_2_3Text;
	else if (_version >= kVersionV1_2_2)
		type = TG_TEXT_V1_2_2_RESOURCE_TYPE;
	else if (_version >= kVersionV1_2_1)
		type = kResV1_2_1Text;

	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), type))
		return false;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_formatTextObject"))
		return false;

	TextObject_v1_2 *object = textObjectFromHandle(_records[index].object);

	const char *format = getStringFromId(READ_LE_UINT16(record + 4));
	if (!format)
		return false;

	if (!object) {
		warning("TopgunEngine_v1_2::o_formatTextObject(): the text object %d does not exist", _records[index].object);
		return false;
	}

	FormatArgument_v1_2 argument;
	argument.value = operandValue(READ_LE_UINT16(record + 6));

	Common::String text = formatWindowsString(format, &argument, 1);
	Common::strlcpy(object->text, text.c_str(), TG_TEXT_V1_2_TEXT_SIZE);

	return refreshTextObjectBitmap(object->handle);
}

void TopgunEngine_v1_2_3::o_setTextParam(const byte *record) {
	uint16 textType = kResV1_2_3Text;
	if (_version >= kVersionV1_2_10)
		textType = kResV1_2_10Text;
	else if (_version >= kVersionV1_2_5)
		textType = kResV1_2_5Text;

	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), textType))
		return;

	uint16 index = operandValue(READ_LE_UINT16(record + 2));
	if (!isRecordIndexValid(index, "o_setTextParam"))
		return;

	TextObject_v1_2 *object = textObjectFromHandle(_records[index].object);
	if (!object) {
		warning("TopgunEngine_v1_2_3::o_setTextParam(): the text object %d does not exist", _records[index].object);
		return;
	}

	object->color = READ_LE_UINT16(record + 4);
	refreshTextObjectBitmap(object->handle);
}

} // End of namespace Topgun
