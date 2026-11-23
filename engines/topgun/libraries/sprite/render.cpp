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
#include "topgun/libraries/sprite/sprite.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun.h"

namespace Topgun {

// Unlike Common::Rect::findIntersectingRect() this ACCEPTS inverted rectangles :P
// This mirrors the original IntersectRect call...
bool SpriteLibrary::intersectRects(const Common::Rect &first, const Common::Rect &second, Common::Rect &result) {
	int16 left = MAX(first.left, second.left);
	int16 top = MAX(first.top, second.top);
	int16 right = MIN(first.right, second.right);
	int16 bottom = MIN(first.bottom, second.bottom);

	if (left >= right || top >= bottom) {
		result = Common::Rect();
		return false;
	}

	result.setRect(left, top, right, bottom);
	return true;
}

// And this, unlike Common::Rect::extend() doesn't ignore empty rectangles, mirroring the original UnionRect call...
Common::Rect SpriteLibrary::unionRects(const Common::Rect &first, const Common::Rect &second) {
	if (first.isEmpty() && second.isEmpty())
		return Common::Rect();

	if (first.isEmpty())
		return second;

	if (second.isEmpty())
		return first;

	Common::Rect result;
	result.setRect(MIN(first.left, second.left), MIN(first.top, second.top), MAX(first.right, second.right), MAX(first.bottom, second.bottom));
	return result;
}

void SpriteLibrary::spritePaint(uint16 sprite) {
	if (sprite != 0) {
		renderSpriteToSurface(sprite);
		return;
	}

	if (_engine->_version >= kVersionV1_2_8 && renderRect(_clipRect)) {
		spriteResetComposeBuffer();
		return;
	}

	if (_backgroundBitmap)
		blitSpriteToScreen((int16)(_scrollPosX - _scrollX), (int16)(_scrollPosY - _scrollY), _backgroundBitmap);

	for (uint16 i = 0; i < _spriteCount; i++)
		renderSpriteToSurface(_spriteTable[i]);
}

void SpriteLibrary::spritePaintRect(const Common::Rect &rect, bool backgroundCoordinates) {
	Common::Rect local = rect;

	if (backgroundCoordinates)
		local.translate(-_scrollX, -_scrollY);

	renderRect(local);
}

void SpriteLibrary::spriteSetBackground(byte *background, Common::Rect *outClipRect) {
	_clipRect.left = 0;
	_clipRect.top = 0;
	_backgroundBitmap = background;

	if (background) {
		_clipRect.right = (int16)READ_LE_UINT16(background + TG_SPR_BITMAP_WIDTH_OFFSET);
		_clipRect.bottom = (int16)READ_LE_UINT16(background + TG_SPR_BITMAP_HEIGHT_OFFSET);
	} else {
		_clipRect.right = _engine->display()->width();
		_clipRect.bottom = _engine->display()->height();
	}

	_scrollY = (_clipRect.bottom - 1) / 2;
	_scrollX = (_clipRect.right - 1) / 2;
	_clipRect.translate(-_scrollX, -_scrollY);

	if (outClipRect)
		*outClipRect = _clipRect;
}

void SpriteLibrary::spriteSetBkgTransparent(bool flag) {
	_bkgTransparent = flag;
}

uint16 SpriteLibrary::spritePicked(int16 x, int16 y) {
	if (_clipRect.left > x || _clipRect.right <= x || _clipRect.top > y || _clipRect.bottom <= y)
		return 0;

	for (int index = (int)_spriteCount - 1; index >= 0; index--) {
		uint16 handle = _spriteTable[index];
		SpriteObject *object = findSprite(handle, "spritePicked");
		if (!object || !object->visible)
			continue;

		const Common::Rect &box = object->boundingBox;
		if (box.left > x || box.right <= x || y < box.top || y >= box.bottom)
			continue;

		if (object->partCount != 0) {
			if (!object->parts) {
				warning("SpriteLibrary::spritePicked(): the multipart sprite has no parts");
				continue;
			}

			for (uint16 part = 0; part < object->partCount; part++) {
				const SpritePart &spritePart = object->parts->parts[part];
				if (spritePart.box.left > x || spritePart.box.right <= x || y < spritePart.box.top || y >= spritePart.box.bottom)
					continue;

				if (!spritePart.frame) {
					warning("SpriteLibrary::spritePicked(): the sprite part has no frame");
					continue;
				}

				if (getBitmapPixel(spritePart.frame, (int16)(x - spritePart.box.left), (int16)(y - spritePart.box.top)) != 0)
					return handle;
			}
		} else {
			if (!object->currentFrame) {
				warning("SpriteLibrary::spritePicked(): the sprite has no frame");
				continue;
			}

			if (getBitmapPixel(object->currentFrame, (int16)(x - box.left), (int16)(y - box.top)) != 0)
				return handle;
		}
	}

	return 0;
}

void SpriteLibrary::spriteCopyRect(Common::Rect &rect, const byte *source, byte *destination) {
	if (!source || !destination)
		return;

	int16 sourceWidth = (int16)READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 sourceHeight = (int16)READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);

