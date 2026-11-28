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
#include "topgun/libraries/sprite/sprite_v1_2_4.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_4.h"

namespace Topgun {

SpriteLibrary_v1_2::SpriteLibrary_v1_2(TopgunEngine_v1_2 *engine) : SpriteLibrary(engine) {
	memset(_eventRecord, 0, sizeof(_eventRecord));
}

SpriteLibrary_v1_2::~SpriteLibrary_v1_2() {
}

TopgunEngine_v1_2 *SpriteLibrary_v1_2::engine() const {
	return static_cast<TopgunEngine_v1_2 *>(_engine);
}

SpriteLibrary_v1_2::SpriteObject_v1_2 *SpriteLibrary_v1_2::findSprite(uint16 sprite, const char *caller) {
	return static_cast<SpriteObject_v1_2 *>(SpriteLibrary::findSprite(sprite, caller));
}

SpriteLibrary_v1_2::SpriteObject_v1_2 *SpriteLibrary_v1_2::lookupSprite(uint16 sprite) {
	if (sprite == 0 || !_sprites.contains(sprite))
		return nullptr;

	return static_cast<SpriteObject_v1_2 *>(_sprites[sprite]);
}

uint16 SpriteLibrary_v1_2::flipFlags(const SpriteObject_v1_2 *object) const {
	return (uint16)((object->mirrorX ? 1 : 0) | (object->flipY ? 0x100 : 0));
}

uint16 SpriteLibrary_v1_2::currentTickSprite() const {
	if (_tickCursor < 0 || _tickCursor >= TG_SPR_TABLE_SIZE)
		return 0;

	return _spriteTable[_tickCursor];
}

void SpriteLibrary_v1_2::setSpriteActive(uint16 sprite, uint16 mode, bool refresh) {
	SpriteObject_v1_2 *object = findSprite(sprite, "setSpriteActive");
	if (!object)
		return;

	if (object->active == mode)
		return;

	if (_engine->_version >= kVersionV1_2_12)
		object->active = (byte)(mode & 1);
	else
		object->active = (byte)mode;

	if (object->active != 0) {
		object->x = object->shadowX;
		object->y = object->shadowY;
		object->mirrorX = object->shadowMirrorX;
		object->flipY = object->shadowFlipY;

		if (refresh) {
			if (object->partCount != 0)
				recomputeCelBoxArray(sprite);
			else
				recomputeSpriteBoundingBox(sprite);

			renderSpriteToSurface(sprite);
		}
	} else {
		object->shadowX = object->x;
		object->shadowY = object->y;
		object->shadowMirrorX = object->mirrorX;
		object->shadowFlipY = object->flipY;
	}
}

void SpriteLibrary_v1_2::appendFrame(SpriteObject_v1_2 *object, const CelReference &cel) {
	object->frames.push_back(cel.bitmap);
	object->frameIds.push_back(cel.id);
}

uint16 SpriteLibrary_v1_2::decodeOperand(uint16 raw) const {
	if (!_variableTable)
		return raw;

	if (_engine->_version >= kVersionV1_2_3_2) {
		if (raw > TG_SPR_V1_2_3_2_VARIABLE_LAST_INDEX)
			return (uint16)(raw + TG_SPR_V1_2_3_2_LITERAL_BIAS);
	} else if (raw > TG_SPR_V1_2_VARIABLE_LAST_INDEX) {
		return (uint16)(raw + TG_SPR_V1_2_LITERAL_BIAS);
	}

	return _variableTable[raw];
}

uint16 SpriteLibrary_v1_2::spriteObjectAlloc(int16 level) {
	uint16 maxSprites = TG_SPR_V1_2_MAX_SPRITES;
	if (_engine->_version >= kVersionV1_2_12)
		maxSprites = TG_SPR_V1_2_12_MAX_SPRITES;
	else if (_engine->_version >= kVersionV1_2_4)
		maxSprites = TG_SPR_V1_2_4_MAX_SPRITES;
	else if (_engine->_version == kVersionV1_2_3_3)
		maxSprites = TG_SPR_V1_2_3_3_MAX_SPRITES;

	if (_spriteCount >= maxSprites) {
		warning("SpriteLibrary_v1_2::spriteObjectAlloc(): the sprite table is full");
		return 0;
	}

	SpriteObject_v1_2 *object = new SpriteObject_v1_2();
	uint16 handle = registerSprite(object);
	if (handle == 0) {
		delete object;
		return 0;
	}

	object->level = level;

	int slot = 0;
	if (_spriteCount != 0) {
		int index = (int)_spriteCount - 1;
		if (_topmostSprite != 0)
			index--;

		while (index >= 0) {
			SpriteObject_v1_2 *other = lookupSprite(_spriteTable[index]);
			if (other && other->level <= level)
				break;

			index--;
		}

		slot = index + 1;
	}

	uint16 moveCount = _spriteCount - slot;
	if (moveCount != 0) {
		if (_tickNesting != 0 && _tickCursor >= slot)
			_tickCursor++;

		memmove(&_spriteTable[slot + 1], &_spriteTable[slot], moveCount * sizeof(uint16));
	}

	_spriteTable[slot] = handle;
	_spriteCount++;
	return handle;
}

void SpriteLibrary_v1_2::freeDistinctCelBitmaps(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "freeDistinctCelBitmaps");
	if (!object)
		return;

	for (uint16 i = 0; i < object->frames.size(); i++) {
		bool distinct = true;
		for (uint16 j = 0; j < i; j++) {
			if (object->frames[j] == object->frames[i] && object->frameIds[j] == object->frameIds[i]) {
				distinct = false;
				break;
			}
		}

		if (distinct && object->frames[i])
			engine()->utilsLibrary()->tgFree(object->frames[i]);
	}

	if (object->celCacheCount != 0)
		releaseAllCachedBitmaps(sprite);
}

bool SpriteLibrary_v1_2::initSpriteFromFirstCel(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "initSpriteFromFirstCel");
	if (!object)
		return false;

	if (object->frames.empty()) {
		warning("SpriteLibrary_v1_2::initSpriteFromFirstCel(): the sprite has no frames");
		return false;
	}

	byte *frame = object->frames[0];
	if (!frame) {
		frame = getCelBitmap(object->frameIds[0], sprite);

		object = findSprite(sprite, "initSpriteFromFirstCel");
		if (!object)
			return false;
	}

	if (!frame)
		return false;

	object->currentFrame = frame;

	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 hotX = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET);
	int16 hotY = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET);
	int16 heightMinusOne = (int16)(height - 1);

	if (_engine->_version >= kVersionV1_2_2 && _engine->_version <= kVersionV1_2_3) {
		if (object->anchorX)
			hotX = (int16)(object->pivotX - hotX);

		if (object->anchorY)
			hotY = (int16)(object->pivotY - hotY);
	}

	object->boundingBox.left = (int16)((int16)(1 - width) / 2 + hotX + object->x);
	object->boundingBox.right = (int16)(width + object->boundingBox.left);
	object->boundingBox.top = (int16)(object->y + hotY - heightMinusOne / 2);
	object->boundingBox.bottom = (int16)(height + object->boundingBox.top);

	if (_engine->_version >= kVersionV1_2_12 && object->scrollable)
		object->boundingBox.translate(_scrollPosX, _scrollPosY);

	return true;
}

