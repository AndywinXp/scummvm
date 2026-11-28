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
#include "common/path.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/ustr.h"
#include "common/util.h"

#include "graphics/font.h"
#include "graphics/managed_surface.h"

#include "gui/message.h"

#include "topgun/games/virgil/re.h"
#include "topgun/libraries/sprite/sprite_v1_2.h"
#include "topgun/topgun_v1_2_4.h"

namespace Topgun {

enum {
	kFunctionReInitEncyclopedia,
	kFunctionReLookup,
	kFunctionReClosestMatch,
	kFunctionReClosestMatchFree,
	kFunctionReCopy,
	kFunctionRePrint,
	kFunctionScrollTextCreate,
	kFunctionScrollTextDestroy,
	kFunctionScrollTextGetScrollState,
	kFunctionScrollTextScroll,
	kFunctionScrollTextSetText,
	kFunctionCount
};

enum {
	kReMaxArguments = 10
};

static const char *const kReDatabaseFileName = "renef431.dat";

static const ReFontDescription kReFontDescriptions[kReFontCount] = {
	{ "Arial", 24, 700, false, false },
	{ "Times New Roman", 12, 400, false, false },
	{ "Times New Roman", 12, 400, true, false },
	{ "Times New Roman", 12, 500, false, false },
	{ "Times New Roman", 12, 400, false, true },
	{ "Times New Roman", 8, 400, false, false }
};

RePlugin::RePlugin(PluginHost *host, const Common::String &fileName) : _host(host), _fileName(fileName), _articlePalette(Graphics::PALETTE_COUNT) {
}

RePlugin::~RePlugin() {
	deleteScrollText();
	delete _referenceEngine;
}

bool RePlugin::init() {
	freeCurrentState();

	if (!_resources.loadFromEXE(Common::Path(_fileName)))
		return false;

	_referenceEngine = new ReferenceEngine();
	initCurrentState();

	return true;
}

void RePlugin::free() {
	freeCurrentState();
}

int RePlugin::findFunction(const Common::String &name) const {
	if (name.equalsIgnoreCase("reInitEncyclopedia"))
		return kFunctionReInitEncyclopedia;
	if (name.equalsIgnoreCase("reLookup"))
		return kFunctionReLookup;
	if (name.equalsIgnoreCase("reClosestMatch"))
		return kFunctionReClosestMatch;
	if (name.equalsIgnoreCase("reClosestMatchFree"))
		return kFunctionReClosestMatchFree;
	if (name.equalsIgnoreCase("reCopy"))
		return kFunctionReCopy;
	if (name.equalsIgnoreCase("rePrint"))
		return kFunctionRePrint;
	if (name.equalsIgnoreCase("ScrollText_Create"))
		return kFunctionScrollTextCreate;
	if (name.equalsIgnoreCase("ScrollText_Destroy"))
		return kFunctionScrollTextDestroy;
	if (name.equalsIgnoreCase("ScrollText_GetScrollState"))
		return kFunctionScrollTextGetScrollState;
	if (name.equalsIgnoreCase("ScrollText_Scroll"))
		return kFunctionScrollTextScroll;
	if (name.equalsIgnoreCase("ScrollText_SetText"))
		return kFunctionScrollTextSetText;

	return -1;
}

uint16 RePlugin::callFunction(int function, const uint16 *arguments, uint16 count) {
	if (function < 0 || function >= kFunctionCount) {
		warning("RePlugin::callFunction(): invalid function %d", function);
		return 0;
	}

	uint16 expected = 0;
	switch (function) {
	case kFunctionReClosestMatch:
	case kFunctionReClosestMatchFree:
	case kFunctionScrollTextDestroy:
	case kFunctionScrollTextSetText:
		expected = 1;
		break;
	case kFunctionScrollTextScroll:
		expected = 2;
		break;
	case kFunctionReLookup:
	case kFunctionRePrint:
		expected = 3;
		break;
	case kFunctionScrollTextCreate:
		expected = kReMaxArguments;
		break;
	}

	if (count < expected)
		warning("RePlugin::callFunction(): function %d expects %d arguments, got %d", function, expected, count);

	uint16 values[kReMaxArguments];
	for (int i = 0; i < kReMaxArguments; i++)
		values[i] = (i < count) ? arguments[i] : 0;

	switch (function) {
	case kFunctionReInitEncyclopedia:
		return reInitEncyclopedia();
	case kFunctionReLookup:
		return reLookup(values[0], values[1], values[2]);
	case kFunctionReClosestMatch:
		return reClosestMatch(values[0]);
	case kFunctionReClosestMatchFree:
		return reClosestMatchFree(values[0]);
	case kFunctionReCopy:
		return reCopy();
	case kFunctionRePrint:
		return rePrint(values[0], values[1], values[2]);
	case kFunctionScrollTextCreate:
		return scrollTextCreate(values);
	case kFunctionScrollTextDestroy:
		return scrollTextDestroy();
	case kFunctionScrollTextGetScrollState:
		return scrollTextGetScrollState();
	case kFunctionScrollTextScroll:
		return scrollTextScroll(values[1]);
	case kFunctionScrollTextSetText:
		return scrollTextSetText();
	}

	return 0;
}

void RePlugin::freeCurrentState() {
	if (_open)
		closeEncyclopedia();

	deleteScrollText();

	for (uint16 i = 0; i < kReFontCount; i++)
		_fonts[i] = ReFont();

	_article.clear();
	_articleCapacity = 0;
	_articleLength = 0;
	_hasArticlePalette = false;

	delete _referenceEngine;
	_referenceEngine = nullptr;

	_application = nullptr;
	_database = nullptr;
	_entryList = nullptr;
	_indexList = nullptr;
}

void RePlugin::initCurrentState() {
	_open = false;
	_paletteDevice = _host->isPaletteDevice();
}

void RePlugin::deleteScrollText() {
	if (!_scrollText)
		return;

	delete _scrollText->surface;
	delete _scrollText;
	_scrollText = nullptr;
}

void RePlugin::selectFont(ReDeviceContext &context, uint16 slot) const {
	if (_host->getFont(_fonts[slot].handle))
		context.font = _fonts[slot];
}

void RePlugin::getTextMetrics(uint16 font, int16 &height, int16 &ascent) const {
	const Graphics::Font *face = _host->getFont(font);

	if (!face) {
		height = kReDefaultFontHeight;
		ascent = kReDefaultFontAscent;
		return;
	}

	height = (int16)face->getFontHeight();
	ascent = (int16)face->getFontAscent();
}

int16 RePlugin::textExtent(const ReDeviceContext &context, const byte *text, uint16 length) const {
	const Graphics::Font *font = _host->getFont(context.font.handle);
	if (!font) {
		warning("RePlugin::textExtent(): there is no font to measure the text with");
		return 0;
	}

	Common::U32String string((const char *)text, length, Common::kWindows1252);
	int16 width = 0;

	for (uint32 i = 0; i < string.size(); i++)
		width = (int16)(width + font->getCharWidth(string[i]));

	return width;
}

void RePlugin::fillSurface(Graphics::ManagedSurface *surface, int left, int top, int right, int bottom, byte color) {
	left = MAX(left, 0);
	top = MAX(top, 0);
	right = MIN<int>(right, surface->w);
	bottom = MIN<int>(bottom, surface->h);

	if (left < right && top < bottom)
		surface->fillRect(Common::Rect(left, top, right, bottom), color);
}

void RePlugin::textOut(ReDeviceContext &context, int16 x, int16 y, const byte *text, uint16 length) const {
	if (!context.surface)
		return;

	const Graphics::Font *font = _host->getFont(context.font.handle);
	if (!font) {
		warning("RePlugin::textOut(): there is no font to draw the text with");
		return;
	}

	Common::U32String string((const char *)text, length, Common::kWindows1252);
	int16 top = (int16)(y - context.originY);
	int16 width = textExtent(context, text, length);

	fillSurface(context.surface, x, top, x + width, top + font->getFontHeight(), context.backColor);

	int left = x;
	for (uint32 i = 0; i < string.size(); i++) {
		font->drawChar(context.surface, string[i], left, top, context.textColor);
		left += font->getCharWidth(string[i]);
	}

	if (context.font.underline) {
		int line = top + font->getFontAscent() + 1;
		fillSurface(context.surface, x, line, x + width, line + 1, context.textColor);
	}
}

byte RePlugin::getNearestPaletteIndex(byte red, byte green, byte blue) const {
	if (!_hasArticlePalette)
		return kReWhite;

	return _articlePalette.findBestColor(red, green, blue, Graphics::kColorDistanceEuclidean);
}

uint16 RePlugin::findNextWordBreak(const byte *buffer, uint16 start, uint16 end, bool wordBreak) const {
	uint16 position = start;
	uint16 included = 1;

	if (end <= start)
		return position;

	do {
		byte character = buffer[position];

		if ((character == ' ' && wordBreak) || character == '\t')
			return (uint16)(position + included);

		if (character == '\n' || character == '\r')
			return position;

		included = 0;
		position++;
	} while (position < end);

	return position;
}

int16 RePlugin::drawTextSegment(ReDeviceContext &context, int16 x, int16 y, const byte *buffer, uint16 start, uint16 end, uint16 flags, int16 overhang) {
	int16 left = _layoutLeft;

	if (flags != kReMeasureOnly && (flags & kReTextAlignMask) != 0) {
		int16 width = drawTextSegment(context, 0, 0, buffer, start, end, kReMeasureOnly, overhang);

		left = (int16)(_layoutWidth - width);
		if ((flags & kReTextAlignMask) == kReTextCenter)
			left >>= 1;

		left = (int16)(left + _layoutLeft);
	}

	int16 right = x;
	uint16 length = (uint16)(end - start);

	if ((flags & kReTextNoDraw) == 0)
		textOut(context, (int16)(left + right), y, buffer + start, length);

	right = (int16)(right + textExtent(context, buffer + start, length));

	if (_layoutMaxRight < right && flags != kReMeasureOnly)
		_layoutMaxRight = right;

	return right;
}

int16 RePlugin::drawWrappedText(ReDeviceContext &context, const byte *buffer, uint16 text, uint16 length, const ReRect &rect, uint16 flags, int16 offsetY, const Common::Point &limits) {
	_layoutWidth = (int16)(rect.right - rect.left);

	if (_layoutWidth == 0 || length == 0) {
		warning("RePlugin::drawWrappedText(): there is no room or no text");
		return 0;
	}

	if (length == kReWholeString)
		length = (uint16)strlen((const char *)buffer + text);

	int16 height = 0;
	int16 ascent = 0;
	getTextMetrics(context.font.handle, height, ascent);

	int16 overhang = 0;

	if ((flags & kReTextExternalLeading) != 0)
		warning("RePlugin::drawWrappedText(): the external leading of the font is not known");

	int16 lineHeight = height;

	if (_layoutLineHeight != 0)
		lineHeight = MAX(_layoutLineHeight, lineHeight);

	_layoutLeft = rect.left;

	int16 lineX = _layoutX;
	uint16 end = (uint16)(text + length);
	int16 lineY = _layoutY;
	uint16 segmentEnd = text;
	uint16 lineStart = text;
	bool firstSegment = true;
	bool newLine = false;
	bool endedWithNewLine = false;

	if ((int16)(lineY + lineHeight) > limits.y)
		return (int16)(lineY - rect.top);

	if (end > text) {
		do {
			if (newLine) {
				_layoutY = (int16)(_layoutY + lineHeight);
				lineY = _layoutY;
				lineX = 0;
				_layoutX = 0;
				newLine = false;
				endedWithNewLine = false;

				if ((flags & kReTextIgnoreBottom) == 0 && (int16)(lineHeight + lineY) > limits.y)
					return (int16)(lineY - rect.top);
			}

			uint16 next = findNextWordBreak(buffer, text, end, (flags & kReTextWordBreak) != 0);
			segmentEnd = next;

			if (firstSegment && _layoutX > 0) {
				if ((int16)(textExtent(context, buffer + text, (uint16)(next - text)) + _layoutX) > _layoutWidth) {
					const byte lineBreak[] = { '\r', '\n', 0 };
					drawTextSegment(context, _layoutX, 0, lineBreak, 0, 2, kReMeasureOnly, overhang);

					_layoutY = (int16)(_layoutY + lineHeight);
					lineY = _layoutY;
					lineX = 0;
					_layoutX = 0;
				}
			}

			_layoutX = (int16)(drawTextSegment(context, _layoutX, 0, buffer, text, next, kReMeasureOnly, overhang) - overhang);
			firstSegment = false;

			if ((flags & kReTextWordBreak) != 0 && (int16)(overhang + _layoutX) > _layoutWidth && lineStart != text) {
				if ((flags & kReTextAlignMask) == 0 && buffer[text] == ' ')
					text++;

				next = text;
				segmentEnd = text;
				newLine = true;
			} else if (end > next) {
				byte character = buffer[next];

				if (character == '\r' || character == '\n') {
					next++;

					if (next < end && buffer[next] == (character ^ 7))
						next++;

					newLine = true;
					endedWithNewLine = true;

					if ((flags & kReTextAlignMask) == 0 && end > next && buffer[next] == ' ')
						next++;
				}
			}

			if (newLine) {
				if ((int16)(lineY + lineHeight) > limits.x)
					drawTextSegment(context, lineX, (int16)(lineY + offsetY), buffer, lineStart, segmentEnd, flags, overhang);

				segmentEnd = next;
				lineStart = next;
			}

			text = next;
		} while (end > text);
	}

	if ((int16)(lineY + lineHeight) > limits.x)
		drawTextSegment(context, lineX, (int16)(lineY + offsetY), buffer, lineStart, segmentEnd, flags, overhang);

	if (endedWithNewLine) {
		_layoutX = 0;
		_layoutY = (int16)(_layoutY + lineHeight);
		return (int16)(_layoutY - rect.top);
	}

	if ((int16)(lineY + lineHeight) > limits.y)
		return (int16)(lineY - rect.top);

	return (int16)(lineHeight - rect.top + lineY);
}

void RePlugin::initTextLayout(const ReDeviceContext &context, const ReRect &rect) {
	int16 height = 0;
	int16 ascent = 0;
	getTextMetrics(context.font.handle, height, ascent);

	_layoutLineHeight = height;
	_layoutY = rect.top;
	_layoutX = 0;
	_layoutMaxRight = 0;
}

uint16 RePlugin::rePrint(uint16 resource, uint16 bitmapOnly, uint16 orientation) {
	if (_printing || resource == 0)
		return 0;

	if (bitmapOnly == 0 && (!_open || _article.empty()))
		return 0;

	warning("RePlugin::rePrint(): printing is UNIMPLEMENTED");
	return 0;
}

void RePlugin::showEncyclopediaLoadError() {
	Common::String title = _resources.loadString(kReStringTitle);
	if (title.size() >= kReTitleSize)
		title = title.substr(0, kReTitleSize - 1);

	Common::String format = _resources.loadString(kReStringLoadError);
	if (format.size() >= kReMessageSize)
		format = format.substr(0, kReMessageSize - 1);

	Common::String message = Common::String::format(format.c_str(), kReDatabaseFileName);
	Common::String text = title + "\n\n" + message;

	GUI::MessageDialog dialog(Common::U32String(text.c_str(), Common::kWindows1252));
	dialog.runModal();
}

uint16 RePlugin::createArticleFonts() {
	for (uint16 i = 0; i < kReFontCount; i++) {
		const ReFontDescription &description = kReFontDescriptions[i];

		Common::String face(description.face);
		if (description.weight >= kReBoldWeight)
			face += " Bold";
		if (description.italic)
			face += " Italic";

		int16 height = (int16)-((description.size * kReScreenDpi + kRePointsPerInch / 2) / kRePointsPerInch);

		if (i == kReFontSmall)
			deleteScrollText();

		_fonts[i].handle = _host->createFont(height, face.c_str());
		_fonts[i].underline = description.underline;

		if (_fonts[i].handle == 0)
			return 0;
	}

	return 1;
}

void RePlugin::createArticlePalette() {
	if (_hasArticlePalette)
		return;

	byte entries[Graphics::PALETTE_COUNT * 4] = {};
	_host->getPalette(entries, 0, Graphics::PALETTE_COUNT);

	for (uint16 i = 0; i < kRePaletteMaskedColors; i++) {
		entries[i * 4] = 0xFF;
		entries[i * 4 + 2] = 0xFF;
	}

	for (uint16 i = Graphics::PALETTE_COUNT - kRePaletteMaskedColors; i < Graphics::PALETTE_COUNT; i++) {
		entries[i * 4] = 0xFF;
		entries[i * 4 + 2] = 0xFF;
	}

	for (uint16 i = 0; i < Graphics::PALETTE_COUNT; i++)
		_articlePalette.set(i, entries[i * 4], entries[i * 4 + 1], entries[i * 4 + 2]);

	_hasArticlePalette = true;
}

void RePlugin::setArticleTextColor(ReDeviceContext &context, byte red, byte green, byte blue) const {
	if (_paletteDevice)
		context.textColor = getNearestPaletteIndex(red, green, blue);
	else if (!_printing)
		context.textColor = kReWhite;
	else
		context.textColor = 0;
}

void RePlugin::appendArticleText(const char *text, uint16 length) {
	if (_articleLength + length > _articleCapacity) {
		_articleCapacity += kReArticleGrowSize;
		_article.resize(_articleCapacity);
	}

	uint16 position = (uint16)_articleLength;

	for (uint16 i = 0; i < length; i++)
		_article[(uint16)(position + i)] = (byte)text[i];

	_articleLength += length;
}

uint16 RePlugin::reClosestMatch(uint16 stringId) {
	if (!_open)
		return 0;

	const ReDatabase *database = static_cast<const ReDatabase *>(_database);

	_article.clear();
	_articleCapacity = 0;

	if (_referenceEngine->reMemory(_indexList, database, kReTypeIndexList, kReOperationOpen, kReClosestMatchTerms) == kReStatusFailed)
		return 0;

	ReIndexList *list = static_cast<ReIndexList *>(_indexList);

	const char *word = _host->getString(stringId);
	if (!word) {
		warning("RePlugin::reClosestMatch(): string %d does not exist", stringId);
		word = "";
	}

	uint16 unk_24 = 0;

	list->word = word;
	list->database = database->id;
	list->unk_0D = 0;
	list->unk_19 = &unk_24;

	if (_referenceEngine->reIdxLst(list, kReOptionIndexList, kReOperationOpen) == kReStatusFailed) {
		_referenceEngine->reMemory(_indexList, database, kReTypeIndexList, kReOperationClose, 0);
		return 0;
	}

	_referenceEngine->reIdxLst(list, kReOptionIndexList, kReOperationClose);

	uint16 count = list->count;
	if (count == 0)
		return 0;

	const char *terms = list->terms;
	uint16 half = count >> 1;

	for (uint16 i = 0; i < half; i++)
		terms += strlen(terms) + 1;

	uint32 size = 0;
	const char *term = terms;

	for (uint16 i = 0; i < half; i++) {
		uint16 length = (uint16)(strlen(term) + 1);
		size += (uint16)(length + 2);
		term += length;
	}

	uint16 handle = _host->globalAlloc(size + 1);

	if (handle != 0) {
		uint32 blockSize = 0;
		byte *block = _host->globalLock(handle, blockSize);
		uint32 position = 0;
		term = terms;

		for (uint16 i = 0; i < half; i++) {
			block[position++] = '.';
			block[position++] = '0';

			uint16 length = (uint16)(strlen(term) + 1);
			memcpy(block + position, term, length);
			position += length;
			term += length;
		}

		block[position] = 0;
	}

	_referenceEngine->reMemory(_indexList, database, kReTypeIndexList, kReOperationClose, 0);

	return handle;
}

uint16 RePlugin::reClosestMatchFree(uint16 handle) {
	if (handle == 0)
		return kReNonZeroResult;

	_host->globalFree(handle);
	return 0;
}

uint16 RePlugin::reLookup(uint16 stringId, uint16 unused, uint16 hit) {
	if (!_open)
		return 0;

	const ReDatabase *database = static_cast<const ReDatabase *>(_database);

	_article.clear();
	_articleCapacity = 0;

	if (_referenceEngine->reMemory(_entryList, database, kReTypeEntryList, kReOperationOpen, 0) == kReStatusFailed)
		return 0;

	ReEntryList *list = static_cast<ReEntryList *>(_entryList);

	const char *word = _host->getString(stringId);
	if (!word) {
		warning("RePlugin::reLookup(): string %d does not exist", stringId);
		word = "";
	}

	list->word = word;
	list->unk_06 = 1;
	list->database = database->id;
	list->unk_09 = 0;

	if (_referenceEngine->reEntLst(list, kReOptionEntryList, kReOperationOpen) == kReStatusFailed) {
		_referenceEngine->reMemory(_entryList, database, kReTypeEntryList, kReOperationClose, 0);
		return 0;
	}

	uint16 hits = list->count;
	ReObject *tokenObject = nullptr;

	if (_referenceEngine->reMemory(tokenObject, database, kReTypeTokenSet, kReOperationOpen, 0) == kReStatusFailed) {
		_referenceEngine->reMemory(_entryList, database, kReTypeEntryList, kReOperationClose, 0);
		return 0;
	}

	ReTokenSet *tokenSet = static_cast<ReTokenSet *>(tokenObject);

	uint32 address = 0;
	if (hit < hits)
		address = list->entries[hit].address;
	else
		warning("RePlugin::reLookup(): hit %d does not exist", hit);

	tokenSet->entry = address;
	tokenSet->database = database->id;
	tokenSet->unk_0B = 0;
	tokenSet->unk_0E = 0;

	_article.resize(kReArticleGrowSize);
	_articleCapacity = kReArticleGrowSize;
	_articleLength = 0;

	byte lastCharacter = ' ';
	uint16 upperCase = 0;
	byte previousType = 0;

	if (_referenceEngine->reToken(tokenSet, kReOptionTokens, kReOperationOpen) != kReStatusFailed) {
		do {
			bool hidden = false;

			for (int16 i = 0; i < tokenSet->count; i++) {
				ReToken &token = tokenSet->tokens[i];

				if ((token.flags & kReTokenNewField) != 0) {
					if (hidden && token.type == 7 && token.text[0] == ',') {
						appendArticleText(",", 1);
						token.text++;
						token.length--;
					}

					hidden = false;

					switch (token.type) {
					case 0:
					case 3:
						if (previousType != 0 && previousType != 3)
							appendArticleText("\n", 1);
						break;
					case 1:
					case 4:
					case 5:
					case 9:
						appendArticleText("\n", 1);
						break;
					case 2:
					case 15:
						hidden = true;
						break;
					case 7:
					case 8:
					case 17:
						if (previousType != 7 && previousType != 8 && previousType != 17)
							appendArticleText("\n", 1);
						break;
					default:
						break;
					}

					if (!hidden) {
						if (_articleLength != 0)
							appendArticleText("", 1);

						const char marker[2] = { kReMarkerStyle, (char)token.type };
						appendArticleText(marker, 2);

						previousType = token.type;
					}

					while (token.length != 0 && token.text[0] == ' ') {
						token.text++;
						token.length--;
					}

					lastCharacter = ' ';
				} else if (token.type == 15 || token.type == 2) {
					hidden = true;
				}

				if (hidden)
					continue;

				if ((token.flags & kReTokenAttribute) != 0 && token.type != 3) {
					if (_articleLength != 0)
						appendArticleText("", 1);

					const char marker[2] = { kReMarkerAttribute, (char)token.attribute };
					appendArticleText(marker, 2);

					upperCase = token.attribute & kReAttributeUpperCase;
				}

				if (upperCase != 0) {
					for (uint16 j = 0; j < token.length; j++) {
						if (token.text[j] >= 'a' && token.text[j] <= 'z')
							token.text[j] -= 'a' - 'A';
					}
				}

				if ((token.flags & kReTokenSpace) != 0 && lastCharacter != ' ' && token.text[0] != ' ')
					appendArticleText(" ", 1);

				if (token.length != 0) {
					appendArticleText(token.text, token.length);

					lastCharacter = _article[(uint16)(_articleLength - 1)];
					if (lastCharacter == 0)
						lastCharacter = ' ';
				}
			}
		} while (_referenceEngine->reToken(tokenSet, kReOptionTokens, kReOperationNext) != kReStatusFailed);
	}

	appendArticleText("", 1);

	_referenceEngine->reToken(tokenSet, kReOptionTokens, kReOperationClose);
	_referenceEngine->reMemory(tokenObject, database, kReTypeTokenSet, kReOperationClose, 0);
	_referenceEngine->reMemory(_entryList, database, kReTypeEntryList, kReOperationClose, 0);

	return hits;
}

uint16 RePlugin::reCopy() {
	if (!_open || _article.empty())
		return 0;

	uint32 size = _articleLength + kReClipboardMargin;
	Common::String text;
	uint16 position = 0;
	uint16 end = (uint16)_articleLength;
	uint16 column = 0;

	if (end > position) {
		do {
			if (_article[position] == kReMarkerStyle)
				position += 2;

			if (_article[position] == kReMarkerAttribute)
				position += 2;

			if (_article[position] != 0) {
				do {
					if (column >= kReClipboardColumns) {
						text += "\r\n";
						column = 0;
					}

					while (_article[position] != 0 && _article[position] != ' ' && _article[position] != '\n') {
						byte character = _article[position];

						if (character == 0x93 || character == 0x94) {
							text += '"';
						} else if (character == 0x96) {
							text += '-';
						} else if (character == 0x97) {
							text += "--";
							column++;
						} else {
							text += (char)character;
						}

						column++;
						position++;
					}

					if (_article[position] != 0) {
						if (_article[position] == '\n') {
							text += "\r\n";
							column = 0;
						} else {
							text += ' ';
							column++;
						}

						position++;
					}
				} while (_article[position] != 0);
			}

			position++;
		} while (position < end);
	}

	if (text.size() >= size)
		warning("RePlugin::reCopy(): the text is longer than the block of the clipboard");

	return g_system->setTextInClipboard(Common::U32String(text.c_str(), Common::kWindows1252)) ? 1 : 0;
}

void RePlugin::reFormat(ReDeviceContext &context, ReRect &rect, uint16 flags, const Common::Point &limits) {
	if (_article.empty())
		return;

	const byte *buffer = _article.data();
	uint16 end = (uint16)_articleLength;
	ReRect textRect = rect;
	textRect.bottom = kReUnboundedBottom;
	int16 drawnHeight = 0;

	int16 height = 0;
	int16 smallAscent = 0;
	selectFont(context, kReFontSmall);
	getTextMetrics(context.font.handle, height, smallAscent);

	ReFont previousFont = context.font;
	int16 ascent = 0;
	selectFont(context, kReFontText);
	getTextMetrics(context.font.handle, height, ascent);

	int16 raise = (int16)(ascent - smallAscent);

	setArticleTextColor(context, 0, 0, 0);
	initTextLayout(context, rect);

	if (end > 0) {
		uint16 text = 0;

		do {
			textRect.bottom = kReUnboundedBottom;
			int16 offsetY = 0;

			if (buffer[text] == kReMarkerStyle) {
				text++;
				byte style = buffer[text];
				text++;

				switch (style) {
				case 0:
				case 3:
					selectFont(context, kReFontHeading);
					setArticleTextColor(context, 0, 0, 0);
					break;
				case 1:
				case 2:
				case 8:
					selectFont(context, kReFontItalic);
					setArticleTextColor(context, 0, 0, 0xFF);
					break;
				case 4:
					selectFont(context, kReFontMedium);
					setArticleTextColor(context, 0, 0xFF, 0);
					break;
				case 5:
					selectFont(context, kReFontMedium);
					setArticleTextColor(context, 0, 0, 0);
					break;
				case 7:
					selectFont(context, kReFontText);
					setArticleTextColor(context, 0, 0, 0);
					break;
				case 9:
					selectFont(context, kReFontItalic);
					setArticleTextColor(context, 0, 0, 0);
					break;
				default:
					break;
				}
			}

			if (buffer[text] == kReMarkerAttribute) {
				text++;
				byte attribute = buffer[text];

				if ((attribute & kReAttributeMedium) != 0) {
					selectFont(context, kReFontMedium);
				} else if ((attribute & kReAttributeItalic) != 0) {
					selectFont(context, kReFontItalic);
				} else if ((attribute & kReAttributeUnderline) != 0) {
					selectFont(context, kReFontUnderline);
				} else if ((attribute & kReAttributeSmall) != 0) {
					selectFont(context, kReFontSmall);
				} else if ((attribute & (kReAttributeRaised | kReAttributeUpperCase)) != 0) {
					selectFont(context, kReFontSmall);
					offsetY = raise;
				} else {
					selectFont(context, kReFontText);
				}

				text++;
			}

			drawnHeight = drawWrappedText(context, buffer, text, kReWholeString, textRect, flags | kReTextFormat, offsetY, limits);

			text = (uint16)(text + strlen((const char *)buffer + text) + 1);
		} while (text < end);
	}

	rect.bottom = drawnHeight;
	context.font = previousFont;
}

uint16 RePlugin::reInitEncyclopedia() {
	return openEncyclopedia();
}

void RePlugin::closeEncyclopedia() {
	_referenceEngine->reDb(static_cast<ReDatabase *>(_database), kReOptionDefault, kReOperationClose);
	_referenceEngine->reMemory(_database, nullptr, kReTypeDatabase, kReOperationClose, 0);
	_referenceEngine->reApp(static_cast<ReApplication *>(_application), kReOptionDefault, kReOperationClose);
	_referenceEngine->reMemory(_application, nullptr, kReTypeApplication, kReOperationClose, 0);

	_article.clear();

	for (uint16 i = 0; i < kReFontCount; i++) {
		if (_fonts[i].handle != 0)
			_host->deleteFont(_fonts[i].handle);
	}

	_hasArticlePalette = false;
	_open = false;
}

uint16 RePlugin::openEncyclopedia() {
	_article.clear();

	if (_referenceEngine->reMemory(_application, nullptr, kReTypeApplication, kReOperationOpen, 0) == kReStatusFailed) {
		showEncyclopediaLoadError();
		return 0;
	}

	ReApplication *application = static_cast<ReApplication *>(_application);

	if (_referenceEngine->reApp(application, kReOptionDefault, kReOperationOpen) == kReStatusFailed) {
		showEncyclopediaLoadError();
		return 0;
	}

	if (_referenceEngine->reMemory(_database, application, kReTypeDatabase, kReOperationOpen, 0) == kReStatusFailed) {
		showEncyclopediaLoadError();
		return 0;
	}

	ReDatabase *database = static_cast<ReDatabase *>(_database);
	database->unk_04 = 0x80;
	database->unk_06 = 0;

	if (_referenceEngine->reDb(database, kReOptionDefault, kReOperationOpen) == kReStatusFailed) {
		showEncyclopediaLoadError();
		return 0;
	}

	if (!createArticleFonts()) {
		showEncyclopediaLoadError();
		return 0;
	}

	_hasArticlePalette = false;
	createArticlePalette();

	_open = true;
	return 1;
}

void RePlugin::adjustBitmapHotSpot(const ReScrollText *scrollText, byte *bitmap) const {
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	uint16 hotX = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET);
	uint16 hotY = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET);

	if ((scrollText->alignment & kReAlignRight) != 0)
		hotX = (uint16)(hotX - (width >> 1));
	else if ((scrollText->alignment & kReAlignCenter) == 0)
		hotX = (uint16)(hotX + ((uint16)(width - 1) >> 1));

	if ((scrollText->alignment & kReAlignBottom) != 0)
		hotY = (uint16)(hotY - (height >> 1));
	else if ((scrollText->alignment & kReAlignVerticalCenter) == 0)
		hotY = (uint16)(hotY + ((uint16)(height - 1) >> 1));

	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET, hotX);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET, hotY);
}

