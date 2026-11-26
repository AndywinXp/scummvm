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

#include "common/textconsole.h"
#include "common/util.h"
#include "graphics/screen.h"

#include "topgun/display.h"

namespace Topgun {

static const byte kStaticColors[2 * TG_DISPLAY_NUM_STATIC_COLORS_PER_END][3] = {
	{   0,   0,   0 }, { 128,   0,   0 }, {   0, 128,   0 }, { 128, 128,   0 }, {   0,   0, 128 },
	{ 128,   0, 128 }, {   0, 128, 128 }, { 192, 192, 192 }, { 192, 220, 192 }, { 166, 202, 240 },
	{ 255, 251, 240 }, { 160, 160, 164 }, { 128, 128, 128 }, { 255,   0,   0 }, {   0, 255,   0 },
	{ 255, 255,   0 }, {   0,   0, 255 }, { 255,   0, 255 }, {   0, 255, 255 }, { 255, 255, 255 }
};

Display::Display(uint16 width, uint16 height) : _width(width), _height(height) {
	memset(_logicalPalette, 0, sizeof(_logicalPalette));
	memset(_screenPalette, 0, sizeof(_screenPalette));
}

Display::~Display() {
}

void Display::getSystemPaletteEntries(byte *destination, uint16 first, uint16 count) const {
	for (uint16 i = 0; i < count; i++) {

		uint32 index = (uint32)first + i;
		if (index >= TG_DISPLAY_NUM_PALETTE_ENTRIES)
			break;

		byte *entry = destination + i * TG_DISPLAY_PALETTE_ENTRY_SIZE;
		entry[0] = 0;
		entry[1] = 0;
		entry[2] = 0;
		entry[3] = 0;

		int staticIndex = -1;
		if (index < TG_DISPLAY_NUM_STATIC_COLORS_PER_END) {
			staticIndex = index;
		} else if (index >= TG_DISPLAY_NUM_PALETTE_ENTRIES - TG_DISPLAY_NUM_STATIC_COLORS_PER_END) {
			staticIndex = TG_DISPLAY_NUM_STATIC_COLORS_PER_END + (index - (TG_DISPLAY_NUM_PALETTE_ENTRIES - TG_DISPLAY_NUM_STATIC_COLORS_PER_END));
		}

		if (staticIndex >= 0) {
			entry[0] = kStaticColors[staticIndex][0];
			entry[1] = kStaticColors[staticIndex][1];
			entry[2] = kStaticColors[staticIndex][2];
		}
	}
}

int Display::saveClipState() {
	_savedClipStates.push_back(_excludedRects);
	return _savedClipStates.size();
}

void Display::restoreClipState(int state) {
	if (state < 1 || state > (int)_savedClipStates.size()) {
		warning("Display::restoreClipState(): invalid clip state %d", state);
		return;
	}

	_excludedRects = _savedClipStates[state - 1];
	_savedClipStates.resize(state - 1);
}

void Display::excludeClipRect(const Common::Rect &rect) {
	if (rect.isEmpty())
		return;

	Common::Rect deviceRect = rect;
	deviceRect.translate(_viewportOrigin.x, _viewportOrigin.y);
	_excludedRects.push_back(deviceRect);
}

void Display::getVisibleRects(const Common::Rect &area, RectArray &visible) const {
	visible.clear();
	visible.push_back(area);

	for (uint i = 0; i < _excludedRects.size(); i++) {
		const Common::Rect &excluded = _excludedRects[i];
		RectArray remaining;

		for (uint j = 0; j < visible.size(); j++) {
			const Common::Rect &piece = visible[j];
			if (!piece.intersects(excluded)) {
				remaining.push_back(piece);
				continue;
			}

			Common::Rect cut = piece.findIntersectingRect(excluded);
			if (cut.top > piece.top)
				remaining.push_back(Common::Rect(piece.left, piece.top, piece.right, cut.top));

			if (cut.bottom < piece.bottom)
				remaining.push_back(Common::Rect(piece.left, cut.bottom, piece.right, piece.bottom));

			if (cut.left > piece.left)
				remaining.push_back(Common::Rect(piece.left, cut.top, cut.left, cut.bottom));

			if (cut.right < piece.right)
				remaining.push_back(Common::Rect(cut.right, cut.top, piece.right, cut.bottom));
		}

		visible = remaining;
	}
}

void Display::putBitmapRows(int16 x, int16 y, uint16 width, uint16 height, const byte *pixels, uint16 pitch, const byte *colorTable) {
	int originX = x + _viewportOrigin.x;
	int originY = y + _viewportOrigin.y;
	int left = MAX(originX, 0);
	int top = MAX(originY, 0);
	int right = MIN(originX + (int)width, (int)_width);
	int bottom = MIN(originY + (int)height, (int)_height);

	if (left >= right || top >= bottom)
		return;

	RectArray visible;
	getVisibleRects(Common::Rect(left, top, right, bottom), visible);

	bool singleByte = (_screen->format.bytesPerPixel == 1);
	uint32 colors[TG_DISPLAY_NUM_PALETTE_ENTRIES];

	if (!singleByte) {
		for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++) {
			const byte *entry = colorTable + i * TG_DISPLAY_PALETTE_ENTRY_SIZE;
			colors[i] = _screen->format.RGBToColor(entry[2], entry[1], entry[0]);
		}
	}