uint16 SpriteLibrary_v1_2::spriteObjectSetCelRange(uint16 sprite, uint16 startFrame, uint16 first, uint16 last) {
	if (_engine->_version >= kVersionV1_2_3) {
		uint16 result = SpriteLibrary::spriteObjectSetCelRange(sprite, startFrame, first, last);
		if (result == 0)
			return 0;

		SpriteObject_v1_2 *object = findSprite(sprite, "spriteObjectSetCelRange");
		if (object)
			object->animates = (last != first);

		return result;
	}

	if (first == 0)
		return 1;

	uint16 decodedStart = decodeOperand(startFrame);
	uint16 decodedFirst = decodeOperand(first);
	uint16 decodedLast = decodeOperand(last);

	uint16 result = SpriteLibrary::spriteObjectSetCelRange(sprite, decodedStart, decodedFirst, decodedLast);
	if (result == 0)
		return 0;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteObjectSetCelRange");
	if (object)
		object->animates = (object->rangeEnd != object->rangeStart);

	return result;
}

void SpriteLibrary_v1_2::recomputeSpriteBoundingBox(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "recomputeSpriteBoundingBox");
	if (!object)
		return;

	if (_engine->_version >= kVersionV1_2_7 && !object->active)
		return;

	byte *frame = object->currentFrame;
	if (!frame) {
		warning("SpriteLibrary_v1_2::recomputeSpriteBoundingBox(): the sprite has no frame");
		return;
	}

	int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 widthMinusOne = (int16)(width - 1);
	int16 heightMinusOne = (int16)(height - 1);

	Common::Rect oldBox = object->boundingBox;

	int16 hotX = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET);
	int16 hotY = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET);

	if (_engine->_version >= kVersionV1_2_3_2) {
		if (object->mirrorX)
			hotX = (int16)-hotX;

		if (object->flipY)
			hotY = (int16)-hotY;
	} else if (_engine->_version >= kVersionV1_2_2 && _engine->_version <= kVersionV1_2_3) {
		if (object->anchorX)
			hotX = (int16)(object->pivotX - hotX);

		if (object->anchorY)
			hotY = (int16)(object->pivotY - hotY);
	}

	object->boundingBox.left = (int16)(object->x + hotX - widthMinusOne / 2);
	object->boundingBox.right = (int16)(width + object->boundingBox.left);
	object->boundingBox.top = (int16)(object->y + hotY - heightMinusOne / 2);
	object->boundingBox.bottom = (int16)(height + object->boundingBox.top);

	if (_engine->_version >= kVersionV1_2_12 && object->scrollable)
		object->boundingBox.translate(_scrollPosX, _scrollPosY);

	object->useUnionRefresh = intersectRectLocal(oldBox, object->boundingBox, oldBox);
}

void SpriteLibrary_v1_2::recomputeCelBoxArray(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "recomputeCelBoxArray");
	if (!object || object->partCount == 0)
		return;

	if (_engine->_version >= kVersionV1_2_7 && !object->active)
		return;

	if (!object->parts) {
		warning("SpriteLibrary_v1_2::recomputeCelBoxArray(): the multipart sprite has no parts");
		return;
	}

	for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		SpritePart &part = object->parts->parts[i];
		byte *frame = part.frame;
		if (!frame) {
			warning("SpriteLibrary_v1_2::recomputeCelBoxArray(): the sprite part has no frame");
			continue;
		}

		int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
		int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
		int16 heightMinusOne = (int16)(height - 1);
		int16 hotX = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET);
		int16 hotY = READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET);

		// The anchors are applied before the mirroring
		if (_engine->_version == kVersionV1_2_2) {
			if (object->anchorX)
				hotX = (int16)(object->pivotX - hotX);

			if (object->anchorY)
				hotY = (int16)(object->pivotY - hotY);
		}

		if (_engine->_version >= kVersionV1_2_1 && _engine->_version != kVersionV1_2_3 && object->mirrorX)
			hotX = (int16)-hotX;

		if (_engine->_version >= kVersionV1_2_1 && _engine->_version != kVersionV1_2_3 && object->flipY)
			hotY = (int16)-hotY;

		part.box.left = (int16)((int16)(1 - width) / 2 + hotX + object->x);
		part.box.right = (int16)(width + part.box.left);
		part.box.top = (int16)(hotY + object->y - heightMinusOne / 2);
		part.box.bottom = (int16)(height + part.box.top);

		if (_engine->_version >= kVersionV1_2_12 && object->scrollable)
			part.box.translate(_scrollPosX, _scrollPosY);

		if (i == 0)
			object->boundingBox = part.box;
		else
			unionRectLocal(object->boundingBox, part.box, object->boundingBox);
	}

	if (_engine->_version != kVersionV1_2_3 || (!object->mirrorX && !object->flipY && !object->anchorX && !object->anchorY))
		return;

	int16 sumX = (int16)(object->boundingBox.left + object->boundingBox.right);
	int16 sumY = (int16)(object->boundingBox.top + object->boundingBox.bottom);
	int16 shiftX = 0;
	int16 shiftY = 0;

	if (object->anchorX)
		shiftX = (int16)(object->x * 2 + object->pivotX - sumX);

	if (object->anchorY)
		shiftY = (int16)(object->y * 2 + object->pivotY - sumY);

	for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		SpritePart &part = object->parts->parts[i];
		if (!part.frame)
			continue;

		int16 width = (int16)READ_LE_UINT16(part.frame + TG_SPR_BITMAP_WIDTH_OFFSET);
		int16 height = (int16)READ_LE_UINT16(part.frame + TG_SPR_BITMAP_HEIGHT_OFFSET);

		if (object->mirrorX)
			part.box.left = (int16)(shiftX - part.box.right + sumX);
		else if (object->anchorX)
			part.box.left = (int16)(part.box.left + shiftX);

		part.box.right = (int16)(width + part.box.left);

		if (object->flipY)
			part.box.top = (int16)(shiftY - part.box.bottom + sumY);
		else if (object->anchorY)
			part.box.top = (int16)(part.box.top + shiftY);

		part.box.bottom = (int16)(height + part.box.top);
	}

	if (object->anchorX) {
		object->boundingBox.left = (int16)(object->boundingBox.left + shiftX);
		object->boundingBox.right = (int16)(object->boundingBox.right + shiftX);
	}

	if (object->anchorY) {
		object->boundingBox.top = (int16)(object->boundingBox.top + shiftY);
		object->boundingBox.bottom = (int16)(object->boundingBox.bottom + shiftY);
	}
}