Graphics::ManagedSurface *RePlugin::createScrollTextBitmap(const ReScrollText *scrollText) const {
	int16 width = (int16)(scrollText->width + 1);
	int16 height = (int16)(scrollText->height + 1);

	if (width <= 0 || height <= 0) {
		warning("RePlugin::createScrollTextBitmap(): invalid size %dx%d", width, height);
		return nullptr;
	}

	return new Graphics::ManagedSurface(width, height, Graphics::PixelFormat::createFormatCLUT8());
}

Graphics::ManagedSurface *RePlugin::drawScrollTextBitmap(ReDeviceContext &context, ReScrollText *scrollText) {
	if (!scrollText->surface)
		return nullptr;

	ReRect rect;
	rect.right = (int16)(scrollText->width + 1);
	rect.bottom = (int16)(scrollText->height + 1);

	if (scrollText->total == 0) {
		Common::Point limits(0, kReUnboundedBottom);
		reFormat(context, rect, kReTextNoDraw, limits);

		if (scrollText->lineHeight != 0) {
			int16 lines = (int16)(rect.bottom / scrollText->lineHeight);
			if (lines < 0)
				lines = 0;

			scrollText->total = (int16)(lines + 1);
		} else {
			scrollText->total = 1;
		}

		rect.right = (int16)(scrollText->width + 1);
		rect.bottom = (int16)(scrollText->height + 1);
	}

	context.surface = scrollText->surface;

	fillSurface(context.surface, rect.left, rect.top, rect.right, rect.bottom, 0);
	context.backColor = 0;
	context.originY = (int16)(scrollText->lineHeight * scrollText->top);

	Common::Point limits(context.originY, (int16)(scrollText->lineHeight * scrollText->visible + context.originY));
	reFormat(context, rect, 0, limits);

	context.surface = nullptr;

	return scrollText->surface;
}