	for (uint i = 0; i < visible.size(); i++) {
		const Common::Rect &piece = visible[i];

		for (int row = piece.top; row < piece.bottom; row++) {
			const byte *source = pixels + (uint32)(height - 1 - (row - originY)) * pitch + (piece.left - originX);

			if (singleByte) {
				memcpy(_screen->getBasePtr(piece.left, row), source, piece.width());
			} else {
				uint32 *destination = (uint32 *)_screen->getBasePtr(piece.left, row);
				for (int column = 0; column < piece.width(); column++)
					destination[column] = colors[source[column]];
			}
		}

		_screen->addDirtyRect(piece);
	}
}

void Display::putBitmapRowsStretched(int16 x, int16 y, uint16 destinationWidth, uint16 destinationHeight, uint16 sourceWidth, uint16 sourceHeight, const byte *pixels, uint16 pitch, const byte *colorTable) {
	if (destinationWidth == 0 || destinationHeight == 0 || sourceWidth == 0 || sourceHeight == 0)
		return;

	int originX = x + _viewportOrigin.x;
	int originY = y + _viewportOrigin.y;
	int left = MAX(originX, 0);
	int top = MAX(originY, 0);
	int right = MIN(originX + (int)destinationWidth, (int)_width);
	int bottom = MIN(originY + (int)destinationHeight, (int)_height);

	if (left >= right || top >= bottom)
		return;

	RectArray visible;
	getVisibleRects(Common::Rect(left, top, right, bottom), visible);

	bool singleByte = (_screen->format.bytesPerPixel == 1);
	uint32 colors[TG_DISPLAY_NUM_PALETTE_ENTRIES];

	if (!singleByte) {
		for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++) {
			const byte *entry = colorTable + i * TG_DISPLAY_PALETTE_ENTRY_SIZE;
			colors[i] = _screen->format.RGBToColor(entry[2], entry[1], entry[0]);
		}
	}

	for (uint i = 0; i < visible.size(); i++) {
		const Common::Rect &piece = visible[i];

		for (int row = piece.top; row < piece.bottom; row++) {
			uint32 rowFromTop = (uint32)(row - originY) * sourceHeight / destinationHeight;
			const byte *source = pixels + (uint32)(sourceHeight - 1 - rowFromTop) * pitch;

			if (singleByte) {
				byte *destination = (byte *)_screen->getBasePtr(piece.left, row);
				for (int column = 0; column < piece.width(); column++)
					destination[column] = source[(uint32)(piece.left + column - originX) * sourceWidth / destinationWidth];
			} else {
				uint32 *destination = (uint32 *)_screen->getBasePtr(piece.left, row);
				for (int column = 0; column < piece.width(); column++)
					destination[column] = colors[source[(uint32)(piece.left + column - originX) * sourceWidth / destinationWidth]];
			}
		}

		_screen->addDirtyRect(piece);
	}
}

void Display::scrollDevice(const Common::Rect &area, int16 deltaX, int16 deltaY) {
	int left = MAX(area.left + _viewportOrigin.x, 0);
	int top = MAX(area.top + _viewportOrigin.y, 0);
	int right = MIN(area.right + _viewportOrigin.x, (int)_width);
	int bottom = MIN(area.bottom + _viewportOrigin.y, (int)_height);
	if (left >= right || top >= bottom || (deltaX == 0 && deltaY == 0))
		return;

	int sourceLeft = MAX(left, left - deltaX);
	int sourceRight = MIN(right, right - deltaX);
	int sourceTop = MAX(top, top - deltaY);
	int sourceBottom = MIN(bottom, bottom - deltaY);
	if (sourceLeft >= sourceRight || sourceTop >= sourceBottom)
		return;

	int rowBytes = (sourceRight - sourceLeft) * _screen->format.bytesPerPixel;
	int rows = sourceBottom - sourceTop;

	for (int i = 0; i < rows; i++) {
		int row = (deltaY > 0) ? (sourceBottom - 1 - i) : (sourceTop + i);
		memmove(_screen->getBasePtr(sourceLeft + deltaX, row + deltaY), _screen->getBasePtr(sourceLeft, row), rowBytes);
	}

	_screen->addDirtyRect(Common::Rect(sourceLeft + deltaX, sourceTop + deltaY, sourceRight + deltaX, sourceBottom + deltaY));
}