	if (sourceWidth < rect.right)
		rect.right = sourceWidth;

	if (sourceHeight < rect.bottom)
		rect.bottom = sourceHeight;

	int16 width = (int16)(rect.right - rect.left);
	int16 height = (int16)(rect.bottom - rect.top);
	if (width <= 0 || height <= 0)
		return;

	blitOpaqueRect(destination, source, width, height, rect.left, rect.top, rect.left, rect.top);
}

void SpriteLibrary::spriteDibFree(byte *bitmap) {
	_engine->utilsLibrary()->tgFree(bitmap);
}

bool SpriteLibrary::renderRect(const Common::Rect &rect) {
	Common::Rect visible;
	if (!intersectRects(rect, _clipRect, visible))
		return true;

	if (!_composeBuffer) {
		warning("SpriteLibrary::renderRect(): the compose buffer does not exist");
		return false;
	}

	uint16 width = visible.width();
	uint16 height = visible.height();
	if (!ensureComposeBufferSize(width, height))
		return false;

	if (_backgroundBitmap)
		blitOpaqueRect(_composeBuffer, _backgroundBitmap, width, height, 0, 0, (int16)(visible.left + _scrollX), (int16)(visible.top + _scrollY));
	else
		fillBitmapByte(_composeBuffer, 0);

	compositeSpritesInRect(visible);
	blitSpriteToScreen(visible.left, visible.top, _composeBuffer);
	return true;
}

bool SpriteLibrary::repaintUnionOfCurrentAndPreviousRect(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "repaintUnionOfCurrentAndPreviousRect");
	if (!object)
		return false;

	return renderRect(unionRects(object->boundingBox, object->previousBox));
}

void SpriteLibrary::repaintPreviousRect(uint16 sprite, bool excludeCurrent) {
	SpriteObject *object = findSprite(sprite, "repaintPreviousRect");
	if (!object)
		return;

	Display *display = _engine->display();
	int savedState = 0;

	if (excludeCurrent) {
		savedState = display->saveClipState();
		display->excludeClipRect(object->boundingBox);
	}

	bool wasVisible = object->visible;
	object->visible = false;
	renderRect(object->previousBox);
	object->visible = wasVisible;

	if (savedState != 0)
		display->restoreClipState(savedState);
}

void SpriteLibrary::renderSpriteToSurface(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "renderSpriteToSurface");
	if (!object || !object->visible)
		return;

	if (object->framePending) {
		applyFrameChange(sprite);

		object = findSprite(sprite, "renderSpriteToSurface");
		if (!object)
			return;
	}

	bool repainted = false;
	if (object->previousValid && object->useUnionRefresh)
		repainted = repaintUnionOfCurrentAndPreviousRect(sprite);

	if (!repainted) {
		if (object->previousValid)
			repaintPreviousRect(sprite, true);

		renderRect(object->boundingBox);
	}

	object->previousBox = object->boundingBox;
	object->previousValid = true;
}

void SpriteLibrary::compositeSpritesInRect(const Common::Rect &rect) {
	for (uint16 i = 0; i < _spriteCount; i++) {
		uint16 handle = _spriteTable[i];
		SpriteObject *object = findSprite(handle, "compositeSpritesInRect");
		if (!object || !object->visible)
			continue;

		Common::Rect common;
		if (!intersectRects(rect, object->boundingBox, common))
			continue;

		if (_engine->_version >= kVersionV1_1 && object->collisionTarget != 0)
			checkCollision(handle);

		if (object->partCount != 0) {
			if (!object->parts) {
				warning("SpriteLibrary::compositeSpritesInRect(): the multipart sprite has no parts");
				continue;
			}

			compositeSpritePartsIntoBuffer(handle, object->partCount, object->parts->parts, rect);
			continue;
		}

		if (!object->currentFrame) {
			warning("SpriteLibrary::compositeSpritesInRect(): the sprite has no frame");
			continue;
		}

		uint16 width = common.width();
		uint16 height = common.height();
		int16 destinationX = (int16)(common.left - rect.left);
		int16 destinationY = (int16)(common.top - rect.top);
		int16 sourceX = (int16)(common.left - object->boundingBox.left);
		int16 sourceY = (int16)(common.top - object->boundingBox.top);

		if (_bkgTransparent && handle != _topmostSprite)
			blitMaskedRect(_composeBuffer, object->currentFrame, width, height, destinationX, destinationY, sourceX, sourceY);
		else
			blitTransparentRect(_composeBuffer, object->currentFrame, width, height, destinationX, destinationY, sourceX, sourceY);
	}
}

