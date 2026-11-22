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
#include "topgun/libraries/audio/audio.h"
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_1.h"

namespace Topgun {

#define TG_BG_FADE_STEP_COUNT   100
#define TG_BG_FIRST_FADE_COLOR  10
#define TG_BG_NUM_FADE_COLORS   200
#define TG_BG_NUM_RINGS         20
#define TG_BG_NUM_TILE_COLUMNS  20

void TopgunEngine_v1_0::syncBackgroundDimensions(uint16 bitmapId) {
	uint16 screenWidth = display()->width();
	uint16 screenHeight = display()->height();

	if (_flagMode640 || _flagLoMem) {
		_width = TG_DISPLAY_DEFAULT_WIDTH;
		_height = TG_DISPLAY_DEFAULT_HEIGHT;
	} else {
		_width = TG_DISPLAY_LARGE_WIDTH;
		_height = TG_DISPLAY_LARGE_HEIGHT;
	}

	if (bitmapId == 0)
		return;

	if (!_records || bitmapId >= _numResources || !_records[bitmapId].data) {
		warning("TopgunEngine_v1_0::syncBackgroundDimensions(): record index out of range");
		return;
	}

	const byte *bitmap = _records[bitmapId].data;
	_width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	_height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	_screenBox = (screenWidth > _width) || (_height < screenHeight);
}

void TopgunEngine_v1_0::commitBackgroundChangeAndRepaint(const Common::Rect &rect) {
	if (_sceneSprite != 0) {
		SpriteInfo info;
		spriteLibrary()->spriteGetInfo(_sceneSprite, info);

		Common::Rect area;
		if (info.visible != 0 && SpriteLibrary::intersectRects(info.boundingBox, rect, area)) {
			display()->excludeClipRect(area);
			handlePaintMessage();
			display()->resetClipRegion();
			spriteLibrary()->spritePaint(_sceneSprite);
			return;
		}
	}

	handlePaintMessage();
}

void TopgunEngine_v1_0::applyBackgroundToSpriteLib(uint16 bitmapId, Common::Rect *rectOut) {
	_backgroundId = bitmapId;

	if (bitmapId == 0) {
		spriteLibrary()->spriteSetBackground(nullptr, rectOut);
	} else if (!display()->hasPalette() && _faded) {
		utilsLibrary()->tgFree(_buffer1);
		_buffer1 = spriteLibrary()->spriteDibCreate(_width, _height, 1);
		if (!_buffer1)
			return;

		spriteLibrary()->spriteSetBackground(_buffer1, rectOut);
	} else {
		if (!_records || bitmapId >= _numResources) {
			warning("TopgunEngine_v1_0::applyBackgroundToSpriteLib(): record index out of range");
			return;
		}

		spriteLibrary()->spriteSetBackground(_records[bitmapId].data, rectOut);
	}
}

void TopgunEngine_v1_0::clearBackground() {
	if (_backgroundId == 0)
		return;

	if (!_records || _backgroundId >= _numResources) {
		warning("TopgunEngine_v1_0::clearBackground(): record index out of range");
		return;
	}

	_buffer2 = _records[_backgroundId].data;
	_records[_backgroundId].data = nullptr;
	_backgroundId = 0;
}

bool TopgunEngine_v1_0::setBackgroundFromBitmapResource(uint16 bitmapId) {
	if (bitmapId != 0 && !validateAndGetResourceByType(bitmapId, kResourceBitmap))
		bitmapId = 0;

	_backgroundId = bitmapId;
	_screenBox = true;
	syncBackgroundDimensions(bitmapId);

	Common::Rect rect;
	applyBackgroundToSpriteLib(bitmapId, &rect);
	commitBackgroundChangeAndRepaint(rect);
	return true;
}

void TopgunEngine_v1_0::copyAndPaintBackgroundRect(Common::Rect &rect) {
	if (_fadeIn)
		spriteLibrary()->spriteCopyRect(rect, _buffer2, _buffer1);
	else
		spriteLibrary()->spriteCopyRect(rect, _buffer1, _buffer2);

	spriteLibrary()->spritePaintRect(rect, true);
}

void TopgunEngine_v1_0::initDissolve() {
	for (uint16 i = 0; i < TG_HOST_NUM_DISSOLVE_TILES; i++)
		_tileOrder[i] = i;

	_stepCount = TG_HOST_NUM_DISSOLVE_TILES;
	_tileSize = _width / TG_BG_NUM_TILE_COLUMNS;
}

void TopgunEngine_v1_0::dissolveOneTile() {
	if (_stepCount == 0) {
		warning("TopgunEngine_v1_0::dissolveOneTile(): there are no tiles left");
		_active = false;
		return;
	}

	uint16 index = getRandom15Bit() % _stepCount;
	uint16 tile = _tileOrder[index];
	int16 left = (int16)((tile % TG_BG_NUM_TILE_COLUMNS) * _tileSize);
	int16 top = (int16)((tile / TG_BG_NUM_TILE_COLUMNS) * _tileSize);

	Common::Rect rect;
	rect.setRect(left, top, (int16)(left + _tileSize), (int16)(top + _tileSize));
	copyAndPaintBackgroundRect(rect);

	_stepCount--;
	_active = (_stepCount != 0);
	memmove(&_tileOrder[index], &_tileOrder[index + 1], (_stepCount - index) * sizeof(uint16));
}

void TopgunEngine_v1_0::computeInsetRingRect(uint16 k, Common::Rect *out, uint16 n) {
	uint32 divisor = (uint16)(2 * n - 1);

	out->left = (int16)((uint32)_width * k / divisor);
	out->right = (int16)(_width - out->left);
	out->top = (int16)((uint32)_height * k / divisor);
	out->bottom = (int16)(_height - out->top);
}

void TopgunEngine_v1_0::paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner) {
	Common::Rect band;
	band.setRect(0, outer.top, (int16)_width, inner.top);
	copyAndPaintBackgroundRect(band);

	band.top = inner.bottom;
	band.bottom = outer.bottom;
	copyAndPaintBackgroundRect(band);

	spriteLibrary()->spriteAnimate();
	audioLibrary()->audioService();
	presentScreenAndWait();
}

