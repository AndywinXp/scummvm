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

#ifndef TOPGUN_GAMES_VIRGIL_RE_H
#define TOPGUN_GAMES_VIRGIL_RE_H

#include "common/array.h"
#include "common/formats/winexe_ne.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"

#include "graphics/palette.h"

#include "topgun/games/virgil/refengine.h"
#include "topgun/plugin.h"

namespace Topgun {

enum {
	kReFontCount = 6,
	kReArticleGrowSize = 0x4000,
	kReClipboardMargin = 0x400,
	kReClipboardColumns = 72,
	kReClosestMatchTerms = 50,
	kReUnboundedBottom = 0x7D00,
	kReWholeString = 0xFFFF,
	kReMeasureOnly = 0xFFFF,
	kReBoldWeight = 700,
	kReScreenDpi = 96,
	kRePointsPerInch = 72,
	kReDefaultFontHeight = 16,
	kReDefaultFontAscent = 13,
	kRePaletteMaskedColors = 10,
	kReWhite = 0xFF,
	kReStringTitle = 0x1000,
	kReStringLoadError = 0x1001,
	kReTitleSize = 60,
	kReMessageSize = 256,
	kReNonZeroResult = 1
};

enum ReFontSlot {
	kReFontHeading = 0,
	kReFontText = 1,
	kReFontItalic = 2,
	kReFontMedium = 3,
	kReFontUnderline = 4,
	kReFontSmall = 5
};

enum ReTextFlags {
	kReTextCenter = 0x0001,
	kReTextAlignMask = 0x0003,
	kReTextIgnoreBottom = 0x0004,
	kReTextWordBreak = 0x0010,
	kReTextFormat = 0x0110,
	kReTextExternalLeading = 0x0200,
	kReTextNoDraw = 0x0400
};

enum ReMarker {
	kReMarkerStyle = 1,
	kReMarkerAttribute = 2
};

enum ReTokenFlags {
	kReTokenSpace = 0x02,
	kReTokenNewField = 0x04,
	kReTokenAttribute = 0x08
};

enum ReAttribute {
	kReAttributeMedium = 0x01,
	kReAttributeItalic = 0x02,
	kReAttributeUnderline = 0x04,
	kReAttributeSmall = 0x10,
	kReAttributeRaised = 0x20,
	kReAttributeUpperCase = 0x40
};

enum ReScrollTextFlags {
	kReScrollTextRight = 1,
	kReScrollTextCenter = 2,
	kReScrollTextKeepHeight = 4
};

enum ReAlignment {
	kReAlignCenter = 1,
	kReAlignRight = 2,
	kReAlignVerticalCenter = 4,
	kReAlignBottom = 8
};

enum ReScrollCommand {
	kReScrollUp = 0,
	kReScrollDown = 1,
	kReScrollPageUp = 2,
	kReScrollPageDown = 3,
	kReScrollHome = 4,
	kReScrollEnd = 5
};

struct ReFontDescription {
	const char *face;
	int16 size;
	uint16 weight;
	bool italic;
	bool underline;
};

struct ReFont {
	uint16 handle = 0;
	bool underline = false;
};

struct ReRect {
	int16 left = 0;
	int16 top = 0;
	int16 right = 0;
	int16 bottom = 0;
};

struct ReDeviceContext {
	Graphics::ManagedSurface *surface = nullptr;
	ReFont font;
	byte textColor = 0;
	byte backColor = kReWhite;
	int16 originY = 0;
};

struct ReScrollText {
	uint16 sprite = 0;
	uint16 spriteHandle = 0;
	uint16 bitmap = 0;
	uint16 x = 0;
	uint16 y = 0;
	uint16 width = 0;
	uint16 height = 0;
	Graphics::ManagedSurface *surface = nullptr;
	uint16 alignment = 0;
	int16 lineHeight = 0;
	int16 top = 0;
	int16 total = 0;
	int16 visible = 0;
};

class RePlugin : public Plugin {
public:
	RePlugin(PluginHost *host, const Common::String &fileName);
	~RePlugin() override;

