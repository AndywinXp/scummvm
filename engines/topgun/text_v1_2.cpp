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
#include "topgun/topgun_v1_2.h"

namespace Topgun {

TextObject_v1_2::TextObject_v1_2() : handle(0), id(0), font(nullptr), fontOwned(false) {
	for (uint16 i = 0; i < TG_TEXT_V1_2_EDIT_RECORD_SIZE; i++)
		edit[i] = 0;

	for (uint16 i = 0; i < TG_TEXT_V1_2_DATA_SIZE; i++)
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
	memcpy(object->data, data, TG_TEXT_V1_2_DATA_SIZE);

	const char *initialText = getStringFromId(READ_LE_UINT16(data + TG_TEXT_V1_2_DATA_TEXT));
	if (initialText)
		Common::strlcpy(object->text, initialText, TG_TEXT_V1_2_TEXT_SIZE);

	createTextObjectFont(object);

	if (!refreshTextObjectBitmap(handle))
		return destroyTextObject(handle);

	return handle;
}

uint16 TopgunEngine_v1_2::destroyTextObject(uint16 handle) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return 0;

	uint16 index = operandValue(object->id);
	if (isRecordIndexValid(index, "destroyTextObject")) {
		if (_records[index].data)
			utilsLibrary()->tgFree(_records[index].data);
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

	object->font = Graphics::loadTTFFontFromArchive(file, size, sizeMode, 0, 0, Graphics::kTTFRenderModeMonochrome);
	if (object->font)
		object->fontOwned = true;
#endif

	if (!object->font) {
		warning("TopgunEngine_v1_2::createTextObjectFont(): the font %s is not available", file);
		object->font = FontMan.getFontByUsage(Graphics::FontManager::kBigGUIFont);
	}
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
	font->drawString(surface, string, 0, 0, width, textColor, Graphics::kTextAlignLeft, 0, false, true);

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
	const char *text = (object->text[0] != '\0') ? object->text : " ";
	uint16 color = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_COLOR);

	Graphics::ManagedSurface *surface = renderTextToBitmap(color, text, object->font);

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
	uint16 hotX = READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_X);
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	byte alignment = object->data[TG_TEXT_V1_2_DATA_ALIGNMENT];

	if (alignment == TG_TEXT_V1_2_ALIGN_CENTER)
		hotX = (uint16)(hotX + ((uint16)(width - 1) >> 1));
	else if (alignment == TG_TEXT_V1_2_ALIGN_RIGHT)
		hotX = (uint16)(hotX - (width >> 1));

	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET, hotX);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET, READ_LE_UINT16(object->data + TG_TEXT_V1_2_DATA_Y));
}

void TopgunEngine_v1_2::replaceTextBitmapInSprites(byte *oldBitmap, byte *newBitmap, const TextObject_v1_2 *object) {
	if (!oldBitmap)
		return;

	CelReference oldCel(oldBitmap, READ_LE_UINT16(oldBitmap + TG_HOST_V1_2_BITMAP_RESOURCE_ID));
	CelReference newCel(newBitmap, READ_LE_UINT16(newBitmap + TG_HOST_V1_2_BITMAP_RESOURCE_ID));

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

	WRITE_LE_UINT16(bitmap + TG_HOST_V1_2_BITMAP_RESOURCE_ID, object->id);
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
	TextObject_v1_2 *object = textObjectFromHandle(getTextObjectFromId(READ_LE_UINT16(record + 2)));
	if (!object)
		return false;

	memcpy(object->edit, record, TG_TEXT_V1_2_EDIT_RECORD_SIZE);

	if (object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] == 0)
		object->edit[TG_TEXT_V1_2_EDIT_MAX_LENGTH] = TG_TEXT_V1_2_DEFAULT_MAX_LENGTH;

	memcpy(object->undo, object->text, strlen(object->text) + 1);

	_textEditObjectId = READ_LE_UINT16(record + 2);
	return true;
}

// The argument is not used, the edit that is active is the one committed
void TopgunEngine_v1_2::commitTextEdit(uint16) {
	uint16 object = getTextObjectFromId(_textEditObjectId);

	if (object != 0) {
		if (endTextEdit(object, true))
			refreshTextObjectBitmap(object);
	}

	_textEditObjectId = 0;
}

bool TopgunEngine_v1_2::endTextEdit(uint16 handle, bool cancel) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

	if (cancel)
		memcpy(object->text, object->undo, strlen(object->undo) + 1);

	uint16 stringId = READ_LE_UINT16(object->edit + TG_TEXT_V1_2_EDIT_STRING);

	char *destination = nullptr;
	if ((stringId & TG_HOST_V1_2_STRING_ID_CONSTANT) == 0)
		destination = getStringFromId(stringId);

	if (destination)
		Common::strlcpy(destination, object->text, TG_HOST_V1_2_STRING_SLOT_SIZE);

	uint16 script = READ_LE_UINT16(object->edit + TG_TEXT_V1_2_EDIT_SCRIPT);
	if (script != 0)
		postMessage(kMsgV1_2RunScript, script, 0);

	_textEditObjectId = 0;
	return true;
}

bool TopgunEngine_v1_2::textEditBackspace(uint16 handle) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

	uint16 length = (uint16)strlen(object->text);
	if (length < 1)
		return false;

	object->text[length - 1] = '\0';
	return true;
}

bool TopgunEngine_v1_2::textEditAppendChar(uint16 handle, byte character) {
	TextObject_v1_2 *object = textObjectFromHandle(handle);
	if (!object)
		return false;

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
	if (!validateAndGetResourceByType(READ_LE_UINT16(record + 2), kResV1_2Text))
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

} // End of namespace Topgun