void TopgunEngine_v1_0::wipeEffectFrameRingsInsideOut() {
	Common::Rect inner;
	Common::Rect outer;

	computeInsetRingRect(TG_BG_NUM_RINGS - 1, &inner, TG_BG_NUM_RINGS);

	Common::Rect band = inner;
	band.left = 0;
	band.right = (int16)_width;
	copyAndPaintBackgroundRect(band);
	presentScreenAndWait();

	for (int16 k = TG_BG_NUM_RINGS - 2; k >= 0; k--) {
		computeInsetRingRect(k, &outer, TG_BG_NUM_RINGS);
		paintFrameBandsTopBottom(outer, inner);
		inner = outer;
	}
}

void TopgunEngine_v1_0::wipeEffectFrameRingsOutsideIn() {
	Common::Rect outer;
	Common::Rect inner;

	computeInsetRingRect(0, &outer, TG_BG_NUM_RINGS);

	for (uint16 k = 1; k < TG_BG_NUM_RINGS; k++) {
		computeInsetRingRect(k, &inner, TG_BG_NUM_RINGS);
		paintFrameBandsTopBottom(outer, inner);
		outer = inner;
	}

	inner.left = 0;
	inner.right = (int16)_width;
	copyAndPaintBackgroundRect(inner);
	presentScreenAndWait();
}

void TopgunEngine_v1_0::advanceSceneFadeStep() {
	if (display()->hasPalette()) {
		uint16 mult = _fadeIn ? _step : (uint16)(_stepCount - _step);
		spriteLibrary()->spriteFadePalette(mult, _stepCount, TG_BG_FIRST_FADE_COLOR, TG_BG_NUM_FADE_COLORS);

		_step++;
		if (_step > _stepCount)
			_active = false;
	} else if (_buffer1) {
		dissolveOneTile();
	}
}

