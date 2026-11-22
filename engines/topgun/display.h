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

#ifndef TOPGUN_DISPLAY_H
#define TOPGUN_DISPLAY_H

#include "common/array.h"
#include "common/rect.h"
#include "common/scummsys.h"

#define TG_DISPLAY_DEFAULT_WIDTH             640
#define TG_DISPLAY_DEFAULT_HEIGHT            480
#define TG_DISPLAY_LARGE_WIDTH               800
#define TG_DISPLAY_LARGE_HEIGHT              600
#define TG_DISPLAY_NUM_PALETTE_ENTRIES       256
#define TG_DISPLAY_PALETTE_ENTRY_SIZE        4
#define TG_DISPLAY_ANIMATABLE_PALETTE_FLAG   1
#define TG_DISPLAY_NUM_STATIC_COLORS_PER_END 10

namespace Graphics {
class Screen;
}

namespace Topgun {

class Display {
public:
	Display(uint16 width, uint16 height);
	~Display();

	void setScreen(Graphics::Screen *screen) { _screen = screen; }

	uint16 width() const { return _width; }
	uint16 height() const { return _height; }

	bool hasPalette() const { return _hasPalette; }
	void setHasPalette(bool hasPalette) { _hasPalette = hasPalette; }
	void getSystemPaletteEntries(byte *destination, uint16 first, uint16 count) const;

	const Common::Point &viewportOrigin() const { return _viewportOrigin; }
	void setViewportOrigin(int16 x, int16 y) {
		_viewportOrigin.x = x;
		_viewportOrigin.y = y;
	}

	// Clip region
	int saveClipState();
	void restoreClipState(int state);
	void excludeClipRect(const Common::Rect &rect);
	void resetClipRegion() { _excludedRects.clear(); }

	// Drawing
	void putBitmapRows(int16 x, int16 y, uint16 width, uint16 height, const byte *pixels, uint16 pitch, const byte *colorTable);
	void putBitmapRowsStretched(int16 x, int16 y, uint16 destinationWidth, uint16 destinationHeight, uint16 sourceWidth, uint16 sourceHeight, const byte *pixels, uint16 pitch, const byte *colorTable);
	void clearScreen();
	void fillScreen(uint32 color);

	// Palette
	bool createLogicalPalette(const byte *entries);
	void deleteLogicalPalette();
	void animateLogicalPalette(uint16 first, uint16 count, const byte *entries);
	bool realizeLogicalPalette();
	void selectDefaultPalette();
	bool isPaletteChanged() const { return _paletteChanged; }
	void clearPaletteChanged() { _paletteChanged = false; }

private:
	typedef Common::Array<Common::Rect> RectArray;

	void getVisibleRects(const Common::Rect &area, RectArray &visible) const;

	Graphics::Screen *_screen = nullptr;
	uint16 _width;
	uint16 _height;
	bool _hasPalette = false;
	Common::Point _viewportOrigin;
	RectArray _excludedRects;
	Common::Array<RectArray> _savedClipStates;
	byte _logicalPalette[TG_DISPLAY_NUM_PALETTE_ENTRIES][TG_DISPLAY_PALETTE_ENTRY_SIZE];
	byte _screenPalette[TG_DISPLAY_NUM_PALETTE_ENTRIES][3];
	bool _logicalPaletteRealized = false;
	bool _paletteChanged = false;
};

} // End of namespace Topgun

#endif // TOPGUN_DISPLAY_H