byte *RePlugin::renderScrollTextDib(ReScrollText *scrollText) {
	ReDeviceContext context;
	Graphics::ManagedSurface *surface = drawScrollTextBitmap(context, scrollText);

	byte *bitmap = nullptr;
	if (surface)
		bitmap = _host->deviceBitmapToDibData(surface, false, 0);

	if (bitmap && !_paletteDevice) {
		byte table[256] = {};
		table[kReWhite] = getNearestPaletteIndex(0, 0, 0);

		uint32 size = (uint32)READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
		_host->mapBytes(bitmap + READ_LE_UINT16(bitmap), size, table);
	}

	return bitmap;
}

void RePlugin::replaceScrollTextBitmap(const ReScrollText *scrollText, byte *oldBitmap, byte *newBitmap) {
	if (!oldBitmap || scrollText->sprite == 0)
		return;

	_host->replaceSpriteBitmap(scrollText->spriteHandle, oldBitmap, newBitmap);
	_host->tgFree(oldBitmap);
}

uint16 RePlugin::updateScrollText(ReScrollText *scrollText) {
	byte *bitmap = renderScrollTextDib(scrollText);
	if (!bitmap)
		return 0;

	WRITE_LE_UINT16(bitmap + TG_HOST_V1_2_4_BITMAP_RESOURCE_ID, scrollText->bitmap);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET, scrollText->x);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET, scrollText->y);
	adjustBitmapHotSpot(scrollText, bitmap);

	ResourceRecord *record = _host->getResource(scrollText->bitmap);
	if (!record) {
		warning("RePlugin::updateScrollText(): resource %d does not exist", scrollText->bitmap);
		_host->tgFree(bitmap);
		return 0;
	}

	byte *oldBitmap = record->data;
	record->data = bitmap;

	replaceScrollTextBitmap(scrollText, oldBitmap, bitmap);

	return 1;
}

