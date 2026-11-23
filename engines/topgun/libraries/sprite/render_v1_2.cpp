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

#include "common/array.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "graphics/screen.h"

#include "topgun/display.h"
#include "topgun/libraries/sprite/sprite_v1_2_3.h"
#include "topgun/libraries/utils/utils.h"
#include "topgun/topgun_v1_2_3.h"

namespace Topgun {

bool SpriteLibrary_v1_2::intersectRectLocal(const Common::Rect &first, const Common::Rect &second, Common::Rect &result) {
	int16 left = MAX(first.left, second.left);
	int16 right = MIN(first.right, second.right);
	if (left >= right)
		return false;

	int16 top = MAX(first.top, second.top);
	int16 bottom = MIN(first.bottom, second.bottom);
	if (top >= bottom)
		return false;

	result.left = left;
	result.top = top;
	result.right = right;
	result.bottom = bottom;
	return true;
}

void SpriteLibrary_v1_2::unionRectLocal(const Common::Rect &first, const Common::Rect &second, Common::Rect &result) {
	int16 left = MIN(first.left, second.left);
	int16 right = MAX(first.right, second.right);
	int16 top = MIN(first.top, second.top);
	int16 bottom = MAX(first.bottom, second.bottom);

	result.left = left;
	result.top = top;
	result.right = right;
	result.bottom = bottom;
}

byte *SpriteLibrary_v1_2::bitmapPixels(const byte *bitmap) const {
	return const_cast<byte *>(bitmap) + READ_LE_UINT16(bitmap);
}

byte *SpriteLibrary_v1_2::spriteDibCreate(uint16 width, uint16 height, uint16 flags) {
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + TG_SPR_V1_2_BITMAP_HEADER_SIZE;

	byte *bitmap = engine()->utilsLibrary()->tgMalloc(size, flags);
	if (!bitmap)
		return nullptr;

	memset(bitmap, 0, TG_SPR_V1_2_BITMAP_HEADER_SIZE);
	WRITE_LE_UINT16(bitmap, TG_SPR_V1_2_BITMAP_HEADER_SIZE);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET, width);
	WRITE_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET, pitch);
	WRITE_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET, height);
	return bitmap;
}

void SpriteLibrary_v1_2::spriteDibBlit(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 mode) {
	if (!destination || !source)
		return;

	switch (mode) {
	case 0:
		blitOpaqueRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
		break;

	case 1:
		blitDibDiffRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
		break;

	case 2:
		blitTransparentRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
		break;

	default:
		break;
	}
}

void SpriteLibrary_v1_2::spriteDibDiff(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (!destination || !source) {
		warning("SpriteLibrary_v1_2::spriteDibDiff(): a bitmap is missing");
		return;
	}

	blitDibDiffRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
}

void SpriteLibrary_v1_2::spriteDibMerge(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY) {
	if (!destination || !source) {
		warning("SpriteLibrary_v1_2::spriteDibMerge(): a bitmap is missing");
		return;
	}

	blitTransparentRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, 0);
}

void SpriteLibrary_v1_2::spriteDibSet(byte *bitmap, int16 x, int16 y, uint16 width, uint16 height, uint16 color) {
	if (!bitmap) {
		warning("SpriteLibrary_v1_2::spriteDibSet(): the bitmap is missing");
		return;
	}

	fillBitmapByte(bitmap, x, y, width, height, color);
}

void SpriteLibrary_v1_2::spriteCopyRect(Common::Rect &rect, byte *destination, const byte *source) {
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

	blitOpaqueRect(destination, source, width, height, rect.left, rect.top, rect.left, rect.top, 0);
}

bool SpriteLibrary_v1_2::blitSetupRaw(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, BlitSetup_v1_2 &setup) {
	uint16 sourceWidth = READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 sourcePitch = READ_LE_UINT16(source + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 sourceHeight = READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);
	uint16 destinationWidth = READ_LE_UINT16(destination + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 destinationPitch = READ_LE_UINT16(destination + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 destinationHeight = READ_LE_UINT16(destination + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool inside = (destinationX >= 0 && destinationY >= 0 && sourceX >= 0 && sourceY >= 0 &&
	               destinationX + width <= destinationWidth && destinationY + height <= destinationHeight &&
	               sourceX + width <= sourceWidth && sourceY + height <= sourceHeight);

	if (!inside) {
		warning("SpriteLibrary_v1_2::blitSetupRaw(): the rectangle is outside the bitmaps");

		if (destinationX < 0 || destinationY < 0 || sourceX < 0 || sourceY < 0)
			return false;

		int availableWidth = MIN(sourceWidth - sourceX, destinationWidth - destinationX);
		int availableHeight = MIN(sourceHeight - sourceY, destinationHeight - destinationY);
		if (availableWidth <= 0 || availableHeight <= 0)
			return false;

		width = MIN((int)width, availableWidth);
		height = MIN((int)height, availableHeight);
	}

	bool mirror = ((flags & 0xFF) != 0);

	int32 sourceStride;
	uint16 sourceRow;
	bool unflipped = (_engine->_version >= kVersionV1_2_3) ? ((flags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) == 0) : (((flags >> 8) & 0xFF) == source[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET]);

	if (unflipped) {
		sourceRow = (uint16)(sourceHeight - sourceY - height);
		sourceStride = sourcePitch;
	} else {
		sourceRow = (uint16)(sourceY + height - 1);
		sourceStride = -(int32)sourcePitch;
	}

	int32 sourceOffset = (int32)((uint32)sourceRow * sourcePitch);
	if (mirror)
		sourceOffset += (int32)sourceWidth - sourceX;
	else
		sourceOffset += sourceX;

	setup.source = bitmapPixels(source) + sourceOffset;

	bool topDown;
	if (_engine->_version >= kVersionV1_2_5)
		topDown = ((READ_LE_UINT16(destination + TG_SPR_V1_2_5_BITMAP_FLAGS_OFFSET) & TG_SPR_V1_2_5_BITMAP_TOP_DOWN_FLAG) != 0);
	else
		topDown = (destination[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] != 0);

	int32 destinationStride;
	uint16 destinationRow;
	if (!topDown) {
		destinationRow = (uint16)(destinationHeight - destinationY - height);
		destinationStride = destinationPitch;
	} else {
		destinationRow = (uint16)(destinationY + height - 1);
		destinationStride = -(int32)destinationPitch;
	}

	setup.destination = bitmapPixels(destination) + (int32)((uint32)destinationRow * destinationPitch) + destinationX;

	if (mirror)
		setup.sourceSkip = sourceStride + width;
	else
		setup.sourceSkip = sourceStride - width;

	setup.destinationSkip = destinationStride - width;
	return true;
}

void SpriteLibrary_v1_2::blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		memmove(setup.destination, setup.source, width);
		setup.destination += width;
		setup.source += width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

void SpriteLibrary_v1_2::blitDibDiffRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (_engine->_version >= kVersionV1_2_5 && isRleBitmap(source))
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			if (setup.destination[column] == TG_SPR_V1_2_OPAQUE_FILL_BYTE)
				setup.destination[column] = setup.source[column];
		}

		setup.destination += width;
		setup.source += width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

void SpriteLibrary_v1_2::blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	bool mirror = ((flags & 1) != 0);

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = mirror ? setup.source[-1 - (int32)column] : setup.source[column];
			if (pixel != 0)
				setup.destination[column] = pixel;
		}

		setup.destination += width;
		setup.source += mirror ? -(int32)width : (int32)width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

void SpriteLibrary_v1_2::blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	bool mirror = ((flags & 1) != 0);

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = mirror ? setup.source[-1 - (int32)column] : setup.source[column];
			if (pixel != 0 && setup.destination[column] != 0)
				setup.destination[column] = pixel;
		}

		setup.destination += width;
		setup.source += mirror ? -(int32)width : (int32)width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}
}

uint16 SpriteLibrary_v1_2::blitPixelOverlapTest(const byte *first, const byte *second, uint16 width, uint16 height, int16 firstX, int16 firstY, int16 secondX, int16 secondY, uint16 flags) {
	if (width == 0 || height == 0)
		return 0;

	BlitSetup_v1_2 setup;
	if (!blitSetupRaw(const_cast<byte *>(first), second, width, height, firstX, firstY, secondX, secondY, flags, setup))
		return 0;

	bool mirror = ((flags & 1) != 0);

	for (uint16 row = 0; row < height; row++) {
		for (uint16 column = 0; column < width; column++) {
			byte pixel = mirror ? setup.source[-1 - (int32)column] : setup.source[column];
			if (pixel != 0 && setup.destination[column] != 0)
				return 1;
		}

		setup.destination += width;
		setup.source += mirror ? -(int32)width : (int32)width;
		setup.destination += setup.destinationSkip;
		setup.source += setup.sourceSkip;
	}

	return 0;
}

uint16 SpriteLibrary_v1_2::getBitmapPixel(const byte *bitmap, int16 x, int16 y, uint16 flip) {
	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	uint16 row;
	if (bitmap[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] == 0)
		row = (uint16)(height - y - 1);
	else
		row = (uint16)y;

	uint16 column = ((flip & 1) != 0) ? (uint16)(pitch - x) : (uint16)x;

	uint32 offset = (uint32)row * pitch + column;
	if (offset >= (uint32)pitch * height) {
		warning("SpriteLibrary_v1_2::getBitmapPixel(): the pixel is outside the bitmap");
		return 0;
	}

	return bitmapPixels(bitmap)[offset];
}