void Display::clearScreen() {
	RectArray visible;
	getVisibleRects(Common::Rect(0, 0, _width, _height), visible);

	uint32 black = 0;
	if (_screen->format.bytesPerPixel != 1)
		black = _screen->format.RGBToColor(0, 0, 0);

	for (uint i = 0; i < visible.size(); i++)
		_screen->fillRect(visible[i], black);
}

void Display::fillScreen(uint32 color) {
	RectArray visible;
	getVisibleRects(Common::Rect(0, 0, _width, _height), visible);

	// The color is a palette index on a palette screen and a 0x00BBGGRR value otherwise
	uint32 pixel = color;
	if (_screen->format.bytesPerPixel != 1)
		pixel = _screen->format.RGBToColor(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);

	for (uint i = 0; i < visible.size(); i++)
		_screen->fillRect(visible[i], pixel);
}

bool Display::createLogicalPalette(const byte *entries) {
	memcpy(_logicalPalette, entries, sizeof(_logicalPalette));
	return true;
}

void Display::deleteLogicalPalette() {
	_logicalPaletteRealized = false;
}

void Display::animateLogicalPalette(uint16 first, uint16 count, const byte *entries) {
	bool screenChanged = false;

	for (uint16 i = 0; i < count; i++) {
		byte *logical = _logicalPalette[first + i];
		if (!(logical[3] & TG_DISPLAY_ANIMATABLE_PALETTE_FLAG))
			continue;

		memcpy(logical, entries + i * TG_DISPLAY_PALETTE_ENTRY_SIZE, TG_DISPLAY_PALETTE_ENTRY_SIZE);

		if (_logicalPaletteRealized) {
			memcpy(_screenPalette[first + i], logical, 3);
			screenChanged = true;
		}
	}

	if (screenChanged) {
		_screen->setPalette(&_screenPalette[first][0], first, count);
		_paletteChanged = true;
	}
}

byte Display::getDeviceColorIndex(byte logicalIndex, const byte *color) const {
	byte systemColors[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];
	getSystemPaletteEntries(systemColors, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);

	const int upperFirst = TG_DISPLAY_NUM_PALETTE_ENTRIES - TG_DISPLAY_NUM_STATIC_COLORS_PER_END;
	for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++) {
		if (i >= TG_DISPLAY_NUM_STATIC_COLORS_PER_END && i < upperFirst)
			continue;

		if (memcmp(systemColors + i * TG_DISPLAY_PALETTE_ENTRY_SIZE, color, 3) == 0)
			return (byte)i;
	}

	return logicalIndex;
}

bool Display::realizeLogicalPalette() {
	byte systemColors[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];
	getSystemPaletteEntries(systemColors, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);

	byte realized[TG_DISPLAY_NUM_PALETTE_ENTRIES][3];
	for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++) {
		const byte *source = systemColors + i * TG_DISPLAY_PALETTE_ENTRY_SIZE;
		if (i >= TG_DISPLAY_NUM_STATIC_COLORS_PER_END && i < TG_DISPLAY_NUM_PALETTE_ENTRIES - TG_DISPLAY_NUM_STATIC_COLORS_PER_END)
			source = _logicalPalette[i];

		memcpy(realized[i], source, 3);
	}

	bool changed = (memcmp(realized, _screenPalette, sizeof(realized)) != 0);
	memcpy(_screenPalette, realized, sizeof(realized));
	_screen->setPalette(&_screenPalette[0][0], 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);
	_logicalPaletteRealized = true;
	_paletteChanged = true;
	return changed;
}

void Display::selectDefaultPalette() {
	byte systemColors[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];
	getSystemPaletteEntries(systemColors, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);

	const int upperFirst = TG_DISPLAY_NUM_PALETTE_ENTRIES - TG_DISPLAY_NUM_STATIC_COLORS_PER_END;
	for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++) {
		if (i < TG_DISPLAY_NUM_STATIC_COLORS_PER_END || i >= upperFirst)
			memcpy(_screenPalette[i], systemColors + i * TG_DISPLAY_PALETTE_ENTRY_SIZE, 3);
	}

	_screen->setPalette(&_screenPalette[0][0], 0, TG_DISPLAY_NUM_STATIC_COLORS_PER_END);
	_screen->setPalette(&_screenPalette[upperFirst][0], upperFirst, TG_DISPLAY_NUM_STATIC_COLORS_PER_END);
	_logicalPaletteRealized = false;
	_paletteChanged = true;
}

} // End of namespace Topgun