void SpriteLibrary::checkCollision(uint16 sprite) {
	SpriteObject *object = findSprite(sprite, "checkCollision");
	if (!object)
		return;

	SpriteObject *target = nullptr;
	for (uint16 i = 0; i < _spriteCount; i++) {
		SpriteObject *candidate = findSprite(_spriteTable[i], "checkCollision");
		if (candidate && candidate->id == object->collisionTarget) {
			target = candidate;
			break;
		}
	}

	if (!target || object->frames.empty() || target->frames.empty() || !target->visible)
		return;

	Common::Rect overlap;
	if (!intersectRects(target->boundingBox, object->boundingBox, overlap))
		return;

	if (!object->currentFrame || !target->currentFrame) {
		warning("SpriteLibrary::checkCollision(): the sprite has no current frame");
		return;
	}

	for (int16 y = overlap.top; y < overlap.bottom; y++) {
		for (int16 x = overlap.left; x < overlap.right; x++) {
			if (getBitmapPixel(target->currentFrame, x - target->boundingBox.left, y - target->boundingBox.top) != 0 &&
				getBitmapPixel(object->currentFrame, x - object->boundingBox.left, y - object->boundingBox.top) != 0) {
				_engine->postMessage(kMsgRunScript, object->collisionValue, 0);
				return;
			}
		}
	}
}

void SpriteLibrary::recomputeCelBoxArray(uint16 sprite, const byte *command) {
	SpriteObject *object = findSprite(sprite, "recomputeCelBoxArray");
	if (!object)
		return;

	uint16 count = command[0x0D];
	if (count > TG_SPR_NUM_SPRITE_PARTS) {
		warning("SpriteLibrary::recomputeCelBoxArray(): %d parts do not fit in the part block", count);
		count = TG_SPR_NUM_SPRITE_PARTS;
	}

	object->partCount = (byte)count;

	for (uint16 i = 0; i < count; i++) {
		uint16 index = READ_LE_UINT16(command + 2 + 2 * i);
		if (index >= object->frames.size()) {
			warning("SpriteLibrary::recomputeCelBoxArray(): frame index %d is out of range", index);
			break;
		}

		byte *frame = object->frames[index];
		int16 width = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_WIDTH_OFFSET);
		int16 height = (int16)READ_LE_UINT16(frame + TG_SPR_BITMAP_HEIGHT_OFFSET);
		int16 widthMinusOne = (int16)(width - 1);
		int16 heightMinusOne = (int16)(height - 1);

		SpritePart &part = object->parts->parts[i];
		part.frame = frame;
		part.box.left = (int16)(object->x + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_X_OFFSET) - widthMinusOne / 2);
		part.box.right = (int16)(width + part.box.left);
		part.box.top = (int16)(object->y + READ_LE_INT16(frame + TG_SPR_BITMAP_HOT_Y_OFFSET) - heightMinusOne / 2);
		part.box.bottom = (int16)(height + part.box.top);

		if (i == 0)
			object->boundingBox = part.box;
		else
			object->boundingBox = unionRects(object->boundingBox, part.box);
	}
}

void SpriteLibrary::compositeSpritePartsIntoBuffer(uint16 sprite, uint16 count, const SpritePart *parts, const Common::Rect &rect) {
	for (uint16 i = 0; i < count; i++) {
		const SpritePart &part = parts[i];

		Common::Rect common;
		if (!intersectRects(rect, part.box, common))
			continue;

		if (!part.frame) {
			warning("SpriteLibrary::compositeSpritePartsIntoBuffer(): the sprite part has no frame");
			continue;
		}

		uint16 width = common.width();
		uint16 height = common.height();
		int16 destinationX = (int16)(common.left - rect.left);
		int16 destinationY = (int16)(common.top - rect.top);
		int16 sourceX = (int16)(common.left - part.box.left);
		int16 sourceY = (int16)(common.top - part.box.top);

		if (_bkgTransparent && sprite != _topmostSprite)
			blitMaskedRect(_composeBuffer, part.frame, width, height, destinationX, destinationY, sourceX, sourceY);
		else
			blitTransparentRect(_composeBuffer, part.frame, width, height, destinationX, destinationY, sourceX, sourceY);
	}
}