void SpriteLibrary_v1_2::fillBitmapByte(byte *bitmap, int16 x, int16 y, uint16 width, uint16 height, uint16 color) {
	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 bitmapHeight = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool topDown;
	if (_engine->_version >= kVersionV1_2_5)
		topDown = ((READ_LE_UINT16(bitmap + TG_SPR_V1_2_5_BITMAP_FLAGS_OFFSET) & TG_SPR_V1_2_5_BITMAP_TOP_DOWN_FLAG) != 0);
	else
		topDown = (bitmap[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] != 0);

	uint16 row;
	if (!topDown)
		row = (uint16)(bitmapHeight - y - height);
	else
		row = (uint16)y;

	if (x < 0 || (uint32)row + height > bitmapHeight || (uint32)x + width > pitch) {
		warning("SpriteLibrary_v1_2::fillBitmapByte(): the rectangle is outside the bitmap");
		return;
	}

	byte *destination = bitmapPixels(bitmap) + (uint32)row * pitch + x;
	for (uint16 i = 0; i < height; i++) {
		memset(destination, color & 0xFF, width);
		destination += pitch;
	}
}

bool SpriteLibrary_v1_2::ensureComposeBufferSize(uint16 width, uint16 height) {
	uint16 pitch = (uint16)(width + 3) & 0xFFFC;
	uint32 size = (uint32)pitch * height + TG_SPR_V1_2_BITMAP_HEADER_SIZE;

	if (_engine->_version < kVersionV1_2_5)
		_composeBuffer[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] = 0;

	WRITE_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_WIDTH_OFFSET, width);
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_V1_2_BITMAP_PITCH_OFFSET, pitch);
	WRITE_LE_UINT16(_composeBuffer + TG_SPR_BITMAP_HEIGHT_OFFSET, height);

	if (size > _composeBufferCapacity) {
		byte *grown = engine()->utilsLibrary()->tgRealloc(_composeBuffer, size, 0);
		if (!grown)
			return false;

		_composeBuffer = grown;
		_composeBufferCapacity = size;
	}

	return true;
}

bool SpriteLibrary_v1_2::renderRect(const Common::Rect &rect) {
	Common::Rect visible;
	if (!intersectRectLocal(rect, _clipRect, visible))
		return true;

	if (!_composeBuffer) {
		warning("SpriteLibrary_v1_2::renderRect(): the compose buffer does not exist");
		return false;
	}

	uint16 width = (uint16)(visible.right - visible.left);
	uint16 height = (uint16)(visible.bottom - visible.top);
	if (!ensureComposeBufferSize(width, height))
		return false;

	if (_engine->_version == kVersionV1_2_6 && _rubberBandActive)
		rubberBandRepaint(visible);

	if (_scrollSurface)
		blitOpaqueRect(_composeBuffer, _scrollSurface, width, height, 0, 0, (int16)(visible.left + _scrollX), (int16)(visible.top + _scrollY), 0);
	else if (_backgroundBitmap)
		blitOpaqueRect(_composeBuffer, _backgroundBitmap, width, height, 0, 0, (int16)(visible.left - _scrollPosX + _scrollX), (int16)(visible.top - _scrollPosY + _scrollY), 0);
	else if (_fillColor != TG_SPR_V1_2_FILL_NONE)
		fillBitmapByte(_composeBuffer, 0, 0, width, height, _fillColor);

	compositeSpritesInRect(visible);
	blitSpriteToScreen(visible.left, visible.top, _composeBuffer);

	if (_engine->_version == kVersionV1_2_6 && _rubberBandActive)
		rubberBandRepaint(visible);

	return true;
}

bool SpriteLibrary_v1_2::repaintUnionOfCurrentAndPreviousRect(uint16 sprite) {
	SpriteObject_v1_2 *object = findSprite(sprite, "repaintUnionOfCurrentAndPreviousRect");
	if (!object)
		return false;

	Common::Rect combined;
	unionRectLocal(object->boundingBox, object->previousBox, combined);
	return renderRect(combined);
}

void SpriteLibrary_v1_2::compositeSpritesInRect(const Common::Rect &rect) {
	for (uint16 i = 0; i < _spriteCount; i++) {
		uint16 handle = _spriteTable[i];
		SpriteObject_v1_2 *object = findSprite(handle, "compositeSpritesInRect");
		if (!object || !object->visible)
			continue;

		Common::Rect common;
		if (!intersectRectLocal(rect, object->boundingBox, common))
			continue;

		if (object->partCount != 0) {
			compositeSpritePartsIntoBuffer(handle, rect);
			continue;
		}

		if (!object->currentFrame) {
			warning("SpriteLibrary_v1_2::compositeSpritesInRect(): the sprite has no frame");
			continue;
		}

		if (_engine->_version >= kVersionV1_2_7) {
			if (object->scrollable && !intersectRectLocal(common, _scrollRegion, common))
				continue;

			if (object->hasClipBox && !intersectRectLocal(common, object->clipBox, common))
				continue;
		}

		uint16 width = (uint16)(common.right - common.left);
		uint16 height = (uint16)(common.bottom - common.top);
		int16 destinationX = (int16)(common.left - rect.left);
		int16 destinationY = (int16)(common.top - rect.top);
		int16 sourceX = (int16)(common.left - object->boundingBox.left);
		int16 sourceY = (int16)(common.top - object->boundingBox.top);

		if (_bkgTransparent && handle != _topmostSprite)
			blitMaskedRect(_composeBuffer, object->currentFrame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
		else
			blitTransparentRect(_composeBuffer, object->currentFrame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
	}
}

void SpriteLibrary_v1_2::compositeSpritePartsIntoBuffer(uint16 sprite, const Common::Rect &rect) {
	SpriteObject_v1_2 *object = findSprite(sprite, "compositeSpritePartsIntoBuffer");
	if (!object)
		return;

	if (!object->parts) {
		warning("SpriteLibrary_v1_2::compositeSpritePartsIntoBuffer(): the multipart sprite has no parts");
		return;
	}

	for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &part = object->parts->parts[i];

		Common::Rect common;
		if (!intersectRectLocal(rect, part.box, common))
			continue;

		if (_engine->_version >= kVersionV1_2_7) {
			if (object->scrollable && !intersectRectLocal(common, _scrollRegion, common))
				continue;

			if (object->hasClipBox && !intersectRectLocal(common, object->clipBox, common))
				continue;
		}

		if (!part.frame) {
			warning("SpriteLibrary_v1_2::compositeSpritePartsIntoBuffer(): the sprite part has no frame");
			continue;
		}

		uint16 width = (uint16)(common.right - common.left);
		uint16 height = (uint16)(common.bottom - common.top);
		int16 destinationX = (int16)(common.left - rect.left);
		int16 destinationY = (int16)(common.top - rect.top);
		int16 sourceX = (int16)(common.left - part.box.left);
		int16 sourceY = (int16)(common.top - part.box.top);

		if (_bkgTransparent && sprite != _topmostSprite)
			blitMaskedRect(_composeBuffer, part.frame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
		else
			blitTransparentRect(_composeBuffer, part.frame, width, height, destinationX, destinationY, sourceX, sourceY, flipFlags(object));
	}
}

void SpriteLibrary_v1_2::compositeSpriteIntoBackground(uint16 sprite, uint16 flag) {
	if (!_backgroundBitmap)
		return;

	SpriteObject_v1_2 *object = findSprite(sprite, "compositeSpriteIntoBackground");
	if (!object || !object->visible)
		return;

	Common::Rect common;
	if (intersectRectLocal(_clipRect, object->boundingBox, common)) {
		if (object->partCount != 0) {
			compositeMultipartIntoBackground(sprite, common);
		} else if (!object->currentFrame) {
			warning("SpriteLibrary_v1_2::compositeSpriteIntoBackground(): the sprite has no frame");
		} else {
			blitTransparentRect(_backgroundBitmap, object->currentFrame, (uint16)(common.right - common.left), (uint16)(common.bottom - common.top),
			                    (int16)(common.left + _scrollX), (int16)(common.top + _scrollY),
			                    (int16)(common.left - object->boundingBox.left), (int16)(common.top - object->boundingBox.top), flipFlags(object));
		}
	}

	if (_engine->_version >= kVersionV1_2_1) {
		if (flag == 0) {
			spriteHide(sprite, false);
			spriteEmptyQueue(sprite);
		}
	} else {
		spriteHide(sprite, true);
	}
}

void SpriteLibrary_v1_2::spriteCompToBackground(uint16 sprite, uint16 flag) {
	if (sprite != 0) {
		compositeSpriteIntoBackground(sprite, flag);
		return;
	}

	for (uint16 i = 0; i < _spriteCount; i++)
		compositeSpriteIntoBackground(_spriteTable[i], flag);
}

void SpriteLibrary_v1_2::compositeMultipartIntoBackground(uint16 sprite, const Common::Rect &rect) {
	SpriteObject_v1_2 *object = findSprite(sprite, "compositeMultipartIntoBackground");
	if (!object)
		return;

	if (!object->parts) {
		warning("SpriteLibrary_v1_2::compositeMultipartIntoBackground(): the multipart sprite has no parts");
		return;
	}

	for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &part = object->parts->parts[i];

		Common::Rect common;
		if (!intersectRectLocal(rect, part.box, common))
			continue;

		if (!part.frame) {
			warning("SpriteLibrary_v1_2::compositeMultipartIntoBackground(): the sprite part has no frame");
			continue;
		}

		const Common::Rect &origin = (_engine->_version >= kVersionV1_2_1) ? part.box : object->boundingBox;

		blitTransparentRect(_backgroundBitmap, part.frame, (uint16)(common.right - common.left), (uint16)(common.bottom - common.top),
		                    (int16)(common.left + _scrollX), (int16)(common.top + _scrollY),
		                    (int16)(common.left - origin.left), (int16)(common.top - origin.top), flipFlags(object));
	}
}

void SpriteLibrary_v1_2::blitSpriteToScreen(int16 x, int16 y, const byte *bitmap) {
	uint16 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	const byte *pixels = bitmapPixels(bitmap);

	if (_zoomActive || _stretch) {
		int16 destinationX = (int16)((int16)((int32)(int16)_zoomFactor * x) - _zoomOffsetX);
		int16 destinationY = (int16)((int16)((int32)(int16)_zoomFactor * y) - _zoomOffsetY);

		engine()->display()->putBitmapRowsStretched(destinationX, destinationY, (uint16)(width * _zoomFactor), (uint16)(height * _zoomFactor), width, height, pixels, pitch, &_colorTable[0][0]);
	} else {
		engine()->display()->putBitmapRows(x, y, width, height, pixels, pitch, &_colorTable[0][0]);
	}
}

uint16 SpriteLibrary_v1_2::spritePicked(int16 x, int16 y) {
	if (_clipRect.left > x || _clipRect.right <= x || _clipRect.top > y || _clipRect.bottom <= y)
		return 0;

	for (int index = (int)_spriteCount - 1; index >= 0; index--) {
		uint16 handle = _spriteTable[index];
		if (spriteIsPicked(handle, x, y) != 0)
			return handle;
	}

	return 0;
}

uint16 SpriteLibrary_v1_2::spriteIsPicked(uint16 sprite, int16 x, int16 y) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteIsPicked");
	if (!object)
		return 0;

	if (_engine->_version >= kVersionV1_2_6) {
		if (object->pickMode == 2 || (!object->visible && object->pickMode != 1))
			return 0;
	} else if (!object->visible && !object->pickInvisible) {
		return 0;
	}

	const Common::Rect &box = object->boundingBox;
	if (box.left > x || box.right <= x || box.top > y || box.bottom <= y)
		return 0;

	if (object->pickWholeBox)
		return 1;

	if (object->partCount != 0) {
		if (!object->parts) {
			warning("SpriteLibrary_v1_2::spriteIsPicked(): the multipart sprite has no parts");
			return 0;
		}

		for (uint16 part = 0; part < object->partCount && part < TG_SPR_V1_2_NUM_SPRITE_PARTS; part++) {
			const SpritePart &spritePart = object->parts->parts[part];
			if (spritePart.box.left > x || spritePart.box.right <= x || spritePart.box.top > y || spritePart.box.bottom <= y)
				continue;

			if (!spritePart.frame) {
				warning("SpriteLibrary_v1_2::spriteIsPicked(): the sprite part has no frame");
				continue;
			}

			if (getBitmapPixel(spritePart.frame, (int16)(x - spritePart.box.left), (int16)(y - spritePart.box.top), flipFlags(object)) != 0)
				return 1;
		}

		return 0;
	}

	if (!object->currentFrame) {
		warning("SpriteLibrary_v1_2::spriteIsPicked(): the sprite has no frame");
		return 0;
	}

	return (getBitmapPixel(object->currentFrame, (int16)(x - box.left), (int16)(y - box.top), flipFlags(object)) != 0) ? 1 : 0;
}

