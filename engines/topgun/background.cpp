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
#include "topgun/libraries/utils.h"
#include "topgun/topgun_v0.h"

namespace Topgun {

enum {
	kFadeStepCount = 100,
	kFirstFadeColor = 10,
	kNumFadeColors = 200,
	kNumRings = 20,
	kNumTileColumns = 20
};

void TopgunEngine_v0::syncBackgroundDimensions(uint16 bitmapId) {
	uint16 screenWidth = display()->width();
	uint16 screenHeight = display()->height();

	if (_flagMode640 || _flagLoMem) {
		_width = Display::kDefaultWidth;
		_height = Display::kDefaultHeight;
	} else {
		_width = Display::kLargeWidth;
		_height = Display::kLargeHeight;
	}

	if (bitmapId == 0)
		return;

	if (!_records || bitmapId >= _numResources || !_records[bitmapId].data) {
		warning("TopgunEngine_v0::syncBackgroundDimensions(): record index out of range");
		return;
	}

	const byte *bitmap = _records[bitmapId].data;
	_width = READ_LE_UINT16(bitmap + SpriteLibrary::kBitmapWidthOffset);
	_height = READ_LE_UINT16(bitmap + SpriteLibrary::kBitmapHeightOffset);
	_screenBox = (screenWidth > _width) || (_height < screenHeight);
}

void TopgunEngine_v0::commitBackgroundChangeAndRepaint(const Common::Rect &rect) {
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

void TopgunEngine_v0::applyBackgroundToSpriteLib(uint16 bitmapId, Common::Rect *rectOut) {
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
			warning("TopgunEngine_v0::applyBackgroundToSpriteLib(): record index out of range");
			return;
		}

		spriteLibrary()->spriteSetBackground(_records[bitmapId].data, rectOut);
	}
}

void TopgunEngine_v0::clearBackground() {
	if (_backgroundId == 0)
		return;

	if (!_records || _backgroundId >= _numResources) {
		warning("TopgunEngine_v0::clearBackground(): record index out of range");
		return;
	}

	_buffer2 = _records[_backgroundId].data;
	_records[_backgroundId].data = nullptr;
	_backgroundId = 0;
}

bool TopgunEngine_v0::setBackgroundFromBitmapResource(uint16 bitmapId) {
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

void TopgunEngine_v0::copyAndPaintBackgroundRect(Common::Rect &rect) {
	if (_fadeIn)
		spriteLibrary()->spriteCopyRect(rect, _buffer2, _buffer1);
	else
		spriteLibrary()->spriteCopyRect(rect, _buffer1, _buffer2);

	spriteLibrary()->spritePaintRect(rect, true);
}

void TopgunEngine_v0::initDissolve() {
	for (uint16 i = 0; i < kNumDissolveTiles; i++)
		_tileOrder[i] = i;

	_stepCount = kNumDissolveTiles;
	_tileSize = _width / kNumTileColumns;
}

void TopgunEngine_v0::dissolveOneTile() {
	if (_stepCount == 0) {
		warning("TopgunEngine_v0::dissolveOneTile(): there are no tiles left");
		_active = false;
		return;
	}

	uint16 index = getRandom15Bit() % _stepCount;
	uint16 tile = _tileOrder[index];
	int16 left = (int16)((tile % kNumTileColumns) * _tileSize);
	int16 top = (int16)((tile / kNumTileColumns) * _tileSize);

	Common::Rect rect;
	rect.setRect(left, top, (int16)(left + _tileSize), (int16)(top + _tileSize));
	copyAndPaintBackgroundRect(rect);

	_stepCount--;
	_active = (_stepCount != 0);
	memmove(&_tileOrder[index], &_tileOrder[index + 1], (_stepCount - index) * sizeof(uint16));
}

void TopgunEngine_v0::computeInsetRingRect(uint16 k, Common::Rect *out, uint16 n) {
	// The divisor is 2 * n - 1, which is 39 for the 20 rings: this is the exact number the game expects
	uint32 divisor = (uint16)(2 * n - 1);

	out->left = (int16)((uint32)_width * k / divisor);
	out->right = (int16)(_width - out->left);
	out->top = (int16)((uint32)_height * k / divisor);
	out->bottom = (int16)(_height - out->top);
}

void TopgunEngine_v0::paintFrameBandsTopBottom(const Common::Rect &outer, const Common::Rect &inner) {
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

void TopgunEngine_v0::wipeEffectFrameRingsInsideOut() {
	Common::Rect inner;
	Common::Rect outer;

	computeInsetRingRect(kNumRings - 1, &inner, kNumRings);

	Common::Rect band = inner;
	band.left = 0;
	band.right = (int16)_width;
	copyAndPaintBackgroundRect(band);
	presentScreenAndWait();

	for (int16 k = kNumRings - 2; k >= 0; k--) {
		computeInsetRingRect(k, &outer, kNumRings);
		paintFrameBandsTopBottom(outer, inner);
		inner = outer;
	}
}

void TopgunEngine_v0::wipeEffectFrameRingsOutsideIn() {
	Common::Rect outer;
	Common::Rect inner;

	computeInsetRingRect(0, &outer, kNumRings);

	for (uint16 k = 1; k < kNumRings; k++) {
		computeInsetRingRect(k, &inner, kNumRings);
		paintFrameBandsTopBottom(outer, inner);
		outer = inner;
	}

	inner.left = 0;
	inner.right = (int16)_width;
	copyAndPaintBackgroundRect(inner);
	presentScreenAndWait();
}

void TopgunEngine_v0::advanceSceneFadeStep() {
	if (display()->hasPalette()) {
		uint16 mult = _fadeIn ? _step : (uint16)(_stepCount - _step);
		spriteLibrary()->spriteFadePalette(mult, _stepCount, kFirstFadeColor, kNumFadeColors);

		_step++;
		if (_step > _stepCount)
			_active = false;
	} else if (_buffer1) {
		dissolveOneTile();
	}
}

void TopgunEngine_v0::sceneTransitionIn() {
	_fadeIn = true;

	if (display()->hasPalette()) {
		_step = 1;
		_stepCount = kFadeStepCount;
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
			warning("TopgunEngine_v0::sceneTransitionIn(): record index out of range");
		else
			_buffer2 = _records[_backgroundId].data;
	}

	spriteLibrary()->spriteSetBackground(_buffer1, nullptr);
	wipeEffectFrameRingsInsideOut();
	_faded = false;
	spriteLibrary()->spriteSetBackground(_buffer2, nullptr);
	_buffer1 = utilsLibrary()->tgFree(_buffer1);
}

void TopgunEngine_v0::sceneTransitionOut() {
	_fadeIn = false;

	if (display()->hasPalette()) {
		_step = 2;
		_stepCount = kFadeStepCount;
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

} // End of namespace Topgun
