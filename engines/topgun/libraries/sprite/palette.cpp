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

#include "topgun/display.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/topgun.h"

namespace Topgun {

bool SpriteLibrary::spriteChangePalette(uint16 first, uint16 count, const byte *entries) {
	uint16 end = (uint16)(first + count);
	if ((uint16)(end - 1) > Display::kNumPaletteEntries)
		return false;

	for (uint16 index = first; index < end; index++) {
		if (index >= Display::kNumPaletteEntries) {
			warning("SpriteLibrary::spriteChangePalette(): palette index %d is out of range", index);
			break;
		}

		memcpy(_paletteCopy[index], entries + (index - first) * Display::kPaletteEntrySize, Display::kPaletteEntrySize);
		_paletteCopy[index][3] = Display::kAnimatablePaletteFlag;
	}

	Display *display = _engine->display();

	if (!display->hasPalette()) {
		buildDibColorTableFromPalette(first, end);
	} else if (_hasLogicalPalette && first < Display::kNumPaletteEntries) {
		uint16 animated = MIN<uint16>(count, (uint16)(Display::kNumPaletteEntries - first));
		display->animateLogicalPalette(first, animated, &_paletteCopy[first][0]);
	}

	return true;
}

void SpriteLibrary::spriteGetPalette(byte *destination, uint16 first, uint16 count) {
	if ((uint32)first + count > Display::kNumPaletteEntries) {
		warning("SpriteLibrary::spriteGetPalette(): the palette range is out of range");
		if (first >= Display::kNumPaletteEntries)
			return;

		count = Display::kNumPaletteEntries - first;
	}

	memmove(destination, &_paletteCopy[first][0], count * Display::kPaletteEntrySize);
}

bool SpriteLibrary::spriteFadePalette(uint16 mult, uint16 divisor, uint16 first, uint16 count) {
	if (divisor == 0)
		divisor = 1;

	if (!_hasLogicalPalette || divisor < mult || divisor > 0xFF)
		return false;

	if ((uint32)first + count > Display::kNumPaletteEntries) {
		warning("SpriteLibrary::spriteFadePalette(): the palette range is out of range");
		if (first >= Display::kNumPaletteEntries)
			return true;

		count = Display::kNumPaletteEntries - first;
	}

	byte scaled[Display::kNumPaletteEntries * Display::kPaletteEntrySize];
	scalePaletteEntries(scaled, &_paletteCopy[first][0], mult, divisor, count);
	_engine->display()->animateLogicalPalette(first, count, scaled);
	return true;
}

bool SpriteLibrary::spriteRealizePalette() {
	if (!_hasLogicalPalette)
		return false;

	return _engine->display()->realizeLogicalPalette();
}

bool SpriteLibrary::spriteSetPalette(const byte *entries) {
	releasePaletteResources();

	if (!entries)
		return false;

	memcpy(_paletteCopy, entries, sizeof(_paletteCopy));

	if (_engine->display()->hasPalette())
		createIdentityLogicalPalette();

	if (!_hasLogicalPalette)
		buildDibColorTableFromPalette(0, Display::kNumPaletteEntries);

	return _hasLogicalPalette;
}

void SpriteLibrary::scalePaletteEntries(byte *destination, const byte *source, uint16 mult, uint16 divisor, uint16 count) {
	for (uint16 i = 0; i < count; i++) {
		destination[3] = source[3];

		for (int component = 0; component < 3; component++)
			destination[component] = (byte)(((mult & 0xFF) * source[component] + (divisor >> 1)) / (divisor & 0xFF));

		destination += Display::kPaletteEntrySize;
		source += Display::kPaletteEntrySize;
	}
}

bool SpriteLibrary::createIdentityLogicalPalette() {
	for (int i = 0; i < Display::kNumPaletteEntries; i++)
		_paletteCopy[i][3] = (i >= 10 && i <= 245) ? Display::kAnimatablePaletteFlag : 0;

	_dibColorUsage = 1;
	_hasLogicalPalette = _engine->display()->createLogicalPalette(&_paletteCopy[0][0]);
	return _hasLogicalPalette;
}

void SpriteLibrary::buildDibColorTableFromPalette(uint16 first, uint16 end) {
	_dibColorUsage = 0;

	for (uint16 i = first; i < end; i++) {
		if (i >= Display::kNumPaletteEntries) {
			warning("SpriteLibrary::buildDibColorTableFromPalette(): palette index %d is out of range", i);
			break;
		}

		_colorTable[i][2] = _paletteCopy[i][0];
		_colorTable[i][1] = _paletteCopy[i][1];
		_colorTable[i][0] = _paletteCopy[i][2];
	}
}

void SpriteLibrary::releasePaletteResources() {
	if (!_hasLogicalPalette)
		return;

	Display *display = _engine->display();
	display->selectDefaultPalette();
	display->deleteLogicalPalette();
	_hasLogicalPalette = false;
}

} // End of namespace Topgun