void SpriteLibrary_v1_2::renderSpriteToSurface(uint16 sprite) {
	if (_engine->_version >= kVersionV1_2_3) {
		SpriteObject_v1_2 *object = findSprite(sprite, "renderSpriteToSurface");
		if (!object)
			return;

		object->renderPending = false;

		if (_engine->_version >= kVersionV1_2_6 && !object->active)
			return;
	}

	SpriteLibrary::renderSpriteToSurface(sprite);
}

uint16 SpriteLibrary_v1_2::spriteHitDetect(uint16 spriteA, uint16 spriteB) {
	SpriteObject_v1_2 *first = findSprite(spriteA, "spriteHitDetect");
	SpriteObject_v1_2 *second = findSprite(spriteB, "spriteHitDetect");
	if (!first || !second || !second->visible || !first->visible)
		return 0;

	Common::Rect overlap;
	if (!intersectRectLocal(first->boundingBox, second->boundingBox, overlap))
		return 0;

	if (first->partCount != 0 || second->partCount != 0)
		return 1;

	if (!first->currentFrame || !second->currentFrame) {
		warning("SpriteLibrary_v1_2::spriteHitDetect(): the sprite has no current frame");
		return 0;
	}

	int16 width = (int16)(overlap.right - overlap.left);
	int16 height = (int16)(overlap.bottom - overlap.top);
	int16 firstX = (int16)(overlap.left - first->boundingBox.left);
	int16 firstY = (int16)(overlap.top - first->boundingBox.top);
	int16 secondX = (int16)(overlap.left - second->boundingBox.left);
	int16 secondY = (int16)(overlap.top - second->boundingBox.top);

	if (first->mirrorX)
		firstX = (int16)(READ_LE_UINT16(first->currentFrame + TG_SPR_BITMAP_WIDTH_OFFSET) - firstX - width);

	if (first->flipY)
		firstY = (int16)(READ_LE_UINT16(first->currentFrame + TG_SPR_BITMAP_HEIGHT_OFFSET) - firstY - height);

	if (second->mirrorX)
		secondX = (int16)(READ_LE_UINT16(second->currentFrame + TG_SPR_BITMAP_WIDTH_OFFSET) - secondX - width);

	if (second->flipY)
		secondY = (int16)(READ_LE_UINT16(second->currentFrame + TG_SPR_BITMAP_HEIGHT_OFFSET) - secondY - height);

	return blitPixelOverlapTest(first->currentFrame, second->currentFrame, (uint16)width, (uint16)height, firstX, firstY, secondX, secondY, 0);
}

uint16 SpriteLibrary_v1_2_3::hitTestMultipartDispatch(const SpriteObject_v1_2 *first, const SpriteObject_v1_2 *second) {
	if (first->partCount != 0 && second->partCount != 0)
		return hitTestMultipartPair(first, second);

	if (first->partCount != 0)
		return hitTestMultipartVsSingle(second, first);

	return hitTestMultipartVsSingle(first, second);
}

uint16 SpriteLibrary_v1_2_3::hitTestMultipartPair(const SpriteObject_v1_2 *first, const SpriteObject_v1_2 *second) {
	if (!first->parts || !second->parts) {
		warning("SpriteLibrary_v1_2_3::hitTestMultipartPair(): the multipart sprite has no parts");
		return 0;
	}

	for (uint16 i = 0; i < first->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &firstPart = first->parts->parts[i];

		for (uint16 j = 0; j < second->partCount && j < TG_SPR_V1_2_NUM_SPRITE_PARTS; j++) {
			const SpritePart &secondPart = second->parts->parts[j];

			Common::Rect overlap;
			if (!intersectRectLocal(firstPart.box, secondPart.box, overlap))
				continue;

			if (!firstPart.frame || !secondPart.frame) {
				warning("SpriteLibrary_v1_2_3::hitTestMultipartPair(): the sprite part has no frame");
				continue;
			}

			uint16 hit = blitPixelOverlapTest(firstPart.frame, secondPart.frame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
			                                  (int16)(overlap.left - firstPart.box.left), (int16)(overlap.top - firstPart.box.top),
			                                  (int16)(overlap.left - secondPart.box.left), (int16)(overlap.top - secondPart.box.top),
			                                  flipFlags(first), flipFlags(second));
			if (hit != 0)
				return 1;
		}
	}

	return 0;
}

uint16 SpriteLibrary_v1_2_3::hitTestMultipartVsSingle(const SpriteObject_v1_2 *single, const SpriteObject_v1_2 *multipart) {
	if (!multipart->parts) {
		warning("SpriteLibrary_v1_2_3::hitTestMultipartVsSingle(): the multipart sprite has no parts");
		return 0;
	}

	for (uint16 i = 0; i < multipart->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++) {
		const SpritePart &part = multipart->parts->parts[i];

		Common::Rect overlap;
		if (!intersectRectLocal(single->boundingBox, part.box, overlap))
			continue;

		if (!single->currentFrame || !part.frame) {
			warning("SpriteLibrary_v1_2_3::hitTestMultipartVsSingle(): the sprite has no frame");
			continue;
		}

		uint16 hit = blitPixelOverlapTest(single->currentFrame, part.frame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		                                  (int16)(overlap.left - single->boundingBox.left), (int16)(overlap.top - single->boundingBox.top),
		                                  (int16)(overlap.left - part.box.left), (int16)(overlap.top - part.box.top),
		                                  flipFlags(single), flipFlags(multipart));
		if (hit != 0)
			return 1;
	}

	return 0;
}