void SpriteLibrary_v1_2::updateSpritePosition(uint16 sprite, int16 x, int16 y, uint16 relative, bool refresh) {
	SpriteObject_v1_2 *object = findSprite(sprite, "updateSpritePosition");
	if (!object)
		return;

	Common::Rect oldBox = object->boundingBox;

	if (_engine->_version >= kVersionV1_2_1 && object->framePending && (_engine->_version < kVersionV1_2_7 || object->active)) {
		applyFrameChange(sprite);

		object = findSprite(sprite, "updateSpritePosition");
		if (!object)
			return;
	}

	bool shadow = (_engine->_version >= kVersionV1_2_7 && !object->active);
	int16 positionX = object->x;
	int16 positionY = object->y;
	if (shadow) {
		positionX = object->shadowX;
		positionY = object->shadowY;
	}

	if (relative != 0) {
		x = (int16)(x + positionX);
		y = (int16)(y + positionY);
	}

	int16 deltaX = (int16)(x - positionX);
	int16 deltaY = (int16)(y - positionY);

	if (shadow) {
		object->shadowX = x;
		object->shadowY = y;
		return;
	}

	object->x = x;
	object->y = y;

	object->boundingBox.left = (int16)(object->boundingBox.left + deltaX);
	object->boundingBox.top = (int16)(object->boundingBox.top + deltaY);
	object->boundingBox.right = (int16)(object->boundingBox.right + deltaX);
	object->boundingBox.bottom = (int16)(object->boundingBox.bottom + deltaY);

	if (_engine->_version >= kVersionV1_2_1) {
		if (_engine->_version >= kVersionV1_2_4)
			object->multipartValid = false;

		object->useUnionRefresh = intersectRectLocal(oldBox, object->boundingBox, oldBox);
	} else {
		object->useUnionRefresh = (deltaX > -TG_SPR_V1_2_UNION_REFRESH_LIMIT && deltaX < TG_SPR_V1_2_UNION_REFRESH_LIMIT &&
		                           deltaY > -TG_SPR_V1_2_UNION_REFRESH_LIMIT && deltaY < TG_SPR_V1_2_UNION_REFRESH_LIMIT);
	}

	if (object->parts) {
		for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++)
			object->parts->parts[i].box.translate(deltaX, deltaY);
	}

	if (refresh)
		renderSpriteToSurface(sprite);
}

bool SpriteLibrary_v1_2::spriteInitLibrary(uint16 mode) {
	if (mode > 4)
		mode = 0;

	_directMode = (mode != 0);
	_tickNesting = 0;
	_bkgTransparent = false;
	_debugProcSet = false;
	_messageProcSet = false;
	_movieProcSet = false;
	_streamProcSet = false;
	_queueDoneProcSet = false;
	_serviceProcSet = false;
	_serviceWord = 0;
	_variableTable = nullptr;

	if (_engine->_version >= kVersionV1_2_3_2)
		_literalBias = TG_SPR_V1_2_3_2_LITERAL_BIAS;
	else
		_literalBias = TG_SPR_V1_2_LITERAL_BIAS;
	_inHostCallback = false;
	_hostEventsSuppressed = false;
	_zoomActive = false;
	_stretch = false;
	_zoomFactor = 0;
	_zoomOffsetX = 0;
	_zoomOffsetY = 0;
	_rubberBandActive = false;
	_userClipBox = Common::Rect();
	_fillColor = 0;
	_composeBuffer = nullptr;
	_composeBufferCapacity = 0;
	_backgroundBitmap = nullptr;
	_topmostSprite = 0;
	_tickCursor = 0;
	_spriteCount = 0;
	_clipRect = Common::Rect();
	_scrollX = 0;
	_scrollY = 0;
	_scrollPosX = 0;
	_scrollPosY = 0;
	_scrollBox = Common::Rect();
	_scrollSurface = nullptr;
	_tileBackground = nullptr;
	_hasLogicalPalette = false;
	_dibColorUsage = 0;
	memset(_paletteCopy, 0, sizeof(_paletteCopy));
	memset(_colorTable, 0, sizeof(_colorTable));

	if (!createComposeBuffer())
		return false;

	_initialized = true;

	Display *display = engine()->display();
	int16 screenWidth = display->width();
	int16 screenHeight = display->height();

	display->setViewportOrigin(screenWidth / 2, screenHeight / 2);

	if (_engine->_version >= kVersionV1_2_12) {
		_screenRect.left = (int16)((1 - screenWidth) / 2);
		_screenRect.top = (int16)((1 - screenHeight) / 2);
		_screenRect.right = (int16)(_screenRect.left + screenWidth);
		_screenRect.bottom = (int16)(_screenRect.top + screenHeight);
		_scrollRegion = _screenRect;
		spriteSetBackground(nullptr, false, 0, nullptr);
	} else {
		_clipRect.right = screenWidth;
		_clipRect.bottom = screenHeight;
		_scrollX = (screenWidth - 1) / 2;
		_scrollY = (screenHeight - 1) / 2;
		_clipRect.translate(-_scrollX, -_scrollY);
	}

	initQueueRecordSizeTable();
	return true;
}

bool SpriteLibrary_v1_2::createComposeBuffer() {
	_composeBuffer = spriteDibCreate(TG_SPR_COMPOSE_BUFFER_WIDTH, TG_SPR_COMPOSE_BUFFER_HEIGHT, 0);
	if (!_composeBuffer)
		return false;

	_composeBufferCapacity = (uint32)READ_LE_UINT16(_composeBuffer + TG_SPR_V1_2_BITMAP_PITCH_OFFSET) * READ_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_HEIGHT_OFFSET) + TG_SPR_V1_2_BITMAP_HEADER_SIZE;
	return true;
}