void TopgunEngine_v1_0::sceneTransitionIn() {
	_fadeIn = true;

	if (display()->hasPalette()) {
		_step = 1;
		_stepCount = TG_BG_FADE_STEP_COUNT;
		_active = true;

		do {
			resourceLoadProgressCallback();
			presentScreenAndWait();
		} while (_active);

		_faded = false;
		return;
	}

	if (!_buffer1) {
		_buffer1 = spriteLibrary()->spriteDibCreate(_width, _height, 1);
		if (!_buffer1)
			return;
	}

	initDissolve();
	_active = true;

	_buffer2 = nullptr;
	if (_backgroundId != 0) {
		if (!_records || _backgroundId >= _numResources)
			warning("TopgunEngine_v1_0::sceneTransitionIn(): record index out of range");
		else
			_buffer2 = _records[_backgroundId].data;
	}

	spriteLibrary()->spriteSetBackground(_buffer1, nullptr);
	wipeEffectFrameRingsInsideOut();
	_faded = false;
	spriteLibrary()->spriteSetBackground(_buffer2, nullptr);
	_buffer1 = utilsLibrary()->tgFree(_buffer1);
}

void TopgunEngine_v1_0::sceneTransitionOut() {
	_fadeIn = false;

	if (display()->hasPalette()) {
		_step = 2;
		_stepCount = TG_BG_FADE_STEP_COUNT;
		_active = true;

		do {
			resourceLoadProgressCallback();
			presentScreenAndWait();
		} while (_active);

		_faded = true;
		return;
	}

	utilsLibrary()->tgFree(_buffer1);
	_buffer1 = spriteLibrary()->spriteDibCreate(_width, _height, 1);
	if (!_buffer1)
		return;

	initDissolve();
	_active = true;
	wipeEffectFrameRingsOutsideIn();
	_faded = true;
	_buffer1 = utilsLibrary()->tgFree(_buffer1);
}

void TopgunEngine_v1_1::paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner) {
	Common::Rect band;
	band.setRect(0, outer.top, (int16)_width, inner.top);
	copyAndPaintBackgroundRect(band);

	band.top = inner.bottom;
	band.bottom = outer.bottom;
	copyAndPaintBackgroundRect(band);

	spriteLibrary()->spriteAnimate();
	presentScreenAndWait();
}

void TopgunEngine_v1_1::advanceSceneFadeStep() {
	spriteLibrary()->spriteAnimate();

	uint16 mult = _fadeIn ? _step : (uint16)(_stepCount - _step);
	spriteLibrary()->spriteFadePalette(mult, _stepCount, TG_BG_FIRST_FADE_COLOR, TG_BG_NUM_FADE_COLORS);

	_step++;
	if (_step > _stepCount)
		_active = false;
}

void TopgunEngine_v1_1::sceneTransitionIn() {
	_fadeIn = true;

	if (display()->hasPalette()) {
		_step = 1;
		_stepCount = TG_BG_FADE_STEP_COUNT;
		_active = true;

		do {
			advanceSceneFadeStep();
			presentScreenAndWait();
		} while (_active);

		_faded = false;
		return;
	}

	if (!_buffer1) {
		_buffer1 = spriteLibrary()->spriteDibCreate(_width, _height, 1);
		if (!_buffer1)
			return;
	}

	_active = true;

	_buffer2 = nullptr;
	if (_backgroundId != 0) {
		if (!_records || _backgroundId >= _numResources)
			warning("TopgunEngine_v1_1::sceneTransitionIn(): record index out of range");
		else
			_buffer2 = _records[_backgroundId].data;
	}

	spriteLibrary()->spriteSetBackground(_buffer1, nullptr);
	wipeEffectFrameRingsInsideOut();
	_faded = false;
	spriteLibrary()->spriteSetBackground(_buffer2, nullptr);
	_buffer1 = utilsLibrary()->tgFree(_buffer1);
	_buffer2 = nullptr;
}