uint16 SpriteLibrary_v1_2_3::spriteHitDetect(uint16 spriteA, uint16 spriteB, uint16 allowInvisibleA, uint16 allowInvisibleB, uint16 sameLevelOnly) {
	SpriteObject_v1_2 *first = findSprite(spriteA, "spriteHitDetect");
	SpriteObject_v1_2 *second = findSprite(spriteB, "spriteHitDetect");
	if (!first || !second)
		return 0;

	if (sameLevelOnly != 0 && first->level != second->level)
		return 0;

	if (!first->visible && allowInvisibleA == 0)
		return 0;

	if (!second->visible && allowInvisibleB == 0)
		return 0;

	Common::Rect overlap;
	if (!intersectRectLocal(first->boundingBox, second->boundingBox, overlap))
		return 0;

	if (first->partCount == 0 && second->partCount == 0) {
		if (!first->currentFrame || !second->currentFrame)
			return 0;

		return blitPixelOverlapTest(first->currentFrame, second->currentFrame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		                            (int16)(overlap.left - first->boundingBox.left), (int16)(overlap.top - first->boundingBox.top),
		                            (int16)(overlap.left - second->boundingBox.left), (int16)(overlap.top - second->boundingBox.top),
		                            flipFlags(first), flipFlags(second));
	}

	return hitTestMultipartDispatch(first, second);
}

uint16 SpriteLibrary_v1_2_3::spriteHitDetectRect(uint16 spriteA, uint16 spriteB, const Common::Rect &rect) {
	SpriteObject_v1_2 *first = findSprite(spriteA, "spriteHitDetectRect");
	SpriteObject_v1_2 *second = findSprite(spriteB, "spriteHitDetectRect");
	if (!first || !second)
		return 0;

	if (!first->visible || !second->visible)
		return 0;

	Common::Rect overlap;
	if (!intersectRectLocal(rect, second->boundingBox, overlap))
		return 0;

	if (first->partCount == 0 && second->partCount == 0) {
		if (!first->currentFrame || !second->currentFrame)
			return 0;

		return blitPixelOverlapTest(first->currentFrame, second->currentFrame, (uint16)(overlap.right - overlap.left), (uint16)(overlap.bottom - overlap.top),
		                            (int16)(overlap.left - first->boundingBox.left), (int16)(overlap.top - first->boundingBox.top),
		                            (int16)(overlap.left - second->boundingBox.left), (int16)(overlap.top - second->boundingBox.top),
		                            flipFlags(first), flipFlags(second));
	}

	return hitTestMultipartDispatch(first, second);
}

void SpriteLibrary_v1_2_3::spriteDibGetBox(const byte *bitmap, Common::Rect &box) {
	int16 width = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 height = (int16)READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 heightMinusOne = (int16)(height - 1);

	box.left = (int16)((int16)(1 - width) / 2 + READ_LE_INT16(bitmap + TG_SPR_BITMAP_HOT_X_OFFSET));
	box.right = (int16)(width + box.left);
	box.top = (int16)(READ_LE_INT16(bitmap + TG_SPR_BITMAP_HOT_Y_OFFSET) - heightMinusOne / 2);
	box.bottom = (int16)(height + box.top);
}

void SpriteLibrary_v1_2_3::spriteScreenShake(int16 deltaX, int16 deltaY) {
	Display *display = engine()->display();

	int16 clientWidth = display->width();
	int16 clientHeight = display->height();
	int16 widthDifference = (int16)(clientWidth - _clipRect.right + _clipRect.left - 1);
	int16 heightDifference = (int16)(clientHeight - _clipRect.bottom + _clipRect.top - 1);

	Common::Rect area = _clipRect;
	if (widthDifference < 0 || heightDifference < 0) {
		int16 inflateX = widthDifference / 2;
		int16 inflateY = heightDifference / 2;
		area.left = (int16)(area.left - inflateX);
		area.right = (int16)(area.right + inflateX);
		area.top = (int16)(area.top - inflateY);
		area.bottom = (int16)(area.bottom + inflateY);
	}

	display->scrollDevice(area, deltaX, deltaY);
	display->scrollDevice(area, (int16)(-2 * deltaX), (int16)(-2 * deltaY));
	display->scrollDevice(area, deltaX, deltaY);

	int16 stripWidth = (deltaX < 0) ? (int16)-deltaX : deltaX;
	int16 stripHeight = (deltaY < 0) ? (int16)-deltaY : deltaY;

	if (stripWidth != 0) {
		Common::Rect strip = area;
		strip.right = (int16)(stripWidth + strip.left);
		spritePaintRect(strip, false);

		strip.right = area.right;
		strip.left = (int16)(area.right - stripWidth);
		spritePaintRect(strip, false);
	}

	if (stripHeight != 0) {
		Common::Rect strip = area;
		strip.bottom = (int16)(stripHeight + strip.top);
		spritePaintRect(strip, false);

		strip.bottom = area.bottom;
		strip.top = (int16)(area.bottom - stripHeight);
		spritePaintRect(strip, false);
	}
}

bool SpriteLibrary_v1_2::isRleBitmap(const byte *bitmap) const {
	if (_engine->_version >= kVersionV1_2_5)
		return (READ_LE_UINT16(bitmap + TG_SPR_V1_2_5_BITMAP_FLAGS_OFFSET) & TG_SPR_V1_2_5_BITMAP_RLE_FLAG) != 0;

	return (bitmap[TG_SPR_V1_2_3_BITMAP_FLAGS_OFFSET] & TG_SPR_V1_2_3_BITMAP_RLE_FLAG) != 0;
}

void SpriteLibrary_v1_2_3::decodeRleRow(const byte *bitmap, uint16 row, uint16 firstColumn, uint16 count, byte *values, byte *forced) const {
	const byte *pixels = bitmapPixels(bitmap);
	const byte *position = pixels + READ_LE_UINT16(pixels + 2 * row);

	uint32 column = 0;
	uint16 filled = 0;

	while (filled < count) {
		byte code = *position++;
		if (code == 0)
			break;

		uint16 length;
		bool literal = false;
		bool tested = false;
		byte runValue = 0;

		if (code <= TG_SPR_V1_2_3_RLE_LAST_RUN_CODE) {
			length = code;
			runValue = *position++;
		} else {
			length = (uint16)(TG_SPR_V1_2_3_RLE_TESTED_LITERAL_CODE - code);
			literal = true;
			if (length == 0) {
				length = *position++;
				tested = true;
			}
		}

		for (uint16 i = 0; i < length; i++) {
			byte value = literal ? *position++ : runValue;
			if (column >= firstColumn && filled < count) {
				values[filled] = value;
				forced[filled] = (literal && !tested) ? 1 : 0;
				filled++;
			}

			column++;
		}
	}

	if (filled < count) {
		warning("SpriteLibrary_v1_2_3::decodeRleRow(): the row ends before the requested pixels");
		for (; filled < count; filled++) {
			values[filled] = 0;
			forced[filled] = 0;
		}
	}
}

bool SpriteLibrary_v1_2_3::blitSetupRleSource(byte *destination, const byte *source, uint16 &width, uint16 &height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, RleBlitSetup &setup) {
	uint16 sourceWidth = READ_LE_UINT16(source + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 sourceHeight = READ_LE_UINT16(source + TG_SPR_BITMAP_HEIGHT_OFFSET);
	uint16 destinationWidth = READ_LE_UINT16(destination + TG_SPR_BITMAP_WIDTH_OFFSET);
	uint16 destinationPitch = READ_LE_UINT16(destination + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	uint16 destinationHeight = READ_LE_UINT16(destination + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool inside = (destinationX >= 0 && destinationY >= 0 && sourceX >= 0 && sourceY >= 0 &&
	               destinationX + width <= destinationWidth && destinationY + height <= destinationHeight &&
	               sourceX + width <= sourceWidth && sourceY + height <= sourceHeight);

	if (!inside) {
		warning("SpriteLibrary_v1_2_3::blitSetupRleSource(): the rectangle is outside the bitmaps");

		if (destinationX < 0 || destinationY < 0 || sourceX < 0 || sourceY < 0)
			return false;

		int availableWidth = MIN(sourceWidth - sourceX, destinationWidth - destinationX);
		int availableHeight = MIN(sourceHeight - sourceY, destinationHeight - destinationY);
		if (availableWidth <= 0 || availableHeight <= 0)
			return false;

		width = MIN((int)width, availableWidth);
		height = MIN((int)height, availableHeight);
	}

	bool mirror = ((flags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0);
	bool flip = ((flags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0);

	if (mirror)
		setup.firstColumn = (uint16)(sourceWidth - width - sourceX);
	else
		setup.firstColumn = (uint16)sourceX;

	if (flip)
		setup.firstRow = (int32)sourceHeight - sourceY - 1;
	else
		setup.firstRow = (int32)sourceY + height - 1;

	bool topDown;
	if (_engine->_version >= kVersionV1_2_5)
		topDown = ((READ_LE_UINT16(destination + TG_SPR_V1_2_5_BITMAP_FLAGS_OFFSET) & TG_SPR_V1_2_5_BITMAP_TOP_DOWN_FLAG) != 0);
	else
		topDown = (destination[TG_SPR_V1_2_BITMAP_ORIENTATION_OFFSET] == 1);

	int32 destinationRow;
	if (!topDown) {
		if (flip) {
			destinationRow = (int32)destinationHeight - destinationY - 1;
			setup.destinationStride = -(int32)destinationPitch;
		} else {
			destinationRow = (int32)destinationHeight - destinationY - height;
			setup.destinationStride = destinationPitch;
		}
	} else {
		if (flip) {
			destinationRow = destinationY;
			setup.destinationStride = destinationPitch;
		} else {
			destinationRow = (int32)destinationY + height - 1;
			setup.destinationStride = -(int32)destinationPitch;
		}
	}

	setup.destination = bitmapPixels(destination) + destinationRow * (int32)destinationPitch + destinationX;
	return true;
}

void SpriteLibrary_v1_2_3::blitRleRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags, RleBlitMode_v1_2_3 mode) {
	RleBlitSetup setup;
	if (!blitSetupRleSource(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, setup))
		return;

	Common::Array<byte> values;
	Common::Array<byte> forced;
	values.resize(width);
	forced.resize(width);

	bool mirror = ((flags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0);

	for (uint16 row = 0; row < height; row++) {
		decodeRleRow(source, (uint16)(setup.firstRow - row), setup.firstColumn, width, &values[0], &forced[0]);

		byte *line = setup.destination + (int32)row * setup.destinationStride;
		for (uint16 column = 0; column < width; column++) {
			uint16 index = mirror ? (uint16)(width - 1 - column) : column;
			byte value = values[index];
			bool opaque = (value != 0 || forced[index] != 0);

			switch (mode) {
			case kSprV1_2_3BlitCopy:
				line[column] = value;
				break;

			case kSprV1_2_3BlitTransparent:
				if (opaque)
					line[column] = value;

				break;

			case kSprV1_2_3BlitMasked:
				if (opaque && line[column] != 0)
					line[column] = value;

				break;
			}
		}
	}
}

void SpriteLibrary_v1_2_3::blitOpaqueRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (isRleBitmap(source))
		blitRleRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, kSprV1_2_3BlitCopy);
	else
		SpriteLibrary_v1_2::blitOpaqueRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags);
}

void SpriteLibrary_v1_2_3::blitTransparentRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (isRleBitmap(source))
		blitRleRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, kSprV1_2_3BlitTransparent);
	else
		SpriteLibrary_v1_2::blitTransparentRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags);
}

