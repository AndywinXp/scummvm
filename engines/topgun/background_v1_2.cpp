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

#include "topgun/display.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2.h"

namespace Topgun {

void TopgunEngine_v1_2::syncBackgroundDimensions(const byte *bitmap) {
	uint16 screenWidth = display()->width();
	uint16 screenHeight = display()->height();

	_width = screenWidth;
	_height = screenHeight;
	_screenBox = true;

	if (!bitmap)
		return;

	_width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	_height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	_screenBox = (screenWidth > _width) || (_height < screenHeight);
}

void TopgunEngine_v1_2::commitBackgroundChangeAndRepaint(const Common::Rect &rect) {
	if (_sceneSprite != 0 && _faded) {
		SpriteInfo_v1_2 info;
		spriteLib()->spriteGetInfo(_sceneSprite, info);

		Common::Rect area;
		if (info.visible != 0 && SpriteLibrary::intersectRects(info.boundingBox, rect, area)) {
			display()->excludeClipRect(area);
			handlePaintMessage();
			display()->resetClipRegion();
			spriteLib()->spritePaint(_sceneSprite);
			return;
		}
	} else if (!display()->hasPalette() && _faded) {
		return;
	}

	handlePaintMessage();
}

void TopgunEngine_v1_2::applyBackgroundToSpriteLib(Common::Rect *rectOut) {
	if (!display()->hasPalette() && _faded) {
		if (_buffer1)
			_buffer1 = utilsLibrary()->tgFree(_buffer1);

		_buffer1 = spriteLib()->spriteDibCreate(_width, _height, 1);
		if (!_buffer1)
			return;

		spriteLib()->spriteSetBackground(_buffer1, false, 0, rectOut);
	} else {
		spriteLib()->spriteSetBackground(_backgroundBitmap, false, 0, rectOut);
	}
}

void TopgunEngine_v1_2::clearBackground() {
	if (_backgroundId == 0 && _backgroundBitmap)
		utilsLibrary()->tgFree(_backgroundBitmap);

	_backgroundBitmap = nullptr;
	_backgroundColor = 0;
	_backgroundId = 0;

	spriteLib()->spriteSetBackground(nullptr, false, 0, nullptr);
}

void TopgunEngine_v1_2::setBackgroundFromBitmapResource(const byte *record) {
	uint16 bitmapId = READ_LE_UINT16(record + 4);
	uint16 alternativeId = READ_LE_UINT16(record + 6);

	if ((_flagLoMem || _flagMode640) && alternativeId != 0)
		bitmapId = alternativeId;

	if (bitmapId == 0)
		return;

	if (!validateAndGetResourceByType(bitmapId, kResV1_2Bitmap))
		return;

	_backgroundId = bitmapId;

	uint16 index = operandValue(bitmapId);
	if (!isRecordIndexValid(index, "setBackgroundFromBitmapResource"))
		return;

	_backgroundBitmap = _records[index].data;
	syncBackgroundDimensions(_backgroundBitmap);

	Common::Rect rect;
	applyBackgroundToSpriteLib(&rect);
	commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2::setSolidBackgroundColorPalette(uint32 color) {
	syncBackgroundDimensions(nullptr);

	Common::Rect rect;
	spriteLib()->spriteSetBackground(nullptr, false, TG_HOST_V1_2_SOLID_COLOR_ENTRY, &rect);

	_backgroundColor = color;

	byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
	entry[0] = (byte)(color & 0xFF);
	entry[1] = (byte)((color >> 8) & 0xFF);
	entry[2] = (byte)((color >> 16) & 0xFF);
	entry[3] = 0;

	spriteLib()->spriteChangePalette(TG_HOST_V1_2_SOLID_COLOR_ENTRY, 1, entry, !_faded);
	commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2::setSolidBackgroundColorBitmap(uint32 color) {
	syncBackgroundDimensions(nullptr);

	_backgroundBitmap = spriteLib()->spriteDibCreate(_width, _height, 0);
	if (!_backgroundBitmap)
		return;

	byte *pixels = _backgroundBitmap + READ_LE_UINT16(_backgroundBitmap);
	uint32 count = (uint32)READ_LE_UINT16(_backgroundBitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET) * _height;
	memset(pixels, TG_HOST_V1_2_SOLID_COLOR_ENTRY, count);

	byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
	entry[0] = (byte)(color & 0xFF);
	entry[1] = (byte)((color >> 8) & 0xFF);
	entry[2] = (byte)((color >> 16) & 0xFF);
	entry[3] = 0;

	spriteLib()->spriteChangePalette(TG_HOST_V1_2_SOLID_COLOR_ENTRY, 1, entry, !_faded);

	_backgroundColor = 0;
	_screenBox = false;

	syncBackgroundDimensions(_backgroundBitmap);

	Common::Rect rect;
	applyBackgroundToSpriteLib(&rect);
	commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2::o_setBackground(const byte *record) {
	clearBackground();

	switch (READ_LE_UINT16(record + 2)) {
	case 0:
		setBackgroundFromBitmapResource(record);
		break;

	case 1:
		warning("setDesktopBackground UNIMPLEMENTED");
		break;

	case 2:
		if (display()->hasPalette())
			setSolidBackgroundColorPalette(READ_LE_UINT32(record + 4));
		else
			setSolidBackgroundColorBitmap(READ_LE_UINT32(record + 4));
		break;

	case 3:
		spriteLib()->spriteSetBackground(nullptr, true, 0, nullptr);
		break;

	default:
		break;
	}
}

void TopgunEngine_v1_2::copyAndPaintBackgroundRect(Common::Rect &rect) {
	if (_fadeIn)
		spriteLib()->spriteCopyRect(rect, _buffer1, _backgroundBitmap);
	else
		spriteLib()->spriteCopyRect(rect, _backgroundBitmap, _buffer1);

	spriteLib()->spritePaintRect(rect, true);
}

void TopgunEngine_v1_2::paintFrameBandsLeftRight(const Common::Rect &outer, const Common::Rect &inner) {
	Common::Rect band;
	band.left = outer.left;
	band.top = inner.top;
	band.right = inner.left;
	band.bottom = inner.bottom;
	copyAndPaintBackgroundRect(band);

	band.left = inner.right;
	band.right = outer.right;
	copyAndPaintBackgroundRect(band);
}

void TopgunEngine_v1_2::paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner) {
	Common::Rect band;
	band.left = outer.left;
	band.top = outer.top;
	band.right = outer.right;
	band.bottom = inner.top;
	copyAndPaintBackgroundRect(band);

	band.top = inner.bottom;
	band.bottom = outer.bottom;
	copyAndPaintBackgroundRect(band);
}

void TopgunEngine_v1_2::wipeEffectFrameRingsInsideOut(uint16 rings, bool insetSides) {
	Common::Rect inner;
	Common::Rect outer;

	computeInsetRingRect(rings - 1, &inner, rings, insetSides);

	Common::Rect band = inner;
	copyAndPaintBackgroundRect(band);
	presentScreenAndWait();

	for (int16 ring = (int16)(rings - 2); ring >= 0; ring--) {
		spriteLib()->spriteAnimate();
		computeInsetRingRect(ring, &outer, rings, insetSides);

		if (insetSides)
			paintFrameBandsLeftRight(outer, inner);

		paintFrameBandsTopBottom(outer, inner);
		presentScreenAndWait();
		inner = outer;
	}
}

void TopgunEngine_v1_2::wipeEffectFrameRingsOutsideIn(uint16 rings, bool insetSides) {
	Common::Rect inner;
	Common::Rect outer;

	computeInsetRingRect(rings - 1, &inner, rings, insetSides);
	computeInsetRingRect(0, &outer, rings, insetSides);

	for (uint16 ring = 1; rings > 1 && ring < rings; ring++) {
		computeInsetRingRect(ring, &inner, rings, insetSides);
		paintFrameBandsTopBottom(outer, inner);

		if (insetSides)
			paintFrameBandsLeftRight(outer, inner);

		spriteLib()->spriteAnimate();
		presentScreenAndWait();
		outer = inner;
	}

	copyAndPaintBackgroundRect(inner);
	presentScreenAndWait();
}

void TopgunEngine_v1_2::computeInsetRingRect(uint16 ring, Common::Rect *out, uint16 rings, bool insetSides) {
	// The divisor is 2 * rings - 1, this is the exact number the game expects
	uint32 divisor = (uint16)(2 * rings - 1);
	if (divisor == 0) {
		warning("TopgunEngine_v1_2::computeInsetRingRect(): there are no rings");
		divisor = 1;
	}

	if (insetSides)
		out->left = (int16)((uint32)ring * _width / divisor);
	else
		out->left = 0;

	out->right = (int16)(_width - out->left);
	out->top = (int16)((uint32)ring * _height / divisor);
	out->bottom = (int16)(_height - out->top);
}

void TopgunEngine_v1_2::advanceSceneFadeStep() {
	spriteLib()->spriteAnimate();

	uint16 mult = _fadeIn ? _step : (uint16)(_stepCount - _step);
	spriteLib()->spriteFadePalette(mult, _stepCount, TG_HOST_V1_2_NUM_SYSTEM_COLORS, TG_HOST_V1_2_NUM_FADE_COLORS);

	_step++;
	if (_step > _stepCount)
		_active = false;
}

void TopgunEngine_v1_2::sceneTransitionIn(uint16 steps) {
	if (!_faded)
		return;

	_fadeIn = true;

	if (display()->hasPalette()) {
		_step = 1;
		_stepCount = steps;
		_active = true;

		do {
			advanceSceneFadeStep();
			presentScreenAndWait();
		} while (_active);

		_faded = false;
		return;
	}

	if (!_buffer1) {
		_buffer1 = spriteLib()->spriteDibCreate(_width, _height, 1);
		if (!_buffer1)
			return;
	}

	uint16 rings = (uint16)((steps * TG_HOST_V1_2_WIPE_RINGS_FACTOR + TG_HOST_V1_2_WIPE_RINGS_BASE) / TG_HOST_V1_2_WIPE_RINGS_DIVISOR);
	if (rings < TG_HOST_V1_2_WIPE_MIN_RINGS)
		rings = TG_HOST_V1_2_WIPE_MIN_RINGS;

	_active = true;

	spriteLib()->spriteSetBackground(_buffer1, false, 0, nullptr);
	wipeEffectFrameRingsInsideOut(rings, true);
	spriteLib()->spriteSetBackground(_backgroundBitmap, false, 0, nullptr);
	_faded = false;
	_buffer1 = utilsLibrary()->tgFree(_buffer1);
}

void TopgunEngine_v1_2::sceneTransitionOut(uint16 steps) {
	if (_faded)
		return;

	_fadeIn = false;

	if (display()->hasPalette()) {
		_step = 2;
		_stepCount = steps;
		_active = true;

		do {
			advanceSceneFadeStep();
			presentScreenAndWait();
		} while (_active);

		_faded = true;
		return;
	}

	_buffer1 = spriteLib()->spriteDibCreate(_width, _height, 1);
	if (!_buffer1)
		return;

	uint16 rings = (uint16)((steps * TG_HOST_V1_2_WIPE_RINGS_FACTOR + TG_HOST_V1_2_WIPE_RINGS_BASE) / TG_HOST_V1_2_WIPE_RINGS_DIVISOR);
	if (rings < TG_HOST_V1_2_WIPE_MIN_RINGS)
		rings = TG_HOST_V1_2_WIPE_MIN_RINGS;

	_active = true;

	wipeEffectFrameRingsOutsideIn(rings, true);
	_faded = true;
	spriteLib()->spriteSetBackground(_buffer1, false, 0, nullptr);
}

void TopgunEngine_v1_2::o_fadeInOut(const byte *record) {
	if (!display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(true);

	uint16 steps = READ_LE_UINT16(record + 2);
	if (steps < 1 || steps > TG_HOST_V1_2_FADE_STEPS)
		steps = TG_HOST_V1_2_FADE_STEPS;

	if (READ_LE_UINT16(record) == 10) {
		sceneTransitionIn(steps);

		if (!display()->hasPalette())
			spriteLib()->spriteSetBkgTransparent(false);

		return;
	}

	if (_entryScriptRunning) {
		_fadeIn = false;
		return;
	}

	sceneTransitionOut(steps);
}

} // End of namespace Topgun