void SpriteLibrary_v1_2::computeClipRect() {
	if (_engine->_version >= kVersionV1_2_12) {
		_scrollPosX = 0;
		_scrollPosY = 0;
	}

	_clipRect.top = 0;
	_clipRect.left = 0;

	if (_backgroundBitmap) {
		_clipRect.right = (int16)READ_LE_UINT16(_backgroundBitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
		_clipRect.bottom = (int16)READ_LE_UINT16(_backgroundBitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	} else {
		_clipRect.right = engine()->display()->width();
		_clipRect.bottom = engine()->display()->height();
	}

	_scrollX = (_clipRect.right - 1) / 2;
	_scrollY = (_clipRect.bottom - 1) / 2;
	_clipRect.translate(-_scrollX, -_scrollY);

	if (_engine->_version >= kVersionV1_2_12)
		_backupRect = _clipRect;

	bool userBoxSet = (_userClipBox.left < _userClipBox.right);
	if (_engine->_version < kVersionV1_2_12)
		userBoxSet = (userBoxSet && _userClipBox.top < _userClipBox.bottom);

	if (userBoxSet) {
		Common::Rect intersection;
		intersectRects(_clipRect, _userClipBox, intersection);
		_clipRect = intersection;
	}

	if (_engine->_version >= kVersionV1_2_12)
		setScrollRegion(_scrollBox);
}

void SpriteLibrary_v1_2::setScrollRegion(const Common::Rect &box) {
	if (box.right > box.left && box.bottom > box.top) {
		intersectRectLocal(_screenRect, box, _scrollRegion);
		intersectRectLocal(_scrollRegion, _clipRect, _scrollRegion);
	} else {
		_scrollRegion = _screenRect;
	}

	if (_tileBackground)
		intersectRectLocal(_scrollRegion, _tileBackground->bounds, _scrollRegion);
	else if (_backgroundBitmap)
		intersectRectLocal(_scrollRegion, _backupRect, _scrollRegion);
}

void SpriteLibrary_v1_2::initQueueRecordSizeTable() {
	for (int i = 0; i < TG_SPR_NUM_QUEUE_COMMANDS; i++)
		_queueCommandSizeTable[i] = 0;

	_queueCommandSizeTable[kSprV1_2OpPlayRange] = 0x0A;
	_queueCommandSizeTable[kSprV1_2OpMultiCel] = (_engine->_version >= kVersionV1_2_6) ? 0x10 : 0x0E;
	_queueCommandSizeTable[kSprV1_2OpReserved3] = 2;
	_queueCommandSizeTable[kSprV1_2OpCompositeBackground] = 2;
	_queueCommandSizeTable[kSprV1_2OpLoop] = 8;
	_queueCommandSizeTable[kSprV1_2OpHide] = 2;
	_queueCommandSizeTable[kSprV1_2OpLinearMove] = (_engine->_version >= kVersionV1_2_6) ? 0x18 : 0x16;
	_queueCommandSizeTable[kSprV1_2OpFrameInterval] = 4;
	_queueCommandSizeTable[kSprV1_2OpSetRange] = 8;
	_queueCommandSizeTable[kSprV1_2OpMoveInterval] = 6;
	_queueCommandSizeTable[kSprV1_2OpShowFrame] = 4;
	_queueCommandSizeTable[kSprV1_2OpReserved18] = 4;
	_queueCommandSizeTable[kSprV1_2OpHostEvent] = 2;
	_queueCommandSizeTable[kSprV1_2OpHostEventPayload] = 4;

	if (_engine->_version >= kVersionV1_2_3_1) {
		_queueCommandSizeTable[kSprV1_2OpQuadPath] = 0x1E;
		_queueCommandSizeTable[kSprV1_2OpFlip] = 0x0A;
		_queueCommandSizeTable[kSprV1_2OpDelayedMove] = 8;
		_queueCommandSizeTable[kSprV1_2OpWait] = 6;
		_queueCommandSizeTable[kSprV1_2OpSetPosition] = 8;
		_queueCommandSizeTable[kSprV1_2OpTimingMode] = 4;
		_queueCommandSizeTable[kSprV1_2_4OpHostEventFixed] = (_engine->_version >= kVersionV1_2_6) ? 0x12 : 0x0E;
		_queueCommandSizeTable[kSprV1_2_4OpMovieFrame] = 0x0C;
	} else {
		_queueCommandSizeTable[kSprV1_2OpQuadPath] = 0x1D;
		_queueCommandSizeTable[kSprV1_2OpFlip] = (_engine->_version >= kVersionV1_2_3) ? TG_SPR_V1_2_3_FLIP_RECORD_SIZE : 6;
		_queueCommandSizeTable[kSprV1_2OpDelayedMove] = 7;
		_queueCommandSizeTable[kSprV1_2OpWait] = 5;
		_queueCommandSizeTable[kSprV1_2OpSetPosition] = 7;
		_queueCommandSizeTable[kSprV1_2OpTimingMode] = 3;
		_queueCommandSizeTable[kSprV1_2OpMovieFrame] = 0x0B;
	}

	if (_engine->_version == kVersionV1_2_2) {
		_queueCommandSizeTable[kSprV1_2_2OpSetAnchor] = 8;
		_queueCommandSizeTable[kSprV1_2_2OpReserved19] = 4;
		_queueCommandSizeTable[kSprV1_2_2OpHostEvent] = 2;
		_queueCommandSizeTable[kSprV1_2_2OpHostEventPayload] = 4;
		_queueCommandSizeTable[kSprV1_2_2OpMovieFrame] = 0x0B;
	}

	if (_engine->_version >= kVersionV1_2_7) {
		_queueCommandSizeTable[kSprV1_2_7OpSetActive] = 4;
		_queueCommandSizeTable[kSprV1_2_7OpFrameInterval] = 4;
		_queueCommandSizeTable[kSprV1_2_7OpSetRange] = 8;
		_queueCommandSizeTable[kSprV1_2_7OpMoveInterval] = 6;
		_queueCommandSizeTable[kSprV1_2_7OpShowFrame] = 4;
		_queueCommandSizeTable[kSprV1_2_7OpReserved19] = 4;
		_queueCommandSizeTable[kSprV1_2_7OpHostEvent] = 2;
		_queueCommandSizeTable[kSprV1_2_7OpHostEventPayload] = 4;
		_queueCommandSizeTable[kSprV1_2_7OpHostEventFixed] = 0x12;
		_queueCommandSizeTable[kSprV1_2_7OpMovieFrame] = 0x0C;
	}

	if (_engine->_version >= kVersionV1_2_7_1)
		_queueCommandSizeTable[kSprV1_2OpMultiCel] = 0x1C;
}

void SpriteLibrary_v1_2::spriteResetLibrary() {
	if (_engine->_version >= kVersionV1_2_3) {
		_zoomOffsetX = 0;
		_zoomOffsetY = 0;
		_zoomFactor = 1;
	}

	_zoomActive = false;

	if (_engine->_version >= kVersionV1_2_12) {
		_scrollPosX = 0;
		_scrollPosY = 0;
	}

	_userClipBox = Common::Rect();

	if (_engine->_version >= kVersionV1_2_12) {
		_scrollBox = Common::Rect();
		_scrollRegion = _screenRect;
		_backupRect = _screenRect;
	}

	_fillColor = 0;
	_backgroundBitmap = nullptr;

	if (_engine->_version >= kVersionV1_2_12) {
		_tileBackground = nullptr;

		if (_scrollSurface) {
			spriteDibFree(_scrollSurface);
			_scrollSurface = nullptr;
		}
	}

	if (_engine->_version >= kVersionV1_2_7_1 && _engine->_version <= kVersionV1_2_11)
		_rubberBandActive = false;

	if (_composeBuffer)
		spriteDibFree(_composeBuffer);

	createComposeBuffer();
}

void SpriteLibrary_v1_2::callServiceProc() {
	engine()->spriteServiceProc(_serviceWord);
}

uint16 SpriteLibrary_v1_2::callDebugProc(uint16 sprite) {
	if (!_debugProcSet)
		return 0;

	warning("spriteDebugService UNIMPLEMENTED");
	return 0;
}

uint16 SpriteLibrary_v1_2::spriteCreate(int16 level, const CelReference &cel) {
	uint16 sprite = spriteObjectAlloc(level);
	if (sprite == 0)
		return 0;

	if (_engine->_version >= kVersionV1_2_7) {
		SpriteObject_v1_2 *created = findSprite(sprite, "spriteCreate");
		if (created)
			created->active = 1;
	}

	if (!cel.bitmap && cel.id == 0)
		return sprite;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteCreate");
	if (!object)
		return 0;

	object->forward = true;
	appendFrame(object, cel);

	if (!initSpriteFromFirstCel(sprite)) {
		spriteDestroy(sprite, false, false);
		return 0;
	}

	return sprite;
}

uint16 SpriteLibrary_v1_2::spriteCopy(uint16 sprite) {
	SpriteObject_v1_2 *source = findSprite(sprite, "spriteCopy");
	if (!source)
		return 0;

	uint16 copy = spriteObjectAlloc(source->level);
	if (copy == 0)
		return 0;

	SpriteObject_v1_2 *object = findSprite(copy, "spriteCopy");
	if (!object)
		return 0;

	Common::Array<byte *> duplicate;
	if (!duplicateSpriteData(source->frames, duplicate)) {
		spriteDestroy(copy, false, false);
		return 0;
	}

	*object = *source;
	object->frames = duplicate;
	object->queueStart = nullptr;
	object->queueCurrent = nullptr;
	object->queueEnd = nullptr;

	if (source->queueStart) {
		if (!spriteSetQueue(copy, source->queueStart, (uint16)(source->queueEnd - source->queueStart), source->queueTag)) {
			spriteDestroy(copy, false, false);
			return 0;
		}
	}

	object = findSprite(copy, "spriteCopy");
	if (!object)
		return 0;

	object->visible = false;
	return copy;
}

uint16 SpriteLibrary_v1_2::spriteDestroy(uint16 sprite, bool hideRefresh, bool freeFrames) {
	int16 index = findSpriteIndex(sprite);
	if (index < 0)
		return 0;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteDestroy");
	if (!object)
		return 0;

	if (object->visible && hideRefresh)
		spriteHide(sprite, true);

	if (freeFrames)
		freeDistinctCelBitmaps(sprite);

	if (_tickNesting != 0 && _tickCursor >= index)
		_tickCursor--;

	cleanupSpriteData(sprite);

	for (uint16 i = index; i < _spriteCount - 1; i++)
		_spriteTable[i] = _spriteTable[i + 1];

	_spriteCount--;
	return 0;
}

bool SpriteLibrary_v1_2::spriteAppendBitmap(uint16 sprite, const CelReference &cel) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteAppendBitmap");
	if (!object)
		return false;

	uint16 count = object->frames.size();
	appendFrame(object, cel);

	if (_engine->_version >= kVersionV1_2_12)
		object->currentFrame = object->frames[0];

	if (count == 0) {
		if (!initSpriteFromFirstCel(sprite)) {
			object = findSprite(sprite, "spriteAppendBitmap");
			if (object) {
				object->frames.pop_back();
				object->frameIds.pop_back();
			}

			return false;
		}

		object = findSprite(sprite, "spriteAppendBitmap");
		if (!object)
			return false;

		object->rangeEnd = 1;
		object->forward = true;
	} else if (object->forward) {
		if ((uint16)(count - object->rangeEnd - 1) == 0)
			object->rangeEnd++;
	} else {
		if ((uint16)(count - object->rangeStart - 1) == 0)
			object->rangeStart++;
	}

	object->animates = (object->rangeStart != object->rangeEnd);
	return true;
}

bool SpriteLibrary_v1_2::spriteAppendBitmaps(uint16 sprite, const CelReference *cels, uint16 count) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteAppendBitmaps");
	if (!object)
		return false;

	uint16 oldCount = object->frames.size();
	if (count == 0 && oldCount == 0) {
		warning("SpriteLibrary_v1_2::spriteAppendBitmaps(): there are no frames to append to a sprite without frames");
		return true;
	}

	if (_engine->_version >= kVersionV1_2_12) {
		if (oldCount != 0)
			object->currentFrame = object->frames[0];
		else
			object->currentFrame = nullptr;
	}

	for (uint16 i = 0; i < count; i++)
		appendFrame(object, cels[i]);

	if (oldCount == 0) {
		if (!initSpriteFromFirstCel(sprite)) {
			object = findSprite(sprite, "spriteAppendBitmaps");
			if (object) {
				for (uint16 i = 0; i < count; i++) {
					object->frames.pop_back();
					object->frameIds.pop_back();
				}
			}

			return false;
		}

		object = findSprite(sprite, "spriteAppendBitmaps");
		if (!object)
			return false;

		object->rangeEnd = (uint16)(count - 1);
		object->forward = true;
	} else if (object->forward) {
		if ((uint16)(oldCount - object->rangeEnd - 1) == 0)
			object->rangeEnd += count;
	} else {
		if ((uint16)(oldCount - object->rangeStart - 1) == 0)
			object->rangeStart += count;
	}

	object->animates = (object->rangeStart != object->rangeEnd);
	return true;
}

