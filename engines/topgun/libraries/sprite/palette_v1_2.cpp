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

#include "topgun/display.h"
#include "topgun/libraries/sprite/sprite_v1_2.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

bool SpriteLibrary_v1_2::spriteChangePalette(uint16 first, uint16 count, const byte *entries, bool animate) {
	uint16 end = (uint16)(first + count);
	if ((uint16)(end - 1) > TG_DISPLAY_NUM_PALETTE_ENTRIES)
		return false;

	for (uint16 index = first; index < end; index++) {
		if (index >= TG_DISPLAY_NUM_PALETTE_ENTRIES) {
			warning("SpriteLibrary_v1_2::spriteChangePalette(): palette index %d is out of range", index);
			break;
		}

		memcpy(_paletteCopy[index], entries + (index - first) * TG_DISPLAY_PALETTE_ENTRY_SIZE, TG_DISPLAY_PALETTE_ENTRY_SIZE);
		_paletteCopy[index][3] = TG_DISPLAY_ANIMATABLE_PALETTE_FLAG;
	}

	Display *display = engine()->display();

	if (!display->hasPalette()) {
		buildDibColorTableFromPalette(first, end);
	} else if (_hasLogicalPalette && animate && first < TG_DISPLAY_NUM_PALETTE_ENTRIES) {
		uint16 animated = MIN<uint16>(count, (uint16)(TG_DISPLAY_NUM_PALETTE_ENTRIES - first));
		display->animateLogicalPalette(first, animated, &_paletteCopy[first][0]);
	}

	return true;
}

bool SpriteLibrary_v1_2::createIdentityLogicalPalette() {
	bool created = SpriteLibrary::createIdentityLogicalPalette();

	byte *table = &_colorTable[0][0];
	for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++)
		WRITE_LE_UINT16(table + 2 * i, (uint16)i);

	return created;
}

uint16 SpriteLibrary_v1_2::spriteGetRgbPalette(byte *destination, uint16 first, uint16 count) {
	uint32 entrySize = _hasLogicalPalette ? 2 : 4;
	uint32 size = entrySize * count;
	uint32 offset = (uint32)first * TG_DISPLAY_PALETTE_ENTRY_SIZE;
	uint32 tableSize = sizeof(_colorTable);

	if (offset + size > tableSize) {
		warning("SpriteLibrary_v1_2::spriteGetRgbPalette(): the color table range is out of range");
		if (offset >= tableSize)
			return _hasLogicalPalette ? 0 : 1;

		size = tableSize - offset;
	}

	memmove(destination, &_colorTable[0][0] + offset, size);
	return _hasLogicalPalette ? 0 : 1;
}

uint16 SpriteLibrary_v1_2::spriteSelectPalette() {
	if (!_hasLogicalPalette)
		return 0;

	return 1;
}

} // End of namespace Topgun
