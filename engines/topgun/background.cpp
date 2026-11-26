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
#include "topgun/topgun_v1_2_3.h"

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

	if (_version >= kVersionV1_2_9)
		_screenBox = _scrollBoxWindowBox;

	if (!bitmap)
		return;

	_width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	_height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	_screenBox = (screenWidth > _width) || (_height < screenHeight);

	if (_version >= kVersionV1_2_9 && _scrollBoxWindowBox)
		_screenBox = true;
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
	} else if (_version >= kVersionV1_2_9 && _tileMap) {
		static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteSetTileBackground(_tileMap, rectOut);
	} else {
		spriteLib()->spriteSetBackground(_backgroundBitmap, false, 0, rectOut);
	}
}

void TopgunEngine_v1_2::clearBackground() {
	if (_version >= kVersionV1_2_9 && _tileMap) {
		uint16 tileId = _tileMap->id;

		if (tileId != 0)
			destroyResourceObjectByType(tileId, kResV1_2_9Tile);

		_tileMap = nullptr;
	}

	if ((_version >= kVersionV1_2_2_2 || _backgroundId == 0) && _backgroundBitmap)
		utilsLibrary()->tgFree(_backgroundBitmap);

	_backgroundBitmap = nullptr;
	_backgroundColor = 0;
	_backgroundId = 0;

	spriteLib()->spriteSetBackground(nullptr, false, 0, nullptr);
}