void SpriteLibrary_v1_2::spriteReplaceBitmap(uint16 sprite, const CelReference &oldCel, const CelReference &newCel) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteReplaceBitmap");
	if (!object || object->frames.empty())
		return;

	bool replaced = false;
	for (uint16 i = 0; i < object->frames.size(); i++) {
		bool same;
		if (oldCel.bitmap)
			same = (object->frames[i] == oldCel.bitmap);
		else
			same = (!object->frames[i] && object->frameIds[i] == oldCel.id);

		if (same) {
			object->frames[i] = newCel.bitmap;
			object->frameIds[i] = newCel.id;
			replaced = true;
		}
	}

	if (!replaced)
		return;

	if (object->partCount != 0) {
		bool changed = false;
		if (object->parts) {
			for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
				if (oldCel.bitmap && object->parts->parts[i].frame == oldCel.bitmap) {
					object->parts->parts[i].frame = newCel.bitmap;
					changed = true;
				}
			}
		}

		if (!changed)
			return;

		recomputeCelBoxArray(sprite);
	} else {
		if (!oldCel.bitmap || object->currentFrame != oldCel.bitmap)
			return;

		object->currentFrame = newCel.bitmap;
		recomputeSpriteBoundingBox(sprite);
	}

	object = findSprite(sprite, "spriteReplaceBitmap");
	if (!object)
		return;

	object->framePending = false;

	if (_engine->_version >= kVersionV1_2_3_1)
		object->multipartValid = false;

	renderSpriteToSurface(sprite);
}

