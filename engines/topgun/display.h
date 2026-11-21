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

#include "common/rect.h"
#include "common/scummsys.h"

namespace Topgun {

class Display {
public:
	enum {
		kDefaultWidth = 640,
		kDefaultHeight = 480,
		kLargeWidth = 800,
		kLargeHeight = 600
	};

	Display(uint16 width, uint16 height);
	~Display();

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

	bool isClipExcluded() const { return _clipExcluded; }
	const Common::Rect &excludedClipRect() const { return _excludedClipRect; }
	void excludeClipRect(const Common::Rect &rect) {
		_excludedClipRect = rect;
		_clipExcluded = true;
	}
	void resetClipRegion() { _clipExcluded = false; }

private:
	uint16 _width;
	uint16 _height;
	bool _hasPalette = false;
	Common::Point _viewportOrigin;
	Common::Rect _excludedClipRect;
	bool _clipExcluded = false;
};

} // End of namespace Topgun

#endif // TOPGUN_DISPLAY_H