void SpriteLibrary_v1_2_3::blitMaskedRect(byte *destination, const byte *source, uint16 width, uint16 height, int16 destinationX, int16 destinationY, int16 sourceX, int16 sourceY, uint16 flags) {
	if (width == 0 || height == 0)
		return;

	if (isRleBitmap(source))
		blitRleRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags, kSprV1_2_3BlitMasked);
	else
		SpriteLibrary_v1_2::blitMaskedRect(destination, source, width, height, destinationX, destinationY, sourceX, sourceY, flags);
}

void SpriteLibrary_v1_2_3::fetchBitmapRow(const byte *bitmap, int32 row, int32 firstColumn, uint16 count, byte *values) const {
	int32 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int32 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	memset(values, 0, count);

	if (row < 0 || row >= height)
		return;

	int32 from = MAX(firstColumn, (int32)0);
	int32 to = MIN(firstColumn + count, width);
	if (from >= to)
		return;

	uint16 length = (uint16)(to - from);

	if (isRleBitmap(bitmap)) {
		Common::Array<byte> forced;
		forced.resize(length);
		decodeRleRow(bitmap, (uint16)row, (uint16)from, length, values + (from - firstColumn), &forced[0]);
	} else {
		uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
		const byte *line = bitmapPixels(bitmap) + (int32)(height - 1 - row) * pitch;
		memcpy(values + (from - firstColumn), line + from, length);
	}
}