void SpriteLibrary_v1_2::spriteShow(uint16 sprite, uint16 frame, bool refresh) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteShow");
	if (!object)
		return;

	if (object->visible && object->currentFrameIndex == frame)
		return;

	if (_engine->_version >= kVersionV1_2_7)
		setSpriteActive(sprite, 1, false);

	object->visible = (object->frames.size() != 0);

	if (_engine->_version >= kVersionV1_2_3_1)
		object->multipartValid = false;

	if (object->frames.size() > frame) {
		object->nextFrameIndex = frame;
		object->framePending = true;
	} else if (_engine->_version >= kVersionV1_2_3_2 && !object->currentFrame) {
		object->nextFrameIndex = object->currentFrameIndex;
		object->framePending = true;
	}

	if (refresh)
		renderSpriteToSurface(sprite);
}

void SpriteLibrary_v1_2::spriteSetPosition(uint16 sprite, int16 x, int16 y, uint16 relative) {
	updateSpritePosition(sprite, x, y, relative);
}

void SpriteLibrary_v1_2::spriteOffset(uint16 sprite, int16 deltaX, int16 deltaY) {
	updateSpritePosition(sprite, deltaX, deltaY, 1);

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteOffset");
	if (!object || !object->queueCurrent)
		return;

	byte *command = object->queueCurrent;
	uint16 quadPathOffset = (_engine->_version >= kVersionV1_2_3_1) ? 0x12 : 0x11;
	uint16 linearMoveOffset = (_engine->_version >= kVersionV1_2_6) ? 0x10 : 0x0E;

	switch (READ_LE_UINT16(command)) {
	case kSprV1_2OpQuadPath:
		WRITE_LE_UINT16(command + quadPathOffset, (uint16)(READ_LE_UINT16(command + quadPathOffset) + deltaX));
		WRITE_LE_UINT16(command + quadPathOffset + 2, (uint16)(READ_LE_UINT16(command + quadPathOffset + 2) + deltaY));
		WRITE_LE_UINT16(command + quadPathOffset + 4, (uint16)(READ_LE_UINT16(command + quadPathOffset + 4) + deltaX));
		WRITE_LE_UINT16(command + quadPathOffset + 6, (uint16)(READ_LE_UINT16(command + quadPathOffset + 6) + deltaY));
		break;

	case kSprV1_2OpLinearMove:
		WRITE_LE_UINT16(command + linearMoveOffset, (uint16)(READ_LE_UINT16(command + linearMoveOffset) + deltaX));
		WRITE_LE_UINT16(command + linearMoveOffset + 2, (uint16)(READ_LE_UINT16(command + linearMoveOffset + 2) + deltaY));
		break;

	default:
		break;
	}
}

void SpriteLibrary_v1_2::spriteSetPriority(uint16 sprite, uint16 mode) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetPriority");
	if (!object)
		return;

	if (_engine->_version >= kVersionV1_2_12)
		object->timingMode = (byte)(mode & 1);
	else
		object->timingMode = (byte)mode;

	object->moveDue = 0;

	if (mode != 0)
		object->frameDue = engine()->getEngineMillis();
	else
		object->frameDue = 0;
}

void SpriteLibrary_v1_2::spriteSetPickMode(uint16 sprite, uint16 flag) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetPickMode");
	if (!object)
		return;

	object->pickWholeBox = ((flag & 0xFF) != 0);
}

void SpriteLibrary_v1_2::spriteSetTopMost(uint16 sprite) {
	int16 index = findSpriteIndex(sprite);
	if (sprite == 0)
		_topmostSprite = 0;

	if (index < 0)
		return;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetTopMost");
	if (!object)
		return;

	for (uint16 i = index; i < _spriteCount - 1; i++)
		_spriteTable[i] = _spriteTable[i + 1];

	object->level = (int16)TG_SPR_V1_2_LEVEL_TOPMOST;
	_spriteTable[_spriteCount - 1] = sprite;
	_topmostSprite = _spriteTable[_spriteCount - 1];

	bool framePending = object->framePending;
	object->framePending = false;
	renderSpriteToSurface(sprite);

	object = findSprite(sprite, "spriteSetTopMost");
	if (object)
		object->framePending = framePending;
}

void SpriteLibrary_v1_2::spriteSetLevel(uint16 sprite, int16 level) {
	int16 index = findSpriteIndex(sprite);
	if (index < 0)
		return;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetLevel");
	if (!object)
		return;

	if (_tickNesting != 0 && _tickCursor >= index)
		_tickCursor--;

	memmove(&_spriteTable[index], &_spriteTable[index + 1], (_spriteCount - index - 1) * sizeof(uint16));
	_spriteCount--;

	object->level = level;

	if (_spriteCount == 0) {
		_spriteTable[0] = sprite;
	} else {
		int slot = (int)_spriteCount - 1;
		if (_topmostSprite != 0)
			slot--;

		while (slot >= 0) {
			SpriteObject_v1_2 *other = lookupSprite(_spriteTable[slot]);
			if (other && other->level <= level)
				break;

			slot--;
		}

		slot++;

		uint16 moveCount = _spriteCount - slot;
		if (moveCount != 0) {
			if (_tickNesting != 0 && _tickCursor >= slot)
				_tickCursor++;

			memmove(&_spriteTable[slot + 1], &_spriteTable[slot], moveCount * sizeof(uint16));
		}

		_spriteTable[slot] = sprite;
	}

	_spriteCount++;
}

bool SpriteLibrary_v1_2::spriteSwapOrder(uint16 spriteA, uint16 spriteB, bool onlyIfBAbove, bool refresh) {
	int16 indexA = findSpriteIndex(spriteA);
	int16 indexB = findSpriteIndex(spriteB);
	if (indexA < 0 || indexB < 0)
		return false;

	SpriteObject_v1_2 *objectA = findSprite(spriteA, "spriteSwapOrder");
	SpriteObject_v1_2 *objectB = findSprite(spriteB, "spriteSwapOrder");
	if (!objectA || !objectB || objectA->level != objectB->level)
		return false;

	if (onlyIfBAbove && indexB <= indexA)
		return false;

	uint16 temp = _spriteTable[indexA];
	_spriteTable[indexA] = _spriteTable[indexB];
	_spriteTable[indexB] = temp;

	if (refresh) {
		renderSpriteToSurface(_spriteTable[indexB > indexA ? indexA : indexB]);
		renderSpriteToSurface(_spriteTable[indexB < indexA ? indexA : indexB]);
	}

	return true;
}

void SpriteLibrary_v1_2::spriteTransfer(uint16 source, uint16 destination) {
	SpriteObject_v1_2 *from = lookupSprite(source);
	SpriteObject_v1_2 *to = lookupSprite(destination);

	SpriteLibrary::spriteTransfer(source, destination);

	if (from && to) {
		from->celCacheCount = 0;

		if (_engine->_version >= kVersionV1_2_1)
			from->currentFrame = nullptr;
	}
}

void SpriteLibrary_v1_2::spriteEmptyQueue(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteEmptyQueue");
	if (!object)
		return;

	SpriteLibrary::spriteEmptyQueue(sprite);

	if (_engine->_version >= kVersionV1_2_7)
		object->active = 1;

	object->advancePending = false;
	object->movieFrame = nullptr;
}