void SpriteLibrary::blitSpriteToScreen(int16 x, int16 y, const byte *bitmap) {
	uint16 dataOffset = READ_LE_UINT16(bitmap);
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 pitch = READ_LE_UINT16(bitmap + 4);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	_engine->display()->putBitmapRows(x, y, width, height, bitmap + dataOffset, pitch, &_colorTable[0][0]);
}

bool SpriteLibrary::ensureComposeBufferSize(uint16 width, uint16 height) {
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + bitmapHeaderSize();

	WRITE_LE_UINT16(_composeBuffer + 8, size > 0xFDE8 ? 1 : 0);
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_WIDTH_OFFSET, width);
	WRITE_LE_UINT16(_composeBuffer + 4, pitch);
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_HEIGHT_OFFSET, height);

	if (size > _composeBufferCapacity) {
		byte *grown = _engine->utilsLibrary()->tgRealloc(_composeBuffer, size, 0);
		if (!grown)
			return false;

		_composeBuffer = grown;
		_composeBufferCapacity = size;
	}

	return true;
}

bool SpriteLibrary::blitSetupRaw(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, BlitSetup &setup) {
	uint16 sourceOffset = READ_LE_UINT16(source);
	uint16 sourceWidth = READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 sourcePitch = READ_LE_UINT16(source + 4);
	uint16 sourceHeight = READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);
	uint16 destinationOffset = READ_LE_UINT16(destination);
	uint16 destinationWidth = READ_LE_UINT16(destination + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 destinationPitch = READ_LE_UINT16(destination + 4);
	uint16 destinationHeight = READ_LE_UINT16(destination + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool inside = (destinationX >= 0 && destinationY >= 0 && sourceX >= 0 && sourceY >= 0 &&
	               destinationX + width <= destinationWidth && destinationY + height <= destinationHeight &&
	               sourceX + width <= sourceWidth && sourceY + height <= sourceHeight);

	if (!inside) {
		warning("SpriteLibrary::blitSetupRaw(): the rectangle is outside the bitmaps");

		if (destinationX < 0 || destinationY < 0 || sourceX < 0 || sourceY < 0)
			return false;

		int availableWidth = MIN(sourceWidth - sourceX, destinationWidth - destinationX);
		int availableHeight = MIN(sourceHeight - sourceY, destinationHeight - destinationY);
		if (availableWidth <= 0 || availableHeight <= 0)
			return false;

		width = MIN((int)width, availableWidth);
		height = MIN((int)height, availableHeight);
	}

	uint16 sourceRow = (uint16)(sourceHeight - sourceY - height);
	setup.source = source + sourceOffset + (uint32)sourceRow * sourcePitch + sourceX;
	uint16 destinationRow = (uint16)(destinationHeight - destinationY - height);
	setup.destination = destination + destinationOffset + (uint32)destinationRow * destinationPitch + destinationX;
	setup.sourceSkip = sourcePitch - width;
	setup.destinationSkip = destinationPitch - width;
	return true;
}

void SpriteLibrary::blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (width == 0 || height == 0)
		return;

	BlitSetup setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		memmove(setup.destination, setup.source, width);
		setup.destination += width + setup.destinationSkip;
		setup.source += width + setup.sourceSkip;
	}
}

void SpriteLibrary::blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (width == 0 || height == 0)
		return;

	BlitSetup setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = setup.source[column];
			if (pixel != 0)
				setup.destination[column] = pixel;
		}

		setup.destination += width + setup.destinationSkip;
		setup.source += width + setup.sourceSkip;
	}
}

void SpriteLibrary::blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (width == 0 || height == 0)
		return;

	BlitSetup setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = setup.source[column];
			if (pixel != 0 && setup.destination[column] != 0)
				setup.destination[column] = pixel;
		}

		setup.destination += width + setup.destinationSkip;
		setup.source += width + setup.sourceSkip;
	}
}

uint16 SpriteLibrary::getBitmapPixel(const byte *bitmap, int16 x, int16 y) {
	uint16 dataOffset = READ_LE_UINT16(bitmap);
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 pitch = READ_LE_UINT16(bitmap + 4);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	if (x < 0 || x >= width || y < 0 || y >= height) {
		warning("SpriteLibrary::getBitmapPixel(): the pixel is outside the bitmap");
		return 0;
	}

	uint16 row = (uint16)(height - y - 1);
	return bitmap[dataOffset + (uint32)row * pitch + x];
}

void SpriteLibrary::fillBitmapByte(byte *bitmap, uint16 color) {
	uint16 dataOffset = READ_LE_UINT16(bitmap);
	uint16 pitch = READ_LE_UINT16(bitmap + 4);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	memset(bitmap + dataOffset, color & 0xFF, (uint32)height * pitch);
}

} // End of namespace Topgun