uint16 RePlugin::scrollTextCreate(const uint16 *arguments) {
	deleteScrollText();
	_scrollText = new ReScrollText();
	_fonts[kReFontSmall] = ReFont();

	ReScrollText *scrollText = _scrollText;
	scrollText->sprite = arguments[0];

	const ResourceRecord *record = _host->getResource(scrollText->sprite);
	if (record)
		scrollText->spriteHandle = record->handle;
	else
		warning("RePlugin::scrollTextCreate(): resource %d does not exist", scrollText->sprite);

	scrollText->bitmap = arguments[1];
	scrollText->x = arguments[2];
	scrollText->y = arguments[3];
	scrollText->width = arguments[4];
	scrollText->height = arguments[5];

	uint16 flags = arguments[9];

	if ((flags & kReScrollTextRight) != 0)
		scrollText->alignment = kReAlignRight;
	else if ((flags & kReScrollTextCenter) != 0)
		scrollText->alignment = kReAlignCenter;

	if (_fonts[kReFontHeading].handle != 0) {
		int16 height = 0;
		int16 ascent = 0;
		getTextMetrics(_fonts[kReFontHeading].handle, height, ascent);

		scrollText->lineHeight = height;

		if (height == 0) {
			warning("RePlugin::scrollTextCreate(): the font has no height");
		} else {
			scrollText->visible = (int16)(scrollText->height / (uint16)height);

			if ((flags & kReScrollTextKeepHeight) == 0)
				scrollText->height = (uint16)((uint16)height * (uint16)scrollText->visible);
		}
	}

	scrollText->surface = createScrollTextBitmap(scrollText);

	if (!updateScrollText(scrollText)) {
		scrollTextDestroy();
		return 0;
	}

	return 1;
}