void SpriteLibrary_v1_2::spriteGetInfo(uint16 sprite, SpriteInfo_v1_2 &info) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteGetInfo");
	if (!object)
		return;

	info.frames[0] = object->currentFrame;
	info.frameCount = object->currentFrame ? 1 : 0;

	if (object->partCount != 0) {
		if (object->partCount > TG_SPR_V1_2_NUM_SPRITE_PARTS || !object->parts)
			warning("SpriteLibrary_v1_2::spriteGetInfo(): invalid multipart record count %d", object->partCount);

		for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS && object->parts; i++)
			info.frames[i] = object->parts->parts[i].frame;

		info.frameCount = object->partCount;
	}

	info.scriptActive = object->scriptActive;
	info.visible = object->visible ? 1 : 0;
	info.x = object->x;
	info.y = object->y;
	info.boundingBox = object->boundingBox;
	info.id = object->id;
	info.currentCommand = object->queueCurrent;
}

bool SpriteLibrary_v1_2::spriteGetBounds(uint16 sprite, Common::Rect &bounds) {
	if (sprite == 0)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteGetBounds");
	if (!object)
		return false;

	bounds = object->boundingBox;

	if (_engine->_version >= kVersionV1_2_12 && object->scrollable)
		bounds.translate(-_scrollPosX, -_scrollPosY);

	return true;
}

bool SpriteLibrary_v1_2::spriteGetPosition(uint16 sprite, int16 &x, int16 &y) {
	if (sprite == 0)
		return false;

	SpriteObject_v1_2 *object = findSprite(sprite, "spriteGetPosition");
	if (!object)
		return false;

	x = object->x;
	y = object->y;
	return true;
}

uint16 SpriteLibrary_v1_2::spriteIsVisible(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteIsVisible");
	if (!object)
		return 0;

	return object->visible ? 1 : 0;
}

void SpriteLibrary_v1_2::spriteSetBackground(byte *background, bool noClear, uint16 colorWord, Common::Rect *outClipRect) {
	_backgroundBitmap = nullptr;

	if (_engine->_version >= kVersionV1_2_12) {
		if (_tileBackground) {
			spriteResetComposeBuffer();
			_tileBackground = nullptr;
		}

		if (_scrollSurface) {
			spriteDibFree(_scrollSurface);
			_scrollSurface = nullptr;
		}
	}

	_fillColor = 0;

	if (_engine->_version < kVersionV1_2_12 || !_directMode) {
		if (noClear)
			_fillColor = TG_SPR_V1_2_FILL_NONE;
		else if (background)
			_backgroundBitmap = background;
		else
			_fillColor = colorWord & 0xFF;
	}

	computeClipRect();

	if (outClipRect)
		*outClipRect = _clipRect;
}

void SpriteLibrary_v1_2::spriteSetClipBox(const Common::Rect *box) {
	if (box) {
		_userClipBox = *box;

		if (_engine->_version >= kVersionV1_2_7_3 && _engine->_version != kVersionV1_2_8) {
			if (_userClipBox.left < _userClipBox.right && _userClipBox.top < _userClipBox.bottom) {
				_userClipBox.right++;
				_userClipBox.bottom++;
			}
		} else {
			_clipRect = _userClipBox;
		}
	} else {
		_userClipBox = Common::Rect();
	}

	computeClipRect();
}

bool SpriteLibrary_v1_2::spriteSetZoom(uint16 factor, int16 panX, int16 panY) {
	bool previous = _zoomActive;

	_zoomOffsetX = (int16)((int32)panX * (int16)factor - 1);
	_zoomOffsetY = (int16)((int32)panY * (int16)factor - 1);
	_zoomFactor = factor;

	_zoomActive = (factor != 0 && (factor != 1 || panX != 0 || panY != 0));

	if (_engine->_version >= kVersionV1_2_3 && !_zoomActive) {
		_zoomOffsetX = 0;
		_zoomOffsetY = 0;
		_zoomFactor = 1;
	}

	return _zoomActive != previous;
}

void SpriteLibrary_v1_2::spriteSetStretch(uint16 flag) {
	_stretch = ((flag & 0xFF) != 0);
}

void SpriteLibrary_v1_2::spriteSetMessageProc(bool set) {
	_messageProcSet = set;
}

void SpriteLibrary_v1_2::spriteSetServiceProc(bool set, uint16 serviceWord) {
	_serviceProcSet = set;
	_serviceWord = serviceWord;
}

void SpriteLibrary_v1_2::spriteSetMovieProc(bool set) {
	_movieProcSet = set;
}

void SpriteLibrary_v1_2::spriteSetStreamProc(bool set) {
	_streamProcSet = set;
}

void SpriteLibrary_v1_2::spriteSetDebugProc(bool set) {
	_debugProcSet = set;
}

void SpriteLibrary_v1_2::spriteSetQueueDoneProc(bool enabled) {
	_queueDoneProcSet = enabled;
}

uint16 SpriteLibrary_v1_2::spriteSetSendMsg(uint16 flag) {
	uint16 previous = _hostEventsSuppressed ? 0 : 1;
	_hostEventsSuppressed = (flag == 0);
	return previous;
}

void SpriteLibrary_v1_2::spriteSetVarTable(const uint16 *table) {
	_variableTable = table;
}

byte *SpriteLibrary_v1_2::callStreamProc(uint16 sprite, uint16 id, uint16 release) {
	if (!_streamProcSet)
		return nullptr;

	SpriteObject_v1_2 *object = findSprite(sprite, "callStreamProc");
	if (!object)
		return nullptr;

	if (_engine->_version >= kVersionV1_2_1)
		_inHostCallback = true;

	object->scriptActive = 0;
	object->inCallback = true;

	byte *result = engine()->spriteBitmapProvider(id, release != 0);

	if (_engine->_version >= kVersionV1_2_1)
		_inHostCallback = false;

	if (currentTickSprite() != sprite && findSpriteIndex(sprite) == -1)
		return nullptr;

	object = lookupSprite(sprite);
	if (!object || !object->inCallback)
		return nullptr;

	object->scriptActive = 1;
	object->inCallback = false;
	return result;
}

byte *SpriteLibrary_v1_2::getCelBitmap(uint16 id, uint16 sprite) {
	byte *bitmap = findBitmapInCelCache(id, sprite);
	if (bitmap)
		return bitmap;

	bitmap = callStreamProc(sprite, id, 0);
	if (!bitmap)
		return nullptr;

	if (addBitmapToCelCache(sprite, bitmap, id))
		return bitmap;

	releaseCachedBitmap(sprite, id);
	return nullptr;
}

byte *SpriteLibrary_v1_2::findBitmapInCelCache(uint16 id, uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "findBitmapInCelCache");
	if (!object || object->celCacheCount == 0 || !object->celCache)
		return nullptr;

	CelReference *entries = object->celCache->entries;
	for (uint16 i = 0; i < object->celCacheCount; i++) {
		if (entries[i].id != id)
			continue;

		CelReference found = entries[i];
		for (uint16 j = i; j > 0; j--)
			entries[j] = entries[j - 1];

		entries[0] = found;
		return found.bitmap;
	}

	return nullptr;
}