void TopgunEngine_v1_1::sceneTransitionOut() {
	_fadeIn = false;

	if (display()->hasPalette()) {
		_step = 2;
		_stepCount = TG_BG_FADE_STEP_COUNT;
		_active = true;

		do {
			advanceSceneFadeStep();
			presentScreenAndWait();
		} while (_active);

		_faded = true;
		return;
	}

	_buffer1 = spriteLibrary()->spriteDibCreate(_width, _height, 1);
	if (!_buffer1)
		return;

	_active = true;
	wipeEffectFrameRingsOutsideIn();
	_faded = true;
	_buffer1 = utilsLibrary()->tgFree(_buffer1);
}

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
	if ((_version >= kVersionV1_2_1 || _backgroundId == 0) && _backgroundBitmap)
		utilsLibrary()->tgFree(_backgroundBitmap);

	_backgroundBitmap = nullptr;
	_backgroundColor = 0;
	_backgroundId = 0;

	spriteLib()->spriteSetBackground(nullptr, false, 0, nullptr);
}

void TopgunEngine_v1_2::setBackgroundFromBitmapResource(const byte *record) {
	uint16 bitmapId = READ_LE_UINT16(record + 4);
	uint16 alternativeId = READ_LE_UINT16(record + 6);

	bool lowMemory = (_version >= kVersionV1_2_1) ? (readVariable(TG_HOST_V1_2_1_VARIABLE_LOMEM) != 0) : _flagLoMem;

	if ((lowMemory || _flagMode640) && alternativeId != 0)
		bitmapId = alternativeId;

	if (_version >= kVersionV1_2_1) {
		byte *copy = static_cast<TopgunEngine_v1_2_1 *>(this)->loadBitmapResourceCopy(bitmapId);
		if (!copy)
			return;

		clearBackground();
		_backgroundBitmap = copy;
	} else {
		if (bitmapId == 0)
			return;

		if (!validateAndGetResourceByType(bitmapId, kResV1_2Bitmap))
			return;

		_backgroundId = bitmapId;

		uint16 index = operandValue(bitmapId);
		if (!isRecordIndexValid(index, "setBackgroundFromBitmapResource"))
			return;

		_backgroundBitmap = _records[index].data;
	}

	syncBackgroundDimensions(_backgroundBitmap);

	Common::Rect rect;
	applyBackgroundToSpriteLib(&rect);
	commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2::setSolidBackgroundColorPalette(uint32 color) {
	syncBackgroundDimensions(nullptr);

	Common::Rect rect;

	spriteLib()->spriteSetBackground(nullptr, false, (_version >= kVersionV1_2_1) ? color : TG_HOST_V1_2_SOLID_COLOR_ENTRY, &rect);

	_backgroundColor = color;

	if (_version < kVersionV1_2_1) {
		byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
		entry[0] = (byte)(color & 0xFF);
		entry[1] = (byte)((color >> 8) & 0xFF);
		entry[2] = (byte)((color >> 16) & 0xFF);
		entry[3] = 0;

		spriteLib()->spriteChangePalette(TG_HOST_V1_2_SOLID_COLOR_ENTRY, 1, entry, !_faded);
	}

	commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2::setSolidBackgroundColorBitmap(uint32 color) {
	syncBackgroundDimensions(nullptr);

	_backgroundBitmap = spriteLib()->spriteDibCreate(_width, _height, 0);
	if (!_backgroundBitmap)
		return;

	byte *pixels = _backgroundBitmap + READ_LE_UINT16(_backgroundBitmap);
	uint32 count = (uint32)READ_LE_UINT16(_backgroundBitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET) * _height;

	if (_version >= kVersionV1_2_1) {
		memset(pixels, (byte)color, count);
	} else {
		memset(pixels, TG_HOST_V1_2_SOLID_COLOR_ENTRY, count);

		byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
		entry[0] = (byte)(color & 0xFF);
		entry[1] = (byte)((color >> 8) & 0xFF);
		entry[2] = (byte)((color >> 16) & 0xFF);
		entry[3] = 0;

		spriteLib()->spriteChangePalette(TG_HOST_V1_2_SOLID_COLOR_ENTRY, 1, entry, !_faded);
	}

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
	if (_wipeEffectActive)
		spriteLib()->spriteCopyRect(rect, _wipeOldBitmap, _wipeNewBitmap);
	else if (_fadeIn)
		spriteLib()->spriteCopyRect(rect, _buffer1, _backgroundBitmap);
	else if (_version >= kVersionV1_2_1)
		static_cast<SpriteLibrary_v1_2_1 *>(spriteLib())->spriteDibSet(_buffer1, rect.left, rect.top, (uint16)(rect.right - rect.left), (uint16)(rect.bottom - rect.top), 0);
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
	spriteLib()->spriteFadePalette(mult, _stepCount, TG_HOST_V1_2_NUM_SYSTEM_COLORS, (_version >= kVersionV1_2_1) ? _maxFadeColors : TG_HOST_V1_2_NUM_FADE_COLORS);

	_step++;
	if (_step > _stepCount)
		_active = false;
}

void TopgunEngine_v1_2::sceneTransitionIn(uint16 steps) {
	if (!_faded)
		return;

	uint16 sendMessage = 0;

	if (_version >= kVersionV1_2_1)
		sendMessage = spriteLib()->spriteSetSendMsg(0);

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
	} else {
		if (!_buffer1)
			_buffer1 = spriteLib()->spriteDibCreate(_width, _height, 1);

		if (_buffer1) {
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
	}

	if (_version >= kVersionV1_2_1)
		spriteLib()->spriteSetSendMsg(sendMessage);
}

void TopgunEngine_v1_2::sceneTransitionOut(uint16 steps) {
	if (_faded)
		return;

	uint16 sendMessage = 0;

	if (_version >= kVersionV1_2_1)
		sendMessage = spriteLib()->spriteSetSendMsg(0);

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
	} else if (_version < kVersionV1_2_1 || _backgroundBitmap) {
		_buffer1 = spriteLib()->spriteDibCreate(_width, _height, (_version >= kVersionV1_2_1) ? 0 : 1);

		if (_buffer1) {
			if (_version >= kVersionV1_2_1) {
				uint32 count = (uint32)READ_LE_UINT16(_backgroundBitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(_backgroundBitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
				utilsLibrary()->copyHugeBytes(_buffer1 + READ_LE_UINT16(_buffer1), _backgroundBitmap + READ_LE_UINT16(_backgroundBitmap), count);
			}

			uint16 rings = (uint16)((steps * TG_HOST_V1_2_WIPE_RINGS_FACTOR + TG_HOST_V1_2_WIPE_RINGS_BASE) / TG_HOST_V1_2_WIPE_RINGS_DIVISOR);
			if (rings < TG_HOST_V1_2_WIPE_MIN_RINGS)
				rings = TG_HOST_V1_2_WIPE_MIN_RINGS;

			_active = true;

			if (_version >= kVersionV1_2_1)
				spriteLib()->spriteSetBackground(_buffer1, false, 0, nullptr);

			wipeEffectFrameRingsOutsideIn(rings, true);
			_faded = true;

			if (_version < kVersionV1_2_1)
				spriteLib()->spriteSetBackground(_buffer1, false, 0, nullptr);
		}
	}

	if (_version >= kVersionV1_2_1)
		spriteLib()->spriteSetSendMsg(sendMessage);
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

byte *TopgunEngine_v1_2_1::loadBitmapResourceCopy(uint16 id) {
	if (id == 0)
		return nullptr;

	if (!validateResourceType(id, kResV1_2_1Bitmap))
		return nullptr;

	byte *copy = nullptr;

	if (isResourceObjectLoaded(id, kResV1_2_1Bitmap)) {
		uint16 index = operandValue(id);
		if (!isRecordIndexValid(index, "loadBitmapResourceCopy"))
			return nullptr;

		copy = utilsLibrary()->tgPtrCpy(_records[index].data);
	} else {
		copy = loadResourceWithProcessingPause(id, nullptr);
	}

	if (copy && (copy[TG_RES_V1_2_1_BITMAP_FLAGS_BYTE_OFFSET] & TG_RES_V1_2_1_BITMAP_COMPRESSED_MASK) != 0) {
		byte *expanded = resourceLib()->resExpandBitmap(_file, copy);
		utilsLibrary()->tgFree(copy);
		copy = expanded;
	}

	return copy;
}

void TopgunEngine_v1_2_1::setBackgroundByMode(const byte *record) {
	switch (READ_LE_UINT16(record + 2)) {
	case 0:
		setBackgroundFromBitmapResource(record);
		break;

	case 1:
		clearBackground();

		if (display()->hasPalette())
			setSolidBackgroundColorPalette(READ_LE_UINT16(record + 8));
		else
			setSolidBackgroundColorBitmap(READ_LE_UINT16(record + 8));
		break;

	case 2:
		clearBackground();
		spriteLib()->spriteSetBackground(nullptr, true, 0, nullptr);
		break;

	default:
		clearBackground();
		break;
	}
}

bool TopgunEngine_v1_2_1::setBackgroundWithWipeEffect(const byte *record) {
	if (!_backgroundBitmap || READ_LE_UINT16(record + 2) != 0)
		return false;

	uint16 bitmapId = READ_LE_UINT16(record + 4);
	if ((_flagLoMem || _flagMode640) && READ_LE_UINT16(record + 6) != 0)
		bitmapId = READ_LE_UINT16(record + 6);

	byte *bitmap = loadBitmapResourceCopy(bitmapId);
	if (!bitmap)
		return false;

	executeWipeEffect(record + 0x0A, bitmap, _backgroundBitmap);
	utilsLibrary()->tgFree(bitmap);
	return true;
}

void TopgunEngine_v1_2_1::wipeEffectIrisRings(uint16 rings) {
	if (rings == 0)
		rings = 10;

	uint16 rowsPerPass = (uint16)((_height + rings - 1) / rings);

	Common::Rect rect;
	rect.left = 0;
	rect.right = _width;

	for (uint16 pass = 0; pass < rings; pass++) {
		uint16 top = pass;

		for (uint16 row = 0; row < rowsPerPass; row++) {
			if (top > _height)
				break;

			rect.top = top;
			rect.bottom = (int16)(top + 1);
			copyAndPaintBackgroundRect(rect);

			top = (uint16)(top + rings);
			spriteLib()->spriteAnimate();
		}

		presentScreenAndWait();
	}
}

void TopgunEngine_v1_2_1::executeWipeEffect(const byte *params, byte *newBitmap, byte *oldBitmap) {
	_wipeOldBitmap = oldBitmap;
	_wipeNewBitmap = newBitmap;
	_wipeEffectActive = true;

	uint16 sendMessage = spriteLib()->spriteSetSendMsg(0);

	switch (READ_LE_UINT16(params)) {
	case 1:
		wipeEffectIrisRings(READ_LE_UINT16(params + 2));
		break;

	case 2:
		wipeEffectFrameRingsOutsideIn(30, true);
		break;

	case 3:
		wipeEffectFrameRingsInsideOut(30, true);
		break;

	case 4:
		wipeEffectFrameRingsOutsideIn(30, false);
		break;

	case 5:
		wipeEffectFrameRingsInsideOut(30, false);
		break;

	default:
		break;
	}

	spriteLib()->spriteSetSendMsg(sendMessage);
	_wipeEffectActive = false;
}

} // End of namespace Topgun