uint16 RePlugin::scrollTextDestroy() {
	if (_scrollText) {
		deleteScrollText();
		return 0;
	}

	if (_fonts[kReFontSmall].handle != 0)
		warning("RePlugin::scrollTextDestroy(): there is no scroll text");

	return kReNonZeroResult;
}

uint16 RePlugin::scrollTextGetScrollState() const {
	if (!_scrollText) {
		warning("RePlugin::scrollTextGetScrollState(): there is no scroll text");
		return 0;
	}

	uint16 state = 0;

	if (_scrollText->top > 0)
		state = 1;

	if ((int16)(_scrollText->total - _scrollText->visible) > _scrollText->top)
		state |= 2;

	return state;
}

uint16 RePlugin::scrollTextScroll(uint16 command) {
	ReScrollText *scrollText = _scrollText;
	if (!scrollText) {
		warning("RePlugin::scrollTextScroll(): there is no scroll text");
		return 0;
	}

	switch (command) {
	case kReScrollUp:
		if (scrollText->top > 0)
			scrollText->top--;
		break;
	case kReScrollDown:
		if ((int16)(scrollText->total - scrollText->visible) > scrollText->top)
			scrollText->top++;
		break;
	case kReScrollPageUp:
		scrollText->top = (int16)(scrollText->top - scrollText->visible);
		if (scrollText->top < 0)
			scrollText->top = 0;
		break;
	case kReScrollHome:
		scrollText->top = 0;
		break;
	case kReScrollPageDown:
		scrollText->top = (int16)(scrollText->top + scrollText->visible);
		if ((int16)(scrollText->total - scrollText->visible) < scrollText->top)
			scrollText->top = MAX<int16>((int16)(scrollText->total - scrollText->visible), 0);
		break;
	case kReScrollEnd:
		scrollText->top = MAX<int16>((int16)(scrollText->total - scrollText->visible), 0);
		break;
	default:
		break;
	}

	updateScrollText(scrollText);

	return (uint16)scrollText->top;
}

uint16 RePlugin::scrollTextSetText() {
	if (!_scrollText) {
		warning("RePlugin::scrollTextSetText(): there is no scroll text");
		return 0;
	}

	_host->loadResource(_scrollText->bitmap);
	_scrollText->top = 0;
	_scrollText->total = 0;

	return updateScrollText(_scrollText);
}

} // End of namespace Topgun