bool SpriteLibrary_v1_2::addBitmapToCelCache(uint16 sprite, byte *bitmap, uint16 id) {
	SpriteObject_v1_2 *object = findSprite(sprite, "addBitmapToCelCache");
	if (!object)
		return false;

	uint16 cacheSize = (_engine->_version >= kVersionV1_2_1) ? TG_SPR_V1_2_1_CEL_CACHE_SIZE : TG_SPR_V1_2_CEL_CACHE_SIZE;

	if (!object->celCache)
		object->celCache = CelCacheBlockPtr(new CelCacheBlock());

	if (object->celCacheCount >= cacheSize) {
		releaseCachedBitmap(sprite, object->celCache->entries[cacheSize - 1].id);

		object = findSprite(sprite, "addBitmapToCelCache");
		if (!object)
			return false;

		object->celCacheCount--;
	}

	CelReference *entries = object->celCache->entries;
	if (object->celCacheCount != 0) {
		for (uint16 j = cacheSize - 1; j > 0; j--)
			entries[j] = entries[j - 1];
	}

	entries[0] = CelReference(bitmap, id);
	object->celCacheCount++;
	return true;
}

void SpriteLibrary_v1_2::releaseCachedBitmap(uint16 sprite, uint16 id) {
	callStreamProc(sprite, id, 1);
}

void SpriteLibrary_v1_2::releaseAllCachedBitmaps(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "releaseAllCachedBitmaps");
	if (!object)
		return;

	uint16 i = 0;
	while (i < object->celCacheCount && object->celCache) {
		callStreamProc(sprite, object->celCache->entries[i].id, 1);

		object = lookupSprite(sprite);
		if (!object)
			return;

		i++;
	}

	object->celCacheCount = 0;
}

SpriteLibrary_v1_2_4::SpriteLibrary_v1_2_4(TopgunEngine_v1_2 *engine) : SpriteLibrary_v1_2(engine) {
}

SpriteLibrary_v1_2_4::~SpriteLibrary_v1_2_4() {
}

void SpriteLibrary_v1_2_4::spriteHide(uint16 sprite, bool refresh) {
	SpriteLibrary::spriteHide(sprite, refresh);

	SpriteObject_v1_2 *object = lookupSprite(sprite);
	if (object)
		object->multipartValid = false;
}

void SpriteLibrary_v1_2_4::spriteBreakLoops(uint16 sprite, uint16 flag) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteBreakLoops");
	if (!object)
		return;

	if (_engine->_version < kVersionV1_2_4)
		object->loopsDisabled = true;
	else if (_engine->_version >= kVersionV1_2_12)
		object->loopsDisabled = ((flag & 1) != 0);
	else
		object->loopsDisabled = ((flag & 0xFF) != 0);
}

void SpriteLibrary_v1_2_4::spriteSetPickMode(uint16 sprite, uint16 wholeBox, uint16 pickInvisible) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetPickMode");
	if (!object)
		return;

	if (_engine->_version >= kVersionV1_2_12)
		object->pickWholeBox = ((wholeBox & 1) != 0);
	else
		object->pickWholeBox = ((wholeBox & 0xFF) != 0);

	if (_engine->_version >= kVersionV1_2_9)
		object->pickMode = pickInvisible;
	else if (_engine->_version >= kVersionV1_2_3_3)
		object->pickInvisible = ((pickInvisible & 0xFF) != 0);
}

void SpriteLibrary_v1_2_4::pauseResumeSprite(uint16 sprite, bool pause) {
	SpriteObject_v1_2 *object = findSprite(sprite, "pauseResumeSprite");
	if (!object)
		return;

	if (pause == object->paused)
		return;

	uint32 now = engine()->getEngineMillis();
	if (pause)
		object->pauseTime = now;
	else
		object->frameDue += now - object->pauseTime;

	object->paused = pause;
}

void SpriteLibrary_v1_2_4::spritePause(uint16 sprite, uint16 flag) {
	bool pause = (flag != 0);

	if (sprite != 0) {
		pauseResumeSprite(sprite, pause);
		return;
	}

	for (uint16 i = 0; i < _spriteCount; i++)
		pauseResumeSprite(_spriteTable[i], pause);
}

void SpriteLibrary_v1_2_4::spriteSkipMessage(uint16 sprite, uint16 count) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSkipMessage");
	if (!object)
		return;

	while (count != 0) {
		count--;

		object->queueCurrent = nextQueueRecordWithLoops(sprite);
		if (!object->queueCurrent)
			break;
	}
}

uint16 SpriteLibrary_v1_2_4::spriteGetExtraId(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteGetExtraId");
	if (!object)
		return 0;

	return object->extraId;
}

void SpriteLibrary_v1_2_4::spriteSetExtraId(uint16 sprite, uint16 extraId) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetExtraId");
	if (!object)
		return;

	object->extraId = extraId;
}

uint16 SpriteLibrary_v1_2_4::spriteGetNumCels(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteGetNumCels");
	if (!object)
		return 0;

	return (uint16)object->frames.size();
}

void SpriteLibrary_v1_2_4::spriteSetSpriteClipBox(uint16 sprite, const Common::Rect *box) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetSpriteClipBox");
	if (!object)
		return;

	if (box && box->right > box->left && box->bottom > box->top) {
		object->hasClipBox = true;
		object->clipBox = *box;
	} else {
		object->hasClipBox = false;
		object->clipBox = Common::Rect();
	}
}

void SpriteLibrary_v1_2_4::spriteGetInfo(uint16 sprite, SpriteInfo_v1_2_4 &info) {
	SpriteLibrary_v1_2::spriteGetInfo(sprite, info);

	SpriteObject_v1_2 *object = lookupSprite(sprite);
	if (!object)
		return;

	info.rangeStart = object->rangeStart;
	info.rangeEnd = object->rangeEnd;
	info.nextFrame = object->nextFrameIndex;
	info.forward = object->forward ? 1 : 0;
}

uint16 SpriteLibrary_v1_2_4::spriteIsValid(uint16 sprite) {
	return (findSpriteIndex(sprite) >= 0) ? 1 : 0;
}

Common::Point SpriteLibrary_v1_2_4::spriteLinePoint(uint16 step, uint16 total, int16 x0, int16 y0, int16 x1, int16 y1) {
	return computeLinePoint(step, total, x0, y0, x1, y1);
}

Common::Point SpriteLibrary_v1_2_4::spriteCurvePoint(uint16 step, uint16 steps, int16 x0, int16 y0, int16 viaX, int16 viaY, int16 endX, int16 endY) {
	return computeQuadCurvePoint(step, steps, x0, y0, viaX, viaY, endX, endY);
}

} // End of namespace Topgun