	bool init() override;
	void free() override;
	int findFunction(const Common::String &name) const override;
	uint16 callFunction(int function, const uint16 *arguments, uint16 count) override;

private:
	PluginHost *_host;
	Common::String _fileName;
	Common::NEResources _resources;
	ReferenceEngine *_referenceEngine = nullptr;

	bool _open = false;
	bool _paletteDevice = false;
	bool _printing = false;
	ReObject *_application = nullptr;
	ReObject *_database = nullptr;
	ReObject *_entryList = nullptr;
	ReObject *_indexList = nullptr;
	uint32 _articleCapacity = 0;
	Common::Array<byte> _article;
	uint32 _articleLength = 0;

	ReFont _fonts[kReFontCount];
	ReScrollText *_scrollText = nullptr;

	Graphics::Palette _articlePalette;
	bool _hasArticlePalette = false;

	int16 _layoutLeft = 0;
	int16 _layoutWidth = 0;
	int16 _layoutMaxRight = 0;
	int16 _layoutLineHeight = 0;
	int16 _layoutY = 0;
	int16 _layoutX = 0;

	// Plug-in state
	void freeCurrentState();
	void initCurrentState();
	void deleteScrollText();

	// Drawing
	static void fillSurface(Graphics::ManagedSurface *surface, int left, int top, int right, int bottom, byte color);
	void selectFont(ReDeviceContext &context, uint16 slot) const;
	void getTextMetrics(uint16 font, int16 &height, int16 &ascent) const;
	int16 textExtent(const ReDeviceContext &context, const byte *text, uint16 length) const;
	void textOut(ReDeviceContext &context, int16 x, int16 y, const byte *text, uint16 length) const;
	byte getNearestPaletteIndex(byte red, byte green, byte blue) const;

	// Article layout
	uint16 findNextWordBreak(const byte *buffer, uint16 start, uint16 end, bool wordBreak) const;
	int16 drawTextSegment(ReDeviceContext &context, int16 x, int16 y, const byte *buffer, uint16 start, uint16 end, uint16 flags, int16 overhang);
	int16 drawWrappedText(ReDeviceContext &context, const byte *buffer, uint16 text, uint16 length, const ReRect &rect, uint16 flags, int16 offsetY, const Common::Point &limits);
	void initTextLayout(const ReDeviceContext &context, const ReRect &rect);
	uint16 createArticleFonts();
	void createArticlePalette();
	void setArticleTextColor(ReDeviceContext &context, byte red, byte green, byte blue) const;
	void appendArticleText(const char *text, uint16 length);
	void reFormat(ReDeviceContext &context, ReRect &rect, uint16 flags, const Common::Point &limits);

	// Encyclopedia
	void showEncyclopediaLoadError();
	void closeEncyclopedia();
	uint16 openEncyclopedia();
	uint16 reInitEncyclopedia();
	uint16 reClosestMatch(uint16 stringId);
	uint16 reClosestMatchFree(uint16 handle);
	uint16 reLookup(uint16 stringId, uint16 unused, uint16 hit);
	uint16 reCopy();
	uint16 rePrint(uint16 resource, uint16 bitmapOnly, uint16 orientation);

	// Scroll text
	void adjustBitmapHotSpot(const ReScrollText *scrollText, byte *bitmap) const;
	Graphics::ManagedSurface *createScrollTextBitmap(const ReScrollText *scrollText) const;
	Graphics::ManagedSurface *drawScrollTextBitmap(ReDeviceContext &context, ReScrollText *scrollText);
	byte *renderScrollTextDib(ReScrollText *scrollText);
	void replaceScrollTextBitmap(const ReScrollText *scrollText, byte *oldBitmap, byte *newBitmap);
	uint16 updateScrollText(ReScrollText *scrollText);
	uint16 scrollTextCreate(const uint16 *arguments);
	uint16 scrollTextDestroy();
	uint16 scrollTextGetScrollState() const;
	uint16 scrollTextScroll(uint16 command);
	uint16 scrollTextSetText();
};

} // End of namespace Topgun

#endif // TOPGUN_GAMES_VIRGIL_RE_H