uint16 SpriteLibrary_v1_2_3::blitPixelOverlapTest(const byte *first, const byte *second, uint16 width, uint16 height, int16 firstX, int16 firstY, int16 secondX, int16 secondY, uint16 firstFlags, uint16 secondFlags) {
	if (width == 0 || height == 0)
		return 0;

	int16 firstWidth = (int16)READ_LE_UINT16(first + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 firstHeight = (int16)READ_LE_UINT16(first + TG_SPR_BITMAP_HEIGHT_OFFSET);
	int16 secondWidth = (int16)READ_LE_UINT16(second + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 secondHeight = (int16)READ_LE_UINT16(second + TG_SPR_BITMAP_HEIGHT_OFFSET);

	if ((firstFlags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0)
		firstX = (int16)(firstWidth - firstX - width);

	if ((firstFlags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0)
		firstY = (int16)(firstHeight - firstY - height);

	if ((secondFlags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0)
		secondX = (int16)(secondWidth - secondX - width);

	if ((secondFlags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0)
		secondY = (int16)(secondHeight - secondY - height);

	if (firstX < 0 || firstY < 0 || firstX + width > firstWidth || firstY + height > firstHeight ||
	    secondX < 0 || secondY < 0 || secondX + width > secondWidth || secondY + height > secondHeight)
		warning("SpriteLibrary_v1_2_3::blitPixelOverlapTest(): the rectangle is outside the bitmaps");

	Common::Array<byte> firstRow;
	Common::Array<byte> secondRow;
	firstRow.resize(width);
	secondRow.resize(width);

	for (uint16 row = 0; row < height; row++) {
		fetchBitmapRow(first, firstY + row, firstX, width, &firstRow[0]);
		fetchBitmapRow(second, secondY + row, secondX, width, &secondRow[0]);

		for (uint16 column = 0; column < width; column++) {
			if (firstRow[column] != 0 && secondRow[column] != 0)
				return 1;
		}
	}

	return 0;
}

uint16 SpriteLibrary_v1_2_3::getBitmapPixel(const byte *bitmap, int16 x, int16 y, uint16 flags) {
	int32 width = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_WIDTH_OFFSET);
	int32 height = READ_LE_UINT16(bitmap + TG_SPR_BITMAP_HEIGHT_OFFSET);

	bool mirror = ((flags & TG_SPR_V1_2_3_MIRROR_FLAG_MASK) != 0);
	bool flip = ((flags & TG_SPR_V1_2_3_FLIP_FLAG_MASK) != 0);

	int32 column = mirror ? width - x - 1 : x;
	int32 row = flip ? height - y - 1 : y;

	if (column < 0 || column >= width || row < 0 || row >= height) {
		warning("SpriteLibrary_v1_2_3::getBitmapPixel(): the pixel is outside the bitmap");
		return 0;
	}

	if (isRleBitmap(bitmap)) {
		byte value = 0;
		byte forced = 0;
		decodeRleRow(bitmap, (uint16)row, (uint16)column, 1, &value, &forced);
		return value;
	}

	uint16 pitch = READ_LE_UINT16(bitmap + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	return bitmapPixels(bitmap)[(height - 1 - row) * pitch + column];
}

void SpriteLibrary_v1_2_3::scrollSprite(uint16 sprite, int16 deltaX, int16 deltaY) {
	SpriteObject_v1_2 *object = findSprite(sprite, "scrollSprite");
	if (!object)
		return;

	if (_engine->_version == kVersionV1_2_6) {
		object->x = (int16)(object->x + deltaX);
		object->y = (int16)(object->y + deltaY);
	}

	object->boundingBox.translate(deltaX, deltaY);
	object->previousBox.translate(deltaX, deltaY);

	if (object->parts) {
		for (uint16 i = 0; i < object->partCount && i < TG_SPR_V1_2_NUM_SPRITE_PARTS; i++)
			object->parts->parts[i].box.translate(deltaX, deltaY);
	}

	byte *command = object->queueCurrent;
	if (!command)
		return;

	if (READ_LE_UINT16(command) == kSprV1_2_3OpQuadPath) {
		for (uint16 offset = 0x0E; offset <= 0x16; offset += 4) {
			WRITE_LE_UINT16(command + offset, (uint16)(READ_LE_UINT16(command + offset) + deltaX));
			WRITE_LE_UINT16(command + offset + 2, (uint16)(READ_LE_UINT16(command + offset + 2) + deltaY));
		}
	} else if (READ_LE_UINT16(command) == kSprV1_2_3OpLinearMove) {
		for (uint16 offset = 0x0C; offset <= 0x10; offset += 4) {
			WRITE_LE_UINT16(command + offset, (uint16)(READ_LE_UINT16(command + offset) + deltaX));
			WRITE_LE_UINT16(command + offset + 2, (uint16)(READ_LE_UINT16(command + offset + 2) + deltaY));
		}
	}
}

void SpriteLibrary_v1_2_3::spriteScrollScreen(int16 deltaX, int16 deltaY) {
	engine()->display()->scrollDevice(_clipRect, deltaX, deltaY);

	for (uint16 i = 0; i < _spriteCount; i++)
		scrollSprite(_spriteTable[i], deltaX, deltaY);

	if (deltaX != 0) {
		Common::Rect strip = _clipRect;
		if (deltaX < 0)
			strip.left = (int16)(strip.right + deltaX);
		else
			strip.right = (int16)(strip.left + deltaX);

		spritePaintRect(strip, false);
	}

	if (deltaY != 0) {
		Common::Rect strip = _clipRect;
		if (deltaY < 0)
			strip.top = (int16)(strip.bottom + deltaY);
		else
			strip.bottom = (int16)(strip.top + deltaY);

		spritePaintRect(strip, false);
	}
}

bool SpriteLibrary_v1_2_3::clampScrollOffsetToBounds(Common::Point &delta, Common::Rect &outRect) {
	if (!_backgroundBitmap) {
		outRect = _clipRect;
		return (delta.x | delta.y) != 0;
	}

	outRect = _scrollRegion;

	Common::Rect bounds = _backupRect;
	if (_tileBackground)
		bounds = _tileBackground->bounds;

	int16 value = (int16)(delta.x + _scrollPosX + bounds.left);
	if (value > outRect.left) {
		value = (int16)(outRect.left - bounds.left);
		delta.x = (int16)(value - _scrollPosX);
	} else {
		value = (int16)(delta.x + _scrollPosX + bounds.right);
		if (value < outRect.right) {
			value = (int16)(outRect.right - bounds.right);
			delta.x = (int16)(value - _scrollPosX);
		}
	}

	value = (int16)(delta.y + _scrollPosY + bounds.top);
	if (value > outRect.top) {
		value = (int16)(outRect.top - bounds.top);
		delta.y = (int16)(value - _scrollPosY);
	} else {
		value = (int16)(delta.y + _scrollPosY + bounds.bottom);
		if (value < outRect.bottom) {
			value = (int16)(outRect.bottom - bounds.bottom);
			delta.y = (int16)(value - _scrollPosY);
		}
	}

	return (delta.x | delta.y) != 0;
}

void SpriteLibrary_v1_2_3::spriteSetScrollPos(int16 x, int16 y) {
	Common::Point delta((int16)(x - _scrollPosX), (int16)(y - _scrollPosY));
	Common::Rect region;

	if (!clampScrollOffsetToBounds(delta, region))
		return;

	_scrollPosX = (int16)(_scrollPosX + delta.x);
	_scrollPosY = (int16)(_scrollPosY + delta.y);

	for (uint16 i = 0; i < _spriteCount; i++) {
		SpriteObject_v1_2 *object = lookupSprite(_spriteTable[i]);
		if (object && object->scrollable)
			scrollSprite(_spriteTable[i], delta.x, delta.y);
	}

	if (_scrollSurface)
		renderTileBackgroundRect(&_scrollRegion);

	spritePaintRect(_scrollRegion, false);
}

Common::Point SpriteLibrary_v1_2_3::spriteGetScrollPos() {
	return Common::Point(_scrollPosX, _scrollPosY);
}

void SpriteLibrary_v1_2_3::spriteScroll(int16 deltaX, int16 deltaY, bool useScrollDevice) {
	Common::Point delta(deltaX, deltaY);
	Common::Rect region;

	if (!clampScrollOffsetToBounds(delta, region))
		return;

	_scrollPosX = (int16)(_scrollPosX + delta.x);
	_scrollPosY = (int16)(_scrollPosY + delta.y);

	if (useScrollDevice)
		engine()->display()->scrollDevice(region, delta.x, delta.y);

	for (uint16 i = 0; i < _spriteCount; i++) {
		SpriteObject_v1_2 *object = lookupSprite(_spriteTable[i]);
		if (object && object->scrollable)
			scrollSprite(_spriteTable[i], delta.x, delta.y);
	}

	if (_scrollSurface)
		performScrollOperation(_scrollSurface, delta.x, delta.y, region);

	if (!useScrollDevice) {
		spritePaintRect(_scrollRegion, false);
		return;
	}

	if (delta.x != 0)
		repaintScrollBoxRevealedColumn(delta.x, region);

	if (delta.y != 0)
		repaintScrollBoxRevealedRow(delta.y, region);
}

void SpriteLibrary_v1_2_3::repaintScrollBoxRevealedColumn(int16 deltaX, Common::Rect rect) {
	if (deltaX < 0)
		rect.left = (int16)(rect.right + deltaX);
	else
		rect.right = (int16)(rect.left + deltaX);

	if (_tileBackground)
		renderTileBackgroundRect(&rect);

	spritePaintRect(rect, false);
}

void SpriteLibrary_v1_2_3::repaintScrollBoxRevealedRow(int16 deltaY, Common::Rect rect) {
	if (deltaY < 0)
		rect.top = (int16)(rect.bottom + deltaY);
	else
		rect.bottom = (int16)(rect.top + deltaY);

	if (_tileBackground)
		renderTileBackgroundRect(&rect);

	spritePaintRect(rect, false);
}

void SpriteLibrary_v1_2_3::performScrollOperation(byte *surface, int16 deltaX, int16 deltaY, const Common::Rect &rect) {
	int32 width = READ_LE_UINT16(surface + TG_SPR_BITMAP_WIDTH_OFFSET);
	int32 pitch = READ_LE_UINT16(surface + TG_SPR_V1_2_BITMAP_PITCH_OFFSET);
	int32 height = READ_LE_UINT16(surface + TG_SPR_BITMAP_HEIGHT_OFFSET);

	int32 rows = (rect.bottom - rect.top) - ABS((int32)deltaY);
	int32 bytes = (rect.right - rect.left) - ABS((int32)deltaX);
	if (rows <= 0 || bytes <= 0)
		return;

	// The surface rows are stored bottom up
	int32 firstColumn = (width - 1) / 2 + rect.left;
	int32 firstRow = height - (rect.top + height / 2);

	int32 sourceColumn = firstColumn;
	int32 destinationColumn = firstColumn;
	if (deltaX < 0)
		sourceColumn -= deltaX;
	else
		destinationColumn += deltaX;

	int32 sourceRow = 0;
	int32 destinationRow = 0;
	if (deltaY < 0)
		sourceRow = -deltaY;
	else
		destinationRow = deltaY;

	int32 highestRow = firstRow - MIN(sourceRow, destinationRow);
	int32 lowestRow = firstRow - (MAX(sourceRow, destinationRow) + rows - 1);
	if (lowestRow < 0 || highestRow >= height || MIN(sourceColumn, destinationColumn) < 0 || MAX(sourceColumn, destinationColumn) + bytes > pitch) {
		warning("SpriteLibrary_v1_2_3::performScrollOperation(): the scrolled rectangle is outside the surface");
		return;
	}

	byte *pixels = bitmapPixels(surface);
	for (int32 i = 0; i < rows; i++) {
		int32 index = (deltaY > 0) ? rows - 1 - i : i;
		byte *destination = pixels + (firstRow - (destinationRow + index)) * pitch + destinationColumn;
		byte *source = pixels + (firstRow - (sourceRow + index)) * pitch + sourceColumn;
		memmove(destination, source, bytes);
	}
}

void SpriteLibrary_v1_2_3::spriteSetScrollBox(const Common::Rect *box) {
	Common::Rect oldRegion = _scrollRegion;

	if (box) {
		_scrollBox = *box;

		if (_scrollBox.left < _scrollBox.right && _scrollBox.top < _scrollBox.bottom) {
			_scrollBox.right++;
			_scrollBox.bottom++;
		}
	} else {
		_scrollBox = Common::Rect();
	}

	setScrollRegion(_scrollBox);

	if (oldRegion.left != _scrollRegion.left || oldRegion.right != _scrollRegion.right ||
	    oldRegion.top != _scrollRegion.top || oldRegion.bottom != _scrollRegion.bottom) {
		Common::Rect combined;
		unionRectLocal(oldRegion, _scrollRegion, combined);
		spritePaintRect(combined, false);
	}
}

Common::Rect SpriteLibrary_v1_2_3::spriteGetScrollBox() {
	return _scrollBox;
}

void SpriteLibrary_v1_2_3::spriteSetScrollable(uint16 sprite, uint16 flag) {
	SpriteObject_v1_2 *object = findSprite(sprite, "spriteSetScrollable");
	if (!object)
		return;

	bool scrollable = (flag != 0);
	if (scrollable == object->scrollable)
		return;

	if (scrollable)
		scrollSprite(sprite, _scrollPosX, _scrollPosY);
	else
		scrollSprite(sprite, (int16)-_scrollPosX, (int16)-_scrollPosY);

	object->scrollable = scrollable;
}

bool SpriteLibrary_v1_2_3::spriteSetTileBackground(TileBackground *tile, Common::Rect *outClipRect) {
	byte *surface = nullptr;
	if (tile)
		surface = initTileBackgroundLayout(tile);

	if (!surface) {
		spriteSetBackground(nullptr, false, 0, outClipRect);
		return true;
	}

	_fillColor = 0;
	_tileBackground = tile;

	if (_scrollSurface)
		spriteDibFree(_scrollSurface);

	_scrollSurface = surface;
	_backgroundBitmap = surface;
	computeClipRect();

	if (outClipRect)
		*outClipRect = _clipRect;

	renderTileBackgroundRect(&_scrollRegion);
	return true;
}

byte *SpriteLibrary_v1_2_3::initTileBackgroundLayout(TileBackground *tile) {
	uint16 columns = tile->columns;
	uint16 rows = tile->rows;
	uint16 tileWidth = tile->tileWidth;
	uint16 tileHeight = tile->tileHeight;

	bool staggered = (tile->layout == TG_SPR_V1_2_7_TILE_STAGGERED_LAYOUT);
	if (tileWidth == 0 || tileHeight == 0 || (staggered && tileHeight < 2)) {
		warning("SpriteLibrary_v1_2_3::initTileBackgroundLayout(): the tile size %dx%d is not valid", tileWidth, tileHeight);
		return nullptr;
	}

	Common::Rect bounds;

	if (staggered) {
		int16 halfWidth = (int16)(tileWidth >> 1);
		int16 halfHeight = (int16)(tileHeight >> 1);
		int16 parityShift = 0;
		if ((rows & 1) == 0)
			parityShift = (int16)(halfHeight / 2);

		tile->width = (int16)(columns * tileWidth + halfWidth);
		tile->halfWidth = (int16)(tile->width / 2);
		tile->height = (int16)(halfHeight * (int16)(rows + 1));
		tile->halfHeight = (int16)(tile->height / 2);

		bounds.left = (int16)(halfWidth / 2 - tile->halfWidth + halfWidth);
		bounds.top = (int16)(parityShift - tile->halfHeight + halfHeight);
		bounds.right = (int16)(tile->width - halfWidth + bounds.left - tileWidth);
		bounds.bottom = (int16)(bounds.top + (int16)(tile->height - parityShift * 2 - tileHeight));
	} else {
		tile->width = (int16)(columns * tileWidth);
		tile->halfWidth = (int16)(tile->width / 2);
		tile->height = (int16)(tileHeight * rows);
		tile->halfHeight = (int16)(tile->height / 2);

		bounds.left = (int16)-tile->halfWidth;
		bounds.top = (int16)-tile->halfHeight;
		bounds.right = (int16)(tile->width + bounds.left);
		bounds.bottom = (int16)(tile->height + bounds.top);
	}

	tile->bounds = bounds;

	Display *display = engine()->display();
	byte *surface = spriteDibCreate(display->width(), display->height(), 1);
	if (!surface)
		return nullptr;

	int16 surfaceWidth = (int16)READ_LE_UINT16(surface + TG_SPR_BITMAP_WIDTH_OFFSET);
	int16 surfaceHeight = (int16)READ_LE_UINT16(surface + TG_SPR_BITMAP_HEIGHT_OFFSET);

	Common::Rect surfaceRect;
	surfaceRect.left = 0;
	surfaceRect.top = 0;
	surfaceRect.right = surfaceWidth;
	surfaceRect.bottom = surfaceHeight;

	tile->centerX = (int16)((surfaceWidth - 1) / 2);
	tile->centerY = (int16)((surfaceHeight - 1) / 2);
	surfaceRect.translate(-tile->centerX, -tile->centerY);

	intersectRectLocal(surfaceRect, bounds, tile->visible);
	return surface;
}

void SpriteLibrary_v1_2_3::renderTileBackgroundRect(const Common::Rect *rect) {
	if (!_tileBackground)
		return;

	_tileClip = _scrollRegion;

	if (rect && !intersectRectLocal(_tileClip, *rect, _tileClip))
		return;

	if (!intersectRectLocal(_tileClip, _tileBackground->visible, _tileClip))
		return;

	if (_tileBackground->layout == TG_SPR_V1_2_7_TILE_STAGGERED_LAYOUT)
		renderStaggeredTileRows();
	else
		renderGridTiles();
}

void SpriteLibrary_v1_2_3::renderStaggeredTileRows() {
	TileBackground *tile = _tileBackground;
	_tileSurface = _scrollSurface;

	int16 rows = (int16)tile->rows;
	int16 tileHeight = (int16)tile->tileHeight;
	int16 halfHeight = (int16)(tileHeight >> 1);
	int16 parityShift = 0;
	if ((rows & 1) == 0)
		parityShift = (int16)(halfHeight / 2);

	int16 verticalOffset = (int16)(tile->halfHeight - parityShift - _scrollPosY);
	int16 row = (int16)((int16)(verticalOffset + _tileClip.top) / halfHeight - 1);
	int16 sourceY = (int16)(verticalOffset - (int16)(row * halfHeight) + _tileClip.top);
	int16 height = (int16)(tileHeight - sourceY);
	bool shifted = ((row & 1) != 0);

	_tileDestOriginX = tile->centerX;
	_tileDestOriginY = tile->centerY;
	_tileCountX = tile->columns;
	_tileWidth = tile->tileWidth;
	_tileHalfWidth = (int16)((int16)_tileWidth >> 1);

	int16 horizontalOffset = (int16)(tile->halfWidth - (_tileHalfWidth >> 1) - _scrollPosX);
	int16 column = (int16)((int16)(horizontalOffset + _tileClip.left) / (int16)_tileWidth);
	int16 sourceX = (int16)(horizontalOffset - (int16)(column * (int16)_tileWidth) + _tileClip.left);
	int16 visibleWidth = (int16)((int16)_tileWidth - sourceX);

	uint32 cellIndex = (uint16)((uint16)((uint16)row * _tileCountX) + (uint16)column);
	int16 count = (int16)((_tileClip.bottom - _tileClip.top) * 2);
	if (count <= 0)
		return;

	int16 destinationY = _tileClip.top;

	for (;;) {
		renderStaggeredTileRow(cellIndex, column, destinationY, sourceX, sourceY, visibleWidth, height, shifted);

		row++;
		if (row >= rows)
			return;

		cellIndex += _tileCountX;
		count = (int16)(count - height);
		destinationY = (int16)(destinationY + height - halfHeight);

		if (destinationY < _tileClip.top) {
			sourceY = (int16)(_tileClip.top - destinationY);
			destinationY = _tileClip.top;
		} else {
			sourceY = 0;
		}

		height = (int16)(tileHeight - sourceY);
		renderStaggeredTileRow(cellIndex, column, destinationY, sourceX, sourceY, visibleWidth, height, !shifted);

		row++;
		if (row >= rows)
			return;

		cellIndex += _tileCountX;
		count = (int16)(count - height);
		destinationY = (int16)(destinationY + height - halfHeight);
		sourceY = 0;
		height = tileHeight;

		if (count <= 0)
			return;
	}
}

void SpriteLibrary_v1_2_3::renderStaggeredTileRow(uint32 cellIndex, int16 column, int16 y, int16 sourceX, int16 sourceY, int16 visibleWidth, int16 height, bool shifted) {
	if (_tileClip.bottom - y < height)
		height = (int16)(_tileClip.bottom - y);

	if (height <= 0)
		return;

	int16 width = visibleWidth;

	if (shifted) {
		if (_tileHalfWidth >= width) {
			width = (int16)(width + _tileHalfWidth);
		} else {
			width = (int16)(width - _tileHalfWidth);
			if (column != 0)
				cellIndex--;
		}

		sourceX = (int16)((int16)_tileWidth - width);
	}

	int16 destinationY = (int16)(y + _tileDestOriginY);
	int16 x = _tileClip.left;
	if (x >= _tileClip.right)
		return;

	for (;;) {
		if (_tileClip.right - x < width)
			width = (int16)(_tileClip.right - x);

		if (width <= 0)
			return;

		byte *cell = nullptr;
		if (cellIndex < _tileBackground->cellCount)
			cell = _tileBackground->cells[cellIndex];
		else
			warning("SpriteLibrary_v1_2_3::renderStaggeredTileRow(): the tile cell %d does not exist", (int)cellIndex);

		if (cell)
			blitTransparentRect(_tileSurface, cell, (uint16)width, (uint16)height, (int16)(_tileDestOriginX + x), destinationY, sourceX, sourceY, 0);

		x = (int16)(x + width);
		width = (int16)_tileWidth;
		sourceX = 0;
		cellIndex++;

		if (_tileClip.right <= x)
			return;
	}
}

void SpriteLibrary_v1_2_3::renderGridTiles() {
	TileBackground *tile = _tileBackground;
	byte *surface = _scrollSurface;
	uint16 columns = tile->columns;

	int16 offsetX = (int16)(tile->halfWidth - _scrollPosX);
	int16 offsetY = (int16)(tile->halfHeight - _scrollPosY);
	uint16 row = (uint16)((uint16)(offsetY + _tileClip.top) / tile->tileHeight);
	int16 y = _tileClip.top;

	if (y >= _tileClip.bottom)
		return;

	uint16 rowBase = (uint16)(row * columns);

	for (;;) {
		uint16 column = (uint16)((uint16)(offsetX + _tileClip.left) / tile->tileWidth);
		if (column >= columns)
			return;

		int16 sourceY = (int16)(y + offsetY - (uint16)(tile->tileHeight * row));
		int16 height = (int16)(tile->tileHeight - sourceY);
		if (height > _tileClip.bottom - y)
			height = (int16)(_tileClip.bottom - y);

		int16 x = _tileClip.left;
		if (x < _tileClip.right) {
			for (;;) {
				int16 sourceX = (int16)(offsetX - (uint16)(tile->tileWidth * column) + x);
				int16 width = (int16)(tile->tileWidth - sourceX);
				if (width > _tileClip.right - x)
					width = (int16)(_tileClip.right - x);

				uint16 index = (uint16)(column + rowBase);
				byte *cell = nullptr;
				if (index < tile->cellCount)
					cell = tile->cells[index];
				else
					warning("SpriteLibrary_v1_2_3::renderGridTiles(): the tile cell %d does not exist", index);

				if (cell) {
					uint16 cellFlags = READ_LE_UINT16(cell + TG_SPR_V1_2_5_BITMAP_FLAGS_OFFSET);
					uint16 flags = 0;
					if ((cellFlags & TG_SPR_V1_2_7_TILE_MIRROR_FLAG) != 0)
						flags |= 1;

					if ((cellFlags & TG_SPR_V1_2_7_TILE_FLIP_FLAG) != 0)
						flags |= 0x100;

					blitTransparentRect(surface, cell, (uint16)width, (uint16)height, (int16)(tile->centerX + x), (int16)(tile->centerY + y), sourceX, sourceY, flags);
				}

				column++;
				x = (int16)(x + width);

				if (x >= _tileClip.right)
					break;
			}
		}

		rowBase = (uint16)(rowBase + columns);
		row++;
		y = (int16)(y + height);

		if (y >= _tileClip.bottom)
			return;
	}
}

void SpriteLibrary_v1_2_3::spriteBoxInBackground(byte *bitmap, uint16 steps, bool insetBoth) {
	if (!engine()->display()->hasPalette() && !_backgroundBitmap) {
		bool bkgTransparent = _bkgTransparent;
		_bkgTransparent = false;
		spritePaintRect(_clipRect, false);
		_bkgTransparent = bkgTransparent;
		return;
	}

	if (!_backgroundBitmap)
		return;

	_wipeExtentX = (uint16)(_backupRect.right - _backupRect.left);
	_wipeExtentY = (uint16)(_backupRect.bottom - _backupRect.top);
	_wipeNewBitmap = bitmap;
	_wipeDrawFromNew = true;
	wipeBoxFromCenter(insetBoth, (steps < 2) ? 2 : steps);
	_backgroundBitmap = bitmap;
}

void SpriteLibrary_v1_2_3::spriteBoxOutBackground(byte *bitmap, uint16 steps, bool insetBoth) {
	if (!bitmap)
		return;

	_backgroundBitmap = bitmap;
	_wipeExtentX = (uint16)(_backupRect.right - _backupRect.left);
	_wipeExtentY = (uint16)(_backupRect.bottom - _backupRect.top);
	_wipeDrawFromNew = false;
	wipeBoxToCenter(insetBoth, (steps < 2) ? 2 : steps);
	_backgroundBitmap = bitmap;
}

void SpriteLibrary_v1_2_3::computeWipeStepRect(bool insetBoth, uint16 steps, uint16 step, Common::Rect &out) {
	uint32 divisor = (uint16)(steps * 2 - 1);

	if (insetBoth)
		out.left = (int16)((uint32)step * _wipeExtentX / divisor);
	else
		out.left = 0;

	out.right = (int16)(_wipeExtentX - out.left);
	out.top = (int16)((uint32)step * _wipeExtentY / divisor);
	out.bottom = (int16)(_wipeExtentY - out.top);
}

void SpriteLibrary_v1_2_3::wipeBoxFromCenter(bool insetBoth, uint16 steps) {
	Common::Rect inner;
	Common::Rect outer;

	computeWipeStepRect(insetBoth, steps, (uint16)(steps - 1), inner);
	drawWipeStepAndRepaint(inner);

	for (int16 step = (int16)(steps - 2); step >= 0; step--) {
		spriteAnimate();
		computeWipeStepRect(insetBoth, steps, (uint16)step, outer);

		if (insetBoth)
			drawWipeSideStrips(outer, inner);

		drawWipeTopBottomStrips(outer, inner);
		inner = outer;
	}
}

void SpriteLibrary_v1_2_3::wipeBoxToCenter(bool insetBoth, uint16 steps) {
	Common::Rect inner;
	Common::Rect outer;

	computeWipeStepRect(insetBoth, steps, (uint16)(steps - 1), inner);
	computeWipeStepRect(insetBoth, steps, 0, outer);

	for (uint16 step = 1; step < steps; step++) {
		computeWipeStepRect(insetBoth, steps, step, inner);
		drawWipeTopBottomStrips(outer, inner);

		if (insetBoth)
			drawWipeSideStrips(outer, inner);

		spriteAnimate();
		outer = inner;
	}

	drawWipeStepAndRepaint(inner);
}

void SpriteLibrary_v1_2_3::drawWipeStepAndRepaint(Common::Rect rect) {
	if (_wipeDrawFromNew)
		spriteCopyRect(rect, _backgroundBitmap, _wipeNewBitmap);
	else
		fillBitmapByte(_backgroundBitmap, rect.left, rect.top, (uint16)(rect.right - rect.left), (uint16)(rect.bottom - rect.top), 0);

	rect.translate(_scrollPosX, _scrollPosY);
	spritePaintRect(rect, true);
}

void SpriteLibrary_v1_2_3::drawWipeSideStrips(const Common::Rect &outer, const Common::Rect &inner) {
	Common::Rect strip;
	strip.left = outer.left;
	strip.top = inner.top;
	strip.right = inner.left;
	strip.bottom = inner.bottom;
	drawWipeStepAndRepaint(strip);

	strip.left = inner.right;
	strip.right = outer.right;
	drawWipeStepAndRepaint(strip);
}

void SpriteLibrary_v1_2_3::drawWipeTopBottomStrips(const Common::Rect &outer, const Common::Rect &inner) {
	Common::Rect strip;
	strip.left = outer.left;
	strip.top = outer.top;
	strip.right = outer.right;
	strip.bottom = inner.top;
	drawWipeStepAndRepaint(strip);

	strip.top = inner.bottom;
	strip.bottom = outer.bottom;
	drawWipeStepAndRepaint(strip);
}

void SpriteLibrary_v1_2_3::spriteRubberBand(bool enable, int16 x, int16 y, bool move) {
	if (enable) {
		if (!_rubberBandActive)
			rubberBandBegin(x, y);
		else
			rubberBandUpdate(x, y, move);
	} else if (_rubberBandActive) {
		rubberBandEnd();
	}
}

void SpriteLibrary_v1_2::rubberBandBegin(int16 x, int16 y) {
	_rubberBandActive = true;
	_rubberBandAnchorX = x;
	_rubberBandAnchorY = y;
	_rubberBandLastX = x;
	_rubberBandLastY = y;
	_rubberBandRect = Common::Rect(x, y, x, y);
	rubberBandDraw();
}

void SpriteLibrary_v1_2::rubberBandEnd() {
	rubberBandDraw();
	_rubberBandActive = false;
}

void SpriteLibrary_v1_2::rubberBandRepaint(const Common::Rect &rect) {
	Common::Rect common;
	if (intersectRectLocal(rect, _rubberBandRect, common))
		rubberBandDraw();
}

void SpriteLibrary_v1_2::rubberBandUpdate(int16 x, int16 y, bool move) {
	rubberBandDraw();

	if (move) {
		int16 deltaX = (int16)(x - _rubberBandLastX);
		int16 deltaY = (int16)(y - _rubberBandLastY);
		_rubberBandAnchorX = (int16)(_rubberBandAnchorX + deltaX);
		_rubberBandAnchorY = (int16)(_rubberBandAnchorY + deltaY);
		_rubberBandRect.translate(deltaX, deltaY);
	} else {
		_rubberBandRect.left = MIN(_rubberBandAnchorX, x);
		_rubberBandRect.right = MAX(_rubberBandAnchorX, x);
		_rubberBandRect.top = MIN(_rubberBandAnchorY, y);
		_rubberBandRect.bottom = MAX(_rubberBandAnchorY, y);
	}

	_rubberBandLastX = x;
	_rubberBandLastY = y;
	rubberBandDraw();
}

void SpriteLibrary_v1_2::rubberBandDraw() {
	if (_rubberBandRect.right <= _rubberBandRect.left || _rubberBandRect.bottom <= _rubberBandRect.top)
		return;

	Display *display = engine()->display();
	Graphics::Screen *screen = engine()->screen();

	// The pen is the yellow of the system palette on a palette screen and plain yellow on any other screen
	uint32 pen = 0;
	if (screen->format.bytesPerPixel == 1) {
		byte systemColors[TG_DISPLAY_NUM_PALETTE_ENTRIES * TG_DISPLAY_PALETTE_ENTRY_SIZE];
		display->getSystemPaletteEntries(systemColors, 0, TG_DISPLAY_NUM_PALETTE_ENTRIES);

		for (int i = 0; i < TG_DISPLAY_NUM_PALETTE_ENTRIES; i++) {
			const byte *entry = systemColors + i * TG_DISPLAY_PALETTE_ENTRY_SIZE;
			if (entry[0] == 255 && entry[1] == 255 && entry[2] == 0) {
				pen = i;
				break;
			}
		}
	} else {
		pen = screen->format.RGBToColor(255, 255, 0);
	}

	Common::Rect outline = _rubberBandRect;
	outline.translate(display->viewportOrigin().x, display->viewportOrigin().y);

	// The corners belong to the horizontal edges so that every pixel of the outline is inverted once
	Common::Rect edges[4];
	edges[0].left = outline.left;
	edges[0].right = outline.right;
	edges[0].top = outline.top;
	edges[0].bottom = outline.top + 1;

	if (outline.bottom - 1 != outline.top) {
		edges[1].left = outline.left;
		edges[1].right = outline.right;
		edges[1].top = outline.bottom - 1;
		edges[1].bottom = outline.bottom;
	}

	edges[2].left = outline.left;
	edges[2].right = outline.left + 1;
	edges[2].top = outline.top + 1;
	edges[2].bottom = outline.bottom - 1;

	if (outline.right - 1 != outline.left) {
		edges[3].left = outline.right - 1;
		edges[3].right = outline.right;
		edges[3].top = outline.top + 1;
		edges[3].bottom = outline.bottom - 1;
	}

	Common::Rect screenRect(0, 0, display->width(), display->height());
	for (int i = 0; i < 4; i++) {
		Common::Rect edge;
		if (!intersectRectLocal(edges[i], screenRect, edge))
			continue;

		for (int16 row = edge.top; row < edge.bottom; row++) {
			for (int16 column = edge.left; column < edge.right; column++) {
				byte *pixel = (byte *)screen->getBasePtr(column, row);

				if (screen->format.bytesPerPixel == 1)
					*pixel ^= (byte)pen;
				else if (screen->format.bytesPerPixel == 2)
					WRITE_UINT16(pixel, READ_UINT16(pixel) ^ (uint16)pen);
				else
					WRITE_UINT32(pixel, READ_UINT32(pixel) ^ pen);
			}
		}

		screen->addDirtyRect(edge);
	}
}

} // End of namespace Topgun