void TopgunEngine_v1_2::setBackgroundFromBitmapResource(const byte *record) {
	uint16 bitmapId = READ_LE_UINT16(record + 4);
	uint16 alternativeId = READ_LE_UINT16(record + 6);

	bool lowMemory = (_version >= kVersionV1_2_3) ? (readVariable(TG_HOST_V1_2_3_VARIABLE_LOMEM) != 0) : _flagLoMem;

	if ((lowMemory || _flagMode640) && alternativeId != 0)
		bitmapId = alternativeId;

	if (_version >= kVersionV1_2_2_2) {
		byte *copy = static_cast<TopgunEngine_v1_2_3 *>(this)->loadBitmapResourceCopy(bitmapId);
		if (!copy)
			return;

		clearBackground();
		_backgroundBitmap = copy;
	} else if (_version >= kVersionV1_2_2_1) {
		if (bitmapId == 0 || !ensureBitmapOrGroupResourceLoaded(bitmapId))
			return;

		uint16 index = operandValue(bitmapId);
		if (!isRecordIndexValid(index, "setBackgroundFromBitmapResource"))
			return;

		_backgroundId = index;
		_backgroundBitmap = _records[index].data;

		syncBackgroundDimensions(_backgroundBitmap);

		// A compressed bitmap is expanded into a copy that belongs to the background
		if ((_backgroundBitmap[TG_RES_V1_2_2_1_BITMAP_FLAGS_BYTE_OFFSET] & TG_RES_V1_2_2_1_BITMAP_COMPRESSED_MASK) != 0) {
			_backgroundId = 0;

			byte *expanded = resourceLib()->resExpandBitmap(_file, _backgroundBitmap);
			if (!expanded) {
				// The original frees the data of the resource, which stays in its record
				warning("TopgunEngine_v1_2::setBackgroundFromBitmapResource(): bitmap %d cannot be expanded", index);
				_backgroundBitmap = nullptr;
				clearBackground();
				return;
			}

			_backgroundBitmap = expanded;
		}
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

	// The demo takes the dimensions from the bitmap before it expands it
	if (_version != kVersionV1_2_2_1)
		syncBackgroundDimensions(_backgroundBitmap);

	Common::Rect rect;
	applyBackgroundToSpriteLib(&rect);
	commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2::setSolidBackgroundColorPalette(uint32 color) {
	syncBackgroundDimensions(nullptr);

	Common::Rect rect;

	spriteLib()->spriteSetBackground(nullptr, false, (_version >= kVersionV1_2_2_2) ? color : _solidColorEntry, &rect);

	_backgroundColor = color;

	if (_version >= kVersionV1_2_7)
		_solidBackground = true;

	if (_version < kVersionV1_2_2_2) {
		byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
		entry[0] = (byte)(color & 0xFF);
		entry[1] = (byte)((color >> 8) & 0xFF);
		entry[2] = (byte)((color >> 16) & 0xFF);
		entry[3] = 0;

		spriteLib()->spriteChangePalette(_solidColorEntry, 1, entry, !_faded);
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

	if (_version >= kVersionV1_2_2_2) {
		memset(pixels, (byte)color, count);
	} else {
		memset(pixels, _solidColorEntry, count);

		byte entry[TG_DISPLAY_PALETTE_ENTRY_SIZE];
		entry[0] = (byte)(color & 0xFF);
		entry[1] = (byte)((color >> 8) & 0xFF);
		entry[2] = (byte)((color >> 16) & 0xFF);
		entry[3] = 0;

		spriteLib()->spriteChangePalette(_solidColorEntry, 1, entry, !_faded);
	}

	if (_version >= kVersionV1_2_7) {
		_backgroundColor = color;
		_solidBackground = true;
	} else {
		_backgroundColor = 0;
	}

	_screenBox = false;

	syncBackgroundDimensions(_backgroundBitmap);

	Common::Rect rect;
	applyBackgroundToSpriteLib(&rect);
	commitBackgroundChangeAndRepaint(rect);
}

bool TopgunEngine_v1_2::setBackgroundWithWipeEffect(const byte *record) {
	if (!_backgroundBitmap || READ_LE_UINT16(record + 2) != 0)
		return false;

	uint16 bitmapId = READ_LE_UINT16(record + 4);
	if ((_flagLoMem || _flagMode640) && READ_LE_UINT16(record + 6) != 0)
		bitmapId = READ_LE_UINT16(record + 6);

	if (_version >= kVersionV1_2_2_2) {
		byte *bitmap = static_cast<TopgunEngine_v1_2_3 *>(this)->loadBitmapResourceCopy(bitmapId);
		if (!bitmap)
			return false;

		executeWipeEffect(record + 0x0A, bitmap, _backgroundBitmap);
		utilsLibrary()->tgFree(bitmap);
		return true;
	}

	if (bitmapId == 0)
		return false;

	if (_version >= kVersionV1_2_2_1) {
		if (!ensureBitmapOrGroupResourceLoaded(bitmapId))
			return false;
	} else if (!validateAndGetResourceByType(bitmapId, kResV1_2_1Bitmap)) {
		return false;
	}

	uint16 index = operandValue(bitmapId);
	if (!isRecordIndexValid(index, "setBackgroundWithWipeEffect"))
		return false;

	uint16 wipeOffset = (_version >= kVersionV1_2_2_1) ? TG_HOST_V1_2_2_1_BACKGROUND_WIPE_OFFSET : 8;

	executeWipeEffect(record + wipeOffset, _records[index].data, _backgroundBitmap);

	// The record of the replaced bitmap is looked up with the stored operand (the index in the demo)
	if (_backgroundId != 0 && isRecordIndexValid(_backgroundId, "setBackgroundWithWipeEffect"))
		_records[_backgroundId].data = nullptr;

	uint16 type = kResV1_2_1Bitmap;
	if (_version >= kVersionV1_2_2_1)
		type = resourceLib()->resGetType(_file, index);

	destroyResourceObjectByType(bitmapId, type);
	return true;
}

void TopgunEngine_v1_2::o_setBackground(const byte *record) {
	uint16 wipeOffset = 8;
	uint16 colorOffset = 4;

	if (_version >= kVersionV1_2_2_1) {
		wipeOffset = TG_HOST_V1_2_2_1_BACKGROUND_WIPE_OFFSET;
		colorOffset = TG_HOST_V1_2_2_1_BACKGROUND_COLOR_OFFSET;
	}

	if (_version >= kVersionV1_2_1 && READ_LE_UINT16(record + wipeOffset) != 0) {
		if (setBackgroundWithWipeEffect(record))
			return;
	}

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
			setSolidBackgroundColorPalette(READ_LE_UINT32(record + colorOffset));
		else
			setSolidBackgroundColorBitmap(READ_LE_UINT32(record + colorOffset));
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
		spriteLib()->spriteDibSet(_buffer1, rect.left, rect.top, (uint16)(rect.right - rect.left), (uint16)(rect.bottom - rect.top), 0);
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
	uint16 fadeColors = TG_HOST_V1_2_NUM_FADE_COLORS;
	if (_version >= kVersionV1_2_2_2)
		fadeColors = _maxFadeColors;
	else if (_version >= kVersionV1_2_1)
		fadeColors = (uint16)(TG_HOST_V1_2_1_FADE_LAST_COLOR - _maxTransColors);
	spriteLib()->spriteFadePalette(mult, _stepCount, TG_HOST_V1_2_NUM_SYSTEM_COLORS, fadeColors);

	_step++;
	if (_step > _stepCount)
		_active = false;
}

void TopgunEngine_v1_2::sceneTransitionIn(uint16 steps) {
	if (!_faded)
		return;

	uint16 sendMessage = 0;

	if (_version >= kVersionV1_2_2_2)
		sendMessage = spriteLib()->spriteSetSendMsg(0);

	_fadeIn = true;

	if (_version >= kVersionV1_2_7 && !display()->hasPalette())
		spriteLib()->spriteSetBkgTransparent(!_solidBackground || _backgroundColor != 0);

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

			if (_version >= kVersionV1_2_9) {
				_active = true;

				static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteBoxInBackground(_backgroundBitmap, rings, true);
			} else {
				if (rings < TG_HOST_V1_2_WIPE_MIN_RINGS)
					rings = TG_HOST_V1_2_WIPE_MIN_RINGS;

				_active = true;

				spriteLib()->spriteSetBackground(_buffer1, false, 0, nullptr);
				wipeEffectFrameRingsInsideOut(rings, true);
				spriteLib()->spriteSetBackground(_backgroundBitmap, false, 0, nullptr);
			}

			_faded = false;
			_buffer1 = utilsLibrary()->tgFree(_buffer1);
		}
	}

	if (_version >= kVersionV1_2_2_2)
		spriteLib()->spriteSetSendMsg(sendMessage);
}

void TopgunEngine_v1_2::sceneTransitionOut(uint16 steps) {
	if (_faded)
		return;

	uint16 sendMessage = 0;

	if (_version >= kVersionV1_2_2_2)
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

			if (_version >= kVersionV1_2_9) {
				_active = true;

				static_cast<SpriteLibrary_v1_2_3 *>(spriteLib())->spriteBoxOutBackground(_buffer1, rings, true);
			} else {
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
	}

	// The fade of a display without palette ends here even when there was no background to fade
	if (_version >= kVersionV1_2_9 && !display()->hasPalette())
		_faded = true;

	if (_version >= kVersionV1_2_2_2)
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

byte *TopgunEngine_v1_2_3::loadBitmapResourceCopy(uint16 id) {
	if (id == 0)
		return nullptr;

	uint16 cellType = kResV1_2_5BitmapGroup;
	if (_version >= kVersionV1_2_9)
		cellType = kResV1_2_9Cell;

	if (_version >= kVersionV1_2_6 && resourceLib()->resGetType(_file, operandValue(id)) == cellType) {
		byte *cell = loadResourceWithProcessingPause(id, nullptr);
		if (!cell)
			return nullptr;

		id = READ_LE_UINT16(cell);
		utilsLibrary()->tgFree(cell);
	}

	if (!validateResourceType(id, kResV1_2_3Bitmap))
		return nullptr;

	byte *copy = nullptr;

	if (isResourceObjectLoaded(id, kResV1_2_3Bitmap)) {
		uint16 index = operandValue(id);
		if (!isRecordIndexValid(index, "loadBitmapResourceCopy"))
			return nullptr;

		if (_version < kVersionV1_2_3) {
			if (_records[index].refCount != 0)
				return nullptr;

			copy = _records[index].data;
			_records[index].data = nullptr;
		} else {
			copy = utilsLibrary()->tgPtrCpy(_records[index].data);
		}
	} else {
		copy = loadResourceWithProcessingPause(id, nullptr);
	}

	uint16 flagsOffset = TG_RES_V1_2_2_1_BITMAP_FLAGS_BYTE_OFFSET;
	if (_version >= kVersionV1_2_5)
		flagsOffset = TG_RES_V1_2_5_BITMAP_FLAGS_BYTE_OFFSET;

	if (copy && (copy[flagsOffset] & TG_RES_V1_2_2_1_BITMAP_COMPRESSED_MASK) != 0) {
		byte *expanded = resourceLib()->resExpandBitmap(_file, copy);
		utilsLibrary()->tgFree(copy);
		copy = expanded;
	}

	return copy;
}

void TopgunEngine_v1_2_3::setBackgroundByMode(const byte *record) {
	if (_version >= kVersionV1_2_7)
		_solidBackground = false;

	uint16 mode = READ_LE_UINT16(record + 2);

	bool tileMode = false;

	if (_version >= kVersionV1_2_9) {
		if (mode == 3)
			tileMode = true;
		else if (mode == 0 && resourceLib()->resGetType(_file, operandValue(READ_LE_UINT16(record + 4))) != kResV1_2_9Bitmap)
			tileMode = true;
	}

	if (tileMode) {
		uint16 tileId = READ_LE_UINT16(record + 4);

		if (!validateAndGetResourceByType(tileId, kResV1_2_9Tile))
			return;

		uint16 index = operandValue(tileId);
		if (!isRecordIndexValid(index, "setBackgroundByMode"))
			return;

		if (!_tileBackgrounds.contains(index)) {
			warning("TopgunEngine_v1_2_3::setBackgroundByMode(): the tile map %d is not loaded", index);
			return;
		}

		setTileScreenRegion(true, _tileBackgrounds[index]);
		return;
	}

	switch (mode) {
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

void TopgunEngine_v1_2_3::setTileScreenRegion(bool repaint, TileBackground *tileMap) {
	if (tileMap && tileMap != _tileMap)
		clearBackground();

	_tileMap = tileMap;
	syncBackgroundDimensions(nullptr);
	_screenBox = _scrollBoxWindowBox;

	Common::Rect rect;

	applyBackgroundToSpriteLib(&rect);

	if (repaint)
		commitBackgroundChangeAndRepaint(rect);
}

void TopgunEngine_v1_2_3::setScrollBox(const Common::Rect &box) {
	spriteLib()->spriteSetScrollBox(&box);

	_scrollBoxWindowBox = (box.right > box.left) || (box.top < box.bottom);
	_screenBox = _scrollBoxWindowBox;
}

void TopgunEngine_v1_2_3::wipeEffectIrisRings(uint16 rings) {
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

void TopgunEngine_v1_2_3::wipeEffectInterlacedRows(uint16 duration, uint16 count) {
	uint16 stride = TG_HOST_V1_2_6_WIPE_ROWS_DEFAULT_COUNT;

	if (count != 0)
		stride = (_height >= count) ? count : _height;

	if (stride == 0 || _height == 0) {
		warning("TopgunEngine_v1_2_3::wipeEffectInterlacedRows(): the background has no rows");
		return;
	}

	uint16 step = (stride == 1) ? 1 : (uint16)(stride - 1);
	uint16 phase = 0;
	uint16 end = step;
	uint16 interval = (uint16)(duration / (uint16)(_height / stride + stride - 1));
	uint32 deadline = getEngineMillis();

	Common::Rect rect;
	rect.left = 0;
	rect.right = _width;

	while (phase < _height) {
		if (getEngineMillis() < deadline) {
			spriteLib()->spriteAnimate();
			presentScreenAndWait();
			continue;
		}

		deadline += interval;

		for (uint16 row = phase; row < end; row = (uint16)(row + step)) {
			rect.top = row;
			rect.bottom = (int16)(row + 1);
			copyAndPaintBackgroundRect(rect);
			spriteLib()->spriteAnimate();
		}

		presentScreenAndWait();

		if (phase < step)
			phase++;
		else
			phase = (uint16)(phase + stride);

		uint16 sum = (uint16)(stride + end);
		end = (sum < _height) ? sum : _height;
	}
}

void TopgunEngine_v1_2_3::wipeEffectSlideColumns(int16 direction, uint16 duration, uint16 steps) {
	uint16 count = TG_HOST_V1_2_6_WIPE_SLIDE_DEFAULT_STEPS;

	if (steps != 0)
		count = (_width >= steps) ? steps : _width;

	if (count == 0) {
		warning("TopgunEngine_v1_2_3::wipeEffectSlideColumns(): the background has no columns");
		return;
	}

	int16 stride = (int16)((_width / count) * direction);
	uint16 left = 0;
	int16 right = _width;

	if (stride < 0)
		left = (uint16)(_width + stride);
	else
		right = stride;

	uint16 interval = duration / count;
	uint32 deadline = getEngineMillis();

	Common::Rect rect;
	rect.top = 0;
	rect.bottom = _height;

	while (left < _width && right >= 0) {
		spriteLib()->spriteAnimate();

		if (getEngineMillis() >= deadline) {
			deadline += interval;
			rect.left = (int16)left;
			rect.right = right;
			copyAndPaintBackgroundRect(rect);
			left = (uint16)(left + stride);
			right = (int16)(right + stride);
		}

		presentScreenAndWait();
	}
}

void TopgunEngine_v1_2_3::wipeEffectSlideRows(int16 direction, uint16 duration, uint16 steps) {
	uint16 count = TG_HOST_V1_2_6_WIPE_SLIDE_DEFAULT_STEPS;

	if (steps != 0)
		count = (_height >= steps) ? steps : _height;

	if (count == 0) {
		warning("TopgunEngine_v1_2_3::wipeEffectSlideRows(): the background has no rows");
		return;
	}

	int16 stride = (int16)((_height / count) * direction);
	uint16 top = 0;
	int16 bottom = _height;

	if (stride < 0)
		top = (uint16)(_height + stride);
	else
		bottom = stride;

	uint16 interval = duration / count;
	uint32 deadline = getEngineMillis();

	Common::Rect rect;
	rect.left = 0;
	rect.right = _width;

	while (top < _height && bottom >= 0) {
		spriteLib()->spriteAnimate();

		if (getEngineMillis() >= deadline) {
			deadline += interval;
			rect.top = (int16)top;
			rect.bottom = bottom;
			copyAndPaintBackgroundRect(rect);
			top = (uint16)(top + stride);
			bottom = (int16)(bottom + stride);
		}

		presentScreenAndWait();
	}
}

void TopgunEngine_v1_2::executeWipeEffect(const byte *params, byte *newBitmap, byte *oldBitmap) {
	_wipeOldBitmap = oldBitmap;
	_wipeNewBitmap = newBitmap;
	_wipeEffectActive = true;

	uint16 sendMessage = 0;

	if (_version >= kVersionV1_2_2_2)
		sendMessage = spriteLib()->spriteSetSendMsg(0);

	switch (READ_LE_UINT16(params)) {
	case 1:
		if (_version >= kVersionV1_2_2_2)
			static_cast<TopgunEngine_v1_2_3 *>(this)->wipeEffectIrisRings(READ_LE_UINT16(params + 2));
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

	case 6:
		if (_version >= kVersionV1_2_6)
			static_cast<TopgunEngine_v1_2_3 *>(this)->wipeEffectInterlacedRows(READ_LE_UINT16(params + TG_HOST_V1_2_6_WIPE_PARAMETER_DURATION), READ_LE_UINT16(params + TG_HOST_V1_2_6_WIPE_PARAMETER_COUNT));

		break;

	case 7:
	case 8:
		if (_version >= kVersionV1_2_6)
			static_cast<TopgunEngine_v1_2_3 *>(this)->wipeEffectSlideColumns((READ_LE_UINT16(params) == 7) ? -1 : 1, READ_LE_UINT16(params + TG_HOST_V1_2_6_WIPE_PARAMETER_DURATION), READ_LE_UINT16(params + TG_HOST_V1_2_6_WIPE_PARAMETER_COUNT));

		break;

	case 9:
	case 10:
		if (_version >= kVersionV1_2_6)
			static_cast<TopgunEngine_v1_2_3 *>(this)->wipeEffectSlideRows((READ_LE_UINT16(params) == 9) ? -1 : 1, READ_LE_UINT16(params + TG_HOST_V1_2_6_WIPE_PARAMETER_DURATION), READ_LE_UINT16(params + TG_HOST_V1_2_6_WIPE_PARAMETER_COUNT));

		break;

	default:
		break;
	}

	if (_version >= kVersionV1_2_2_2)
		spriteLib()->spriteSetSendMsg(sendMessage);

	_wipeEffectActive = false;
}

} // End of namespace Topgun
